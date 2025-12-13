////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : external metaData - for dataProcessors, reports
////////////////////////////////////////////////////////////////////////////

#include "metadataDataProcessor.h"
#include "backend/appData.h"

CMetaDataDataProcessor::CMetaDataDataProcessor() : IMetaData(),
m_commonObject(nullptr),
m_moduleManager(nullptr),
m_ownerMeta(nullptr),
m_configOpened(false),
m_version(version_oes_last)
{
	//create main metaObject
	m_commonObject = new CMetaObjectExternalDataProcessor;
	m_commonObject->SetName(
		IMetaData::GetNewName(g_metaExternalDataProcessorCLSID, nullptr, m_commonObject->GetClassName())
	);

	if (m_commonObject->OnCreateMetaObject(this, newObjectFlag)) {
		m_moduleManager = new CModuleManagerExternalDataProcessor(this, m_commonObject);
		m_moduleManager->IncrRef();
		if (!m_commonObject->OnLoadMetaObject(this)) {
			wxASSERT_MSG(false, "m_commonObject->OnLoadMetaObject() == false");
		}
		m_moduleManager->PrepareNames();
	}

	m_commonObject->PrepareNames();
	m_commonObject->IncrRef();

	wxASSERT(m_moduleManager);
	m_ownerMeta = this;
}

CMetaDataDataProcessor::CMetaDataDataProcessor(IMetaData* metaData, CMetaObjectDataProcessor* srcDataProcessor) : IMetaData(),
m_commonObject(srcDataProcessor),
m_moduleManager(nullptr),
m_ownerMeta(nullptr),
m_configOpened(false),
m_version(version_oes_last)
{
	if (srcDataProcessor == nullptr) {
		IMetaObject* commonMetaObject = metaData->GetCommonMetaObject();
		wxASSERT(commonMetaObject);
		//create main metaObject
		m_commonObject = new CMetaObjectDataProcessor();
		m_commonObject->SetName(
			IMetaData::GetNewName(g_metaDataProcessorCLSID, nullptr, m_commonObject->GetClassName())
		);
		if (commonMetaObject != nullptr) {
			m_commonObject->SetParent(commonMetaObject);
			commonMetaObject->AddChild(m_commonObject);
		}
	}

	m_commonObject->PrepareNames();
	m_commonObject->IncrRef();

	m_ownerMeta = metaData;
}

CMetaDataDataProcessor::~CMetaDataDataProcessor()
{
	if (m_commonObject->IsExternalCreate()) {
		
		if (!m_moduleManager->DestroyMainModule()) {
			wxASSERT_MSG(false, "m_moduleManager->DestroyMainModule() == false");
		}
		
		//delete module manager
		if (m_moduleManager != nullptr) {
			m_moduleManager->DecrRef();
		}
		
		//clear data 
		if (!ClearDatabase()) {
			wxASSERT_MSG(false, "ClearDatabase() == false");
		}

		//delete common metaObject
		m_commonObject->DecrRef();
	}
}

bool CMetaDataDataProcessor::LoadDatabase()
{
	return RunDatabase();
}

bool CMetaDataDataProcessor::SaveDatabase()
{
	return true;
}

bool CMetaDataDataProcessor::ClearDatabase()
{
	if (!ClearChildMetadata(m_commonObject))
		return false;
	
	return true;
}

bool CMetaDataDataProcessor::ClearChildMetadata(IMetaObject* object)
{
	for (unsigned int idx = 0; idx < object->GetChildCount(); idx++) {

		auto child = object->GetChild(idx);
		if (!object->FilterChild(child->GetClassType()))
			continue;

		if (!child->IsDeleted() && !child->OnDeleteMetaObject())
			return false;

		if (!ClearChildMetadata(child))
			return false;

		object->RemoveChild(child);
		idx--;
	}

	if (object != m_commonObject) 
		object->DecrRef();

	return true;
}

