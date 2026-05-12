# Decoupling `ibValueModuleManager` from `ibMetaData`

**Status:** planned, not started
**Author:** rollback session 2026-04-25
**Related:** `runtime-facade.md`, `connection-pool.md`, `session-registry.md`, memory note `project_runtime_descriptor_refactor.md`

---

## Goal

Remove the pure virtual

```cpp
virtual ibValueModuleManager* GetModuleManager() const = 0;
```

from `ibMetaData` (`src/engine/backend/metadata.h:23`).

`ibValueModuleManager` is the **runtime root** of the module tree (main module + common modules + procUnit). It is **per-session**, not per-metadata. Hanging it on `ibMetaData` makes the metadata configuration look like a singleton runtime owner, which contradicts the connection-pool / session-registry direction (each worker thread → session scope → its own runtime tree).

After this refactor:

- `ibSession` owns the runtime `ibValueModuleManager` (already wired — `ibSession::GetModuleManager()` exists).
- `ibMetaData` owns only **compile-tree** data (the static description of which common modules exist, their source text, their compileModule artifacts when in designer mode).
- All ~50 call-sites currently going through `metaData->GetModuleManager()` are reclassified into runtime, designer, and design-time-mutation paths and routed accordingly.

---

## Critical disambiguation — two unrelated `GetModuleManager`

Same name, different methods, different return types. Only the **first** is in scope here.

| Method | Defined on | Returns | Action |
|---|---|---|---|
| `ibMetaData::GetModuleManager()` | `metadata.h:23` | `ibValueModuleManager*` (runtime) | **REMOVE** |
| `ibValueMetaObjectXxx::GetModuleManager()` | Catalog/Document/InfoReg/AccumReg/AccountingReg/ChartOf*/ChartOfAccounts metaobjects, `ibValueRecordData::GetModuleManager()` | `ibValueMetaObjectCommonModule*` (design-time manager-module on the metaobject) | **KEEP** |

If you are reading a call like `m_metaObject->GetModuleManager()` or `catalog->GetModuleManager()` or `recordData->GetModuleManager()` — that is the second one and it stays. The first appears only as `m_metaData->GetModuleManager()` / `activeMetaData->GetModuleManager()` / `metaData->GetModuleManager()`.

---

## Rollback note (already done in current session)

Earlier in this session the parent-chain GetSession machinery was rolled back. Current state on disk:

- `ibValueSystemFunction` is fully static again (no per-session instance, no `m_session` field).
- `ibProcUnit::GetSession()` is removed.
- `ibRuntimeModuleDataObject::GetSession()` / `GetSystemFunctions()` is removed.
- `ibProcUnitRoot` namespace class is removed.
- `ibValueModuleManagerConfiguration` no longer takes session in ctor; no per-instance session field.
- `ibSession::Current()` and `ibSession::CurrentFrame()` are the canonical accessors, backed by thread_local + `ibSessionScope` depth counter.
- `ibBackendDocFrame` (renamed from `ibBackendDocMDIFrame`) — frame singleton machinery removed; frame is owned by `ibGUISession`.
- Debugger bridge installed/destroyed inside `ibDesignerSession::OnCreateSession` / `OnDestroySession`.

Build was green Debug|x86 across all 12 projects when this plan was written.

---

## Affected call-sites — full inventory (~50)

### Group 1 — Runtime (replace with `ibSession::Current()->GetModuleManager()`)

Touches actual script execution / form open / OnStart-OnExit dispatch.

| File | Line | Why |
|---|---|---|
| `backend/appData.cpp` | 532, 549 | OnStart / OnExit dispatch — should live on `ibSession` lifecycle, not `appData`. Moving these into `ibSession::OnCreateSession` / `OnDestroySession` is the cleaner end-state. |
| `backend/system/systemManagerFunc.cpp` | 948 | Runtime helper. |
| `frontend/visualView/ctrl/formObject.cpp` | 278 | Form open uses `meta->GetModuleManager()` to dispatch form-module events; route through current session. |
| `frontend/mainFrame/mainFrameParts.cpp` | 155 | Fallback under `m_session->GetModuleManager()` (line 143 already uses session). Drop the fallback once line 143 is the only path. |
| `enterprise/mainFrame/mainFrameEnterprise.cpp` | 104, 115 | Already uses `s->GetModuleManager()` ✓ — only listed because the grep matched. |
| `frontend/web/webApplication.cpp` | 169 | Web init — replace with session-bound mm. |
| `frontend/web/webSession.cpp` | 131, 157 | Same. |

