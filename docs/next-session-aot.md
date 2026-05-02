# AOT bytecode cache

**Goal:** persist compiled `ibByteCode` to disk so cold sessions skip
re-compilation. Halves cold-start latency at scale; the resolver work
landed 2026-05-02 made bc self-sufficient (no compile-context
dependency at runtime), unlocking this step.

## Readiness assessment

| Dimension              | Status | Notes                                         |
|------------------------|--------|-----------------------------------------------|
| bc self-sufficient     | ✅ 95% | m_listVar/m_listFunc + binder + resolve all bc-only |
| Identity / versioning  | 🟡 50% | m_id stamped; m_version is random GUID per compile |
| Persistence layer      | ✅ 100% | `SerializeAOT` / `DeserializeAOT` landed 2026-05-02 |
| Cache lifecycle hooks  | ❌ 0%  | no DB schema, no read-before-compile hook     |
| Invalidation           | 🟡 10% | dependency-version drift detection sketched   |

Net: ~70% ready after Step 1. DB schema and compile-time integration
remain.

## Plan

### 1. Persistence layer ✅ landed 2026-05-02

**Method names**: `ibByteCode::SerializeAOT(ibWriterMemory&) const` /
`DeserializeAOT(const ibReaderMemory&)`. Renamed from `Serialize` to
keep it distinct from `ibValue::Serialize` (string form, client-server
data exchange) — different layer, different audience.

**File**: `src/engine/backend/compiler/byteCodeAOT.cpp` (sibling of
`valueSerialization.cpp`). Declarations live in `byteCode.h`.

**Format**: linear binary, host-endian, magic `'PBC1'` + format
version `1`. No `fs.h` chunk wrappers around the top-level layout —
each collection is `u32 count` + `count` entries laid out back-to-back.
Strings use `w_stringZ` / `r_stringZ` (UTF-8 zero-terminated).
`ibNumber` constants use the binary `ibNumber::GetBuffer` /
`SetBuffer` form (length-prefixed, exact-decimal — not lossy double).

**Persisted fields**:

* `m_id`, `m_version` — raw 16-byte GUID write/read.
* `m_descriptorClsid` (`u64`).
* Header: `m_strModuleName` (zstr), `m_lStartModule` (`s32`),
  `m_lVarCount` (`s32`), `m_bExpressionOnly` (`u8`).
* `m_listCode` — `u32 count` + per-entry `ibByteUnit` (oper, string,
  line, 4 param-units, 3 source-attribution strings).
* `m_listConst` — `u8 typeClass` discriminator + payload. Supports
  `TYPE_EMPTY`, `TYPE_NULL`, `TYPE_BOOLEAN`, `TYPE_NUMBER`,
  `TYPE_DATE`, `TYPE_STRING`. Any other type (TYPE_REFFER /
  TYPE_VALUE / TYPE_ENUM / TYPE_OLE) returns false → cache miss.
* `m_listVar` — kind, slot, clsid, scoped, parentRef, real-name,
  context-name.
* `m_listFunc` — entry IP, paramCount, retFlag, varCount, return-clsid,
  kind, parentRef, real-name, context-name; embedded `m_listParam`
  (with default-values + clsid), `m_listParamRealName`, and
  `m_listLocals`.
* `m_dependencyIds`, `m_dependencyVersions` — vectors of `ibGuid`.

**Skipped (live pointers — re-resolved on load)**: `m_parent`,
`m_dependencies`. Reader stamps `m_bCompile = true` on success.

**Sanity cap**: every collection-count is rejected if > 16M — guards
the reader against pre-allocating GB on garbage input.

**Tests**: `tests/test_byteCodeAOT.cpp` (11 round-trip cases),
`tests/test_compiler.cpp::CompilerAOT::RealCompileOutputRoundTrips`
(end-to-end: real compile → serialize → deserialize → field shape
preserved).

**Out of scope (deferred)**: portable LE encoding (kAOTFlagPortable
bit reserved, not yet used — current format is host-endian);
forward-compatibility of older format-version blobs (current reader
hard-rejects mismatch → cache miss → recompile, which is the safe
default but loses any pre-bump cached work).

### 2. DB schema ✅ landed 2026-05-02

