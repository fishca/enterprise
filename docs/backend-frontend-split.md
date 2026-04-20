# Backend / frontend DLL split — architecture review

This doc reviews the mechanism that keeps `backend.dll` UI-agnostic
while letting `frontend.dll` (desktop wx) or `wfrontend.dll` (web,
`OES_USE_WEB`) provide the concrete UI. The linchpin is
`ibBackendDocMDIFrame` — an abstract interface in backend, implemented
once per frontend, reachable everywhere via a `backend_mainFrame`
singleton pointer. It works, but it carries well-known design smells
worth documenting before the next scale step.

> **Naming note:** the "MDI" in `ibBackendDocMDIFrame` /
> `ibFrontendDocMDIFrame` / `GetDocMDIFrame` is a relic of the old
> wxWidgets MDI API. New code MUST NOT introduce the "MDI" suffix
> anywhere — use `Shell`, `HostFrame`, `DocFrame`, `DocParentFrame`,
> or plain `MainFrame`. Existing symbols are tracked for rename in
> "Suggested direction" below. References to the current symbol names
> in this review stay verbatim because the code still uses them.

## How it works today

**Interfaces in `backend/backend_mainFrame.h`:**

- `ibBackendDocMDIFrame` — abstract. Methods cover four unrelated
  concerns:
  - **Form factory**: `CreateNewForm`, `CreateFormUniqueKey`,
    `FindFormByUniqueKey*`, `UpdateFormUniqueKey`.
  - **Frame/window service**: `RaiseFrame`, `ActiveWindow`,
    `ActivateForm`, `ShowForm` side effects.
  - **Authentication**: `AuthenticationUser`.
  - **Misc metadata accessors**: `FindMetadataByPath`.

- `ibBackendValueForm` — the minimum form API for backend (LoadForm /
  SaveForm / BuildForm / ShowForm / Notify* / access flags). Frontend
  provides a concrete `ibValueForm` that implements all this on top
  of wxWidgets (or the web equivalent on wfrontend).

- `ibBackendControlFrame`, `ibBackendMetaDocument`,
  `ibSourceDataObject` — minimal surface for controls, documents,
  data sources. Same pattern: backend talks to the abstract, frontend
  downcasts to concrete in implementations.

**Wiring:**

- `backend_mainFrame` is a macro: `((ibBackendDocMDIFrame*)ibBackendDocMDIFrame::GetDocMDIFrame())`.
- On **desktop**, `GetDocMDIFrame()` returns a process-wide singleton
  pinned by `ibFrontendDocMDIFrame`'s constructor. One process = one
  user = one frame = fine.
- On **web**, `GetDocMDIFrame()` returns a `thread_local` slot pinned
  by `ibWebApplication::WorkerLoop` when its worker thread starts.
  One worker per session → thread-local is "session-local" by
  construction, so N concurrent sessions in one process don't clobber
  each other.
- On **headless** (daemon.exe, codeRunner.exe), the singleton is
  `nullptr`. Every static accessor in `backend_form.cpp` starts with
  `if (backend_mainFrame != nullptr) ... else Error(...)` to surface a
  clean `ibBackendCoreException::Error(_("Context functions are not available!"))`
  instead of a segfault.

## What works

- **backend.dll is truly UI-free.** `daemon.exe`, `codeRunner.exe`,
  `classChecker.exe` link backend without dragging in wx. The
  pattern has proven itself over years of desktop operation.
- **Swapping frontend is cheap.** `wfrontend.dll` replacing
  `frontend.dll` was possible largely because this interface was
  already in place — web didn't need to touch backend to publish its
  own `ibWebFrame`.
- **Polymorphism, not ifdefs.** Backend call sites stay identical
  for desktop and web. No compile-time branching at the call site; the
  virtual call dispatches at runtime.
- **Per-thread isolation on web** works well enough to run multiple
  sessions in a single `wenterprise-server.exe` process without the
  singleton clash we would have seen with process-wide state.

## What smells

1. **`ibBackendDocMDIFrame` is a god-interface.** Form factory, frame
   service, authentication UI, metadata lookup — all glued onto one
   vtable. Cohesion is low; a future component that needs only to
   raise a window ends up depending on form-creation too.

