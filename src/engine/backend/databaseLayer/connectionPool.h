#ifndef __IB_CONNECTION_POOL_H__
#define __IB_CONNECTION_POOL_H__

// ibConnectionPool — bounded pool of ibDatabaseLayer connections.
//
// The pool owns the master connection (passed to Init as a shared_ptr)
// plus up to maxSize-1 clones created lazily on demand. Every check-out
// hands the caller a shared_ptr<ibDatabaseLayer> with a custom deleter
// that does NOT actually destroy the layer — instead it parks the
// pool's long-lived shared_ptr back on the idle queue. Because the
// layer object is always reachable through at least one pool-held
// shared_ptr, the first-shared_ptr-wins rule of
// std::enable_shared_from_this is satisfied once and stays satisfied:
// callers inside the layer can rely on `shared_from_this()` for the
// lifetime of the pool.
//
// Lifecycle:
//   Init(primary, maxSize)    — store the already-opened master as
//                               m_source (used for Clone) and push a
//                               copy into m_idle so the first Checkout
//                               pops it. No extra clones yet — lazy.
//   Checkout()                — return an idle clone. Clones the
//                               master via ibDatabaseLayer::Clone() on
//                               first demand, up to maxSize. Blocks
//                               briefly until one is available once the
//                               pool is saturated.
//   Return(conn)              — drop the caller's shared_ptr (its
//                               deleter parks the pool's ref). Does
//                               NOT Close() — the connection stays
//                               open for the next checkout.
//   Shutdown()                — close + drop every connection the pool
//                               holds. Called from
//                               ibApplicationData::Disconnect.

#include "backend/backend.h"

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>

#include "connectionScope.h"

class ibDatabaseLayer;

class BACKEND_API ibConnectionPool {
public:
	ibConnectionPool();
	~ibConnectionPool();

	ibConnectionPool(const ibConnectionPool&) = delete;
	ibConnectionPool& operator=(const ibConnectionPool&) = delete;

	// Initialise. `primary` is the already-opened master connection —
	// the pool takes shared ownership and also uses it as the first
	// hand-out-able conn. `maxSize` caps total live conns; once
	// reached, Checkout blocks until a prior borrower releases.
	//
	// Re-initialising replaces m_source and resets the idle set. Any
	// previously handed-out shared_ptrs stay valid (their deleter
	// captures their original pool's shared_ptr and parks it back on
	// drop, but into THIS fresh state — harmless; the new pool just
	// sees an extra idle entry).
	void Init(std::shared_ptr<ibDatabaseLayer> primary, std::size_t maxSize);

	// Close and drop every connection the pool holds. Idle and
	// outstanding checkouts are both invalidated — callers must be
	// stopped before this is called. Idempotent.
	void Shutdown();

	// Borrow a connection from this pool instance. Blocks if all
	// clones are checked out and the pool is at maxSize. Returns
	// nullptr if Shutdown has been called or Init was never called.
	//
	// Does NOT touch any thread-local slot — callers receive a raw
	// shared_ptr they own exclusively. Used by consumers that
	// legitimately need multiple parallel conns on one thread and
	// therefore can't go through ibConnectionScope (e.g.
	// ibSessionRegistry holds three persistent checkouts — lock /
	// write / probe). Prefer ibConnectionScope::GetFreeConnection
	// for normal code; reach for Checkout only when you explicitly
	// want a conn independent of the thread's TL state.
	std::shared_ptr<ibDatabaseLayer> Checkout();

	// Acquire a connection scope for the current thread's work.
	// Returns a fully-constructed ibConnectionScope by value — the
	// scope's ctor handles the priority chain internally:
	//   1. Inherit parent scope's conn if one is active on this
	//      thread.
	//   2. Else adopt the thread's active-TX conn if any (raw
	//      db_query->BeginTransaction() outside a scope still pins
	//      the thread to its TX conn).
	//   3. Else Checkout a fresh clone from the pool.
	//
	// Typical usage:
	//   ibConnectionScope scope = ibConnectionPool::GetFreeConnection();
	//
	// The returned scope is move-constructed (or elided) into the
	// caller's variable. Its lifetime governs the thread's TL slot;
	// on scope dtor the TL is restored.
	static ibConnectionScope GetFreeConnection();

