#ifndef __IB_SESSION_H__
#define __IB_SESSION_H__

// ibSession — per-session record owned by ibSessionRegistry. Identity
// (guid, user, computer, app mode, started) + runtime state machine
// (lifecycle / auth) + script bindings (module manager, ProcUnit map).
//
// Renamed from ibSessionContext as part of the session-registry
// refactor. ibSessionScope / Current() stay available as legacy shims
// during migration — direct ibSession pointer passing (via ibProcUnit
// etc.) is the target, thread_local Current() is deprecated.

#include "backend/backend.h"
#include "backend/userInfo.h"
#include "backend/appData.h"      // ibRunMode
#include "backend/backend_exception.h"
#include "backend/compiler/value.h" // ibValue (base for ibValuePtr)
#include "backend/value_ptr.h"    // ibValuePtr for m_root

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <wx/datetime.h>
#include <wx/string.h>

class ibValueModuleManager;
class ibValueModuleManagerConfiguration;
class ibRuntimeModuleDataObject;
class ibProcUnit;
struct ibRunContext;
class ibMetaData;
class ibValueMetaObjectConfiguration;
class BACKEND_API ibBackendDocFrame;

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
//
// Inherits std::enable_shared_from_this so Authenticate() can submit
// Attach requests to the registry without an ibSessionTicket in the
// call path — the request's shared_ptr<ibSession> is minted via
// shared_from_this(), safe because every ibSession instance is created
// via std::make_shared (registry path) or the typed factory.
// ------------------------------------------------------------------
class BACKEND_API ibSession : public std::enable_shared_from_this<ibSession> {
public:
	// kind = what this session does in the running process. The process-
	// level run mode (how the exe was launched) lives on appData and is
	// the same for every session inside a process — not duplicated here.
	ibSession(std::string id, ibSessionKind kind);

	// Virtual — derived ibGUISession (frontend.dll) and per-exe concrete
	// sessions (ibEnterpriseSession, ibDesignerSession) hang off this.
	// Registry + ticket hold shared_ptr<ibSession>; concrete dtor runs
	// through the virtual chain.
	virtual ~ibSession();

	ibSession(const ibSession&)            = delete;
	ibSession& operator=(const ibSession&) = delete;

	// Main UI frame this session owns. Default returns the session-stored
	// pointer (set via SetFrame from non-derived owners like ibWebSession),
	// derived GUI sessions (ibGUISession) override to return their typed
	// frame and ignore m_frame. Frameless sessions (WebServer, WebCompute,
	// Service headless) inherit null because nothing calls SetFrame on them.
	virtual ibBackendDocFrame* GetFrame() const { return m_frame; }

	// Setter for non-derived owners that build the frame separately
	// (ibWebSession owns ibWebApplication which owns ibWebFrame; the
	// session itself is plain ibSession, so it needs a way to publish
	// the frame for ibSession::CurrentFrame() lookups from script-side
	// CreateNewForm / FindFormByUniqueKey calls).
	void SetFrame(ibBackendDocFrame* f) { m_frame = f; }

	// Lifecycle event hooks. OnCreateSession fires once on the main
	// (caller) thread after the registry has Added the session — GUI
	// subclasses create their wx frame here so ctor work lands on the
	// UI thread. OnDestroySession fires symmetrically from
	// ibApplicationData::CloseSession on the main thread before the
	// ticket is released so frame/UI resources are torn down in a safe
	// place. Returns false to veto the close (only honoured when
	// Close was called with force=false). Base bodies are no-ops/true.
	virtual bool OnCreateSession()  { return true; }
	virtual bool OnDestroySession(bool force = false) { (void)force; return true; }

	// Make the session's main frame visible. Called from the host app's
	// OnRun after Authenticate but before the wx event loop starts. GUI
	// sessions override (ibGUISession does Show + Center + Raise on their
	// frame); frameless sessions (WebServer, headless service, codeRunner)
	// inherit the default no-op success — nothing to show, OnRun proceeds.
	virtual bool ShowFrame() { return true; }

	// Session-owned auth orchestration. Submits Attach to the registry
	// directly (via shared_from_this so no ticket is required in the call
	// path). If CLI creds already work, returns true silently — no prompt
	// event. On Attach failure raises OnShowAuthenticate — GUI override
	// shows the login dialog, writes singleton userInfo / rawPassword,
	// returns true — then re-submits Attach with the stored session creds.
	// Returns true iff the auth axis transitioned to Authenticated.
	// Non-GUI sessions inherit the default OnShowAuthenticate (false) so
	// the fallback no-ops and Authenticate reports the original failure;
	// GUI app OnInit terminates the process in that case.
	bool Open(const wxString& user, const wxString& password);

