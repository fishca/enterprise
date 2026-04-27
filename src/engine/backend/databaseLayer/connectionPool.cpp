#include "connectionPool.h"

#include "backend/appData.h"
#include "databaseLayer.h"

// --- Thread-local connection slots -----------------------------------------
//
// Per-thread views of the pool:
//   s_tlCurrent — set by ibConnectionScope for its lifetime.
//   s_tlActiveTx — set by ibDatabaseLayer::BeginTransaction when depth
//                  goes 0→1; cleared on the matching Commit/RollBack.
// The active-TX slot holds a shared_ptr tied to the pool's control
// block (via enable_shared_from_this), so the layer stays alive even
// if the hand-out that originally checked it out has been dropped.
namespace {
	thread_local std::shared_ptr<ibDatabaseLayer> s_tlCurrent;
	thread_local std::shared_ptr<ibDatabaseLayer> s_tlActiveTx;
}

std::shared_ptr<ibDatabaseLayer> ibConnectionPool::GetCurrentConnection()
{
	return s_tlCurrent;
}

void ibConnectionPool::SetCurrentConnection(std::shared_ptr<ibDatabaseLayer> conn)
{
	s_tlCurrent = std::move(conn);
}

std::shared_ptr<ibDatabaseLayer> ibConnectionPool::GetActiveTxConnection()
{
	return s_tlActiveTx;
}

void ibConnectionPool::SetActiveTxConnection(std::shared_ptr<ibDatabaseLayer> conn)
{
	s_tlActiveTx = std::move(conn);
}

std::shared_ptr<ibDatabaseLayer> ibConnectionPool::GetPrimaryConnection()
{
	auto* pool = ibApplicationData::GetConnectionPool();
	return pool != nullptr ? pool->m_source : nullptr;
}

std::shared_ptr<ibDatabaseLayer> ibConnectionPool::GetDatabaseLayer()
{
	if (auto txConn = GetActiveTxConnection())
		return txConn;
	if (auto scopeConn = GetCurrentConnection())
		return scopeConn;
	return GetPrimaryConnection();
}

ibConnectionScope ibConnectionPool::GetFreeConnection()
{
	// The full priority chain (TX > scope TL > Checkout) is
	// implemented inside ibConnectionScope's default ctor. Returning
	// the scope directly lets callers write the natural RAII form
	// `ibConnectionScope scope = GetFreeConnection()` without an
	// intermediate shared_ptr round-trip. Mandatory copy elision
	// in C++17 means no move actually happens for prvalue returns.
	return ibConnectionScope();
}

ibConnectionPool::ibConnectionPool() = default;

ibConnectionPool::~ibConnectionPool()
{
	// Call Shutdown defensively in case the owner forgot to. No-op
	// after an explicit Shutdown.
	Shutdown();
}

void ibConnectionPool::Init(std::shared_ptr<ibDatabaseLayer> primary, std::size_t maxSize, std::size_t minIdle)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	// Drop existing idle entries on re-init — their shared_ptrs
	// release, layers that no other handout captured are destructed.
	m_idle.clear();
	m_source   = primary;
	m_maxSize  = maxSize;
	m_minIdle  = minIdle;
	// Master counts as the first live conn. It's also the first idle
	// entry so the earliest Checkout hands it out directly rather
	// than paying a Clone() cost.
	m_live     = primary ? 1 : 0;
	const auto now = std::chrono::steady_clock::now();
	if (primary)
		m_idle.push_back({ primary, now });
	// Pre-warm — Clone() up to minIdle-1 additional conns so light
	// bursts don't pay the Open cost. Server modes (wes, future
	// oes-server) pass minIdle high enough to absorb typical
	// concurrency; single-session GUI hosts use minIdle=1, which
	// means the loop below is a no-op.
	for (std::size_t i = m_live; i < m_minIdle && m_live < m_maxSize; ++i) {
		ibDatabaseLayer* raw = primary ? primary->Clone() : nullptr;
		if (raw == nullptr) break;
		m_idle.push_back({ std::shared_ptr<ibDatabaseLayer>(raw), now });
		++m_live;
	}
	m_shutdown = false;
	// Wake anyone blocked on Checkout — they'll re-check state and
	// either get a connection or bail.
	m_cv.notify_all();
}

