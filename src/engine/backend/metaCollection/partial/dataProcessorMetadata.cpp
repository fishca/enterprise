////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : dataProcessor - metaData
////////////////////////////////////////////////////////////////////////////

#include "dataProcessor.h"
#include "backend/metaData.h"

wxIMPLEMENT_DYNAMIC_CLASS(ibValueMetaObjectDataProcessor, ibValueMetaObjectRecordDataExt)
wxIMPLEMENT_DYNAMIC_CLASS(ibValueMetaObjectExternalDataProcessor, ibValueMetaObjectDataProcessor)

//********************************************************************************************
//*                                      metaData                                            *
//********************************************************************************************

ibValueMetaObjectDataProcessor::ibValueMetaObjectDataProcessor() : ibValueMetaObjectRecordDataExt()
{
}

ibValueMetaObjectDataProcessor::~ibValueMetaObjectDataProcessor()
{
}

ibValueMetaObjectFormBase* ibValueMetaObjectDataProcessor::GetDefaultFormByID(const ibFormID& id) const
{
	if (id == eFormDataProcessor && m_propertyDefFormObject->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormObject->GetValueAsInteger());
	}

	return nullptr;
}

#include "dataProcessorManager.h"

ibValueManagerDataObject* ibValueMetaObjectDataProcessor::CreateManagerDataObjectValue()
{
	return ibValue::CreateAndPrepareValueRef<ibValueManagerDataObjectDataProcessor>(this);
}

#include "backend/appData.h"

ibValueRecordDataObjectExt* ibValueMetaObjectDataProcessor::CreateObjectExtValue()
{
	ibValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);
	ibValueRecordDataObjectDataProcessor* pDataRef = nullptr;
	if (appData->DesignerMode()) {
		if (!IsExternalCreate()) {
			if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef))
				return ibValue::CreateAndPrepareValueRef<ibValueRecordDataObjectDataProcessor>(this);
		}
		else {
			return dynamic_cast<ibValueRecordDataObjectExt*>(moduleManager->GetObjectValue());
		}
	}
	else {
		if (!IsExternalCreate()) {
			pDataRef = ibValue::CreateAndPrepareValueRef<ibValueRecordDataObjectDataProcessor>(this);
		}
		else {
			return dynamic_cast<ibValueRecordDataObjectExt*>(moduleManager->GetObjectValue());
		}
	}

	return pDataRef;
}

ibSourceDataObject* ibValueMetaObjectDataProcessor::CreateSourceObject(ibValueMetaObjectFormBase* metaObject)
{
	switch (metaObject->GetTypeForm())
	{
	case eFormDataProcessor:
		return CreateObjectValue();
	}

	return nullptr;
}

#pragma region _form_builder_h_
ibBackendValueForm* ibValueMetaObjectDataProcessor::GetObjectForm(const wxString& strFormName, ibBackendControlFrame* ownerControl, const ibUniqueKey& formGuid)
{
	return ibValueMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		ibValueMetaObjectDataProcessor::eFormDataProcessor,
		ownerControl,
		CreateObjectValue(),
		formGuid
	);
}
#pragma endregion

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool ibValueMetaObjectDataProcessor::LoadData(ibReaderMemory& dataReader)
{
	//Load object module
	(*m_propertyModuleObject)->LoadMeta(dataReader);
	(*m_propertyModuleManager)->LoadMeta(dataReader);

	//Load default form 
	m_propertyDefFormObject->SetValue(GetIdByGuid(dataReader.r_stringZ()));

	return ibValueMetaObjectRecordDataExt::LoadData(dataReader);
}

bool ibValueMetaObjectDataProcessor::SaveData(ibWriterMemory& dataWritter)
{
	//Save object module
	(*m_propertyModuleObject)->SaveMeta(dataWritter);
	(*m_propertyModuleManager)->SaveMeta(dataWritter);

	//Save default form 
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormObject->GetValueAsInteger()));

	return ibValueMetaObjectRecordDataExt::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

#include "backend/appData.h"

bool ibValueMetaObjectDataProcessor::OnCreateMetaObject(ibMetaData* metaData, int flags)
{
	if (!ibValueMetaObjectRecordDataExt::OnCreateMetaObject(metaData, flags))
		return false;

	return (!IsExternalCreate() ? (*m_propertyModuleManager)->OnCreateMetaObject(metaData, flags) : true) &&
		(*m_propertyModuleObject)->OnCreateMetaObject(metaData, flags);
}

