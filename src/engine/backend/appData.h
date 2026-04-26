#ifndef __APP_DATA_H__
#define __APP_DATA_H__

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "backend/backend_core.h"

#define appData				(ibApplicationData::Get())

#define appDataCreateFile(mode,database,locale) (ibApplicationData::CreateFileAppDataEnv(mode,database,locale))
#define appDataCreateServer(mode,server,port,user,password,database,locale) (ibApplicationData::CreateServerAppDataEnv(mode,server,port,user,password,database,locale))

#define appDataDestroy()	(ibApplicationData::DestroyAppDataEnv())

#define db_query  (ibApplicationData::GetDatabaseLayer())

enum ibRunMode {
	eLAUNCHER_MODE = 1,		// for create db, only backmode
	eDESIGNER_MODE = 2,		// backmode + frontmode
	eENTERPRISE_MODE = 3,	// backmode + frontmode (thick client)
	eSERVICE_MODE = 4,		// only backmode
	eWEB_ENTERPRISE_MODE = 5	// backmode + wfrontend (web — wes process)
};

enum ibDatabaseMode {
	eFILE,
	eSERVER,

	eNONE = 1000
};

//////////////////////////////////////////////////////////////////
#define _app_start_default_flag 0x0000
#define _app_start_create_debug_server_flag 0x0080
//////////////////////////////////////////////////////////////////

class BACKEND_API ibDatabaseLayer;
class BACKEND_API ibSession;
enum class ibSessionKind : int;   // defined in backend/session/session.h

#pragma region session  
class BACKEND_API ibApplicationDataSessionArray {

	struct ibApplicationDataSessionUnit {

		ibApplicationDataSessionUnit(ibRunMode runMode, int kind, const wxDateTime& startedDateTime,
			const wxString& strUserName, const wxString& strComputerName, const wxString& strSession) :
			m_runMode(runMode), m_kind(kind), m_startedDate(startedDateTime), m_strUserName(strUserName), m_strComputerName(strComputerName), m_strSession(strSession)
		{
		}

		ibRunMode m_runMode;
		// session-layer role, numeric ibSessionKind. Stored as int so the
		// header stays independent of session.h (appData.h is included
		// from backend-wide code that predates session-registry).
		int m_kind;
		wxDateTime m_startedDate;
		wxString m_strUserName;
		wxString m_strComputerName;
		wxString m_strSession;
		// Process-wide exclusive (monopoly) flag — true when this row
		// holds it. Filled in by JobRefreshSnapshot from sys_session.exclusive.
		bool m_exclusive = false;
	};

public:

	ibApplicationDataSessionArray() : m_sessionArrayHash(wxNewUniqueGuid) {}

	void AppendSession(ibRunMode runMode, int kind, const wxDateTime& startedTime,
		const wxString& strUserName, const wxString& strComputerName, const wxString& strSession) {
		m_listSession.emplace_back(runMode, kind, startedTime, strUserName, strComputerName, strSession);
	}

	wxString GetSessionArrayHash() const { return wxString(m_sessionArrayHash.str()); }

	wxString GetUserName(unsigned int idx) const;
	wxString GetComputerName(unsigned int idx) const;
	wxString GetSession(unsigned int idx) const;
	wxString GetStartedDate(unsigned int idx) const;
	wxString GetApplication(unsigned int idx) const;

	void ClearSession() {
		m_sessionArrayHash = wxNewUniqueGuid;
		m_listSession.clear();
	}

	ibRunMode GetSessionApplication(unsigned int idx) const;
	int       GetSessionKind(unsigned int idx) const;
	// Short "Server" / "Client" label derived from (runMode, kind):
	//   Web runtime + kind=WebClient (100) → "Client"
	//   Web runtime + kind=WebServer (5) or legacy 0 → "Server"
	//   Any desktop runtime → "Client"
	// Used by designer's Active Users dialog.
	wxString  GetSessionKindDescr(unsigned int idx) const;
	unsigned int const GetSessionCount() const { return m_listSession.size(); }

	// Back-fill kinds after rows were appended — used by the registry
	// snapshot builder to tolerate legacy schemas where the `kind`
	// column lives in a second optional SELECT.
	void SetKindsFromMap(const std::unordered_map<std::string, int>& kindBySession);

	// Back-fill exclusive flags from a second optional SELECT — same
	// pattern as kinds, tolerant of pre-migration schemas where the
	// column doesn't exist yet.
	void SetExclusiveFromMap(const std::unordered_map<std::string, bool>& exclusiveBySession);

