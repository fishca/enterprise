#ifndef __IB_CONNECTION_POOL_H__
#define __IB_CONNECTION_POOL_H__

// ibConnectionPool — bounded pool of ibDatabaseLayer connections.
//
// The primary ibDatabaseLayer* is still held on ibApplicationData
// (returned by the db_query macro) and is what the main thread uses for
// its own queries. The pool is a separate set of *additional* clones
// handed out to worker threads that need an independent connection: the
// per-session heartbeat writer, SSE streams, future per-session workers.
//
// Lifecycle:
//   Init(prototype, maxSize)  — stash the prototype, no connections
//                               allocated yet (lazy).
//   Checkout()                — return an idle connection; clones the
//                               prototype via ibDatabaseLayer::Clone()
//                               on first demand, up to maxSize. Blocks
//                               briefly until one is available once the
//                               pool is saturated.
//   Return(conn)              — put a borrowed connection back into the
//                               idle set. Does NOT Close() — the
//                               connection stays open for the next
//                               checkout.
//   Shutdown()                — close + drop every connection the pool
//                               ever allocated. Called from
//                               ibApplicationData::Disconnect.
//
// RAII helper ibConnectionScope wraps the checkout/return pair so
// worker code can't leak a borrowed connection on early return /
// exception.
//
// Phase 4 (this file): additive. No existing call sites are rewired;
// db_query macro stays pointing at the single shared connection. Later
// commits route the heartbeat writer, per-session SSE publishers and
// ibWebSession worker threads through the pool.

#include "backend/backend.h"

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>

class ibDatabaseLayer;

class BACKEND_API ibConnectionPool {
public:
	ibConnectionPool();
	~ibConnectionPool();

	ibConnectionPool(const ibConnectionPool&) = delete;
	ibConnectionPool& operator=(const ibConnectionPool&) = delete;

	// Initialise. `prototype` is the already-opened master connection
	// (typically ibApplicationData::m_db). `maxSize` caps the total
	// number of live clones the pool will ever hand out; once reached,
	// Checkout blocks until a prior borrower Returns.
	//
	// Safe to call Init twice: it replaces the prototype and resets the
	// idle set, but any checkouts issued against the previous prototype
	// stay valid (their destruction still routes through this instance
	// via their original caller's Return / ibConnectionScope).
	void Init(ibDatabaseLayer* prototype, std::size_t maxSize);

	// Close and drop every clone the pool still holds. Idle and
	// outstanding checkouts are both invalidated — callers must be
	// stopped before this is called. Idempotent.
	void Shutdown();

	// Borrow a connection. Blocks if all clones are checked out and
	// the pool is at maxSize. Returns nullptr if Shutdown has been
	// called or Init was never called.
	std::shared_ptr<ibDatabaseLayer> Checkout();

	// Return a previously-checked-out connection to the idle set.
	// No-op on nullptr. If the returned connection was not issued by
	// this pool, the behaviour is undefined — always pair each
	// Checkout with exactly one Return (or use ibConnectionScope).
	void Return(std::shared_ptr<ibDatabaseLayer> conn);

	std::size_t MaxSize()    const { return m_maxSize; }
	std::size_t LiveSize()   const;
	std::size_t IdleSize()   const;

private:
	// Called by the shared_ptr custom deleter when a checked-out
	// connection is dropped by its last owner. Re-parks it on the
	// idle list (or closes+deletes it if the pool is shutting down).
	void OnBorrowerDropped(ibDatabaseLayer* raw);

	mutable std::mutex              m_mutex;
	std::condition_variable         m_cv;
	ibDatabaseLayer*                m_prototype = nullptr;  // borrowed; not owned
	std::size_t                     m_maxSize   = 0;
	std::size_t                     m_live      = 0;         // total clones created (idle + checked-out)
	// Idle clones stored as raw — pool owns them. Checkout hands one
	// out wrapped in a shared_ptr whose custom deleter calls
	// OnBorrowerDropped(raw) so the clone is reparked on drop.
	std::deque<ibDatabaseLayer*>    m_idle;
	bool                            m_shutdown  = false;
};

// RAII guard for a pool checkout.
//
//   {
//       ibConnectionScope scope(pool);
//       if (!scope) return false;
//       scope->RunQuery(...);
//   }   // returns to pool automatically
//
// Move-only; use std::move to hand the borrow across function
// boundaries when needed.
class BACKEND_API ibConnectionScope {
public:
	explicit ibConnectionScope(ibConnectionPool& pool)
		: m_pool(&pool), m_conn(pool.Checkout()) {}

	~ibConnectionScope() { reset(); }

	ibConnectionScope(const ibConnectionScope&)            = delete;
	ibConnectionScope& operator=(const ibConnectionScope&) = delete;

	ibConnectionScope(ibConnectionScope&& other) noexcept
		: m_pool(other.m_pool), m_conn(std::move(other.m_conn))
	{
		other.m_pool = nullptr;
	}

	ibConnectionScope& operator=(ibConnectionScope&& other) noexcept
	{
		if (this != &other) {
			reset();
			m_pool = other.m_pool;
			m_conn = std::move(other.m_conn);
			other.m_pool = nullptr;
		}
		return *this;
	}

	ibDatabaseLayer* get()        const { return m_conn.get(); }
	ibDatabaseLayer* operator->() const { return m_conn.get(); }
	explicit operator bool()      const { return m_conn != nullptr; }

	// Release the checkout early without destroying the scope object.
	// Rarely needed — scope dtor is the normal path.
	void reset()
	{
		if (m_pool != nullptr && m_conn != nullptr)
			m_pool->Return(std::move(m_conn));
		m_conn.reset();
		m_pool = nullptr;
	}

private:
	ibConnectionPool*                m_pool;
	std::shared_ptr<ibDatabaseLayer> m_conn;
};

#endif  // __IB_CONNECTION_POOL_H__