`sys_bytecode_cache` final layout (simpler than originally drafted —
metadata_version / source_hash / compiler_version dropped per the
"track only `ibValueMetaObjectModuleBase` add/remove/text-change"
discussion; descriptor identity + bytecode version cover write-side
freshness, AOT magic + dep version walk cover read-side validity):

```sql
CREATE TABLE sys_bytecode_cache (
    descriptor_id     VARCHAR(36) NOT NULL PRIMARY KEY,
    bytecode_version  VARCHAR(36) NOT NULL,
    bc_blob           BLOB        NOT NULL    -- BYTEA on PostgreSQL
);
```

`bc_blob` not `blob` — Firebird parses the column-name `blob` against
the `BLOB` type keyword and silently mis-creates the table.

DAO at `backend/cache/byteCodeCache.{h,cpp}`:
`Save(bc) / Load(outBc, descId) / Invalidate(descId) / InvalidateAll()`.
`Save` extracts identity from `bc.m_id` / `bc.m_version`, runs
`SerializeAOT` into the blob; `Load` runs `DeserializeAOT` from the
blob and returns false on any failure (magic / format-version / row
missing) so the caller falls through to recompile. Cross-driver via
`ibPreparedStatement::SetParamBlob(int, const void*, long)` — the
`(int, const wxMemoryBuffer&)` overload is a no-op default in the
base class, never call it.

`MigrateTableBytecodeCache` runs unconditionally in `CreateAppDataEnv`
/ `CreateServerAppDataEnv` next to `MigrateTableSession`. PG uses
`BYTEA`, every other driver gets `BLOB`.

### 3. Compile-time integration ✅ landed 2026-05-02

`ibRuntimeModuleDataObject::Compile` runs three phases:

```
A1: ibByteCodeCache::Load(bc, descId)
    hit  → ResolveAndVerifyDependencies → ready (skip A2)
    miss → A2
    drift→ Invalidate, bc.Reset(), A2
A2: m_compileModule->Compile() + stamp identity + Save(bc)
B : binder over bc.m_listVar + PrepareNames on every bound value +
    Register(bc) in process-wide registry
```

Live pointers AOT skipped on serialize are restored in B:
- `bc.m_parent ← parent compile module's m_cByteCode` (mirrors the
  setup at `compileModule.cpp:79`).
- `m_listConst[i].m_bReadOnly = true` stamped inside `DeserializeAOT`
  (const-pool entries are literals by definition).
- `PrepareNames()` called on every `m_listExternValue` /
  `m_listContextValue` before binder seed — fresh-compile path runs
  this inside `PrepareModuleData`; cache-hit path bypasses it.

### 4. Dependency resolution registry ✅ landed 2026-05-02

`ibByteCode::Register / Unregister / Find` — process-wide
`unordered_map<wxString, ibByteCode*>` keyed by descriptor GUID,
guarded by `std::shared_mutex` (read-mostly). `Register` runs at
the end of every successful `Compile` (both fresh and cache-hit
arms); `Unregister` runs from `ibRuntimeModuleDataObject` dtor.

`bc.ResolveAndVerifyDependencies()` walks `m_dependencyIds`, looks
each up via `Find`, populates `m_dependencies`, and verifies
`m_dependencyVersions[i] == dep->m_version`. Returns false on any
missing dep or version drift — caller treats as cache-miss case (c)
and recompiles from source.

### 5. Invalidation tests — pending

Cold-start (empty cache) → all bcs compile + persist. Verified
manually.  
Warm-start (full cache) → all bcs load from cache. Verified manually
(see telemetry below).  
Source edit → Designer's `OnSaveMetaObject(saveConfigFlag)`
invalidates the changed module's row; its dependents' rows stay
valid until next compile, when their `m_dependencyVersions[i]` stops
matching after the dep recompiles → cascade. Verified manually.  
Module deletion → `OnDeleteMetaObject` invalidates its own row
(hygiene; orphan rows are harmless).  
Format-version bump → readers reject; everything recompiles.  

Automated gtests not yet written; use `oes_tests` once
`enterprise/tests/test_byteCodeAOT.cpp` framework is wired against
a live appData (currently exercises serialize/deserialize without DB).

