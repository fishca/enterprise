////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxFormBuider
//	Description : app info
////////////////////////////////////////////////////////////////////////////

#include "backend/appData.h"

//databases
#include "backend/databaseLayer/firebird/firebirdDatabaseLayer.h"
#include "backend/databaseLayer/postgres/postgresDatabaseLayer.h"
#include "backend/databaseLayer/sqllite/sqliteDatabaseLayer.h"

//sandbox
#include "metadataConfiguration.h"

///////////////////////////////////////////////////////////////////////////////
//								CApplicationDataSessionArray
///////////////////////////////////////////////////////////////////////////////

wxString CApplicationDataSessionArray::GetUserName(unsigned int idx) const {
	if (idx > m_listSession.size())
		return wxEmptyString;
	return m_listSession[idx].m_strUserName;
}

wxString CApplicationDataSessionArray::GetComputerName(unsigned int idx) const {
	if (idx > m_listSession.size())
		return wxEmptyString;
	return m_listSession[idx].m_strComputerName;
}

wxString CApplicationDataSessionArray::GetSession(unsigned int idx) const {
	if (idx > m_listSession.size())
		return wxEmptyString;
	return m_listSession[idx].m_strSession;
}

wxString CApplicationDataSessionArray::GetStartedDate(unsigned int idx) const {
	if (idx > m_listSession.size())
		return wxEmptyString;
	const wxDateTime& startedDate = m_listSession[idx].m_startedDate;
	return startedDate.Format(wxT("%d.%m.%Y %H:%M:%S"));
}

wxString CApplicationDataSessionArray::GetApplication(unsigned int idx) const {
	if (idx > m_listSession.size())
		return wxEmptyString;
	return appData->GetModeDescr(m_listSession[idx].m_runMode);
}

///////////////////////////////////////////////////////////////////////////////

eRunMode CApplicationDataSessionArray::GetSessionApplication(unsigned int idx) const
{
	if (idx > m_listSession.size())
		return eRunMode::eLAUNCHER_MODE;
	return m_listSession[idx].m_runMode;
}

///////////////////////////////////////////////////////////////////////////////
//								CApplicationData
///////////////////////////////////////////////////////////////////////////////

CApplicationData* CApplicationData::s_instance = nullptr;

///////////////////////////////////////////////////////////////////////////////

CApplicationData::CApplicationData(eRunMode runMode) :
	m_db(nullptr), m_runMode(runMode),
	m_sessionGuid(wxNewUniqueGuid),
	m_startedDate(wxDateTime::Now()),
	m_strComputer(wxGetHostName()),
	m_sessionUpdater(nullptr)
{
}

CApplicationData::~CApplicationData()
{
}

///////////////////////////////////////////////////////////////////////////////

bool CApplicationData::CreateAppDataEnv()
{
	if (s_instance != nullptr) s_instance->DestroyAppDataEnv();
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	try {
#endif
		s_instance = new CApplicationData(eRunMode::eENTERPRISE_MODE);
		return true;
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	}
	catch (const DatabaseLayerException* err) {
		return false;
	}
#endif
	return false;
}

#define sys_db wxT("sys.fdb")

bool CApplicationData::CreateAppDataEnv(eRunMode runMode, const wxString& strDirDatabase)
{
	if (s_instance != nullptr) s_instance->DestroyAppDataEnv();
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	try {
#endif
		std::shared_ptr<CFirebirdDatabaseLayer> db(new CFirebirdDatabaseLayer());

		if (db->Open(strDirDatabase + wxT("\\") + sys_db)) {

			s_instance = new CApplicationData(runMode);
			s_instance->m_strFile = strDirDatabase;

			s_instance->m_db = db;
			s_instance->m_exclusiveMode = runMode == eRunMode::eDESIGNER_MODE;

			if (runMode == eRunMode::eDESIGNER_MODE && !CApplicationData::TableAlreadyCreated()) {
				CApplicationData::CreateTableSession();
				CApplicationData::CreateTableUser();
				CApplicationData::CreateTableEvent();
			}
			else if (!CApplicationData::TableAlreadyCreated()) {
				s_instance->DestroyAppDataEnv();
				return false;
			}

			return true;
		}
		return false;
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	}
	catch (const DatabaseLayerException* err) {
		return false;
	}
#endif
	return false;
}

bool CApplicationData::CreateAppDataEnv(eRunMode runMode, const wxString& strServer, const wxString& strPort,
	const wxString& strUser, const wxString& strPassword, const wxString& strDatabase)
{
	if (s_instance != nullptr) s_instance->DestroyAppDataEnv();
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	try {
#endif
		std::shared_ptr<CPostgresDatabaseLayer> db(new CPostgresDatabaseLayer());
		if (db->Open(strServer, strPort, strDatabase, strUser, strPassword)) {

			s_instance = new CApplicationData(runMode);

			s_instance->m_strServer = strServer;
			s_instance->m_strPort = strPort;
			s_instance->m_strUser = strUser;
			s_instance->m_strPassword = strPassword;
			s_instance->m_strDatabase = strDatabase;

			s_instance->m_db = db;
			s_instance->m_exclusiveMode = runMode == eRunMode::eDESIGNER_MODE;

			if (runMode == eRunMode::eDESIGNER_MODE && !CApplicationData::TableAlreadyCreated()) {
				CApplicationData::CreateTableSession();
				CApplicationData::CreateTableUser();
				CApplicationData::CreateTableEvent();
			}
			else if (!CApplicationData::TableAlreadyCreated()) {
				s_instance->DestroyAppDataEnv();
				return false;
			}

			return true;
		}
		return false;
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	}
	catch (const DatabaseLayerException* err) {
		return false;
	}
#endif
	return false;
}

