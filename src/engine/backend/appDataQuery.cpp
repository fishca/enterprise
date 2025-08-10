#include "appData.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/databaseLayer/databaseErrorCodes.h"

#include "backend/backend_mainFrame.h"
#include "backend/backend_exception.h"

///////////////////////////////////////////////////////////////////////////////
#define timeInterval 5
///////////////////////////////////////////////////////////////////////////////

wxCriticalSection CApplicationData::CApplicationDataSessionUpdater::sm_sessionLocker;

///////////////////////////////////////////////////////////////////////////////
//								CApplicationDataSessionUpdater
///////////////////////////////////////////////////////////////////////////////

void CApplicationData::CApplicationDataSessionUpdater::Job_ClearLostSession()
{
	m_session_db->BeginTransaction();

	wxDateTime prevDateTime = m_currentDateTime.GetValue();
	(void)prevDateTime.Subtract(wxTimeSpan(0, 0, timeInterval));

	const wxString& strLastActive = prevDateTime.FormatISOCombined(' ');
	IDatabaseResultSet* dbResultSetRecord = m_session_db->RunQueryWithResults(
		//wxT("SELECT lastActive FROM %s WHERE lastActive < '%s' FOR UPDATE WITH LOCK SKIP LOCKED;"),
		wxT("SELECT lastActive FROM %s WHERE lastActive < '%s' FOR UPDATE;"),
		session_table,
		strLastActive
	);

	//clear 
	m_session_db->RunQuery(
		wxT("DELETE FROM %s WHERE lastActive < '%s';"),
		session_table,
		strLastActive
	);

	m_session_db->Commit();
	m_session_db->CloseResultSet(dbResultSetRecord);
}

void CApplicationData::CApplicationDataSessionUpdater::Job_CalcActiveSession()
{
	m_session_db->BeginTransaction();

	auto dbSessionCountResult = m_session_db->RunQueryWithResults(
		//wxT("SELECT session, userName, lastActive FROM %s WHERE session = '%s' FOR UPDATE WITH LOCK SKIP LOCKED;"),
		wxT("SELECT session, userName, lastActive FROM %s WHERE session = '%s' FOR UPDATE;"),
		session_table,
		m_session.str()
	);

	const wxDateTime& currentTime = wxDateTime::Now();

	if (dbSessionCountResult != nullptr &&
		dbSessionCountResult->Next()) {

		m_session_db->RunQuery(
			wxT("UPDATE %s SET lastActive = '%s', userName = '%s' WHERE session = '%s';"),
			session_table,
			currentTime.FormatISOCombined(' '),
			appData->m_strUserIB,
			m_session.str()
		);
	}
	else {

		//start lost session 
		IPreparedStatement* dbSessionPrepareData = m_session_db->PrepareStatement(
			wxT("INSERT INTO %s (session, userName, application, started, lastActive, computer) VALUES (?,?,?,?,?,?);"),
			session_table
		);

		dbSessionPrepareData->SetParamString(1, m_session.str());
		dbSessionPrepareData->SetParamString(2, appData->m_strUserIB); //empty user!
		dbSessionPrepareData->SetParamInt(3, appData->m_runMode);
		dbSessionPrepareData->SetParamDate(4, appData->m_startedDate);
		dbSessionPrepareData->SetParamDate(5, currentTime);
		dbSessionPrepareData->SetParamString(6, appData->m_strComputer);

		dbSessionPrepareData->RunQuery();

		m_session_db->CloseStatement(dbSessionPrepareData);
	}

	m_session_db->CloseResultSet(dbSessionCountResult);
	m_session_db->Commit();
}

void CApplicationData::CApplicationDataSessionUpdater::Job_UpdateActiveSession()
{
	//append sessions 
	IDatabaseResultSet* dbSessionResult = m_session_db->RunQueryWithResults(
		//wxT("SELECT userName, application, started, computer, session FROM %s ORDER BY started, session WITH LOCK SKIP LOCKED;"),
		wxT("SELECT userName, application, started, computer, session FROM %s ORDER BY started, session;"),
		session_table
	);

	wxCriticalSectionLocker enter(sm_sessionLocker);

	if (dbSessionResult != nullptr) {

		CApplicationDataSessionArray sessionArray;

		while (dbSessionResult->Next()) {

			sessionArray.AppendSession(
				static_cast<eRunMode>(dbSessionResult->GetResultInt("application")),
				dbSessionResult->GetResultDate("started"),
				dbSessionResult->GetResultString("userName"),
				dbSessionResult->GetResultString("computer"),
				dbSessionResult->GetResultString("session")
			);
		};

		if (m_sessionArray != sessionArray)
			m_sessionArray = sessionArray;
	}

	m_session_db->CloseResultSet(dbSessionResult);
}

