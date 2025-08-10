#ifndef __APP_DATA_H__
#define __APP_DATA_H__

#include "backend/backend_core.h"

#define appData				(CApplicationData::Get())

#define appDataCreateFile(mode,database) (CApplicationData::CreateAppDataEnv(mode,database))
#define appDataCreateServer(mode,server,port,user,password,database) (CApplicationData::CreateAppDataEnv(mode,server,port,user,password,database))

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

	Guid m_sessionArrayHash;

	struct CApplicationDataSessionUnit {

		eRunMode m_runMode;
		wxDateTime m_startedDate;
		wxString m_strUserName, m_strComputerName, m_strSession;

		CApplicationDataSessionUnit(eRunMode runMode, const wxDateTime& startedDateTime,
			const wxString strUserName, const wxString strComputerName, const Guid& strSession) :
			m_runMode(runMode),
			m_startedDate(startedDateTime),
			m_strUserName(strUserName),
			m_strComputerName(strComputerName),
			m_strSession(strSession.str())
		{
		}

		CApplicationDataSessionUnit(const CApplicationDataSessionUnit& src) :
			m_runMode(src.m_runMode),
			m_startedDate(src.m_startedDate),
			m_strUserName(src.m_strUserName),
			m_strComputerName(src.m_strComputerName),
			m_strSession(src.m_strSession)
		{
		}

		bool operator == (const CApplicationDataSessionUnit& other) const { return m_strSession == other.m_strSession && other.m_strUserName == m_strUserName; }
		bool operator != (const CApplicationDataSessionUnit& other) const { return m_strSession != other.m_strSession || other.m_strUserName != m_strUserName; }

		// default comparison to use if nothing else is specified:
		bool operator < (const CApplicationDataSessionUnit& other) const { return m_strSession < other.m_strSession && m_strUserName < other.m_strUserName; }
	};

	std::vector<CApplicationDataSessionUnit> m_listSession;

public:

	bool operator == (const CApplicationDataSessionArray& other) const { return m_listSession == other.m_listSession; }
	bool operator != (const CApplicationDataSessionArray& other) const { return m_listSession != other.m_listSession; }

	CApplicationDataSessionArray() : m_sessionArrayHash(wxNewUniqueGuid) {}
	CApplicationDataSessionArray(const CApplicationDataSessionArray& src) : m_sessionArrayHash(src.m_sessionArrayHash), m_listSession(src.m_listSession) {}

	void AppendSession(eRunMode runMode, const wxDateTime& startedTime,
		const wxString strUserName, const wxString strComputerName, const Guid& strSession) {
		m_listSession.emplace_back(
			runMode,
			startedTime,
			strUserName,
			strComputerName,
			strSession
		);
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
};
#pragma endregion

// This class is a singleton class.
class BACKEND_API CApplicationData {

#pragma region session  
	class CApplicationDataSessionUpdater : public wxThread {
		bool m_sessionCreated, m_sessionStarted;
		CApplicationDataSessionArray m_sessionArray;
		std::shared_ptr<IDatabaseLayer> m_session_db;
		const Guid m_session;
		wxDateTime m_currentDateTime;
		static wxCriticalSection sm_sessionLocker;
	private:
		void Job_ClearLostSession();
		void Job_CalcActiveSession();
		void Job_UpdateActiveSession();
	private:
		void ClearLostSessionUpdater();
		bool VerifySessionUpdater() const;
	public:
		CApplicationDataSessionUpdater(CApplicationData* application, const Guid& session);
		virtual ~CApplicationDataSessionUpdater();
		bool InitSessionUpdater();
		void StartSessionUpdater();
		const CApplicationDataSessionArray GetSessionArray() const;
	protected:
		virtual ExitCode Entry();
	};
#pragma endregion

	CApplicationData(eRunMode runMode);

public:

	virtual ~CApplicationData();
	static CApplicationData* Get() { return s_instance; }

	///////////////////////////////////////////////////////////////////////////
	static bool CreateAppDataEnv();
	///////////////////////////////////////////////////////////////////////////

	static bool CreateAppDataEnv(eRunMode runMode, const wxString& strDirDatabase);
	static bool CreateAppDataEnv(eRunMode runMode, const wxString& strServer, const wxString& strPort,
		const wxString& strUser = _(""), const wxString& strPassword = _(""), const wxString& strDatabase = _(""));

	static bool DestroyAppDataEnv();
	///////////////////////////////////////////////////////////////////////////

	// Initialize application
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
	static void CreateTableEvent();
private:
	static CApplicationData* s_instance;

	eRunMode m_runMode;
	wxString m_strUserIB, m_strPasswordIB;
	wxString m_strComputer;
	wxDateTime m_startedDate;
	wxDateTime m_lastActivity;
	Guid m_sessionGuid;

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

	bool m_exclusiveMode; //Монопольный режим
};

///////////////////////////////////////////////////////////////////////////////
#define user_table		wxT("sys_user")
#define session_table	wxT("sys_session")
#define event_table		wxT("sys_event")
///////////////////////////////////////////////////////////////////////////////

#endif
