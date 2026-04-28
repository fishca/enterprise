# Next-session plan

State at the end of the codeEditor cleanup landing (on top of the
API-sweep in `c6299de6` + callTip fix in `b3d9f03d`). This file is
the picking-up point for the next round — not "everything ever
planned", just what is actionable now and easy to forget.

## Designer codeEditor cleanup — landed 2026-04-28

End-to-end pass over the autocomplete / IntelliSense / fold / print
path in `src/engine/designer/win/editor/codeEditor`. Purely internal
refactoring + a handful of real bug fixes; no public API change for
the rest of the designer.

### Bug fixes

- **`m_currentPos` shadow bug** (critical). Derived
  `ibPrecompileCode::m_nCurrentPos` was renamed to `m_currentPos`
  during the Hungarian sweep — but base `ibTranslateCode` already
  has an `m_currentPos` field (lexer's tokenize cursor). Derived
  shadowed base, derived field stayed at NSDMI 0, base stayed
  uninitialised (Debug heap pattern `0xCDCDCDCD`). Result:
  `ibTranslateCode::IsEnd()` returned true on the first call,
  `PrepareLexem` while-loop never ran, `m_listLexem` ended up with
  only the ENDPROGRAM marker. Symptom: fold and Enter-indent broken
  on every module load. Fix: derived renamed to `m_caretPos`
  (semantically distinct — the user's caret position for the
  IntelliSense walk, not the lexer cursor); `Clear()` and lexer
  paths once again resolve to the base field.
- **Autocomplete dropdown empty on blank lines.** `strCurrentWord`
  could carry stray `\r` / `\n` from `PrepareExpression` walking
  through whitespace lexems; `IsEmpty()` returned false on the
  control-char-only string, the `Find` filter rejected every real
  identifier, `m_aKeywords` stayed empty. Fix: `Append` trims
  whitespace before the filter check; if the trimmed word is empty
  the filter is skipped (show all items).
- **`PrepareExpression` CONSTANT branch leak.** A string literal
  appearing earlier in the source (e.g. `var = "hello"`) leaked into
  `currentWord` regardless of context — autocomplete then filtered
  every entry against `"hello"` and showed nothing. Fix: gate the
  literal capture on the `type` / `showCommonForm` / `getCommonForm`
  context, parenthesise the operator-precedence-bug `&& !hasPoint`.
- **Error message — NUL bytes between marker and code line.**
  `ibBackendException::FindErrorCodeLine` called
  `strError.Replace('\r', '\0')` — the char overload of
  `wxString::Replace` promotes the second arg to a 1-char string
  containing NUL, so every `\r` became a NUL in the user-visible
  error text. Fix: `Replace(wxT("\r"), wxEmptyString)`.

### Refactoring

- **`ibPrecompileCode` / `ibParserModule`.** Renamed from
  `CPrecompile*`. Hungarian stripped (`m_nCurrentPos` →
  `m_caretPos`, `m_pContext` → `m_activeContext`,
  `m_pCurrentContext` → `m_cursorContext`, `m_numCurrentCompile` →
  `m_cursor`, etc.). Modern C++17 init, NSDMI defaults, deleted
  copy-ctor on owning structs (`ibPrecompileFunction`).
- **`ibFoldLevelParser` (in `codeEditor.h`).** Replaced 5 separate
  `m_proc_count` / `m_func_count` / … fields with a single
  `std::array<short, FoldKindCount> m_counts`; chained `if/else`
  trees in `OpenFold` / `CloseFold` collapsed into table lookups
  (`OpenKindFor` / `CloseKindFor`). `FoldPoint` struct replaces the
  `std::pair<int, short>` magic.
- **`PrepareTABs` (codeEditorLoader.cpp).** ~230 LOC → ~80. Four
  near-identical branches now share a single `rewriteIndent` lambda
  + 3 free-function helpers (`StripEOL`, `AppendEOL`,
  `CountLeadingIndent`). Dead `else if (current_fold != 0 &&
  fold_level == 0)` branches in WHITE/BASE removed (unreachable).
- **Incremental `PrepareLexem(line, line_offset, …)`.** Empty-array
  guard at top (avoids `size() - 1` underflow on `unsigned int`).
  Confused `lexem_start_idx` (compared to a line number, used as an
  index) replaced with direct `i - 1` rewind. ENDPROGRAM branch
  merged with the trigger-line branch via an `atEndProgram` flag.
- **Autocomplete + calltip widgets.** Members of `ibAutoComplete` /
  `ibCallTip` got `m_`-prefix + private + NSDMI defaults; ctor body
  ditched explicit zero-init for fields that now have NSDMI.
- **Cyrillic comment cleanup.** Empty doc-stub blocks (`/** *
  IsNextDelimeter * : * - */`-style remnants from a cp1251 →
  UTF-8 conversion) replaced with real one-line summaries.

### Visual / UX polish

- **Autocomplete + calltip follow the editor zoom.** Font size and
  the listbox window size both scale with `wxStyledTextCtrl::GetZoom()`,
  so the dropdown / tip stay proportional when the user zooms the
  editor with `Ctrl+wheel`.
- **Zoom-event handler.** Margins (line-number, breakpoint, fold)
  are recomputed on `wxEVT_STC_ZOOM` so the gutter scales together
  with the font instead of staying at the unzoomed baseline width.
  `SetEditorSettings` uses the same `max(FromDIP(16),
  TextHeight(0))` formula for the initial sizing, so a saved zoom
  loaded from settings produces a correctly-sized gutter without
  waiting for the first wheel tick.

### Open / known issues

- **Print preview — sub-pixel gaps between characters.** wxSTC /
  Scintilla on Windows + GDI rounds per-glyph advance widths at the
  print-DC scale, producing visible spaces inside words on the
  preview page (e.g. `P rocedure`, `S howC omm onForm`). Switching
  to `wxSTC_TECHNOLOGY_DIRECTWRITE` fixes the rendering but breaks
  the font binding from settings — `SetFontColorSettings` calls
  `StyleSetFont(style, wxFont)` which Scintilla doesn't propagate
  the face name from under DirectWrite. Real fix needs to rewrite
  `SetFontColorSettings` to use explicit `StyleSetFaceName` /
  `StyleSetSize` instead of `StyleSetFont(wxFont&)`. Deferred.

## API-sweep — landed 2026-04-27

`ibApplicationData` is no longer the proxy / gateway for types that
have a logical home of their own. Each domain object now owns its
own table-level operations.

- **`ibSession` API-shrink.** Registry-only state-machine
  (`Transition`, `TransitionAuth`, `SetIdentity`, `SetInserted`,
  `Inserted`, `WaitForState`, `WaitForAuth`) and auth-flow setters
  (`SetUserInfo`, `SetSessionRawPassword`,
  `ClearSessionRawPassword`, `EnableDebug`, `DisableDebug`) moved to
  `private` under `friend ibSessionRegistry`. Public surface for
  external code is now ~15 methods (identity / state reads / control
  flags / `Submit` / `Open` / `Close` / `Detach` / `SetActivity`).
- **Registry façades for auth.** `ibSessionRegistry::InstallUser(s,
  info, pwd)` and `EnableDebugForSession(s)` are the single public
  entry points for what used to be ad-hoc poking at session
  internals from `appData::InstallUser` / `appData::Connect`.
- **`ibUserInfo`** (was `ibApplicationDataUserInfo`). Renamed,
  relocated DB / buffer I/O onto the type itself:
  `ibUserInfo::Read(guid|name)`, `Save`, `Serialize`, `Deserialize`,
  `HasAny`, `ListAll` (returns nested `Brief` projection — replaces
  `ibApplicationDataShortUserInfo`). `appData` no longer mediates
  any sys_user query.
- **`ibSessionSnapshot`** (was `ibApplicationDataSessionArray`).
  Renamed and relocated to `backend/session/sessionSnapshot.{h,cpp}`.
  `appData::GetSessionArray()` removed — call sites use
  `ibSessionRegistry::Instance().GetClusterSnapshot()` directly.
  Added cluster-wide aggregate helpers: `HasActiveUsers`,
  `IsUserActive(name)`, `CountByKind(ibSessionKind)`.
- **Per-session configuration language.**
  `ibSession::m_languageCode` (explicit override) +
  `m_resolvedLanguageCode` (cached `override || user-default`) —
  `GetLanguageCode()` is inline `const wxString&`, single field
  load, refreshed only by `SetLanguageCode` / `SetUserInfo`.
  `ibBackendLocalization::GetActiveLanguage()` /
  `SetActiveLanguage()` route through the bound session, with
  `ms_strUserLanguage` as process-default fallback. Closes the web
  multi-tab last-language-wins symptom (every tab now renders its
  own user's synonym translations).

Files touched: `session.{h,cpp}`, `sessionRegistry.{h,cpp}`,
`userInfo.{h,cpp}` (new .cpp), `sessionSnapshot.{h,cpp}` (new),
`appData.{h,cpp}`, `appDataQuery.cpp`,
`backend_localization.{h,cpp}`, `metadataConfiguration.cpp`,
`metaObjectMetadataProperty.cpp`, `userItem.cpp`,
`authorization.cpp`, `wfrontend.cpp`, `activeUser.cpp`,
`backend.vcxproj` / `.filters`.

## What landed earlier

- **Per-session interpreter state.** `ibSession::m_procUnitState`
  (`procUnitState.h`) holds `m_currentRunModule`, `m_runContext`
  stack, `m_errorPlace`, `m_recCount`. Accessed via
  `ibSession::GetPUState()`. `ibProcUnit`'s static accessors removed —
  callers go through `GetPUState()` directly. Hot loop in
  `ibProcUnit::Execute` caches the pointer once at function entry.
  See `docs/worker-pool-tls-audit.md`.

- **Worker pool.** `ibWorkerPool` abstract + `ibWorkerPoolHeadless`
  concrete (in `backend/session/`). Lazy spawn up to `maxWorkers`,
  idle-shrink with 60s timeout (`kMinIdle = 1` survivor), per-session
  FIFO + lease, reentrant Submit-on-current-session inline.
  `ibSessionRegistry` owns the pool; ctor picks `maxWorkers` per
  runMode (`PickWorkerCount`). `ibSession::Submit(task)` is the
  public dispatch API; `ibWebApplication::PostWork` /
  `RunOnWorker` forward to it. Per-session worker thread on
  `ibWebApplication` removed. `ibWorkerPoolGUI` scaffold lives in
  `frontend/session/` — wraps `wxTheApp::CallAfter` for GUI hosts;
  not auto-installed yet.

- **Cancellation + force-exit per session.** `ibSession::RequestCancel`
  / `IsCancelRequested` (atomic flag, interpreter checks every opcode
  via cached pointer; throws `ibBackendInterruptException`).
  `ibSession::RequestForceExit` / `IsForceExit` (one-shot atomic flag,
  fires `OnForceExit()` virtual once). `ibSession::Close(true)` folds
  in `RequestForceExit`. `ibSessionRegistry::CloseAll(force)` for GUI
  shutdown. Process-level `appData::ForceExit` /
  `SetProcessExitHook` removed; callers migrated.

- **Eval-mode + processing-error per session.** `m_evalMode`,
  `m_processingBackendError` atomics on `ibSession`. `ibBackendException::IsEvalMode` /
  `SetEvalMode` forward through `Current()`. Multi-tab debug works —
  three sessions in eval don't silence the other two.

- **Debug-thread Current() redirect.** `ibSessionRegistry`'s debug
  queue + `RegisterDebugThread` / `UnregisterDebugThread` /
  `EnterDebugLoop` / `LeaveDebugLoop` / `GetActiveDebugTarget`.
  `ibSession::Current()` on a registered debug-thread tid resolves to
  the front of the queue. Eval handlers
  (`AddExpression`/`ExpandExpression`/`EvalToolTip`/`EvalAutocomplete`,
  `SetStack`) use `ibSession::CurrentRunContext()` instead of the
  legacy `ms_debugServer->m_runContext`.

- **Connection pool growth/shrink.** `Init(primary, maxSize, minIdle = 2)` —
  pre-warms clones, idle-shrink at 60s, never below `minIdle`. Master
  pinned. Per-runMode `minIdle`: 4 web, 2 daemon, 2 GUI.

- **Registry slimmed.** `m_lockConn` removed (was retired with
  `HoldRowLocks` liveness model); two persistent checkouts
  (write + probe) remain.

- **Frame storage off `ibSession` base.** `GetFrame()` virtual
  returning nullptr default; `ibGUISession` and `ibWebClientSession`
  carry typed `m_frame` and override.

## Open items — next session

### Verify under load
- Spin up wes with 50+ concurrent browser tabs. Watch:
  - Worker pool grow/shrink (`AliveWorkers` / `IdleWorkers` getters).
  - Connection pool grow/shrink (`LiveSize` / `IdleSize`).
  - No "compilation failed" / "Context functions are not available"
    in server log under sustained load.
  - Multi-tab debug still keeps per-session isolation (eval-mode,
    cancel, force-exit) under churn.

### Observability
- `/admin/diag` HTTP endpoint on wes returning JSON:
  - `workerPool` { alive, idle, max }
  - `connectionPool` { live, idle, max, minIdle }
  - `sessions` snapshot (count, kinds, auth states)
  - `debugQueue` length + front session id
- Same data exposed in designer's Active Users dialog as extra columns.

### Cancellation use cases
- Wire `ibWorkerPool::CancelSession` to the **debug Pause** command.
  Currently `Pause` only sets `m_bDebugStopLine`; with `CancelSession`
  a long-running script outside a breakpoint can be interrupted.
- Wire to **admin Kick** (currently kicks via Remove submission).
  CancelSession before Kick gives the script a chance to wind down.

### RAII guard for `SetEvalMode`
- `ibValueSystemFunction::Evaluate` does manual set/reset around
  `ibProcUnit::Evaluate`. If the inner Evaluate throws, the flag
  leaks. Per-session storage limits the blast radius but the flag
  still bleeds across calls on the same session. Add a small
  `ibEvalModeScope` RAII guard.

### Pool / dispatch refinements
- `ibWorkerPoolGUI` is scaffold-only — no consumers. Either wire it
  (background-pool for heavy ops on desktop GUI to keep the wx event
  loop responsive) or delete as dead code.
- `PostWork` (fire-and-forget) silently drops exceptions from the
  task. At least log via `wxLogWarning` inside the worker before
  `set_exception` — current behaviour can mask timer-driven script
  bugs.
- Web per-tab session lifecycle: confirm DropSession from
  ProcessRemove is hit on every teardown path (force-kill,
  idle-timeout, manual logout). Add an assert / counter.

### Runtime facade plan
- Steps 1-2 of `docs/runtime-facade.md` (descriptor `m_runtimes` map +
  `ibSession::Submit` / `mm->Start/Stop()` integration) — preconditions
  shifted today. Re-read the plan, see what's already implicit and
  what still needs landing.

### Compute-server tiering — Phase 3 onwards
- Phase 2 (in-process shared dispatcher) is largely landed; verify
  `compute-server-tiering.md` against today's reality and trim the
  obsolete bits.
- Phase 3 (transport abstraction + binary RPC frame) — start of
  thin-client work. Out of scope for the immediate next session but
  good to keep the picture clean.

## Decisions to remember

- Worker pool is **owned by the session registry** (not appData).
  Pool config (`maxWorkers`) goes in registry's ctor. appData no
  longer has a `GetWorkerPool()` facade — callers use
  `ibSession::Submit` (transparent) or
  `appData->GetSessionRegistry()->GetWorkerPool()` (explicit).

- Force-exit and Cancel are **distinct concepts**:
  - Cancel = "interrupt current task; session keeps running".
  - ForceExit = "stop running on this session for the rest of its life".
  - `ibSession::Close(true)` raises ForceExit + submits Remove.
  - `ibSessionRegistry::CloseAll(force)` is the GUI app-shutdown
    entry point.

- `ibSession::GetPUState()` resolves through `Current()` each call.
  Hot loops (interpreter Execute) cache the pointer once at function
  entry; non-hot callers pay one shared_lock + map find per call,
  which is fine given the low frequency.

- Master connection in `ibConnectionPool` stays in the idle deque but
  is pinned against reaping (moved to the back if it would have been
  closed). Don't try to "fix" this back to a separate slot without
  also reviewing the legacy `db_query` fallback path that depends on
  the master being reachable from `m_source`.

## Files to re-read on context switch

- `docs/worker-pool-tls-audit.md` — TLS migration rationale + step
  plan (steps 4-5 done).
- `docs/connection-pool.md` — pool semantics + new minIdle / reap
  section.
- `docs/session-registry.md` — registry lifecycle, two persistent
  checkouts (write + probe).
- `docs/runtime-facade.md` — bigger runtime refactor; some adjacent
  preconditions landed.
- `docs/compute-server-tiering.md` — multi-process roadmap; Phase 2
  largely done.