void CApplicationData::CApplicationDataSessionUpdater::ClearLostSessionUpdater()
{
	wxCriticalSectionLocker enter(sm_sessionLocker);

	m_session_db->BeginTransaction();

	const wxDateTime& currentTime = wxDateTime::Now();
	IDatabaseResultSet* dbResultSetRecord = m_session_db->RunQueryWithResults(
		//wxT("SELECT lastActive FROM %s WHERE lastActive < '%s' FOR UPDATE WITH LOCK SKIP LOCKED;"),
		wxT("SELECT lastActive FROM %s WHERE lastActive < '%s' FOR UPDATE;"),
		session_table,
		currentTime.Subtract(wxTimeSpan(0, 0, timeInterval)).FormatISOCombined(' ')
	);

	//clear 
	m_session_db->RunQuery(
		wxT("DELETE FROM %s WHERE lastActive < '%s';"),
		session_table,
		currentTime.Subtract(wxTimeSpan(0, 0, timeInterval)).FormatISOCombined(' ')
	);

	m_session_db->CloseResultSet(dbResultSetRecord);
	m_session_db->Commit();

	//clear session 
	m_sessionArray.ClearSession();

	//append sessions 
	IDatabaseResultSet* dbSessionResult = m_session_db->RunQueryWithResults(
		//wxT("SELECT userName, application, started, computer, session FROM %s ORDER BY started, session WITH LOCK SKIP LOCKED;"),
		wxT("SELECT userName, application, started, computer, session FROM %s ORDER BY started, session;"),
		session_table
	);

	if (dbSessionResult != nullptr) {

		while (dbSessionResult->Next()) {

			m_sessionArray.AppendSession(
				static_cast<eRunMode>(dbSessionResult->GetResultInt("application")),
				dbSessionResult->GetResultDate("started"),
				dbSessionResult->GetResultString("userName"),
				dbSessionResult->GetResultString("computer"),
				dbSessionResult->GetResultString("session")
			);
		};
	}

	m_session_db->CloseResultSet(dbSessionResult);
}

///////////////////////////////////////////////////////////////////////////////

CApplicationData::CApplicationDataSessionUpdater::CApplicationDataSessionUpdater(CApplicationData* application, const Guid& session) :
	wxThread(wxTHREAD_JOINABLE),
	m_session_db(application->m_db != nullptr ? application->m_db->Clone() : nullptr),
	m_session(session),
	m_sessionCreated(false), m_sessionStarted(false)
{
	wxThread::SetPriority(wxPRIORITY_MIN);
}

bool CApplicationData::CApplicationDataSessionUpdater::InitSessionUpdater()
{
	if (m_session_db == nullptr)
		return false;

	if (wxThread::Run() != wxThreadError::wxTHREAD_NO_ERROR)
		return false;

	while (!m_sessionCreated)
	{
		if (!wxThread::IsRunning())
			break;

		wxMilliSleep(50);
	}

	return m_sessionCreated;
}

void CApplicationData::CApplicationDataSessionUpdater::StartSessionUpdater()
{
	m_sessionStarted = true;
}

const CApplicationDataSessionArray CApplicationData::CApplicationDataSessionUpdater::GetSessionArray() const
{
	wxCriticalSectionLocker enter(sm_sessionLocker);
	return m_sessionArray;
}

bool CApplicationData::CApplicationDataSessionUpdater::VerifySessionUpdater() const
{
	for (unsigned int idx = 0; idx < m_sessionArray.GetSessionCount(); idx++) {

		if (appData->m_runMode == m_sessionArray.GetSessionApplication(idx) &&
			appData->m_runMode == eDESIGNER_MODE) {

			try {
				CBackendException::Error(
					_("Another designer process is already running:\n%s, %s, %s"),		
					m_sessionArray.GetStartedDate(idx),
					m_sessionArray.GetComputerName(idx),
					m_sessionArray.GetUserName(idx)
				);
			}
			catch (...) {
			}

			return false;
		}
	}

	return true;
}

CApplicationData::CApplicationDataSessionUpdater::~CApplicationDataSessionUpdater()
{
	if (m_session_db != nullptr) m_session_db->Close();
}

///////////////////////////////////////////////////////////////////////////////

