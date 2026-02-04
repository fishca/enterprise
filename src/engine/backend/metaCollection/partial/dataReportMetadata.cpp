////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : report - metaData
////////////////////////////////////////////////////////////////////////////

#include "dataReport.h"
#include "backend/metaData.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectReport, IValueMetaObjectRecordDataExt)
wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectExternalReport, CValueMetaObjectReport)

//********************************************************************************************
//*                                      metaData                                            *
//********************************************************************************************

CValueMetaObjectReport::CValueMetaObjectReport() : IValueMetaObjectRecordDataExt()
{
}

CValueMetaObjectReport::~CValueMetaObjectReport()
{
}

IValueMetaObjectForm* CValueMetaObjectReport::GetDefaultFormByID(const form_identifier_t& id) const
{
	if (id == eFormReport && m_propertyDefFormObject->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormObject->GetValueAsInteger());
	}

	return nullptr;
}

#include "dataReportManager.h"

IValueManagerDataObject* CValueMetaObjectReport::CreateManagerDataObjectValue()
{
	return CValue::CreateAndPrepareValueRef<CValueManagerDataObjectReport>(this);
}

#include "backend/appData.h"

IValueRecordDataObjectExt* CValueMetaObjectReport::CreateObjectExtValue()
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);
	CValueRecordDataObjectReport* pDataRef = nullptr;
	if (appData->DesignerMode()) {
		if (!IsExternalCreate()) {
			if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
				CValueRecordDataObjectReport* createdObj = CValue::CreateAndPrepareValueRef<CValueRecordDataObjectReport>(this);
				if (!createdObj->InitializeObject()) {
					wxDELETE(createdObj);
					return nullptr;
				}
				return createdObj;
			}
		}
		else {
			return dynamic_cast<IValueRecordDataObjectExt*>(moduleManager->GetObjectValue());
		}
	}
	else {
		if (!IsExternalCreate()) {
			pDataRef = CValue::CreateAndPrepareValueRef<CValueRecordDataObjectReport>(this);
			if (!pDataRef->InitializeObject()) {
				wxDELETE(pDataRef);
				return nullptr;
			}
		}
		else {
			return dynamic_cast<IValueRecordDataObjectExt*>(moduleManager->GetObjectValue());
		}
	}

	return pDataRef;
}

ISourceDataObject* CValueMetaObjectReport::CreateSourceObject(IValueMetaObjectForm* metaObject)
{
	switch (metaObject->GetTypeForm())
	{
	case eFormReport:
		return CreateObjectValue();
	}

	return nullptr;
}

#pragma region _form_builder_h_
IBackendValueForm* CValueMetaObjectReport::GetObjectForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IValueMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CValueMetaObjectReport::eFormReport,
		ownerControl, CreateObjectValue(),
		formGuid
	);
}
#pragma endregion

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool CValueMetaObjectReport::LoadData(CMemoryReader& dataReader)
{
	//Load object module
	m_propertyModuleObject->LoadData(dataReader);
	m_propertyModuleManager->LoadData(dataReader);

	//Load default form 
	m_propertyDefFormObject->SetValue(GetIdByGuid(dataReader.r_stringZ()));

	return IValueMetaObjectRecordDataExt::LoadData(dataReader);
}

bool CValueMetaObjectReport::SaveData(CMemoryWriter& dataWritter)
{
	//Save object module
	m_propertyModuleObject->SaveData(dataWritter);
	m_propertyModuleManager->SaveData(dataWritter);

	//Save default form 
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormObject->GetValueAsInteger()));

	return IValueMetaObjectRecordDataExt::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool CValueMetaObjectReport::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IValueMetaObjectRecordDataExt::OnCreateMetaObject(metaData, flags))
		return false;

	return (!IsExternalCreate() ? (*m_propertyModuleManager)->OnCreateMetaObject(metaData, flags) : true) &&
		(*m_propertyModuleObject)->OnCreateMetaObject(metaData, flags);
}

bool CValueMetaObjectReport::OnLoadMetaObject(IMetaData* metaData)
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
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

	return IValueMetaObjectRecordDataExt::OnLoadMetaObject(metaData);
}

bool CValueMetaObjectReport::OnSaveMetaObject(int flags)
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnSaveMetaObject(flags))
			return false;
	}

	if (!(*m_propertyModuleObject)->OnSaveMetaObject(flags))
		return false;

	return IValueMetaObjectRecordDataExt::OnSaveMetaObject(flags);
}

bool CValueMetaObjectReport::OnDeleteMetaObject()
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnDeleteMetaObject())
			return false;
	}

	if (!(*m_propertyModuleObject)->OnDeleteMetaObject())
		return false;

	return IValueMetaObjectRecordDataExt::OnDeleteMetaObject();
}

bool CValueMetaObjectReport::OnReloadMetaObject()
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		CValueRecordDataObjectReport* pDataRef = nullptr;
		if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
			return true;
		}
		return pDataRef->InitializeObject();
	}

	return true;
}

bool CValueMetaObjectReport::OnBeforeRunMetaObject(int flags)
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

bool CValueMetaObjectReport::OnAfterRunMetaObject(int flags)
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

bool CValueMetaObjectReport::OnBeforeCloseMetaObject()
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnBeforeCloseMetaObject())
			return false;
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

bool CValueMetaObjectReport::OnAfterCloseMetaObject()
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

void CValueMetaObjectReport::OnCreateFormObject(IValueMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CValueMetaObjectReport::eFormReport
		&& m_propertyDefFormObject->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormObject->SetValue(metaForm->GetMetaID());
	}
}

void CValueMetaObjectReport::OnRemoveMetaForm(IValueMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CValueMetaObjectReport::eFormReport
		&& m_propertyDefFormObject->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormObject->SetValue(metaForm->GetMetaID());
	}
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CValueMetaObjectReport, "report", g_metaReportCLSID);
METADATA_TYPE_REGISTER(CValueMetaObjectExternalReport, "externalReport", g_metaExternalReportCLSID);