	// Interactive prompt event — fires only when silent Attach fails.
	// Overridden by ibGUISession (shared for designer + enterprise; shows
	// the wx login dialog) and future ibWebClientSession (HTTP login
	// form). Base no-op returns false so non-GUI sessions fail hard on
	// wrong creds without hanging on a non-existent prompt.
	virtual bool OnShowAuthenticate(const wxString& /*user*/, const wxString& /*password*/) { return false; }

	// Legacy id (string representation of m_guid or external cookie).
	// New code should prefer m_identity.m_guid directly.
	const std::string& GetId()   const { return m_id; }
	ibSessionKind      GetKind() const { return m_kind; }

	// Root runtime of this session. Populated by CreateRoot() at
	// StartSession / Login; stays nullptr for sessions that never run
	// scripts (Designer, WebServer technical session, Launcher).
	ibValueModuleManagerConfiguration* GetModuleManager() const;

	// Create the session's root module manager. The configuration's
	// commonMetaObject is taken directly from metaData (typed accessor —
	// no dynamic_cast needed). Returns a pointer for immediate use;
	// nullptr on failure. Calling twice replaces the old root — previous
	// ibValuePtr releases its ref (delete-if-last) after running
	// DestroyMainModule on it. CompileRoot is separate so callers can
	// register common modules in metadata's storage between the two.
	ibValueModuleManagerConfiguration* CreateRoot(class ibMetaDataConfigurationBase* metaData);

	// Explicit close — fires OnDestroySession on the calling thread
	// (main-thread wx frame teardown for GUI sessions) and submits
	// Remove@Urgent to the registry so this session is dropped from
	// m_own. After Close returns, the session pointer may dangle once
	// the registry processes the Remove; callers should null their
	// pointer immediately after Close.
	//
	// force=false (default) — soft close. OnDestroySession may run veto
	//                         checks (AllowClose / unsaved-data prompts).
	// force=true             — hard close. Skips veto, always destroys —
	//                         used by debug-Destroy and shutdown paths
	//                         where the close cannot be cancelled.
	void Close(bool force = false);

	// Drop the user identity bound to this session. Auth axis transitions
	// back to Anonymous; the session itself stays Added so the caller can
	// retry Authenticate. Best-effort with a soft timeout.
	void Detach(std::chrono::milliseconds timeout = std::chrono::seconds(5));

	// Fire-and-forget activity label update — submits SetActivity@Low so
	// it never preempts real Add / Remove work. Registry handler UPDATEs
	// `sys_session.currentActivity`; admin UI sees it on the next snapshot.
	void SetActivity(const wxString& activity);

	// Compile the root mm — runs CreateMainModule on the allocated
	// m_root. Called after metadata->RunDatabase() has populated common-
	// module descriptors in metadata's ibModuleStorage. Returns false
	// if root isn't allocated or compile fails.
	bool CompileRoot();

	// Symmetric teardown — DestroyMainModule on the root mm without
	// dropping it. Used by callers that want to tear down compile state
	// before metadata-side close cascade. ClearRoot also runs this
	// internally before resetting m_root.
	bool DestroyRoot();

	// Drop the session's root explicitly. Must be called before the
	// process-level metadata tree is torn down (ibMetaData::CloseDatabase),
	// otherwise the root's refs to metadata descriptors dangle. No-op
	// when the session never had a root.
	void ClearRoot();

	// Idempotent CreateRoot driven by the active process-level metadata.
	// Called by ibSessionRegistry::NotifyAuthenticated between the
	// OnFirstConnect phase (which may run metadataCreate, populating
	// activeMetaData) and the OnAuthenticated phase (whose listeners —
	// e.g. RunDatabase → OnBeforeRunMetaObject — need session->mm to
	// already exist). No-op when m_root already set or activeMetaData null.
	void EnsureRoot();

	ibApplicationDataUserInfo&       GetUserInfo()       { return m_userInfo; }
	const ibApplicationDataUserInfo& GetUserInfo() const { return m_userInfo; }
	void SetUserInfo(const ibApplicationDataUserInfo& info) { m_userInfo = info; }

