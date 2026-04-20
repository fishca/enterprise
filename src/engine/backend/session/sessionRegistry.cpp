#include "sessionRegistry.h"
#include "sessionPolicy.h"

#include "backend/appData.h"
#include "backend/backend_exception.h"
#include "backend/guid.h"
#include "backend/databaseLayer/connectionPool.h"
#include "backend/databaseLayer/databaseLayer.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <utility>

#if defined(_WIN32)
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#	include <process.h>
#else
#	include <unistd.h>
#endif

#include <wx/log.h>
#include <wx/datetime.h>

namespace {

int CurrentPid()
{
#if defined(_WIN32)
	return static_cast<int>(GetCurrentProcessId());
#else
	return static_cast<int>(::getpid());
#endif
}

// Temporary diagnostic sink — mirrors [session …] stderr lines into a
// file so GUI apps (launcher → designer / enterprise) whose stderr is
// closed can still be diagnosed. Remove once session plumbing on
// server-mode DBs (PG / FB-server / MSSQL) is confirmed working.
std::mutex g_sessLogMtx;
void LogSession(const std::string& msg)
{
	std::ostringstream line;
	line << "[pid=" << CurrentPid() << "] " << msg;
	const std::string tagged = line.str();
	std::cerr << tagged << std::endl;
	std::lock_guard<std::mutex> lk(g_sessLogMtx);
	if (FILE* f = fopen("F:/projects/oes-work/session-diag.log", "a")) {
		fputs(tagged.c_str(), f); fputc('\n', f); fclose(f);
	}
}

} // namespace

#define SESSION_LOG(expr) do { std::ostringstream _o; _o << expr; LogSession(_o.str()); } while(0)

ibSessionRegistry& ibSessionRegistry::Instance()
{
	static ibSessionRegistry instance;
	return instance;
}

ibSessionRegistry::~ibSessionRegistry()
{
	// Best-effort — in normal shutdown Stop() should have been called via
	// ibApplicationData::Disconnect. Reaching the dtor with m_thread still
	// joinable means something forgot to stop; join here so the process
	// doesn't terminate with a running thread (which std::thread's dtor
	// would turn into std::terminate anyway).
	Stop();
}

// --- Legacy Phase-2 API ---------------------------------------------------

ibSession* ibSessionRegistry::Create(const std::string& id, ibRunMode runMode)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	auto s = std::make_unique<ibSession>(id, SessionKindFromRunMode(runMode));
	ibSession* raw = s.get();
	m_sessions[id] = std::move(s);
	return raw;
}

void ibSessionRegistry::Destroy(const std::string& id)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_sessions.erase(id);
}

ibSession* ibSessionRegistry::Find(const std::string& id)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_sessions.find(id);
	return it != m_sessions.end() ? it->second.get() : nullptr;
}

std::vector<std::string> ibSessionRegistry::List() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	std::vector<std::string> ids;
	ids.reserve(m_sessions.size());
	for (const auto& kv : m_sessions)
		ids.push_back(kv.first);
	return ids;
}

std::size_t ibSessionRegistry::Count() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_sessions.size();
}

// --- Thread lifecycle ----------------------------------------------------

void ibSessionRegistry::Start()
{
	if (m_threadAlive.load(std::memory_order_acquire)) return;
	if (m_fatal.load(std::memory_order_acquire))       return;

	// When we own sys_session, grab three dedicated pool connections:
	// one for the persistent lock TX, one for INSERT/UPDATE/DELETE
	// outside it, one for NOWAIT probes during Sweep. nullptr-tolerant
	// downstream — if the pool is not yet initialised (early startup /
	// test harness) the DB ops no-op gracefully.
	if (m_ownsSysSession) {
		if (auto* pool = ibApplicationData::GetConnectionPool()) {
			m_lockConn  = pool->Checkout();
			m_writeConn = pool->Checkout();
			m_probeConn = pool->Checkout();
		}
	}

	m_stop.store(false, std::memory_order_release);
	m_thread = std::thread([this]{ ThreadBody(); });

	// Wait until ThreadBody has entered its loop and flipped
	// m_threadAlive to true. Without this, a Connect / Submit issued
	// immediately after Start races the thread's start-up: the check at
	// the top of Connect sees m_threadAlive == false and bails with
	// RegistryDown. Short bounded wait — if the OS can't start a thread
	// in 2s something is seriously broken, fall through and let Connect
	// report RegistryDown for real.
	using namespace std::chrono;
	const auto deadline = steady_clock::now() + seconds(2);
	while (!m_threadAlive.load(std::memory_order_acquire) &&
	       !m_fatal.load(std::memory_order_acquire) &&
	       steady_clock::now() < deadline) {
		std::this_thread::sleep_for(milliseconds(1));
	}
}

