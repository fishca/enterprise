////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : dataProcessor - metaData
////////////////////////////////////////////////////////////////////////////

#include "dataProcessor.h"
#include "backend/metaData.h"

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectDataProcessor, IMetaObjectRecordDataExt)
wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectExternalDataProcessor, CMetaObjectDataProcessor)

//********************************************************************************************
//*                                      metaData                                            *
//********************************************************************************************

CMetaObjectDataProcessor::CMetaObjectDataProcessor() : IMetaObjectRecordDataExt()
{
}

CMetaObjectDataProcessor::~CMetaObjectDataProcessor()
{
}

IMetaObjectForm* CMetaObjectDataProcessor::GetDefaultFormByID(const form_identifier_t& id) const
{
	if (id == eFormDataProcessor && m_propertyDefFormObject->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormObject->GetValueAsInteger());
	}

	return nullptr;
}

ISourceDataObject* CMetaObjectDataProcessor::CreateSourceObject(IMetaObjectForm* metaObject)
{
	switch (metaObject->GetTypeForm())
	{
	case eFormDataProcessor:
		return CreateObjectValue(); 
	}

	return nullptr;
}

#include "backend/appData.h"

IRecordDataObjectExt* CMetaObjectDataProcessor::CreateObjectExtValue()
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);
	CRecordDataObjectDataProcessor* pDataRef = nullptr;
	if (appData->DesignerMode()) {
		if (!IsExternalCreate()) {
			if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef))
				return CValue::CreateAndPrepareValueRef<CRecordDataObjectDataProcessor>(this);
		}
		else {
			return dynamic_cast<IRecordDataObjectExt*>(moduleManager->GetObjectValue());
		}
	}
	else {
		if (!IsExternalCreate()) {
			pDataRef = CValue::CreateAndPrepareValueRef<CRecordDataObjectDataProcessor>(this);
		}
		else {
			return dynamic_cast<IRecordDataObjectExt*>(moduleManager->GetObjectValue());
		}
	}

	return pDataRef;
}

#pragma region _form_builder_h_
IBackendValueForm* CMetaObjectDataProcessor::GetObjectForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IMetaObjectGenericData::CreateAndBuildForm(
		strFormName, 
		CMetaObjectDataProcessor::eFormDataProcessor, 
		ownerControl, 
		CreateObjectValue(), 
		formGuid
	);
}
#pragma endregion

bool CMetaObjectDataProcessor::GetFormObject(CPropertyList* prop)
{	
	for (auto formObject : GetFormArrayObject()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormDataProcessor == formObject->GetTypeForm()) {
			prop->AppendItem(
				formObject->GetName(), 
				formObject->GetMetaID(), 
				formObject->GetIcon(),
				formObject);
		}
	}
	return true;
}

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool CMetaObjectDataProcessor::LoadData(CMemoryReader& dataReader)
{
	//Load object module
	(*m_propertyModuleObject)->LoadMeta(dataReader);
	(*m_propertyModuleManager)->LoadMeta(dataReader);

	//Load default form 
	m_propertyDefFormObject->SetValue(GetIdByGuid(dataReader.r_stringZ()));

	return IMetaObjectRecordDataExt::LoadData(dataReader);
}

bool CMetaObjectDataProcessor::SaveData(CMemoryWriter& dataWritter)
{
	//Save object module
	(*m_propertyModuleObject)->SaveMeta(dataWritter);
	(*m_propertyModuleManager)->SaveMeta(dataWritter);

	//Save default form 
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormObject->GetValueAsInteger()));

	return IMetaObjectRecordDataExt::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

#include "backend/appData.h"

bool CMetaObjectDataProcessor::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IMetaObjectRecordDataExt::OnCreateMetaObject(metaData, flags))
		return false;

	return (!IsExternalCreate() ? (*m_propertyModuleManager)->OnCreateMetaObject(metaData, flags) : true) &&
		(*m_propertyModuleObject)->OnCreateMetaObject(metaData, flags);
}

bool CMetaObjectDataProcessor::OnLoadMetaObject(IMetaData* metaData)
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
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

	return IMetaObjectRecordDataExt::OnLoadMetaObject(metaData);
}

bool CMetaObjectDataProcessor::OnSaveMetaObject(int flags)
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnSaveMetaObject(flags))
			return false;
	}

	if (!(*m_propertyModuleObject)->OnSaveMetaObject(flags))
		return false;

	return IMetaObjectRecordDataExt::OnSaveMetaObject(flags);
}

bool CMetaObjectDataProcessor::OnDeleteMetaObject()
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnDeleteMetaObject())
			return false;
	}

	if (!(*m_propertyModuleObject)->OnDeleteMetaObject())
		return false;

	return IMetaObjectRecordDataExt::OnDeleteMetaObject();
}

bool CMetaObjectDataProcessor::OnReloadMetaObject()
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		CRecordDataObjectDataProcessor* pDataRef = nullptr;
		if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
			return true;
		}
		return pDataRef->InitializeObject();
	}

	return true;
}

bool CMetaObjectDataProcessor::OnBeforeRunMetaObject(int flags)
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnBeforeRunMetaObject(flags)) {
			return false;
		}
	}

	if (!(*m_propertyModuleObject)->OnBeforeRunMetaObject(flags)) {
		return false;
	}

	return IMetaObjectRecordDataExt::OnBeforeRunMetaObject(flags);
}

bool CMetaObjectDataProcessor::OnAfterRunMetaObject(int flags)
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnAfterRunMetaObject(flags)) {
			return false;
		}
	}

	if (!(*m_propertyModuleObject)->OnAfterRunMetaObject(flags)) {
		return false;
	}

	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		if (IMetaObjectRecordDataExt::OnAfterRunMetaObject(flags))
			return moduleManager->AddCompileModule(m_propertyModuleObject->GetMetaObject(), CreateObjectValue());
		return false;
	}

	return IMetaObjectRecordDataExt::OnAfterRunMetaObject(flags);
}

bool CMetaObjectDataProcessor::OnBeforeCloseMetaObject()
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnBeforeCloseMetaObject()) {
			return false;
		}
	}

	if (!(*m_propertyModuleObject)->OnBeforeCloseMetaObject()) {
		return false;
	}

	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		if (IMetaObjectRecordDataExt::OnBeforeCloseMetaObject())
			return moduleManager->RemoveCompileModule(m_propertyModuleObject->GetMetaObject());
		return false;
	}

	return IMetaObjectRecordDataExt::OnBeforeCloseMetaObject();
}

bool CMetaObjectDataProcessor::OnAfterCloseMetaObject()
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnAfterCloseMetaObject())
			return false;
	}

	if (!(*m_propertyModuleObject)->OnAfterCloseMetaObject()) {
		return false;
	}

	return IMetaObjectRecordDataExt::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                             form events                             *
//***********************************************************************

void CMetaObjectDataProcessor::OnCreateFormObject(IMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CMetaObjectDataProcessor::eFormDataProcessor
		&& m_propertyDefFormObject->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormObject->SetValue(metaForm->GetMetaID());
	}
}

void CMetaObjectDataProcessor::OnRemoveMetaForm(IMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CMetaObjectDataProcessor::eFormDataProcessor
		&& m_propertyDefFormObject->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormObject->SetValue(metaForm->GetMetaID());
	}
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CMetaObjectDataProcessor, "dataProcessor", g_metaDataProcessorCLSID);
METADATA_TYPE_REGISTER(CMetaObjectExternalDataProcessor, "externalDataProcessor", g_metaExternalDataProcessorCLSID);