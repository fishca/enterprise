# OES Architecture

## Table of Contents

1. [System Overview](#system-overview)
2. [Layer Descriptions](#layer-descriptions)
3. [Module Descriptions](#module-descriptions)
4. [Bytecode Engine](#bytecode-engine)
5. [Metadata System](#metadata-system)
6. [Sessions and Runtime Ownership](#sessions-and-runtime-ownership)
7. [Database Abstraction](#database-abstraction)
8. [Form System](#form-system)
9. [Debugger Architecture](#debugger-architecture)
10. [Key Data Flows](#key-data-flows)

---

## System Overview

```
┌──────────────────────────────────────────────────────────────┐
│                      Executables                             │
│  designer.exe   enterprise.exe   launcher.exe   daemon.exe   │
│  codeRunner.exe   classChecker.exe                           │
└────────────┬─────────────────┬────────────────┬─────────────┘
             │                 │                │
             ▼                 ▼                ▼
┌────────────────────┐  ┌────────────────────────────────────┐
│   frontend.dll     │  │           backend.dll              │
│                    │  │                                    │
│  ibValueForm       │◄─►  ibApplicationData (singleton)     │
│  ibVisualHost      │  │  ibMetaDataConfiguration           │
│  22 Controls       │  │  ibCompileCode / ibProcUnit        │
│  ibMainFrame       │  │  ibDatabaseLayer (+ 5 drivers)     │
│  Property editor   │  │  ibDebuggerServer                  │
└────────────────────┘  └────────────────┬───────────────────┘
                                         │
                        ┌────────────────▼────────────────────┐
                        │         Database tier                │
                        │  Firebird  PostgreSQL  SQLite        │
                        │  MySQL     ODBC                      │
                        └─────────────────────────────────────┘
```

Communication between `frontend.dll` and `backend.dll` goes through abstract C++ interfaces exported from `backend.dll`. The frontend never accesses database drivers or the compiler directly.

---

## Layer Descriptions

### Application Layer (executables)

Each executable links against both DLLs and provides a `wxApp` subclass that selects the run mode:

| Executable | Run Mode (`ibRunMode`) | Purpose |
|---|---|---|
| `launcher.exe` | `eLAUNCHER_MODE` | Connection chooser; creates/selects database |
| `designer.exe` | `eDESIGNER_MODE` | Full IDE — metadata editor, form designer, debugger client |
| `enterprise.exe` | `eENTERPRISE_MODE` | Desktop thick-client runtime (GUI, single user session per process) |
| `wenterprise-server.exe` | `eWEB_ENTERPRISE_MODE` | Web runtime host — HTTP server, N per-cookie user sessions, browser client |
| `daemon.exe` | `eSERVICE_MODE` | Headless background service |
| `codeRunner.exe` | `eSERVICE_MODE` | Executes a single script module |
| `classChecker.exe` | — | Validates metadata consistency |

> A rename `eENTERPRISE_MODE → eRUNTIME_MODE` is planned — the current name misleads, both thick-client and web hosts are "runtime", just with different UI transports. The constants stay as-is until the rename lands.

### Backend Layer (`backend.dll`)

The backend is the core engine. It is self-contained — no GUI dependencies. Key objects:

- **`ibApplicationData`** (`src/engine/backend/appData.h`) — master runtime object; holds database connection pool, run mode, and application metadata. Accessed via the `appData` macro. No longer holds per-session state after the session/runtime split (user info, ProcUnits, frame all moved to `ibSession`).
- **`ibMetaDataConfiguration`** (`src/engine/backend/metadataConfiguration.h`) — loads, saves, and manages the metadata tree (all business objects). Accessed via `activeMetaData`. Stores compile cache (compiled bytecode) per module descriptor; runtime instances live in sessions.
- **`ibSession` / `ibSessionRegistry`** (`src/engine/backend/session/`) — per-session state and process-wide session manager. Every script-executing thread runs under a `SessionScope` pinning one `ibSession*` to thread-local. See [Sessions and Runtime Ownership](#sessions-and-runtime-ownership).
- **`ibDebuggerServer`** (`src/engine/backend/debugger/debugServer.h`) — TCP server that accepts designer connections and relays debugger events.

### Frontend Layer (`frontend.dll`, `wfrontend.dll`)

Two sibling DLLs share the same form/view/control code paths through `OES_USE_WEB` ifdefs and `ibFrontendWindow` typedef (`wxWindow` for desktop, `ibWebWindow` for web):

- **`frontend.dll`** — wxWidgets GUI. Used by `enterprise.exe`, `designer.exe`, `launcher.exe`, `daemon.exe`, `codeRunner.exe`.
- **`wfrontend.dll`** — web UI (HTML serialisation of form control trees via `ToJSON()`, cpp-httplib transport). Used by `wenterprise-server.exe`.

Shared frontend objects:
- **`ibValueForm`** — the runtime representation of an open form; holds the control tree and responds to user events. Same class on both builds.
- **`ibVisualHost` / `ibVisualHostClient`** — render and input routing surface. Desktop = wxWindow; web = `ibWebWindow` tree serialised to JSON.
- **`ibFrontendDocParentFrame` / `ibFrontendDocChildFrame`** — document-view framework wrappers (wxWidgets on desktop, `ibWebFrame` / `ibWebDocChildFrame` on web). Template-mixin rewrite in progress to follow `wxDocParentFrameAny` pattern.

---

## Module Descriptions

### `src/engine/backend/compiler/`

| File | Class | Role |
|---|---|---|
| `translateCode.h/cpp` | `ibTranslateCode` | Lexer: tokenises source text into `ibLexem` stream |
| `compileCode.h/cpp` | `ibCompileCode` | Parser and code generator: consumes lexemes, emits `ibByteCode` |
| `byteCode.h` | `ibByteCode`, `ibByteUnit` | Bytecode container: array of `ibByteUnit` instructions |
| `procUnit.h/cpp` | `ibProcUnit` | Interpreter: executes `ibByteCode` against a variable stack |
| `procContext.h/cpp` | `ibRunContext` | Execution context: local variable frame, call stack |
| `value.h/cpp` | `ibValue` | Universal value type (Undefined, Boolean, Number, Date, String, Reference) |
| `codeDef.h` | enums | Opcode (`OPER_*`) and keyword (`KEY_*`) definitions |

### `src/engine/backend/databaseLayer/`

| File | Class | Role |
|---|---|---|
| `databaseLayer.h/cpp` | `ibDatabaseLayer` | Abstract base: Open/Close/RunQuery/PrepareStatement/Transactions |
| `databaseResultSet.h/cpp` | `ibDatabaseResultSet` | Abstract result set cursor |
| `preparedStatement.h` | `ibPreparedStatement` | Abstract prepared statement |
| `firebird/` | `ibFirebirdDatabaseLayer` | Firebird 3/4 driver |
| `postgres/` | `ibPostgresDatabaseLayer` | PostgreSQL driver |
| `sqllite/` | `ibSqliteDatabaseLayer` | SQLite 3 driver (note: directory named `sqllite`) |
| `mysql/` | `ibMysqlDatabaseLayer` | MySQL driver |
| `odbc/` | `ibOdbcDatabaseLayer` | ODBC generic driver |

### `src/engine/backend/metaCollection/`

Metadata object hierarchy. Every business object type extends `ibValueMetaObject` (defined in `metaObject.h`), which itself extends `ibValue` so metadata objects can be passed as script values.

### `src/engine/backend/debugger/`

| File | Class | Role |
|---|---|---|
| `debugServer.h/cpp` | `ibDebuggerServer` | TCP server accepting designer connections |
| `debugClient.h/cpp` | `ibDebuggerClient` | Client-side connection (runs in designer) |
| `debugClientBridge.h/cpp` | `ibDebuggerClientBridge` | Bridge between client socket and UI |
| `debugDefs.h` | enums | `CommandId`, `EventId`, `ConnectionType` |

### `src/engine/backend/system/`

| File | Class | Role |
|---|---|---|
| `systemManager.h/cpp` | `ibSystemManager` | Built-in function dispatcher; registers 88 built-in functions |
| `systemEnum.h` | enums | System-level enumeration constants |

### `src/engine/frontend/visualView/`

| File/Dir | Content |
|---|---|
| `ctrl/form.h/cpp` | `ibValueForm`, form control collection, action wiring |
| `ctrl/control.h/cpp` | `ibControlFrame` base class for all controls |
| `ctrl/widgets.h` | Declarations for all control types |
| `visualHost.h/cpp` | `ibVisualHost` — wxWindow rendering surface |
| `visualHostClient.h/cpp` | Client-side form binding |
| `ctrl/` | Individual control implementations (see Form System) |

---

## Bytecode Engine

### Compiler Pipeline

```
Source text (wxString)
        │
        ▼
 ibTranslateCode::Translate()
   Lexer: produces vector<ibLexem>
   Each ibLexem: { type, keyword/delimiter number, string data, ibValue, line, position }
        │
        ▼
 ibCompileCode::Compile()
   Recursive-descent parser
   Emits ibByteUnit records into ibByteCode
   Resolves forward function references in a second pass
        │
        ▼
 ibByteCode
   vector<ibByteUnit>   — instruction stream
   map<wxString, ibByteFunction>  — function table (name → code line)
   vector<ibValue>      — constant pool
   vector<ibValue*>     — variable table
        │
        ▼
 ibProcUnit::Execute()
   Stack machine with ibRunContext frames
   Dispatches on ibByteUnit::m_numOper
```

### Opcode Categories

Opcodes are defined as plain integer constants in `src/engine/backend/compiler/codeDef.h`. The 66 opcodes fall into these groups:

| Category | Opcodes |
|---|---|
| Arithmetic | `OPER_ADD`, `OPER_SUB`, `OPER_MULT`, `OPER_DIV`, `OPER_MOD`, `OPER_INVERT` |
| Comparison | `OPER_GT`, `OPER_EQ`, `OPER_LS`, `OPER_GE`, `OPER_LE`, `OPER_NE` |
| Logical | `OPER_NOT`, `OPER_AND`, `OPER_OR` |
| Control flow | `OPER_GOTO`, `OPER_IF`, `OPER_FOR`, `OPER_FOREACH`, `OPER_IN`, `OPER_NEXT`, `OPER_NEXT_ITER` |
| Variables | `OPER_LET`, `OPER_CONST`, `OPER_CONSTN`, `OPER_SET`, `OPER_SETREF`, `OPER_SETCONST` |
| Functions | `OPER_FUNC`, `OPER_ENDFUNC`, `OPER_CALL`, `OPER_CALL_M`, `OPER_RET` |
| Arrays | `OPER_GET_ARRAY`, `OPER_SET_ARRAY`, `OPER_CHECK_ARRAY`, `OPER_SET_ARRAY_SIZE`, `OPER_ENTER_A`, `OPER_GET_A`, `OPER_SET_A` |
| Objects | `OPER_NEW`, `OPER_SET_TYPE` |
| Exceptions | `OPER_TRY`, `OPER_ENDTRY`, `OPER_RAISE`, `OPER_RAISE_T` |
| Optimised const variants | `OPER_ADDCONS`, `OPER_SUBCONS`, `OPER_MULTCONS`, `OPER_DIVCONS`, `OPER_MODCONS`, `OPER_GTCONS`, etc. |

Each opcode has type-specialised variants selected by adding `TYPE_DELTA1` (number), `TYPE_DELTA2` (string), `TYPE_DELTA3` (date), or `TYPE_DELTA4` (boolean) to the base opcode.

### Execution Model

`ibProcUnit` maintains a linked list of parent `ibProcUnit` objects representing the module scope chain. Each call creates an `ibRunContext` holding the local variable array. Exceptions use the throw-by-pointer pattern: `throw(new ibBackendInterruptException)`, caught as `catch(const ibBackendException*)`.

### Keyword Inventory

44 keywords defined in `KEY_*` enumerators (e.g., `KEY_IF`, `KEY_FOR`, `KEY_FOREACH`, `KEY_PROCEDURE`, `KEY_FUNCTION`, `KEY_TRY`, `KEY_EXCEPT`, `KEY_ENDTRY`, `KEY_RAISE`, `KEY_NEW`, etc.) plus preprocessor keywords (`KEY_DEFINE`, `KEY_UNDEF`, `KEY_IFDEF`, `KEY_IFNDEF`, `KEY_REGION`, `KEY_ENDREGION`). Two syntax modes allow English (`If…Then…EndIf`) and abbreviated forms.

---

## Metadata System

### ibValueMetaObject

`ibValueMetaObject` (defined in `src/engine/backend/metaCollection/metaObject.h`) is the abstract base for all configuration objects. It extends both `ibValue` (so it can be assigned to script variables) and `ibPropertyObjectHelper` (so properties appear in the designer's object inspector).

Key attributes:
- `ibMetaID m_metaId` — numeric identifier within the configuration
- `ibGuid m_metaGuid` — globally-unique GUID
- `wxString GetName()` / `GetSynonym()` — developer name and user-visible synonym
- `ibMetaData* m_metaData` — back-pointer to the owning metadata container

### Eleven Business Object Types

| Type | Class | File |
|---|---|---|
| Catalog | `ibValueMetaObjectCatalog` | `metaCollection/partial/catalog.h` |
| Document | `ibValueMetaObjectDocument` | `metaCollection/partial/document.h` |
| Enumeration | `ibValueMetaObjectEnumeration` | `metaCollection/partial/enumeration.h` |
| Constant | `ibValueMetaObjectConstant` | `metaCollection/partial/constant.h` |
| InformationRegister | `ibValueMetaObjectInformationRegister` | `metaCollection/partial/informationRegister.h` |
| AccumulationRegister | `ibValueMetaObjectAccumulationRegister` | `metaCollection/partial/accumulationRegister.h` |
| DataProcessor | `ibValueMetaObjectDataProcessor` | `metaCollection/partial/dataProcessor.h` |
| Report | `ibValueMetaObjectReport` | `metaCollection/partial/dataReport.h` |
| ChartOfCharacteristicTypes | `ibValueMetaObjectChartOfCharacteristicTypes` | `metaCollection/partial/chartOfCharacteristicTypes.h` |
| ChartOfAccounts | `ibValueMetaObjectChartOfAccounts` | `metaCollection/partial/chartOfAccounts.h` |
| AccountingRegister | `ibValueMetaObjectAccountingRegister` | `metaCollection/partial/accountingRegister.h` |

### Inheritance Chain (simplified)

```
ibValue
  └─ ibValueMetaObject
       ├─ ibValueMetaObjectAttribute         (scalar field descriptor)
       │    └─ ibValueMetaObjectConstant
       ├─ ibValueMetaObjectRecordData        (record-based objects)
       │    ├─ ibValueMetaObjectRecordDataMutableRef
       │    │    └─ ibValueMetaObjectDocument
       │    ├─ ibValueMetaObjectRecordDataHierarchyMutableRef
       │    │    ├─ ibValueMetaObjectCatalog
       │    │    ├─ ibValueMetaObjectChartOfCharacteristicTypes
       │    │    └─ ibValueMetaObjectChartOfAccounts
       │    └─ ibValueMetaObjectRecordDataExt
       │         ├─ ibValueMetaObjectDataProcessor
       │         └─ ibValueMetaObjectReport
       ├─ ibValueMetaObjectRegisterData
       │    ├─ ibValueMetaObjectInformationRegister
       │    ├─ ibValueMetaObjectAccumulationRegister
       │    └─ ibValueMetaObjectAccountingRegister
       └─ ibValueMetaObjectRecordDataEnumRef
            └─ ibValueMetaObjectEnumeration
```

### Configuration Persistence

`ibMetaDataConfiguration::LoadDatabase()` / `SaveDatabase()` serialise the entire metadata tree to/from the system database using `ibReaderMemory` / `ibWriterMemory` binary streams. Each metadata class registers a `ibClassID` CLSID (e.g. `MD_CAT` for Catalog) used as a type discriminator during loading.

### Text Format (XML / JSON)

Configurations can be exported and imported in text formats for Git VCS, AI generation, and human editing:

- **XML** (OES-XML-2.0): `metadataConfigurationXML.cpp` — `SaveConfigToXML()` / `LoadConfigFromXML()`
- **JSON** (OES-JSON-1.0): `metadataConfigurationJSON.cpp` — `SaveConfigToJSON()` / `LoadConfigFromJSON()` using nlohmann/json

Both formats support full round-trip serialization of all 11 metadata types with attributes, type qualifiers, tabular sections, forms, modules, predefined values, and all bindings.

### Accounting Objects (Work in Progress)

Three metadata types support double-entry bookkeeping. Core classes and designer integration are implemented; SQL DDL, runtime bindings, and Balance/Turnovers queries are under active development.

| Type | CLSID | Purpose |
|---|---|---|
| ChartOfCharacteristicTypes (MD_CHRC) | Subconto type definitions — each element stores `ibTypeDescription` with allowed value types |
| ChartOfAccounts (MD_CHOA) | Chart of accounts with AccountType (Active/Passive/AP), accounting signs, predefined SubcontoKinds tabular section |
| AccountingRegister (MD_AREG) | Double-entry register with Account, RecordType (Debit/Credit), Subconto1-3, Balance/Turnovers/DrCrTurnovers |

Bindings: AccountingRegister → ChartOfAccounts (via `ibPropertyChartOfAccounts`), ChartOfAccounts → ChartOfCharacteristicTypes (via `ibPropertyChartOfCharacteristicTypes`). Each binding has its own property class and advprop UI handler with CLSID-filtered selection dialog.

---

## Sessions and Runtime Ownership

OES distinguishes between **metadata** (compile-time, process-wide, shared) and **runtime state** (per-session, bound to one user context).

### ibSession

`ibSession` (`src/engine/backend/session/session.h`) is the unit of runtime state:

- **Identity** — guid, userName, computer, pid, address, appMode, started timestamp
- **State machine** — `ibSessionState` (Created / Added / Rejected / Stopping / Gone), `ibAuthState` (Anonymous / Authenticated / AuthFailed)
- **User info** — `ibApplicationDataUserInfo` — OES-user (from `sys_user` table), distinct from the DB-level admin user used to open the database connection
- **ProcUnit map** — `map<const ibModuleDataObject*, shared_ptr<ibProcUnit>>` — per-session runtime instances for every script-hosting descriptor (main module, common modules, per-object runtimes as they spawn)

Every script-executing thread runs under a `SessionScope` RAII guard that pins one `ibSession*` to a `thread_local` slot. `ibSession::Current()` returns the scoped session. Script dispatch (`ibModuleDataObject::GetProcUnit()`) resolves through the current session's map — different sessions invoking the same module descriptor get **different** ProcUnits, each with their own local state.

### ibSessionRegistry

`ibSessionRegistry` (`src/engine/backend/session/sessionRegistry.h`) is the process-wide session manager:

- **Single-consumer queue + priority** — one registry thread processes `Add / Attach / Detach / Remove / SetActivity` requests (Urgent → Normal → Low → Background). All DB mutation for `sys_session` happens on this thread.
- **Row-level pessimistic lock as source of truth** — the registry holds a long-running transaction with `SELECT ... WITH LOCK` over its own-session rows. Process crash → DB rolls back → another process's sweep acquires the lock via `TryProbeRowLock` → zombie row DELETEd. Replaces old heartbeat-UPDATE model.
- **Connect/Disconnect API** — desktop `ibApplicationData::Connect` and web `ibWebSession::Login` both call `registry.Connect(req)` which returns an `ibSessionTicket` (RAII, dtor submits Remove@Urgent).

The registry supports multiple concurrent sessions (N on web, 1 on desktop) through the same mechanism. See `project_session_registry_refactor` memory entry for the current implementation status.

### Runtime ownership — current state and direction

**Current (partially migrated):**
- Compile state (`ibCompileCode` with immutable `ibByteCode`) is shared-in-metadata, per module descriptor.
- ProcUnits are per-session, stored in `ibSession::m_procUnitMap`. Created by `InitRuntimeForSession(session)` at Login, destroyed by `ExitRuntimeForSession(session)` at Logout.
- A legacy `ibModuleDataObject::m_procUnit` field still exists on descriptors as a migration fallback (desktop path) and has dual-write with the session map.
- `ibValueModuleManager` (the "module manager" — main module of configuration plus list of common modules) lives as a shared singleton in metadata (`activeMetaData->GetModuleManager()`). It owns compile state and is the dispatch point for `BeforeStart / OnStart / BeforeExit / OnExit` events.

**Direction (in progress, see `project_runtime_owner_refactor` memory):**

`ibValueModuleManager` is conceptually the **runtime entry point** — the root of a session's runtime tree. It should not live in metadata; it should be a **per-session runtime unit** owned by the session. Metadata keeps only compile-state (the `shared_ptr<ibCompileCode>` / `shared_ptr<ibByteCode>` per descriptor).

Target structure for a runtime unit (`ibValueModuleManager` after refactor):

```
ibValueModuleManager (per-session)
  ├── kind        — Main / CommonModule / ObjectManager / Instance / External / Eval
  ├── descriptor  — which metadata node it represents
  ├── session     — which session owns it
  ├── byteCode    — shared_ptr<ibByteCode> (immutable, shared across sessions)
  ├── procUnit    — owned ibProcUnit (mutable runtime state)
  └── parent      — shared_ptr<ibValueModuleManager> (common→main, form→main, eval→enclosing)
```

- Main module = session's **runtimeRoot**; common modules, forms, per-instance catalog/document runtimes hang off as children via `parent` shared_ptr chain.
- Script dispatch (`obj.Method()`, `Catalogs.X.Find(...)`) walks the parent chain — each runtime reaches enclosing globals through parent's procUnit.
- Teardown is clean cascade: drop session → drop runtimeRoot → parent chain unwinds bottom-up (children go first).
- `ibByteCode` becomes self-contained (holds its own moduleName, rootContext, parent-bytecode ref); the `byteCode->m_compileModule` back-pointer is removed. This decouples runtime lifetime from `ibCompileCode` lifetime — metadata reload can drop compile state while running sessions hold their bytecode through their shared_ptr.

**Same model for desktop and web** — desktop has N=1 session (process-wide), web has N per-cookie. The `ibSessionRegistry + ibSession + SessionScope + runtime unit` stack is identical; differences are only in session count and threading (desktop = wx main thread dispatches scripts; web = per-session worker thread via `RunOnWorker`).

### Designer — compile only

Designer (`eDESIGNER_MODE`) creates sessions without runtime — `InitRuntimeForSession` returns early for Designer role. Designer reads `ibCompileCode` for autocomplete, function search, jump-to-definition, and cascading recompile. Scripts are not executed. Debug sessions attach to a separate runtime process (enterprise.exe / wenterprise-server.exe) via the TCP debug protocol.

---

## Database Abstraction

### Class Hierarchy

```
ibDatabaseLayer  (abstract — databaseLayer.h)
  ├─ ibFirebirdDatabaseLayer   (databaseLayer/firebird/)
  ├─ ibPostgresDatabaseLayer   (databaseLayer/postgres/)
  ├─ ibSqliteDatabaseLayer     (databaseLayer/sqllite/)
  ├─ ibMysqlDatabaseLayer      (databaseLayer/mysql/)
  └─ ibOdbcDatabaseLayer       (databaseLayer/odbc/)

ibDatabaseResultSet  (abstract)
  ├─ ibFirebirdResultSet
  ├─ ibPostgresResultSet
  ├─ ibSqliteResultSet
  ├─ ibMysqlPreparedStatementResultSet
  └─ ibOdbcResultSet

ibPreparedStatement  (abstract)
  ├─ ibFirebirdPreparedStatement
  ├─ ibPostgresPreparedStatement
  ├─ ibSqlitePreparedStatement
  ├─ ibMysqlPreparedStatement
  └─ ibOdbcPreparedStatement
```

### Access Pattern

All database access goes through the `db_query` macro, which calls `ibApplicationData::GetDatabaseLayer()` and returns the active `ibDatabaseLayer*`. Example:

```cpp
ibDatabaseResultSet* rs = db_query->RunQueryWithResults(
    wxT("SELECT * FROM %s WHERE guid = ?"), table_name);
```

Each driver folder contains an `engine/` subdirectory with the vendored native client library.

---

## Form System

### ibValueForm Hierarchy

```
ibBackendControlFrame  (abstract interface — backend_form.h)
  └─ ibBackendValueForm  (abstract — backend_form.h)
       └─ ibValueForm  (concrete — frontend/visualView/ctrl/form.h)
```

`ibValueForm` owns the complete control tree for one open form. It is created by `ibBackendValueForm::CreateNewForm()`, which resolves the `ibValueMetaObjectFormBase` descriptor from metadata and instantiates the correct form type.

### 22 Visual Controls

Implemented in `src/engine/frontend/visualView/ctrl/`:

| Control | File |
|---|---|
| Button | `button.cpp` |
| CheckBox | `checkbox.cpp` |
| Choice | `choice.cpp` |
| ComboBox | `combobox.cpp` |
| Form (root) | `form.cpp` |
| Frame | `frame.cpp` |
| Gauge | `gauge.cpp` |
| GridBox | `gridBox.cpp/.h` |
| HtmlBox | `htmlBox.cpp/.h` |
| ListBox | `listbox.cpp` |
| Notebook | `notebook.cpp/.h` |
| RadioButton | `radiobutton.cpp` |
| Slider | `slider.cpp` |
| StaticText | `statictext.cpp` |
| StaticLine | `staticline.cpp` |
| TableBox | `tableBox.cpp/.h` |
| TextBox | `textBox.cpp/.h` |
| TextCtrl | `textctrl.cpp` |
| ToolBar | `toolBar.cpp/.h` |
| BoxSizer | `boxsizer.cpp` |
| GridSizer | `gridsizer.cpp` |
| ChartBox | `chartBox.cpp/.h` |

### Form Rendering

`ibVisualHost` is the wxWindow that owns the visual surface. It receives layout instructions from `ibValueForm` and positions wxWidgets controls. `ibVisualHostClient` handles the reverse channel: user input events travel from wxWidgets up through the host into the form's script event handlers.

---

## Debugger Architecture

### Client-Server Model

```
enterprise.exe                     designer.exe
      │                                  │
ibDebuggerServer ◄──── TCP ──────► ibDebuggerClient
(src/engine/backend/debugger/)     (src/engine/frontend/ or designer/)

Default port: 1650  (defined as defaultDebuggerPort in debugDefs.h)
```

### Connection Types (`ConnectionType` enum)

| Value | Purpose |
|---|---|
| `ConnectionType_Scanner` | Designer scanning for debuggable processes |
| `ConnectionType_Waiter` | Enterprise waiting for a connection |
| `ConnectionType_Debugger` | Active debug session |

### Command Flow

The designer sends `CommandId` packets; enterprise responds with `EventId` packets:

```
Designer                           Enterprise
   │  CommandId_VerifyConnection      │
   │ ────────────────────────────────►│
   │  CommandId_SetConnectionType     │
   │ ◄────────────────────────────────│
   │  CommandId_StartSession          │
   │ ────────────────────────────────►│
   │  EventId_SessionStart            │
   │ ◄────────────────────────────────│
   │  CommandId_ToggleBreakpoint      │
   │ ────────────────────────────────►│
   │  CommandId_Continue              │
   │ ────────────────────────────────►│
   │  EventId_EnterLoop               │
   │ ◄────────────────────────────────│
   │  CommandId_GetArrayBreakpoint    │
   │  CommandId_SetLocalVariables     │
   │  CommandId_SetStack              │
   │ ◄────────────────────────────────│
```

The server runs each connection as a `wxThread` (`ibDebuggerServer::ibDebuggerServerConnection`). Raw binary packets are sent via `SendCommand` / `RecvCommand`.

---

## Key Data Flows

### User Login (desktop)

```
launcher.exe (or direct enterprise.exe with CLI creds)
  └─ ibApplicationData::CreateServerAppDataEnv(mode, server, port, user, pwd, db, locale)
       └─ ibDatabaseLayer::Open(server, port, db, user, pwd)   # DB-level admin connection
            └─ ibApplicationData::Connect(userName, userPassword)
                 └─ ibSessionRegistry::Start(+EnableSysSessionOwnership)
                      └─ registry.Connect(req) — Submit(Add, Normal), wait state Added
                           └─ Add → DB INSERT sys_session row under row-lock
                                └─ ticket.Attach(user, pwd) — Submit(Attach, Normal)
                                     └─ AuthenticateUser (PBKDF2 preferred, MD5 silent-upgrade path)
                                          └─ InstallUser → SessionScope(session)'d dual-write
                                               └─ InitRuntimeForSession(session)
                                                    └─ create main ProcUnit + Execute top-level
                                                         └─ StartMainModule: BeforeStart / OnStart events
                                                              └─ wx main loop handles UI
```

### User Login (web — wenterprise-server)

```
HTTP: POST /w/<dbalias>/login  (body: user+pwd, cookie: tabSid GUID)
  └─ wfrontendCreateSessionWithId(tabSid)      # if unknown cookie, create new session
       └─ Sessions().Login(tabSid, user, pwd)
            └─ registry.Connect(req)           # same path as desktop
                 └─ ticket.Attach(user, pwd)   # AuthenticateUser on worker-side
                      └─ InitRuntimeForSession(session)
                           └─ SessionScope(session) on HTTP handler thread
                                └─ ibWebApplication::OnInit
                                     └─ StartMainModule (BeforeStart / OnStart, under runtimeMutex)
                                          └─ StartWorker — per-session worker thread
                                               └─ future HTTP calls POST to worker via RunOnWorker
```

### Form Open

```
Script (desktop) / HTTP POST /open?metaID=N (web):
OpenForm("Catalog.Products.ListForm")
  └─ ibValueMetaObjectFormBase::CreateAndBuildForm(metaForm, owner, srcObj, uniqueKey)
       └─ ibBackendMainFrame::CreateNewForm(metaFormObject, ownerControl, srcObject)
            └─ ibValueForm constructed (per-instance compileModule + ProcUnit)
                 └─ LoadFormData / BuildForm — control tree built from metadata
                      └─ ibBackendMainFrame::CreateDocumentWindow()
                           └─ ibVisualHost created (wxWindow on desktop, ibWebWindow on web)
                                └─ controls instantiated and laid out
                                     └─ OnOpen() script handler via ibProcUnit::CallAsProc()
```

### Document Save

```
Script: Write()   (or user presses Save)
  └─ ibValueRecordDataObject::Write()
       └─ ibDatabaseLayer::BeginTransaction()
            └─ generate SQL INSERT/UPDATE for main table
                 └─ iterate tabular sections → INSERT/UPDATE child rows
                      └─ post RegisterRecords() for registers if Document
                           └─ ibDatabaseLayer::Commit()
                                └─ OnWrite() script handler called
```

### Session Teardown (web — /logout or pagehide beacon)

```
HTTP: POST /w/<dbalias>/logout?sid=<tabSid>  (sendBeacon from browser pagehide)
  └─ Sessions().Destroy(sid)
       └─ shared_ptr<ibWebSession> dropped from sessions map
            └─ ~ibWebSession → OnExit
                 └─ RunOnWorker DeleteAllViews of open tabs (form dtors)
                      └─ StopWorker (joins per-session worker thread)
                           └─ ExitMainModule (BeforeExit / OnExit events, under runtimeMutex)
                                └─ ExitRuntimeForSession — drop session's ProcUnit map entries
                                     └─ delete frame
                                          └─ ticket.reset → Submit(Remove, Urgent)
                                               └─ registry DELETE sys_session row, release row-lock
```
