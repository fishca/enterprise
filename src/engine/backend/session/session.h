#ifndef __IB_SESSION_H__
#define __IB_SESSION_H__

// ibSession — per-session record owned by ibSessionRegistry. Identity
// (guid, user, computer, app mode, started) + runtime state machine
// (lifecycle / auth) + script bindings (module manager, ProcUnit map).
//
// Renamed from ibSessionContext as part of the session-registry
// refactor. SessionScope / Current() stay available as legacy shims
// during migration — direct ibSession pointer passing (via ibProcUnit
// etc.) is the target, thread_local Current() is deprecated.

#include "backend/backend.h"
#include "backend/userInfo.h"
#include "backend/appData.h"   // ibRunMode

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <wx/datetime.h>
#include <wx/string.h>

class ibValueModuleManager;
class ibModuleDataObject;
class ibProcUnit;

// ------------------------------------------------------------------
// ibSessionState — lifecycle axis. Advances strictly forward through
// Created → Added → Stopping → Gone, with Rejected as a terminal
// branch out of Created. Auth axis lives in ibAuthState separately.
// ------------------------------------------------------------------
enum class ibSessionState : int {
	Created   = 0,  // object constructed, not yet submitted to registry
	Added     = 1,  // policies passed, row in sys_session, lock held
	Rejected  = 2,  // terminal: policy veto / DbError / duplicate guid
	Stopping  = 3,  // ~ticket or Kick in progress
	Gone      = 4,  // row DELETED, lock released, safe to destroy
};

// ------------------------------------------------------------------
// ibAuthState — auth axis, independent of lifecycle. A session can be
// Anonymous (anonymous phase while the login dialog / cookie is open,
// or technical session that never attaches a user) and switch to
// Authenticated via Attach. AuthFailed is non-terminal — retry is
// legal within the same session.
// ------------------------------------------------------------------
enum class ibAuthState : int {
	Anonymous     = 0,
	Authenticated = 1,
	AuthFailed    = 2,  // retry allowed; session still Added
};

// ------------------------------------------------------------------
// ibSessionKind — sessions-layer enum. Shares numeric values with
// ibRunMode for the 1:1 cases (Launcher/Designer/Enterprise/Service)
// so casts round-trip; splits the web case into two distinct session
// roles that both share ibRunMode::eWEB_ENTERPRISE_MODE as the host
// process's run mode. Physically only the wes process runs — inside
// it sessions come in two flavours:
//   WebServer  — the process's own technical sys_session row
//   WebClient  — per-tab / per-API-caller connections
// Desktop binaries populate their corresponding session kind directly;
// SessionKindFromRunMode is the default for unambiguous cases and
// returns WebClient for eWEB_ENTERPRISE_MODE (the common per-tab case).
// ------------------------------------------------------------------
enum class ibSessionKind : int {
	Launcher   = eLAUNCHER_MODE,       // 1
	Designer   = eDESIGNER_MODE,       // 2
	Enterprise = eENTERPRISE_MODE,     // 3
	Service    = eSERVICE_MODE,        // 4
	WebServer  = eWEB_ENTERPRISE_MODE, // 5 — wes process technical row
	WebClient  = 100,                  // per-tab / API caller
};

inline ibSessionKind SessionKindFromRunMode(ibRunMode m) {
	// Web run mode is ambiguous at this layer — default to WebClient
	// (the per-tab common case). Callers that need WebServer set the
	// kind explicitly (see ibApplicationData::StartSession).
	if (m == eWEB_ENTERPRISE_MODE) return ibSessionKind::WebClient;
	return static_cast<ibSessionKind>(m);
}

// ------------------------------------------------------------------
// ibPriority — request priority for ibSessionRegistry's single-consumer
// queue. Strict descending: all Urgent drained before any Normal, etc.
// Default for Submit is Normal; callers escalate explicitly when
// needed (Remove/Kick/Stop get Urgent; SetActivity gets Low).
// ------------------------------------------------------------------
enum class ibPriority : int {
	Urgent     = 0,
	Normal     = 1,
	Low        = 2,
	Background = 3,
};