	// Per-row exclusive flag accessor — returns the holder session (the
	// only one with m_exclusive==true), if any. Used by ProcessAdd's
	// cross-process exclusive gate and by ProcessSetExclusive's
	// sole-live check.
	bool        IsExclusive(unsigned int idx) const;
	wxString    ExclusiveHolderSession() const;
	wxString    ExclusiveHolderUser() const;

private:
	ibGuid m_sessionArrayHash;
	std::vector<ibApplicationDataSessionUnit> m_listSession;
};
#pragma endregion

#pragma region config
struct ibApplicationDataConfigInfo {

	bool IsSetLocale() const { return !m_strLocale.IsEmpty(); }

	//Locale info 
	wxString m_strLocale;
};
#pragma endregion 

#pragma region user
struct ibApplicationDataShortUserInfo {
	wxString m_strUserGuid;
	wxString m_strUserName;
	wxString m_strUserFullName;
};

// ibApplicationDataUserInfo moved to backend/userInfo.h so ibSession
// can reach it without pulling all of appData.h. Include kept for source
// compatibility — call sites that already saw the struct through appData.h
// continue to do so.
#include "backend/userInfo.h"
#pragma endregion

// This class is a singleton class.
class BACKEND_API ibApplicationData {

	ibApplicationData(ibRunMode runMode);

public:

	virtual ~ibApplicationData();
	static ibApplicationData* Get() { return s_instance; }


	///////////////////////////////////////////////////////////////////////////
	static bool CreateAppDataEnv(ibRunMode runMode = ibRunMode::eENTERPRISE_MODE);
	///////////////////////////////////////////////////////////////////////////

	static bool CreateFileAppDataEnv(ibRunMode runMode, const wxString& strDirDatabase, const wxString& strLocale = wxT(""));
	static bool CreateServerAppDataEnv(ibRunMode runMode, const wxString& strServer, const wxString& strPort,
		const wxString& strUser = wxT(""), const wxString& strPassword = wxT(""), const wxString& strDatabase = wxT(""), const wxString& strLocale = wxT(""));

	static bool SetLocaleAppDataEnv(const wxString& strLocale = wxT(""));
	static bool DestroyAppDataEnv();
	///////////////////////////////////////////////////////////////////////////

	// Initialize application
	bool InitLocale(const wxString& locale = wxT(""));

	// ------------------------------------------------------------------
	// Phased startup — split of Connect(). Apps compose:
	//
	//   CreateSession()     → ticket visible in sys_session, registry
	//                         policies fire (DesignerExclusive etc.).
	//   Authenticate(u, p)  → ticket.Attach with creds (+ dialog
	//                         fallback on fail).
	//   LoadMetadata(flags) → compile descriptors.
	//   frame->Initialize(session)  → bind + runtime (by session kind).
	//   frame->Show()       → AllowRun fires BeforeStart veto.
	//
	// A failed Authenticate keeps the ticket visible (user sees
	// "login in progress" in admin) until it's explicitly dropped.
	// Retry loops can call Authenticate again without re-creating the
	// session.
	// ------------------------------------------------------------------

	// Phase 1: registry session — anonymous Connect (no creds yet).
	// Row inserted in sys_session immediately so admin / policies see
	// "someone is logging in". DesignerExclusivePolicy (etc.) fire
	// here before any auth. Returns session pointer on success;
	// nullptr if registry Connect failed (policy veto, row-lock dup).
	ibSession* CreateSession();

	// Typed overload of CreateSession. The caller (enterprise/mainApp.cpp,
	// designer/mainApp.cpp, web code) picks the concrete derived session
	// class (ibEnterpriseSession, ibDesignerSession, ibWebClientSession, …).
	// Template bodies live in backend/session/sessionRegistry.h (callers
	// that instantiate the typed overload include it) so the registry's
	// CreateSessionWithFactory is visible at instantiation.
	// Flow:
	//   1. m_sessionRegistry->CreateSessionWithFactory runs
	//      EnsureStartedForCreateSession + Connect(req) under a factory
	//      that builds SessionT instead of the plain base.
	//   2. OnCreateSession() fires on the main (caller) thread for UI-side
	//      setup — GUI sessions create their wx frame here.
	//   3. On OnCreateSession returning false, the session is Close()d so
	//      the registry ticket is dropped and CreateSession returns nullptr.
	template<class SessionT>
	SessionT* CreateSession();

