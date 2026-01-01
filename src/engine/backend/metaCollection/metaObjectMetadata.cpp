////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : metaobject common metaData
////////////////////////////////////////////////////////////////////////////

#include "metaObjectMetadata.h"
#include "metaModuleObject.h"

//*****************************************************************************************
//*                         metaData													  * 
//*****************************************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectConfiguration, IMetaObject);

//*****************************************************************************************
//*                                  MetadataObject                                       *
//*****************************************************************************************

#include "backend/metaCollection/metaLanguageObject.h"

wxString CMetaObjectConfiguration::GetLangCode() const
{
	const CMetaObjectLanguage* language =
		FindAnyObjectByFilter<CMetaObjectLanguage>(GetLanguage());

	if (language != nullptr)
		return language->GetLangCode();

	return wxT("");
}

CMetaObjectConfiguration::CMetaObjectConfiguration() : IMetaObject(configurationDefaultName)
{
	//set default proc
	(*m_propertyModuleConfiguration)->SetDefaultProcedure("beforeStart", eContentHelper::eProcedureHelper, { "cancel" });
	(*m_propertyModuleConfiguration)->SetDefaultProcedure("onStart", eContentHelper::eProcedureHelper);
	(*m_propertyModuleConfiguration)->SetDefaultProcedure("beforeExit", eContentHelper::eProcedureHelper, { "cancel" });
	(*m_propertyModuleConfiguration)->SetDefaultProcedure("onExit", eContentHelper::eProcedureHelper);

	//set def metaid
	m_metaId = defaultMetaID;
}

CMetaObjectConfiguration::~CMetaObjectConfiguration()
{
}

bool CMetaObjectConfiguration::LoadData(CMemoryReader& dataReader)
{
	m_propertyVersion->SetValue(dataReader.r_s32());

	m_propertyDefRole->LoadData(dataReader);
	m_propertyDefLanguage->LoadData(dataReader);

	return (*m_propertyModuleConfiguration)->LoadMeta(dataReader);
}

bool CMetaObjectConfiguration::SaveData(CMemoryWriter& dataWritter)
{
	dataWritter.w_s32(m_propertyVersion->GetValueAsInteger());

	m_propertyDefRole->SaveData(dataWritter);
	m_propertyDefLanguage->SaveData(dataWritter);

	return (*m_propertyModuleConfiguration)->SaveMeta(dataWritter);
}

//***********************************************************************
//*                          common value object                        *
//***********************************************************************

#include "backend/metaData.h"

bool CMetaObjectConfiguration::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!(*m_propertyModuleConfiguration)->OnCreateMetaObject(metaData, flags)) {
		return false;
	}

	return IMetaObject::OnCreateMetaObject(metaData, flags);
}

bool CMetaObjectConfiguration::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyModuleConfiguration)->OnLoadMetaObject(metaData)) {
		return false;
	}

	return IMetaObject::OnLoadMetaObject(metaData);
}

bool CMetaObjectConfiguration::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyModuleConfiguration)->OnSaveMetaObject(flags)) {
		return false;
	}

	if (m_propertyDefLanguage->IsEmptyProperty())
		return false; 

	return IMetaObject::OnSaveMetaObject(flags);
}

bool CMetaObjectConfiguration::OnDeleteMetaObject()
{
	if (!(*m_propertyModuleConfiguration)->OnDeleteMetaObject()) {
		return false;
	}

	return IMetaObject::OnDeleteMetaObject();
}

bool CMetaObjectConfiguration::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyModuleConfiguration)->OnBeforeRunMetaObject(flags))
		return false;

	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!moduleManager->AddCompileModule(m_propertyModuleConfiguration->GetMetaObject(), moduleManager))
		return false;

	return IMetaObject::OnBeforeRunMetaObject(flags);
}

bool CMetaObjectConfiguration::OnAfterCloseMetaObject()
{
	if (!(*m_propertyModuleConfiguration)->OnAfterCloseMetaObject())
		return false;

	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!moduleManager->RemoveCompileModule(m_propertyModuleConfiguration->GetMetaObject()))
		return false;

	return IMetaObject::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CMetaObjectConfiguration, "commonMetadata", g_metaCommonMetadataCLSID);
