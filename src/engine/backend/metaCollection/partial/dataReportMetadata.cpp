////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : report - metaData
////////////////////////////////////////////////////////////////////////////

#include "dataReport.h"
#include "backend/metaData.h"

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectReport, IMetaObjectRecordDataExt)
wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectExternalReport, CMetaObjectReport)

//********************************************************************************************
//*                                      metaData                                            *
//********************************************************************************************

CMetaObjectReport::CMetaObjectReport() : IMetaObjectRecordDataExt()
{
}

CMetaObjectReport::~CMetaObjectReport()
{
}

IMetaObjectForm* CMetaObjectReport::GetDefaultFormByID(const form_identifier_t& id) const
{
	if (id == eFormReport && m_propertyDefFormObject->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormObject->GetValueAsInteger());
	}

	return nullptr;
}

ISourceDataObject* CMetaObjectReport::CreateSourceObject(IMetaObjectForm* metaObject)
{
	switch (metaObject->GetTypeForm())
	{
	case eFormReport:
		return CreateObjectValue();
	}

	return nullptr;
}

#include "backend/appData.h"

IRecordDataObjectExt* CMetaObjectReport::CreateObjectExtValue()
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);
	CRecordDataObjectReport* pDataRef = nullptr;
	if (appData->DesignerMode()) {
		if (!IsExternalCreate()) {
			if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
				CRecordDataObjectReport* createdObj = CValue::CreateAndPrepareValueRef<CRecordDataObjectReport>(this);
				if (!createdObj->InitializeObject()) {
					wxDELETE(createdObj);
					return nullptr;
				}
				return createdObj;
			}
		}
		else {
			return dynamic_cast<IRecordDataObjectExt*>(moduleManager->GetObjectValue());
		}
	}
	else {
		if (!IsExternalCreate()) {
			pDataRef = CValue::CreateAndPrepareValueRef<CRecordDataObjectReport>(this);
			if (!pDataRef->InitializeObject()) {
				wxDELETE(pDataRef);
				return nullptr;
			}
		}
		else {
			return dynamic_cast<IRecordDataObjectExt*>(moduleManager->GetObjectValue());
		}
	}

	return pDataRef;
}

#pragma region _form_builder_h_
IBackendValueForm* CMetaObjectReport::GetObjectForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CMetaObjectReport::eFormReport,
		ownerControl, CreateObjectValue(),
		formGuid
	);
}
#pragma endregion

bool CMetaObjectReport::GetFormObject(CPropertyList* prop)
{
	for (auto formObject : GetFormArrayObject()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormReport == formObject->GetTypeForm()) {
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

bool CMetaObjectReport::LoadData(CMemoryReader& dataReader)
{
	//Load object module
	m_propertyModuleObject->LoadData(dataReader);
	m_propertyModuleManager->LoadData(dataReader);

	//Load default form 
	m_propertyDefFormObject->SetValue(GetIdByGuid(dataReader.r_stringZ()));

	return IMetaObjectRecordDataExt::LoadData(dataReader);
}

bool CMetaObjectReport::SaveData(CMemoryWriter& dataWritter)
{
	//Save object module
	m_propertyModuleObject->SaveData(dataWritter);
	m_propertyModuleManager->SaveData(dataWritter);

	//Save default form 
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormObject->GetValueAsInteger()));

	return IMetaObjectRecordDataExt::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool CMetaObjectReport::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IMetaObjectRecordDataExt::OnCreateMetaObject(metaData, flags))
		return false;

	return (!IsExternalCreate() ? (*m_propertyModuleManager)->OnCreateMetaObject(metaData, flags) : true) &&
		(*m_propertyModuleObject)->OnCreateMetaObject(metaData, flags);
}

bool CMetaObjectReport::OnLoadMetaObject(IMetaData* metaData)
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!IsExternalCreate()) {

		if (!(*m_propertyModuleManager)->OnLoadMetaObject(metaData))
			return false;

		(*m_propertyModuleObject)->SetMetaData(m_metaData);
	
		if (!(*m_propertyModuleObject)->OnLoadMetaObject(metaData))
			return false;
	}
	else {

		(*m_propertyModuleObject)->SetMetaData(m_metaData);

		if (!(*m_propertyModuleObject)->OnLoadMetaObject(metaData))
			return false;
	}

	return IMetaObjectRecordDataExt::OnLoadMetaObject(metaData);
}

bool CMetaObjectReport::OnSaveMetaObject(int flags)
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnSaveMetaObject(flags))
			return false;
	}

	if (!(*m_propertyModuleObject)->OnSaveMetaObject(flags))
		return false;

	return IMetaObjectRecordDataExt::OnSaveMetaObject(flags);
}

bool CMetaObjectReport::OnDeleteMetaObject()
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnDeleteMetaObject())
			return false;
	}

	if (!(*m_propertyModuleObject)->OnDeleteMetaObject())
		return false;

	return IMetaObjectRecordDataExt::OnDeleteMetaObject();
}

bool CMetaObjectReport::OnReloadMetaObject()
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		CRecordDataObjectReport* pDataRef = nullptr;
		if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
			return true;
		}
		return pDataRef->InitializeObject();
	}

	return true;
}

bool CMetaObjectReport::OnBeforeRunMetaObject(int flags)
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

bool CMetaObjectReport::OnAfterRunMetaObject(int flags)
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

bool CMetaObjectReport::OnBeforeCloseMetaObject()
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnBeforeCloseMetaObject())
			return false;
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

bool CMetaObjectReport::OnAfterCloseMetaObject()
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

void CMetaObjectReport::OnCreateFormObject(IMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CMetaObjectReport::eFormReport
		&& m_propertyDefFormObject->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormObject->SetValue(metaForm->GetMetaID());
	}
}

void CMetaObjectReport::OnRemoveMetaForm(IMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CMetaObjectReport::eFormReport
		&& m_propertyDefFormObject->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormObject->SetValue(metaForm->GetMetaID());
	}
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CMetaObjectReport, "report", g_metaReportCLSID);
METADATA_TYPE_REGISTER(CMetaObjectExternalReport, "externalReport", g_metaExternalReportCLSID);