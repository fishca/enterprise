#include "metadataConfiguration.h"

#include "backend/debugger/debugClient.h"
#include "backend/debugger/debugServer.h"

#include "backend/backend_mainFrame.h"
#include "backend/appData.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
IMetaDataConfiguration* IMetaDataConfiguration::ms_instance = nullptr;
//////////////////////////////////////////////////////////////////////////////////////////////////////

bool IMetaDataConfiguration::Initialize(eRunMode mode, const int flags)
{
	if (ms_instance == nullptr) {

		switch (mode)
		{
		case eLAUNCHER_MODE: break;
		case eDESIGNER_MODE:
			ms_instance = new CMetaDataConfigurationStorage();
			break;
		default:
			ms_instance = new CMetaDataConfiguration();
			break;
		}

		return ms_instance != nullptr ?
			ms_instance->OnInitialize(flags) : false;
	}

	return false;
}

bool IMetaDataConfiguration::Destroy()
{
	if (ms_instance != nullptr) {
		ms_instance->OnDestroy();
	}
	wxDELETE(ms_instance);
	return true;
}

#include "backend/metaCollection/metaLanguageObject.h"

//**************************************************************************************************
//*                                          ConfigMetadata										   *
//**************************************************************************************************

CMetaDataConfigurationFile::CMetaDataConfigurationFile() : IMetaDataConfiguration(),
m_commonObject(nullptr), m_moduleManager(nullptr), m_configOpened(false)
{
	//create main metaObject
	m_commonObject = new CMetaObjectConfiguration();
	//m_commonObject->SetReadOnly(!m_metaReadOnly);

	if (m_commonObject->OnCreateMetaObject(this, newObjectFlag)) {
		m_moduleManager = new CModuleManagerConfiguration(this, m_commonObject);
		m_moduleManager->IncrRef();
		if (!m_commonObject->OnLoadMetaObject(this)) {
			wxASSERT_MSG(false, "m_commonObject->OnLoadMetaObject() == false");
		}
		m_moduleManager->PrepareNames();
	}

	m_commonObject->PrepareNames();
	m_commonObject->IncrRef();

	{
		CValue* ppParams[] = { m_commonObject };
		CMetaObjectLanguage* commonLanguage =
			CValue::CreateAndConvertObjectRef<CMetaObjectLanguage>(g_metaLanguageCLSID, ppParams, 1);

		if (commonLanguage->OnCreateMetaObject(this, newObjectFlag)) {

			if (!commonLanguage->OnLoadMetaObject(this)) {
				wxASSERT_MSG(false, "commonLanguage->OnLoadMetaObject() == false");
			}

			commonLanguage->SetName(wxT("english"));
		}

		commonLanguage->PrepareNames();
		commonLanguage->IncrRef();

		m_commonObject->SetLanguage(commonLanguage->GetMetaID());
	}

	wxASSERT(m_moduleManager);
}

CMetaDataConfigurationFile::~CMetaDataConfigurationFile()
{
	//delete module manager
	wxDELETE(m_moduleManager);

	//clear data 
	if (!ClearDatabase()) {
		wxASSERT_MSG(false, "ClearDatabase() == false");
	}

	//delete common metaObject
	wxDELETE(m_commonObject);
}

////////////////////////////////////////////////////////////////////

wxString CMetaDataConfigurationFile::GetLangCode() const
{
	if (m_commonObject != nullptr)
		return m_commonObject->GetLangCode();

	return wxT("");
}

bool CMetaDataConfigurationFile::IsFullAccess() const
{
	bool access_right = true;

	for (const auto object : GetAnyArrayObject(g_metaRoleCLSID)) {
		access_right = false;
		break;
	}

	if (access_right)
		return true;

	if (m_commonObject != nullptr)
		return m_commonObject->AccessRight_Administration();

	return true;
}

////////////////////////////////////////////////////////////////////

