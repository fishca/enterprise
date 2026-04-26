#ifndef __IB_SESSION_REGISTRY_H__
#define __IB_SESSION_REGISTRY_H__

// ibSessionRegistry — owner of all ibSession instances in the current
// process. Renamed from SessionManager as part of the session-registry
// refactor; see memory note `project_session_registry_refactor.md` for
// the full design (DB row-lock as liveness truth, unified Connect(req)
// entry, ibSessionTicket RAII, single-consumer priority queue, fatal
// invariant on thread death).
//
// This header exposes:
//   - Legacy Create / Destroy / Find / List / Count (Phase-2 API, kept
//     behaviour-identical until Connect(req) lands in a follow-up).
//   - Queue + worker-thread plumbing (Start / Stop / Submit / request
//     bins). Thread stays idle until Start() is called explicitly by
//     `ibApplicationData::Connect`; Stop() joins it before Disconnect
//     frees the DB. DrainAll takes snapshots per-priority top-down
//     (strict descending Urgent → Normal → Low → Background, FIFO
//     within each bin) so Urgent evictions overtake pending Normal
//     Adds as soon as the next tick runs.
//   - Fatal-invariant plumbing (m_fatal + Die) — registry thread death
//     means sys_session no longer reflects reality, so any Connect /
//     Submit after that point must not pretend to succeed.
//
// Real Add / Attach / Remove / SetActivity handlers are not implemented
// yet (next commit). The scaffolding lives here so callers can wire
// Start/Stop into Connect/Disconnect without a follow-up churn.

#include "backend/backend.h"
#include "session.h"
#include "sessionPolicy.h"   // unique_ptr<ibSessionPolicy> needs complete type

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

class ibApplicationDataSessionArray;

// -------------------------------------------------------------------
// Request types + payload. Producers fill a request and Submit it with
// an explicit priority (Normal by default). Registry thread drains in
// strict descending-priority order.
// -------------------------------------------------------------------
enum class ibRegistryRequestKind {
	Add,          // register a newly-constructed session (anonymous or creds-bound)
	Attach,       // bind user identity to an already-Added session
	Detach,       // drop user identity, session stays Anonymous
	Remove,       // teardown — DELETE row, release lock, drop from m_own
	SetActivity,  // update currentActivity column (scripts / handlers)
};

struct BACKEND_API ibRegistryRequest {
	ibRegistryRequestKind      kind = ibRegistryRequestKind::Add;
	std::shared_ptr<ibSession> session;

	// Attach payload.
	wxString user;
	wxString password;

	// SetActivity payload.
	wxString activity;
};

// -------------------------------------------------------------------
// ibConnectRequest — unified entry-point parameters for Connect().
// Desktop, designer, web-server technical, web per-cookie all fill one
// of these. When user+password are empty the session stays anonymous
// after Connect (INSERT row with empty userName, expectsAnonPhase =
// true). When creds are supplied Connect attempts Attach inline and
// returns Ok only if Auth succeeded.
// -------------------------------------------------------------------
struct BACKEND_API ibConnectRequest {
	wxString  m_computer;
	wxString  m_address;           // "host:port" for web; "" for desktop
	// Process-level run mode — stays eWEB_ENTERPRISE_MODE across all
	// sessions that belong to a wes process, even per-tab clients.
	ibRunMode m_appMode = eENTERPRISE_MODE;
	// Session-level role. WebServer for wes's own technical row;
	// WebClient for per-tab connections; other values mirror runMode.
	// Default computed from m_appMode (see SessionKindFromRunMode).
	ibSessionKind m_kind = ibSessionKind::Enterprise;

	// Optional caller-supplied session guid. When non-empty and parsable
	// as an ibGuid, Connect uses it as the session's identifier instead
	// of minting a fresh one — lets the client tab's sessionStorage id
	// (tabSid) serve as sys_session.session, cookie value, SessionManager
	// map key, and sys_session row PK all at once. Empty / invalid →
	// fall back to wxNewUniqueGuid.
	wxString  m_presetGuid;

	wxString  m_userName;          // empty = anonymous (technical / pre-auth)
	wxString  m_password;          // plain-text transient — not stored

	// Expected anonymous-phase vs one-shot. If m_userName is empty
	// Connect always goes through anonymous phase (INSERT at Add,
	// registry drives the row visible immediately — useful for web
	// cookie mint / desktop dialog). If m_userName is set Connect
	// submits Add then Attach synchronously; on Attach failure the
	// session is torn down before Connect returns.

