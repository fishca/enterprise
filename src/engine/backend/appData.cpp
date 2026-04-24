////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxFormBuider
//	Description : app info
////////////////////////////////////////////////////////////////////////////

#include "backend/appData.h"

#include <thread>

#include <wx/filename.h>

#include "backend/utils/passwordHash.hpp"
#include "backend/plugin/pluginManager.h"
#include "backend/session/session.h"
#include "backend/session/sessionRegistry.h"
#include "backend/session/sessionTicket.h"
#include "backend/moduleManager/moduleManager.h"

//databases
#include "backend/databaseLayer/firebird/firebirdDatabaseLayer.h"
#include "backend/databaseLayer/postgres/postgresDatabaseLayer.h"
#include "backend/databaseLayer/sqllite/sqliteDatabaseLayer.h"
#include "backend/databaseLayer/connectionPool.h"

// GetDatabaseLayer — trivial delegate. The actual priority chain
// lives inside ibConnectionPool (TX > scope TL > primary). Kept here
// so the db_query macro's target stays stable; legacy call sites
// continue to see ibApplicationData::GetDatabaseLayer as the entry
// point even as the pool grows more responsibilities.
std::shared_ptr<ibDatabaseLayer> ibApplicationData::GetDatabaseLayer()
{
	return ibConnectionPool::GetDatabaseLayer();
}

//sandbox
#include "metadataConfiguration.h"

///////////////////////////////////////////////////////////////////////////////
//								ibApplicationDataSessionArray
///////////////////////////////////////////////////////////////////////////////

wxString ibApplicationDataSessionArray::GetUserName(unsigned int idx) const {
	if (idx > m_listSession.size())
		return wxEmptyString;
	return m_listSession[idx].m_strUserName;
}

wxString ibApplicationDataSessionArray::GetComputerName(unsigned int idx) const {
	if (idx > m_listSession.size())
		return wxEmptyString;
	return m_listSession[idx].m_strComputerName;
}

wxString ibApplicationDataSessionArray::GetSession(unsigned int idx) const {
	if (idx > m_listSession.size())
		return wxEmptyString;
	return m_listSession[idx].m_strSession;
}

wxString ibApplicationDataSessionArray::GetStartedDate(unsigned int idx) const {
	if (idx > m_listSession.size())
		return wxEmptyString;
	const wxDateTime& startedDate = m_listSession[idx].m_startedDate;
	return startedDate.Format(wxT("%d.%m.%Y %H:%M:%S"));
}

wxString ibApplicationDataSessionArray::GetApplication(unsigned int idx) const {
	if (idx > m_listSession.size())
		return wxEmptyString;
	const auto& row = m_listSession[idx];
	// ibSessionKind values mirror ibRunMode for 1:1 cases (Launcher /
	// Designer / Enterprise / Service), plus two web roles that share
	// the same ibRunMode::eWEB_ENTERPRISE_MODE. Pick a web-specific
	// label when the kind disambiguates, otherwise fall back to the
	// run-mode description.
	if (row.m_runMode == ibRunMode::eWEB_ENTERPRISE_MODE) {
		// ibSessionKind::WebServer = 5, WebClient = 100.
		if (row.m_kind == 100) return _("Web client");
		return _("Web server");
	}
	return appData->GetRunModeDescr(row.m_runMode);
}

///////////////////////////////////////////////////////////////////////////////

ibRunMode ibApplicationDataSessionArray::GetSessionApplication(unsigned int idx) const
{
	if (idx > m_listSession.size())
		return ibRunMode::eLAUNCHER_MODE;
	return m_listSession[idx].m_runMode;
}

int ibApplicationDataSessionArray::GetSessionKind(unsigned int idx) const
{
	if (idx > m_listSession.size())
		return 0;
	return m_listSession[idx].m_kind;
}

wxString ibApplicationDataSessionArray::GetSessionKindDescr(unsigned int idx) const
{
	if (idx >= m_listSession.size())
		return wxEmptyString;
	const auto& row = m_listSession[idx];
	// Only web runtime carries the server/client split; desktop modes
	// are always user-facing, reported as "Client" for consistency in
	// the Active Users dialog. Legacy schemas (no `kind` column) read
	// m_kind = 0; interpret that as Server when run-mode is web (the
	// historic behaviour before per-tab sessions landed) and Client
	// otherwise.
	if (row.m_runMode == ibRunMode::eWEB_ENTERPRISE_MODE) {
		// ibSessionKind::WebClient = 100.
		return (row.m_kind == 100) ? _("Client") : _("Server");
	}
	return _("Client");
}

void ibApplicationDataSessionArray::SetKindsFromMap(
	const std::unordered_map<std::string, int>& kindBySession)
{
	for (auto& u : m_listSession) {
		auto it = kindBySession.find(std::string(u.m_strSession.ToUTF8().data()));
		if (it != kindBySession.end())
			u.m_kind = it->second;
	}
}

