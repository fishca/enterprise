# Next-session plan

State at the end of the FB driver hardening + register-totals
strategy discussion on `2026-04-30`, on top of the ibNumber +
DDL-gate landing of `2026-04-29`. This file is the picking-up point
for the next round — not "everything ever planned", just what is
actionable now and easy to forget.

## Web debug F5 / multi-tab stability — landed 2026-04-30

Browser F5 on a tab whose session was parked at a breakpoint hung
`wenterprise-server` (worker thread parked in `DoDebugLoop`'s CV
wait, never released; `m_app->OnExit`'s `RunOnWorker(...).get()`
blocked indefinitely; queued new session waited forever for the
session row-level lock). With multiple tabs the debugger also
"detached itself" intermittently — connection listed in designer
but inert. Fixed by five small changes spanning session destroy,
the wire protocol, and the read loop:

1. **`ibSession::WakeDebugLoop()`** (session.h / session.cpp) —
   sets `m_forceExit` on the session (so the opcode loop unwinds
   on the next iteration after returning from `DoDebugLoop`) and
   notifies the per-session debug CV. Bypasses
   `RequestForceExit` — that path's `OnForceExit` would kill the
   wes process when started with `--debug`. Cancels only THIS
   session's interpreter.
2. **`ibWebSession::OnExit`** (webSession.cpp) — calls
   `WakeDebugLoop` *before* `DetachRuntime`, then drains
   the worker via `Submit([]{}).get()`. The FIFO queue guarantees
   that when the no-op runs, the parked-then-cancelled task has
   unwound and the worker is idle, so `DetachRuntime` no
   longer races the unwinding script over `ProcUnit` destruction.
3. **`ibDebuggerServerConnection::SendCommand` mutex** (debugServer.h /
   .cpp) — `std::mutex m_sendMutex` serialises the two-step
   `WriteMsg(length) + WriteMsg(payload)` pair. Without it,
   concurrent senders (multiple session workers emitting LeaveLoop
   on simultaneous F5s, or a worker writing while another emits
   EnterLoop) interleaved bytes on the wire and the designer
   dropped the connection on the next garbled frame.
4. **`IsConnected` no longer filters on `LastError()`**
   (debugServer.h) — `wxSocketBase::LastError()` is per-socket
   state, not per-thread. A transient `WOULDBLOCK` / `TIMEDOUT` /
   `IOERR` left by any operation on any thread leaked into the
   connection-thread's next liveness check and falsely declared
   the connection dead. Real disconnects still flip
   `IsConnected()` / `IsOk()` to false; ReadMsg / WriteMsg own
   their own short-read / short-write detection.
5. **Read-loop drops the inner `WaitForRead`** (debugServer.cpp,
   symmetric in debugClient.cpp) — sockets are constructed with
   `wxSOCKET_BLOCK | wxSOCKET_WAITALL`, so `ReadMsg` blocks until
   every requested byte is delivered. The previous
   `WaitForRead(0, 50ms)` between length and payload timed out
   under network jitter / multi-tab debug traffic and skipped the
   payload while the length was already consumed; the next outer
   iteration read payload bytes as a fresh length, tripped the
   16 MiB max-packet guard, and broke out → `ResetDebugger`. Side
   benefit: step commands feel snappier (one less polling cycle
   per message).

Status: landed 2026-04-30, pushed to `oes/develop` + `origin/develop`.
Split into `424b8d25 debug: stable wire under multi-tab F5 / step
workloads` (the five-change pile above) + `9b734795 docs: ...`.

## Firebird driver hardening — landed in working tree 2026-04-30

Audit and fix pass on the FB driver under embedded FB 5.0
deployment. Full write-up in `firebird-driver-hardening.md`.
Highlights:

- UB fixes: `XSQLDA` malloc / `delete[]` mismatches in result-set
  and parameter-collection cleanup; `wxStrncpy` over UTF-8 byte
  buffer in string-parameter binding.
- Result-set integer extraction: removed silent SQL_INT64 / INT128
  → `long` truncation, fixed `abs(scale)*10` scale arithmetic
  (was wrong for any |scale| ≥ 2), switched LP64-fragile
  `*(long*)` casts to `int32_t` `memcpy`.