void ibSessionRegistry::Stop()
{
	if (!m_thread.joinable()) return;
	{
		std::lock_guard<std::mutex> lk(m_submitMtx);
		m_stop.store(true, std::memory_order_release);
	}
	m_submitCv.notify_all();
	m_thread.join();

	// Release row locks + drop pool checkouts. shared_ptr custom deleter
	// on pool connections reparks them on the pool's idle list — no
	// explicit pool-side Return call needed.
	if (m_lockConn) {
		m_lockConn->ReleaseRowLocks();
		m_lockConn.reset();
	}
	m_writeConn.reset();
	m_probeConn.reset();
}

// --- Submit / Drain ------------------------------------------------------

void ibSessionRegistry::Submit(ibRegistryRequest req, ibPriority priority)
{
	// Dropped silently when the registry is stopped or fatal. Producers
	// detect the failure through the session's state machine (WaitState
	// times out, state remains Created).
	if (m_fatal.load(std::memory_order_acquire)) return;

	const std::size_t bin = static_cast<std::size_t>(priority);
	if (bin >= kPriorityBinCount) return;   // defensive — unreachable

	{
		std::lock_guard<std::mutex> lk(m_submitMtx);
		if (m_stop.load(std::memory_order_acquire)) return;
		m_bins[bin].push_back(std::move(req));
	}
	m_submitCv.notify_one();
}

// --- Connect -------------------------------------------------------------

ibConnectResult ibSessionRegistry::Connect(const ibConnectRequest& req,
                                           std::chrono::milliseconds timeout)
{
	ibConnectResult r;

	if (m_fatal.load(std::memory_order_acquire) || !m_threadAlive.load(std::memory_order_acquire)) {
		r.m_code   = ibConnectResult::RegistryDown;
		r.m_reason = m_fatalReason.IsEmpty() ? _("registry not running") : m_fatalReason;
		return r;
	}

	// Identity from req. Caller-supplied preset guid wins when valid
	// (web tab passes its sessionStorage tabSid so the id is shared
	// between browser header, cookie, SessionManager key, ibSession,
	// and sys_session.session PK — single identifier end-to-end).
	// Fresh mint only when the caller didn't set one.
	ibSessionIdentity identity;
	if (!req.m_presetGuid.IsEmpty()) {
		ibGuid candidate(req.m_presetGuid);
		identity.m_guid = candidate.isValid() ? candidate : wxNewUniqueGuid;
	}
	else {
		identity.m_guid = wxNewUniqueGuid;
	}
	identity.m_userName          = req.m_userName;
	identity.m_userGuid.clear();  // filled by Attach
	identity.m_computer          = req.m_computer;
	identity.m_address           = req.m_address;
	identity.m_appMode           = req.m_appMode;
	identity.m_started           = wxDateTime::Now();
	identity.m_pid               = CurrentPid();
	identity.m_expectsAnonPhase  = req.m_userName.IsEmpty();

	const std::string idStr = identity.m_guid.str();

	auto session = std::make_shared<ibSession>(idStr, req.m_kind);
	session->SetIdentity(identity);

	// --- Submit Add + wait for Created → Added / Rejected ---
	{
		ibRegistryRequest add;
		add.kind    = ibRegistryRequestKind::Add;
		add.session = session;
		Submit(std::move(add), ibPriority::Normal);
	}
	ibSessionState st = session->WaitForState(ibSessionState::Created, timeout);
	if (st == ibSessionState::Created) {
		r.m_code   = ibConnectResult::Timeout;
		r.m_reason = _("registry did not answer to Add");
		return r;
	}
	if (st == ibSessionState::Rejected) {
		r.m_code   = ibConnectResult::RejectedPolicy;
		r.m_reason = session->Reason();
		return r;
	}
	if (st != ibSessionState::Added) {
		r.m_code   = ibConnectResult::RegistryDown;
		r.m_reason = _("unexpected session state after Add");
		return r;
	}

	// --- Optional Attach for creds-supplied flow ---
	if (!req.m_userName.IsEmpty()) {
		{
			ibRegistryRequest att;
			att.kind     = ibRegistryRequestKind::Attach;
			att.session  = session;
			att.user     = req.m_userName;
			att.password = req.m_password;
			Submit(std::move(att), ibPriority::Normal);
		}
		ibAuthState auth = session->WaitForAuth(ibAuthState::Anonymous, timeout);
		if (auth == ibAuthState::Anonymous) {
			// Producer gave up; tear down the session so we don't leak an
			// orphan — Remove runs before we return.
			ibRegistryRequest rm;
			rm.kind    = ibRegistryRequestKind::Remove;
			rm.session = session;
			Submit(std::move(rm), ibPriority::Urgent);
			r.m_code   = ibConnectResult::Timeout;
			r.m_reason = _("registry did not answer to Attach");
			return r;
		}
		if (auth != ibAuthState::Authenticated) {
			// Auth failed — drop the session (anonymous row no longer useful).
			ibRegistryRequest rm;
			rm.kind    = ibRegistryRequestKind::Remove;
			rm.session = session;
			Submit(std::move(rm), ibPriority::Urgent);
			r.m_code   = ibConnectResult::RejectedAuth;
			r.m_reason = session->Reason();
			return r;
		}
	}

	// Success — hand over ticket ownership.
	r.m_code   = ibConnectResult::Ok;
	r.m_ticket = ibSessionTicket(session, this);
	return r;
}