wxThread::ExitCode CApplicationData::CApplicationDataSessionUpdater::Entry()
{
	//ñlear lost session 
	ClearLostSessionUpdater();

	Job_UpdateActiveSession();

	//verify session updater
	if (!VerifySessionUpdater())
		return (wxThread::ExitCode)1;

	//start empty session 
	IPreparedStatement* dbSessionPrepareData = m_session_db->PrepareStatement(
		wxT("INSERT INTO %s (session, userName, application, started, lastActive, computer) VALUES (?,?,?,?,?,?);"),
		session_table
	);

	if (dbSessionPrepareData == nullptr) return (wxThread::ExitCode)1;

	dbSessionPrepareData->SetParamString(1, m_session.str());
	dbSessionPrepareData->SetParamString(2, wxEmptyString); //empty user!
	dbSessionPrepareData->SetParamInt(3, appData->m_runMode);
	dbSessionPrepareData->SetParamDate(4, appData->m_startedDate);
	dbSessionPrepareData->SetParamDate(5, wxDateTime::Now());
	dbSessionPrepareData->SetParamString(6, appData->m_strComputer);

	dbSessionPrepareData->RunQuery();
	dbSessionPrepareData->Close();

	m_sessionArray.AppendSession(
		appData->m_runMode,
		appData->m_startedDate,
		appData->m_strUserIB,
		appData->m_strComputer,
		m_session
	);

	m_session_db->CloseStatement(dbSessionPrepareData);

	m_sessionCreated = true;

	bool lastSessionStarted = m_sessionStarted;

	while (!TestDestroy()) {

		m_currentDateTime = wxDateTime::UNow();

		{
			Job_ClearLostSession();
		}

		const wxTimeSpan& job_ClearLostSession_done = wxDateTime::UNow() - m_currentDateTime;

		{
			Job_CalcActiveSession();
		}

		const wxTimeSpan& job_CalcActiveSession_done = wxDateTime::UNow() - m_currentDateTime -
			job_ClearLostSession_done;

		{
			Job_UpdateActiveSession();
		}

		const wxTimeSpan& job_UpdateActiveSession_done = wxDateTime::UNow() - m_currentDateTime -
			job_ClearLostSession_done - job_CalcActiveSession_done;

		const wxTimeSpan& job_allTotal_done =
			wxDateTime::UNow() - m_currentDateTime;

		for (unsigned int milliseconds =
			job_allTotal_done.GetMilliseconds().GetValue();
			milliseconds < 3000;
			milliseconds += 250)
		{
			if (m_sessionStarted != lastSessionStarted)
				break;

			if (TestDestroy())
				break;

			wxMilliSleep(250);
		}

		lastSessionStarted = m_sessionStarted;
	};

	const int result = m_session_db->RunQuery(
		wxT("DELETE FROM %s WHERE session = '%s';"),
		session_table,
		m_session.str()
	);

	m_sessionArray.ClearSession();
	m_sessionCreated = m_sessionStarted = false;

	if (result != DATABASE_LAYER_QUERY_RESULT_ERROR)
		return (wxThread::ExitCode)1;

	return (wxThread::ExitCode)0;
}

///////////////////////////////////////////////////////////////////////////////
//								CApplicationData
///////////////////////////////////////////////////////////////////////////////

bool CApplicationData::TableAlreadyCreated()
{
	return db_query->TableExists(user_table) &&
		db_query->TableExists(session_table);
}

///////////////////////////////////////////////////////////////////////////////

void CApplicationData::CreateTableUser()
{
	if (!db_query->TableExists(user_table)) {
		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL) {
			db_query->RunQuery("create table %s ("
				"guid              VARCHAR(36)   NOT NULL PRIMARY KEY,"
				"name              VARCHAR(64)  NOT NULL,"
				"fullName          VARCHAR(128)  NOT NULL,"
				"changed		   TIMESTAMP  NOT NULL,"
				"dataSize          INTEGER       NOT NULL,"
				"binaryData        BYTEA      NOT NULL);", user_table);
		}
		else {
			db_query->RunQuery("create table %s ("
				"guid              VARCHAR(36)   NOT NULL PRIMARY KEY,"
				"name              VARCHAR(64)  NOT NULL,"
				"fullName          VARCHAR(128)  NOT NULL,"
				"changed		   TIMESTAMP  NOT NULL,"
				"dataSize          INTEGER       NOT NULL,"
				"binaryData        BLOB      NOT NULL);", user_table);
		}
		db_query->RunQuery("create index if not exists user_index on %s (guid, name);", user_table);
	}
}