**Effort:** ~7 sites, mechanical replacement. No semantic risk if `ibSession::Current()` is non-null at the call (worker scope discipline guarantees this for runtime paths).

### Group 2 — Designer (compile-only, no runtime needed)

Designer needs a `ibValueModuleManager` for parsing / autocomplete / interpreter. Runtime (procUnit, root context) is not needed in designer mode.

| File | Line | Use |
|---|---|---|
| `designer/win/editor/codeEditor/codeEditor.cpp` | 93, 469, 718 | Autocomplete / parse / find-symbol |
| `designer/win/editor/codeEditor/codeEditorInterpreter.cpp` | 77, 202, 684 | Interpreter context (resolve identifiers in IDE) |
| `designer/win/editor/visualEditor/visualEditorPanel.cpp` | 80 | Visual editor wiring |

**Two viable strategies:**

a. **Use session.** Designer always runs inside an `ibDesignerSession`, so `ibSession::Current()->GetModuleManager()` works. Simplest. Per `ibSession::CurrentFrame()` pattern this is consistent.

b. **Compile-only accessor on metadata.** Add a `GetCompileTree()` (returning a different struct, no runtime) and route designer through that. Cleaner separation but more code.

**Recommended: (a).** Designer is single-session in current architecture; using `Current()` matches the rest of the codebase.

**Effort:** ~7 sites, mechanical.

### Group 3 — Backend metadata mutation / load / save (THE HARD PART)

These ~30 sites in `metaCollection/` register and deregister common-modules with the parent moduleManager when:
- the metadata tree is loaded from disk
- the user adds/deletes/edits a common module in designer
- `OnReloadMetaObject` rebinds compile artefacts after a reload

**Example pattern** (`accumulationRegisterMetadata.cpp:153`):

```cpp
bool ibValueMetaObjectAccumulationRegister::OnReloadMetaObject() {
    ibValueModuleManager* moduleManager = m_metaData->GetModuleManager();
    wxASSERT(moduleManager);
    if (appData->DesignerMode()) {
        ibValueRecordSetObjectAccumulationRegister* recordSet = nullptr;
        if (moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), recordSet)) {
            if (!recordSet->InitializeObject()) return false;
        }
    }
    return true;
}
```

`FindCompileModule` is a **compile-tree query**, not a runtime query. The current code reaches it through the runtime `ibValueModuleManager` only because that's where the compile-tree currently lives.

**Affected files (~30):**

`metaCollection/partial/`:
- `catalogMetadata.cpp` (4×), `catalogManager.cpp`
- `documentMetadata.cpp`, `documentManager.cpp`
- `informationRegisterMetadata.cpp`, `informationRegisterManager.cpp`
- `accumulationRegisterMetadata.cpp` (4×), `accumulationRegisterManager.cpp`
- `accountingRegisterMetadata.cpp` (4×), `accountingRegisterManager.cpp`
- `chartOfAccountsMetadata.cpp` (4×), `chartOfAccountsManager.cpp`
- `chartOfCharacteristicTypesMetadata.cpp` (4×), `chartOfCharacteristicTypesManager.cpp`
- `dataReportMetadata.cpp`, `dataReportManager.cpp`
- `dataProcessorMetadata.cpp`, `dataProcessorManager.cpp`
- `enumerationManager.cpp`
- `constantMetadata.cpp`, `constantObject.cpp`
- `commonObject.cpp`

`metaCollection/`:
- `metaFormObject.cpp` (5×), `metaFormObjectProperty.cpp`
- `metaModuleObject.cpp` (5×), `metaObjectMetadata.cpp` (2×)

`backend/`:
- `metadataConfiguration.cpp` (1 live + comment)
- `moduleManager/moduleManagerExt.cpp` (7×) — extension functions

**Two viable strategies:**