std::vector<ibRegistryRequest> ibSessionRegistry::DrainAll()
{
	std::vector<ibRegistryRequest> out;
	// Snapshot all bins top-down under one lock. Strict descending:
	// Urgent → Normal → Low → Background. FIFO preserved within each bin.
	std::lock_guard<std::mutex> lk(m_submitMtx);
	std::size_t total = 0;
	for (auto& bin : m_bins) total += bin.size();
	out.reserve(total);
	for (auto& bin : m_bins) {
		while (!bin.empty()) {
			out.push_back(std::move(bin.front()));
			bin.pop_front();
		}
	}
	return out;
}

// --- Policy chain --------------------------------------------------------

void ibSessionRegistry::AddPolicy(std::unique_ptr<ibSessionPolicy> policy)
{
	if (!policy) return;
	// Only safe before Start(); the thread reads m_policies without a
	// lock. Callers wire policies during app bootstrap.
	m_policies.push_back(std::move(policy));
}

// --- Handlers ------------------------------------------------------------

static bool InsertSessionRow(ibDatabaseLayer* db, const ibSession& s, const wxString& userName)
{
	if (db == nullptr) {
		SESSION_LOG("[session INSERT] skip — db is null");
		return false;
	}
	const ibSessionIdentity& id = s.Identity();

	// Primary INSERT — core 6 columns present in every sys_session
	// since the original schema. Additional columns (pid / address /
	// currentActivity), added 2026-04-20, are filled by a separate
	// UPDATE below. Splitting these avoids a single INSERT failure
	// when the DB was created before the migration ran (the 9-column
	// INSERT would throw on unknown column and leave the session
	// unregistered, which breaks Active Users listings across the
	// cluster).
	ibStatementGuard stmt(db,
		db->PrepareStatement(
			wxT("INSERT INTO %s (session, userName, application, started, lastActive, computer) ")
			wxT("VALUES (?, ?, ?, ?, ?, ?);"),
			session_table));
	if (!stmt) {
		SESSION_LOG("[session INSERT] PrepareStatement returned null");
		return false;
	}
	stmt->SetParamString(1, wxString::FromUTF8(s.GetId().c_str()));
	stmt->SetParamString(2, userName);
	stmt->SetParamInt   (3, int(id.m_appMode));
	stmt->SetParamDate  (4, id.m_started);
	stmt->SetParamDate  (5, id.m_started);
	stmt->SetParamString(6, id.m_computer);
	try {
		stmt->RunQuery();
		SESSION_LOG("[session INSERT] ok guid=" << s.GetId()
		          << " user='" << (const char*)userName.ToUTF8().data() << "'"
		          << " mode=" << int(id.m_appMode));
	}
	catch (const ibBackendException& err) {
		SESSION_LOG("[session INSERT] FAILED: "
		          << (const char*)err.GetErrorDescription().ToUTF8().data());
		return false;
	}
	catch (...) {
		SESSION_LOG("[session INSERT] FAILED: unknown exception");
		return false;
	}

	// Best-effort population of extension columns — silently skipped
	// when the schema pre-dates them. MigrateTableSession tries to
	// add the columns on startup; this is the follow-up writer.
	ibStatementGuard stmtExt(db,
		db->PrepareStatement(
			wxT("UPDATE %s SET pid = ?, address = ?, kind = ? WHERE session = ?;"),
			session_table));
	if (stmtExt) {
		stmtExt->SetParamInt   (1, id.m_pid);
		stmtExt->SetParamString(2, id.m_address);
		stmtExt->SetParamInt   (3, int(s.GetKind()));
		stmtExt->SetParamString(4, wxString::FromUTF8(s.GetId().c_str()));
		try { stmtExt->RunQuery(); } catch (...) { /* legacy schema — fine */ }
	}

	return true;
}

static bool UpdateSessionUser(ibDatabaseLayer* db, const wxString& guidStr, const wxString& userName, const wxString& userGuid)
{
	if (db == nullptr) return false;
	ibStatementGuard stmt(db,
		db->PrepareStatement(
			wxT("UPDATE %s SET userName = ? WHERE session = ?;"),
			session_table));
	(void)userGuid;   // schema currently has no userGuid column — placeholder for when we add it
	if (!stmt) return false;
	stmt->SetParamString(1, userName);
	stmt->SetParamString(2, guidStr);
	try { stmt->RunQuery(); return true; } catch (...) { return false; }
}