	// Per-tab variant for the wes web frontend. Caller supplies the cookie
	// guid (used as sys_session.session PK + the registry's session id —
	// one identifier across cookie / SessionManager / sys_session row) and
	// the listener address ("host:port", surfaced in admin UI). Kind is
	// fixed at WebClient (per-tab), independent of process run mode.
	// Server() is auto-populated by the registry — it tracks the most
	// recent WebServer-kind session in the process and attaches subsequent
	// non-server sessions to it. Single-session apps never register a
	// WebServer session so their Server() stays null.
	template<class SessionT>
	SessionT* CreateSession(const wxString& presetGuid,
	                        const wxString& address);

private:
	// Register the session-lifecycle event listeners that drive
	// metadata + per-session runtime bring-up/teardown. Called once
	// from the ctor — listeners stay live for the appData's lifetime.
	void WireSessionEvents();
public:

#pragma region config
	// Read setting from file
	void ReadEngineConfig();
#pragma endregion

	// Returns the connection the current thread should use for
	// database work. Resolution order:
	//   1. Thread-local active-TX connection (pool-tracked) — while a
	//      transaction is open, every db_query access on this thread
	//      must land on the same conn.
	//   2. Thread-local current connection set by the innermost live
	//      ibConnectionScope on this thread.
	//   3. Process-wide m_db — the legacy single shared connection
	//      held directly by ibApplicationData. Unchanged fallback
	//      for code that hasn't been migrated to ibConnectionScope.
	//
	// This is what the `db_query` macro expands to. Defined out-of-
	// line in appData.cpp; the TL slots themselves live on
	// ibConnectionPool (see connectionPool.h).
	static std::shared_ptr<ibDatabaseLayer> GetDatabaseLayer();

	// Process-wide connection pool — the sole owner of every live
	// ibDatabaseLayer. Pool holds the master (opened at Init) as
	// m_source and lazily clones up to maxSize on demand. Init/
	// Shutdown are driven by CreateFile/Server AppDataEnv and
	// DestroyAppDataEnv respectively. Borrowed pointer — lifetime
	// tied to ibApplicationData; never null while s_instance is alive.
	static class ibConnectionPool* GetConnectionPool() {
		return s_instance != nullptr ? s_instance->m_connectionPool.get() : nullptr;
	}

#pragma region execute
	// RunApplication overloads spawn any OES bin (enterprise / designer /
	// daemon / codeRunner / wenterprise-server). Connection flags are
	// emitted in unified `--flag=value` form which every bin's parser
	// accepts.
	// useManifest=true switches to wenterprise-server's bind-handshake
	// protocol: --port=0 --manifest=<tempfile>, poll manifest for the
	// real URL and open the default browser. Returns pid.
	long RunApplication(const wxString& strAppName,
		bool searchDebug = true,
		bool useManifest = false) const;
	long RunApplication(const wxString& strAppName,
		const wxString& strUserName,
		const wxString& strUserPassword,
		bool searchDebug = true,
		bool useManifest = false) const;

	// Append `--port=0 --manifest=<tempfile>` to cmd, spawn detached,
	// poll the manifest for up to 5s and open the default browser at the
	// reported URL. Returns pid on spawn-ok, 0 on failure. Exposed for
	// callers that build the cmd from external connection data (e.g.
	// launcher picks from a saved IB list, not the appData singleton).
	static long SpawnWebServerWithManifest(wxString cmd, bool searchDebug = false);
#pragma endregion

	ibRunMode GetAppMode() const { return m_runMode; }

	bool LauncherMode() const { return m_runMode == ibRunMode::eLAUNCHER_MODE; }
	bool DesignerMode() const { return m_runMode == ibRunMode::eDESIGNER_MODE; }
	bool EnterpriseMode() const {
		return m_runMode == ibRunMode::eENTERPRISE_MODE
			|| m_runMode == ibRunMode::eWEB_ENTERPRISE_MODE;
	}
	bool WebEnterpriseMode() const { return m_runMode == ibRunMode::eWEB_ENTERPRISE_MODE; }
	bool ServiceMode() const { return m_runMode == ibRunMode::eSERVICE_MODE; }

	inline wxString GetRunModeDescr(const ibRunMode& mode) const {
		switch (mode)
		{
		case eLAUNCHER_MODE:
			return wxEmptyString;
		case eDESIGNER_MODE:
			return _("Designer");
		case eENTERPRISE_MODE:
			return _("Thick client (GUI)");
		case eWEB_ENTERPRISE_MODE:
			return _("Web server");
		case eSERVICE_MODE:
			return _("Daemon");
		}
		return wxEmptyString;
	}