void ibConnectionPool::Shutdown()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	m_shutdown = true;

	// Explicitly Close() every idle conn before dropping our
	// shared_ptrs — drivers don't always Close in their dtor (some
	// only release native handles on explicit Close). Outstanding
	// hand-outs are a separate concern: their lambda-captured ref
	// keeps the layer alive until the caller drops it; the deleter
	// then checks m_shutdown and releases without re-parking.
	for (auto& entry : m_idle) {
		if (entry.conn && entry.conn->IsOpen())
			entry.conn->Close();
	}
	m_idle.clear();

	if (m_source && m_source->IsOpen())
		m_source->Close();
	m_source.reset();

	m_live = 0;
	m_cv.notify_all();
}

std::shared_ptr<ibDatabaseLayer> ibConnectionPool::Checkout()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	while (true) {
		if (m_shutdown || !m_source)
			return nullptr;

		// Reap stale idle entries before serving — keeps the pool from
		// holding onto idle conns that haven't been touched in a while
		// once load drops back below minIdle.
		ReapStaleLocked();

		if (!m_idle.empty()) {
			// Hand out the most recently parked entry — it's the
			// hottest (least likely to have stale driver state) and
			// keeps the front-of-queue old entries available for
			// future Reap.
			auto entry = m_idle.back();
			m_idle.pop_back();
			return WrapHandout(std::move(entry.conn));
		}
		if (m_live < m_maxSize) {
			// Lazy Clone — only on first demand for this slot. m_source
			// stays alive (separate member) so Clone() has a live
			// subject to copy from even when every previously-cloned
			// conn is currently checked out.
			ibDatabaseLayer* raw = m_source->Clone();
			if (raw == nullptr)
				return nullptr;
			// This is the FIRST shared_ptr wrapping `raw`. It is the
			// shared_ptr that initialises the weak_ptr inside
			// std::enable_shared_from_this on the layer, so subsequent
			// `shared_from_this()` calls route through this control
			// block for the lifetime of the pool.
			auto sp = std::shared_ptr<ibDatabaseLayer>(raw);
			++m_live;
			return WrapHandout(std::move(sp));
		}
		// Saturated — wait for a Return / Shutdown.
		m_cv.wait(lock);
	}
}

void ibConnectionPool::ReapStaleLocked()
{
	const auto now = std::chrono::steady_clock::now();
	while (m_idle.size() > m_minIdle) {
		// Oldest sits at the front (push_back on park). Pin the master
		// — never drop the conn that equals m_source even if it's
		// front-most, since it's the fallback for legacy db_query.
		auto& front = m_idle.front();
		if (front.conn == m_source) {
			// Master is at the head and we'd drop it — reshuffle: move
			// it to the back so subsequent Reap sees a non-master at
			// the front. Mark its lastUsed = now so it doesn't get
			// re-considered stale next call.
			front.lastUsed = now;
			m_idle.push_back(std::move(front));
			m_idle.pop_front();
			continue;
		}
		if (now - front.lastUsed < kIdleTimeout)
			break;   // freshest non-master is still warm; everything behind is even warmer
		if (front.conn && front.conn->IsOpen())
			front.conn->Close();
		m_idle.pop_front();
		--m_live;
	}
}

void ibConnectionPool::Return(std::shared_ptr<ibDatabaseLayer> conn)
{
	// Explicit Return is a convenience — dropping the last shared_ptr
	// reference has the same effect via the hand-out's deleter. Reset
	// here so the caller doesn't have to assign nullptr separately.
	conn.reset();
}


std::shared_ptr<ibDatabaseLayer> ibConnectionPool::WrapHandout(
	std::shared_ptr<ibDatabaseLayer> sp)
{
	// Hand-out shared_ptr has its OWN control block (constructed here
	// via the (raw, deleter) ctor). When the caller drops the last
	// ref, the deleter runs: parks the pool-owned `sp` back on the
	// idle queue (or releases it silently if the pool is shutting
	// down). The raw pointer is NEVER deleted by the hand-out — the
	// layer's actual destruction is driven by the pool-owned
	// shared_ptr in `sp` / m_idle.
	ibDatabaseLayer* raw = sp.get();
	return std::shared_ptr<ibDatabaseLayer>(
		raw,
		[this, sp = std::move(sp)](ibDatabaseLayer*) mutable {
			std::lock_guard<std::mutex> lock(m_mutex);
			if (!m_shutdown) {
				m_idle.push_back({ std::move(sp), std::chrono::steady_clock::now() });
				m_cv.notify_one();
			}
			// else: sp released by lambda dtor — layer destructed if
			// it was the last ref, nothing to park into.
		}
	);
}

std::size_t ibConnectionPool::LiveSize() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_live;
}

std::size_t ibConnectionPool::IdleSize() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_idle.size();
}