	// Primary connection accessor — the master that the pool Clone()s
	// from. Also serves as the fallback used by GetDatabaseLayer
	// when no thread-local connection is active on the calling
	// thread. Returns nullptr when the pool is not initialised.
	static std::shared_ptr<ibDatabaseLayer> GetPrimaryConnection();

	// The `db_query` entry point. Returns the connection the current
	// thread should use, following the priority chain:
	//   1. Active transaction on this thread (TX pinning).
	//   2. Active ibConnectionScope on this thread (scope TL).
	//   3. Primary / master conn (legacy fallback).
	// Returns nullptr only when the pool is not initialised.
	static std::shared_ptr<ibDatabaseLayer> GetDatabaseLayer();

	// Thread-local active-transaction connection accessors — set by
	// ibDatabaseLayer::BeginTransaction (depth 0→1), cleared by the
	// matching Commit/RollBack (depth 1→0). While set, every
	// db_query access on the thread must route to this conn so the
	// TX stays connection-local. Stored shared_ptr holds the layer
	// alive through the pool's control block (via
	// enable_shared_from_this), pinning the TX to this thread even
	// when the scope that started it has been dropped.
	static std::shared_ptr<ibDatabaseLayer> GetActiveTxConnection();
	static void SetActiveTxConnection(std::shared_ptr<ibDatabaseLayer> conn);

	std::size_t MaxSize()    const { return m_maxSize; }
	std::size_t LiveSize()   const;
	std::size_t IdleSize()   const;

	// Return a previously-checked-out connection. Equivalent to
	// letting the caller's shared_ptr go out of scope — kept for the
	// symmetric "Return" wording; most callers just drop the
	// shared_ptr and let its deleter re-park.
	void Return(std::shared_ptr<ibDatabaseLayer> conn);

private:
	// ibConnectionScope is the sole consumer of the per-thread
	// current-connection slot — its ctor / dtor push-save-restore
	// the slot and need direct access to it.
	friend class ibConnectionScope;

	// Raw TL-slot accessors for the scope-owned "current" conn. Not
	// public: callers should acquire conns via GetFreeConnection
	// (RAII scope) rather than poking the slot directly.
	static std::shared_ptr<ibDatabaseLayer> GetCurrentConnection();
	static void SetCurrentConnection(std::shared_ptr<ibDatabaseLayer> conn);

	// Wrap a pool-owned shared_ptr as a hand-out for a caller. The
	// returned shared_ptr has its own control block whose custom
	// deleter parks the pool's shared_ptr back on the idle queue when
	// the caller drops it. The lambda captures `sp` by value so the
	// pool-owned layer stays alive for the hand-out's whole life.
	std::shared_ptr<ibDatabaseLayer> WrapHandout(
		std::shared_ptr<ibDatabaseLayer> sp);

	mutable std::mutex              m_mutex;
	std::condition_variable         m_cv;

	// Master connection. Always kept alive by the pool so Clone() has
	// a live source even when every clone is currently checked out.
	std::shared_ptr<ibDatabaseLayer>              m_source;

	// Idle clones — owning shared_ptrs. Checkout pops the front,
	// hand-out's deleter pushes back.
	std::deque<std::shared_ptr<ibDatabaseLayer>>  m_idle;

	std::size_t                     m_maxSize   = 0;
	std::size_t                     m_live      = 0;  // total clones created (idle + checked-out)
	bool                            m_shutdown  = false;
};

// The canonical per-function RAII scope that ALSO installs the
// connection as the thread-local current for the duration of the
// scope (so `db_query->...` inside the scope routes to it) lives in
// connectionScope.h — ibConnectionScope. The earlier low-level
// pool-only RAII previously declared here was redundant with
// shared_ptr's pool-deleter and had no call sites.

#endif  // __IB_CONNECTION_POOL_H__