static bool DeleteSessionRow(ibDatabaseLayer* db, const wxString& guidStr)
{
	if (db == nullptr) return false;
	ibStatementGuard stmt(db,
		db->PrepareStatement(
			wxT("DELETE FROM %s WHERE session = ?;"),
			session_table));
	if (!stmt) return false;
	stmt->SetParamString(1, guidStr);
	try { stmt->RunQuery(); return true; } catch (...) { return false; }
}

void ibSessionRegistry::ProcessAdd(ibRegistryRequest& req)
{
	if (!req.session) return;
	ibSession& s = *req.session;

	// Policy veto chain — first reject wins.
	for (auto& p : m_policies) {
		wxString reason;
		if (!p->CanAdd(s, reason)) {
			s.Transition(ibSessionState::Rejected, reason);
			return;
		}
	}

	// Claim ownership before touching DB — ProcessRemove finds us in
	// m_own regardless of whether the INSERT below succeeded.
	m_own[s.GetId()] = req.session;

	// DB ownership gate. When `m_ownsSysSession` is off, registry only
	// drives state transitions (useful for tests / embedders that keep
	// their own sys_session). When on, INSERT the anonymous row here
	// so peers can see the in-progress login via the Active Users
	// snapshot.
	//
	// Note: row-level pessimistic locks (HoldRowLocks) are NOT used
	// as the liveness signal anymore — holding `SELECT ... WITH LOCK`
	// on our own rows from a long-lived TX blocks any UPDATE to those
	// rows from a second connection, including our own `JobHeartbeatOwn`
	// refresh of `lastActive`. Liveness is now heartbeat-driven:
	// we UPDATE lastActive every refresh tick; sweep considers rows
	// whose lastActive is older than `kStaleCutoffSec` (see
	// JobSweepStale) as zombies.
	if (m_ownsSysSession && m_writeConn) {
		if (s.Identity().m_expectsAnonPhase) {
			if (InsertSessionRow(m_writeConn.get(), s, /*userName=*/wxEmptyString)) {
				s.SetInserted(true);
			}
			// Failed INSERT doesn't veto Add — session goes Added with
			// m_inserted=false. Heartbeat will skip it; no peer sees a
			// row; no cleanup needed. Best-effort during bring-up.
		}
	}

	s.Transition(ibSessionState::Added);
}

void ibSessionRegistry::ProcessAttach(ibRegistryRequest& req)
{
	if (!req.session) return;
	ibSession& s = *req.session;

	if (appData == nullptr) {
		s.TransitionAuth(ibAuthState::AuthFailed, _("appData unavailable"));
		return;
	}

	ibApplicationDataUserInfo info;
	const bool ok = appData->AuthenticateUser(req.user, req.password, info);
	if (!ok) {
		s.TransitionAuth(ibAuthState::AuthFailed, _("invalid user or password"));
		return;
	}

	// Open-access pass-through: AuthenticateUser returned true with an
	// empty info (no sys_user rows AND caller supplied no creds).
	// Transition to Authenticated anyway — from the caller's perspective
	// "auth is settled, you may proceed"; userInfo just stays empty,
	// mirroring the legacy AuthenticationAndSetUser pass-through path.
	if (!info.IsOk()) {
		s.TransitionAuth(ibAuthState::Authenticated);
		return;
	}

	// Install on this session directly. Legacy singleton mirroring is
	// the registry's responsibility: we bind Current() to the target
	// session for the InstallUser call so the dual-write reaches this
	// session's m_userInfo (not the main-thread session that happens
	// to be Current on the registry thread otherwise).
	{
		SessionScope scope(&s);
		appData->InstallUser(info, req.password);
	}

	// Surface the authenticated user in the row-level identity too.
	ibSessionIdentity id = s.Identity();
	id.m_userName = info.m_strUserName;
	id.m_userGuid = info.m_strUserGuid;
	s.SetIdentity(id);

	// DB wiring — two paths depending on whether the anonymous row
	// was already inserted by ProcessAdd.
	if (m_ownsSysSession && m_writeConn) {
		const wxString guidStr = wxString::FromUTF8(s.GetId().c_str());
		if (s.Inserted()) {
			// Was anonymous, now authenticated — update userName only.
			UpdateSessionUser(m_writeConn.get(), guidStr, info.m_strUserName, info.m_strUserGuid);
		} else {
			// Creds-first path — Connect(req) with non-empty user skipped
			// the anon-phase INSERT. Write the row now.
			if (InsertSessionRow(m_writeConn.get(), s, info.m_strUserName))
				s.SetInserted(true);
		}
	}

	s.TransitionAuth(ibAuthState::Authenticated);
}

