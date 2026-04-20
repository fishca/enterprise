# Authentication flow — desktop vs web

OES has two entry points into authentication:

1. `ibApplicationData::StartSession(user, password)` — invoked at app
   startup (enterprise.exe / designer.exe) with credentials read from the
   command line or from launcher arguments.
2. `ibWebSession::Login(user, password)` — invoked per-session from the
   browser via `POST /w/<prefix>/login`.

Both funnel into `ibApplicationData::AuthenticationAndSetUser` which
reads the user row from `sys_user`, verifies the password through
`ibPasswordHash::Verify` (PBKDF2 with transparent MD5-legacy fallback,
and lazy upgrade via `NeedsRehash`), and stashes the result.

## StartSession fallback — AuthenticationUser

`StartSession` is the desktop path. If `AuthenticationAndSetUser` fails
(bad/empty CLI credentials), the session doesn't give up yet — it
falls back to the frontend:

```cpp
// src/engine/backend/appDataQuery.cpp (~712)
if (!AuthenticationAndSetUser(strUserName, strUserPassword)) {
    succes = false;
    if (backend_mainFrame != nullptr &&
        backend_mainFrame->AuthenticationUser(strUserName, strUserPassword)) {
        succes = true;
    }
}
```

`backend_mainFrame->AuthenticationUser` is a virtual method on
`ibBackendDocMDIFrame`. Concrete implementations:

- **Desktop** (`ibFrontendDocMDIFrame::AuthenticationUser`,
  `frontend/mainFrame/mainFrameParts.cpp:125`): shows the modal
  `ibDialogAuthentication` with the prefilled login/password fields,
  blocks until the user submits or cancels. Returns `true` on submit
  (dialog internally re-runs `AuthenticationAndSetUser` with the
  user-entered credentials).
- **Web** (`ibWebFrame::AuthenticationUser` — currently NOT overridden,
  base returns `false`): no fallback today. If StartSession ever runs
  on the web backend with invalid CLI credentials, it just fails.
- **Headless** (daemon / codeRunner / classChecker): `backend_mainFrame`
  is `nullptr`, so the fallback branch is skipped entirely. CLI
  credentials must be correct; failure exits cleanly.

## Why web doesn't hit StartSession today

Web credentials come from the browser via `POST /login`, not from the
server process command line. The flow is:

```
browser → GET / → wfrontendCreateSession → session w/o auth
        → POST /login (user, password)
          → wfrontendLogin → ibWebSession::Login
            → appData->AuthenticationAndSetUser
              → 200 OK (success) or 401 (failure, client re-shows form)
```

`StartSession` is invoked by `enterprise.exe` / `designer.exe` startup,
not by `wenterprise-server`. The web server binds its DB connection
without an "IB user" per se — that connection is a service account
configured via `--ibuser=` / `--ibpwd=` flags when launcher or designer
spawn the server process; `ibWebSession::Login` then attaches the real
end-user identity on top.

## TODO: interactive re-auth on web

If a future use-case needs CLI-driven auto-login on web (e.g. kiosk
mode pre-filling `--ibuser=...` / `--ibpwd=...` for a default user,
with the option to override interactively), we need a web equivalent
of `AuthenticationUser`. Proposal:

1. `ibWebFrame::AuthenticationUser` flags the session as "needs auth"
   (add `bool m_needsAuth` on `ibWebSession`, or a setter on
   `ibWebApplication`). Returns `false` — current auth attempt still
   fails.
2. Every HTTP endpoint that needs an authenticated session checks the
   flag at entry. If set, returns `401` with body
   `{"needsAuth": true, "reason": "re-auth required"}`.
3. `webClient.cpp` sees `needsAuth` in any fetch response and re-shows
   the login form overlay (same path as the initial login screen).
4. `POST /login` clears the flag on successful
   `AuthenticationAndSetUser`.

This keeps the desktop modal semantics without blocking the HTTP
handler thread — the server publishes a state, the client observes
and re-prompts asynchronously. It also composes with the existing SSE
live-update channel (the `needsAuth` signal can flow through `/stream`
as a special frame so the client can react even during idle polling).

Until that use-case materialises, web auth is a single-shot
POST /login with failure surfaced via HTTP status — no server-side
retry machinery.

## Per-session identity storage

`AuthenticationAndSetUser` dual-writes the resolved user identity to
both the legacy `ibApplicationData` singleton and to the current
session's `ibSession` (via `SessionScope::Current()`):

- `m_userInfo` — OES user row from `sys_user` (name, full name, guid,
  roles).
- `m_sessionGuid` — per-session DB guid (used as session identifier
  on the `sys_session` heartbeat row and in debug protocol handshake).
- `m_sessionRawPassword` — plain-text of the submitted password.
  Cleared in `CloseSession` / `ClearSessionRawPassword`. Never
  persisted to disk.

  **Why plain-text**: Designer's "Start debugging" spawns a child
  runtime process (enterprise.exe or wenterprise-server.exe) and
  needs to pass the current user's creds to it so the debugger
  attaches without re-prompting. The stored hash is useless here —
  `AuthenticationAndSetUser` re-verifies against whatever the child
  process pulls from `sys_user`, so plain-text is what must travel.
  Risk is bounded: only in-memory, wiped on logout, not written to
  disk. When / if a token-based one-shot debug handshake replaces the
  creds-passthrough, `m_sessionRawPassword` can be dropped.

Readers still go through `appData->GetUserInfo()` etc. — migration
of readers to `ibSession::Current()->GetUserInfo()` is a
separate phase (see `project_unified_session_architecture.md` step
2e(iii.a) → readers migration).

**Empty `userName` in `sys_session`** has three legitimate meanings,
not a single sentinel — see `reference_empty_username_meanings.md`
in the memory notes:

1. Open-access mode — `sys_user` table is empty, any cookie gets in.
2. Pending-auth — cookie minted via GET `/`, `/login` not completed
   yet; heartbeat row exists but user isn't known.
3. Web server technical session (Role::System) — process lives
   before any client connection; heartbeat row marks "server alive".

Admin listings should disambiguate via the `application` column
(ibRunMode enum) plus whether `sys_user` has any populated rows.

## Password storage

See `backend/utils/passwordHash.{hpp,cpp}` for the actual hash
primitive. New passwords are PBKDF2-HMAC-SHA256 at 600k iterations
(OWASP 2023 recommendation) with a 16-byte system-RNG salt, stored in
PHC format `$pbkdf2-sha256$<iter>$<saltB64>$<hashB64>`. `Verify` also
accepts legacy 32-hex MD5 strings for compatibility with pre-migration
databases; `NeedsRehash` returns true for those so `AuthenticationAndSetUser`
silently re-stores the hash in the modern format on the next successful
login.

Argon2id would be the stronger primitive (memory-hard, resists GPU
attacks) but requires vendoring a third-party library; revisit when
the threat model calls for it.