bool CApplicationData::DestroyAppDataEnv()
{
	if (s_instance != nullptr && s_instance->m_db != nullptr) {

		if (!s_instance->Disconnect()) return false;

		s_instance->m_strServer = wxEmptyString;
		s_instance->m_strPort = wxEmptyString;
		s_instance->m_strUser = wxEmptyString;
		s_instance->m_strPassword = wxEmptyString;
		s_instance->m_strDatabase = wxEmptyString;

		if (s_instance->m_db->IsOpen())
			s_instance->m_db->Close();

		s_instance->m_db = nullptr;

		wxDELETE(s_instance);
		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////

bool CApplicationData::Connect(const wxString& user, const wxString& password, const int flags)
{
	if (m_created_metadata)
		return false;
	if (!metaDataCreate(m_runMode, flags))
		return false;
	m_created_metadata = true;
	if (!StartSession(user, password))
		return false;  //start session	 
	m_run_metadata = commonMetaData->RunDatabase();
	return m_connected_to_db &&
		m_created_metadata &&
		m_run_metadata;
}

bool CApplicationData::Disconnect()
{
	if (m_created_metadata) {

		if (!CloseSession())
			return false;

		if (m_run_metadata) {
			const bool isConfigOpen = commonMetaData->IsConfigOpen();
			if (isConfigOpen && !commonMetaData->CloseDatabase(forceCloseFlag))
				return false;
			m_connected_to_db = false;
		}

		metaDataDestroy();
		m_created_metadata = false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
#include "backend/debugger/debugClient.h"

#pragma region execute 
long CApplicationData::RunApplication(const wxString& strAppName, bool searchDebug) const
{
	wxString executeCmd = strAppName + " ";

	if (m_strFile.IsEmpty()) {
		if (!m_strServer.IsEmpty())
			executeCmd += wxT(" /srv ") + m_strServer;
		if (!m_strPort.IsEmpty())
			executeCmd += wxT(" /p ") + m_strPort;
		if (!m_strDatabase.IsEmpty())
			executeCmd += wxT(" /db ") + m_strDatabase;
		if (!m_strUser.IsEmpty())
			executeCmd += wxT(" /usr ") + m_strUser;
		if (!m_strPassword.IsEmpty())
			executeCmd += wxT(" /pwd ") + m_strPassword;
	}
	else {
		executeCmd += wxT(" /file ") + m_strFile;
	}

	if (searchDebug)
		executeCmd += wxT(" /debug");
	if (!m_strUserIB.IsEmpty())
		executeCmd += wxT(" /ib_usr ") + m_strUserIB;
	if (!m_strPasswordIB.IsEmpty())
		executeCmd += wxT(" /ib_pwd ") + m_strPasswordIB;

	const long execute = wxExecute(executeCmd);

	if (searchDebug) {

		debugClient->SearchServer(true);

		while (debugClient != nullptr) {

			if (debugClient->GetConnectionSuccess())
				break;

			wxMilliSleep(5);
		}
	}

	return execute;
}

long CApplicationData::RunApplication(const wxString& strAppName, const wxString& user, const wxString& password, bool searchDebug) const
{
	wxString executeCmd = strAppName + " ";

	if (m_strFile.IsEmpty()) {

		if (!m_strServer.IsEmpty())
			executeCmd += wxT(" /srv ") + m_strServer;
		if (!m_strPort.IsEmpty())
			executeCmd += wxT(" /p ") + m_strPort;
		if (!m_strDatabase.IsEmpty())
			executeCmd += wxT(" /db ") + m_strDatabase;
		if (!m_strUser.IsEmpty())
			executeCmd += wxT(" /usr ") + m_strUser;
		if (!m_strPassword.IsEmpty())
			executeCmd += wxT(" /pwd ") + m_strPassword;
	}
	else {
		executeCmd += wxT(" /file ") + m_strFile;
	}

	if (searchDebug)
		executeCmd += wxT(" /debug");

	if (!user.IsEmpty())
		executeCmd += wxT(" /ib_usr ") + user;
	if (!password.IsEmpty())
		executeCmd += wxT(" /ib_pwd ") + password;

	const long execute = wxExecute(executeCmd);

	if (searchDebug) {

		debugClient->SearchServer(true);

		while (debugClient != nullptr) {

			if (debugClient->GetConnectionSuccess())
				break;

			wxMilliSleep(5);
		}
	}

	return execute;
}
#pragma endregion
///////////////////////////////////////////////////////////////////////////////

#include <backend/utils/wxmd5.hpp>

wxString CApplicationData::ComputeMd5(const wxString& userPassword) const
{
	if (userPassword.Length() > 0)
		return wxMD5::ComputeMd5(userPassword);

	return wxEmptyString;
}

///////////////////////////////////////////////////////////////////////////////