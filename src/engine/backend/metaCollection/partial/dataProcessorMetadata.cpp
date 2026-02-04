////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : dataProcessor - metaData
////////////////////////////////////////////////////////////////////////////

#include "dataProcessor.h"
#include "backend/metaData.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectDataProcessor, IValueMetaObjectRecordDataExt)
wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectExternalDataProcessor, CValueMetaObjectDataProcessor)

//********************************************************************************************
//*                                      metaData                                            *
//********************************************************************************************

CValueMetaObjectDataProcessor::CValueMetaObjectDataProcessor() : IValueMetaObjectRecordDataExt()
{
}

CValueMetaObjectDataProcessor::~CValueMetaObjectDataProcessor()
{
}

IValueMetaObjectForm* CValueMetaObjectDataProcessor::GetDefaultFormByID(const form_identifier_t& id) const
{
	if (id == eFormDataProcessor && m_propertyDefFormObject->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormObject->GetValueAsInteger());
	}

	return nullptr;
}

#include "dataProcessorManager.h"

IValueManagerDataObject* CValueMetaObjectDataProcessor::CreateManagerDataObjectValue()
{
	return CValue::CreateAndPrepareValueRef<CValueManagerDataObjectDataProcessor>(this);
}

#include "backend/appData.h"

IValueRecordDataObjectExt* CValueMetaObjectDataProcessor::CreateObjectExtValue()
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);
	CValueRecordDataObjectDataProcessor* pDataRef = nullptr;
	if (appData->DesignerMode()) {
		if (!IsExternalCreate()) {
			if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef))
				return CValue::CreateAndPrepareValueRef<CValueRecordDataObjectDataProcessor>(this);
		}
		else {
			return dynamic_cast<IValueRecordDataObjectExt*>(moduleManager->GetObjectValue());
		}
	}
	else {
		if (!IsExternalCreate()) {
			pDataRef = CValue::CreateAndPrepareValueRef<CValueRecordDataObjectDataProcessor>(this);
		}
		else {
			return dynamic_cast<IValueRecordDataObjectExt*>(moduleManager->GetObjectValue());
		}
	}

	return pDataRef;
}

ISourceDataObject* CValueMetaObjectDataProcessor::CreateSourceObject(IValueMetaObjectForm* metaObject)
{
	switch (metaObject->GetTypeForm())
	{
	case eFormDataProcessor:
		return CreateObjectValue();
	}

	return nullptr;
}

#pragma region _form_builder_h_
IBackendValueForm* CValueMetaObjectDataProcessor::GetObjectForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IValueMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CValueMetaObjectDataProcessor::eFormDataProcessor,
		ownerControl,
		CreateObjectValue(),
		formGuid
	);
}
#pragma endregion

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool CValueMetaObjectDataProcessor::LoadData(CMemoryReader& dataReader)
{
	//Load object module
	(*m_propertyModuleObject)->LoadMeta(dataReader);
	(*m_propertyModuleManager)->LoadMeta(dataReader);

	//Load default form 
	m_propertyDefFormObject->SetValue(GetIdByGuid(dataReader.r_stringZ()));

	return IValueMetaObjectRecordDataExt::LoadData(dataReader);
}

bool CValueMetaObjectDataProcessor::SaveData(CMemoryWriter& dataWritter)
{
	//Save object module
	(*m_propertyModuleObject)->SaveMeta(dataWritter);
	(*m_propertyModuleManager)->SaveMeta(dataWritter);

	//Save default form 
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormObject->GetValueAsInteger()));

	return IValueMetaObjectRecordDataExt::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

#include "backend/appData.h"

bool CValueMetaObjectDataProcessor::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IValueMetaObjectRecordDataExt::OnCreateMetaObject(metaData, flags))
		return false;

	return (!IsExternalCreate() ? (*m_propertyModuleManager)->OnCreateMetaObject(metaData, flags) : true) &&
		(*m_propertyModuleObject)->OnCreateMetaObject(metaData, flags);
}

