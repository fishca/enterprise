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

bool CApplicationData::m_forceExit = false;
wxCriticalSection CApplicationData::m_cs_force_exit;

///////////////////////////////////////////////////////////////////////////////

CApplicationData::CApplicationData(eRunMode runMode) :
	m_db(nullptr), m_runMode(runMode),
	m_sessionGuid(wxNewUniqueGuid),
	m_startedDate(wxDateTime::Now()),
	m_strComputer(wxGetHostName()),
	m_sessionUpdater(nullptr),
	m_locale_lang(wxLanguage::wxLANGUAGE_UNKNOWN)
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
		s_instance->ReadEngineConfig();

		if (!SetLocaleAppDataEnv())
			return false;
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

bool CApplicationData::CreateFileAppDataEnv(eRunMode runMode, const wxString& strDirDatabase, const wxString& strLocale)
{
	if (s_instance != nullptr) s_instance->DestroyAppDataEnv();
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	try {
#endif
		std::shared_ptr<CFirebirdDatabaseLayer> db(new CFirebirdDatabaseLayer());

		if (db->Open(strDirDatabase + wxT("\\") + sys_db)) {

			s_instance = new CApplicationData(runMode);
			s_instance->m_strFile = strDirDatabase;

			s_instance->ReadEngineConfig();

			s_instance->m_db = db;
			s_instance->m_exclusiveMode = runMode == eRunMode::eDESIGNER_MODE;

			if (runMode == eRunMode::eDESIGNER_MODE && !CApplicationData::TableAlreadyCreated()) {
				CApplicationData::CreateTableSession();
				CApplicationData::CreateTableUser();
				CApplicationData::CreateTableSequence();
				CApplicationData::CreateTableEvent();
			}
			else if (!CApplicationData::TableAlreadyCreated()) {
				DestroyAppDataEnv();
				return false;
			}

			if (!SetLocaleAppDataEnv(strLocale))
				return false;

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

bool CApplicationData::CreateServerAppDataEnv(eRunMode runMode, const wxString& strServer, const wxString& strPort,
	const wxString& strUser, const wxString& strPassword, const wxString& strDatabase, const wxString& strLocale)
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

			s_instance->ReadEngineConfig();

			s_instance->m_db = db;
			s_instance->m_exclusiveMode = runMode == eRunMode::eDESIGNER_MODE;

			if (!SetLocaleAppDataEnv(strLocale))
				return false;

			if (runMode == eRunMode::eDESIGNER_MODE && !CApplicationData::TableAlreadyCreated()) {
				CApplicationData::CreateTableSession();
				CApplicationData::CreateTableUser();
				CApplicationData::CreateTableSequence();
				CApplicationData::CreateTableEvent();
			}
			else if (!CApplicationData::TableAlreadyCreated()) {
				DestroyAppDataEnv();
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

bool CApplicationData::SetLocaleAppDataEnv(const wxString& strLocale)
{
	if (s_instance == nullptr)
		return false;

	return s_instance->InitLocale(strLocale);
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

#include <wx/fileconf.h>
#include <wx/stdpaths.h>

bool CApplicationData::InitLocale(const wxString& locale)
{
	if (m_locale_lang == wxLanguage::wxLANGUAGE_UNKNOWN) {

#ifdef DEBUG_TRANSLATE
		wxLog::AddTraceMask(wxS("i18n"));
#endif // WXDEBUG

		m_locale_lang = wxLocale::GetSystemLanguage();

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

		if (!m_locale.Init(m_locale_lang)) {
			if (!m_locale.Init(wxLanguage::wxLANGUAGE_ENGLISH))
				return false;
			m_locale_lang = wxLanguage::wxLANGUAGE_ENGLISH;
		}

		// Initialize the catalogs we'll be using.
		m_locale.AddCatalog(wxT("open_es"));

		// Initialize localization engine 
		CBackendLocalization::SetUserLanguage(m_locale.GetName());

		//Set default time 
		wxDateTime::SetCountry(wxDateTime::Country::Country_Default);
		return true;
	}

	return false;
}

bool CApplicationData::Connect(const wxString& strUserName, const wxString& strUserPassword, const int flags)
{
	if (m_created_metadata)
		return false;
	if (!StartSession(strUserName, strUserPassword))
		return false;  //start session
	if (!metaDataCreate(m_runMode, flags))
		return false;
	m_created_metadata = true;
	m_run_metadata = activeMetaData->RunDatabase();
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
			const bool isConfigOpen = activeMetaData->IsConfigOpen();
			if (isConfigOpen && !activeMetaData->CloseDatabase(forceCloseFlag))
				return false;
			m_connected_to_db = false;
		}

		metaDataDestroy();
		m_created_metadata = false;
	}

	return true;
}

#pragma region config

#define BACKEND_CONF wxT("backend.conf")

void CApplicationData::ReadEngineConfig()
{
	const wxString& workingDir = wxGetCwd(); wxString strConfigFile;
	if (wxFileName::FileExists(workingDir + wxFILE_SEP_PATH + BACKEND_CONF)) {
		strConfigFile = workingDir +
			wxFILE_SEP_PATH + BACKEND_CONF;
	}
	else {
		wxFileName fn(wxStandardPaths::Get().GetExecutablePath());
		strConfigFile = fn.GetPath() +
			wxFILE_SEP_PATH + BACKEND_CONF;
	}

	wxFileConfig fc(wxT(""), wxT(""), wxT(""), strConfigFile);
	fc.Read(wxT("Locale"), &m_configInfo.m_strLocale);
}

#pragma endregion 

///////////////////////////////////////////////////////////////////////////////
#include "backend/debugger/debugClient.h"

#pragma region execute 
long CApplicationData::RunApplication(const wxString& strAppName, bool searchDebug) const
{
	return RunApplication(strAppName,
		m_userInfo.m_strUserName,
		m_userInfo.m_strUserPassword,
		searchDebug
	);
}

long CApplicationData::RunApplication(const wxString& strAppName, const wxString& strUserName, const wxString& strUserPassword, bool searchDebug) const
{
	wxString executeCmd = strAppName + wxT(' ');

	if (m_strFile.IsEmpty()) {

		if (!m_strServer.IsEmpty())
			executeCmd += wxString::Format(wxT(" /srv %s"), m_strServer);
		if (!m_strPort.IsEmpty())
			executeCmd += wxString::Format(wxT(" /p %s"), m_strPort);
		if (!m_strDatabase.IsEmpty())
			executeCmd += wxString::Format(wxT(" /db %s"), m_strDatabase);
		if (!m_strUser.IsEmpty())
			executeCmd += wxString::Format(wxT(" /usr %s"), m_strUser);
		if (!m_strPassword.IsEmpty())
			executeCmd += wxString::Format(wxT(" /pwd %s"), m_strPassword);
	}
	else {
		executeCmd += wxString::Format(wxT(" /file %s"), m_strFile);
	}

	if (searchDebug)
		executeCmd += wxString::Format(wxT(" /debug"));

	if (!strUserName.IsEmpty())
		executeCmd += wxString::Format(wxT(" /ib_usr %s"), strUserName);

	if (!strUserPassword.IsEmpty())
		executeCmd += wxString::Format(wxT(" /ib_pwd %s"), strUserPassword);

	executeCmd += wxString::Format(wxT(" /lc %s"), m_locale.GetName());

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

#pragma endregion

///////////////////////////////////////////////////////////////////////////////

bool CApplicationData::AuthenticationAndSetUser(const wxString& strUserName, const wxString& strUserPassword)
{
	if (!strUserName.IsEmpty() || HasAllowedUser()) {
		m_userInfo = ReadUserData(strUserName);
		return m_userInfo.IsOk() &&
			m_userInfo.m_strUserPassword == CApplicationData::ComputeMd5(strUserPassword);
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////

void CApplicationData::ReadUserData_Password(const wxMemoryBuffer& buffer, CApplicationDataUserInfo& userInfo) const
{
	CMemoryReader reader(buffer);

	userInfo.m_strUserGuid = reader.r_stringZ();
	userInfo.m_strUserName = reader.r_stringZ();
	userInfo.m_strUserFullName = reader.r_stringZ();
	userInfo.m_strUserPassword = reader.r_stringZ();
}

void CApplicationData::ReadUserData_Role(const wxMemoryBuffer& buffer, CApplicationDataUserInfo& userInfo) const
{
	CMemoryReader reader(buffer);

	unsigned int count = reader.r_u32();
	userInfo.m_roleArray.reserve(count);
	for (unsigned int idx = 0; idx < count; idx++) {
		CApplicationDataUserInfo::CApplicationDataUserRole entry;
		entry.m_strRoleGuid = reader.r_stringZ();
		entry.m_miRoleId = reader.r_s32();
		userInfo.m_roleArray.emplace_back(std::move(entry));
	}
}

void CApplicationData::ReadUserData_Language(const wxMemoryBuffer& buffer, CApplicationDataUserInfo& userInfo) const
{
	CMemoryReader reader(buffer);
	userInfo.m_strLanguageGuid = reader.r_stringZ();
	userInfo.m_strLanguageCode = reader.r_stringZ();
}

///////////////////////////////////////////////////////////////////////////////

wxMemoryBuffer CApplicationData::SaveUserData_Password(const CApplicationDataUserInfo& userInfo) const
{
	CMemoryWriter writter;
	writter.w_stringZ(userInfo.m_strUserGuid);
	writter.w_stringZ(userInfo.m_strUserName);
	writter.w_stringZ(userInfo.m_strUserFullName);
	writter.w_stringZ(userInfo.m_strUserPassword);
	return writter.buffer();
}

wxMemoryBuffer CApplicationData::SaveUserData_Role(const CApplicationDataUserInfo& userInfo) const
{
	CMemoryWriter writter;
	writter.w_u32(userInfo.m_roleArray.size());
	for (const auto role : userInfo.m_roleArray) {
		writter.w_stringZ(role.m_strRoleGuid);
		writter.w_s32(role.m_miRoleId);
	}
	return writter.buffer();
}

wxMemoryBuffer CApplicationData::SaveUserData_Language(const CApplicationDataUserInfo& userInfo) const
{
	CMemoryWriter writter;
	writter.w_stringZ(userInfo.m_strLanguageGuid);
	writter.w_stringZ(userInfo.m_strLanguageCode);
	return writter.buffer();
}

///////////////////////////////////////////////////////////////////////////////

#include <backend/utils/wxmd5.hpp>

wxString CApplicationData::ComputeMd5(const wxString& userPassword) const
{
	if (userPassword.Length() > 0)
		return wxMD5::ComputeMd5(userPassword);

	return wxEmptyString;
}

///////////////////////////////////////////////////////////////////////////////