void ibSessionRegistry::ProcessDetach(ibRegistryRequest& req)
{
	if (!req.session) return;
	ibSession& s = *req.session;
	s.SetUserInfo({});

	if (m_ownsSysSession && m_writeConn && s.Inserted()) {
		const wxString guidStr = wxString::FromUTF8(s.GetId().c_str());
		UpdateSessionUser(m_writeConn.get(), guidStr, wxEmptyString, wxEmptyString);
	}

	ibSessionIdentity id = s.Identity();
	id.m_userName.clear();
	id.m_userGuid.clear();
	s.SetIdentity(id);

	s.TransitionAuth(ibAuthState::Anonymous);
}

void ibSessionRegistry::ProcessRemove(ibRegistryRequest& req)
{
	if (!req.session) return;
	ibSession& s = *req.session;
	s.Transition(ibSessionState::Stopping);

	if (m_ownsSysSession && m_writeConn && s.Inserted()) {
		const wxString guidStr = wxString::FromUTF8(s.GetId().c_str());
		DeleteSessionRow(m_writeConn.get(), guidStr);
		s.SetInserted(false);
	}

	// Erase only if the map entry still points to the same ibSession we
	// are removing. On a refresh cycle with tabSid-preset guid, Add for
	// session2 replaces m_own[guid] with session2 before session1's
	// Remove runs; a blind erase here would yank session2's entry and
	// break subsequent heartbeat / snapshot for a live session.
	auto it = m_own.find(s.GetId());
	if (it != m_own.end() && it->second.get() == &s)
		m_own.erase(it);

	s.Transition(ibSessionState::Gone);
}

void ibSessionRegistry::ProcessSetActivity(ibRegistryRequest& req)
{
	if (!req.session) return;
	if (!m_ownsSysSession || !m_writeConn) return;
	if (!req.session->Inserted()) return;   // nothing to update yet

	const wxString guidStr = wxString::FromUTF8(req.session->GetId().c_str());
	ibStatementGuard stmt(m_writeConn.get(),
		m_writeConn->PrepareStatement(
			wxT("UPDATE %s SET currentActivity = ? WHERE session = ?;"),
			session_table));
	if (!stmt) return;
	stmt->SetParamString(1, req.activity);
	stmt->SetParamString(2, guidStr);
	try { stmt->RunQuery(); } catch (...) {}
}

void ibSessionRegistry::RebuildLockHold()
{
	if (!m_lockConn) return;

	std::vector<wxString> guids;
	guids.reserve(m_own.size());
	for (const auto& kv : m_own) {
		if (kv.second && kv.second->Inserted())
			guids.push_back(wxString::FromUTF8(kv.first.c_str()));
	}

	// HoldRowLocks commits the prior TX and reopens. Not all drivers
	// implement row locks (SQLite / PG / MySQL / ODBC default to false);
	// skip silently — registry stays in "state-only" mode for them.
	(void)m_lockConn->HoldRowLocks(session_table, wxT("session"), guids);
}

// --- Periodic jobs (placeholders) ----------------------------------------

void ibSessionRegistry::JobSweepStale()
{
	if (!m_ownsSysSession || !m_writeConn) return;

	// Liveness detection — `lastActive` staleness. Each process's
	// `JobHeartbeatOwn` UPDATEs lastActive every 1s on its own rows,
	// so any row whose lastActive trails `now` by more than
	// `kStaleCutoffSec` is treated as a zombie (force-killed or
	// otherwise dead owner).
	//
	// Row-lock probes (TryProbeRowLock) were tried as a fast path but
	// removed: nobody holds a long-running WITH LOCK on their own
	// rows anymore (that design ate a self-deadlock — see
	// docs/session-registry.md §4). Without HoldRowLocks the probe
	// just succeeds on *every* row, making it useless for
	// distinguishing alive from dead.
	ibDatabaseLayer* writer = m_writeConn.get();

	constexpr int kStaleCutoffSec = 10;   // 10× heartbeat interval — force-killed
	                                      // owners disappear from Active Users
	                                      // within ~10s of their last heartbeat

	wxDateTime cutoff = wxDateTime::Now();
	(void)cutoff.Subtract(wxTimeSpan(0, 0, kStaleCutoffSec));

	std::vector<wxString> zombies;
	try {
		ibStatementGuard stmt(writer,
			writer->PrepareStatement(
				wxT("SELECT session, lastActive FROM %s;"),
				session_table));
		if (!stmt) return;
		ibResultSetGuard rs(writer, stmt->RunQueryWithResults());
		if (!rs) return;

		while (rs->Next()) {
			const wxString guid = rs->GetResultString(wxT("session"));
			const std::string idStr = std::string(guid.ToUTF8().data());
			if (m_own.find(idStr) != m_own.end())
				continue;  // our own — heartbeat keeps lastActive fresh

			const wxDateTime lastActive = rs->GetResultDate(wxT("lastActive"));
			if (lastActive.IsValid() && lastActive.IsEarlierThan(cutoff))
				zombies.push_back(guid);
		}
	}
	catch (...) {
		return;  // transient DB error — swallow, try again next tick
	}

	if (!zombies.empty()) {
		SESSION_LOG("[session sweep] removing " << zombies.size()
		          << " zombie row(s) from sys_session");
	}
	for (const wxString& g : zombies) {
		try { DeleteSessionRow(writer, g); } catch (...) {}
	}
}

