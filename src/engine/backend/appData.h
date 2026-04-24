#ifndef __APP_DATA_H__
#define __APP_DATA_H__

#include <atomic>
#include <memory>
#include <string>
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

	// Session this process runs under (desktop: the single logged-in
	// user session; wes: the WebServer technical session). Forwards to
	// m_mainTicket's session — nullptr before StartSession completes
	// and after CloseSession. Per-cookie WebClient sessions (web) are
	// NOT this — they live on ibWebSession and are reached through the
	// corresponding ibWebFrame.
	class ibSession* GetMainSession() const;

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

	bool Connect(const wxString& strUserName, const wxString& strUserPassword, const int flags = _app_start_default_flag);
	bool Disconnect();

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
	static long SpawnWebServerWithManifest(wxString cmd);
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

	// Legacy wrapper — verify creds AND install the resolved user info on
	// the singleton + current session. Kept for existing callers
	// (StartSession desktop path, ibWebSession::Login). Prefer the split
	// below in new code: AuthenticateUser() is the pure verifier that
	// ProcessAttach in ibSessionRegistry calls without touching singleton
	// state, then the registry's handler drives installation into the
	// target ibSession directly.
	bool AuthenticationAndSetUser(const wxString& strUserName, const wxString& strUserPassword);

	// Pure credential check. Looks up the user, verifies the password
	// (PBKDF2 with silent MD5→PBKDF2 upgrade via NeedsRehash), fills
	// `outInfo` on success, and does NOT mutate singleton / current
	// session state. Returns true for open-access mode too (empty
	// sys_user populating + any creds → pass). Safe to call from the
	// registry thread without a SessionScope.
	bool AuthenticateUser(const wxString& strUserName,
	                      const wxString& strUserPassword,
	                      ibApplicationDataUserInfo& outInfo);

	// Commit side: install a previously-verified UserInfo onto the
	// legacy singleton slots (m_userInfo, m_sessionRawPassword) AND the
	// current SessionScope's ibSession (dual-write while call sites
	// migrate). `rawPassword` is cached for Designer "Start debugging"
	// re-attach — pass the plain-text the caller received; pass empty
	// to skip the raw-password cache (e.g. token-based flows).
	void InstallUser(const ibApplicationDataUserInfo& info,
	                 const wxString& rawPassword);

	bool ExclusiveMode() const { return m_exclusiveMode; }

	// User-identity accessors — session-aware. Readers see the user info
	// of the current thread's `ibSession` when a SessionScope is active
	// (per-cookie on web, main user session on desktop); they fall back
	// to the process singleton `m_userInfo` only when no session is
	// scoped (pre-auth bootstrap, standalone codeRunner). This closes
	// the multi-tab "last login wins" bug described in
	// project_web_session_bug.md.
	const wxString& GetUserName()     const;
	const wxString& GetUserPassword() const;
	const ibApplicationDataUserInfo& GetUserInfo() const;

	wxString GetComputerName() const { return m_strComputer; }
	wxDateTime GetStartedDate() const { return m_startedDate; }

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

	static void ForceExit() {
		wxCriticalSectionLocker enter(m_cs_force_exit);
		if (!m_forceExit) {
			wxTheApp->CallAfter(
				[]() {
					wxTheApp->Exit();
				}
			);
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
	bool StartSession(const wxString& userName, const wxString& md5Password);
	bool CloseSession();

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
	wxDateTime m_startedDate;
	wxDateTime m_lastActivity;
	ibGuid m_sessionGuid;

#pragma region config
	ibApplicationDataConfigInfo m_configInfo;
#pragma endregion 
#pragma region user
	ibApplicationDataUserInfo m_userInfo;
	// Raw password kept in memory only for the lifetime of the session. Needed so
	// child processes (debug-target enterprise.exe launched from designer.exe)
	// can be handed the original password rather than the stored hash — passing
	// the hash lets the child "log in" with equivalent credentials, which is a
	// privilege-escalation sink. Cleared on CloseSession.
	wxString m_sessionRawPassword;
#pragma endregion

	static bool m_forceExit;
	static wxCriticalSection m_cs_force_exit;

	std::unique_ptr<class ibPluginManager> m_pluginManager;
	// Connection pool — the sole owner of every ibDatabaseLayer in
	// the process. Master (opened at Init) plus lazy clones up to
	// maxSize. `db_query` / GetDatabaseLayer resolve through the
	// pool; ibApplicationData keeps no direct DB handle of its own.
	std::unique_ptr<class ibConnectionPool> m_connectionPool;

	// Main-thread SessionScope — bound to the single user session on
	// the desktop runtime. Alive between Connect()/Disconnect() so all
	// script execution on the main thread sees ibSession::Current()
	// rather than falling through to the legacy descriptor-owned
	// ProcUnit. Web processes don't use this field (per-request scopes
	// are set inside worker lambdas).
	std::unique_ptr<class SessionScope> m_mainThreadScope;

	// Main-thread session ticket from ibSessionRegistry::Connect(). Owns
	// the ibSession lifetime; dtor submits Remove@Urgent so the DELETE
	// of the sys_session row runs on the registry thread. Holds the
	// ibSessionTicket forward-declared type; defined out-of-line in
	// appData.cpp where sessionTicket.h is fully visible.
	std::unique_ptr<class ibSessionTicket> m_mainTicket;

	bool m_connected_to_db = false;
	bool m_created_metadata = false;
	bool m_run_metadata = false;

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

	bool m_exclusiveMode = false; //Exclusive mode
};

///////////////////////////////////////////////////////////////////////////////
#define user_table		wxT("sys_user")
#define session_table	wxT("sys_session")
#define sequence_table	wxT("sys_sequence")
#define event_table		wxT("sys_event")
///////////////////////////////////////////////////////////////////////////////

#endif
