# Session lifecycle

> **2026-04-20:** Classes renamed — `ibSessionContext` → `ibSession`,
> `SessionManager` → `ibSessionRegistry`. File paths: `backend/session/session.h`
> and `backend/session/sessionRegistry.h`. Behaviour unchanged; the full
> refactor (single-consumer thread, priority queue, DB-row pessimistic
> locks, `ibSessionTicket` RAII handle, unified `Connect(req)` entry) is
> in progress — see `docs/web/open-issues.md` → "Session registry
> refactor" and memory note `project_session_registry_refactor.md`.

Two-phase model since the compile/runtime split (2026-04-19):

1. **Server bootstrap** (`wfrontendInit*` → `appData->Connect` → metadata
   load) compiles all modules once, into the process-wide
   `ibValueModuleManagerConfiguration` that lives on `activeMetaData`.
   No `ibProcUnit` is created at this stage.
2. **Per-session runtime** (`ibWebSession::Login` → `InitRuntimeForSession`
   → `ibWebApplication::OnInit`) allocates the session's `ibProcUnit`
   instances against the shared bytecode and fires the `OnStart` script
   on them.

`ibWebApplication` is the per-session "app" analogue of `wxTheApp`. It
does **not** own the module manager — that's shared. It owns the
frame, the worker thread, and a borrowed pointer to the session's
`ibSession`.

## Session run mode

Every `ibSession` carries its own `ibRunMode m_runMode` set at
creation time. Often equals `ibApplicationData::GetAppMode()`, but
diverges inside a web process: the wenterprise-server's startup
session is `eWEB_ENTERPRISE_MODE` (technical bootstrap, no runtime),
every `/login` mints a per-cookie session as `eENTERPRISE_MODE`
(thick-client-like user runtime living inside the web process).

`InitRuntimeForSession` reads `session->GetRunMode()` to decide
whether to spin up `ibProcUnit`s:

```cpp
const ibRunMode mode = session->GetRunMode();
const bool wantsRuntime =
    (mode == eENTERPRISE_MODE) || (mode == eSERVICE_MODE);
if (!wantsRuntime) return true;  // designer, launcher, web technical
```

**Don't try to derive session run mode from state** (`userInfo`-
populated, or similar). An open-access configuration (empty `sys_user`
table) leaves `userInfo` empty even after a successful `/login` —
`AuthenticationAndSetUser` early-returns `true` without dual-writing
to the session. A "empty userInfo → skip runtime" heuristic would
falsely skip that legitimate user session. The previous `ibSessionRole`
enum was collapsed into `ibRunMode` (2026-04-20); attempting to drop
the per-session field entirely was reverted for this reason.

## Login flow

```
POST /w/<prefix>/login
    │
    └── ibWebSession::Login(user, password)
        ├── appData->AuthenticationAndSetUser(user, pw)
        │     OES-level auth; mirrors user info both onto appData
        │     (legacy singleton) and onto Current() session
        │     (ibSession::m_userInfo / m_sessionGuid /
        │      m_sessionRawPassword — dual-write during migration).
        │
        ├── ibSessionRegistry::Create(sessionId, ibSessionRole::User)
        │     Returns a borrowed ibSession*. Lives in the
        │     manager's map until Destroy; HTTP thread and the
        │     session's worker thread install it via SessionScope.
        │
        ├── { SessionScope initScope(ctx);
        │     │
        │     ├── mm->InitRuntimeForSession(ctx)
        │     │     Creates shared_ptr<ibProcUnit> for the main module
        │     │     plus one per non-global common module, wires up
        │     │     SetParent / extern tables, Executes bytecode to
        │     │     populate globals, then AttachProcUnit(key, pu) on
        │     │     the session — keyed by the ibModuleDataObject
        │     │     descriptor. Session-only; System/Designer sessions
        │     │     skip this.
        │     │
        │     └── app->OnInit()                    ← ORDER MATTERS
        │           │                                (see note below)
        │           ├── new ibWebFrame(this)       ← frame first; scripts
        │           │                                need it during OnStart
        │           └── StartMainModuleSafe(mgr)
        │                 │ SEH-wrapped call into:
        │                 └── ibValueModuleManagerConfiguration::StartMainModule
        │                     ├── BeforeStart()  → GetProcUnit() →
        │                     │     CallAsProc("beforeStart", bCancel)
        │                     │     (script veto; false ⇒ abort login)
        │                     └── OnStart()      → GetProcUnit() →
        │                           CallAsProc("onStart")
        │                           (script opens default forms via
        │                            OpenForm(...))
        │   }
        │
        └── m_app = move(app)
```

