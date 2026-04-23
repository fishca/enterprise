# Connection pool — architecture, invariants, usage

Process-wide bounded pool of `ibDatabaseLayer` connections with RAII
scope handles, thread-local pinning, and nested-safe transaction
counter. Replaces the pre-existing "single `m_db` + `ibTransactionGuard`
template" model.

> **Status:** landed. Pool + scope + TX counter + TX TL pinning + all
> object mutation entry points migrated. Build clean on Debug|x86.
> Smoke-tested on `wenterprise-server.exe` (FB embedded, parallel
> sessions, login + forms) and desktop `enterprise.exe` (catalog
> save-and-close flow).

---

## Table of contents

1. [Motivation](#motivation)
2. [Architecture](#architecture)
3. [Public API](#public-api)
4. [Usage patterns](#usage-patterns)
5. [Invariants](#invariants)
6. [Nested transaction semantics](#nested-transaction-semantics)
7. [Thread-local slots](#thread-local-slots)
8. [Migration status](#migration-status)
9. [Pitfalls and gotchas](#pitfalls-and-gotchas)
10. [Future work](#future-work)

---

## Motivation

Before the refactor OES had:

- One `shared_ptr<ibDatabaseLayer>` on `ibApplicationData::m_db` —
  the master connection, shared across every thread in the process.
- `ibTransactionGuard<>` template in `commonObject.h` — per-guard
  `m_active_transaction` flag with direct driver calls. Nested guards
  on the same `m_db` were not properly coordinated: each guard's
  second `Commit()` hit the driver, breaking atomicity.
- No pool — `ibSessionRegistry` had its own ad-hoc "Checkout" against
  the master connection.

Problems:

1. **No thread isolation.** Every thread hit `m_db`. On FB embedded
   a global `fb_mutex` serialised them; on PG / MySQL / MSSQL the
   driver-level state got racey.
2. **Nested TX broken.** `Document.Write()` opens a TX, calls
   `RegisterRecordSet.Write()` which opens its own guard → both hit
   the same driver, Commit of the inner guard committed the outer's
   work prematurely. Inner rollback did not propagate to outer.
3. **No parallelism.** Web server's 10 concurrent sessions could not
   execute concurrent reads/writes — all serialised on one `m_db`.

The pool refactor addresses all three at once by making the connection
the unit of ownership and the transaction the unit of bookkeeping.

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│  ibApplicationData (singleton)                                  │
│    • holds std::unique_ptr<ibConnectionPool>                    │
│    • NO direct m_db — pool is the sole owner of every conn      │
└──────────────────────┬──────────────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────────────┐
│  ibConnectionPool                                               │
│    • m_source : shared_ptr<ibDatabaseLayer>   (master, alive)   │
│    • m_idle   : deque<shared_ptr<ibDatabaseLayer>>              │
│    • lazy Clone() on demand up to maxSize                       │
│    • thread-safe via mutex + cv                                 │
│    • thread-local slots for scope + active-TX tracking          │
└──────────────────────┬──────────────────────────────────────────┘
                       │ Checkout + custom-deleter hand-out
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│  ibConnectionScope (RAII)                                       │
│    • ctor: inherit parent's TL conn OR Checkout fresh           │
│    • installs scope's conn on thread-local slot                 │
│    • SafeBeginTransaction / SafeCommitTransaction /             │
│      SafeRollBackTransaction (merged from legacy TransactionGuard)│
│    • dtor: restores TL slot, drops shared_ptr → pool reparks    │
│    • dtor safety-net: any unmatched SafeBegin → RollBack        │
└──────────────────────┬──────────────────────────────────────────┘
                       │ scope->X() / db_query->X()
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│  ibDatabaseLayer (base)                                         │
│    • inherits std::enable_shared_from_this<ibDatabaseLayer>     │
│    • public non-virtual Begin/Commit/RollBack wrappers:         │
│        - counter m_txDepth + flag m_txAborted                   │
│        - only depth 0→1 fires DoBeginTransaction                │
│        - only depth 1→0 fires DoCommit (or DoRollBack if        │
│          aborted → "inner rollback poisons outer commit")       │
│        - sets/clears pool's active-TX TL slot on transitions    │
│    • protected pure-virtual Do* — implemented by 5 drivers      │
└──────────────────────┬──────────────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────────────┐
│  Driver implementations (firebird/postgres/sqlite/mysql/odbc)   │
│    • DoBeginTransaction/DoCommit/DoRollBack — real SQL/native   │
│    • no m_transaction_is_active flag (removed); base owns state │
└─────────────────────────────────────────────────────────────────┘
```

**Key files:**

| File | Purpose |
|---|---|
| `backend/databaseLayer/connectionPool.{h,cpp}` | Pool + TL slots |
| `backend/databaseLayer/connectionScope.{h,cpp}` | RAII scope + merged Safe* tx API |
| `backend/databaseLayer/databaseLayer.{h,cpp}` | Base counter layer + wrapper methods |
| `backend/databaseLayer/{firebird,postgres,sqllite,mysql,odbc}/*` | 5 drivers — DoBegin/DoCommit/DoRollBack protected virtual |
| `backend/appData.{h,cpp}` | Pool owner, `GetDatabaseLayer()` resolving priority chain |

---

## Public API

### `ibConnectionPool`

```cpp
class BACKEND_API ibConnectionPool {
public:
    // Lifecycle
    void Init(std::shared_ptr<ibDatabaseLayer> primary, std::size_t maxSize);
    void Shutdown();

    // Low-level checkout — does NOT touch TL. For registry-style
    // consumers that need N parallel conns on one thread.
    std::shared_ptr<ibDatabaseLayer> CheckoutIndependent();

    // Canonical RAII factory. Returns an ibConnectionScope by value.
    static ibConnectionScope GetFreeConnection();

    // Master connection — the fallback used by GetDatabaseLayer
    // when no thread-local slot is set.
    static std::shared_ptr<ibDatabaseLayer> GetPrimaryConnection();

    // The `db_query` resolution function. Priority chain:
    //   1. Active-TX TL (if any)
    //   2. Active-scope TL (if any)
    //   3. Primary conn
    static std::shared_ptr<ibDatabaseLayer> GetDatabaseLayer();

    // Thread-local active-transaction accessors — set by
    // ibDatabaseLayer::BeginTransaction on depth 0→1, cleared on 1→0.
    static std::shared_ptr<ibDatabaseLayer> GetActiveTxConnection();
    static void SetActiveTxConnection(std::shared_ptr<ibDatabaseLayer>);

    // Observability
    std::size_t MaxSize()  const;
    std::size_t LiveSize() const;
    std::size_t IdleSize() const;
};
```

### `ibConnectionScope`

```cpp
class BACKEND_API ibConnectionScope {
public:
    ibConnectionScope();        // inherit TL or Checkout + install TL
    ~ibConnectionScope();       // rollback unmatched Begin + restore TL + drop conn
    // move-only

    // Driver access — scope->X() / (*scope).X()
    ibDatabaseLayer* operator->() const;
    ibDatabaseLayer* get() const;
    explicit operator bool() const;

    // Underlying shared_ptr — for code that needs shared ownership
    const std::shared_ptr<ibDatabaseLayer>& shared() const;

    // Scope-tracked transactions (safety-net: dtor rolls back any
    // unresolved SafeBegin)
    void SafeBeginTransaction(const ibDatabaseLayer::ibTxOptions& = {});
    void SafeCommitTransaction();
    void SafeRollBackTransaction();

    bool HasActiveTransaction() const;
    bool Owns() const;  // true = outer-most scope, false = inherited
};
```

### `ibDatabaseLayer` (base)

```cpp
class ibDatabaseLayer : public std::enable_shared_from_this<ibDatabaseLayer> {
public:
    // Public non-virtual wrappers — counter + aborted-flag semantics
    void BeginTransaction(const ibTxOptions& opts = {});
    void Commit();
    void RollBack();
    bool IsActiveTransaction();   // m_txDepth > 0

protected:
    // Driver hooks — dialect-specific SQL / native API
    virtual void DoBeginTransaction(const ibTxOptions& opts) = 0;
    virtual void DoCommit() = 0;
    virtual void DoRollBack() = 0;

    int  m_txDepth   = 0;
    bool m_txAborted = false;
};
```

---

## Usage patterns

### Entry-point method (migrated)

```cpp
bool ibValueRecordDataObjectCatalog::WriteObject()
{
    if (!appData->DesignerMode())
    {
        ibConnectionScope scope = ibConnectionPool::GetFreeConnection();

        if (!scope || !scope->IsOpen())
            ibBackendCoreException::Error(_("Database is not open!"));

        if (!ibBackendException::IsEvalMode())
        {
            if (!m_metaObject->AccessRight_Write()) {
                ibBackendAccessException::Error();
                return false;
            }

            scope.SafeBeginTransaction();

            m_procUnit->CallAsProc(wxT("BeforeWrite"), cancel);
            if (cancel.GetBoolean()) {
                scope.SafeRollBackTransaction();
                ibBackendCoreException::Error(_("Failed to write object in db!"));
                return false;
            }

            if (!SaveData()) {
                scope.SafeRollBackTransaction();
                ibBackendCoreException::Error(_("Failed to write object in db!"));
                return false;
            }

            m_procUnit->CallAsProc(wxT("OnWrite"), cancel);
            if (cancel.GetBoolean()) {
                scope.SafeRollBackTransaction();
                ibBackendCoreException::Error(_("Failed to write object in db!"));
                return false;
            }

            scope.SafeCommitTransaction();

            if (valueForm != nullptr) valueForm->NotifyChange(GetReference());
        }
    }
    return true;
}
```

### Nested Write (Document → Registers)

```cpp
// Document.Write — outer scope
{
    ibConnectionScope scope = ibConnectionPool::GetFreeConnection();
    scope.SafeBeginTransaction();           // counter 0→1, REAL Begin

    m_procUnit->CallAsProc("BeforeWrite"); // script may mutate state
    SaveData();                             // Document row

    for (auto& rs : m_registerRecordSets) {
        rs.Write();                         // Register.Write — inner scope
        //   scope2 inherits conn1 via TL
        //   scope2.SafeBeginTransaction → counter 1→2, SKIP driver Begin
        //   register SaveData on SAME conn1
        //   scope2.SafeCommitTransaction → counter 2→1, SKIP driver Commit
    }

    scope.SafeCommitTransaction();          // counter 1→0, REAL Commit
}                                           // ~scope → pool repark
```

If any inner `scope2.SafeRollBackTransaction()` fires, it sets
`m_txAborted = true` on the conn; the outer `scope.SafeCommitTransaction`
then silently becomes a real `DoRollBack`. Atomicity preserved.

### Registry-style (non-TL, parallel conns on one thread)

```cpp
// ibSessionRegistry::Start — needs 3 parallel conns on its own thread
m_lockConn  = pool->CheckoutIndependent();   // raw Checkout, no TL touch
m_writeConn = pool->CheckoutIndependent();
m_probeConn = pool->CheckoutIndependent();
```

`CheckoutIndependent` does NOT install TL — each returned conn is the
caller's sole owner. Drops re-park via custom deleter.

### Legacy `db_query->X()` without scope

Still works. Macro resolves via `ibConnectionPool::GetDatabaseLayer()`
→ priority chain:

1. Active-TX TL (from a bare `db_query->BeginTransaction()` someone did)
2. Active-scope TL (from an outer `ibConnectionScope`)
3. Primary conn (m_source)

Unmigrated call sites keep working; they just don't get parallelism.

---

## Invariants

1. **Pool is the sole owner** of every live `ibDatabaseLayer`. No
   direct ownership on `ibApplicationData` or anywhere else.
2. **One conn per thread at a time** (for any given hand-out).
   `shared_ptr` is not Share-Across-Threads-And-Use-Simultaneously —
   native driver state races.
3. **Nested scopes on one thread inherit** the outer scope's conn.
   Never Checkout a second conn when one is already active — breaks
   nested TX (SQL is connection-local).
4. **Active-TX TL pins conn to thread** for the TX's full duration.
   `db_query` on this thread routes to the TX's conn regardless of
   scope state.
5. **`shared_from_this()` requires pool-owned layer.** Layers
   constructed outside the pool (test harnesses) won't work with the
   TX TL pinning — bad_weak_ptr on BeginTransaction.
6. **Scope dtor rolls back** any unmatched SafeBegin. Exception
   between SafeBegin and SafeCommit cleans up automatically.
7. **Lazy Clone** — pool only opens connections on demand. Desktop
   single-user process stays on master alone; `maxSize=32` is a cap,
   not a target.
8. **Pool Shutdown requires stopped workers.** Outstanding hand-outs
   are valid until dropped by their owner; pool's m_source is
   cleared. Shutdown order: stop all workers → join threads →
   pool.Shutdown.

---

## Nested transaction semantics

Counter-based, uniform across all 5 drivers:

- First `BeginTransaction()` on a conn: `m_txDepth 0→1`, fires
  `DoBeginTransaction` (real driver Begin).
- Subsequent nested `BeginTransaction`: `m_txDepth++`, NO driver call.
- `Commit()`:
  - If `m_txDepth > 1`: just decrement.
  - If `m_txDepth == 1`: decrement to 0, then fire real `DoCommit`
    (or `DoRollBack` if `m_txAborted` was set).
- `RollBack()`:
  - Sets `m_txAborted = true` (poisons outer Commit).
  - Decrement.
  - If `m_txDepth` now 0: fire real `DoRollBack`.

**Why counter instead of real nested TX per driver:**

| Driver | Native nested BEGIN behaviour |
|---|---|
| Firebird | Creates parallel TX (`isc_start_transaction`) — inner commits don't affect outer, breaking atomicity |
| SQLite | Errors: `cannot start a transaction within a transaction` |
| PostgreSQL | Warns + ignores — only one TX active anyway |
| MySQL | Implicit-commits outer TX when starting new — destroys outer's atomicity |
| ODBC / MSSQL | Increments `@@TRANCOUNT` (basically the same counter model) |

Counter on base unifies all 5 drivers to a single flat-TX semantic
with correct atomic nesting. Inner rollback propagates to outer's
commit via the aborted flag.

**What this does NOT give:** true savepoints. If a caller needs to
roll back inner work while committing outer, use `SAVEPOINT` /
`ROLLBACK TO SAVEPOINT` explicitly at the driver level. Not currently
exposed through `ibDatabaseLayer`.

---

## Thread-local slots

Two distinct TL slots on `ibConnectionPool` (defined in
`connectionPool.cpp` as namespace-local `thread_local shared_ptr`):

1. **`s_tlCurrent`** — scope's current conn. Set by `ibConnectionScope`
   ctor (if outer-most), cleared by dtor. Private to the
   pool/scope friend pair.

2. **`s_tlActiveTx`** — active-TX conn. Set by
   `ibDatabaseLayer::BeginTransaction` when `m_txDepth 0→1` via
   `shared_from_this()`. Cleared when `m_txDepth 1→0` in Commit or
   RollBack. Holds the layer alive through the pool's control block
   (via `enable_shared_from_this`) — the layer survives even if the
   hand-out shared_ptr that started the TX has been dropped.

**Priority chain for `GetDatabaseLayer()`:**

```
if (s_tlActiveTx)  return s_tlActiveTx;    // TX pin — connection-local
if (s_tlCurrent)   return s_tlCurrent;     // scope inheritance
return GetPrimaryConnection();             // legacy m_db fallback
```

---

## Migration status

### Migrated (scope + Safe* TX API)

| File | Methods |
|---|---|
| `catalogObject.cpp` | `WriteObject`, `DeleteObject` |
| `documentObject.cpp` | `WriteObject`, `DeleteObject` |
| `chartOfAccountsObject.cpp` | `WriteObject`, `DeleteObject` |
| `chartOfCharacteristicTypesObject.cpp` | `WriteObject`, `DeleteObject` |
| `informationRegisterObject.cpp` | `WriteRecordSet`, `DeleteRecordSet`, `WriteRegister`, `DeleteRegister` |
| `accumulationRegisterObject.cpp` | `WriteRecordSet`, `DeleteRecordSet` |
| `accountingRegisterObject.cpp` | `WriteRecordSet`, `DeleteRecordSet` |
| `constantObject.cpp` | `SetConstValue` |
| `commonObjectQuery.cpp` | `ExistData` |

### Not migrated (read-only or complex TX flow)

| File | Reason |
|---|---|
| `metadataConfigurationQuery.cpp` | Save/Load TX spans multiple methods (OnBeforeSave → OnSave → OnAfterSave). Works via TX TL pinning (thread-level). Migration possible but needs careful scope-as-member refactor. Low priority — config save blocks main thread anyway. |
| `systemManagerFunc.cpp` | Runtime `BeginTransaction/Commit/RollBack` built-ins — **intentionally bare**. These are transparent forwarders for script-side TX API; adding a scope wrapper would create a per-call checkout, breaking the thread-sticky semantic the counter layer relies on. |
| `*Manager_impl.cpp` (catalog, document, register managers) | Read-only `FindByCode` / `FindByName` etc. No TX. Migration gives parallelism but isn't correctness-critical. Low priority. |
| `objectListQuery.cpp`, `*MetadataQuery.cpp` | Read-only SELECTs. Same as above. |

### Consumers using `CheckoutIndependent`

| File | Purpose |
|---|---|
| `sessionRegistry.cpp` | 3 persistent conns (lock / write / probe) on registry thread |
| `wfrontend.cpp::CheckMetadataAndEvict` | 15s metadata-watcher per-tick |

---

## Pitfalls and gotchas

1. **`m_txDepth` and `m_txAborted` are plain `int`/`bool`.** Not
   atomic. Invariant: one conn used by at most one thread at a time
   (enforced by pool's Checkout + shared_ptr). If the invariant is
   broken (a caller passes conn to another thread and both use it),
   counter races. Defensive option: `std::atomic<int>` — small
   overhead, protects against debugging hell.

2. **Primary conn as fallback for legacy `db_query`.** Call sites
   without a scope go to `m_source`. All threads share master.
   FB embedded has a global `fb_mutex` that serialises; PG/MySQL/MSSQL
   may race. Migration is opt-in per call site — focus on mutation
   paths first (Write/Delete/Update).

3. **Lazy Clone latency burst.** First Checkout for a new slot pays
   Open() cost (FB embedded ~100ms, PG/MySQL remote ~50-500ms).
   Sequential burst of N new sessions → N × Open() serialised through
   pool's mutex. Acceptable for typical login pattern; mitigate with
   `minIdle` pre-warm if needed for benchmark scenarios.

4. **`shared_from_this()` requires pool-owned layer.** Layers
   constructed outside the pool (e.g., unit-test harnesses with
   `new ibDatabaseLayerFirebird()`) throw `bad_weak_ptr` when
   `BeginTransaction` tries `shared_from_this`. Test code must wrap
   in `std::shared_ptr` first.

5. **Hand-out's deleter runs on the dropping thread.** If a caller
   passes shared_ptr to another thread and drops there, the deleter
   (lambda capturing pool's sp) runs on that thread → acquires pool
   mutex → pushes to m_idle. Thread-safe, but unusual flow.

6. **Pool Shutdown vs live scopes.** `pool->Shutdown()` resets
   `m_source` and clears `m_idle`. If a scope is still alive on
   another thread, it holds the hand-out → lambda deleter on drop
   sees `m_shutdown` → releases without parking. Layer destructed.
   **Order:** stop all workers → join threads → pool.Shutdown.

7. **TL overhead on hot paths.** Each `scope ctor/dtor` does a TL
   push/pop + shared_ptr copy (atomic refcount bump). ~10ns on hot
   path — fine for per-method scope; avoid per-iteration scope in
   tight loops.

8. **Scope inheritance across threads doesn't happen.** TL is
   per-thread. If worker thread dispatches sub-task to another
   thread via a callback/lambda, that callback does NOT inherit the
   scope's conn. Must explicitly pass shared_ptr or open a new scope.

9. **TX TL not cleared on bare `Begin` + crash.** If code does
   `db_query->BeginTransaction()` without scope/guard, throws, and
   the catcher doesn't Commit/Rollback, TX TL stays set → next
   request on this thread lands on the old TX. **Fix:** migrate to
   `scope.SafeBeginTransaction` (RAII-cleaned) or wrap bare calls
   in `ibTransactionGuard` equivalent.

10. **Pool saturation blocks indefinitely.** Checkout on a saturated
    pool `cv.wait`s forever if no one Returns. No timeout. Worker
    leak = deadlock.  Mitigation: large enough `maxSize` for expected
    concurrency; or add a `CheckoutWithTimeout` overload.

11. **Web per-session worker thread holds scope indefinitely** (if
    applied). Every session = one persistent pool conn. 1000 sessions
    = 1000 conns → well over `maxSize=32`. Fix: scope per-request,
    not per-worker-lifetime (see `compute-server-tiering.md` for
    the 3-tier plan).

---

## Future work

- **`minIdle` pre-warm option.** `Init(primary, maxSize, minIdle)` —
  eagerly clone `minIdle` conns at Init so first N Checkouts are
  fast. For benchmark / high-burst scenarios.

- **CheckoutWithTimeout.** `Checkout(std::chrono::milliseconds)`
  returning nullptr if pool saturated. Debug safety net.

- **Per-connection atomic TX counter.** `std::atomic<int> m_txDepth`
  and `std::atomic<bool> m_txAborted`. Defense in depth.

- **Savepoints API.** `ibDatabaseLayer::Savepoint(name) / RollbackTo(name)`
  for callers that need real sub-TX (inner rollback without affecting
  outer). Supported by FB/PG/MySQL/MSSQL, not SQLite.

- **FB `fb_tr_list` cleanup.** Native nested-TX stack is now dead
  (counter collapses everything to depth 1). Remove the linked-list
  code for clarity — `m_fbNode` becomes a single-slot.

- **Pool stats endpoint.** `/admin/pool` on wes returning JSON with
  LiveSize / IdleSize / MaxSize. For ops monitoring.

- **Tiered server architecture.** See `compute-server-tiering.md` —
  the pool is ready for shared-worker + per-session-queue model;
  the server-side dispatcher is the missing piece.
