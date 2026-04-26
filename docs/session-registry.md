# Session registry refactor — full picture

Полный reference по session-registry'му рефактору 2026-04-20. Ниже — архитектура, что landed по шагам, все найденные нюансы и gotchas, что осталось.

## Цель

Заменить старый `ibApplicationDataSessionUpdater` (heartbeat thread на singleton `ibApplicationData` с 1Hz UPDATE своей строки в `sys_session`) на:

- **Per-process session registry** (`ibSessionRegistry`) с единым consumer-thread'ом и priority queue.
- **Per-session объект** (`ibSession`) с state-machine (lifecycle + auth) и cv для producer'ов.
- **Ticket RAII** (`ibSessionTicket`) — owner handle, dtor submits Remove@Urgent.
- **Unified `Connect(req)` entry** для desktop / designer / web-server / web-cookie.
- **Policy chain** (`ibSessionPolicy`) вместо scattered veto hooks.
- **Session-aware user accessors** на `ibApplicationData` — закрывает web multi-tab "last login wins" bug.

## Основные компоненты

| Файл | Роль |
|---|---|
| `backend/session/session.{h,cpp}` | `ibSession` + state enums + `SessionScope` (legacy thread-local). |
| `backend/session/sessionRegistry.{h,cpp}` | Registry class: thread + priority queue + tick loop + DB ops. |
| `backend/session/sessionTicket.{h,cpp}` | RAII ticket — move-only owner. |
| `backend/session/sessionPolicy.h` | Pure interface для `CanAdd`-veto. |
| `backend/session/designerExclusivePolicy.{h,cpp}` | Ports `VerifySessionUpdater`. |

## Lifecycle (desktop)

The phased entry points on `ibApplicationData` are
`CreateSession()` (or typed `CreateSession<SessionT>()`) plus the
session's own `Open(user, pwd)`. The legacy monolithic `StartSession`
is gone — failed `Open` keeps the anonymous row in `sys_session`
until the caller drops the ticket explicitly, which lets the GUI
login-retry loop reuse one row across attempts.

```
ibSession* s = appData->CreateSession<ibEnterpriseSession>();
│   ├── registry.EnableSysSessionOwnership(true)         # one-shot
│   ├── (designer only) registry.AddPolicy(DesignerExclusivePolicy)
│   ├── registry.EnsureStarted()                         # spawn thread,
│   │                                                    # 3 pool checkouts
│   │                                                    # (lock / write / probe)
│   ├── registry.CreateSessionWithFactory(...)            # builds typed ibSession
│   ├── registry.Connect(req)                             # Submit(Add, Normal) → Wait
│   │   └── ProcessAdd:
│   │       ├── policy chain
│   │       ├── m_own[guid] = session
│   │       └── InsertSessionRow (+ ext UPDATE)
│   └── OnCreateSession() fires on main thread            # GUI subclass
│                                                          # builds wx frame here
│
s->Open(user, pwd)
│   ├── submitAttach(user, pwd) → ProcessAttach
│   │   ├── appData->AuthenticateUser(…)                 # pure verifier
│   │   ├── ibSessionScope(&s) + appData->InstallUser     # session-aware writer
│   │   └── UPDATE userName / userGuid
│   ├── (on Attach fail) OnShowAuthenticate              # GUI dialog override
│   │   └── re-submitAttach with dialog creds
│   └── registry.NotifyAuthenticated(s)                   # 3-phase (see below)
```

### NotifyAuthenticated phases

`ibSessionRegistry::NotifyAuthenticated` orchestrates session bring-up in three strict phases. Listeners that depend on later state must hook the right phase.