`GetProcUnit()` on the shared moduleManager delegates through
`ibSession::Current()->GetProcUnitFor(this)`; Current() is set by
`SessionScope` on the running thread. Fallback is the descriptor's own
`m_procUnit` (legacy / designer path). See
`src/engine/backend/moduleInfo.cpp`.

### InitRuntimeForSession must run before OnInit

`app->OnInit()` calls `StartMainModule` which fires `BeforeStart` /
`OnStart`. Those resolve the runtime via `GetProcUnit()` delegate; the
delegate returns the session's attached ProcUnit. If the attach happens
**after** OnInit, the map is still empty at OnStart time, the delegate
falls back to the now-cleared shared `m_procUnit`, returns `nullptr`,
and the script (including every `OpenForm` it makes) is silently
skipped — symptom: `tabCount=0` after a successful login.

Order is hard-coded in `webSession.cpp::Login`. Don't swap it.

## Teardown flow

Destruction mirrors Login in reverse:

```
ibWebSession::OnExit
    ├── { SessionScope exitScope(ctx);
    │     └── mm->ExitRuntimeForSession(ctx)
    │           DetachProcUnit per descriptor; shared_ptr refcount
    │           drops; ibProcUnit dtor runs.
    │   }
    ├── app->OnExit()
    │     ├── RunOnWorker([]{ DeleteAllViews }).get()  ← close tabs on
    │     │                                              worker thread;
    │     │                                              script events
    │     │                                              (beforeClose /
    │     │                                              onClose) fire
    │     │                                              where ibProcUnit's
    │     │                                              thread-local state
    │     │                                              is valid
    │     ├── StopWorker()
    │     ├── ExitMainModuleSafe(sharedMgr)
    │     │     BeforeExit() → CallAsProc("beforeExit", bCancel)
    │     │     OnExit()     → CallAsProc("onExit")
    │     └── delete m_frame
    └── ibSessionRegistry::Destroy(sessionId)
```

Shared `ibValueModuleManagerConfiguration` is **not** destroyed at
session exit — it lives for the process lifetime on metadata.

## SEH wrapper — why

`BeforeStart` / `OnStart` are wrapped in `try { ... } catch (...) { }`
inside `moduleManagerEvent.cpp`. That catches C++ exceptions, but a
**Windows structured exception** (access violation — desktop-only
singleton dereferenced by the script call path) is not a C++ exception
under `/EHsc` and bypasses the catch. Without a `__try/__except` at the
outer call site, the process dies and the HTTP server goes with it.

`StartMainModuleSafe` / `ExitMainModuleSafe` in
`src/engine/frontend/web/webApplication.cpp` wrap the call in SEH. On an
AV, the helper captures `GetExceptionCode()` + `ExceptionAddress` and
calls `ReportFaultAddress(...)` which uses dbghelp (`SymFromAddr` +
`SymGetLineFromAddr64`) to log:

```
[app] StartMainModule SEH: code=0xC0000005 at 0x6355E2C0
    (wfrontend.dll+0x34e2c0) ibValueFrame::LoadControl+0x80
    [F:\...\frame.cpp:69]
```

The helper runs in the `__except` filter expression (C++-safe, no SEH
unwinding of C++ objects under `/EHsc`).

## Worker thread

`ibWebApplication::StartWorker` spawns one thread per session at the
end of `OnInit`. That thread is the sole script-runner from then on:
HTTP handlers submit via `RunOnWorker<T>(fn).get()` (blocking),
timers via `PostWork(fn)` (fire-and-forget). `ibProcUnit` is
thread-affine to the worker — serialising through one thread removes
any per-access mutex.

The worker installs `SessionScope(m_sessionContext)` and
`ibDebuggerServer::SetCurrent(m_debugServer)` at the top of
`WorkerLoop` so breakpoints and `GetProcUnit()` lookups resolve to
this session's state rather than the process-level fallback.

## `ibWebApplication` fields today

- `m_frame` — raw `ibWebFrame*`. Owned; `new` in OnInit, `delete` in
  OnExit. Not refcounted.
- `m_sessionContext` — borrowed `ibSession*`. Set by the owning
  `ibWebSession` right after construction; ibSessionRegistry owns the
  session's lifetime.
- `m_debugServer` — per-session `ibDebuggerServer` for breakpoints.
- Worker-thread state (`m_worker`, `m_workerMtx`, `m_workerCv`,
  `m_workerQueue`, `m_workerStop`).
- Live-update counter (`m_seq`, `m_seqMtx`, `m_seqCv`) for SSE.

The former `ibValuePtr<ibValueModuleManagerConfiguration> m_moduleManager`
field is gone — `GetModuleManager()` now returns the shared instance
via `static_cast<ibValueModuleManagerConfiguration*>(activeMetaData->GetModuleManager())`.
