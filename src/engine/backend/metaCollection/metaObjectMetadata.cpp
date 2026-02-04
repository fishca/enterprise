////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : metaobject common metaData
////////////////////////////////////////////////////////////////////////////

#include "metaObjectMetadata.h"
#include "metaModuleObject.h"

//*****************************************************************************************
//*                         metaData													  * 
//*****************************************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectConfiguration, IValueMetaObject);

//*****************************************************************************************
//*                                  MetadataObject                                       *
//*****************************************************************************************

#include "backend/metaCollection/metaLanguageObject.h"

wxString CValueMetaObjectConfiguration::GetLangCode() const
{
	const CValueMetaObjectLanguage* language =
		FindAnyObjectByFilter<CValueMetaObjectLanguage>(GetLanguage());

	if (language != nullptr)
		return language->GetLangCode();

	return wxT("");
}

CValueMetaObjectConfiguration::CValueMetaObjectConfiguration() : IValueMetaObject(configurationDefaultName)
{
	//set default proc
	(*m_propertyModuleConfiguration)->SetDefaultProcedure("beforeStart", eContentHelper::eProcedureHelper, { "cancel" });
	(*m_propertyModuleConfiguration)->SetDefaultProcedure("onStart", eContentHelper::eProcedureHelper);
	(*m_propertyModuleConfiguration)->SetDefaultProcedure("beforeExit", eContentHelper::eProcedureHelper, { "cancel" });
	(*m_propertyModuleConfiguration)->SetDefaultProcedure("onExit", eContentHelper::eProcedureHelper);

	//set def metaid
	m_metaId = defaultMetaID;
}

CValueMetaObjectConfiguration::~CValueMetaObjectConfiguration()
{
}

bool CValueMetaObjectConfiguration::LoadData(CMemoryReader& dataReader)
{
	m_propertyVersion->SetValue(dataReader.r_s32());

	m_propertyDefRole->LoadData(dataReader);
	m_propertyDefLanguage->LoadData(dataReader);

	return (*m_propertyModuleConfiguration)->LoadMeta(dataReader);
}

bool CValueMetaObjectConfiguration::SaveData(CMemoryWriter& dataWritter)
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

bool CValueMetaObjectConfiguration::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!(*m_propertyModuleConfiguration)->OnCreateMetaObject(metaData, flags)) {
		return false;
	}

	return IValueMetaObject::OnCreateMetaObject(metaData, flags);
}

bool CValueMetaObjectConfiguration::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyModuleConfiguration)->OnLoadMetaObject(metaData)) {
		return false;
	}

	return IValueMetaObject::OnLoadMetaObject(metaData);
}

bool CValueMetaObjectConfiguration::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyModuleConfiguration)->OnSaveMetaObject(flags)) {
		return false;
	}

	if (m_propertyDefLanguage->IsEmptyProperty()) {
		s_restructureInfo.AppendError(_("! Doesn't have default language ") + GetFullName());
		return false;
	}

	return IValueMetaObject::OnSaveMetaObject(flags);
}

bool CValueMetaObjectConfiguration::OnDeleteMetaObject()
{
	if (!(*m_propertyModuleConfiguration)->OnDeleteMetaObject()) {
		return false;
	}

	return IValueMetaObject::OnDeleteMetaObject();
}

bool CValueMetaObjectConfiguration::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyModuleConfiguration)->OnBeforeRunMetaObject(flags))
		return false;

	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!moduleManager->AddCompileModule(m_propertyModuleConfiguration->GetMetaObject(), moduleManager))
		return false;

	return IValueMetaObject::OnBeforeRunMetaObject(flags);
}

bool CValueMetaObjectConfiguration::OnAfterCloseMetaObject()
{
	if (!(*m_propertyModuleConfiguration)->OnAfterCloseMetaObject())
		return false;

	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!moduleManager->RemoveCompileModule(m_propertyModuleConfiguration->GetMetaObject()))
		return false;

	return IValueMetaObject::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CValueMetaObjectConfiguration, "commonMetadata", g_metaCommonMetadataCLSID);