bool CMetaDataDataProcessor::RunDatabase(int flags)
{
	if (m_commonObject->IsExternalCreate()) {

		if (!m_commonObject->OnBeforeRunMetaObject(flags)) {
			wxASSERT_MSG(false, "m_commonObject->OnBeforeRunMetaObject() == false");
			return false;
		}
		if (!RunChildMetadata(m_commonObject, flags, true)) {
			return false;
		}

		if (m_moduleManager->CreateMainModule()) {
			if (!m_commonObject->OnAfterRunMetaObject(flags)) {
				wxASSERT_MSG(false, "m_commonObject->OnBeforeRunMetaObject() == false");
				return false;
			}
			if (!RunChildMetadata(m_commonObject, flags, false)) {
				return false;
			}
			m_configOpened = true;
			if (!m_moduleManager->StartMainModule()) {
				return false;
			}
			return true;
		}
	}
	else if (!m_commonObject->IsExternalCreate()) {

		if (!m_commonObject->OnBeforeRunMetaObject(flags)) {
			wxASSERT_MSG(false, "m_commonObject->OnBeforeRunMetaObject() == false");
			return false;
		}
		if (!RunChildMetadata(m_commonObject, flags, true)) {
			return false;
		}
		if (!m_commonObject->OnAfterRunMetaObject(flags)) {
			wxASSERT_MSG(false, "m_commonObject->OnBeforeRunMetaObject() == false");
			return false;
		}
		if (!RunChildMetadata(m_commonObject, flags, false)) {
			return false;
		}

		return true;
	}

	return false;
}

bool CMetaDataDataProcessor::RunChildMetadata(IMetaObject* object, int flags, bool before)
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

bool CMetaDataDataProcessor::CloseDatabase(int flags)
{
	wxASSERT(m_configOpened);

	if (!ExitMainModule((flags & forceCloseFlag) != 0))
		return false;

	if (!m_commonObject->IsDeleted()) {
		if (!CloseChildMetadata(m_commonObject, (flags & forceCloseFlag) != 0, true)) {
			return false;
		}
	}
	m_commonObject->OnBeforeCloseMetaObject();
	if (!CloseChildMetadata(m_commonObject, (flags & forceCloseFlag) != 0, false))
		return false;
	m_commonObject->OnAfterCloseMetaObject();
	m_configOpened = false;
	return true;
}

bool CMetaDataDataProcessor::CloseChildMetadata(IMetaObject* object, int flags, bool before)
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

#include <fstream>

bool CMetaDataDataProcessor::LoadFromFile(const wxString& strFileName)
{
	if (!m_commonObject->IsExternalCreate()) {
		if (!m_commonObject->OnCreateMetaObject(m_ownerMeta, newObjectFlag))
			return false;
	}
	else if (m_commonObject->IsExternalCreate()) {
		//close data 
		if (m_configOpened && !CloseDatabase(forceCloseFlag)) {
			wxASSERT_MSG(false, "CloseDatabase() == false");
			return false;
		}
		//clear data 
		if (!ClearDatabase()) {
			wxASSERT_MSG(false, "ClearDatabase() == false");
			return false;
		}
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

	m_fullPath = strFileName;

	//loading common metaData and child item
	if (!LoadCommonMetadata(g_metaExternalDataProcessorCLSID, readerData)) {
		if (m_commonObject->IsExternalCreate()) {
			//clear data 
			if (!ClearDatabase()) {
				wxASSERT_MSG(false, "ClearDatabase() == false");
			}
		}
		return false;
	}

	if (!m_commonObject->IsExternalCreate()) {
		m_commonObject->BuildNewName();
	}

	return LoadDatabase();
}

bool CMetaDataDataProcessor::SaveToFile(const wxString& strFileName)
{
	//common data
	CMemoryWriter writterData;

	//Save header info 
	if (!SaveHeader(writterData))
		return false;

	m_fullPath = strFileName;

	//Save common object
	if (!SaveCommonMetadata(g_metaExternalDataProcessorCLSID, writterData, saveConfigFlag))
		return false;

	//Delete common object
	if (!DeleteCommonMetadata(g_metaExternalDataProcessorCLSID))
		return false;

	std::ofstream datafile;
	datafile.open(strFileName.ToStdWstring(), std::ios::binary);
	datafile.write(reinterpret_cast <char*> (writterData.pointer()), writterData.size());
	datafile.close();

	return true;
}

bool CMetaDataDataProcessor::LoadHeader(CMemoryReader& readerData)
{
	CMemoryReader* readerMemory = readerData.open_chunk(eHeaderBlock);

	if (!readerMemory)
		return false;

	u64 metaSign = readerMemory->r_u64();
	if (metaSign != sign_dataProcessor)
		return false;

	m_version = readerMemory->r_u32();

	wxString metaGuid;
	readerMemory->r_stringZ(metaGuid);

	readerMemory->close();
	return true;
}

bool CMetaDataDataProcessor::LoadCommonMetadata(const class_identifier_t& clsid, CMemoryReader& readerData)
{
	CMemoryReader* readerMemory = readerData.open_chunk(clsid);

	if (!readerMemory)
		return false;

	u64 meta_id = 0;
	CMemoryReader* readerMetaMemory = readerMemory->open_chunk_iterator(meta_id);

	if (!readerMetaMemory)
		return true;

	std::shared_ptr <CMemoryReader> readerChildMemory(readerMetaMemory->open_chunk(eChildBlock));
	if (readerChildMemory) {
		if (!LoadChildMetadata(clsid, *readerChildMemory, m_commonObject))
			return false;
	}

	std::shared_ptr <CMemoryReader>readerDataMemory(readerMetaMemory->open_chunk(eDataBlock));
	//m_commonObject->SetReadOnly(!m_metaReadOnly);

	if (!m_commonObject->LoadMetaObject(m_ownerMeta, *readerDataMemory))
		return false;

	if (!m_commonObject->IsExternalCreate()) {
		m_commonObject->ResetAll();
	}

	return true;
}

bool CMetaDataDataProcessor::LoadChildMetadata(const class_identifier_t&, CMemoryReader& readerData, IMetaObject* object)
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

		while (!readerData.eof())
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
			if (readerChildMemory != nullptr) {
				if (!LoadChildMetadata(clsid, *readerChildMemory, newMetaObject))
					return false;
			}

			std::shared_ptr <CMemoryReader>readerDataMemory(readerMetaMemory->open_chunk(eDataBlock));
			if (!newMetaObject->LoadMetaObject(m_ownerMeta, *readerDataMemory))
				return false;
			if (!m_commonObject->IsExternalCreate()) {
				newMetaObject->ResetId();
			}
			prevReaderMetaMemory = readerMetaMemory;
		}

		prevReaderMemory = readerMemory;
	}

	return true;
}