	inline wxString GetRunModeDescr() const { return GetRunModeDescr(m_runMode); }

	ibDatabaseMode GetDatabaseMode() const { return m_dbMode; }

	inline wxString GetDatabaseModeDescr(const ibDatabaseMode& mode) const {
		switch (mode)
		{
		case eNONE:
			return wxEmptyString;
		case eFILE:
			return _("File");
		case eSERVER:
			return _("Server");
		}
		return wxEmptyString;
	}

	inline wxString GetDatabaseModeDescr() const { return GetDatabaseModeDescr(m_dbMode); }

	// Verify credentials and install the resolved user onto the current
	// ibSessionScope's ibSession. Single entry point used by both the GUI
	// login dialog (main-thread, scope = the desktop session) and the
	// registry's ProcessAttach (registry-thread, scope = the target tab
	// session). Returns true on successful verification — `outInfo` is
	// filled in that case; the install side runs only when outInfo.IsOk()
	// (open-access mode passes verification with empty info — no install).
	// Returns false on bad creds; outInfo is left untouched.
	bool Login(const wxString& strUserName,
	           const wxString& strUserPassword,
	           ibApplicationDataUserInfo& outInfo);

	// Pure credential check used by Login above. Looks up the user, verifies
	// the password (PBKDF2 with silent MD5→PBKDF2 upgrade via NeedsRehash),
	// fills `outInfo` on success, and does NOT mutate any session state.
	// Returns true for open-access mode too (empty sys_user populating +
	// any creds → pass with outInfo.IsOk()==false). Safe to call from the
	// registry thread without a ibSessionScope. Exposed as a building block
	// so registry's ProcessAttach can short-circuit on bad creds before
	// pinning a session scope.
	bool AuthenticateUser(const wxString& strUserName,
	                      const wxString& strUserPassword,
	                      ibApplicationDataUserInfo& outInfo);

	// Commit side of Login. Writes the resolved user onto the current
	// ibSessionScope's ibSession. `rawPassword` is cached for Designer
	// "Start debugging" re-attach — pass the plain-text the caller received;
	// pass empty to skip the raw-password cache (e.g. token-based flows).
	// No-op when no session is scoped — the caller is in a pre-auth path
	// that has no business installing a user. Most callers should go
	// through Login above instead of invoking this directly.
	void InstallUser(const ibApplicationDataUserInfo& info,
	                 const wxString& rawPassword);

	// Process-wide exclusive (monopoly) mode — true when any session
	// currently holds it. Facade over ibSessionRegistry; out-of-line so
	// this header doesn't have to pull sessionRegistry.h.
	bool ExclusiveMode() const;

	// User-identity accessors — read from the current thread's `ibSession`
	// (per-cookie on web, main user session on desktop). Without an active
	// ibSessionScope the readers see an empty/unauthenticated state — used
	// only by pre-auth bootstrap and standalone tools (codeRunner).
	const wxString& GetUserName()     const;
	const wxString& GetUserPassword() const;
	const ibApplicationDataUserInfo& GetUserInfo() const;

	wxString GetComputerName() const { return m_strComputer; }

	wxString GetLocale() const { return m_locale.GetCanonicalName(); }

	std::vector<ibApplicationDataShortUserInfo> GetAllowedUser() const;

#pragma region session

	// Cluster-wide sys_session snapshot, refreshed by the registry
	// background sweep (~3s). Defined out-of-line in appData.cpp so this
	// header doesn't need to pull sessionRegistry.h.
	ibApplicationDataSessionArray GetSessionArray() const;

	class ibPluginManager* GetPluginManager() const { return m_pluginManager.get(); }

#pragma endregion

	const std::vector<ibApplicationDataUserInfo::ibApplicationDataUserRole>& GetUserRoleArray() const;

#pragma region language

	ibGuid   GetUserLanguageGuid() const;
	wxString GetUserLanguageCode() const;

#pragma endregion

	static bool IsForceExit() {
		wxCriticalSectionLocker enter(m_cs_force_exit);
		return m_forceExit;
	}

	// Optional hook called inside ForceExit before the wxApp path. Used by
	// hosts whose main loop is not wxApp's (wenterprise-server runs an
	// httplib listen_after_bind in the main thread, so wxTheApp->Exit()
	// would fire into a queue nobody is draining). wes's main installs a
	// hook that calls svr->stop() — listen_after_bind returns, the rest
	// of main runs wfrontendShutdown which DELETEs sys_session rows and
	// joins workers cleanly. Set ONCE, ideally right after wfrontendInit.
	using ProcessExitHook = void (*)();
	static void SetProcessExitHook(ProcessExitHook hook) { s_processExitHook = hook; }

