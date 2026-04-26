////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : report - metaData
////////////////////////////////////////////////////////////////////////////

#include "dataReport.h"
#include "backend/metaData.h"
#include "backend/session/session.h"

wxIMPLEMENT_DYNAMIC_CLASS(ibValueMetaObjectReport, ibValueMetaObjectRecordDataExt)
wxIMPLEMENT_DYNAMIC_CLASS(ibValueMetaObjectExternalReport, ibValueMetaObjectReport)

//********************************************************************************************
//*                                      metaData                                            *
//********************************************************************************************

ibValueMetaObjectReport::ibValueMetaObjectReport() : ibValueMetaObjectRecordDataExt()
{
}

ibValueMetaObjectReport::~ibValueMetaObjectReport()
{
}

ibValueMetaObjectFormBase* ibValueMetaObjectReport::GetDefaultFormByID(const ibFormID& id) const
{
	if (id == eFormReport && m_propertyDefFormObject->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormObject->GetValueAsInteger());
	}

	return nullptr;
}

#include "dataReportManager.h"

ibValueManagerDataObject* ibValueMetaObjectReport::CreateManagerDataObjectValue()
{
	return ibValue::CreateAndPrepareValueRef<ibValueManagerDataObjectReport>(this);
}

#include "backend/appData.h"

ibValueRecordDataObjectExt* ibValueMetaObjectReport::CreateObjectExtValue()
{
	ibSession* session = ibSession::Current();
	ibValueModuleManager* moduleManager = session ? session->GetModuleManager() : nullptr;
	wxASSERT(moduleManager);
	ibValueRecordDataObjectReport* pDataRef = nullptr;
	if (auto* cc = m_metaData->GetCompileCache()) {
		if (!IsExternalCreate()) {
			if (!cc->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
				ibValueRecordDataObjectReport* createdObj = ibValue::CreateAndPrepareValueRef<ibValueRecordDataObjectReport>(this);
				if (!createdObj->InitializeObject()) {
					wxDELETE(createdObj);
					return nullptr;
				}
				return createdObj;
			}
		}
		else {
			return dynamic_cast<ibValueRecordDataObjectExt*>(moduleManager->GetObjectValue());
		}
	}
	else {
		if (!IsExternalCreate()) {
			pDataRef = ibValue::CreateAndPrepareValueRef<ibValueRecordDataObjectReport>(this);
			if (!pDataRef->InitializeObject()) {
				wxDELETE(pDataRef);
				return nullptr;
			}
		}
		else {
			return dynamic_cast<ibValueRecordDataObjectExt*>(moduleManager->GetObjectValue());
		}
	}

	return pDataRef;
}

ibSourceDataObject* ibValueMetaObjectReport::CreateSourceObject(ibValueMetaObjectFormBase* metaObject)
{
	switch (metaObject->GetTypeForm())
	{
	case eFormReport:
		return CreateObjectValue();
	}

	return nullptr;
}

#pragma region _form_builder_h_
ibBackendValueForm* ibValueMetaObjectReport::GetObjectForm(const wxString& strFormName, ibBackendControlFrame* ownerControl, const ibUniqueKey& formGuid)
{
	return ibValueMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		ibValueMetaObjectReport::eFormReport,
		ownerControl, CreateObjectValue(),
		formGuid
	);
}
#pragma endregion

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool ibValueMetaObjectReport::LoadData(ibReaderMemory& dataReader)
{
	//Load object module
	m_propertyModuleObject->LoadData(dataReader);
	m_propertyModuleManager->LoadData(dataReader);

	//Load default form 
	m_propertyDefFormObject->SetValue(GetIdByGuid(dataReader.r_stringZ()));

	return ibValueMetaObjectRecordDataExt::LoadData(dataReader);
}