bool CMetaDataConfigurationFile::RunDatabase(int flags)
{
	wxASSERT(!m_configOpened);

	if (!m_commonObject->OnBeforeRunMetaObject(flags)) {
		wxASSERT_MSG(false, "m_commonObject->OnBeforeRunMetaObject() == false");
		return false;
	}

	for (unsigned int idx = 0; idx < m_commonObject->GetChildCount(); idx++) {

		auto child = m_commonObject->GetChild(idx);
		if (!m_commonObject->FilterChild(child->GetClassType()))
			continue;

		if (child->IsDeleted())
			continue;

		if (!child->OnBeforeRunMetaObject(flags))
			return false;

		if (!RunChildMetadata(child, flags, true))
			return false;
	}

	if (m_moduleManager->CreateMainModule()) {

		if (!m_commonObject->OnAfterRunMetaObject(flags)) {
			wxASSERT_MSG(false, "m_commonObject->OnBeforeRunMetaObject() == false");
			return false;
		}

		for (unsigned int idx = 0; idx < m_commonObject->GetChildCount(); idx++) {

			auto child = m_commonObject->GetChild(idx);
			if (!m_commonObject->FilterChild(child->GetClassType()))
				continue;

			if (child->IsDeleted())
				continue;

			if (!child->OnAfterRunMetaObject(flags))
				return false;

			if (!RunChildMetadata(child, flags, false))
				return false;
		}
		//if (!StartMainModule())
		//	return false;
		m_configOpened = true;
		return true;
	}

	return false;
}

bool CMetaDataConfigurationFile::RunChildMetadata(IMetaObject* object, int flags, bool before)
{
	for (unsigned int idx = 0; idx < object->GetChildCount(); idx++) {

		auto child = object->GetChild(idx);
		if (!object->FilterChild(child->GetClassType()))
			continue;

		if (child->IsDeleted())
			continue;

		if (before && !child->OnBeforeRunMetaObject(flags))
			return false;

		if (!before && !child->OnAfterRunMetaObject(flags))
			return false;

		if (!RunChildMetadata(child, flags, before))
			return false;
	}

	return true;
}

bool CMetaDataConfigurationFile::CloseDatabase(int flags)
{
	wxASSERT(m_configOpened);

	//if (!ExitMainModule((flags & forceCloseFlag) != 0))
	//	return false;

	for (unsigned int idx = 0; idx < m_commonObject->GetChildCount(); idx++) {

		auto child = m_commonObject->GetChild(idx);
		if (!m_commonObject->FilterChild(child->GetClassType()))
			continue;

		if (child->IsDeleted())
			continue;

		if (!child->OnBeforeCloseMetaObject())
			return false;

		if (!CloseChildMetadata(child, (flags & forceCloseFlag) != 0, true))
			return false;
	}

	if (!m_commonObject->OnBeforeCloseMetaObject()) {
		wxASSERT_MSG(false, "m_commonObject->OnAfterCloseMetaObject() == false");
		return false;
	}

	if (!m_moduleManager->DestroyMainModule()) {
		return false;
	}

	for (unsigned int idx = 0; idx < m_commonObject->GetChildCount(); idx++) {

		auto child = m_commonObject->GetChild(idx);
		if (!m_commonObject->FilterChild(child->GetClassType()))
			continue;

		if (child->IsDeleted())
			continue;

		if (!child->OnAfterCloseMetaObject())
			return false;

		if (!CloseChildMetadata(child, (flags & forceCloseFlag) != 0, false))
			return false;
	}

	if (!m_commonObject->OnAfterCloseMetaObject()) {
		wxASSERT_MSG(false, "m_commonObject->OnAfterCloseMetaObject() == false");
		return false;
	}

	m_configOpened = false;
	return true;
}

bool CMetaDataConfigurationFile::CloseChildMetadata(IMetaObject* object, int flags, bool before)
{
	for (unsigned int idx = 0; idx < object->GetChildCount(); idx++) {

		auto child = object->GetChild(idx);
		if (!object->FilterChild(child->GetClassType()))
			continue;

		if (child->IsDeleted())
			continue;

		if (before && !child->OnBeforeCloseMetaObject())
			return false;

		if (!before && !child->OnAfterCloseMetaObject())
			return false;

		if (!CloseChildMetadata(child, flags, before))
			return false;
	}

	return true;
}

bool CMetaDataConfigurationFile::ClearDatabase()
{
	for (unsigned int idx = 0; idx < m_commonObject->GetChildCount(); idx++) {

		auto child = m_commonObject->GetChild(idx);
		if (!m_commonObject->FilterChild(child->GetClassType()))
			continue;

		if (!child->OnDeleteMetaObject())
			return false;

		if (!ClearChildMetadata(child))
			return false;

		m_commonObject->RemoveChild(child);
		idx--;
	}

	if (!m_commonObject->OnDeleteMetaObject()) {
		wxASSERT_MSG(false, "m_commonObject->OnDeleteMetaObject() == false");
		return false;
	}

	return true;
}