///////////////////////////////////////////////////////////////////////////////
//								ibApplicationData
///////////////////////////////////////////////////////////////////////////////

ibApplicationData* ibApplicationData::s_instance = nullptr;

///////////////////////////////////////////////////////////////////////////////

bool ibApplicationData::m_forceExit = false;
wxCriticalSection ibApplicationData::m_cs_force_exit;

///////////////////////////////////////////////////////////////////////////////

ibApplicationData::ibApplicationData(ibRunMode runMode) :
	m_runMode(runMode),
	m_strComputer(wxGetHostName()),
	m_startedDate(wxDateTime::Now()),
	m_sessionGuid(wxNewUniqueGuid),
	m_pluginManager(std::make_unique<ibPluginManager>()),
	m_connectionPool(std::make_unique<ibConnectionPool>()),
	m_dbMode(ibDatabaseMode::eNONE),
	m_locale_lang(wxLanguage::wxLANGUAGE_UNKNOWN)
{
	// Load everything under <exe-dir>/plugins that exports the OES plugin ABI.
	// Launcher has no script/metadata subsystem so plugins have nothing to hook —
	// skip it there to avoid paying the scan cost on every connection chooser.
	if (runMode != eLAUNCHER_MODE)
		m_pluginManager->LoadAll();
}

ibApplicationData::~ibApplicationData()
{
	// Explicit early unload so plugins see Destroy() while the host is still
	// alive; also clears the vector before appData's other members die.
	if (m_pluginManager) m_pluginManager->UnloadAll();
}

// ---------------------------------------------------------------------------
// User-identity accessors — prefer the current thread's ibSession when a
// SessionScope is active (per-cookie on web, main user session on desktop);
// fall back to the process singleton only when no scope is set (pre-auth
// bootstrap, standalone codeRunner). Moved out-of-line so the header
// doesn't have to include session/session.h (which would create a cycle
// via userInfo.h's own transitive includes).
// ---------------------------------------------------------------------------

const ibApplicationDataUserInfo& ibApplicationData::GetUserInfo() const
{
	if (auto* ctx = ibSession::Current())
		return ctx->GetUserInfo();
	return m_userInfo;
}

const wxString& ibApplicationData::GetUserName() const
{
	return GetUserInfo().m_strUserName;
}

const wxString& ibApplicationData::GetUserPassword() const
{
	// Historical quirk: GetUserPassword returned fullName — kept for
	// source compat with call sites that still use the alias.
	return GetUserInfo().m_strUserFullName;
}

const std::vector<ibApplicationDataUserInfo::ibApplicationDataUserRole>&
ibApplicationData::GetUserRoleArray() const
{
	return GetUserInfo().m_roleArray;
}

ibGuid ibApplicationData::GetUserLanguageGuid() const
{
	return GetUserInfo().m_strLanguageGuid;
}

wxString ibApplicationData::GetUserLanguageCode() const
{
	return GetUserInfo().m_strLanguageCode;
}

wxString ibApplicationData::ComputeMd5() const
{
	return ComputeMd5(GetUserInfo().m_strUserPassword);
}

// Out-of-line so callers don't need to include sessionRegistry.h just to
// read the snapshot. Forwards to the registry's cluster-wide cache
// maintained by JobRefreshSnapshot.
ibApplicationDataSessionArray ibApplicationData::GetSessionArray() const
{
	return ibSessionRegistry::Instance().GetClusterSnapshot();
}

///////////////////////////////////////////////////////////////////////////////

bool ibApplicationData::CreateAppDataEnv(ibRunMode runMode)
{
	if (s_instance != nullptr) s_instance->DestroyAppDataEnv();
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	try {
#endif
		s_instance = new ibApplicationData(runMode);
		s_instance->ReadEngineConfig();

		if (!SetLocaleAppDataEnv())
			return false;
		return true;
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	}
	catch (const ibDatabaseLayerException* err) {
		return false;
	}
#endif
	return false;
}

#define sys_db wxT("sys.fdb")