bool ibValueMetaObjectReport::SaveData(ibWriterMemory& dataWritter)
{
	//Save object module
	m_propertyModuleObject->SaveData(dataWritter);
	m_propertyModuleManager->SaveData(dataWritter);

	//Save default form 
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormObject->GetValueAsInteger()));

	return ibValueMetaObjectRecordDataExt::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool ibValueMetaObjectReport::OnCreateMetaObject(ibMetaData* metaData, int flags)
{
	if (!ibValueMetaObjectRecordDataExt::OnCreateMetaObject(metaData, flags))
		return false;

	return (!IsExternalCreate() ? (*m_propertyModuleManager)->OnCreateMetaObject(metaData, flags) : true) &&
		(*m_propertyModuleObject)->OnCreateMetaObject(metaData, flags);
}

bool ibValueMetaObjectReport::OnLoadMetaObject(ibMetaData* metaData)
{
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

	return ibValueMetaObjectRecordDataExt::OnLoadMetaObject(metaData);
}

bool ibValueMetaObjectReport::OnSaveMetaObject(int flags)
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnSaveMetaObject(flags))
			return false;
	}

	if (!(*m_propertyModuleObject)->OnSaveMetaObject(flags))
		return false;

	return ibValueMetaObjectRecordDataExt::OnSaveMetaObject(flags);
}

bool ibValueMetaObjectReport::OnDeleteMetaObject()
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnDeleteMetaObject())
			return false;
	}

	if (!(*m_propertyModuleObject)->OnDeleteMetaObject())
		return false;

	return ibValueMetaObjectRecordDataExt::OnDeleteMetaObject();
}

bool ibValueMetaObjectReport::OnReloadMetaObject()
{
	if (auto* cc = m_metaData->GetCompileCache()) {
		ibValueRecordDataObjectReport* pDataRef = nullptr;
		if (!cc->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
			return true;
		}
		return pDataRef->InitializeObject();
	}

	return true;
}

bool ibValueMetaObjectReport::OnBeforeRunMetaObject(int flags)
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

bool ibValueMetaObjectReport::OnAfterRunMetaObject(int flags)
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnAfterRunMetaObject(flags)) {
			return false;
		}
	}

	if (!(*m_propertyModuleObject)->OnAfterRunMetaObject(flags)) {
		return false;
	}

	if (auto* cc = m_metaData->GetCompileCache()) {
		if (ibValueMetaObjectRecordDataExt::OnAfterRunMetaObject(flags))
			return cc->AddCompileModule(m_propertyModuleObject->GetMetaObject(), CreateObjectValue());
		return false;
	}

	return ibValueMetaObjectRecordDataExt::OnAfterRunMetaObject(flags);
}

bool ibValueMetaObjectReport::OnBeforeCloseMetaObject()
{
	if (!IsExternalCreate()) {
		if (!(*m_propertyModuleManager)->OnBeforeCloseMetaObject())
			return false;
	}

	if (!(*m_propertyModuleObject)->OnBeforeCloseMetaObject()) {
		return false;
	}

	if (auto* cc = m_metaData->GetCompileCache()) {
		if (ibValueMetaObjectRecordDataExt::OnBeforeCloseMetaObject())
			return cc->RemoveCompileModule(m_propertyModuleObject->GetMetaObject());
		return false;
	}

	return ibValueMetaObjectRecordDataExt::OnBeforeCloseMetaObject();
}

bool ibValueMetaObjectReport::OnAfterCloseMetaObject()
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

void ibValueMetaObjectReport::OnCreateFormObject(ibValueMetaObjectFormBase* metaForm)
{
	if (metaForm->GetTypeForm() == ibValueMetaObjectReport::eFormReport
		&& m_propertyDefFormObject->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormObject->SetValue(metaForm->GetMetaID());
	}
}

void ibValueMetaObjectReport::OnRemoveMetaForm(ibValueMetaObjectFormBase* metaForm)
{
	if (metaForm->GetTypeForm() == ibValueMetaObjectReport::eFormReport
		&& m_propertyDefFormObject->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormObject->SetValue(metaForm->GetMetaID());
	}
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(ibValueMetaObjectReport, "Report", g_metaReportCLSID);
METADATA_TYPE_REGISTER(ibValueMetaObjectExternalReport, "ExternalReport", g_metaExternalReportCLSID);