```
NotifyAuthenticated(s):
  1. OnFirstConnect listeners (one-shot per process):
       appData lambda → metaDataCreate(runMode, flags)
                        # populates the activeMetaData singleton

  2. s->EnsureRoot()        # idempotent CreateRoot(activeMetaData)
                            # session's own root mm allocated NOW
                            # — so step 3 listeners see GetModuleManager() != null

  3. OnAuthenticated listeners (every authenticated session):
       appData lambda:
         ├── BindSessionToThread
         ├── (Shared mode + no fallback) registry.SetFallback(s)
         ├── (one-shot) activeMetaData->RunDatabase()
         │     # fires OnBefore/AfterRunMetaObject which reach into
         │     # session->GetModuleManager() and the metadata-side
         │     # ibCompileValueCache — both required to be live by now
         ├── s->CompileRoot()
         └── (runtime modes) mm->InitRuntimeForSession(s)
```

**Why three phases, not two.** The pre-2026-04-26 layout fired `OnFirstConnect` then `OnAuthenticated` directly. `CreateRoot` lived in `appData`'s `OnAuthenticated` listener, and `RunDatabase` was nominally below it but ordering was easy to flip. The crash that drove this refactor was `OnBeforeRunMetaObject` reading `ibSession::Current()->GetModuleManager()` while `RunDatabase` was iterating — mm null because `CreateRoot` hadn't run yet on the very first session. Putting `EnsureRoot` between phases makes the contract explicit: every `OnAuthenticated` listener can rely on `s->GetModuleManager()` being non-null when `activeMetaData` is set.

**Where `CreateRoot` lives.** In `ibSession`. The session is the owner of its root mm; the registry just calls the hook at the right moment. `appData`'s `OnAuthenticated` no longer touches `CreateRoot` — only `RunDatabase`/`CompileRoot`/`InitRuntimeForSession`.

## Lifecycle (web per-cookie)

```
GET /                                            # cookie mint (id = new guid)
└── SessionManager::Create → ibWebSession(id)
    └── OnInit — metadata name; no runtime yet

POST /login { user, password }
└── ibWebSession::Login(user, pwd)
    ├── ibConnectRequest req { computer, eENTERPRISE_MODE,
    │                          address = wfrontendServerAddress() }
    ├── registry.Connect(req)                    # anonymous row INSERT
    ├── m_ticket = move(result.ticket)
    ├── ticket.Attach(user, pwd)                 # UPDATE userName
    ├── SessionScope(ticket->Session()) on HTTP thread
    ├── InitRuntimeForSession(ticket->Session()) # ProcUnits for this cookie
    └── app->OnInit() → StartMainModule → OnStart script fires
```

## Thread model

**Registry thread** (single consumer) owns:
- `m_own : unordered_map<guid, shared_ptr<ibSession>>` — sessions this process tracks.
- 3 pool-checkout'ed connections:
  - `m_lockConn` — reserved, не использовать для WITH LOCK-hold'а (см. гиотча-dictionary ниже).
  - `m_writeConn` — INSERT / UPDATE / DELETE + JobRefreshSnapshot SELECT.
  - `m_probeConn` — `TryProbeRowLock` NOWAIT probe.
- `m_snapshot` под RW mutex — публично через `GetClusterSnapshot()`.
- Priority queue bins (Urgent / Normal / Low / Background).

**Tick schedule:**

| Interval | Job |
|---|---|
| 1s | `JobHeartbeatOwn` (UPDATE own lastActive), `JobRefreshSnapshot` (SELECT * → snapshot). |
| 3s | `JobSweepStale` (delete zombies via probe-lock ИЛИ lastActive cutoff). |
| On-demand | drain queue (Add/Attach/Detach/Remove/SetActivity) — strict descending priority. |
| Eager on Start | one initial sweep + refresh so UI has data immediately. |

**Fatal invariant:** если ThreadBody вылетит с exception — `Die()` → `std::terminate`. Registry-thread must-be-alive; в противном случае sys_session больше не reflects правду, других процессов ClearLost'ят наши row'ы.

## Gotchas & нюансы (всё что вскрылось)

### 1. Start/Connect race

**Симптом:** после `reg.Start()` немедленный `reg.Connect()` возвращает `RegistryDown`.

