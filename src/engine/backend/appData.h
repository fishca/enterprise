#ifndef __APP_DATA_H__
#define __APP_DATA_H__

#include "backend/backend_core.h"

#define appData				(CApplicationData::Get())

#define appDataCreateFile(mode,database,locale) (CApplicationData::CreateFileAppDataEnv(mode,database,locale))
#define appDataCreateServer(mode,server,port,user,password,database,locale) (CApplicationData::CreateServerAppDataEnv(mode,server,port,user,password,database,locale))

#define appDataDestroy()	(CApplicationData::DestroyAppDataEnv())

#define db_query  (CApplicationData::GetDatabaseLayer())

enum eRunMode {
	eLAUNCHER_MODE = 1,		// for create db, only backmode  
	eDESIGNER_MODE = 2,		// backmode + frontmode
	eENTERPRISE_MODE = 3,	// backmode + frontmode
	eSERVICE_MODE = 4		// only backmode 
};

//////////////////////////////////////////////////////////////////
#define _app_start_default_flag 0x0000
#define _app_start_create_debug_server_flag 0x0080
//////////////////////////////////////////////////////////////////

class BACKEND_API IDatabaseLayer;

#pragma region session  
class BACKEND_API CApplicationDataSessionArray {

	struct CApplicationDataSessionUnit {

		CApplicationDataSessionUnit(eRunMode runMode, const wxDateTime& startedDateTime,
			const wxString strUserName, const wxString strComputerName, const wxString& strSession) :
			m_runMode(runMode), m_startedDate(startedDateTime), m_strUserName(strUserName), m_strComputerName(strComputerName), m_strSession(strSession)
		{
		}

		eRunMode m_runMode;
		wxDateTime m_startedDate;
		wxString m_strUserName;
		wxString m_strComputerName;
		wxString m_strSession;
	};

public:

	CApplicationDataSessionArray() : m_sessionArrayHash(wxNewUniqueGuid) {}

	void AppendSession(eRunMode runMode, const wxDateTime& startedTime,
		const wxString strUserName, const wxString strComputerName, const wxString& strSession) {
		m_listSession.emplace_back(runMode, startedTime, strUserName, strComputerName, strSession);
	}

	wxString GetSessionArrayHash() const { return m_sessionArrayHash; }

	wxString GetUserName(unsigned int idx) const;
	wxString GetComputerName(unsigned int idx) const;
	wxString GetSession(unsigned int idx) const;
	wxString GetStartedDate(unsigned int idx) const;
	wxString GetApplication(unsigned int idx) const;

	void ClearSession() {
		m_sessionArrayHash = wxNewUniqueGuid;
		m_listSession.clear();
	}

	eRunMode GetSessionApplication(unsigned int idx) const;
	unsigned int const GetSessionCount() const { return m_listSession.size(); }

private:
	CGuid m_sessionArrayHash;
	std::vector<CApplicationDataSessionUnit> m_listSession;
};
#pragma endregion

// This class is a singleton class.
class BACKEND_API CApplicationData {

#pragma region session  
	class CApplicationDataSessionUpdater : public wxThread {
	public:
		CApplicationDataSessionUpdater(CApplicationData* application, const CGuid& session);
		virtual ~CApplicationDataSessionUpdater();
		bool InitSessionUpdater();
		void StartSessionUpdater();
		const CApplicationDataSessionArray GetSessionArray() const;
	protected:

		virtual ExitCode Entry();

		// This one is called by Delete() before actually deleting the thread and
		// is executed in the context of the thread that called Delete().
		virtual void OnDelete() { m_sessionUpdaterLoop = false; }

		// This one is called by Kill() before killing the thread and is executed
		// in the context of the thread that called Kill().
		virtual void OnKill() { m_sessionUpdaterLoop = false; }

		// called when the thread exits - in the context of this thread
		//
		// NB: this function will not be called if the thread is Kill()ed
		virtual void OnExit() { m_sessionUpdaterLoop = false; }

	private:

		void Job_ClearLostSession();
		void Job_CalcActiveSession();
		void Job_UpdateActiveSession();

		void ClearLostSessionUpdater();
		bool VerifySessionUpdater() const;

		bool m_sessionCreated, m_sessionStarted;
		bool m_sessionUpdaterLoop;