bool CValueMetaObjectDataProcessor::OnLoadMetaObject(IMetaData* metaData)
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!IsExternalCreate()) {

		if (!(*m_propertyModuleManager)->OnLoadMetaObject(metaData))
			return false;

		if (!(*m_propertyModuleObject)->OnLoadMetaObject(metaData))
			return false;
	}
	else {

		if (!(*m_propertyModuleObject)->OnLoadMetaObject(metaData))
			return false;
	}

	return IValueMetaObjectRecordDataExt::OnLoadMetaObject(metaData);
}

bool CValueMetaObjectDataProcessor::OnSaveMetaObject(int flags)
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnSaveMetaObject(flags))
			return false;
	}

	if (!(*m_propertyModuleObject)->OnSaveMetaObject(flags))
		return false;

	return IValueMetaObjectRecordDataExt::OnSaveMetaObject(flags);
}

bool CValueMetaObjectDataProcessor::OnDeleteMetaObject()
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnDeleteMetaObject())
			return false;
	}

	if (!(*m_propertyModuleObject)->OnDeleteMetaObject())
		return false;

	return IValueMetaObjectRecordDataExt::OnDeleteMetaObject();
}

bool CValueMetaObjectDataProcessor::OnReloadMetaObject()
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		CValueRecordDataObjectDataProcessor* pDataRef = nullptr;
		if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
			return true;
		}
		return pDataRef->InitializeObject();
	}

	return true;
}

bool CValueMetaObjectDataProcessor::OnBeforeRunMetaObject(int flags)
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnBeforeRunMetaObject(flags)) {
			return false;
		}
	}

	if (!(*m_propertyModuleObject)->OnBeforeRunMetaObject(flags)) {
		return false;
	}

	return IValueMetaObjectRecordDataExt::OnBeforeRunMetaObject(flags);
}

bool CValueMetaObjectDataProcessor::OnAfterRunMetaObject(int flags)
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnAfterRunMetaObject(flags)) {
			return false;
		}
	}

	if (!(*m_propertyModuleObject)->OnAfterRunMetaObject(flags)) {
		return false;
	}

	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		if (IValueMetaObjectRecordDataExt::OnAfterRunMetaObject(flags))
			return moduleManager->AddCompileModule(m_propertyModuleObject->GetMetaObject(), CreateObjectValue());
		return false;
	}

	return IValueMetaObjectRecordDataExt::OnAfterRunMetaObject(flags);
}

bool CValueMetaObjectDataProcessor::OnBeforeCloseMetaObject()
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnBeforeCloseMetaObject()) {
			return false;
		}
	}

	if (!(*m_propertyModuleObject)->OnBeforeCloseMetaObject()) {
		return false;
	}

	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		if (IValueMetaObjectRecordDataExt::OnBeforeCloseMetaObject())
			return moduleManager->RemoveCompileModule(m_propertyModuleObject->GetMetaObject());
		return false;
	}

	return IValueMetaObjectRecordDataExt::OnBeforeCloseMetaObject();
}

bool CValueMetaObjectDataProcessor::OnAfterCloseMetaObject()
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnAfterCloseMetaObject())
			return false;
	}

	if (!(*m_propertyModuleObject)->OnAfterCloseMetaObject()) {
		return false;
	}

	return IValueMetaObjectRecordDataExt::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                             form events                             *
//***********************************************************************

void CValueMetaObjectDataProcessor::OnCreateFormObject(IValueMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CValueMetaObjectDataProcessor::eFormDataProcessor
		&& m_propertyDefFormObject->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormObject->SetValue(metaForm->GetMetaID());
	}
}

void CValueMetaObjectDataProcessor::OnRemoveMetaForm(IValueMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CValueMetaObjectDataProcessor::eFormDataProcessor
		&& m_propertyDefFormObject->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormObject->SetValue(metaForm->GetMetaID());
	}
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CValueMetaObjectDataProcessor, "dataProcessor", g_metaDataProcessorCLSID);
METADATA_TYPE_REGISTER(CValueMetaObjectExternalDataProcessor, "externalDataProcessor", g_metaExternalDataProcessorCLSID);