void ibSessionRegistry::JobHeartbeatOwn()
{
	if (!m_ownsSysSession || !m_writeConn) return;
	if (m_own.empty()) return;

	const wxDateTime now = wxDateTime::Now();
	ibStatementGuard stmt(m_writeConn.get(),
		m_writeConn->PrepareStatement(
			wxT("UPDATE %s SET lastActive = ? WHERE session = ?;"),
			session_table));
	if (!stmt) return;

	for (const auto& kv : m_own) {
		if (!kv.second || !kv.second->Inserted()) continue;
		try {
			stmt->SetParamDate  (1, now);
			stmt->SetParamString(2, wxString::FromUTF8(kv.first.c_str()));
			stmt->RunQuery();
		}
		catch (...) { /* transient — next tick retries */ }
	}
}

// Shared UPDATE path for admin directives. Uses the global `db_query`
// (not the registry's pool-checkout'ed connections) because the admin
// endpoints may be reached from callers — designer's Active Users
// dialog — that haven't checked out a pool connection themselves.
static bool WriteSessionSignal(const wxString& sessionGuid,
                                const wxString& signalValue)
{
	if (db_query == nullptr) return false;
	if (sessionGuid.IsEmpty()) return false;
	try {
		ibStatementGuard stmt(db_query,
			db_query->PrepareStatement(
				wxT("UPDATE %s SET signal = ? WHERE session = ?;"),
				session_table));
		if (!stmt) return false;
		stmt->SetParamString(1, signalValue);
		stmt->SetParamString(2, sessionGuid);
		stmt->RunQuery();
		return true;
	} catch (...) {
		return false;
	}
}

bool ibSessionRegistry::Kick(const wxString& sessionGuid)
{
	return WriteSessionSignal(sessionGuid, wxT("kick"));
}

bool ibSessionRegistry::Reload(const wxString& sessionGuid)
{
	return WriteSessionSignal(sessionGuid, wxT("reload"));
}

void ibSessionRegistry::JobCheckSignal()
{
	if (!m_ownsSysSession || !m_writeConn) return;
	if (m_own.empty()) return;

	ibDatabaseLayer* writer = m_writeConn.get();

	// Read-phase: collect (guid, signal) for each own row. Done through
	// a prepared SELECT because parameterised queries don't support
	// IN (?, ?, ?) portably — one statement per row keeps the code
	// portable at the cost of a few queries per tick.
	struct Pending { wxString guid; wxString signal; };
	std::vector<Pending> pending;
	try {
		ibStatementGuard stmt(writer,
			writer->PrepareStatement(
				wxT("SELECT signal FROM %s WHERE session = ?;"),
				session_table));
		if (!stmt) return;
		for (const auto& kv : m_own) {
			if (!kv.second || !kv.second->Inserted()) continue;
			const wxString guid = wxString::FromUTF8(kv.first.c_str());
			stmt->SetParamString(1, guid);
			ibResultSetGuard rs(writer, stmt->RunQueryWithResults());
			if (!rs || !rs->Next()) continue;
			const wxString sig = rs->GetResultString(wxT("signal"));
			if (sig.IsEmpty()) continue;
			pending.push_back({ guid, sig });
		}
	} catch (...) { /* signal column may be missing on legacy schema */ return; }

	if (pending.empty()) return;

	// Act-phase: dispatch the signal, then clear the cell so the
	// directive fires exactly once per admin write.
	ibStatementGuard clearStmt(writer,
		writer->PrepareStatement(
			wxT("UPDATE %s SET signal = NULL WHERE session = ?;"),
			session_table));

	for (const auto& p : pending) {
		SESSION_LOG("[session SIGNAL] guid=" << (const char*)p.guid.ToUTF8().data()
		          << " value='" << (const char*)p.signal.ToUTF8().data() << "'");

		if (p.signal == wxT("kick")) {
			// Find the own session and submit Remove@Urgent. Ticket dtor
			// elsewhere would do this on lifecycle end; here the admin
			// request replaces that signal path.
			const std::string idStr = std::string(p.guid.ToUTF8().data());
			auto it = m_own.find(idStr);
			if (it != m_own.end() && it->second) {
				ibRegistryRequest rm;
				rm.kind    = ibRegistryRequestKind::Remove;
				rm.session = it->second;
				Submit(std::move(rm), ibPriority::Urgent);
			}
		}
		else if (p.signal == wxT("reload")) {
			// Broad "reload metadata" directive — same effect as a Destroy
			// on every web-client session the target process owns. Clients
			// hit 401 on their next poll and re-land on /login with the
			// fresh metadata already loaded process-wide (metadata is
			// reloaded by a separate mechanism that watches sys_config).
			// `kick` on a single row → single-session kick; `reload` on
			// any row triggers process-wide client eviction so admin
			// doesn't have to enumerate every per-tab session.
			m_reloadRequested.store(true, std::memory_order_release);
		}
		// Future: "refresh" → broadcast SSE prod to client.

		if (clearStmt) {
			try {
				clearStmt->SetParamString(1, p.guid);
				clearStmt->RunQuery();
			} catch (...) { /* transient — retries next tick */ }
		}
	}
}