		CApplicationDataSessionArray m_sessionArray;
		std::shared_ptr<IDatabaseLayer> m_session_db;
		const CGuid m_session;
		wxDateTime m_currentDateTime;
		static wxCriticalSection ms_sessionLocker;
	};
#pragma endregion

	CApplicationData(eRunMode runMode);

public:

	virtual ~CApplicationData();
	static CApplicationData* Get() { return s_instance; }

	///////////////////////////////////////////////////////////////////////////
	static bool CreateAppDataEnv();
	///////////////////////////////////////////////////////////////////////////

	static bool CreateFileAppDataEnv(eRunMode runMode, const wxString& strDirDatabase, const wxString& strLocale = wxT(""));
	static bool CreateServerAppDataEnv(eRunMode runMode, const wxString& strServer, const wxString& strPort,
		const wxString& strUser = wxT(""), const wxString& strPassword = wxT(""), const wxString& strDatabase = wxT(""), const wxString& strLocale = wxT(""));

	static bool SetLocaleAppDataEnv(const wxString& strLocale = wxT(""));
	static bool DestroyAppDataEnv();
	///////////////////////////////////////////////////////////////////////////

	// Initialize application
	bool InitLocale(const wxString& locale = wxT(""));

	bool Connect(const wxString& user, const wxString& password, const int flags = _app_start_default_flag);
	bool Disconnect();

	static std::shared_ptr<IDatabaseLayer> GetDatabaseLayer() {
		if (s_instance != nullptr)
			return s_instance->m_db;
		return nullptr;
	}

#pragma region execute 
	long RunApplication(const wxString& strAppName, bool searchDebug = true) const;
	long RunApplication(const wxString& strAppName, const wxString& user, const wxString& password, bool searchDebug = true) const;
#pragma endregion

	eRunMode GetAppMode() const { return m_runMode; }

	bool DesignerMode() const { return m_runMode == eRunMode::eDESIGNER_MODE; }
	bool EnterpriseMode() const { return m_runMode == eRunMode::eENTERPRISE_MODE; }
	bool ServiceMode() const { return m_runMode == eRunMode::eSERVICE_MODE; }

	inline wxString GetModeDescr(const eRunMode& mode) const {
		switch (mode)
		{
		case eDESIGNER_MODE:
			return _("Designer");
		case eENTERPRISE_MODE:
			return _("Thick client");
		case eSERVICE_MODE:
			return _("Deamon");
		}
		return wxEmptyString;
	}

	bool AuthenticationAndSetUser(const wxString& userName, const wxString& md5Password);

	bool ExclusiveMode() const { return m_exclusiveMode; }
	const wxString& GetUserName() const { return m_strUserIB; }
	const wxString& GetUserPassword() const { return m_strPasswordIB; }
	wxString GetComputerName() const { return m_strComputer; }
	wxArrayString GetAllowedUser() const;

#pragma region session  
	const CApplicationDataSessionArray GetSessionArray() const {
		return m_sessionUpdater->GetSessionArray();
	}
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

private:
	bool HasAllowedUser() const;
	bool AuthenticationUser(const wxString& userName, const wxString& md5Password) const;
	bool StartSession(const wxString& userName, const wxString& md5Password);
	bool CloseSession();
private:
	wxString ComputeMd5() const { return ComputeMd5(m_strPasswordIB); }
	wxString ComputeMd5(const wxString& userPassword) const;
private:
	static bool TableAlreadyCreated();
	static void CreateTableUser();
	static void CreateTableSession();
	static void CreateTableSequence();
	static void CreateTableEvent();
private:

	static CApplicationData* s_instance;

	eRunMode m_runMode;
	wxString m_strUserIB, m_strPasswordIB;
	wxString m_strComputer;
	wxDateTime m_startedDate;
	wxDateTime m_lastActivity;
	CGuid m_sessionGuid;

	static bool m_forceExit;
	static wxCriticalSection m_cs_force_exit;

	std::shared_ptr<IDatabaseLayer> m_db;
	std::shared_ptr<CApplicationDataSessionUpdater> m_sessionUpdater;

	bool m_connected_to_db = false;
	bool m_created_metadata = false;
	bool m_run_metadata = false;

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