2. **Global singleton (even if thread-local).** Classic test-hostile
   pattern. Unit tests that touch any form code have to stand up a
   complete mainFrame or mock one. There's no clean way to inject a
   fake — everything reaches into the singleton. The `thread_local`
   trick on web sidesteps concurrency but keeps the ambient-state
   problem.

3. **15+ null-check call sites.** Every `ibBackendValueForm::Find*` /
   `CreateNewForm` accessor in `backend/backend_form.cpp` hand-checks
   `backend_mainFrame != nullptr`. A new method that forgets the
   check will crash headless silently. A null-object pattern — a
   `ibNoUIFrame` that throws `ibBackendCoreException::Error`
   uniformly from every method — would centralise this.

4. **Thread-local pinning is fragile.** Every thread that ends up
   calling backend code must `InstallOnThread` before the first call.
   Miss it and the thread-local slot is `nullptr`, so the call
   behaves as headless even though it's inside a live session. The
   debugger's `ibDebugServerScope` RAII pattern (see
   `debugServer.h:226`) is documented and present, but no one calls
   it — the pin happens via a one-shot `SetCurrent` in `WorkerLoop`
   instead. That's fine as long as every script path goes through
   the worker; the day one doesn't, we get hard-to-diagnose
   "breakpoints silently skipped" bugs. We saw a variant of this in
   the per-session debug listener test (see
   `docs/web/open-issues.md`).

5. **MDI naming implies structure that doesn't hold on web.** MDI
   means Multiple-Document-Interface — child windows inside a parent
   window. Web uses tabs with no nesting; some of the MDI semantics
   (z-order, active-child tracking) map awkwardly. Renaming the
   interface to `ibBackendShell` or `ibBackendHostFrame` would better
   describe what it actually is.

6. **Backend knows frontend concepts via the interface.** `ShowForm`
   takes `ibBackendMetaDocument*` as the parent — that document is
   really a `ibFormVisualDocument` (a frontend wxDocument subclass).
   The abstract interface has to carry enough types to let every
   frontend's concrete document fit through; if tomorrow a native-UI
   frontend needs a different document representation, the interface
   has to grow again.

7. **Auth UI is a side door.** `AuthenticationUser` is desktop-only
   by silent omission. Web leaves it at the base `return false`, so
   `StartSession` on web fails hard instead of prompting. We could
   wire a web analogue (see `docs/web/authentication.md`), but the
   fact that it hasn't been needed so far also hints the method is
   in the wrong interface — it's not a frame responsibility, it's an
   auth responsibility that happens to need a UI affordance.

## Suggested direction (not urgent)

The split is worth keeping — it's earning its cost. The specific
refactors that would pay off later:

- **Carve `ibBackendDocMDIFrame` into cohesive services.**
  - `ibBackendFormFactory` (CreateNewForm / Find* / UpdateFormUniqueKey).
  - `ibBackendFrameService` (RaiseFrame / ActiveWindow /
    ActivateForm).
  - `ibBackendInteractiveAuth` (AuthenticationUser — optional on
    headless and web).

  Each service is independently null-object'able, independently
  testable, and backend call sites only depend on the one they need.

- **Replace the global singleton with an explicit session
  context.** Prerequisite for the per-session state refactor
  (`project_web_session_bug.md`). Instead of `backend_mainFrame`, an
  `ibSession*` lives on the call stack (or in a narrow
  thread-local just at the entry points, not dereferenced from the
  depths). Concrete impl passes it through the call chain. Web
  naturally has one context per request; desktop has one forever.

- **Null-object pattern for headless.** No more `if (x != nullptr)`
  scattered through `backend_form.cpp` — either the service is
  bound to a real implementation or to a throw-only fallback. One
  place, one policy.

- **Rename.** `ibBackendDocMDIFrame` → `ibBackendShell` (or similar)
  once the service split settles. MDI is a desktop-isms leak.

None of this blocks current work. It starts paying off when:

- A third frontend needs to ship (native mobile UI? terminal UI for
  `codeRunner` interactive mode?) — the god-interface will make
  each new impl harder than necessary.
- PostgreSQL-scale production needs true per-session isolation with
  many concurrent users on one process — thread-local state will be
  the bottleneck.

Until then the current architecture is adequate: it works, it's
understood, and the smells are documented so nobody trips over them
twice. The `project_web_session_bug.md` plan is the obvious first
move when the scaling conversation gets real.