	const ibGuid& GetSessionGuid() const { return m_sessionGuid; }
	void          SetSessionGuid(const ibGuid& guid) { m_sessionGuid = guid; }

	const wxString& GetSessionRawPassword() const { return m_sessionRawPassword; }
	void            SetSessionRawPassword(const wxString& pwd) { m_sessionRawPassword = pwd; }
	void            ClearSessionRawPassword() { m_sessionRawPassword.clear(); }

	// Per-session "working date" — the conceptual business-date used by
	// script's WorkingDate() helper (reports, document registration,
	// etc.). Initialized to the session-creation wall-clock; scripts
	// that want a pinned past/future date call SetWorkDate. Replaces
	// the legacy static ibValueSystemFunction::ms_workDate so two web
	// sessions in the same process don't step on each other's value.
	const wxDateTime& GetWorkDate() const { return m_workDate; }
	void              SetWorkDate(const wxDateTime& d) { m_workDate = d; }

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

	// Access mode — set once by the application at startup, before any
	// session is created.
	//
	//   Single — the process runs exactly one session for its entire life
	//            (designer.exe, enterprise.exe, daemon.exe, codeRunner.exe,
	//            classChecker.exe). Current() returns the lone session
	//            regardless of the calling thread; bindings are recorded
	//            for diagnostics but lookup ignores them.
	//
	//   Shared — per-thread lookup with a process-wide fallback.
	//            wenterprise-server.exe — workers serving a tab register
	//            their session under their thread id; the wes process's
	//            own system session is registered via SetFallback and
	//            served to any thread that isn't a tab worker (registry
	//            consumer, signal handlers, etc.).
	enum class AccessMode { Single, Shared };

	static void       SetAccessMode(AccessMode mode);
	static AccessMode GetAccessMode();

	// Canonical "session this code is currently working on". Lookup
	// strategy depends on AccessMode (see above).
	static ibSession* Current();

	// Shared-mode fallback — session returned by Current() when the
	// calling thread isn't bound. Effective only when AccessMode == Shared.
	static void SetFallback(ibSession* s);
	static void ClearFallback();

	// Diagnostic — given a thread id, what session is currently scoped on
	// that thread? In Single mode returns the lone session; in Multi mode
	// returns the bound session or the fallback.
	static ibSession* GetByThread(std::thread::id tid);

	// Explicit bind — pin a session under an arbitrary thread id without
	// going through ibSessionScope. Use cases: eval / parallel-execute
	// scenarios where one session is shared across worker threads, or
	// pre-binding a session for a thread that hasn't started yet.
	// Pairs with UnbindThread; not RAII — caller owns lifetime.
	static void BindSessionToThread(ibSession* s, std::thread::id tid);
	static void UnbindThread(std::thread::id tid);

	// Erase every thread-binding pointing to this session, regardless of
	// which thread put it there. Used by registry-thread teardown
	// (OnDisconnect listener) where the original binding-thread isn't
	// available — find by session pointer instead.
	static void UnbindSession(ibSession* s);

	// Diagnostic — atomic snapshot of the full thread→session map. The
	// k_singletonKey entry (default-constructed thread::id) appears in
	// the snapshot when singleton mode is active. Snapshot is by-value;
	// safe to iterate after returning.
	static std::vector<std::pair<std::thread::id, ibSession*>> SnapshotByThread();

	// Convenience: frame of the currently-scoped session. Single
	// canonical entry point for backend code reaching the process's
	// main UI window. Equivalent to:
	//   ibSession* s = Current(); return s ? s->GetFrame() : nullptr;
	// Null when no scope is active or when the scoped session is
	// frameless (web-server, headless, codeRunner).
	static ibBackendDocFrame* CurrentFrame();

private:
	friend class ibSessionScope;
	friend class ibSessionRegistry;

	std::string    m_id;
	ibSessionKind  m_kind;

	// Non-owning. Set via SetFrame by ibWebSession after ibWebApplication
	// builds its ibWebFrame; ibGUISession ignores this and returns its own
	// typed pointer through GetFrame() override.
	ibBackendDocFrame* m_frame = nullptr;

public:
	// Per-session debug state. nullptr -> session is not being debugged
	// (ibProcUnit::Execute skips breakpoint checks fast). Allocated by
	// EnableDebug() when --debug starts the session or designer attaches;
	// destroyed in DisableDebug() / ~ibSession. Migrated here from
	// process-level ibDebuggerServer fields so concurrent web sessions
	// in wenterprise-server can each enter their own debug loop without
	// blocking the others.
	struct ibDebugSession {
		// True while ibProcUnit::Execute is parked in DoDebugLoop's CV
		// wait. Designer's Continue/Step/Detach commands clear this and
		// notify the CV for the matching session.
		std::atomic<bool>       m_debugLoop{false};