// ------------------------------------------------------------------
// ibSessionIdentity — immutable (after Add) descriptor of who/what
// this session represents. Populated by the producer at Connect time;
// the registry never mutates it except through explicit Attach (which
// fills user fields).
// ------------------------------------------------------------------
struct BACKEND_API ibSessionIdentity {
	ibGuid       m_guid;              // PK in sys_session.session
	wxString     m_userName;          // empty = anonymous phase / technical
	wxString     m_userGuid;           // sys_user row, empty before Attach
	wxString     m_computer;          // hostname
	wxString     m_address;            // "host:port" for web; "" for desktop
	ibRunMode    m_appMode;            // eENTERPRISE / eDESIGNER / eWEB_ENTERPRISE / ...
	wxDateTime   m_started;
	int          m_pid = 0;            // OS pid — for kick / attach debugger
	bool         m_expectsAnonPhase = true;  // true: INSERT on Add; false: INSERT deferred to Attach success
};

// ------------------------------------------------------------------
// ibSession — the record itself.
// Fields split into three groups:
//   - identity: immutable post-Add (except userInfo via Attach)
//   - state: atomic axes + activity string under mutex
//   - runtime: per-session ProcUnit map + module manager + raw password
//
// Single-consumer registry thread reads/writes identity+runtime
// freely; producers interact via the registry queue. State transitions
// are notified via m_cv so WaitState() can block producer threads
// safely.
// ------------------------------------------------------------------
class BACKEND_API ibSession {
public:
	// kind = what this session does in the running process. The process-
	// level run mode (how the exe was launched) lives on appData and is
	// the same for every session inside a process — not duplicated here.
	ibSession(std::string id, ibSessionKind kind);
	~ibSession();

	ibSession(const ibSession&)            = delete;
	ibSession& operator=(const ibSession&) = delete;

	// Legacy id (string representation of m_guid or external cookie).
	// New code should prefer m_identity.m_guid directly.
	const std::string& GetId()   const { return m_id; }
	ibSessionKind      GetKind() const { return m_kind; }

	ibValueModuleManager* GetModuleManager() const { return m_moduleManager; }
	void                  SetModuleManager(ibValueModuleManager* mm) { m_moduleManager = mm; }

	ibApplicationDataUserInfo&       GetUserInfo()       { return m_userInfo; }
	const ibApplicationDataUserInfo& GetUserInfo() const { return m_userInfo; }
	void SetUserInfo(const ibApplicationDataUserInfo& info) { m_userInfo = info; }

	const ibGuid& GetSessionGuid() const { return m_sessionGuid; }
	void          SetSessionGuid(const ibGuid& guid) { m_sessionGuid = guid; }

	const wxString& GetSessionRawPassword() const { return m_sessionRawPassword; }
	void            SetSessionRawPassword(const wxString& pwd) { m_sessionRawPassword = pwd; }
	void            ClearSessionRawPassword() { m_sessionRawPassword.clear(); }

	ibProcUnit* GetProcUnitFor(const ibModuleDataObject* descriptor) const;
	void        AttachProcUnit(const ibModuleDataObject* descriptor, std::shared_ptr<ibProcUnit> pu);
	void        DetachProcUnit(const ibModuleDataObject* descriptor);

	// State accessors — lock-free reads.
	ibSessionState State() const { return m_state.load(std::memory_order_acquire); }
	ibAuthState    Auth()  const { return m_auth.load(std::memory_order_acquire); }

	const ibSessionIdentity& Identity() const { return m_identity; }
	void                     SetIdentity(const ibSessionIdentity& id) { m_identity = id; }