## Telemetry from initial smoke test

Hot-path on cache-hit (FB embedded, ~20 modules from a small fb_test
configuration):

| descriptor             | blob   | load     |
|------------------------|--------|----------|
| ManagerModule (×12)    | 832 B  | 3-4 ms   |
| CommonModule1          | 824 B  | 4 ms     |
| CommonForm1            | 1469 B | 4 ms     |
| CommonForm2            | 2247 B | 6 ms     |
| CommonForm3            | 1805 B | 7 ms     |
| ObjectModule           | 1209 B | 4 ms     |

Latency = single SELECT round-trip + `DeserializeAOT`. Compile path
is 0 µs / 0 µs (compiler not invoked). Cache-miss "first run" cost
matches today's startup.

## Firebird driver fix landed alongside

`firebirdDatabaseLayer::DoRunQuery` and `DoRunQueryWithResults` —
on a per-statement error inside a quickie-TX path (DDL fails because
table exists, etc.), the old code did manual driver-side
`isc_rollback_transaction` but bypassed the public `RollBack()` →
`m_txDepth` stayed at 1 forever → next `OnBeforeSaveDatabase`
tripped its `IsActiveTransaction()` guard and refused to save. Fixed
by routing the rollback through public `RollBack()` so the depth
counter and pool TX-pin clear correctly. Other drivers (PG / SQLite
/ MySQL / ODBC) don't use the quickie pattern — not affected.

## Pre-work that's nice but not blocking

* **Compile-side kind enum** (`ibVariable.m_kind` / `ibFunction.m_kind`)
  — symmetric to bc-side. Drops 4 booleans on compile-side. Makes
  the writer/reader symmetric across both halves of the pipeline.

* **Registry `clsid → helper-template`** — eliminates ContextProp /
  ContextMethod from `m_listVar` / `m_listFunc`. Cuts AOT cache row
  size by 10-30%. Architecturally the cleanest endpoint, but doesn't
  block correctness — large bcs serialize fine, just bigger.

* **`m_strName` drop** (compile-side) — mechanical cleanup, ~50
  callsites. Makes ibVariable smaller. Independent of AOT.

## Things to confirm before starting

1. **Bytecode portability across machines** — do we want machine-
   independent format (so cache rows survive deploy)? If yes,
   careful with endianness on POD-write (use explicit little-endian).
   For dev / per-machine cache — host-endian is fine.

2. **Cache lifetime / size cap** — table grows monotonically unless
   explicit eviction. Drop rows on metadata bump (whole-database
   wipe), or per-descriptor on edit.

3. **Encryption / IP protection** — eventual goal: ship a config to
   prod with bytecode cache populated and source columns nulled.
   Source-hash check would need to skip when source missing. For
   first iteration: cache is dev-time optimization, source stays.

## File touch list

* `src/engine/backend/compiler/byteCode.h` — add Serialize/Deserialize
  declarations.
* `src/engine/backend/compiler/byteCodeSerialization.cpp` — new file,
  writer/reader bodies.
* `src/engine/backend/cache/byteCodeCache.h/.cpp` — new, DAO.
* `src/engine/backend/moduleInfo.cpp::Compile` — cache hook.
* `src/engine/backend/metadataReader.cpp` (or wherever metadata
  generation lives) — bump generation on save.
* SQL: `firebird/init.sql`, `postgres/init.sql`, `sqlite/init.sql`,
  `mysql/init.sql` — add `sys_bytecode_cache` DDL.
* `enterprise/CLAUDE.md` — update "Known Issues" / add AOT cache
  section.

## Estimate (status)

* Step 1 — persistence layer: ✅ landed 2026-05-02.
* Step 2 — DB schema + DAO: ✅ landed 2026-05-02.
* Step 3 — compile-time hook: ✅ landed 2026-05-02.
* Step 4 — dependency registry + verify: ✅ landed 2026-05-02.
* Step 5 — automated invalidation tests: pending; manual smoke OK.

## Out of scope for this session

* Compile-side kind enum — separate cleanup session.
* Registry clsid→helper — separate session, after AOT lands.
* Worker pool / TLS removal — orthogonal to AOT, separate track.
* Multi-process / distributed scaling — much later.
