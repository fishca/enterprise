# Web runtime — architecture

## Compile/runtime split

Since 2026-04-19 the module manager separates **compile** (process-wide,
shared) from **runtime** (per-session).

`ibValueModuleManagerConfiguration` lives on `activeMetaData` — one
instance per process, owns the compiled bytecode for main + common
modules. No `ibProcUnit` on the manager itself.

`ibSession::m_procUnitMap<descriptor, shared_ptr<ibProcUnit>>`
holds the per-session runtime. `ibValueModuleManager::InitRuntimeForSession(ctx)`
creates the ProcUnits against the shared bytecode and attaches them;
`ExitRuntimeForSession(ctx)` tears down. Called from `ibWebSession::Login`
BEFORE `app->OnInit()` — OnStart resolves its ProcUnit via `GetProcUnit()`
delegate which reads `ibSession::Current()->GetProcUnitFor(this)`.

No `ibValueModuleManagerWebConfiguration` — web specifics live in
session lifecycle (frame ownership, worker thread, debug server),
not in the module manager.

## Binary split

```
wenterprise-server.exe       <- HTTP host (cpp-httplib), one per deployment
└── wfrontend.dll             <- web-built frontend (OES_USE_WEB)
    ├── backend.dll           <- unchanged, shared with desktop
    └── fbclient.dll + plugins
```

`wfrontend.dll` is compiled from the **same source tree** as `frontend.dll`
but with `OES_USE_WEB` defined. UI-specific call sites take the web branch
(HTML/JSON) instead of the native wx branch. There is no `wxApp` loop.

Build in Visual Studio: `src/engine/frontend/wfrontend.vcxproj` (Win32
Debug). SolutionDir must be the enterprise root — props at the root
(`Common.props`, `ConfigurationDefs.props`) resolve `$(SolutionDir)`.

## Per-session isolation

Each HTTP session gets its own `ibWebSession` owning an `ibWebApplication`.
`ibWebApplication` owns the **session-scoped module manager** (with its
own compiled bytecode, `ibProcUnit`, common modules, context variables)
and the `ibWebFrame` (logical MDI root).

The desktop singleton `ibBackendDocMDIFrame::GetDocMDIFrame()` is
per-thread in the web build (`thread_local` in
`src/engine/backend/backend_mainFrame.cpp`). One HTTP request handler
thread = one logical "desktop app" session.

## Frame / tabs / form tree

```
ibWebFrame : ibBackendDocMDIFrame, ibWebWindow
├── m_tabs : vector<unique_ptr<ibWebDocChildFrame>>
│   └── each owns:
│       ibVisualHostClient           <- ibValuePtr<ibValueForm> m_valueForm
│       └── ibValueForm
│           └── control tree (ibValueFrame subtree)
└── m_activeForm : ibBackendValueForm*   (borrowed; host owns the ref)
```

Closing a tab = `vector::erase` on `m_tabs`. Dtor chain drops
`ibVisualHostClient` → `ibValuePtr<ibValueForm>` → when no other ref, the
form and its control tree are freed.

## JSON walk

`ibVisualHostClient::WalkToJSON` (in `visualHost.cpp`) walks the form's
control tree and, for every `ibValueFrame`, calls `Create(webParent, host)`
which constructs a matching `ibWebWindow` node (`ibWebBoxSizer`,
`ibWebButton`, `ibWebStaticText`, ...). The walker then serialises that
parallel `ibWebWindow` tree into JSON. The browser's JS layer
(`wenterprise-server/main.cpp`, inline `<script>`) renders each node
through a per-type `BaseControl` subclass registered in a `renderers{}`
map.

## Control registry (name-based factory)

Metadata-backed form deserialisation (`ibValueForm::LoadForm` →
`ibValueForm::NewObject(clsid, ...)`) resolves control classes by
`ibClassID` using the same registry as desktop, populated through
`CONTROL_TYPE_REGISTER` statics in `visualView/ctrl/*.cpp`. At time of
writing, wfrontend.dll has ~34 ctors visible (`/diag/ctors` to enumerate).

The typed path (`CreateAndPrepareValueRef<T>`) used by `CreateNewForm` and
the `/demo` endpoint bypasses the registry entirely — it does not load
metadata and does not populate property defaults. That's why direct API
form creation works even when the registry is missing control types.

## DB connection pool

`ibApplicationData::m_db` is the single shared `ibDatabaseLayer` used
by the main thread (`db_query` macro points at it). For additional
worker threads that need their own connection — session heartbeat
writer, metadata-watcher SELECT, future SSE streams and per-session
script workers — `ibConnectionPool` (`src/engine/backend/databaseLayer/connectionPool.{h,cpp}`)
lends clones out of a bounded set.

Lifecycle: `Init(prototype, maxSize=32)` at
`CreateFile/ServerAppDataEnv` time, `Shutdown` at `DestroyAppDataEnv`.
Clones are allocated lazily on first `Checkout` so a single-threaded
run (designer, classChecker) opens zero extra FB handles. A
`shared_ptr`'s custom deleter re-parks the clone on the pool's idle
list when the last borrower drops it — the connection stays open for
the next checkout. `ibConnectionScope` is the RAII guard.

The metadata-change watcher uses the same pool: every sweep tick
borrows one connection, runs `SELECT file_guid FROM sys_config`, and
returns the clone. On guid change it evicts all sessions and does
`CloseDatabase(forceCloseFlag) + LoadDatabase + RunDatabase` on
`activeMetaData` to recompile against the deployed config; the next
login spins up user runtime on the fresh bytecode.