**Причина:** `Start()` возвращался СРАЗУ после `m_thread = std::thread(...)`, до того как ThreadBody выставит `m_threadAlive = true`. Connect's top check видел `!m_threadAlive` → `RegistryDown`.

**Фикс:** `Start()` делает короткий spin (≤2s) пока thread не поднимет alive-флаг.

### 2. Attach-empty-creds deadlock

**Симптом:** вызов `ticket.Attach("", "")` (CLI без creds) виснет на timeout → `ibAttachResult::Timeout`.

**Причина:** `Attach` делал `TransitionAuth(Anonymous)` reset → Submit Attach → `WaitForAuth(from=Anonymous, ...)`. В ProcessAttach early-return'овая ветка для empty creds делала ещё один `TransitionAuth(Anonymous)` — state не менялся, predicate `state != Anonymous` never flipped, 20s waiting.

**Фикс:** убрал early-return в ProcessAttach. Всегда идёт через `AuthenticateUser` — на open-access (sys_user пустой, creds пустые) transitions на `Authenticated` с empty info.

### 3. Launcher/designer stuck в tasklist после close

**Симптом:** после закрытия окна enterprise.exe / designer.exe остаются в tasklist, держат backend.dll.

**Причина:** `ibApplicationData::Disconnect()` вызывал `registry.Stop()` только если `m_created_metadata` + все промежуточные проверки прошли. Ранний `return false` (например `CloseDatabase` failed) пропускал Stop → thread live → process alive.

**Фикс:** Stop() вызывается безусловно в конце Disconnect. Success flag собирается но не влияет на вызов Stop.

### 4. HoldRowLocks self-deadlock (КРИТИЧНО)

**Симптом:** HTTP запросы виснут на бесконечный timeout, Active Users empty, registry thread как будто мёртв.

**Причина:** схема "row-lock = liveness" пыталась держать `SELECT ... WITH LOCK` long-TX на наших своих row'ах через `m_lockConn`. Одновременно `JobHeartbeatOwn` пытался `UPDATE lastActive` на тех же row'ах через `m_writeConn`. FB видит две независимые TX, из которых первая держит update-intent lock → вторая ждёт. Вторая — на registry thread'е. Thread зависает → ничего не обновляется.

**Фикс:** схема переведена на **heartbeat-based liveness**.
- `HoldRowLocks` больше не вызывается (`RebuildLockHold` удалён из ProcessAdd/Remove).
- `JobHeartbeatOwn` UPDATE'ит lastActive каждую секунду на своих inserted row'ах.
- `JobSweepStale` использует hybrid: `TryProbeRowLock` как fast path (clean exits), `lastActive < now - 60s` как fallback (force-kill'ed processes, orphan TX в FB не rollback'нут).

### 5. Force-kill'ed процессы → persistent zombies

**Симптом:** после kill -9 enterprise.exe в sys_session остаётся его row, designer'ова Active Users показывает "мёртвого" пользователя.

**Причина:** FB embedded не делает orphan-rollback при каждой операции. TX убитого процесса может остаться в "active" state до следующего full DB reopen / gfix. Наш probe-lock пробует WITH LOCK NOWAIT на занятой row'е → conflict → думаем что owner alive.

**Фикс:** `JobSweepStale` использует lastActive-cutoff 60s как fallback. После 60s без heartbeat'а row считается zombie регардлесс. Eager sweep сразу на Start() убирает zombies от прошлых запусков.

### 6. Schema migration для existing DBs

**Симптом (предполагаемый):** на production DB (Configuration на PG) Active Users пуст после cutover. Новые колонки pid/address/currentActivity не существуют в legacy schema. `InsertSessionRow` с 9 колонок падает → row не пишется.

**Фикс:** `InsertSessionRow` делает 6-колоночный INSERT (core columns всегда присутствуют), затем отдельный `UPDATE SET pid=?, address=? WHERE session=?` — если schema legacy, UPDATE тихо провалится, INSERT остаётся валидным. `MigrateTableSession` вызывается на CreateFile/ServerAppDataEnv, пробует `ALTER TABLE ADD COLUMN` per missing column; каждый ALTER в try/catch — если driver не поддерживает (редко), просто не добавляется.