	static void ForceExit() {
		wxCriticalSectionLocker enter(m_cs_force_exit);
		if (!m_forceExit) {
			if (s_processExitHook != nullptr)
				s_processExitHook();
			else if (wxTheApp != nullptr)
				wxTheApp->CallAfter([]() { wxTheApp->Exit(); });
			m_forceExit = true;
		}
	}

#pragma region user  
	ibApplicationDataUserInfo ReadUserData(const ibGuid& userGuid) const;
	ibApplicationDataUserInfo ReadUserData(const wxString& userName) const;
	bool SaveUserData(const ibApplicationDataUserInfo& userInfo) const;
#pragma endregion

#pragma region database

	bool LoadDatabase(const wxString& strFullPath);
	bool SaveDatabase(const wxString& strFullPath);

	bool ClearDatabase();

#pragma endregion 

	wxString GetDatabaseDescription();

private:

	bool HasAllowedUser() const;

	void ReadUserData_Password(const wxMemoryBuffer& buffer, ibApplicationDataUserInfo& userInfo) const;
	void ReadUserData_Role(const wxMemoryBuffer& buffer, ibApplicationDataUserInfo& userInfo) const;
	void ReadUserData_Language(const wxMemoryBuffer& buffer, ibApplicationDataUserInfo& userInfo) const;

	wxMemoryBuffer SaveUserData_Password(const ibApplicationDataUserInfo& userInfo) const;
	wxMemoryBuffer SaveUserData_Role(const ibApplicationDataUserInfo& userInfo) const;
	wxMemoryBuffer SaveUserData_Language(const ibApplicationDataUserInfo& userInfo) const;

	bool LoadUserInfoFromBuffer(wxMemoryBuffer& buffer);
	bool SaveUserInfoToBuffer(wxMemoryBuffer& buffer) const;

	wxString ComputeMd5() const;
	wxString ComputeMd5(const wxString& userPassword) const;

	static bool TableAlreadyCreated();
	static void CreateTableUser();
	static void CreateTableSession();
	static void CreateTableEvent();
	static void MigrateTableSession();

	static bool ClearTableUser();

private:

	static ibApplicationData* s_instance;

	ibRunMode m_runMode;
	wxString m_strComputer;

#pragma region config
	ibApplicationDataConfigInfo m_configInfo;
#pragma endregion

	static bool m_forceExit;
	static wxCriticalSection m_cs_force_exit;
	static ProcessExitHook s_processExitHook;

	std::unique_ptr<class ibPluginManager> m_pluginManager;
	// Connection pool — the sole owner of every ibDatabaseLayer in
	// the process. Master (opened at Init) plus lazy clones up to
	// maxSize. `db_query` / GetDatabaseLayer resolve through the
	// pool; ibApplicationData keeps no direct DB handle of its own.
	std::unique_ptr<class ibConnectionPool> m_connectionPool;

	// Session manager (registry). Owned here — created in ctor, destroyed
	// in dtor. Process-wide singleton in practice (appData itself is one),
	// accessed via ibSessionRegistry::Instance() facade or directly via
	// GetSessionRegistry().
	std::unique_ptr<class ibSessionRegistry> m_sessionRegistry;

public:
	class ibSessionRegistry* GetSessionRegistry() const { return m_sessionRegistry.get(); }

private:

	bool m_connected_to_db = false;
	bool m_created_metadata = false;
	bool m_run_metadata = false;

public:
	// Flags stashed before Authenticate fires the OnFirstConnect
	// listener — listener can't take a flags argument (callback shape is
	// fixed) so the flags ride here instead. Apps that need non-default
	// flags (Enterprise's debug-server flag) write directly before
	// CreateSession + Authenticate.
	int  m_loadMetadataFlags = _app_start_default_flag;
private:

	ibDatabaseMode m_dbMode;

	// FILE ENTRY
	wxString m_strFile;

	// SERVER ENTRY
	wxString m_strServer;
	wxString m_strPort;
	wxString m_strDatabase;

	wxString m_strUser;
	wxString m_strPassword;

	// LOCALE
	wxLocale m_locale;
	int m_locale_lang;
};

///////////////////////////////////////////////////////////////////////////////
#define user_table		wxT("sys_user")
#define session_table	wxT("sys_session")
#define sequence_table	wxT("sys_sequence")
#define event_table		wxT("sys_event")
///////////////////////////////////////////////////////////////////////////////

#endif