bool CMetaDataDataProcessor::SaveHeader(CMemoryWriter& writterData)
{
	CMemoryWriter writterMemory;
	writterMemory.w_u64(sign_dataProcessor); //sign 
	writterMemory.w_u32(m_version); // version 1 - DEFAULT
	writterMemory.w_stringZ(m_commonObject->GetDocPath()); //guid conf 

	writterData.w_chunk(eHeaderBlock, writterMemory.pointer(), writterMemory.size());
	return true;
}

bool CMetaDataDataProcessor::SaveCommonMetadata(const class_identifier_t& clsid, CMemoryWriter& writterData, int flags)
{
	//Save common object
	CMemoryWriter writterMemory;

	CMemoryWriter writterMetaMemory;
	CMemoryWriter writterDataMemory;

	if (!m_commonObject->SaveMetaObject(m_ownerMeta, writterDataMemory, flags)) {
		return false;
	}

	writterMetaMemory.w_chunk(eDataBlock, writterDataMemory.pointer(), writterDataMemory.size());

	CMemoryWriter writterChildMemory;

	if (!SaveChildMetadata(clsid, writterChildMemory, m_commonObject, flags))
		return false;

	writterMetaMemory.w_chunk(eChildBlock, writterChildMemory.pointer(), writterChildMemory.size());
	writterMemory.w_chunk(m_commonObject->GetMetaID(), writterMetaMemory.pointer(), writterMetaMemory.size());

	writterData.w_chunk(clsid, writterMemory.pointer(), writterMemory.size());
	return true;
}

bool CMetaDataDataProcessor::SaveChildMetadata(const class_identifier_t&, CMemoryWriter& writterData, IMetaObject* object, int flags)
{
	for (unsigned int idx = 0; idx < object->GetChildCount(); idx++) {
		
		auto child = object->GetChild(idx);
		if (!object->FilterChild(child->GetClassType()))
			continue;
		
		CMemoryWriter writterMemory;
		if (child->IsDeleted())
			continue;

		CMemoryWriter writterMetaMemory;
		CMemoryWriter writterDataMemory;
		if (!child->SaveMetaObject(m_ownerMeta, writterDataMemory, flags)) {
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

bool CMetaDataDataProcessor::DeleteCommonMetadata(const class_identifier_t& clsid)
{
	return DeleteChildMetadata(clsid, m_commonObject);
}

bool CMetaDataDataProcessor::DeleteChildMetadata(const class_identifier_t& clsid, IMetaObject* object)
{
	for (unsigned int idx = 0; idx < object->GetChildCount(); idx++) {
		
		auto child = object->GetChild(idx);
		if (!object->FilterChild(child->GetClassType()))
			continue;
		
		if (child->IsDeleted()) {

			if (!child->DeleteMetaObject(m_ownerMeta))
				return false;

			if (!DeleteChildMetadata(child->GetClassType(), child))
				return false;

			object->RemoveChild(child);

			child->DecrRef();
		}
		else {
			if (!DeleteChildMetadata(child->GetClassType(), child))
				return false;
		}
	}

	return true;
}