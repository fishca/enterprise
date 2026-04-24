#include "webSession.h"

#include "backend/appData.h"
#include "backend/metadataConfiguration.h"
#include "backend/session/session.h"
#include "backend/session/sessionRegistry.h"
#include "backend/session/sessionTicket.h"
#include "backend/moduleManager/moduleManager.h"

#include "webApplication.h"
#include "wfrontend.h"

namespace {

std::int64_t NowMs()
{
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

} // namespace

ibWebSession::ibWebSession(std::string id, std::string user)
	: m_id(std::move(id))
	, m_user(std::move(user))
	, m_lastActiveMs(NowMs())
{
}

ibWebSession::~ibWebSession()
{
	if (m_initialized)
		OnExit();
}

ibSession* ibWebSession::Session() const
{
	return m_ticket ? m_ticket->Session() : nullptr;
}

bool ibWebSession::OnInit()
{
	if (activeMetaData != nullptr) {
		const wxString& name = activeMetaData->GetConfigName();
		m_configName = std::string(name.mb_str(wxConvUTF8));
	}

	m_initialized = true;
	return true;
}

bool ibWebSession::Login(const std::string& user, const std::string& password)
{
	if (!m_initialized || appData == nullptr)
		return false;
	if (m_app)
		return true;  // already logged in — idempotent

	// Anonymous-phase Connect through the session registry. The anonymous
	// row lands in sys_session immediately (empty userName + eWEB_ENTERPRISE_MODE
	// app mode) so admin / Active-Users listings see "login in progress"
	// between GET / (cookie mint) and POST /login settling.
	//
	// Process run mode stays eWEB_ENTERPRISE_MODE both for the wes
	// technical session and for every browser tab — physically only the
	// server process is running. Disambiguation (WebServer vs WebClient)
	// lives in ibSessionKind, not ibRunMode.
	auto& reg = ibSessionRegistry::Instance();
	ibConnectRequest req;
	req.m_computer = appData->GetComputerName();
	req.m_appMode  = ibRunMode::eWEB_ENTERPRISE_MODE;
	req.m_kind     = ibSessionKind::WebClient;
	req.m_address  = wxString::FromUTF8(wfrontendServerAddress().c_str());
	// Reuse the tab's sessionStorage id (our SessionManager key) as
	// the registry's session guid — one identifier across header,
	// cookie, SessionManager, sys_session.session column.
	req.m_presetGuid = wxString::FromUTF8(m_id.c_str());
	// leave userName + password empty — Attach happens separately below
	// so a wrong password just rejects the Attach; the ticket (and the
	// anonymous row) can be retried with new creds.

	auto result = reg.Connect(req);
	if (result.m_code != ibConnectResult::Ok)
		return false;

	m_ticket = std::make_unique<ibSessionTicket>(std::move(result.m_ticket));

	// Attach credentials. Empty creds + empty sys_user = open-access pass-
	// through (AuthenticateUser returns true with empty info) → Ok.
	// Populated sys_user + wrong creds → WrongPassword → bail; caller
	// (HTTP /login handler) reports 401, ibWebSession is destroyed by
	// the session manager on the error path.
	const wxString wxUser = wxString::FromUTF8(user.c_str());
	const wxString wxPass = wxString::FromUTF8(password.c_str());
	ibAttachResult ar = m_ticket->Attach(wxUser, wxPass);
	if (ar != ibAttachResult::Ok) {
		m_ticket.reset();   // dtor → Remove@Urgent — sys_session row DELETEd
		return false;
	}

	m_user = user;

	// Spin up the per-cookie application. Session pointer borrowed from
	// the ticket — lifetime tied to m_ticket (dropped in OnExit / dtor).
	auto app = std::make_unique<ibWebApplication>();
	app->SetSessionContext(m_ticket->Session());

	// InitRuntimeForSession MUST run before app->OnInit(): OnInit calls
	// StartMainModule which fires OnStart; OnStart resolves its ProcUnit
	// through GetProcUnit() → Current()'s m_procUnitMap. Attach after
	// OnInit would leave the map empty at OnStart time, the delegate
	// would fall through to the now-empty legacy descriptor and OnStart
	// (and every default OpenForm it invokes) would be silently skipped.
	bool initOk = false;
	{
		SessionScope scope(m_ticket->Session());

		// Commit 2 (runtime facade plan): bind the session's own root
		// module manager. Per-tab WebClient session runs scripts →
		// needs a root. Readers still go through activeMetaData
		// singleton in this commit; migration lands next commit.
		if (activeMetaData != nullptr) {
			m_ticket->Session()->CreateRoot(
				activeMetaData,
				activeMetaData->GetCommonMetaObject());
		}

		if (activeMetaData != nullptr) {
			if (auto* mm = activeMetaData->GetModuleManager())
				mm->InitRuntimeForSession(m_ticket->Session());
		}
		initOk = app->OnInit();
	}
	if (!initOk) {
		m_ticket.reset();
		return false;
	}

	m_app = std::move(app);
	return true;
}

void ibWebSession::OnExit()
{
	if (!m_initialized)
		return;

	// Symmetric teardown — drop this session's ProcUnit entries from the
	// shared moduleManager before destroying the app + ticket. SessionScope
	// pinned to our session so ExitRuntimeForSession's bookkeeping is
	// consistent with the rest of the web runtime's expectations.
	if (m_ticket && m_ticket->Session()) {
		SessionScope scope(m_ticket->Session());
		if (activeMetaData != nullptr) {
			if (auto* mm = activeMetaData->GetModuleManager())
				mm->ExitRuntimeForSession(m_ticket->Session());
		}

		// Commit 2 symmetric: drop session's own root so its refs to
		// metadata descriptors release now, not later when the registry
		// thread eventually destroys the ibSession.
		m_ticket->Session()->ClearRoot();
	}

	if (m_app) {
		m_app->OnExit();
		m_app.reset();
	}

	// Ticket dtor submits Remove@Urgent — registry thread DELETEs the
	// sys_session row and releases the row lock. Safe to reset here:
	// the worker already joined inside m_app->OnExit, so no one is
	// holding a SessionScope on this session anymore.
	m_ticket.reset();

	m_initialized = false;
}

std::int64_t ibWebSession::LastActiveMs() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_lastActiveMs;
}

void ibWebSession::Touch()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_lastActiveMs = NowMs();
}