bool CMetaDataConfigurationFile::ClearChildMetadata(IMetaObject* object)
{
	for (unsigned int idx = 0; idx < object->GetChildCount(); idx++) {

		auto child = object->GetChild(idx);
		if (!object->FilterChild(child->GetClassType()))
			continue;

		if (!child->OnDeleteMetaObject())
			return false;

		if (!ClearChildMetadata(child))
			return false;

		object->RemoveChild(child);
		idx--;
	}

	object->DecrRef();
	return true;
}

#include <fstream>

bool CMetaDataConfigurationFile::LoadFromFile(const wxString& strFileName)
{
	//close data 
	if (IsConfigOpen()) {
		if (!CloseDatabase(forceCloseFlag)) {
			wxASSERT_MSG(false, "CloseDatabase() == false");
			return false;

		}
	}

	//clear data 
	if (!ClearDatabase()) {
		wxASSERT_MSG(false, "ClearDatabase() == false");
		return false;
	}

	std::ifstream in(strFileName.ToStdWstring(), std::ios::in | std::ios::binary);

	if (!in.is_open())
		return false;

	//go to end
	in.seekg(0, in.end);
	//get size of file
	std::streamsize fsize = in.tellg();
	//go to beginning
	in.seekg(0, in.beg);

	wxMemoryBuffer tempBuffer(fsize);
	in.read((char*)tempBuffer.GetWriteBuf(fsize), fsize);

	CMemoryReader readerData(tempBuffer.GetData(), tempBuffer.GetBufSize());

	if (readerData.eof())
		return false;

	in.close();

	//Save header info 
	if (!LoadHeader(readerData))
		return false;

	//loading common metaData and child item
	if (!LoadCommonMetadata(g_metaCommonMetadataCLSID, readerData)) {
		//clear data 
		if (!ClearDatabase()) {
			wxASSERT_MSG(false, "ClearDatabase() == false");
		}
		return false;
	}

	return true;
}

bool CMetaDataConfigurationFile::LoadHeader(CMemoryReader& readerData)
{
	CMemoryReader* readerMemory = readerData.open_chunk(eHeaderBlock);

	if (!readerMemory)
		return false;

	u64 metaSign = readerMemory->r_u64();

	if (metaSign != sign_metadata)
		return false;

	wxString metaGuid;
	readerMemory->r_stringZ(metaGuid);

	readerMemory->close();
	return true;
}

bool CMetaDataConfigurationFile::LoadCommonMetadata(const class_identifier_t& clsid, CMemoryReader& readerData)
{
	CMemoryReader* readerMemory = readerData.open_chunk(clsid);

	if (!readerMemory)
		return false;

	u64 meta_id = 0;
	CMemoryReader* readerMetaMemory = readerMemory->open_chunk_iterator(meta_id);

	if (!readerMetaMemory)
		return true;

	std::shared_ptr <CMemoryReader>readerDataMemory(readerMetaMemory->open_chunk(eDataBlock));

	//m_commonObject->SetReadOnly(!m_metaReadOnly);

	if (!m_commonObject->LoadMetaObject(this, *readerDataMemory))
		return false;

	std::shared_ptr <CMemoryReader> readerChildMemory(readerMetaMemory->open_chunk(eChildBlock));

	if (readerChildMemory) {
		if (!LoadDatabase(clsid, *readerChildMemory, m_commonObject))
			return false;
	}

	return true;
}