### 7. Session-aware user accessors

**Суть:** web HTTP worker thread работает под `SessionScope(cookieSession)`. Singleton `appData->GetUserName()` раньше читал `m_userInfo` — last-login-wins на multi-tab. Теперь:

```cpp
const wxString& ibApplicationData::GetUserName() const {
    return GetUserInfo().m_strUserName;
}
const ibApplicationDataUserInfo& ibApplicationData::GetUserInfo() const {
    if (auto* ctx = ibSession::Current())
        return ctx->GetUserInfo();   // per-session mirror
    return m_userInfo;                // fallback (pre-auth, codeRunner)
}
```

25+ call-site'ов подхватили поведение автоматически. Singleton поля остались как fallback — не удалены (это cleanup на потом).

### 8. Per-driver NoWait plumbing

`ibTxOptions::noWait = true` в `BeginTransaction` теперь honoured во всех 4 драйверах:
- **FB**: `isc_tpb_nowait` в TPB.
- **PG**: `SET LOCAL lock_timeout = 0` после BEGIN.
- **MySQL/InnoDB**: `SET SESSION innodb_lock_wait_timeout = 1`.
- **ODBC/MSSQL**: `SET LOCK_TIMEOUT 0`.
- **SQLite**: no-op (single-process).

Concrete `HoldRowLocks` / `TryProbeRowLock` реализации — пока только FB. На других драйверах base class возвращает false → registry работает на heartbeat-cutoff схеме без probe-lock fast path.

### 9. Cookie / session guid unification

**Note:** web cookie value на данный момент отдельный 32-hex random из `wfrontend.cpp::newSessionId()`, не равен ibSession's guid. Легаси поведение. В будущем стоит выдавать ibGuid прямо в cookie — единый id везде.

## Что сделано (список commit'ов)