bool ibApplicationData::CreateFileAppDataEnv(ibRunMode runMode, const wxString& strDirDatabase, const wxString& strLocale)
{
	if (s_instance != nullptr) s_instance->DestroyAppDataEnv();
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	try {
#endif
		std::shared_ptr<ibDatabaseLayerFirebird> db(new ibDatabaseLayerFirebird());

		wxString pathSep = wxFileName::GetPathSeparator();
		if (db->Open(strDirDatabase + pathSep + sys_db)) {
			s_instance = new ibApplicationData(runMode);
			s_instance->m_strFile = strDirDatabase;

			s_instance->ReadEngineConfig();

			s_instance->m_exclusiveMode = runMode == ibRunMode::eDESIGNER_MODE;

			s_instance->m_dbMode = ibDatabaseMode::eFILE;

			// The pool is the single owner of every connection in the
			// process. `db` here is the master — the pool holds it as
			// m_source for Clone() and also as the first idle entry so
			// the earliest Checkout hands it out directly. Size=32 —
			// slack over ~20 concurrent User sessions. Pool allocates
			// clones lazily so single-threaded runs (designer,
			// classChecker) open zero extra FB handles.
			s_instance->m_connectionPool->Init(db, 32);

			if (runMode == ibRunMode::eDESIGNER_MODE && !ibApplicationData::TableAlreadyCreated()) {
				ibApplicationData::CreateTableSession();
				ibApplicationData::CreateTableUser();
				ibApplicationData::CreateTableEvent();
			}
			else if (!ibApplicationData::TableAlreadyCreated()) {
				DestroyAppDataEnv();
				return false;
			}

			// Additive migration for pre-2026-04-20 schemas — registry's
			// INSERT / snapshot SELECT assume pid / address / currentActivity.
			ibApplicationData::MigrateTableSession();

			if (!SetLocaleAppDataEnv(strLocale))
				return false;

			return true;
		}
		return false;
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	}
	catch (const ibDatabaseLayerException* err) {
		return false;
	}
#endif
	return false;
}

bool ibApplicationData::CreateServerAppDataEnv(ibRunMode runMode, const wxString& strServer, const wxString& strPort,
	const wxString& strUser, const wxString& strPassword, const wxString& strDatabase, const wxString& strLocale)
{
	if (s_instance != nullptr) s_instance->DestroyAppDataEnv();
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	try {
#endif
		std::shared_ptr<ibDatabaseLayerPostgres> db(new ibDatabaseLayerPostgres());
		if (db->Open(strServer, strPort, strDatabase, strUser, strPassword)) {

			s_instance = new ibApplicationData(runMode);

			s_instance->m_strServer = strServer;
			s_instance->m_strPort = strPort;
			s_instance->m_strUser = strUser;
			s_instance->m_strPassword = strPassword;
			s_instance->m_strDatabase = strDatabase;

			s_instance->ReadEngineConfig();

			s_instance->m_exclusiveMode = runMode == ibRunMode::eDESIGNER_MODE;

			s_instance->m_dbMode = ibDatabaseMode::eSERVER;

			// Pool owns every connection. `db` becomes the master (source
			// for Clone + first hand-out). maxSize=32 covers heartbeat +
			// metadata watcher + ~20 concurrent sessions/SSE + slack. CLI
			// override can land later; lazy allocation keeps unused
			// capacity free.
			s_instance->m_connectionPool->Init(db, 32);

			if (!SetLocaleAppDataEnv(strLocale))
				return false;

			if (runMode == ibRunMode::eDESIGNER_MODE && !ibApplicationData::TableAlreadyCreated()) {
				ibApplicationData::CreateTableSession();
				ibApplicationData::CreateTableUser();
				ibApplicationData::CreateTableEvent();
			}
			else if (!ibApplicationData::TableAlreadyCreated()) {
				DestroyAppDataEnv();
				return false;
			}

			ibApplicationData::MigrateTableSession();

			return true;
		}
		return false;
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	}
	catch (const ibDatabaseLayerException* err) {
		return false;
	}
#endif
	return false;
}

bool ibApplicationData::SetLocaleAppDataEnv(const wxString& strLocale)
{
	if (s_instance == nullptr)
		return false;

	return s_instance->InitLocale(strLocale);
}