a. **Move compile-tree onto `ibMetaData`.** Add `GetCompileTree()` returning a compile-only structure (just the list of `ibCompileModule*` keyed by metaobject GUID, no procUnit / root context / runtime state). Group 3 callers go through `m_metaData->GetCompileTree()`. Runtime-tree (the procUnit-bearing one) stays on session. Two parallel trees, but they only overlap at compile-side (same `ibCompileModule*` pointers; runtime extends each with its own procUnit).

b. **Lazy compile in session.** Metadata holds only source text / metaobject definitions. When session starts, it walks metadata, compiles, and builds its own tree. Group 3 mutators only mutate the *metaobject definitions* directly (no register/deregister); the next session pickup recompiles. Mid-session designer edits would need to invalidate / recompile per-session.

**Recommended: (a).** Lower risk, smaller diff. The compile-tree on metadata mirrors what's already there; we're just splitting runtime out into session and renaming the metadata-side accessor. Strategy (b) is correct long-term but is a bigger move (touches designer Save flow, AOT cache, hot-reload semantics).

**Effort:** ~30 sites, requires deciding compile-tree shape first.

### Group 4 — Save/load serialization (mostly Pattern B, no change)

`metadataConfigurationXML.cpp` and `metadataConfigurationJSON.cpp` use `GetModuleManager()` for two purposes:

- `catalog->GetModuleManager()` / `recordData->GetModuleManager()` — **Pattern B** (returns `ibValueMetaObjectCommonModule*`). Stays.
- A handful of `m_metaData->GetModuleManager()` (the ManagerModule path) — same as Group 3, follow Group 3 strategy.

### Group 5 — Documentation

- `docs/runtime-facade.md` — line 34, 269, 285, 334, 430 mention `activeMetaData->GetModuleManager()` as the legacy entry. Update to `ibSession::Current()->GetModuleManager()`.
- `docs/web/session-lifecycle.md` — line 203.
- `docs/ARCHITECTURE.md` — line 321.

---

## Recommended order of work (do not skip steps)

1. **Add `ibSession::GetModuleManager()` accessor** if not already canonical (memory note `project_runtime_descriptor_refactor.md` says it exists).
2. **Group 1 (runtime, ~7 sites)** — mechanical replacement. Build & run after each file. Smoke-test enterprise.exe + wenterprise-server.exe (open a form, run a script).
3. **Group 2 (designer, ~7 sites)** — replace with `ibSession::Current()->GetModuleManager()`. Smoke-test designer.exe (open code editor, autocomplete a symbol from a common module).
4. **Decide compile-tree shape for Group 3.** Most likely: a thin struct on `ibMetaData` exposing `FindCompileModule`, `AddCommonModule`, `RemoveCommonModule`, `GetCommonModules()`. Document in this file before coding.
5. **Group 3 (~30 sites)** — split into sub-batches by metaobject family (catalog, document, registers, charts, processors). One PR per family. Build green between each.
6. **Group 4 (XML/JSON)** — easy after Group 3.
7. **Remove `ibMetaData::GetModuleManager()` pure virtual** — last step. If the build is green and grep returns zero hits, remove the declaration.
8. **Group 5 docs** — update prose.

---

## Nuances and pitfalls

### Nuance 1 — `appData.cpp:532, 549` is OnStart/OnExit dispatch

These two sites are *not* just "any runtime call". They are the configuration-level `OnStart` / `BeforeStart` / `OnExit` / `BeforeExit` event dispatch. In the new model these belong to **session lifecycle**, not application lifecycle. Specifically:

- Desktop thick client: dispatch on `ibEnterpriseSession::OnCreateSession` / `OnDestroySession`.
- Web: dispatch on `ibWebClientSession::OnCreateSession` / `OnDestroySession`.
- Service / daemon: dispatch on `ibServiceSession`'s lifecycle.

Don't blindly replace `activeMetaData->GetModuleManager()` with `ibSession::Current()->GetModuleManager()` here — move the entire `Start()` / `Stop()` call into the session class. Otherwise we keep the singleton bias.

### Nuance 2 — `moduleManagerExt.cpp` is *the* extension API

This file (`backend/moduleManager/moduleManagerExt.cpp`) is C-style extension functions called from script:

```cpp
ibValueModuleManager* moduleManager = activeMetaData->GetModuleManager();
```