	// Has the registry INSERT'd the sys_session row for this session?
	// Written only by the registry thread; read by ProcessAttach (to
	// decide INSERT vs UPDATE — deferred-INSERT path when creds were
	// supplied at Connect time) and by ProcessRemove (to skip DELETE
	// when no row was ever created). No mutex — registry thread is the
	// only writer and the only reader outside that thread is for
	// diagnostics, where a torn read is harmless.
	bool Inserted()      const { return m_inserted; }
	void SetInserted(bool v)   { m_inserted = v; }

	// Diagnostic string set by the registry thread on Rejected / AuthFailed
	// transitions. Read after State() / Auth() changes to report the
	// reason to producers. Returned by value to avoid exposing the mutex.
	wxString Reason() const;

	// Registry-thread-only mutators. Take the session's mutex, store the
	// new state, and cv.notify_all so producers waiting on WaitForState
	// wake up. Declared public so the registry (a different class) can
	// call them; call sites outside the registry thread are bugs.
	void Transition(ibSessionState next, const wxString& reason = wxEmptyString);
	void TransitionAuth(ibAuthState   next, const wxString& reason = wxEmptyString);

	// Block the calling thread until the lifecycle state changes away
	// from `from` (or timeout). Returns the new state on success, `from`
	// on timeout. Producers use this to wait for Add/Attach to settle.
	ibSessionState WaitForState(ibSessionState from, std::chrono::milliseconds timeout);

	// Block the calling thread until the auth state changes away from
	// `from` (or timeout). Same contract as WaitForState but on the auth
	// axis.
	ibAuthState    WaitForAuth(ibAuthState from, std::chrono::milliseconds timeout);

	// Legacy current-session-on-thread accessor. Deprecated; explicit
	// pointer passing (ibProcUnit::GetSession etc.) is the target.
	static ibSession* Current();

private:
	friend class SessionScope;
	friend class ibSessionRegistry;

	std::string    m_id;
	ibSessionKind  m_kind;

	ibValueModuleManager* m_moduleManager = nullptr;

	std::unordered_map<const ibModuleDataObject*, std::shared_ptr<ibProcUnit>> m_procUnitMap;
	// Guards m_procUnitMap. mutable so const accessors (GetProcUnitFor)
	// can lock too.
	mutable std::mutex m_procUnitMtx;

	// Identity fields — populated progressively as the session moves
	// through Add → Attach. Registry thread is the sole writer.
	ibSessionIdentity m_identity;
	bool              m_inserted = false;  // sys_session row present

	// State machine. atomic for lock-free reads by observers
	// (IsActive / Auth checks from snapshot readers, admin UI).
	std::atomic<ibSessionState> m_state { ibSessionState::Created };
	std::atomic<ibAuthState>    m_auth  { ibAuthState::Anonymous };

	// cv + mutex — producer waits for state transitions here. Registry
	// thread notifies after each Transition(). NotifyAll because
	// multiple producers may be waiting on different predicates
	// (e.g. WaitAdded, WaitAttachSettled).
	mutable std::mutex              m_mtx;
	mutable std::condition_variable m_cv;

	wxString m_activity;   // guarded by m_mtx; last-reported activity string
	wxString m_reason;     // guarded by m_mtx; reject / auth-fail diagnostic

	// Legacy mirrors — populated alongside identity during migration
	// so existing readers (appData->GetUserInfo, scripts) see consistent
	// state. Eventually these become the primary storage, identity's
	// analogues become computed views.
	ibApplicationDataUserInfo m_userInfo;
	ibGuid                    m_sessionGuid;
	wxString                  m_sessionRawPassword;
};

// RAII guard that makes a session the Current() on the calling
// thread for the duration of the scope. Nested scopes restore the
// prior value on exit. Legacy — will be removed once direct ibSession
// pointer passing reaches all script call sites.
class BACKEND_API SessionScope {
public:
	explicit SessionScope(ibSession* s);
	~SessionScope();

	SessionScope(const SessionScope&)            = delete;
	SessionScope& operator=(const SessionScope&) = delete;

private:
	ibSession* m_prev;
};

#endif