bool ibApplicationData::DestroyAppDataEnv()
{
	if (s_instance != nullptr && s_instance->m_connectionPool != nullptr &&
	    ibConnectionPool::GetPrimaryConnection() != nullptr) {

		if (!s_instance->Disconnect()) return false;

		s_instance->m_strServer = wxEmptyString;
		s_instance->m_strPort = wxEmptyString;
		s_instance->m_strUser = wxEmptyString;
		s_instance->m_strPassword = wxEmptyString;
		s_instance->m_strDatabase = wxEmptyString;

		// Pool shutdown closes every connection it owns — the master
		// (m_source) and every clone in m_idle. Outstanding hand-outs
		// are invalidated via the m_shutdown flag — their deleter
		// drops the captured pool ref instead of re-parking.
		s_instance->m_connectionPool->Shutdown();

		wxDELETE(s_instance);
		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////

#include <wx/fileconf.h>
#include <wx/stdpaths.h>

bool ibApplicationData::InitLocale(const wxString& locale)
{
	if (m_locale_lang == wxLanguage::wxLANGUAGE_UNKNOWN) {

#ifdef DEBUG_TRANSLATE
		wxLog::AddTraceMask(wxS("i18n"));
#endif // WXDEBUG

		m_locale_lang = wxLocale::GetSystemLanguage();
		if (m_locale_lang == wxLanguage::wxLANGUAGE_UNKNOWN)
			m_locale_lang = wxLanguage::wxLANGUAGE_DEFAULT;

		if (!locale.IsEmpty()) {
			const wxLanguageInfo* foundedLocale = wxLocale::FindLanguageInfo(locale);
			if (foundedLocale != nullptr)
				m_locale_lang = foundedLocale->Language;
		}
		else if (m_configInfo.IsSetLocale()) {
			const wxLanguageInfo* foundedLocale = wxLocale::FindLanguageInfo(m_configInfo.m_strLocale);
			if (foundedLocale != nullptr)
				m_locale_lang = foundedLocale->Language;
		}

		// Independently of whether we succeeded to set the locale or not, try
		// to load the translations (for the default system language) here.

		const wxString& workingDir = wxGetCwd();

		// normally this wouldn't be necessary as the catalog files would be found
		// in the default locations, but when the program is not installed the
		// catalogs are in the build directory where we wouldn't find them by
		// default

		wxFileName fn(wxStandardPaths::Get().GetExecutablePath());

		wxLocale::AddCatalogLookupPathPrefix(workingDir + wxFILE_SEP_PATH + wxT("lang"));
		wxLocale::AddCatalogLookupPathPrefix(fn.GetPath() + wxFILE_SEP_PATH + wxT("lang"));
#if defined(__WXOSX__) || defined(__APPLE__)
		// On macOS, also check outside .app bundle
		wxFileName bundleLang(fn.GetPath());
		bundleLang.RemoveLastDir(); bundleLang.RemoveLastDir(); bundleLang.RemoveLastDir();
		wxLocale::AddCatalogLookupPathPrefix(bundleLang.GetPath() + wxFILE_SEP_PATH + wxT("lang"));
#endif

		if (!m_locale.Init(m_locale_lang)) {
			if (!m_locale.Init(wxLanguage::wxLANGUAGE_ENGLISH))
				return false;
			m_locale_lang = wxLanguage::wxLANGUAGE_ENGLISH;
		}

		// Initialize the catalogs we'll be using.
		m_locale.AddCatalog(wxT("open_es"));

		// Initialize localization engine 
		ibBackendLocalization::SetUserLanguage(m_locale.GetName());

		//Set default time 
		wxDateTime::SetCountry(wxDateTime::Country::Country_Default);
		return true;
	}

	return false;
}

ibSession* ibApplicationData::GetMainSession() const
{
	return m_mainTicket ? m_mainTicket->Session() : nullptr;
}

bool ibApplicationData::Connect(const wxString& strUserName, const wxString& strUserPassword, const int flags)
{
	if (m_created_metadata)
		return false;

	// StartSession now creates the main ibSession via ibSessionRegistry::
	// Connect(req) and installs the SessionScope on this thread bound to
	// the ticket's session. The scope lives until CloseSession resets it,
	// so module manager / ProcUnit construction below sees a valid
	// ibSession::Current().
	if (!StartSession(strUserName, strUserPassword))
		return false;  //start session
	if (!metaDataCreate(m_runMode, flags))
		return false;
	m_created_metadata = true;
	m_run_metadata = activeMetaData->RunDatabase();

	// Commit 2 (runtime facade plan): bind the session's own root module
	// manager. Populated only for modes that actually run scripts — the
	// wes technical session (eWEB_ENTERPRISE_MODE → WebServer kind),
	// designer and launcher skip this. Per-cookie WebClient sessions are
	// handled separately in ibWebSession::Login.
	//
	// Readers still go through activeMetaData->GetModuleManager() (legacy
	// singleton) in this commit; the session-aware overload + reader
	// migration lands in the next commit.
	if (m_run_metadata && activeMetaData != nullptr &&
	    (m_runMode == eENTERPRISE_MODE || m_runMode == eSERVICE_MODE))
	{
		if (auto* session = m_mainTicket ? m_mainTicket->Session() : nullptr) {
			session->CreateRoot(
				activeMetaData,
				activeMetaData->GetCommonMetaObject());
		}
	}

	// Phase A: bring up per-session runtime for the current session.
	// Desktop's User session gets ProcUnits for main + common modules
	// under its m_procUnitMap, so subsequent meta->GetProcUnit() calls
	// resolve via the session (delegate in ibRuntimeModuleDataObject::GetProcUnit).
	// Web's System session returns early inside InitRuntimeForSession
	// (kind != user-runtime) so no runtime is spun up at server
	// startup. Explicit session from the main ticket — no thread_local
	// indirection.
	if (activeMetaData != nullptr && m_mainTicket) {
		if (auto* mm = activeMetaData->GetModuleManager()) {
			if (auto* session = m_mainTicket->Session())
				mm->InitRuntimeForSession(session);
		}
	}

	return m_connected_to_db &&
		m_created_metadata &&
		m_run_metadata;
}

bool ibApplicationData::Disconnect()
{
	bool success = true;

	if (m_created_metadata) {

		// Symmetric teardown — drop per-session ProcUnits before
		// DestroyMainModule tears down the shared compile state.
		// Explicit session from the ticket; no thread_local indirection.
		if (activeMetaData != nullptr && m_mainTicket) {
			if (auto* mm = activeMetaData->GetModuleManager()) {
				if (auto* session = m_mainTicket->Session())
					mm->ExitRuntimeForSession(session);
			}
		}

		// Commit 2: drop session's own root module manager BEFORE the
		// metadata tree goes down in CloseDatabase. The root's common
		// modules + compile state reference metadata descriptors; without
		// explicit clear, the root could outlive metadata (registry drains
		// session removal asynchronously) and its dtor would touch dead
		// pointers.
		if (m_mainTicket) {
			if (auto* session = m_mainTicket->Session())
				session->ClearRoot();
		}

		// CloseSession drops m_mainThreadScope + m_mainTicket. Ticket
		// dtor submits Remove@Urgent — drained by the registry thread
		// before Stop() joins it below. Keep going on failure so the
		// registry thread is always joined; otherwise the process
		// stays alive with an orphaned registry thread and the user
		// sees a zombie entry in tasklist after closing the window.
		if (!CloseSession())
			success = false;

		if (m_run_metadata) {
			const bool isConfigOpen = activeMetaData != nullptr && activeMetaData->IsConfigOpen();
			if (isConfigOpen && !activeMetaData->CloseDatabase(forceCloseFlag))
				success = false;
		}

		metaDataDestroy();
		m_created_metadata = false;
	}

	// Stop the registry unconditionally — even when we partially failed
	// above, the thread MUST be joined, otherwise the process can't
	// exit. The thread's graceful final-drain handles any pending
	// Remove requests (including the Urgent from m_mainTicket's dtor).
	ibSessionRegistry::Instance().Stop();

	return success;
}

#pragma region config

#define BACKEND_CONF wxT("backend.conf")

void ibApplicationData::ReadEngineConfig()
{
	const wxString& workingDir = wxGetCwd(); wxString strConfigFile;
	if (wxFileName::FileExists(workingDir + wxFILE_SEP_PATH + BACKEND_CONF)) {
		strConfigFile = workingDir +
			wxFILE_SEP_PATH + BACKEND_CONF;
	}
	else {
		wxFileName fn(wxStandardPaths::Get().GetExecutablePath());
		wxString exeDir = fn.GetPath();
		if (wxFileName::FileExists(exeDir + wxFILE_SEP_PATH + BACKEND_CONF)) {
			strConfigFile = exeDir + wxFILE_SEP_PATH + BACKEND_CONF;
		}
#if defined(__WXOSX__) || defined(__APPLE__)
		// On macOS, exe is inside .app/Contents/MacOS/ — check 3 levels up
		else {
			wxFileName bundlePath(exeDir);
			bundlePath.RemoveLastDir(); // MacOS
			bundlePath.RemoveLastDir(); // Contents
			bundlePath.RemoveLastDir(); // .app
			wxString bundleDir = bundlePath.GetPath();
			if (wxFileName::FileExists(bundleDir + wxFILE_SEP_PATH + BACKEND_CONF)) {
				strConfigFile = bundleDir + wxFILE_SEP_PATH + BACKEND_CONF;
			}
		}
#endif
	}

	wxFileConfig fc(wxT(""), wxT(""), wxT(""), strConfigFile);
	fc.Read(wxT("Locale"), &m_configInfo.m_strLocale);
}

#pragma endregion 

///////////////////////////////////////////////////////////////////////////////
#include "backend/debugger/debugClient.h"

#pragma region execute 
long ibApplicationData::RunApplication(const wxString& strAppName, bool searchDebug, bool useManifest) const
{
	// Hand the child process the raw password captured at login, not the stored hash.
	// Otherwise enterprise.exe authenticates with the hash itself — which only worked
	// before MD5→PBKDF2 because the verifier silently treated hash==hash as a match,
	// and even then it let the stored hash act as a bearer token.
	return RunApplication(strAppName,
		m_userInfo.m_strUserName,
		m_sessionRawPassword,
		searchDebug,
		useManifest
	);
}

long ibApplicationData::RunApplication(const wxString& strAppName, const wxString& strUserName, const wxString& strUserPassword, bool searchDebug, bool useManifest) const
{
	// All OES binaries spawn with unified `--flag=value` syntax using
	// wenterprise-server's long-name set (server/dbport/db/user/password/
	// file/ibuser/ibpwd/locale/debug). enterprise.exe / designer.exe /
	// daemon.exe declare these as the long name of their legacy short
	// options, so one builder feeds every parser.
	wxString executeCmd = strAppName + wxT(' ');

	if (m_strFile.IsEmpty()) {

		if (!m_strServer.IsEmpty())
			executeCmd += wxString::Format(wxT(" --server=%s"), m_strServer);
		if (!m_strPort.IsEmpty())
			executeCmd += wxString::Format(wxT(" --dbport=%s"), m_strPort);
		if (!m_strDatabase.IsEmpty())
			executeCmd += wxString::Format(wxT(" --db=%s"), m_strDatabase);
		if (!m_strUser.IsEmpty())
			executeCmd += wxString::Format(wxT(" --user=%s"), m_strUser);
		if (!m_strPassword.IsEmpty())
			executeCmd += wxString::Format(wxT(" --password=%s"), m_strPassword);
	}
	else {
		executeCmd += wxString::Format(wxT(" --file=%s"), m_strFile);
	}

	if (searchDebug)
		executeCmd += wxT(" --debug");

	if (!strUserName.IsEmpty())
		executeCmd += wxString::Format(wxT(" --ibuser=%s"), strUserName);

	if (!strUserPassword.IsEmpty())
		executeCmd += wxString::Format(wxT(" --ibpwd=%s"), strUserPassword);

	executeCmd += wxString::Format(wxT(" --locale=%s"), m_locale.GetName());

	// Manifest-mode: wes-style spawn. Skip the debug-client handshake —
	// that's enterprise.exe's in-process debugger attach, not relevant
	// to a headless wes child.
	if (useManifest)
		return SpawnWebServerWithManifest(executeCmd);

	const long execute = wxExecute(executeCmd);

	if (searchDebug) {

		unsigned short num_attempts = 0;

		debugClient->SearchServer(true);
		while (debugClient != nullptr) {

			if (debugClient->GetConnectionSuccess())
				break;

			if (num_attempts > 300)
				break;

			num_attempts++;
			wxMilliSleep(5);
		}
	}

	return execute;
}

long ibApplicationData::SpawnWebServerWithManifest(wxString cmd)
{
	// --port=0 → OS picks an ephemeral port. --manifest=<file> → wes writes
	// host/port/prefix/url there once it is actually accepting connections.
	const wxString manifestPath = wxFileName::CreateTempFileName(
		wxT("oes-wes-"));
	cmd += wxString::Format(wxT(" --port=0 --manifest=\"%s\""), manifestPath);

	// Spawn detached so the caller can exit without killing the server.
	const long pid = wxExecute(cmd, wxEXEC_ASYNC);
	if (pid == 0) {
		wxRemoveFile(manifestPath);
		return 0;
	}

	// Poll manifest up to 5s in the calling thread. wxYieldIfNeeded
	// lets the GUI dispatch paint / mouse events so the window stays
	// responsive while wes's InitBackend (metadata + DB connect) runs
	// for ~1s. Detaching the poll to a worker thread was tried but
	// raced with the caller's Close(true) / wxApp teardown — browser
	// sometimes got launched after wx state was already half-torn-
	// down, ending up on a URL the running wes wasn't yet serving.
	// Synchronous poll keeps the caller alive long enough for
	// wxLaunchDefaultBrowser to fully ShellExecute before return.
	wxString url;
	for (int waited = 0; waited < 5000; waited += 100) {
		wxMilliSleep(100);
		wxYieldIfNeeded();
		wxFile f;
		if (!f.Open(manifestPath, wxFile::read))
			continue;
		wxCharBuffer buf(static_cast<size_t>(f.Length()));
		if (f.Read(buf.data(), f.Length()) <= 0) {
			f.Close();
			continue;
		}
		const wxString content = wxString::FromUTF8(buf.data(), f.Length());
		f.Close();
		const int pos = content.Find(wxT("url="));
		if (pos == wxNOT_FOUND)
			continue;
		url = content.Mid(pos + 4);
		url = url.BeforeFirst(wxT('\n'));
		url.Trim().Trim(false);
		if (!url.IsEmpty())
			break;
	}
	wxRemoveFile(manifestPath);
	if (!url.IsEmpty())
		wxLaunchDefaultBrowser(url);

	return pid;
}

#pragma endregion

///////////////////////////////////////////////////////////////////////////////

bool ibApplicationData::AuthenticateUser(const wxString& strUserName,
                                          const wxString& strUserPassword,
                                          ibApplicationDataUserInfo& outInfo)
{
	// Open-access mode — no sys_user rows at all AND caller did not
	// supply a user name. Historical behaviour is "pass through";
	// outInfo stays default-constructed (IsOk() false).
	if (strUserName.IsEmpty() && !HasAllowedUser())
		return true;

	outInfo = ReadUserData(strUserName);
	if (!outInfo.IsOk())
		return false;

	if (!ibPasswordHash::Verify(strUserPassword, outInfo.m_strUserPassword))
		return false;

	// Lazy upgrade: if we just verified a legacy MD5 hash or a PBKDF2 hash
	// with a below-policy iteration count, re-store the password using the
	// current parameters. Silent — if the DB write fails we don't fail the
	// login, just keep the old hash.
	if (ibPasswordHash::NeedsRehash(outInfo.m_strUserPassword)) {
		try {
			outInfo.m_strUserPassword = ibPasswordHash::Hash(strUserPassword);
			(void)SaveUserData(outInfo);
		} catch (...) {
			// ignore — login already succeeded
		}
	}

	return true;
}

void ibApplicationData::InstallUser(const ibApplicationDataUserInfo& info,
                                     const wxString& rawPassword)
{
	// Singleton legacy mirrors. Readers still go through `appData->GetUserInfo()`
	// for most existing code; those will migrate to ibSession-level reads
	// once the session field-extract phase (project_session_registry_refactor
	// step 8) lands.
	m_userInfo = info;
	if (!rawPassword.IsEmpty())
		m_sessionRawPassword = rawPassword;

	// Per-session mirror. The registry thread calls InstallUser under a
	// SessionScope bound to the target ibSession, so Current() resolves
	// to the session we intend to mutate. Desktop StartSession path also
	// runs under the "main" SessionScope set up in Connect().
	if (auto* ctx = ibSession::Current()) {
		ctx->SetUserInfo(info);
		ctx->SetSessionRawPassword(rawPassword);
		ctx->SetSessionGuid(m_sessionGuid);
	}
}

bool ibApplicationData::AuthenticationAndSetUser(const wxString& strUserName, const wxString& strUserPassword)
{
	ibApplicationDataUserInfo info;
	if (!AuthenticateUser(strUserName, strUserPassword, info))
		return false;

	// Open-access path: AuthenticateUser returns true with IsOk()=false.
	// Skip Install — nothing to install, keep singleton state as-is.
	if (!info.IsOk())
		return true;

	InstallUser(info, strUserPassword);
	return true;
}

///////////////////////////////////////////////////////////////////////////////

#include <wx/zipstrm.h>
#include <wx/wfstream.h>
#include <wx/mstream.h>
#include <wx/filename.h>

bool ibApplicationData::LoadDatabase(const wxString& strFullPath)
{
	wxFileInputStream fis(strFullPath);
	if (!fis.IsOk()) {
		wxLogError("Couldn't open the file '%s'.", strFullPath);
		return false;
	}

	if (!ClearDatabase() && !ClearTableUser())
		return false;

	wxZipInputStream zis(fis);
	std::unique_ptr<wxZipEntry> entry;

	// Iterate through all entries in the zip file
	while (entry.reset(zis.GetNextEntry()), entry) {

		if (!entry->IsDir() && entry->GetName() == wxT("config")) {

			// Open the entry for reading and write its data to a new file
			if (zis.OpenEntry(*entry)) {
				wxMemoryOutputStream fos;
				if (fos.IsOk()) {
					zis.Read(fos); // Read from zip stream, write to file stream
				}
				else {
					return false;
				}

				//load configuration 
				wxMemoryBuffer buffer;
				fos.CopyTo(buffer.GetAppendBuf(fos.GetSize()), fos.GetSize());
				buffer.SetDataLen(fos.GetSize());

				if (!activeMetaData->LoadConfigFromBuffer(buffer))
					return false;

				activeMetaData->RunDatabase();
				activeMetaData->SaveDatabase(saveConfigFlag);
			}
		}
		else if (!entry->IsDir() && entry->GetName() == wxT("user")) {

			// Open the entry for reading and write its data to a new file
			if (zis.OpenEntry(*entry)) {
				wxMemoryOutputStream fos;
				if (fos.IsOk()) {
					zis.Read(fos); // Read from zip stream, write to file stream
				}
				else {
					return false;
				}

				//load user 
				wxMemoryBuffer buffer;
				fos.CopyTo(buffer.GetAppendBuf(fos.GetSize()), fos.GetSize());
				buffer.SetDataLen(fos.GetSize());

				if (!LoadUserInfoFromBuffer(buffer))
					return false;
			}
		}
		else if (!entry->IsDir() && entry->GetName() == wxT("data")) {

			// Open the entry for reading and write its data to a new file
			if (zis.OpenEntry(*entry)) {
				wxMemoryOutputStream fos;
				if (fos.IsOk()) {
					zis.Read(fos); // Read from zip stream, write to file stream
				}
				else {
					return false;
				}

				//load data 
				wxMemoryBuffer buffer;
				fos.CopyTo(buffer.GetAppendBuf(fos.GetSize()), fos.GetSize());
				buffer.SetDataLen(fos.GetSize());

				if (!activeMetaData->LoadDataFromBuffer(buffer))
					return false;
			}
		}
	}

	return true;
}

bool ibApplicationData::SaveDatabase(const wxString& strFullPath)
{
	// 1. Create the physical file output stream
	wxFFileOutputStream out(strFullPath);
	if (!out.IsOk())
	{
		wxLogError("Cannot create output file %s", strFullPath);
		return false;
	}

	// 2. Wrap it in a wxZipOutputStream for compression
	wxZipOutputStream zip(out);

	// PutNextEntry sets up the new entry in the archive
	if (zip.PutNextEntry(wxT("config"))) {
		//save configuration 
		wxMemoryBuffer bufferConfig;
		if (activeMetaData->SaveConfigToBuffer(bufferConfig)) {
			// Wrap the content in an input stream to write it easily
			wxMemoryInputStream contentStream(bufferConfig.GetData(), bufferConfig.GetDataLen());
			zip.Write(contentStream); // Write the data from the content stream
		}
	}

	if (zip.PutNextEntry(wxT("user"))) {
		//save users 
		wxMemoryBuffer bufferUser;
		if (!SaveUserInfoToBuffer(bufferUser))
			return false;
		// Wrap the content in an input stream to write it easily
		wxMemoryInputStream contentStream(bufferUser.GetData(), bufferUser.GetDataLen());
		zip.Write(contentStream); // Write the data from the content stream
	}

	if (zip.PutNextEntry(wxT("data"))) {
		//save data
		wxMemoryBuffer bufferData;
		if (!activeMetaData->SaveDataToBuffer(bufferData))
			return false;
		// Wrap the content in an input stream to write it easily
		wxMemoryInputStream contentStream(bufferData.GetData(), bufferData.GetDataLen());
		zip.Write(contentStream); // Write the data from the content stream
	}

	// 3. Close the zip stream (this is essential to finalize the archive)
	// The underlying file stream 'out' will be closed automatically when 'zip' goes out of scope,
	// or when explicitly calling zip.Close().
	return zip.Close();
}

bool ibApplicationData::ClearDatabase()
{
	if (!m_created_metadata)
		return false;

	if (!activeMetaData->ReCreateDatabase())
		return false;

	return true;
}

wxString ibApplicationData::GetDatabaseDescription()
{
	if (m_dbMode == ibDatabaseMode::eFILE)
		return m_strFile;

	if (m_dbMode == ibDatabaseMode::eSERVER)
		return m_strServer + wxT(":") + m_strPort + wxT("/") + m_strDatabase;

	return wxT("");
}

///////////////////////////////////////////////////////////////////////////////

void ibApplicationData::ReadUserData_Password(const wxMemoryBuffer& buffer, ibApplicationDataUserInfo& userInfo) const
{
	ibReaderMemory reader(buffer);

	userInfo.m_strUserGuid = reader.r_stringZ();
	userInfo.m_strUserName = reader.r_stringZ();
	userInfo.m_strUserFullName = reader.r_stringZ();
	userInfo.m_strUserPassword = reader.r_stringZ();
}

void ibApplicationData::ReadUserData_Role(const wxMemoryBuffer& buffer, ibApplicationDataUserInfo& userInfo) const
{
	ibReaderMemory reader(buffer);

	unsigned int count = reader.r_u32();
	userInfo.m_roleArray.reserve(count);
	for (unsigned int idx = 0; idx < count; idx++) {
		ibApplicationDataUserInfo::ibApplicationDataUserRole entry;
		entry.m_strRoleGuid = reader.r_stringZ();
		entry.m_miRoleId = reader.r_s32();
		userInfo.m_roleArray.emplace_back(std::move(entry));
	}
}

void ibApplicationData::ReadUserData_Language(const wxMemoryBuffer& buffer, ibApplicationDataUserInfo& userInfo) const
{
	ibReaderMemory reader(buffer);
	userInfo.m_strLanguageGuid = reader.r_stringZ();
	userInfo.m_strLanguageCode = reader.r_stringZ();
}

///////////////////////////////////////////////////////////////////////////////

wxMemoryBuffer ibApplicationData::SaveUserData_Password(const ibApplicationDataUserInfo& userInfo) const
{
	ibWriterMemory writer;
	writer.w_stringZ(userInfo.m_strUserGuid);
	writer.w_stringZ(userInfo.m_strUserName);
	writer.w_stringZ(userInfo.m_strUserFullName);
	writer.w_stringZ(userInfo.m_strUserPassword);
	return writer.buffer();
}

wxMemoryBuffer ibApplicationData::SaveUserData_Role(const ibApplicationDataUserInfo& userInfo) const
{
	ibWriterMemory writer;
	writer.w_u32(userInfo.m_roleArray.size());
	for (const auto& role : userInfo.m_roleArray) {
		writer.w_stringZ(role.m_strRoleGuid);
		writer.w_s32(role.m_miRoleId);
	}
	return writer.buffer();
}

wxMemoryBuffer ibApplicationData::SaveUserData_Language(const ibApplicationDataUserInfo& userInfo) const
{
	ibWriterMemory writer;
	writer.w_stringZ(userInfo.m_strLanguageGuid);
	writer.w_stringZ(userInfo.m_strLanguageCode);
	return writer.buffer();
}

///////////////////////////////////////////////////////////////////////////////

#include "backend/utils/md5.hpp"

wxString ibApplicationData::ComputeMd5(const wxString& userPassword) const
{
	if (userPassword.Length() > 0)
		return ibMD5::ComputeMd5(userPassword);

	return wxEmptyString;
}

///////////////////////////////////////////////////////////////////////////////