void ibSessionRegistry::JobRefreshSnapshot()
{
	if (!m_ownsSysSession || !m_writeConn) {
		static bool warned = false;
		if (!warned) {
			SESSION_LOG("[session REFRESH] skip — ownsSysSession="
			          << m_ownsSysSession << " writeConn="
			          << (m_writeConn ? "ok" : "null"));
			warned = true;
		}
		return;
	}

	// Re-SELECT the full table. Fresh snapshot built outside the lock;
	// swap it in under the writer lock so readers see either the old
	// or new one, never a half-mutated array.
	auto fresh = std::make_unique<ibApplicationDataSessionArray>();
	unsigned rowCount = 0;
	try {
		// kind column may be missing on pre-migration schemas — fall back
		// to reading it via a second pass-tolerant lookup. SELECT itself
		// stays restricted to core columns so the query parses everywhere;
		// kind is read best-effort (missing column would throw from some
		// drivers mid-stream and abort the snapshot).
		ibResultSetGuard rs(m_writeConn.get(),
			m_writeConn->RunQueryWithResults(
				wxT("SELECT userName, application, started, computer, session FROM %s ")
				wxT("ORDER BY started, session;"),
				session_table));
		if (rs) {
			while (rs->Next()) {
				fresh->AppendSession(
					static_cast<ibRunMode>(rs->GetResultInt("application")),
					0,  // kind filled in below for legacy-schema safety
					rs->GetResultDate  ("started"),
					rs->GetResultString("userName"),
					rs->GetResultString("computer"),
					rs->GetResultString("session"));
				++rowCount;
			}
		}
		// Second pass: augment with kind where the column exists. Wrapped
		// in its own try/catch so a legacy schema missing `kind` doesn't
		// torpedo the snapshot built above.
		try {
			ibResultSetGuard rsk(m_writeConn.get(),
				m_writeConn->RunQueryWithResults(
					wxT("SELECT session, kind FROM %s;"),
					session_table));
			if (rsk) {
				std::unordered_map<std::string, int> kindBySession;
				while (rsk->Next()) {
					kindBySession[std::string(rsk->GetResultString("session").ToUTF8().data())]
						= rsk->GetResultInt("kind");
				}
				fresh->SetKindsFromMap(kindBySession);
			}
		} catch (...) { /* legacy schema — fine, kinds stay 0 */ }
	}
	catch (const ibBackendException& err) {
		SESSION_LOG("[session REFRESH] SELECT failed: "
		          << (const char*)err.GetErrorDescription().ToUTF8().data());
		return;
	}
	catch (...) {
		SESSION_LOG("[session REFRESH] SELECT failed: unknown exception");
		return;
	}

	static unsigned lastCount = UINT_MAX;
	if (rowCount != lastCount) {
		SESSION_LOG("[session REFRESH] snapshot now has " << rowCount
		          << " row(s) (was " << (lastCount == UINT_MAX ? 0 : lastCount)
		          << ")");
		lastCount = rowCount;
	}

	std::unique_lock<std::shared_mutex> lk(m_snapshotMtx);
	m_snapshot = std::move(fresh);
}

ibApplicationDataSessionArray ibSessionRegistry::GetClusterSnapshot() const
{
	std::shared_lock<std::shared_mutex> lk(m_snapshotMtx);
	if (!m_snapshot) return {};
	return *m_snapshot;   // copy under read-lock
}

bool ibSessionRegistry::ProbeSessionRowLock(const wxString& sessionGuid)
{
	if (!m_probeConn) return false;   // policies get "assume alive" default
	return m_probeConn->TryProbeRowLock(session_table, wxT("session"), sessionGuid);
}

