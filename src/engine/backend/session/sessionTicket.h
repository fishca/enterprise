#ifndef __IB_SESSION_TICKET_H__
#define __IB_SESSION_TICKET_H__

// ibSessionTicket — RAII owner handle returned by
// `ibSessionRegistry::Connect()`. Move-only, dtor submits a Remove
// request at Urgent priority so the DELETE of the session's row runs
// ahead of any pending Normal-priority Adds.
//
// All producer-facing session operations (Attach creds, Detach, read
// identity / state) go through the ticket. The raw ibSession* is
// reachable via Session() for scripts that still need to read user info
// directly; external code must NOT destroy / drop the session through
// anything other than letting the ticket go out of scope.

#include "backend/backend.h"
#include "session.h"

#include <chrono>
#include <memory>

class ibSessionRegistry;

// Attempt result — returned by Attach / Detach to distinguish transient
// failures (wrong password, allowed to retry on same session) from
// terminal ones (session already stopping, registry fatal).
enum class ibAttachResult : int {
	Ok,             // Auth state == Attached
	WrongPassword,  // Auth state == AuthFailed; retry on the same ticket is legal
	Timeout,        // registry didn't answer within the given window
	RegistryDown,   // registry fatal / stopped; ticket is unusable
};

class BACKEND_API ibSessionTicket {
public:
	// Construct-disengaged ticket (empty). Used as the return value of
	// failed Connect() calls — check IsValid() before Session().
	ibSessionTicket() = default;

	// Produced by ibSessionRegistry::Connect — external code never calls
	// this directly.
	ibSessionTicket(std::shared_ptr<ibSession> session, ibSessionRegistry* registry);

	~ibSessionTicket();

	ibSessionTicket(const ibSessionTicket&)            = delete;
	ibSessionTicket& operator=(const ibSessionTicket&) = delete;

	ibSessionTicket(ibSessionTicket&& other) noexcept;
	ibSessionTicket& operator=(ibSessionTicket&& other) noexcept;

	bool          IsValid()  const { return m_session != nullptr && m_registry != nullptr; }
	ibSession*    Session()  const { return m_session.get(); }
	ibSessionState State()   const { return m_session ? m_session->State() : ibSessionState::Gone; }
	ibAuthState    Auth()    const { return m_session ? m_session->Auth()  : ibAuthState::Anonymous; }
	bool           IsActive() const { return State() == ibSessionState::Added; }

	// Request auth binding. Submits Attach@Normal, blocks on the
	// session's cv until the auth axis moves out of Anonymous (or
	// timeout). Safe to retry after WrongPassword — the session stays
	// Added throughout.
	ibAttachResult Attach(const wxString& user, const wxString& password,
	                      std::chrono::milliseconds timeout = std::chrono::seconds(20));

	// Inverse — drop user identity, session stays Added as Anonymous.
	// Not idempotent with Attach in the sense that it can be called at
	// any time; the handler is a no-op when already Anonymous.
	void Detach(std::chrono::milliseconds timeout = std::chrono::seconds(5));

	// Fire-and-forget activity label update — submits SetActivity@Low
	// so it never preempts real Add / Remove work. Registry handler
	// UPDATEs `sys_session.currentActivity` on the next tick; readers
	// (Active Users, admin endpoint) see the new label on the next
	// snapshot refresh.
	void SetActivity(const wxString& activity);

	// Early release — same effect as letting the ticket go out of scope.
	// After this returns, the ticket is empty; IsValid() reports false.
	void Release();

private:
	void DoRelease();

	std::shared_ptr<ibSession> m_session;
	ibSessionRegistry*         m_registry = nullptr;
};

#endif
