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

### 2. DB schema (½ day)

`sys_bytecode_cache` table:

```sql
CREATE TABLE sys_bytecode_cache (
    descriptor_id     CHAR(36)    NOT NULL,   -- ibGuid str (key)
    bytecode_version  CHAR(36)    NOT NULL,   -- m_version
    source_hash       CHAR(64),               -- SHA-256 of source text
    metadata_version  BIGINT,                 -- ibMetaData generation
    compiler_version  INTEGER,                -- g_compilerVersion constant
    blob              BLOB,                   -- serialized bc
    PRIMARY KEY (descriptor_id)
);
```

DAO: `ibByteCodeCache::Load(descriptorId, [out] blob, [out] versions)`,
`Save(descriptorId, blob, versions)`, `Invalidate(descriptorId)`.

Cross-driver: Firebird / PostgreSQL / SQLite / MySQL / ODBC — use
`ibPreparedStatement` (project rule).

### 3. Compile-time integration (1 day)

In `ibRuntimeModuleDataObject::Compile` (or `ibCompileCode::Compile`):

```cpp
const ibGuid descId = GetMetaForCompile()->GetGuid();
const auto sourceHash = SHA256(GetSourceText());
const auto metaVer  = activeMetaData->GetGeneration();
const auto compVer  = g_compilerVersion;

if (auto blob = ibByteCodeCache::Load(descId, sourceHash, metaVer, compVer)) {
    bc.Deserialize(*blob);
    // Resolve m_dependencyIds → ibByteCode* via session-level registry.
    return true;   // cache hit
}

if (CompileFromSource()) {
    auto blob = bc.Serialize();
    ibByteCodeCache::Save(descId, blob, sourceHash, metaVer, compVer);
    return true;
}
return false;
```

### 4. Dependency resolution registry (½ day)

Session-level: `unordered_map<ibGuid, ibByteCode*>` populated as bcs
are compiled or loaded. At load, walk `m_dependencyIds`, look each up
in registry, write to parallel `m_dependencies`. Missing → defer
(probably triggers re-compile of dependency).

### 5. Invalidation tests (1 day)

Cold-start (empty cache) → all bcs compile + persist.
Warm-start (full cache) → all bcs load from cache.
Source edit → invalidates own row, parents stay (their
`m_dependencyVersions[i]` stops matching → cascade).
Metadata bump (e.g. user adds a Catalog) → all rows invalidated.
Compiler version bump (binary upgrade) → all rows invalidated.

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

## Estimate (remaining after Step 1)

* DB schema + DAO: **½ day**.
* Compile-time hook + dependency registry: **1½ days**.
* Invalidation logic + cross-driver SQL: **1-2 days**.
* Tests (cold/warm/edit/bump scenarios): **1 day**.

Total: **~1 working week**, no architectural risk.

## Out of scope for this session

* Compile-side kind enum — separate cleanup session.
* Registry clsid→helper — separate session, after AOT lands.
* Worker pool / TLS removal — orthogonal to AOT, separate track.
* Multi-process / distributed scaling — much later.