	// Optional session factory — when set, Connect() builds the session
	// through this callback instead of make_shared<ibSession>. Lets the
	// typed factory path (ibApplicationData::CreateSessionTyped<T>)
	// construct derived sessions (ibGUISession / ibEnterpriseSession /
	// ibWebClientSession etc.) while the registry's Add/Attach/Remove
	// pipeline keeps working through the base ibSession interface.
	// Default-constructed (empty std::function) → default behaviour.
	using SessionFactory =
		std::function<std::shared_ptr<ibSession>(std::string id, ibSessionKind kind)>;
	SessionFactory m_sessionFactory;
};

struct BACKEND_API ibConnectResult {
	enum Code {
		Ok,              // session Added (+ Attached if creds given) — m_session non-null
		RejectedPolicy,  // ProcessAdd policy veto — terminal
		RejectedAuth,    // ProcessAttach AuthenticateUser failed — terminal here
		Timeout,         // registry didn't answer within the window
		RegistryDown,    // registry fatal / not started
	};
	Code        m_code    = Timeout;
	wxString    m_reason;
	ibSession*  m_session = nullptr;   // owned by registry's m_own; session->Close() to remove
};

class ibDatabaseLayer;

class BACKEND_API ibSessionRegistry {
public:
	static ibSessionRegistry& Instance();

	// ---- Legacy API (Phase 2) ----
	// Left intact so existing callers (`ibApplicationData::Connect`,
	// `ibWebSession::Login`, `moduleInfo::DetachFromCurrentSession`) keep
	// working. Migrated to Submit-based flow in a follow-up commit.
	ibSession* Create(const std::string& id, ibRunMode runMode);
	void       Destroy(const std::string& id);
	ibSession* Find(const std::string& id);

	// Reverse lookup — find the session in m_own whose root module-manager
	// equals `mm`. Used by mm::CreateMainModule to recover its owning
	// session deterministically (without going through ibSession::Current()
	// which depends on AccessMode + thread-binding state). Returns nullptr
	// when no live session in m_own owns this mm. Iterates m_own under
	// m_ownMutex (shared lock).
	ibSession* FindSessionByRoot(ibValueModuleManagerConfiguration* mm) const;

	// Symmetric lookup by main-window pointer. Frame's own m_guiSession
	// back-link is the cheap path; this is for backend code that has
	// the frame pointer but no direct field on it (e.g. cross-DLL hooks).
	// Iterates m_own comparing s->GetFrame() == frame.
	ibSession* FindSessionByFrame(class ibBackendDocFrame* frame) const;

	std::vector<std::string> List() const;
	std::size_t              Count() const;

	// ---- Thread + queue ----
	// Set whether the registry owns sys_session row I/O. Default false —
	// handlers only drive in-memory state transitions (useful for tests
	// or embedding scenarios where a different actor keeps the table).
	// Desktop `ibApplicationData::Connect` flips this to true before
	// Start(), so handlers take over INSERT / UPDATE / DELETE and
	// `HoldRowLocks` takes real pessimistic locks. Must be set BEFORE
	// Start() — Start decides whether to check out pool connections
	// based on this flag.
	void EnableSysSessionOwnership(bool enabled) { m_ownsSysSession = enabled; }
	bool OwnsSysSession() const { return m_ownsSysSession; }

	// Start the consumer thread. No-op if already running. Invariant:
	// must be called once — and only once — before any Submit().
	// Typically called from `ibApplicationData::Connect` right after the
	// DB is open.
	void Start();

	// Signal stop, wake the thread, join it. The kill-switch — submits
	// Remove@Urgent for every session still in m_own before flipping
	// m_stop, so the worker's final drain pass DELETEs each sys_session
	// row + fires OnDisconnect listeners on its way out. Safe to call on
	// an already-stopped registry.
	void Stop();

	// Hosts register a predicate "should the process keep running?".
	// Used by debug-Destroy / shutdown paths: if any registered hook
	// returns true, the kill is declined (e.g. wes keeps serving while
	// user tabs are still connected). Empty list -> default false
	// (nothing wants to keep us alive, exit is OK).
	using KeepAliveHook = std::function<bool()>;
	void OnShouldKeepAlive(KeepAliveHook h);
	bool ShouldKeepAlive() const;

	// Submit a request. Thread-safe from any caller. Returns immediately;
	// producer blocks on the session's cv (ibSession::m_cv) for the
	// result. Submit on a stopped / fatal registry: request is dropped
	// silently for now (callers that care about Timeout use WaitState with
	// a timeout and observe the session never left Created).
	void Submit(ibRegistryRequest req, ibPriority priority = ibPriority::Normal);