void CApplicationData::CreateTableSession()
{
	if (!db_query->TableExists(session_table)) {

		db_query->RunQuery(wxT("CREATE TABLE %s ("
			"session              VARCHAR(36) NOT NULL PRIMARY KEY,"
			"userName             VARCHAR(64) NOT NULL,"
			"application	   INTEGER  NOT NULL,"
			"started		   TIMESTAMP  NOT NULL,"
			"lastActive		   TIMESTAMP  NOT NULL,"
			"computer          VARCHAR(128) NOT NULL);"),
			session_table
		);

		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL) {

			db_query->RunQuery(
				wxT("create index if not exists session_index_1 on %s (session, userName);"),
				session_table
			);

			db_query->RunQuery(
				wxT("create index if not exists session_index_2 on %s (session);"),
				session_table
			);

			db_query->RunQuery(
				wxT("create index if not exists session_index_3 on %s (lastActive);"),
				session_table
			);
		}
		else
		{
			db_query->RunQuery(
				wxT("create index session_index_1 on %s (session, userName);"),
				session_table
			);

			db_query->RunQuery(
				wxT("create index session_index_2 on %s (session);"),
				session_table
			);

			db_query->RunQuery(
				wxT("create index session_index_3 on %s (lastActive);"),
				session_table
			);
		}
	}
}

void CApplicationData::CreateTableEvent()
{
	if (!db_query->TableExists(event_table)) {
	}
}

///////////////////////////////////////////////////////////////////////////////
bool CApplicationData::HasAllowedUser() const
{
	IDatabaseResultSet* dbUserResult = db_query->RunQueryWithResults(
		wxT("SELECT name FROM %s;"),
		user_table
	);
	if (dbUserResult == nullptr) return false;
	bool hasUsers = false;
	if (dbUserResult->Next()) hasUsers = true;
	db_query->CloseResultSet(dbUserResult);
	return hasUsers;
}

#include "fileSystem/fs.h"

bool CApplicationData::AuthenticationUser(const wxString& userName, const wxString& userPassword) const
{
	if (!HasAllowedUser()) return true;
	
	IDatabaseResultSet* dbUserResult = db_query->RunQueryWithResults(
		wxT("SELECT * FROM %s WHERE name = '%s';"),
		user_table, userName
	);
	
	if (dbUserResult == nullptr) return false;

	bool succes = false; 

	if (dbUserResult->Next()) {
		wxString md5Password; wxMemoryBuffer buffer;
		dbUserResult->GetResultBlob(wxT("binaryData"), buffer);
		CMemoryReader reader(buffer.GetData(), buffer.GetDataLen());
		reader.r_stringZ(md5Password);
		if (md5Password == CApplicationData::ComputeMd5(userPassword)) succes = true; 
	}
	
	db_query->CloseResultSet(dbUserResult);	
	return succes;
}

bool CApplicationData::AuthenticationAndSetUser(const wxString& userName, const wxString& userPassword)
{
	if (AuthenticationUser(userName, userPassword)) {
		m_strUserIB = userName;
		m_strPasswordIB = userPassword;
		return true;
	}
	return false;
}

wxArrayString CApplicationData::GetAllowedUser() const
{
	IDatabaseResultSet* dbUserResult =
		db_query->RunQueryWithResults(
			wxT("SELECT name FROM %s;"),
			user_table
		);
	if (dbUserResult == nullptr) return wxArrayString();
	wxArrayString arrayUsers;
	while (dbUserResult->Next()) arrayUsers.Add(dbUserResult->GetResultString(wxT("name")));
	db_query->CloseResultSet(dbUserResult);
	return arrayUsers;
}

bool CApplicationData::StartSession(const wxString& userName, const wxString& userPassword)
{
	if (!CloseSession())
		return false;

	m_sessionUpdater = std::shared_ptr<CApplicationDataSessionUpdater>(
		new CApplicationDataSessionUpdater(this, m_sessionGuid)
	);

	if (!m_sessionUpdater->InitSessionUpdater())
		return false;

	bool succes = true;

	if (!AuthenticationAndSetUser(userName, userPassword)) {

		succes = false;

		if (backend_mainFrame != nullptr &&
			backend_mainFrame->AuthenticationUser(userName, userPassword)) {
			succes = true;
		}
	}

	if (succes)
		m_sessionUpdater->StartSessionUpdater();
	else if (!succes && !CloseSession())
		return false;

	m_connected_to_db = succes;
	return succes;
}

bool CApplicationData::CloseSession()
{
	if (m_sessionUpdater != nullptr) {

		if (m_sessionUpdater->Delete() != wxTHREAD_NO_ERROR)
			return false;

		m_sessionUpdater = nullptr;
	}

	m_connected_to_db = false;
	return true;
}