bool ibValueMetaObjectDataProcessor::OnLoadMetaObject(ibMetaData* metaData)
{
	ibValueModuleManager* moduleManager = m_metaData->GetModuleManager();
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

	return ibValueMetaObjectRecordDataExt::OnLoadMetaObject(metaData);
}

bool ibValueMetaObjectDataProcessor::OnSaveMetaObject(int flags)
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnSaveMetaObject(flags))
			return false;
	}

	if (!(*m_propertyModuleObject)->OnSaveMetaObject(flags))
		return false;

	return ibValueMetaObjectRecordDataExt::OnSaveMetaObject(flags);
}

bool ibValueMetaObjectDataProcessor::OnDeleteMetaObject()
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnDeleteMetaObject())
			return false;
	}

	if (!(*m_propertyModuleObject)->OnDeleteMetaObject())
		return false;

	return ibValueMetaObjectRecordDataExt::OnDeleteMetaObject();
}

bool ibValueMetaObjectDataProcessor::OnReloadMetaObject()
{
	ibValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		ibValueRecordDataObjectDataProcessor* pDataRef = nullptr;
		if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
			return true;
		}
		return pDataRef->InitializeObject();
	}

	return true;
}

bool ibValueMetaObjectDataProcessor::OnBeforeRunMetaObject(int flags)
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnBeforeRunMetaObject(flags)) {
			return false;
		}
	}

	if (!(*m_propertyModuleObject)->OnBeforeRunMetaObject(flags)) {
		return false;
	}

	return ibValueMetaObjectRecordDataExt::OnBeforeRunMetaObject(flags);
}

bool ibValueMetaObjectDataProcessor::OnAfterRunMetaObject(int flags)
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnAfterRunMetaObject(flags)) {
			return false;
		}
	}

	if (!(*m_propertyModuleObject)->OnAfterRunMetaObject(flags)) {
		return false;
	}

	ibValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		if (ibValueMetaObjectRecordDataExt::OnAfterRunMetaObject(flags))
			return moduleManager->AddCompileModule(m_propertyModuleObject->GetMetaObject(), CreateObjectValue());
		return false;
	}

	return ibValueMetaObjectRecordDataExt::OnAfterRunMetaObject(flags);
}

bool ibValueMetaObjectDataProcessor::OnBeforeCloseMetaObject()
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnBeforeCloseMetaObject()) {
			return false;
		}
	}

	if (!(*m_propertyModuleObject)->OnBeforeCloseMetaObject()) {
		return false;
	}

	ibValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		if (ibValueMetaObjectRecordDataExt::OnBeforeCloseMetaObject())
			return moduleManager->RemoveCompileModule(m_propertyModuleObject->GetMetaObject());
		return false;
	}

	return ibValueMetaObjectRecordDataExt::OnBeforeCloseMetaObject();
}

bool ibValueMetaObjectDataProcessor::OnAfterCloseMetaObject()
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnAfterCloseMetaObject())
			return false;
	}

	if (!(*m_propertyModuleObject)->OnAfterCloseMetaObject()) {
		return false;
	}

	return ibValueMetaObjectRecordDataExt::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                             form events                             *
//***********************************************************************

void ibValueMetaObjectDataProcessor::OnCreateFormObject(ibValueMetaObjectFormBase* metaForm)
{
	if (metaForm->GetTypeForm() == ibValueMetaObjectDataProcessor::eFormDataProcessor
		&& m_propertyDefFormObject->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormObject->SetValue(metaForm->GetMetaID());
	}
}

void ibValueMetaObjectDataProcessor::OnRemoveMetaForm(ibValueMetaObjectFormBase* metaForm)
{
	if (metaForm->GetTypeForm() == ibValueMetaObjectDataProcessor::eFormDataProcessor
		&& m_propertyDefFormObject->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormObject->SetValue(metaForm->GetMetaID());
	}
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(ibValueMetaObjectDataProcessor, "DataProcessor", g_metaDataProcessorCLSID);
METADATA_TYPE_REGISTER(ibValueMetaObjectExternalDataProcessor, "ExternalDataProcessor", g_metaExternalDataProcessorCLSID);