	// ---- Unified entry-point ----
	// Submit Add (+ optional Attach if req.m_userName is non-empty),
	// block until the session settles or timeout. On success returns an
	// ibSessionTicket whose dtor submits Remove@Urgent. On failure the
	// result carries the terminal code + reason and an empty ticket.
	ibConnectResult Connect(const ibConnectRequest& req,
	                        std::chrono::milliseconds timeout = std::chrono::seconds(20));

	// ---- Admin signals (write to sys_session.signal for cross-process
	// control). Owning process picks them up on its next JobCheckSignal
	// tick (~3s) and acts:
	//   Kick   → sole-row kill; that session is Destroyed.
	//   Reload → process-wide eviction; every web-client session the
	//            owner has is torn down so clients re-login. Any row of
	//            the target process may carry the reload directive; the
	//            guid argument just names WHICH process to poke.
	// Both return true on successful UPDATE, false on DB error. Safe to
	// call from any thread.
	bool Kick(const wxString& sessionGuid);
	bool Reload(const wxString& sessionGuid);

	// ---- Cluster snapshot ----
	// Returns a copy of the last-refreshed snapshot of sys_session,
	// across all processes / machines. Refreshed every sweep tick
	// (~3s) by JobRefreshSnapshot. Caller UI (Active Users dialog,
	// admin endpoint) polls this; no blocking on the registry thread.
	// Returns an empty array before the first refresh or when
	// m_ownsSysSession is false (the registry isn't reading the table).
	ibApplicationDataSessionArray GetClusterSnapshot() const;

	// Policy-facing probe — wraps TryProbeRowLock on the registry's
	// dedicated probe connection. Returns true when the row is NOT
	// locked by anyone (zombie → safe to treat as dead). Only meaningful
	// when `m_ownsSysSession` is true; returns false otherwise (we can't
	// assert liveness without row-lock semantics, so policies should
	// default to "assume alive" = veto-friendly).
	bool ProbeSessionRowLock(const wxString& sessionGuid);

	// ---- Policy chain ----
	// Add a policy to the veto chain consulted by ProcessAdd. First veto
	// wins; subsequent policies don't run. Registry takes ownership of
	// the pointer — pass via std::make_unique.
	void AddPolicy(std::unique_ptr<ibSessionPolicy> policy);

	// ---- Lifecycle events ----
	// Process-wide event hooks fired by registry as sessions move through
	// their lifecycle. Listeners are typically wired during app bootstrap
	// (ibApplicationData ctor / OnInit) and replace the legacy monolithic
	// orchestration in appData::Connect/Disconnect. All callbacks run
	// synchronously on the thread that triggered the event (Authenticate
	// caller's thread, registry thread for Add/Remove, etc.) — listeners
	// must not block long.
	using SessionCallback = std::function<void(ibSession*)>;
	using VoidCallback    = std::function<void()>;

	// Fires after a session is added to the registry (ProcessAdd success).
	void OnConnectCreate(SessionCallback cb);

	// Fires when a session transitions to Authenticated (auth success).
	void OnAuthenticated(SessionCallback cb);

	// Fires once-per-process the first time a session reaches
	// Authenticated, OR after the registry was empty and a session
	// authenticates again. Use case: load metadata exactly once on
	// the first authenticated user.
	void OnFirstConnect(SessionCallback cb);

	// Fires before a session is removed (ProcessRemove).
	void OnDisconnect(SessionCallback cb);

	// Fires when the registry transitions from non-empty to empty.
	// Use case: unload metadata when no users remain.
	void OnLastDisconnect(VoidCallback cb);

	// Fires from mm::CreateMainModule after compile succeeded — handy
	// for post-compile diagnostics, AOT-cache writes, etc.
	void OnAfterCompile(SessionCallback cb);

	// Per-session reload signal (admin issued "reload" on this session's
	// sys_session row). JobCheckSignal fires this for the matching own-
	// session; frontend / web-side listeners react (close frame, evict
	// tab, etc.) — backend stays UI-free.
	void OnReload(SessionCallback cb);

	// Internal — called by session/registry at the right transition points.
	// Public so session.cpp's Authenticate can fire NotifyAuthenticated;
	// registry's ProcessAdd/Remove fire the others.
	void NotifyConnectCreate(ibSession* s);
	void NotifyAuthenticated(ibSession* s);
	void NotifyDisconnect(ibSession* s);
	void NotifyAfterCompile(ibSession* s);
	void NotifyReload(ibSession* s);

