#include "connectionScope.h"

#include "backend/appData.h"
#include "connectionPool.h"

ibConnectionScope::ibConnectionScope()
	: m_ownsConn(false)
{
	// Peek the thread-local current connection. If a parent scope is
	// already active on this thread, inherit its connection — nested
	// scopes share one pool checkout. Otherwise perform a real
	// Checkout so `*this` becomes the outer-most scope.
	m_prev = ibConnectionPool::GetCurrentConnection();

	if (m_prev) {
		// Inherit — no pool round-trip, do not overwrite the TL slot
		// (parent's pointer still matches our m_conn). The dtor will
		// see m_ownsConn == false and leave the TL alone.
		m_conn = m_prev;
		return;
	}

	// No parent scope. If a transaction is already live on this thread
	// (raw `db_query->BeginTransaction()` outside any scope), we MUST
	// use that exact connection — SQL TX are connection-local.
	if (auto tx = ibConnectionPool::GetActiveTxConnection()) {
		m_conn = tx;
	}
	else if (auto* pool = ibApplicationData::GetConnectionPool()) {
		m_conn = pool->Checkout();
	}

	if (m_conn) {
		m_ownsConn = true;
		ibConnectionPool::SetCurrentConnection(m_conn);
	}
	// Else — pool unavailable / saturated; scope stays passive. The
	// db_query macro will resolve via the pool's primary-conn
	// fallback.
}

ibConnectionScope::~ibConnectionScope()
{
	// Safety-net: any unresolved Begin on this scope gets rolled
	// back so an exception between Begin and Commit can't leave a
	// dangling TX on the conn. Swallow driver exceptions here —
	// propagating from a dtor would std::terminate.
	if (m_activeTx && m_conn) {
		try { m_conn->RollBack(); } catch (...) {}
		m_activeTx = false;
	}

	if (m_ownsConn) {
		// Restore what was here before us (typically nullptr for the
		// outer-most scope). m_conn's shared_ptr drop runs the pool's
		// custom deleter and reparks the clone.
		ibConnectionPool::SetCurrentConnection(m_prev);
	}
	// Inherit / passive path: parent still owns the TL slot; nothing
	// to restore. Dropping our m_conn just decrements its refcount;
	// the parent's lease keeps the connection alive and in the TL.
}

ibConnectionScope::ibConnectionScope(ibConnectionScope&& other) noexcept
	: m_prev(std::move(other.m_prev))
	, m_conn(std::move(other.m_conn))
	, m_ownsConn(other.m_ownsConn)
	, m_activeTx(other.m_activeTx)
{
	other.m_ownsConn = false;  // source becomes passive
	other.m_activeTx = false;  // TX responsibility moved to destination
}

ibConnectionScope& ibConnectionScope::operator=(ibConnectionScope&& other) noexcept
{
	if (this != &other) {
		// Roll back our own state first (any unresolved TX, and
		// restore TL if we were the owner) before adopting the
		// source's state.
		if (m_activeTx && m_conn) {
			try { m_conn->RollBack(); } catch (...) {}
		}
		if (m_ownsConn)
			ibConnectionPool::SetCurrentConnection(m_prev);

		m_prev     = std::move(other.m_prev);
		m_conn     = std::move(other.m_conn);
		m_ownsConn = other.m_ownsConn;
		m_activeTx = other.m_activeTx;
		other.m_ownsConn = false;
		other.m_activeTx = false;
	}
	return *this;
}

void ibConnectionScope::SafeBeginTransaction(const ibDatabaseLayer::ibTxOptions& opts)
{
	if (!m_activeTx && m_conn) {
		m_conn->BeginTransaction(opts);
		m_activeTx = true;
	}
}

void ibConnectionScope::SafeCommitTransaction()
{
	if (m_activeTx && m_conn) {
		m_conn->Commit();
		m_activeTx = false;
	}
}

void ibConnectionScope::SafeRollBackTransaction()
{
	if (m_activeTx && m_conn) {
		m_conn->RollBack();
		m_activeTx = false;
	}
}