Script-callable functions like `BeforeStart`, `OnExit`, etc. land here. They are runtime by definition — `Current()` is non-null because the script is running. Replace with `ibSession::Current()->GetModuleManager()`.

### Nuance 3 — `metaModuleObject.cpp` (5 sites) is THE pivotal file

This file is where every common-module's `OnLoadMetaObject` / `OnSaveMetaObject` / `OnDeleteMetaObject` / `OnReloadMetaObject` registers itself with the parent moduleManager. If you get this file right, half of Group 3 follows by analogy. Look here first when designing the compile-tree API.

### Nuance 4 — Designer sometimes runs without a `ibSession`

Historically `designer.exe` could open a configuration without authenticating against the database (no DB → no session). Verify this is still true in the new architecture before assuming `ibSession::Current()` is non-null in designer. If yes, designer needs `ibDesignerSession` to start *before* configuration is loaded, which is the direction memory note `project_runtime_facade_plan.md` is going anyway.

### Nuance 5 — `ibValueModuleManager` itself owns the compile-tree currently

The pivot is: today `ibValueModuleManager` holds **both** the compile-tree (`m_compileModule`, `m_listCommonModuleManager` for design-time) and the runtime-tree (`m_procUnit`, root context, runtime instances of common modules). After the split:

- Compile-tree stays on `ibValueModuleManagerConfiguration` (or moves into a new sibling struct on metadata) — DESIGN-TIME, no procUnit.
- Runtime-tree stays on `ibValueModuleManager` instances owned by sessions — RUNTIME, has procUnit.

OR (cleaner):

- Rename `ibValueModuleManager` → `ibValueModuleRuntime` (runtime-only).
- Introduce `ibValueModuleCompileTree` (design-time-only).
- Configuration variant of moduleManager goes away; metadata holds compile-tree directly.

Decide before coding Group 3.

### Nuance 6 — Web reload + per-session metadata generation

Per memory note `project_connection_pool_metadata_reload.md`, web has metaGeneration + client banner for hot-reload. After the split:

- Compile-tree on metadata is recompiled on reload.
- Each session's runtime-tree must be rebuilt against the new compile-tree.
- ConnectionPool workers all dump and reinit their session-tree.

Today this is done by tossing the whole `ibValueModuleManager`. After split, both halves dump.

### Nuance 7 — `wxASSERT(moduleManager)` is everywhere in Group 3

The current code `wxASSERT(m_metaData->GetModuleManager())` will become an assertion on `m_metaData->GetCompileTree()`. The compile-tree should be **always non-null** for a loaded metadata (it is built during configuration load). If load fails, `m_metaData->GetCompileTree()` returns null and asserts fire — this is correct behaviour, no change needed.

For Group 1 sites (runtime), the assertion becomes `wxASSERT(ibSession::Current())` which is **already implicit** in worker scope discipline. Drop the assert or replace with a clearer error.

### Nuance 8 — Don't break `ibValueModuleManagerConfiguration`'s SYSTEM_TYPE_REGISTER

`moduleManager.cpp:372` registers `ibValueModuleManagerConfiguration` as a singleton-style class via `SYSTEM_TYPE_REGISTER`. Moving compile-tree out must preserve this registration if the class is still script-reachable as `ConfigModuleManager`. If the class is renamed/split, update the CLSID `"SO_COMM"` mapping carefully or scripts that reference it break.

---

## How to verify after each group

1. `MSBuild.exe enterprise.sln /p:Configuration=Debug /p:Platform=x86 /m` — must be green for all 12 projects.
2. Smoke tests:
   - **Group 1:** Start `enterprise.exe`, open any catalog, run `OnStart`/`OnExit` cycle (close window).
   - **Group 2:** Start `designer.exe`, open code editor on a common module, type a symbol — autocomplete must list it.
   - **Group 3:** Start `designer.exe`, open configuration, add a common module, save, reopen — module appears in tree and in compile tree.
   - **All groups:** Start `wenterprise-server.exe`, open two browser tabs, run a script in each, ensure no cross-talk.
3. Memory: run with one session active, run with 5 concurrent web sessions — `ibValueModuleManager` instance count should match session count + 1 (one config-side compile-tree).

---

## Why this is worth doing

`ibMetaData::GetModuleManager()` is the single most coupled accessor in the backend. Untangling it:

- Lets `ibConnectionPool` workers each own their runtime tree without races on a global mm.
- Removes a singleton from the type system (metadata is supposed to be passive description, not owner of running code).
- Makes the AOT bytecode cache plan tractable — compile-tree is hashable; runtime-tree is not.
- Closes the `ibSession::Current()` migration started in this session.

---

## Done in 2026-04-26 — `ibCompileValueCache` extraction

**Context.** Group 3 strategy (a) called for moving the compile-tree onto `ibMetaData`. A precursor — distinct from the runtime-bearing compile tree — landed first: the **compile-VALUE cache**. This is the designer-side table that maps `ibValueMetaObject*` → `ibValue*` (compiled-in-RAM module bindings used by the code editor for autocomplete, intellisense, and live preview). It had been bolted onto `ibBackendMetadataTree` (the designer's GUI tree of metaobjects) and accessed through `if (appData->DesignerMode()) tree->AddCompileModule(...)` gates scattered across ~58 callsites.

**What changed.**

- New `ibCompileValueCache` class on `ibMetaData` (`backend/metadata.h`/`.cpp`) — lifecycle-independent of any GUI structure. Owns a `std::map<const ibValueMetaObject*, ibCompileEntry>` with `ibValuePtr<ibValue>` plus optional `ibDeferredForm` for lazy form materialization.
- `ibDeferredForm` moved here from `backend_metatree.h` (no longer GUI-coupled).
- `ibMetaDataConfigurationStorage` constructor (designer-edit configuration) allocates the cache as `m_compileCache = std::make_unique<ibCompileValueCache>()`. Other configurations leave it null.
- Callsites pivoted from `metaTree->...CompileModule(...)` to:
  ```cpp
  if (auto* cc = m_metaData->GetCompileCache())
      cc->AddCompileModule(this, value);
  ```
  The null-check replaces the old `if (appData->DesignerMode())` runtime gate. ~58 callsites across 19 files migrated.
- `ibBackendMetadataTree` and `ibTreeMetaConfiguration` lost the cache API entirely. Tree is strictly UI-side again.

**What this unblocks.**

- Step 10 of `runtime-facade.md`'s plan ("Designer = compile + intellisense only") now has its primary storage in a non-GUI class. A headless designer compile / lint tool can use the cache without instantiating a wx tree.
- Group 3's strategy (a) — moving the *runtime* compile-tree onto `ibMetaData` — has a template to follow: same null-on-non-storage-configuration pattern, same callsite shape (`if (auto* x = md->GetX())`).

**What's still on `ibValueModuleManager`.** The runtime compile-tree (`m_compileModule`, `m_listCommonModuleManager`, root context, runtime instances of common modules) is unchanged. That's Group 3's actual scope.

---

## Done in 2026-04-26 — `CreateRoot` ownership pivot

Closely related — moves toward Step 1-2 of `runtime-facade.md`:

- `ibSession::EnsureRoot()` added — idempotent `CreateRoot(activeMetaData)` wrapper.
- `ibSessionRegistry::NotifyAuthenticated` now calls `s->EnsureRoot()` between the `OnFirstConnect` and `OnAuthenticated` listener phases. See `session-registry.md` for the full three-phase contract.
- `appData`'s `OnAuthenticated` listener no longer creates the root — it only drives `RunDatabase` (one-shot) + `CompileRoot` + `AttachRuntime`.

This makes the session the explicit owner of its root mm and removes the ordering hazard that `OnBeforeRunMetaObject` would fire against null `session->GetModuleManager()` during the very first auth.

---

## Restoration checklist (for next session)

1. Read this file end to end.
2. Verify build is green Debug|x86 (rollback baseline).
3. Confirm `ibSession::GetModuleManager()` exists and works (check `session.h`/`session.cpp`).
4. Pick Group 1 first. Do all 7 sites. Build. Smoke-test. Commit (with explicit user permission).
5. Group 2. Same drill.
6. **Pause before Group 3.** Decide compile-tree shape with the user. Update this doc with the decision before writing code.
7. Group 3 in sub-batches. Group 4. Group 5 docs.
8. Final removal of `ibMetaData::GetModuleManager()` from `metadata.h:23`.