	// ---- Session access mode + fallback ----
	// Process-wide flag describing how ibSession::Current() resolves the
	// active session. Owned by registry because it's session-population
	// policy: Single-mode app has 1 session and resolution is constant;
	// Client-mode runs N concurrent sessions strictly per-thread; Server-
	// mode is per-thread with a process-wide fallback. Mode is set by
	// ibApplicationData ctor based on runMode and persists for the
	// process lifetime. ibSession::Current() reads through here.
	void                SetAccessMode(ibSession::AccessMode mode);
	ibSession::AccessMode GetAccessMode() const;

	// Shared-mode fallback session — returned by Current() when calling
	// thread is unbound. No-op in Single mode (which ignores the calling
	// thread entirely).
	void       SetFallback(ibSession* s);
	void       ClearFallback();
	ibSession* GetFallback() const;

	// Registry-thread invariant: if the thread exits abnormally (exception
	// escaped, stuck tick, DB hang) the process must terminate — continuing
	// would mean sys_session reflects lies to peer processes. These
	// accessors let callers check state before acting on stale session
	// pointers. Die() is [[noreturn]] — logs + std::terminate().
	bool IsThreadAlive() const { return m_threadAlive.load(std::memory_order_acquire); }
	bool IsFatal()       const { return m_fatal.load(std::memory_order_acquire); }

	ibSessionRegistry();
	~ibSessionRegistry();

	ibSessionRegistry(const ibSessionRegistry&)            = delete;
	ibSessionRegistry& operator=(const ibSessionRegistry&) = delete;

private:

	void ThreadBody() noexcept;

	// Drain queue: pop snapshot from each bin top-down (Urgent first),
	// return as a flat vector in processing order. Runs under the submit
	// lock briefly — actual handlers execute outside the lock.
	std::vector<ibRegistryRequest> DrainAll();

	// Per-request handlers. All stubs for now; real impl lands with
	// ibSessionTicket + Connect(req).
	void ProcessAdd(ibRegistryRequest& req);
	void ProcessAttach(ibRegistryRequest& req);
	void ProcessDetach(ibRegistryRequest& req);
	void ProcessRemove(ibRegistryRequest& req);
	void ProcessSetActivity(ibRegistryRequest& req);

	// Periodic jobs. Real impl ports `Job_*` from appDataQuery.cpp in a
	// follow-up; current bodies are placeholders that simply bump the
	// tick counter.
	void JobSweepStale();
	void JobRefreshSnapshot();

	// Refresh the pessimistic-lock hold on m_lockConn so every currently-
	// inserted session's row is locked. Called after any m_own change
	// (Add / Remove). Commits the prior hold-TX before opening a fresh
	// one — small blip window (~ms) during which a peer's Sweep could
	// DELETE our rows; re-verified on next loop pass (future refinement:
	// re-INSERT missing rows on blip detect).
	void RebuildLockHold();

	// Refresh lastActive on every own row. Companion to JobSweepStale's
	// fallback-cutoff path — peers use lastActive staleness to detect
	// force-killed processes whose lock TX hasn't been rolled back by
	// the FB engine yet. Cheap single UPDATE statement guarded by our
	// m_writeConn.
	void JobHeartbeatOwn();

	// Poll sys_session.signal for every row in m_own. Non-empty value is
	// an admin directive:
	//   "kick"   — submits Remove@Urgent for that session.
	//   "reload" — flips m_reloadRequested so the embedding runtime
	//              (wfrontend's SweepLoop) can evict its web-clients and
	//              force a reconnect against the freshly-loaded metadata.
	// Signal column is cleared (UPDATE ... SET signal=NULL) immediately
	// after dispatch so it fires once per write. Runs at the sweep
	// interval — admin operations are not latency-sensitive.
	void JobCheckSignal();

public:
	// One-shot consume: returns true exactly once after a "reload"
	// signal was processed, resetting the flag so the next caller sees
	// false until another directive lands. Embedders poll this on their
	// own tick (e.g. wfrontend's SweepLoop every 15s).
	bool ConsumeReloadRequest() {
		return m_reloadRequested.exchange(false, std::memory_order_acq_rel);
	}

private:

	// Fatal fail-stop. Does NOT return — logs `why` + std::terminate.
	[[noreturn]] void Die(const wxString& why);

	// --- storage (thread-owned; only ThreadBody touches after Start) ---
	// Until queue-based Add lands, m_sessions is written by Create/Destroy
	// under m_mutex — classic Phase 2 layout.
	mutable std::mutex                                           m_mutex;
	std::unordered_map<std::string, std::unique_ptr<ibSession>>  m_sessions;

