# Next-session plan

State at the end of the worker-pool / per-session-state landing
(commit `97344bbf` on develop). This file is the picking-up point for
the next round — not "everything ever planned", just what is
actionable now and easy to forget.

## What landed today

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