- DPB additions: `isc_dpb_lc_ctype = UTF8` (charset on attach,
  `set_db_charset` only honoured at CREATE), `isc_dpb_force_write
  = 0` (async writes for fresh-CREATE — ~5–10× faster on Win NTFS),
  `isc_dpb_session_time_zone = UTC`.
- TPB additions: `isc_tpb_lock_timeout = 30s` on wait-mode (was
  blocking forever on contention), new read-only mode via
  `ibTxOptions::readOnly` (FB 4+ `read_consistency` for
  snapshot-style reads without write-intent locks).
- Atomic seed phase in `metadataConfigurationQuery::OnAfterSaveDatabase`
  — wraps the post-DDL repair loop in its own TX so a partial seed
  failure rolls back instead of leaving half-filled enum tables
  committed by per-statement quickie transactions.
- Idempotent probe in `ProcessPredefinedValue` (`SELECT 1 ...
  WHERE uuid = ?` before plain INSERT) so re-seeding existing tables
  doesn't PK-violate.
- `m_pageSize` widened to `int32_t` and bumped 8192 → 16384 (FB 5
  OLTP recommendation; cache footprint 32 MB per attach is fine).
- `fb_tr_list` stack collapsed to single `isc_tr_handle
  m_pTransaction` (base-class nesting counter makes the stack always
  one node deep).
- `HoldRowLocks` / `TryProbeRowLock` outer-TX guards (refuse if
  already in a TX — inner Begin would just bump the counter,
  silently breaking the lock semantics).
- `Open()` is now safe to re-call (detaches old handle first);
  recursive `Mkdir(..., wxPATH_MKDIR_FULL)`.

State: landed 2026-04-30, pushed to `oes/develop` + `origin/develop`.
Split into `7494bae5 firebird: driver hardening for FB 5.0 embedded`
(driver UB / DPB / TPB / fb_tr_list collapse / Open re-call) +
`6a0de70c metadata save: atomic seed phase + idempotent
predefined-record probe` + `9b734795 docs: ...`.

## Register totals — trigger-maintained strategy proposal

Proposal documented in `register-totals-strategy.md`. Move totals
maintenance from C++ (`*Query.cpp` per-register hand-written
UPSERT logic) to SQL triggers, keep totals table for read
performance, expose via views. Removes drift, removes the periodic
"пересчёт итогов" command, simplifies cross-register code in OES
core. Production target: PostgreSQL with 10K readers + 100 writers
profile; FB embedded as smaller-scale companion.

Status: design only. Not started. PoC plan:

1. Single-dim / single-resource accumulation register on PG.
2. Generate `mov_X` + `totals_X` + 3 triggers + 3 views via
   templated SQL emitter in `accumulationRegisterQuery::CreateAndUpdateTableDB`.
3. Read-path migration: `RegisterSet::GetBalance` etc. read
   through `vw_balance_X`.
4. Generalise to other register kinds + `updateMetaTable` flow
   (drop trigger / drop totals / rebuild / regenerate).
5. Per-driver dialect generator (PSQL for FB, PL/pgSQL for PG, etc).

The plain-VIEW alternative (no totals table, live aggregation only)
was discussed and rejected for production scale: at 10K readers on
a multi-million-movements register, live aggregation costs ~20–80
CPU-cores sustained. Trigger-maintained totals is ~1000× cheaper
on read at the cost of ~5–15% per-write trigger overhead.

## ibNumber lazy-grow exact decimal + ttmath removal — landed 2026-04-29

Replacement of `typedef ttmath::Big<128,128> ibNumber` with a
self-contained 8-byte tagged-immediate / heap-vector class. Full
rewrite of the number type plus follow-on cleanup of every call site
across backend / frontend / designer / launcher / codeRunner /
classChecker / wfrontend / wenterprise-server. ttmath library
removed from the tree entirely (13 files).

### What ibNumber is

- **`sizeof == 8`** on stack, regardless of value. Single `uint64_t`
  payload with bit-tagged immediate or pointer-to-heap.
- **Inline tier**: 47-bit signed mantissa + 16-bit signed exp10 +
  1-bit tag. Covers ±70 трлн × 10^±32767 — most business numbers
  never allocate.
- **Heap tier**: `BigImpl { std::vector<uint32_t> limbs; bool
  negative; int32_t exp; }`. Magnitude grows by demand, exact
  decimal preserved. Tested at 200-digit (100 fractional).
