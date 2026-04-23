#ifndef __IB_CONNECTION_SCOPE_H__
#define __IB_CONNECTION_SCOPE_H__

// ibConnectionScope — per-function / per-block RAII that ACQUIRES a
// connection from ibConnectionPool and pins it as the thread-local
// current connection for the duration of the scope. While the scope
// is alive, the `db_query` macro on the same thread resolves to this
// scope's connection; other threads that ask the pool for a fresh
// checkout get a different idle connection.
//
//   bool ibValueRecordDataObjectCatalog::DeleteObject() {
//       ibConnectionScope scope = ibConnectionPool::GetFreeConnection();
//       scope.SafeBeginTransaction();
//       scope->RunQuery(...);                 // or db_query->RunQuery — same conn
//       scope.SafeCommitTransaction();
//   }                                         // ~scope → pool repark
//
// Nested scopes share the parent's connection. Only the outer-most
// scope actually performs a pool Checkout; inner scopes observe the
// thread-local slot, reuse the same connection, and do NOT touch the
// pool at their own ctor / dtor. Nested Safe*Transaction calls collapse
// onto one real driver transaction through the base-class counter
// layer (see databaseLayer.h — nested-safe Begin/Commit/RollBack).
//
// Transaction API is merged into the scope via Safe{Begin,Commit,
// RollBack}Transaction. The dtor rolls back any unresolved Begin so
// an exception between Safe*Begin and Safe*Commit cleans up
// automatically.
//
// Fallback:
//   - If the pool is not initialised (early startup / unit-test
//     harness) or is saturated, the scope is "passive" — it has no
//     connection of its own and does not touch the TL slot. The
//     `db_query` macro then continues to resolve via the process-
//     wide ibApplicationData::m_db, preserving legacy behaviour
//     rather than failing.

#include "backend/backend.h"

#include <memory>

#include "databaseLayer.h"

class BACKEND_API ibConnectionScope
{
public:
	// Acquire a connection. TL-aware: if another scope is already
	// active on this thread, inherit its connection (nested scopes
	// share one pool checkout); otherwise perform a pool Checkout
	// and install this conn as the thread's TL-current.
	//
	// Usage — direct or via the factory on ibConnectionPool:
	//   ibConnectionScope scope;
	//   ibConnectionScope scope = ibConnectionPool::GetFreeConnection();
	//
	// The factory form relies on mandatory RVO / move: the scope is
	// constructed once at the caller's variable location. No
	// intermediate shared_ptr round-trip.
	ibConnectionScope();

	~ibConnectionScope();

	ibConnectionScope(const ibConnectionScope&)            = delete;
	ibConnectionScope& operator=(const ibConnectionScope&) = delete;

	// Move-enabled so the factory `GetFreeConnection() -> scope` form
	// compiles on implementations that don't elide the return. On
	// move the destination inherits the owns-conn flag; the source
	// is left passive (its dtor becomes a no-op).
	ibConnectionScope(ibConnectionScope&& other) noexcept;
	ibConnectionScope& operator=(ibConnectionScope&& other) noexcept;

	// Accessors — scope->Foo() and db_query->Foo() both reach the
	// same driver while the scope is on the TL stack.
	ibDatabaseLayer* operator->() const { return m_conn.get(); }
	ibDatabaseLayer* get()        const { return m_conn.get(); }
	explicit operator bool()      const { return m_conn != nullptr; }

	// Expose the underlying shared_ptr — needed when a consumer
	// takes shared ownership for its own lifetime (code that outlives
	// the scope). Copying the shared_ptr bumps the refcount so the
	// conn stays alive. Prefer `operator->` for simple in-scope
	// driver calls.
	const std::shared_ptr<ibDatabaseLayer>& shared() const { return m_conn; }

	// Transaction API — merged into the scope. The scope tracks
	// whether it has an unresolved Begin; the dtor rolls back any
	// unmatched Begin so an exception between Begin and Commit cleans
	// up automatically. "Safe" prefix distinguishes these from the
	// driver-direct `scope->BeginTransaction()` calls (which bypass
	// the scope's tracking).
	//
	// The actual nested-safe counter lives on ibDatabaseLayer (see
	// databaseLayer.h). Multiple scopes on the same thread all call
	// Safe* on the same shared conn; the counter collapses them onto
	// one real driver-level TX.
	void SafeBeginTransaction(const ibDatabaseLayer::ibTxOptions& opts = {});
	void SafeCommitTransaction();
	void SafeRollBackTransaction();

	// Is this scope holding an unmatched SafeBeginTransaction? Informational.
	bool HasActiveTransaction() const { return m_activeTx; }

	// True if this scope actually owns a pool checkout (i.e. is the
	// outer-most scope on its thread). Inner scopes that inherit
	// their parent's connection return false. Informational — most
	// callers don't need to distinguish.
	bool Owns() const { return m_ownsConn; }

private:
	std::shared_ptr<ibDatabaseLayer> m_prev;      // saved TL slot (restored on dtor)
	std::shared_ptr<ibDatabaseLayer> m_conn;      // checked-out (owner) OR inherited (nested)
	bool                             m_ownsConn;  // true = pool checkout, false = inherited
	bool                             m_activeTx = false;  // unmatched BeginTransaction
};

#endif  // __IB_CONNECTION_SCOPE_H__
