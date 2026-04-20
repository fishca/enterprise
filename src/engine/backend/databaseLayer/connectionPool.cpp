#include "connectionPool.h"

#include "databaseLayer.h"

ibConnectionPool::ibConnectionPool() = default;

ibConnectionPool::~ibConnectionPool()
{
	// Call Shutdown defensively in case the owner forgot to. No-op
	// after an explicit Shutdown.
	Shutdown();
}

void ibConnectionPool::Init(ibDatabaseLayer* prototype, std::size_t maxSize)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	// Drop existing idle clones on re-init — close + delete each.
	for (auto* conn : m_idle) {
		if (conn != nullptr) {
			if (conn->IsOpen())
				conn->Close();
			delete conn;
		}
	}
	m_idle.clear();
	m_live      = 0;
	m_prototype = prototype;
	m_maxSize   = maxSize;
	m_shutdown  = false;
	// Wake anyone blocked on Checkout — they'll re-check state and
	// either get a connection or bail.
	m_cv.notify_all();
}

void ibConnectionPool::Shutdown()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	m_shutdown = true;
	for (auto* conn : m_idle) {
		if (conn != nullptr) {
			if (conn->IsOpen())
				conn->Close();
			delete conn;
		}
	}
	m_idle.clear();
	// Outstanding checkouts: when their shared_ptr drops, the deleter
	// routes via OnBorrowerDropped which sees m_shutdown and closes +
	// deletes the clone directly.
	m_live      = 0;
	m_prototype = nullptr;
	m_cv.notify_all();
}

std::shared_ptr<ibDatabaseLayer> ibConnectionPool::Checkout()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	while (true) {
		if (m_shutdown || m_prototype == nullptr)
			return nullptr;

		ibDatabaseLayer* raw = nullptr;
		if (!m_idle.empty()) {
			raw = m_idle.front();
			m_idle.pop_front();
		}
		else if (m_live < m_maxSize) {
			// Lazy allocation — clone on first demand so an unused pool
			// never opens extra DB connections.
			raw = m_prototype->Clone();
			if (raw == nullptr)
				return nullptr;
			++m_live;
		}
		else {
			// Saturated — wait for a Return / Shutdown.
			m_cv.wait(lock);
			continue;
		}

		// Hand out wrapped in a shared_ptr whose deleter re-parks the
		// clone back into the idle queue. Capturing `this` is safe
		// because the pool outlives any borrower in practice (owned
		// by ibApplicationData, torn down in DestroyAppDataEnv after
		// all workers have stopped).
		return std::shared_ptr<ibDatabaseLayer>(
			raw,
			[this](ibDatabaseLayer* p) { OnBorrowerDropped(p); }
		);
	}
}

void ibConnectionPool::Return(std::shared_ptr<ibDatabaseLayer> conn)
{
	// Explicit Return is a convenience — dropping the last shared_ptr
	// reference has the same effect via OnBorrowerDropped. Reset here
	// so the caller doesn't have to assign nullptr separately.
	conn.reset();
}

void ibConnectionPool::OnBorrowerDropped(ibDatabaseLayer* raw)
{
	if (raw == nullptr)
		return;

	std::unique_lock<std::mutex> lock(m_mutex);
	if (m_shutdown) {
		// Pool is dead — close + delete. m_live already zeroed by
		// Shutdown so no accounting needed.
		lock.unlock();
		if (raw->IsOpen())
			raw->Close();
		delete raw;
		return;
	}

	m_idle.push_back(raw);
	m_cv.notify_one();
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
