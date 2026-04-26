#include "webSession.h"

#include "backend/appData.h"
#include "backend/metadataConfiguration.h"
#include "backend/session/session.h"
#include "backend/session/sessionRegistry.h"
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
	return m_session;
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

	m_session = result.m_session;

	// Session-owned auth — unified path with desktop ibAppEnterprise /
	// ibAppDesigner flow. ibSession::Authenticate submits Attach through
	// the registry directly (shared_from_this), and on failure fires
	// OnShowAuthenticate. Base ibSession returns false there — web's
	// HTTP login form is the user-visible prompt, driven from the
	// client-side, not from a modal.
	const wxString wxUser = wxString::FromUTF8(user.c_str());
	const wxString wxPass = wxString::FromUTF8(password.c_str());
	if (!m_session->Open(wxUser, wxPass)) {
		m_session->Close();   // submit Remove → sys_session row DELETEd
		m_session = nullptr;
		return false;
	}

	m_user = user;

	// Spin up the per-cookie application. Session lifetime tied to
	// m_session — explicitly Close()d in OnExit / dtor.
	auto app = std::make_unique<ibWebApplication>();
	app->SetSessionContext(m_session);

	// CreateRoot / CompileRoot / InitRuntimeForSession are driven by
	// ibSessionRegistry::NotifyAuthenticated → appData::WireSessionEvents
	// (OnFirstConnect → metadataCreate; EnsureRoot; OnAuthenticated →
	// RunDatabase + CompileRoot + InitRuntimeForSession for runtime modes).
	// Both fired inside the m_session->Open(...) call above. Calling them
	// again here would re-execute the main module's top-level script.
	bool initOk = false;
	{
		ibSessionScope scope(m_session);
		initOk = app->OnInit();
	}
	if (!initOk) {
		m_session->Close();
		m_session = nullptr;
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
	// shared moduleManager before destroying the app + closing the session.
	// ibSessionScope pinned so ExitRuntimeForSession's bookkeeping is
	// consistent with the rest of the web runtime's expectations.
	if (m_session != nullptr) {
		ibSessionScope scope(m_session);
		if (activeMetaData != nullptr) {
			if (auto* mm = m_session->GetModuleManager())
				mm->ExitRuntimeForSession(m_session);
		}

		// Drop session's own root so its refs to metadata descriptors
		// release now, not later when the registry thread eventually
		// destroys the ibSession.
		m_session->ClearRoot();
	}

	if (m_app) {
		m_app->OnExit();
		m_app.reset();
	}

	// Submit Remove@Urgent through session — registry thread DELETEs the
	// sys_session row and releases the row lock. Safe to call here: the
	// worker already joined inside m_app->OnExit.
	if (m_session != nullptr) {
		m_session->Close();
		m_session = nullptr;
	}

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