- **Self-contained**: zero `#include` of ttmath; all bigint
  algorithms (Add/Sub/Mul/Div/Compare, decimal ToString/FromString)
  live in `backend/number.cpp` as schoolbook routines. MSVC x86/x64
  Add/Sub use `_addcarry_u32`/`_subborrow_u32` intrinsics; portable
  64-bit fallback for clang/gcc/ARM.
- **Public API** (final):
  - Construction: `int`, `unsigned int`, `int64_t`, `uint64_t`,
    `double`, `wxString`, copy/move.
  - Arithmetic: `+ - * / %` and compound, pre/post `++`, unary `-`.
  - Compare: `== != < > <= >=`.
  - Conversions: `ToInt` / `ToUInt` (clamped) / `ToInt(int64_t&)`
    (overflow-detecting) / `ToInt64` (throws) / `ToDouble` /
    `ToFloat` / `ToString` / `ToWString` / `ToString(Format&)`.
  - Math compat: `Round` / `Round(n)` / `Trunc` / `Pow(int)` /
    `Pow(ibNumber)` / `Sqrt` / `Log(base)` / `Ln` / `Exp` / `Abs` /
    `IsSign` / `ChangeSign` / `SetZero` / `IsZero` / `IsNan` (always
    false — exact decimal has no NaN).
  - Buffer / stream:
    - `wxMemoryBuffer GetBuffer()` (returning) /
      `void GetBuffer(wxMemoryBuffer&)` (overwrite caller's buffer).
    - `bool GetBuffer(ibWriterMemory&)` — chunked stream write,
      uses internal `kIbNumberChunk` ID, hidden from callers.
    - `bool SetBuffer(const wxMemoryBuffer&)` /
      `bool SetBuffer(const void*, size_t)` /
      `bool SetBuffer(const ibReaderMemory&)`.
  - 128-bit raw: `To128Bytes(uint8_t[16])` /
    `From128Bytes(const uint8_t[16])` — for Firebird SQL_INT128.
  - Stream: `operator<<(std::ostream&, ibNumber)` and wide variant.
  - Format struct: `ibNumber::Format` with `fracDigits`,
    `precision`, `decimalSep`, `groupSep`, `groupSize` for OES
    `Format()` system function.

### Optimisations

- **Compact-zero encoding**: `GetBuffer()` for zero produces
  zero-byte buffer (no allocation, no header). `SetBuffer(empty) →
  zero`. `w_chunk` of zero is just 8-byte chunk header, zero
  payload. Saves IO on millions-of-zero DB columns.
- **No-churn-on-zero arithmetic**: `StoreBig` with zero result
  keeps the heap allocation if any (just clears limbs in place).
  Accumulator patterns (`ibNumber sum;` then `+= ...` with
  occasional reset to zero) don't thrash heap.
- **Single-pass `ToString` / `ToString(Format&)`**: digits generated
  backward into a `wchar_t` buffer, walked forward emitting sign,
  groups, decimal sep, fraction. No `std::string`, no `Mid`/`Find`,
  no UTF-8 round-trip. Direct `wxString(wchar_t*, len)` ctor.
- **Parser walks `wchar_t` directly** via `wxString::wc_str()` — no
  UTF-8 conversion, since digits/sign/sep/exp marker are all ASCII.

### Tests

- `enterprise/tests/test_number.cpp` — GoogleTest, ~50 `TEST(...)`
  cases covering layout, ctors, parser edges, ToString round-trip,
  arithmetic, compare across tiers, rounding, conversions, math
  compat, Get/SetBuffer (incl. compact-zero), 128-byte API, stream
  operators. Hooked into the existing `tests/CMakeLists.txt`
  alongside `test_clsid.cpp` / `test_value.cpp` (target `oes_tests`,
  GoogleTest fetched via FetchContent v1.14.0). Build with
  `cmake -B build -DBUILD_TESTING=ON`.
- `enterprise/src/engine/test/numberTest/numberTest.{cpp,vcxproj}` —
  temporary MSBuild stop-gap (custom main, wxEntryStart, prints to
  stdout + `numberTest.log`). Used while CMake build wasn't set up
  to include backend with `OES_USE_FIREBIRD=ON`. Run from
  `bin/Win32/Debug/numberTest.exe`. Most recent run: 111/111 PASS.
  Delete this project after CMake-side gtest is wired and proven.

### DDL gate via exclusive mode

`ibRestructureInfo::RequireExclusiveForDDL()` static helper —
checks `appData->ExclusiveMode()`, throws
`ibBackendCoreException::Error` if not set. Single call site at the
top of `OnBeforeSaveDatabase` (before `BeginTransaction`). Applies
to every Apply / save-database flow.

`ibRestructureInfo` itself was tidied:
- `enum class ibRestructure { info, warning, error }` (was bare
  enum with `restructure_*` prefixes).
- Inner type renamed `ibRestructureData` → `Entry { type, descr }`.
- Range-based for support (`begin()`/`end()`).
- Severity counters (`HasErrors()`, `HasWarnings()`, `IsEmpty()`).
- Legacy methods (`GetCount`, `GetType`, `GetDescription`,
  `HasRestructureInfo`, `ResetRestructureInfo`) preserved as thin
  aliases for back-compat with `applyChange.cpp`.

### Other cleanup

- `db_query` macro now resolves through `ThreadHolder` (per-thread
  reservation) instead of `CurrentHolder` (session-aware). Semantic
  split — runtime calls go through `Current()->X()`, service / DDL
  calls keep `db_query`. See
  `memory/project_db_query_runtime_split.md` for migration plan.
- `ibConnectionPool::DbQueryHolder` renamed to
  `ibConnectionPool::ThreadHolder`, made `static thread_local` —
  no longer process-wide singleton.
- `StrGetLine` rewritten — fixed `nIndex < 0` unsigned bug,
  thread-local cache (was process-wide static), handles `\r\n` /
  `\n` / `\r` line endings, no per-call source-string copy.

### Open items / known risks

- **Apply-flow UX**: gate is backend-only. Designer Apply still
  needs the user-flow described in
  `memory/project_apply_exclusive_ux.md` — pre-detect DDL vs
  code-only, kick-users dialog, auto-set exclusive. Without this,
  Apply on a multi-user system silently throws when DDL is
  attempted.
- **Wire-format break**: serialized blobs from the old ttmath::Big
  layout are not readable by `SetBuffer`. Production DBs with
  pre-existing TYPE_NUMBER blobs need migration. Fresh DBs OK.
- **CMake build with DB drivers OFF** fails to link backend
  (appData.cpp references Firebird/Postgres unconditionally). Test
  exe currently rides MSBuild; CMake gtest path needs
  `-DOES_USE_FIREBIRD=ON` or a structural fix that conditionally
  excludes the driver references.
- **Round-half mode parity**: `ibNumber::Round` is half-away-from-
  zero. Old ttmath default was round-half-down. `systemManagerFunc`
  emulates the latter via −0.4 nudge for the default branch — needs
  golden-test verification on financial 0.5-копеечные cases.
- **Performance**: not yet benchmarked. Div is base-2 long division
  (O(bits·limbs)); for 200-digit dividends this is microseconds —
  fine for typical workloads, but if a hot loop hits it, switch to
  Knuth Algorithm D (~50× speedup).

## Session-lifecycle hardening — landed 2026-04-29 (same day)

Pile of fixes around session creation / exclusive-mode flow that
surfaced during ibNumber's manual smoke testing. All bundled because
they share the apply-flow code path.

### Bug fixes

- **Designer-exclusive policy was permitting second designer.** The
  `ibDesignerExclusivePolicy` used `ProbeSessionRowLock`/`TryProbeRowLock`
  to decide alive vs zombie row. The historical `HoldRowLocks`
  liveness model was retired in favour of heartbeat (`lastActive`
  column updated by registry refresh tick; sweep deletes rows older
  than `kStaleCutoffSec`). With no row locks ever held, the probe
  always succeeded → policy treated every live designer as a zombie
  → permitted the new designer.
  Fix: any other `eDESIGNER_MODE` row visible in the snapshot now
  vetoes the new designer; sweep clears stale rows within ~30s for
  honest cleanup. `designerExclusivePolicy.cpp`.
- **Snapshot was stale during ProcessAdd policy check.** Race window
  between first session's INSERT and the periodic 1 Hz refresh tick
  meant the second arrival saw a stale snapshot and policies couldn't
  veto. `ProcessAdd` now calls `JobRefreshSnapshot()` (best-effort,
  caught) right before the policy chain. `sessionRegistry.cpp`.
- **`Failed to create session` swallowed the real reason.**
  `CreateSessionWithFactory` returned `nullptr` when `Connect` came
  back as `RejectedPolicy` / exclusive-mode block / etc., dropping
  `result.m_reason` on the floor. `FinishCreateSession` then threw the
  generic wrapper. Both `CreateSessionWithFactory` overloads now
  throw `ibBackendCoreException::Error(result.m_reason)` when the
  reason is non-empty, so the caller's `catch` shows
  `Another designer process is already running: <date>, <pc>, <user>`
  or `Database is in exclusive mode (...)` instead of the placeholder.
  `sessionRegistry.cpp`.
- **`enterprise.exe` and `designer.exe` crashed on session
  rejection.** The throw from `FinishCreateSession` /
  `CreateSessionWithFactory` propagated past `CreateSession<T>()` to
  wxApp's event loop → `OnUnhandledException` → wxDebugReport → crash
  dialog. Both main apps now wrap the `CreateSession + Open` pair in
  try/catch (covering `ibBackendException` and `std::exception`),
  display the actual rejection reason in a `wxMessageBox`, and exit 1
  cleanly. Web (`webSession.cpp`) was already wrapped — http handlers
  expected bool return, so this was always fixed there.
- **`ibAppEnterprise::OnUnhandledException` lost the exception
  type/message.** Stack trace at the catch site is past unwind, so
  the dump showed only wx's debug-report machinery and the original
  throw site was already gone. The handler now rethrows
  `std::current_exception()` to identify the type, writes a one-line
  diagnostic to `enterprise_unhandled.log` next to the exe (timestamp +
  exception class + message), and falls through to the existing dump
  path. Made finding "Failed to create session" possible.
- **DDL exclusive-mode gate was throwing through bootstrap path.**
  `RequireExclusiveForDDL` skips when `ibSession::Current() == nullptr`
  or registry isn't ready — this happens during first-open auto-save
  (`m_configNew=true` triggers `LoadDatabase → SaveDatabase` before
  any session is attached). Without the skip, fresh-IB open by
  `enterprise.exe` faulted instantly. `metaObject.cpp`.
- **`OnBeforeSaveDatabase` propagated unhandled exceptions to designer.**
  Catch widened to `ibBackendException` + `std::exception` + `(...)`;
  on any catch, error is appended to `s_restructureInfo` and the
  function returns false. Designer's apply pipeline already surfaces
  the ledger to the user. `metadataConfigurationQuery.cpp`.
- **`ProcessAdd` same-process exclusive parking → reject.** Briefly
  changed and reverted — parking is the documented in-process flow
  for re-entry under SetExclusive; broke testing scenarios so
  reverted. The cross-process gate (lines ~954-968) already does
  proper Reject with reason for peer holders.

### New design — exclusive-mode auto-acquire/release

`ibRestructureInfo::RequireExclusiveForDDL`:
- If `appData->ExclusiveMode()` already true → no-op (caller manually
  acquired earlier).
- Else if `ibSession::Current()` and registry available → submit
  `SetExclusive(session, true)` through the registry queue, block on
  verdict. Granted → mark `thread_local ts_acquiredByGate=true`,
  proceed. NotSole / HeldByOther → throw with clear reason ("other
  sessions are connected — disconnect them and try again").
- Else (bootstrap) → no-op.

`ibRestructureInfo::ReleaseAutoExclusive` releases iff `ts_acquiredByGate`
was set on this thread. Paired in `OnAfterSaveDatabase` via RAII
`AutoRelease` guard so even a throw from `OnSaveDatabase` between
acquire and after-save cleanly drops the flag.

### Optimisations (unrelated but bundled)

- **`StrGetLine` rewritten.** `systemManagerFunc.cpp:194`.
  - Replaced process-wide `static wxString _csStaticSource` cache
    (race in worker pool) with `thread_local Cache`.
  - Fixed `if (nIndex < 0)` bug — `find` returns `size_t`/`wxString::npos`,
    so the unsigned check never tripped; loop could read past end.
  - Replaced fixed `\r\n` separator with `find_first_of(wxT("\r\n"))`
    so Unix `\n`-only and old-Mac `\r`-only line endings work.
  - Dropped per-call `stringValue + "\r\n"` copy; works on `const
    wxString& src` directly.

### `ibRestructureInfo` tidy

- `enum ibRestructure` → `enum class ibRestructure { info, warning, error }`
  (was `restructure_info` etc with prefix).
- Inner `ibRestructureData` → `Entry { type, descr }`.
- Added `IsEmpty()`, `HasErrors()`, `HasWarnings()`, `Count()`,
  `At(idx)`, range-for `begin/end`. Severity counters maintained on
  `Append*`.
- Legacy methods (`GetCount`, `GetType`, `GetDescription`,
  `HasRestructureInfo`, `ResetRestructureInfo`) preserved as thin
  aliases for `applyChange.cpp` and other call sites.
- One direct-enum caller (`mainFrameDesignerEvent.cpp`) migrated to
  range-for + new enum literals.

### Open items

- Apply-flow UX (kick-users dialog, code-only-update branch, auto-set
  exclusive) still backend-only. See
  `memory/project_apply_exclusive_ux.md`.
- `db_query` → `Current()` runtime migration deferred. See
  `memory/project_db_query_runtime_split.md`.
- `ibConnectionPool` API surface (16 public methods including the
  TX trio and three release variants) — name-by-implementation
  rather than name-by-lifecycle. Cleanup pass deferred.
- ttmath credit in `frontend/win/dlgs/about.cpp:10` and stale
  ttmath-compat comments in number.h/cpp/systemManagerFunc.cpp/
  firebirdResultSet.cpp left as cosmetic followup.

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

### Cancellation use cases — landed 2026-05-02
- ✅ `ibWorkerPool::CancelSession` wired to the debugger Pause command
  (`debugServer.cpp::RecvCommand` / `CommandId_Pause`). Soft path
  (`m_bDebugStopLine`) still fires for scripts reaching line markers;
  hard path covers tight loops / native blocking calls.
- ✅ Wired to admin Kick (`sessionRegistry.cpp::JobCheckSignal` /
  `ibSessionSignal::Kick`). CancelSession runs before submitting
  Remove so OnExit / pool drain races against an idle worker.

### RAII guard for `SetEvalMode` — landed 2026-05-02
- ✅ `ibBackendException::ibEvalModeScope` (in `backend_exception.h`)
  is the new RAII guard. `ibProcUnit::Evaluate` drops three manual
  set/reset pairs.

### Pool / dispatch refinements
- `ibWorkerPoolGUI` is scaffold-only — no consumers. Either wire it
  (background-pool for heavy ops on desktop GUI to keep the wx event
  loop responsive) or delete as dead code.
- ✅ `PostWork` exception logging — landed 2026-05-02. New
  `LogWorkerException` helper in `workerPoolHeadless.cpp` emits
  `wxLogWarning` for both reentrant-inline and worker-thread-loop
  catches. RunOnWorker double-logs; acceptable cost.
- Web per-tab session lifecycle: confirm DropSession from
  ProcessRemove is hit on every teardown path (force-kill,
  idle-timeout, manual logout). Add an assert / counter.

### Runtime facade plan — Steps 1-11 + 13-17 landed 2026-05-04
- Steps 1-2 (descriptor-side runtime composition + Connect/Disconnect
  wiring), Step 3 (`Init/ExitRuntimeForSession` → `AttachRuntime/
  DetachRuntime` rename), Step 4 (Start/Stop on mm), Step 5
  (`ExecAsProc/Func` facade on `ibRuntimeModuleDataObject` + 38 object
  call-sites migrated), Steps 6-10 (parent chain, compile cache on
  descriptor, bytecode self-containment, designer-without-runtime),
  Step 11 (`bc.CreateBinder()` extern binder), Steps 13-17 (AOT cache
  pipeline) — all landed.
- **Step 12 remaining** — `ibMetadataRef{guid}` encoding for
  cross-bc metadata refs (`Catalogs.Товары` etc.) so AOT blobs
  survive descriptor renames. See `docs/runtime-facade.md` status
  banner for the up-to-date map.

### Sequence allocator — atomic landed 2026-05-04
- `GenerateNextIdentifier` uses `UPDATE ... RETURNING` for atomic
  increment + bootstrap-INSERT for first-ever call (FB / PG).
  Cross-machine safe via DB row lock; MySQL / ODBC throw with a
  clear message (not used in production OES). See
  `memory/reference_generate_next_identifier.md`.
- `ibNumber::Format::minIntDigits` added for leading-zero pad of
  the integer part (no int64 cap, big counters work). See
  `memory/reference_number_format_padding.md`.

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
