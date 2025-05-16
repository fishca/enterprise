////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : report - metaData
////////////////////////////////////////////////////////////////////////////

#include "dataReport.h"
#include "backend/metaData.h"

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectReport, IMetaObjectRecordDataExt)
wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectReportExternal, CMetaObjectReport)

//********************************************************************************************
//*                                      metaData                                            *
//********************************************************************************************

CMetaObjectReport::CMetaObjectReport(int objMode) : IMetaObjectRecordDataExt(objMode)
{
}


CMetaObjectReport::~CMetaObjectReport()
{
}

CMetaObjectForm* CMetaObjectReport::GetDefaultFormByID(const form_identifier_t& id)
{
	if (id == eFormReport
		&& m_propertyDefFormObject->GetValueAsInteger() != wxNOT_FOUND) {
		for (auto& obj : GetObjectForms()) {
			if (m_propertyDefFormObject->GetValueAsInteger() == obj->GetMetaID()) {
				return obj;
			}
		}
	}

	return nullptr;
}

ISourceDataObject* CMetaObjectReport::CreateObjectData(IMetaObjectForm* metaObject)
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
		if (m_objMode == METAOBJECT_NORMAL) {
			if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
				CRecordDataObjectReport* createdObj = m_metaData->CreateAndConvertObjectValueRef<CRecordDataObjectReport>(this);
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
		if (m_objMode == METAOBJECT_NORMAL) {
			pDataRef = m_metaData->CreateAndConvertObjectValueRef<CRecordDataObjectReport>(this);
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

IBackendValueForm* CMetaObjectReport::GetObjectForm(const wxString& formName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	CMetaObjectForm* defList = nullptr;

	if (!formName.IsEmpty()) {
		for (auto metaForm : GetObjectForms()) {
			if (CMetaObjectReport::eFormReport == metaForm->GetTypeForm()
				&& stringUtils::CompareString(formName, metaForm->GetName())) {
				return defList->GenerateFormAndRun(ownerControl,
					CreateObjectValue()
				);
			}
		}
		wxASSERT(defList);
	}
	else {
		defList = GetDefaultFormByID(CMetaObjectReport::eFormReport);
		if (defList) {
			return defList->GenerateFormAndRun(ownerControl,
				CreateObjectValue(), formGuid
			);
		}
	}

	if (defList == nullptr) {
		IRecordDataObject* objectData = CreateObjectValue();
		IBackendValueForm* valueForm = IBackendValueForm::CreateNewForm(ownerControl, nullptr,
			objectData, formGuid);
		valueForm->BuildForm(CMetaObjectReport::eFormReport);
		return valueForm;
	}

	return defList->GenerateFormAndRun();
}

bool CMetaObjectReport::GetFormObject(CPropertyList* prop)
{
	prop->AppendItem(_("<not selected>"), wxNOT_FOUND, wxEmptyValue);

	for (auto formObject : GetObjectForms()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormReport == formObject->GetTypeForm()) {
			prop->AppendItem(formObject->GetName(), formObject->GetMetaID(), formObject);
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

	return IMetaObjectRecordData::LoadData(dataReader);
}

bool CMetaObjectReport::SaveData(CMemoryWriter& dataWritter)
{
	//Save object module
	m_propertyModuleObject->SaveData(dataWritter);
	m_propertyModuleManager->SaveData(dataWritter);

	//Save default form 
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormObject->GetValueAsInteger()));

	return IMetaObjectRecordData::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool CMetaObjectReport::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IMetaObjectRecordData::OnCreateMetaObject(metaData, flags))
		return false;

	return (m_objMode == METAOBJECT_NORMAL ? m_propertyModuleManager->GetMetaObject()->OnCreateMetaObject(metaData, flags) : true) &&
		m_propertyModuleObject->GetMetaObject()->OnCreateMetaObject(metaData, flags);
}

bool CMetaObjectReport::OnLoadMetaObject(IMetaData* metaData)
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (m_objMode == METAOBJECT_NORMAL) {

		if (!m_propertyModuleManager->GetMetaObject()->OnLoadMetaObject(metaData))
			return false;

		m_propertyModuleObject->GetMetaObject()->SetMetaData(m_metaData);
	
		if (!m_propertyModuleObject->GetMetaObject()->OnLoadMetaObject(metaData))
			return false;
	}
	else {

		m_propertyModuleObject->GetMetaObject()->SetMetaData(m_metaData);

		if (!m_propertyModuleObject->GetMetaObject()->OnLoadMetaObject(metaData))
			return false;
	}

	return IMetaObjectRecordData::OnLoadMetaObject(metaData);
}

bool CMetaObjectReport::OnSaveMetaObject()
{
	if (m_objMode == METAOBJECT_NORMAL) {
		if (!m_propertyModuleManager->GetMetaObject()->OnSaveMetaObject())
			return false;
	}

	if (!m_propertyModuleObject->GetMetaObject()->OnSaveMetaObject())
		return false;

	return IMetaObjectRecordData::OnSaveMetaObject();
}

bool CMetaObjectReport::OnDeleteMetaObject()
{
	if (m_objMode == METAOBJECT_NORMAL) {
		if (!m_propertyModuleManager->GetMetaObject()->OnDeleteMetaObject())
			return false;
	}

	if (!m_propertyModuleObject->GetMetaObject()->OnDeleteMetaObject())
		return false;

	return IMetaObjectRecordData::OnDeleteMetaObject();
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
	if (m_objMode == METAOBJECT_NORMAL) {
		if (!m_propertyModuleManager->GetMetaObject()->OnBeforeRunMetaObject(flags)) {
			return false;
		}
	}

	if (!m_propertyModuleObject->GetMetaObject()->OnBeforeRunMetaObject(flags)) {
		return false;
	}

	return IMetaObjectRecordData::OnBeforeRunMetaObject(flags);
}

bool CMetaObjectReport::OnAfterRunMetaObject(int flags)
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		if (IMetaObjectRecordData::OnAfterRunMetaObject(flags))
			return moduleManager->AddCompileModule(m_propertyModuleObject->GetMetaObject(), CreateObjectValue());
		return false;
	}

	return IMetaObjectRecordData::OnAfterRunMetaObject(flags);
}

bool CMetaObjectReport::OnBeforeCloseMetaObject()
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		if (IMetaObjectRecordData::OnBeforeCloseMetaObject())
			return moduleManager->RemoveCompileModule(m_propertyModuleObject->GetMetaObject());
		return false;
	}

	return IMetaObjectRecordData::OnBeforeCloseMetaObject();
}

bool CMetaObjectReport::OnAfterCloseMetaObject()
{
	if (m_objMode == METAOBJECT_NORMAL) {
		if (!m_propertyModuleManager->GetMetaObject()->OnAfterCloseMetaObject())
			return false;
	}

	if (!m_propertyModuleObject->GetMetaObject()->OnAfterCloseMetaObject()) {
		return false;
	}

	return IMetaObjectRecordData::OnAfterCloseMetaObject();
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
METADATA_TYPE_REGISTER(CMetaObjectReportExternal, "externalReport", g_metaExternalReportCLSID);