// --- Thread body ---------------------------------------------------------

void ibSessionRegistry::ThreadBody() noexcept
{
	m_threadAlive.store(true, std::memory_order_release);

	using clock = std::chrono::steady_clock;
	// Snapshot refresh runs at 1 Hz — cheap (one SELECT on sys_session).
	// Sweep runs at 3 s — heavier (one SELECT + one TryProbeRowLock per
	// cluster row that isn't ours) and its output (zombie DELETEs) is
	// not time-critical for UI.
	constexpr auto kRefreshInterval = std::chrono::seconds(1);
	constexpr auto kSweepInterval   = std::chrono::seconds(3);
	auto nextRefresh = clock::now() + kRefreshInterval;
	auto nextSweep   = clock::now() + kSweepInterval;

	// Eager initial sweep + snapshot — two reasons:
	//   - UI (Active Users) gets data immediately instead of waiting
	//     for the first refresh tick.
	//   - Zombie rows left over from force-killed previous runs get
	//     cleaned up at startup, so the new process's UI never shows
	//     a list polluted with stale entries. sweep is bounded by
	//     `kStaleCutoffSec` staleness check so even when the FB engine
	//     hasn't rolled back the orphan lock TXs yet, we still DELETE
	//     rows whose lastActive is older than the cutoff.
	try { JobSweepStale();      } catch (...) {}
	try { JobRefreshSnapshot(); } catch (...) {}

	try {
		while (!m_stop.load(std::memory_order_acquire)) {
			// Wait for work OR next refresh tick OR stop. Refresh is
			// the shorter timer so we pick it here.
			{
				std::unique_lock<std::mutex> lk(m_submitMtx);
				m_submitCv.wait_until(lk, nextRefresh, [this]{
					for (auto& bin : m_bins)
						if (!bin.empty()) return true;
					return m_stop.load(std::memory_order_acquire);
				});
			}
			if (m_stop.load(std::memory_order_acquire)) break;

			// Drain queue by strict descending priority.
			auto batch = DrainAll();
			for (auto& req : batch) {
				switch (req.kind) {
					case ibRegistryRequestKind::Add:         ProcessAdd(req);         break;
					case ibRegistryRequestKind::Attach:      ProcessAttach(req);      break;
					case ibRegistryRequestKind::Detach:      ProcessDetach(req);      break;
					case ibRegistryRequestKind::Remove:      ProcessRemove(req);      break;
					case ibRegistryRequestKind::SetActivity: ProcessSetActivity(req); break;
				}
			}

			// Refresh every second — UI polling sees updates within a
			// frame of each other instead of waiting a full sweep.
			const auto now = clock::now();
			if (now >= nextRefresh) {
				JobHeartbeatOwn();       // bump lastActive for every own row
				JobRefreshSnapshot();
				nextRefresh = now + kRefreshInterval;
			}

			// Sweep only every 3s — cluster-wide zombie cleanup, bounded
			// latency for dead-session pickup is fine. Signal check piggy-
			// backs on the same tick since it's cheap and similarly
			// latency-tolerant.
			if (now >= nextSweep) {
				JobSweepStale();
				JobCheckSignal();
				nextSweep = now + kSweepInterval;
			}

			m_tickCounter.fetch_add(1, std::memory_order_release);
		}

		// Graceful stop: drain urgent work one last time so pending
		// Removes get a chance to DELETE their rows before DB closes.
		auto final_batch = DrainAll();
		for (auto& req : final_batch) {
			if (req.kind == ibRegistryRequestKind::Remove)
				ProcessRemove(req);
			// Non-urgent pending requests dropped — producers waiting on
			// their session's cv observe Timeout / state-unchanged.
		}
	}
	catch (const std::exception& e) {
		wxString msg = wxString::Format(wxT("registry-thread exception: %s"), e.what());
		Die(msg);
	}
	catch (...) {
		Die(wxT("registry-thread unknown exception"));
	}

	m_threadAlive.store(false, std::memory_order_release);
}

// --- Fatal fail-stop -----------------------------------------------------

void ibSessionRegistry::Die(const wxString& why)
{
	m_fatalReason = why;
	m_fatal.store(true, std::memory_order_release);
	m_threadAlive.store(false, std::memory_order_release);

	// Log first — want this to survive even if terminate_handler blocks.
	std::cerr << "[session] FATAL: " << why.ToUTF8().data() << std::endl;
	wxLog::FlushActive();

	// std::terminate lets a custom terminate_handler log / dump before exit;
	// abort() would skip any registered handler. Either way the process
	// stops — registry-thread death means sys_session lies, nothing good
	// can come from continuing.
	std::terminate();
}