bool CMetaDataConfigurationFile::LoadDatabase(const class_identifier_t&, CMemoryReader& readerData, IMetaObject* object)
{
	class_identifier_t clsid = 0;
	CMemoryReader* prevReaderMemory = nullptr;

	while (!readerData.eof())
	{
		CMemoryReader* readerMemory = readerData.open_chunk_iterator(clsid, &*prevReaderMemory);

		if (!readerMemory)
			break;

		u64 meta_id = 0;
		CMemoryReader* prevReaderMetaMemory = nullptr;

		while (!readerMemory->eof())
		{
			CMemoryReader* readerMetaMemory = readerMemory->open_chunk_iterator(meta_id, &*prevReaderMetaMemory);

			if (!readerMetaMemory)
				break;

			wxASSERT(clsid != 0);

			IMetaObject* newMetaObject = nullptr;
			CValue* ppParams[] = { object };
			try {
				newMetaObject = CValue::CreateAndConvertObjectRef<IMetaObject>(clsid, ppParams, 1);
				newMetaObject->IncrRef();
			}
			catch (...) {
				return false;
			}

			std::shared_ptr <CMemoryReader> readerChildMemory(readerMetaMemory->open_chunk(eChildBlock));
			if (readerChildMemory) {
				if (!LoadChildMetadata(clsid, *readerChildMemory, newMetaObject))
					return false;
			}

			std::shared_ptr <CMemoryReader>readerDataMemory(readerMetaMemory->open_chunk(eDataBlock));

			if (!newMetaObject->LoadMetaObject(this, *readerDataMemory))
				return false;

			prevReaderMetaMemory = readerMetaMemory;
		}

		prevReaderMemory = readerMemory;
	};

	return true;
}

bool CMetaDataConfigurationFile::LoadChildMetadata(const class_identifier_t&, CMemoryReader& readerData, IMetaObject* object)
{
	class_identifier_t clsid = 0;
	CMemoryReader* prevReaderMemory = nullptr;

	while (!readerData.eof())
	{
		CMemoryReader* readerMemory = readerData.open_chunk_iterator(clsid, &*prevReaderMemory);

		if (!readerMemory)
			break;

		u64 meta_id = 0;
		CMemoryReader* prevReaderMetaMemory = nullptr;

		while (!readerMemory->eof())
		{
			CMemoryReader* readerMetaMemory = readerMemory->open_chunk_iterator(meta_id, &*prevReaderMetaMemory);

			if (!readerMetaMemory)
				break;

			wxASSERT(clsid != 0);

			IMetaObject* newMetaObject = nullptr;
			CValue* ppParams[] = { object };
			try {
				newMetaObject = CValue::CreateAndConvertObjectRef<IMetaObject>(clsid, ppParams, 1);
				newMetaObject->IncrRef();
			}
			catch (...) {
				return false;
			}

			std::shared_ptr <CMemoryReader> readerChildMemory(readerMetaMemory->open_chunk(eChildBlock));
			if (readerChildMemory) {
				if (!LoadChildMetadata(clsid, *readerChildMemory, newMetaObject))
					return false;
			}

			std::shared_ptr <CMemoryReader>readerDataMemory(readerMetaMemory->open_chunk(eDataBlock));
			if (!newMetaObject->LoadMetaObject(this, *readerDataMemory))
				return false;

			prevReaderMetaMemory = readerMetaMemory;
		}

		prevReaderMemory = readerMemory;
	}

	return true;
}

//**************************************************************************************************
//*                                          ConfigMetadata                                        *
//**************************************************************************************************

bool CMetaDataConfiguration::OnInitialize(const int flags)
{
	m_metaGuid = CGuid::newGuid();

	if (!CMetaDataConfigurationStorage::TableAlreadyCreated())
		return false;

	debugServerInit(flags);

	if (!LoadDatabase())
		return false;

#pragma region language  
	// Check current language
	const IMetaObject* foundedLanguage =
		IMetaData::FindAnyObjectByFilter(appData->GetUserLanguageGuid(), g_metaLanguageCLSID);
	// Initialize localization engine  
	CBackendLocalization::SetUserLanguage(foundedLanguage != nullptr ? appData->GetUserLanguageCode() : GetLangCode());
#pragma endregion 

	if (backend_mainFrame != nullptr)
		backend_mainFrame->OnInitializeConfiguration(GetConfigType());

	if ((flags & _app_start_create_debug_server_flag) != 0)
		debugServer->CreateServer(defaultHost, defaultDebuggerPort, true);

	return true;
}

