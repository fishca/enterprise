#ifndef __IB_DATABASE_CONNECTION_HOLDER_H__
#define __IB_DATABASE_CONNECTION_HOLDER_H__

// ibDatabaseConnectionHolder — identity tag for "something that can
// reserve a database connection across thread boundaries".
//
// The runtime holder today is ibSession; the pool keys its
// active-transaction reservation map on this base pointer so the
// pool / databaseLayer don't pull in session.h, and so future non-
// session holders (compute-server batch runner, daemon job scope)
// can plug in without growing ibSession.
//
// Reservation storage lives in ibConnectionPool — see
// Reserve/Release/GetReserved on the pool. The layer caches a
// weak_ptr to the holder that owns its current TX so a layer
// returned via shared_from_this() can be cross-checked against
// the pool's map.

#include "backend/backend.h"

#include <memory>

class ibDatabaseLayer;

class BACKEND_API ibDatabaseConnectionHolder {
public:
	virtual ~ibDatabaseConnectionHolder() = default;

	// Conn currently reserved or scope-bound to this holder. TX pin
	// takes priority over scope binding (the active TX must always
	// route to the same conn). Returns nullptr if the holder has
	// neither reservation in flight. Resolves via ibConnectionPool
	// internally so callers don't need pool plumbing at the use
	// site:
	//
	//   if (auto conn = ibSession::Current()->GetConnection()) {
	//       conn->RunQuery(...);
	//   }
	//
	// Defined out-of-line in connectionPool.cpp where the pool's
	// header is in scope.
	std::shared_ptr<ibDatabaseLayer> GetConnection() const;

	// Fresh conn from the pool — wrapped Checkout, NOT bound to this
	// holder. The returned shared_ptr's deleter releases the entry
	// back to the pool when the caller drops the last reference. Use
	// for work that must run on a separate conn from this holder's
	// current TX/scope (e.g. session running parallel side queries
	// while its main TX stays open):
	//
	//   auto fresh = ibSession::Current()->AcquireFreeConnection();
	//   fresh->RunQuery(...);                  // independent of session's TX
	//   // fresh drops at scope end → pool repark
	//
	// Returns nullptr if the pool is saturated / not initialised.
	std::shared_ptr<ibDatabaseLayer> AcquireFreeConnection() const;
};

// ibSingleConnectionHolder — generic empty holder. The OES runtime
// uses a single process-wide instance as the db_query channel
// (ibConnectionPool::DbQueryHolder); subsystems that need an
// alternate non-session channel (parallel isolation, dedicated
// quota) instantiate their own static instance and pass its address
// to ibConnectionScope(&customHolder).
//
// Self-cleaning: the dtor releases any pool registrations (TX pin
// and scope binding) keyed on this holder. Process-shutdown teardown
// of the singleton runs after the pool is gone, so the dtor's
// release is a no-op then.
class BACKEND_API ibSingleConnectionHolder : public ibDatabaseConnectionHolder {
public:
	ibSingleConnectionHolder();
	~ibSingleConnectionHolder() override;
};

#endif