	// --- queue-based ownership (populated by ProcessAdd) ---
	// shared_ptr — ticket co-owns. When ProcessRemove erases the map
	// entry, the ticket's shared_ptr keeps the session alive until it
	// drops too; that's fine because at Stopping → Gone the session has
	// no DB row and no lock anymore.
	//
	// m_ownMutex guards m_own for cross-thread reads (FindSessionByRoot
	// from compile threads). Writers — ProcessAdd / ProcessRemove on the
	// registry thread — take a unique lock; readers take a shared lock.
	mutable std::shared_mutex                                    m_ownMutex;
	std::unordered_map<std::string, std::shared_ptr<ibSession>>  m_own;

	// Policy chain. Built at Start-time, read-only once the thread runs
	// (no need for extra locking — only ThreadBody touches on Add).
	std::vector<std::unique_ptr<ibSessionPolicy>>                m_policies;

	// Process-wide access mode + Shared-mode fallback. Set once at app
	// startup (see SetAccessMode); read-only afterwards under shared lock
	// from ibSession::Current.
	mutable std::shared_mutex                                    m_accessMutex;
	ibSession::AccessMode                                        m_accessMode = ibSession::AccessMode::Single;
	ibSession*                                                   m_fallback = nullptr;

	// Lifecycle event listeners. Mutated only at app bootstrap (single
	// thread); read at notify time. Mutex guards both the lists and
	// m_authenticatedCount / m_firstConnectFired against concurrent
	// notifications from auth threads and registry thread.
	mutable std::mutex                  m_eventMutex;
	std::vector<SessionCallback>        m_listConnectCreate;
	std::vector<SessionCallback>        m_listAuthenticated;
	std::vector<SessionCallback>        m_listFirstConnect;
	std::vector<SessionCallback>        m_listDisconnect;
	std::vector<VoidCallback>           m_listLastDisconnect;
	std::vector<SessionCallback>        m_listAfterCompile;
	std::vector<KeepAliveHook>          m_listKeepAlive;
	std::vector<SessionCallback>        m_listReload;
	std::size_t                         m_authenticatedCount = 0;
	bool                                m_firstConnectFired  = false;

	// --- sys_session ownership ----
	// Off by default — handlers only drive state transitions. Flip to
	// true (via EnableSysSessionOwnership) before Start() to take over
	// sys_session row I/O + pessimistic row locks.
	bool                                                         m_ownsSysSession = false;

	// Separate pool checkouts — lock-conn holds the long-running TX
	// that pins `HoldRowLocks` for every row in m_own; write-conn
	// executes INSERT / UPDATE / DELETE outside that TX. Probe-conn
	// runs `TryProbeRowLock` via its own NOWAIT TX. All three come
	// from `ibApplicationData::GetConnectionPool()` on Start when
	// m_ownsSysSession is true; nullptr otherwise.
	std::shared_ptr<ibDatabaseLayer>                             m_lockConn;
	std::shared_ptr<ibDatabaseLayer>                             m_writeConn;
	std::shared_ptr<ibDatabaseLayer>                             m_probeConn;

	// --- submit queue + thread plumbing ---
	std::thread                                  m_thread;
	std::atomic<bool>                            m_stop         { false };
	std::atomic<bool>                            m_threadAlive  { false };
	std::atomic<bool>                            m_fatal        { false };
	wxString                                     m_fatalReason;  // set before m_fatal = true

	// "reload" admin signal latch — set by JobCheckSignal, consumed by
	// ConsumeReloadRequest() on the caller's polling tick.
	std::atomic<bool>                            m_reloadRequested { false };

	// Tick counter — monotonic, incremented once per loop pass. External
	// watchdogs read this to detect a stuck thread (separate future commit).
	std::atomic<std::uint64_t>                   m_tickCounter  { 0 };

	// Priority bins — one deque per ibPriority value. Drain iterates
	// bins top-down so Urgent evictions always overtake Normal adds in
	// a given tick.
	std::mutex                                   m_submitMtx;
	std::condition_variable                      m_submitCv;
	static constexpr std::size_t kPriorityBinCount = 4;
	std::array<std::deque<ibRegistryRequest>, kPriorityBinCount> m_bins;

	// Cluster snapshot — mirror of sys_session across every process.
	// Written by JobRefreshSnapshot on the registry thread, read by UI
	// via GetClusterSnapshot() on any thread. shared_mutex: writers are
	// rare (~3s), readers may be frequent (polling dialogs).
	mutable std::shared_mutex                              m_snapshotMtx;
	std::unique_ptr<ibApplicationDataSessionArray>         m_snapshot;
};

#endif