		// Run context of the script frame currently stopped at the
		// breakpoint. Eval / locals / stack on the debugger side resolve
		// through this.
		ibRunContext*           m_runContext{nullptr};

		// CV + mutex pair: producer (script thread inside DoDebugLoop)
		// waits on m_cv; consumer (designer command handler in the
		// debug-server worker) flips m_debugLoop and notifies.
		std::condition_variable m_cv;
		std::mutex              m_mutex;

		// Per-session watch expressions (id -> source). Breakpoints
		// stay process-level on ibDebuggerServer because module bytecode
		// is shared across sessions.
		std::map<unsigned long long, wxString> m_expressions;
	};

	bool IsDebug() const     { return m_debug != nullptr; }
	ibDebugSession* Debug()  { return m_debug.get(); }
	void EnableDebug()       { if (!m_debug) m_debug = std::make_unique<ibDebugSession>(); }
	void DisableDebug()      { m_debug.reset(); }

private:
	// nullptr unless the session was created with debug attached.
	std::unique_ptr<ibDebugSession> m_debug;

	// Root runtime — intrusive-refcounted owner (ibValuePtr is the
	// project convention for ibValue-derived types). Nested descriptors
	// (common modules, object instances, forms) parent up through
	// m_parent chain. See project_runtime_facade_plan.md.
	ibValuePtr<ibValueModuleManagerConfiguration> m_root;

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

	// Script-visible "working date" — see GetWorkDate/SetWorkDate.
	// Initialized to the session-creation wall-clock in the ctor.
	wxDateTime                m_workDate;
};

// RAII binding for the calling thread — calls
// ibSession::BindSessionToThread on construction and ibSession::UnbindThread
// on destruction. Use in app entry points (OnRun) where the binding
// should live for the entire app lifetime: declare on the stack
// alongside the session pointer and let it cover every error-return
// path automatically. Differs from ibSessionScope in that it doesn't
// nest / restore prior values — the calling thread is expected to be
// unbound before construction and after destruction.
class BACKEND_API ibSessionThreadBinding {
public:
	explicit ibSessionThreadBinding(ibSession* s) noexcept;
	~ibSessionThreadBinding();

	ibSessionThreadBinding(const ibSessionThreadBinding&)            = delete;
	ibSessionThreadBinding& operator=(const ibSessionThreadBinding&) = delete;

private:
	std::thread::id m_tid;
};

// RAII guard that makes a session the Current() on the calling
// thread for the duration of the scope. Nested scopes restore the
// prior value on exit. Legacy — will be removed once direct ibSession
// pointer passing reaches all script call sites.
class BACKEND_API ibSessionScope {
public:
	explicit ibSessionScope(ibSession* s);
	~ibSessionScope();

	ibSessionScope(const ibSessionScope&)            = delete;
	ibSessionScope& operator=(const ibSessionScope&) = delete;

private:
	ibSession* m_prev;
};

// ---------------------------------------------------------------------
// ibApplicationData::CreateSessionTyped<SessionT> — template body kept
// here so every callsite that already includes session.h (to use the
// derived type) gets the definition automatically. Instantiation needs
// the concrete SessionT ctor (std::string, ibSessionKind) to be visible,
// which it is at the call site that uses the derived type.
// ---------------------------------------------------------------------
template<class SessionT>
SessionT* ibApplicationData::CreateSessionTyped()
{
	static_assert(std::is_base_of<ibSession, SessionT>::value,
		"CreateSessionTyped<T>: T must derive from ibSession");

	ibSession* base = CreateSessionWithFactory(
		[](std::string id, ibSessionKind kind) -> std::shared_ptr<ibSession> {
			return std::make_shared<SessionT>(std::move(id), kind);
		});
	if (base == nullptr)
		ibBackendCoreException::Error(_("Failed to create session"));

	SessionT* derived = static_cast<SessionT*>(base);
	if (!derived->OnCreateSession()) {
		derived->Close();   // submit Remove → registry drops m_own entry
		ibBackendCoreException::Error(_("Failed to create session frame"));
	}
	return derived;
}

#endif