bool CMetaDataConfiguration::OnDestroy()
{
	debugServerDestroy();
	if (backend_mainFrame != nullptr) backend_mainFrame->OnDestroyConfiguration(GetConfigType());
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

CMetaDataConfiguration::CMetaDataConfiguration() :
	CMetaDataConfigurationFile(), m_configNew(true)
{
}

//**************************************************************************************************
//*                                          ConfigSaveMetadata                                    *
//**************************************************************************************************

bool CMetaDataConfigurationStorage::OnInitialize(const int flags)
{
	m_metaGuid = CGuid::newGuid();

	if (!CMetaDataConfigurationStorage::TableAlreadyCreated()) {
		CMetaDataConfigurationStorage::CreateConfigTable();
		CMetaDataConfigurationStorage::CreateConfigSaveTable();
	}

	// Initialize debugger
	debugClientInit();

	// Load database
	if (!LoadDatabase())
		return false;

#pragma region access
	if (!AccessRight_Administration()) {

		try {
			CBackendException::Error(_("Not enough access rights for this user!"));
		}
		catch (...) {
		}

		return false;
	}
#pragma endregion

#pragma region language  

	// Initialize localization engine 
	CBackendLocalization::SetUserLanguage(GetLangCode());

#pragma endregion 

	if (backend_mainFrame != nullptr)
		backend_mainFrame->OnInitializeConfiguration(GetConfigType());

	return true;
}

bool CMetaDataConfigurationStorage::OnDestroy()
{
	debugClientDestroy();

	if (backend_mainFrame != nullptr)
		backend_mainFrame->OnDestroyConfiguration(GetConfigType());

	return true;
}

////////////////////////////////////////////////////////////////////////////////

CMetaDataConfigurationStorage::CMetaDataConfigurationStorage() :
	CMetaDataConfiguration(), m_configMetadata(new CMetaDataConfiguration()) {
}

CMetaDataConfigurationStorage::~CMetaDataConfigurationStorage() {
	wxDELETE(m_configMetadata);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool CMetaDataConfigurationStorage::LoadDatabase(int flags)
{
	if (m_configMetadata->LoadDatabase(onlyLoadFlag)) {

		//close if opened
		if (CMetaDataConfiguration::IsConfigOpen()
			&& !CloseDatabase(forceCloseFlag)) {
			return false;
		}

		if (CMetaDataConfiguration::LoadDatabase()) {
			Modify(!CompareMetadata(m_configMetadata));
			if (m_configNew)
				SaveDatabase(saveConfigFlag);
			m_configNew = false;
			return true;
		}
	}

	return false;
}

bool CMetaDataConfigurationStorage::SaveToFile(const wxString& strFileName)
{
	//common data
	CMemoryWriter writterData;

	//Save header info 
	if (!SaveHeader(writterData))
		return false;

	//Save common object
	if (!SaveCommonMetadata(g_metaCommonMetadataCLSID, writterData, saveToFileFlag))
		return false;

	std::ofstream datafile;
	datafile.open(strFileName.ToStdWstring(), std::ios::binary);
	datafile.write(reinterpret_cast <char*> (writterData.pointer()), writterData.size());
	datafile.close();

	return true;
}

bool CMetaDataConfigurationStorage::SaveHeader(CMemoryWriter& writterData)
{
	CMemoryWriter writterMemory;
	writterMemory.w_u64(sign_metadata); //sign 
	writterMemory.w_stringZ(m_commonObject->GetDocPath()); //guid conf 

	writterData.w_chunk(eHeaderBlock, writterMemory.pointer(), writterMemory.size());
	return true;
}

bool CMetaDataConfigurationStorage::SaveCommonMetadata(const class_identifier_t& clsid, CMemoryWriter& writterData, int flags)
{
	//Save common object
	CMemoryWriter writterMemory;

	CMemoryWriter writterMetaMemory;
	CMemoryWriter writterDataMemory;

	if (!m_commonObject->SaveMetaObject(this, writterDataMemory, flags)) {
		return false;
	}

	writterMetaMemory.w_chunk(eDataBlock, writterDataMemory.pointer(), writterDataMemory.size());

	CMemoryWriter writterChildMemory;

	if (!SaveDatabase(clsid, writterChildMemory, flags))
		return false;

	writterMetaMemory.w_chunk(eChildBlock, writterChildMemory.pointer(), writterChildMemory.size());
	writterMemory.w_chunk(m_commonObject->GetMetaID(), writterMetaMemory.pointer(), writterMetaMemory.size());

	writterData.w_chunk(clsid, writterMemory.pointer(), writterMemory.size());
	return true;
}

bool CMetaDataConfigurationStorage::SaveDatabase(const class_identifier_t&, CMemoryWriter& writterData, int flags)
{
	bool saveToFile = (flags & saveToFileFlag) != 0;

	for (unsigned int idx = 0; idx < m_commonObject->GetChildCount(); idx++) {

		auto child = m_commonObject->GetChild(idx);
		if (!m_commonObject->FilterChild(child->GetClassType()))
			continue;
		if (child->IsDeleted())
			continue;
		CMemoryWriter writterMemory;
		CMemoryWriter writterMetaMemory;
		CMemoryWriter writterDataMemory;
		if (!child->SaveMetaObject(this, writterDataMemory, flags)) {
			return false;
		}
		writterMetaMemory.w_chunk(eDataBlock, writterDataMemory.pointer(), writterDataMemory.size());
		CMemoryWriter writterChildMemory;
		if (!SaveChildMetadata(child->GetClassType(), writterChildMemory, child, flags)) {
			return false;
		}
		writterMetaMemory.w_chunk(eChildBlock, writterChildMemory.pointer(), writterChildMemory.size());
		writterMemory.w_chunk(child->GetMetaID(), writterMetaMemory.pointer(), writterMetaMemory.size());
		writterData.w_chunk(child->GetClassType(), writterMemory.pointer(), writterMemory.size());
	}

	return true;
}

bool CMetaDataConfigurationStorage::SaveChildMetadata(const class_identifier_t&, CMemoryWriter& writterData, IMetaObject* object, int flags)
{
	bool saveToFile = (flags & saveToFileFlag) != 0;

	for (unsigned int idx = 0; idx < object->GetChildCount(); idx++) {

		auto child = object->GetChild(idx);
		if (!object->FilterChild(child->GetClassType()))
			continue;
		if (child->IsDeleted())
			continue;
		CMemoryWriter writterMemory;
		CMemoryWriter writterMetaMemory;
		CMemoryWriter writterDataMemory;
		if (!child->SaveMetaObject(this, writterDataMemory, flags)) {
			return false;
		}
		writterMetaMemory.w_chunk(eDataBlock, writterDataMemory.pointer(), writterDataMemory.size());
		CMemoryWriter writterChildMemory;
		if (!SaveChildMetadata(child->GetClassType(), writterChildMemory, child, flags)) {
			return false;
		}
		writterMetaMemory.w_chunk(eChildBlock, writterChildMemory.pointer(), writterChildMemory.size());
		writterMemory.w_chunk(child->GetMetaID(), writterMetaMemory.pointer(), writterMetaMemory.size());
		writterData.w_chunk(child->GetClassType(), writterMemory.pointer(), writterMemory.size());
	}

	return true;
}

bool CMetaDataConfigurationStorage::DeleteCommonMetadata(const class_identifier_t& clsid)
{
	return DeleteMetadata(clsid);
}

bool CMetaDataConfigurationStorage::DeleteMetadata(const class_identifier_t& clsid)
{
	for (unsigned int idx = 0; idx < m_commonObject->GetChildCount(); idx++) {

		auto child = m_commonObject->GetChild(idx);
		if (!m_commonObject->FilterChild(child->GetClassType()))
			continue;

		if (child->IsDeleted()) {
			if (!child->DeleteMetaObject(this)) {
				return false;
			}
		}
		if (!DeleteChildMetadata(child->GetClassType(), child)) {
			return false;
		}
		if (child->IsDeleted()) {
			m_commonObject->RemoveChild(child);
			child->DecrRef();
		}
	}

	return true;
}

bool CMetaDataConfigurationStorage::DeleteChildMetadata(const class_identifier_t& clsid, IMetaObject* object)
{
	for (unsigned int idx = 0; idx < object->GetChildCount(); idx++) {

		auto child = object->GetChild(idx);
		if (!object->FilterChild(child->GetClassType()))
			continue;

		if (child->IsDeleted()) {
			if (!child->DeleteMetaObject(this)) {
				return false;
			}
		}
		if (!DeleteChildMetadata(child->GetClassType(), child)) {
			return false;
		}
		if (child->IsDeleted()) {
			object->RemoveChild(child);
			child->DecrRef();
		}
	}

	return true;
}