1. **Skeleton rename** — `ibSessionContext → ibSession`, `SessionManager → ibSessionRegistry` (18 files).
2. **DB row-lock API** — `HoldRowLocks / TryProbeRowLock / ReleaseRowLocks` на `ibDatabaseLayer`; `BeginTransaction(const ibTxOptions&)`; FB concrete impl.
3. **Registry thread + priority queue** — Submit/DrainAll strict descending, fatal invariant, `IsThreadAlive/IsFatal`.
4. **Ticket + Connect + state machine** — `ibSession::Transition/WaitForState`, `ibSessionTicket`, `ibConnectRequest/Result`, unified entry.
5. **Auth split** — `AuthenticateUser` pure + `InstallUser` side-effect; `ProcessAttach` routes through them.
6. **DB ops в handlers (gated)** — INSERT/UPDATE/DELETE через `m_writeConn`; `m_ownsSysSession` flag.
7. **Cutover (desktop)** — StartSession через registry; `ibApplicationDataSessionUpdater` deleted (~370 lines); `GetSessionArray` → `GetClusterSnapshot`.
8. **Bugfixes** — Start/Connect race (#1), Attach-empty-creds deadlock (#2).
9. **Web wiring** — `ibWebSession::Login` через ticket; smoke test end-to-end (GET /, POST /login, GET /session).
10. **DesignerExclusivePolicy** — probe-lock based replacement для `VerifySessionUpdater`.
11. **Session-aware accessors** — `GetUserInfo / GetUserName / GetUserPassword / GetUserRoleArray / GetUserLanguageGuid / GetUserLanguageCode / ComputeMd5` out-of-line, Current()-first.
12. **Schema extension** — pid / address / currentActivity columns + `MigrateTableSession` + `ProcessSetActivity` real UPDATE + `ibSessionTicket::SetActivity`.
13. **Per-driver NoWait** — PG / MySQL / ODBC плюс FB.
14. **wfrontend server address plumbing** — `wfrontendSetServerAddress/ServerAddress` exports; `ibWebSession::Login` stamps `sys_session.address`.
15. **Disconnect force-Stop** — Stop() вызывается безусловно, force-kill процесса больше не оставляет зомби-thread.
16. **HoldRowLocks removed** (#4 fix) — liveness = heartbeat + 60s cutoff + probe as fast path. Hybrid sweep (JobSweepStale).
17. **Split timers** — 1s refresh (UI отклик), 3s sweep (cluster cleanup).
18. **Eager initial sweep + refresh** — Active Users не пуст на старте.
19. **INSERT split (6-col + ext-UPDATE)** — legacy-schema tolerant.

## Что осталось (приоритет ↓)

- Concrete `HoldRowLocks / TryProbeRowLock` для PG / MySQL / MSSQL.
- `signal` column в sys_session + admin kick/reload dispatcher + `/admin/sessions` endpoint.
- Snapshot SELECT читает новые колонки в `ibApplicationDataSessionArray` (pid/address/currentActivity accessors).
- Cookie / ibGuid unification на web (сейчас cookie отдельно).
- Full removal singleton `m_userInfo/m_sessionGuid/m_sessionRawPassword` полей (сейчас dual-write+fallback).
- Remove `SessionScope::Current()` legacy thread-local (после migration `AppUser()`-style built-ins на `ibProcUnit::GetSession()`).
- Real auth в web login — сейчас принимает любой user/pwd на open-access DB; для populated sys_user логика уже через `AuthenticateUser`.
- Interactive verification: designer-exclusive policy под двумя designer.exe одновременно.
- Designer Active-Users UI: колонка «Тип» по `ibSessionKind` (WebServer / WebClient / Enterprise / Designer / Service). Данные уже в snapshot через `GetSessionKind(idx)`.

## ibSessionKind (landed 2026-04-20)

`ibSession` no longer carries `m_runMode` — process-level info lives on `appData` only. The session-level axis is a separate enum:

```cpp
enum class ibSessionKind : int {
    Launcher   = eLAUNCHER_MODE,
    Designer   = eDESIGNER_MODE,
    Enterprise = eENTERPRISE_MODE,
    Service    = eSERVICE_MODE,
    WebServer  = eWEB_ENTERPRISE_MODE,   // wes process technical row
    WebClient  = 100,                     // per-tab / API caller
};
```

- Desktop `appData->CreateSession()` → `SessionKindFromRunMode(m_runMode)` (wes process passes `WebServer` explicitly via the typed factory).
- `ibWebSession::Login` → `ibSessionKind::WebClient` + `m_appMode = eWEB_ENTERPRISE_MODE`.
- `sys_session.kind` is added via `MigrateTableSession` (best-effort ALTER TABLE); legacy schemas without it still work (kinds read as 0).
- `moduleManager::InitRuntimeForSession` filters by kind: runtime runs for `Enterprise / WebClient / Service`, skipped for the rest.

## wes console-close cleanup

`wenterprise-server/main.cpp` installs `SetConsoleCtrlHandler` (Windows) and `SIGINT/SIGTERM` (POSIX) so the sys_session row is DELETEd when the console closes. For `CTRL_CLOSE_EVENT / LOGOFF / SHUTDOWN` (Windows grants 5s before force-kill) shutdown runs inline in the handler thread (`wfrontendShutdown()` + `ExitProcess(0)`) rather than via main's return — `listen_after_bind` on Windows doesn't always unblock fast enough after `svr.stop()`.

## HTTP worker threads: SessionScope + per-tab session id

Two independent foot-guns, landed 2026-04-20:

**1. SessionScope on worker threads.** cpp-httplib dispatches handlers on worker threads where `ibSession::Current()` is null by default. `ibModuleDataObject::GetProcUnit()` then returns nullptr, the form gets `SetParent(nullptr)`, and `Execute` throws `ibBackendCoreException("compilation failed (#2)")`. Any handler that touches forms/modules must pin the session explicitly:
```cpp
SessionScope scope(webSession->Session());
```
Current sink is `OpenFormInSession` in `frontend/wfrontend.cpp`. Other per-session handlers (/action, /change, /fire, /toggle, /tab, /form) should be checked if "compilation failed" recurs on web.

**2. Per-tab session id routing.** JS in `webClient.cpp` generates a `tabSid` in sessionStorage (per-browser-tab), independent of the server's `oes_session` cookie. netFetch attaches it via the `X-OES-Session` header. Two subtleties:
- **POST /login must create a session for unknown ids** (`wfrontendCreateSessionWithId`). A fresh tab can first hit the server through /login instead of GET /; strict 401 on unknown id breaks login.
- **`<img src>` / `<script src>` cannot carry custom headers** — only cookies. For plain-GET per-tab endpoints (e.g. `/tab/<i>/icon`) the URL must include `?sid=<tabSid>`. `SessionIdFromReq` reads the query param as a fallback between header and cookie.

## PNG handler registration

`wenterprise-server.exe` itself doesn't link against the PNG codec (wxBase-only). The PNG handler is registered inside `wfrontendInitFile` / `wfrontendInitServer` via `wxImage::AddHandler(new wxPNGHandler)`. Without it, `wxImage::SaveFile(..., wxBITMAP_TYPE_PNG)` in `TabIconPNG` returns false, `/tab/<i>/icon` replies 404, and the browser `<img>` renders as broken. The backend's `picturePredefined.cpp` also calls `wxInitAllImageHandlers` for its own paths.

## Tab-close beacon

`frontend/web/webClient.cpp` adds a `pagehide` listener that fires `navigator.sendBeacon(API + '/logout?sid=' + encodeURIComponent(tabSid))` when the tab is closing or navigating away. The browser queues the beacon and delivers it even after the page unloads, so the server DELETEs the session row immediately rather than waiting for the 2-minute idle sweep in `SessionManager::SweepLoop`.

The historic concern — reload-GET racing a beacon-destroyed session — no longer applies: `GET /` re-mints sessions for unknown ids, so beacon+reload lands on a clean same-id session. `sendBeacon` runs through a try/catch so throwing browsers don't break the rest of pagehide, and the 2-min idle cutoff remains as a fallback for Safari edge cases, Task Manager kill, or tab crashes where the beacon never fires.

## Admin signals (kick / reload)

`sys_session.signal VARCHAR(32)` is a cross-process control channel
added by `MigrateTableSession` (legacy schemas without it are tolerated —
the column simply stays `NULL` and `JobCheckSignal` skips work). Every
owning process's registry thread polls `signal` for its own rows on the
sweep tick (~3s), acts on non-empty values, and clears the cell:

- `"kick"` — submits `Remove@Urgent` for that specific session; ticket
  drops → row DELETE.
- `"reload"` — flips `m_reloadRequested` on the registry.
  `ConsumeReloadRequest()` is a one-shot consume used by embedders
  (wfrontend's `SweepLoop`) to evict every web-client session the owner
  holds, forcing clients through `/login` against the freshly-loaded
  metadata.

Three entry points share one UPDATE path (`WriteSessionSignal` in
`sessionRegistry.cpp`):

- C++ from any thread: `ibSessionRegistry::Instance().Kick(guid)` /
  `Reload(guid)`.
- HTTP on wes: `POST /admin/sessions/<guid>/kick` and
  `POST /admin/sessions/<guid>/reload`.
- Designer dialog: right-click a row in Active Users → `Kick session`
  / `Reload clients`.

## Web auth form

`GET /auth-info` returns `{"hasUsers": bool}` — a lightweight metadata
probe that the web client uses on boot to choose between the
open-access fast path (empty `sys_user` → `POST /login` with empty
creds) and a credentials prompt. The prompt lives in `#authOverlay`
inside `webClient.cpp`; it loops until `/login` accepts the creds, then
reveals the rest of the UI together with the bootOverlay.

The server-side auth chain is the same whichever path fires:
`/login` → `wfrontendLogin` → `ibWebSession::Login` → `reg.Connect` +
`ticket.Attach` → `ProcessAttach` → `appData->AuthenticateUser` (PBKDF2
with MD5 fallback for legacy databases) → `InstallUser` (writes to
both the session's `m_userInfo` and the legacy singleton mirrors).

## Unified session id across web layers

`sessionStorage.oes_tab_sid` (browser), `oes_session` cookie, the
`X-OES-Session` header, `SessionManager::m_sessions` key,
`ibSession::m_id`, and the `sys_session.session` PK all carry the same
dashed-UUID string (`xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`).
`ibConnectRequest::m_presetGuid` lets the producer hand a specific id
to `Connect`; the web session path fills it with the client-supplied
tab id so the cluster's row ids visibly match what the browser sees.

## TryProbeRowLock across drivers

The designer-exclusive policy calls
`ibSessionRegistry::ProbeSessionRowLock(guid)`, which delegates to the
driver's `TryProbeRowLock`. Implementations:

- **Firebird** — `SELECT ... WITH LOCK` inside a NOWAIT-configured TX.
- **PostgreSQL** — `SELECT ... FOR UPDATE NOWAIT`.
- **MySQL** — session-level `innodb_lock_wait_timeout = 1` plus
  `SELECT ... FOR UPDATE NOWAIT` (NOWAIT keyword on MySQL 8+, timeout
  is the fallback).
- **ODBC** (targeting MSSQL) — `SET LOCK_TIMEOUT 0` plus
  `SELECT ... WITH (UPDLOCK, ROWLOCK)`.
- **SQLite** — file-level locks only, probe stays a no-op; policy
  falls back to the `lastActive` cutoff.

All implementations wrap the probe in their own TX and ROLLBACK before
returning, so no lock survives the call.

`IsActiveTransaction` now reads `m_transaction_is_active` on the base
class; every driver flips the flag in `BeginTransaction` /
`Commit` / `RollBack`. Firebird keeps its native-handle override as a
belt-and-suspenders check.

## Session list label

`ibApplicationDataSessionArray::GetApplication(idx)` must disambiguate WebServer vs WebClient on web because both share `ibRunMode::eWEB_ENTERPRISE_MODE`. Implementation: when the stored run-mode is `eWEB_ENTERPRISE_MODE`, pick "Web client" if `m_kind == ibSessionKind::WebClient (100)`, otherwise "Web server". Other run modes fall through to `GetRunModeDescr`. `m_kind` is kept as `int` in `ibApplicationDataSessionUnit` to avoid pulling `session.h` into `appData.h`.

## Testing

**Smoke test на embedded FB (fb_test251):**

```bash
cd bin/Win32/Debug
./wenterprise-server.exe --file=F:/projects/oes-bin/examples/fb_test251 --port=8080 &
# expect: "OES wenterprise-server listening on http://localhost:8080/w/fb_test251/"

curl -s -c /tmp/ck http://127.0.0.1:8080/w/fb_test251/                      # 200, HTML shell
curl -s -b /tmp/ck -X POST http://127.0.0.1:8080/w/fb_test251/login \
  -d "user=&password="                                                       # 200, "ok (<cookie> / )"
curl -s -b /tmp/ck http://127.0.0.1:8080/w/fb_test251/session                # 200, authenticated=true tabCount=2
```

**Smoke test на server-mode PG (Configuration):** same pattern, замени `--file=` на `--server=... --db=Configuration`.

## Связанные memory-заметки

- `project_session_registry_refactor.md` — история дизайна + полный список commit'ов.
- `project_web_session_bug.md` — старый bug (multi-tab last-login-wins), закрытый commit'ом 11.
- `feedback_no_passwords_in_db.md` — policy: хэш только, plain-text in-memory only.
- `reference_session_raw_password.md` — why we still need `m_sessionRawPassword`.
- `reference_empty_username_meanings.md` — 3 кейса пустого userName в sys_session.
