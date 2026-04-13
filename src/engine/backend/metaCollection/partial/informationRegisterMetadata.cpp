#include "informationRegister.h"
#include "list/objectList.h"
#include "backend/metaData.h"
#include "backend/moduleManager/moduleManager.h"

//***********************************************************************
//*                         metaData                                    * 
//***********************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(ibValueMetaObjectInformationRegister::ibValueMetaObjectRecordManager, ibValueMetaObject);
wxIMPLEMENT_DYNAMIC_CLASS(ibValueMetaObjectInformationRegister, ibValueMetaObjectRegisterData);

/////////////////////////////////////////////////////////////////////////

ibValueMetaObjectInformationRegister::ibValueMetaObjectInformationRegister() : ibValueMetaObjectRegisterData(),
m_metaRecordManager(new ibValueMetaObjectRecordManager())
{
	//set default proc
	(*m_propertyModuleObject)->SetDefaultProcedure(wxT("BeforeWrite"), ibContentHelper::eProcedureHelper, { wxT("Cancel") });
	(*m_propertyModuleObject)->SetDefaultProcedure(wxT("OnWrite"), ibContentHelper::eProcedureHelper, { wxT("Cancel") });
}

ibValueMetaObjectInformationRegister::~ibValueMetaObjectInformationRegister()
{
	wxDELETE(m_metaRecordManager);
}

ibValueMetaObjectFormBase* ibValueMetaObjectInformationRegister::GetDefaultFormByID(const ibFormID& id) const
{
	if (id == eFormRecord && m_propertyDefFormRecord->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormRecord->GetValueAsInteger());
	}
	else if (id == eFormList && m_propertyDefFormList->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormList->GetValueAsInteger());
	}

	return nullptr;
}

#pragma region _form_builder_h_
ibBackendValueForm* ibValueMetaObjectInformationRegister::GetRecordForm(const wxString& strFormName, ibBackendControlFrame* ownerControl, const ibUniqueKey& formGuid)
{
	return ibValueMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		ibValueMetaObjectInformationRegister::eFormRecord,
		ownerControl, CreateRecordManagerObjectValue(),
		formGuid
	);
}

ibBackendValueForm* ibValueMetaObjectInformationRegister::GetListForm(const wxString& strFormName, ibBackendControlFrame* ownerControl, const ibUniqueKey& formGuid)
{
	return ibValueMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		ibValueMetaObjectInformationRegister::eFormList,
		ownerControl, ibValue::CreateAndPrepareValueRef<ibValueListRegisterObject>(this, ibValueMetaObjectInformationRegister::eFormList),
		formGuid
	);
}
#pragma endregion

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool ibValueMetaObjectInformationRegister::LoadData(ibReaderMemory& dataReader)
{
	//load default form 
	m_propertyDefFormRecord->SetValue(GetIdByGuid(dataReader.r_stringZ()));
	m_propertyDefFormList->SetValue(GetIdByGuid(dataReader.r_stringZ()));

	//load data 
	m_propertyWriteMode->SetValue(dataReader.r_u16());
	m_propertyPeriodicity->SetValue(dataReader.r_u16());

	//load object module
	(*m_propertyModuleObject)->LoadMeta(dataReader);
	(*m_propertyModuleManager)->LoadMeta(dataReader);

	return ibValueMetaObjectRegisterData::LoadData(dataReader);
}

bool ibValueMetaObjectInformationRegister::SaveData(ibWriterMemory& dataWritter)
{
	//save default form 
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormRecord->GetValueAsInteger()));
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormList->GetValueAsInteger()));

	//save data
	dataWritter.w_u16(m_propertyWriteMode->GetValueAsInteger());
	dataWritter.w_u16(m_propertyPeriodicity->GetValueAsInteger());

	//Save object module
	(*m_propertyModuleObject)->SaveMeta(dataWritter);
	(*m_propertyModuleManager)->SaveMeta(dataWritter);

	//create or update table:
	return ibValueMetaObjectRegisterData::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

#include "backend/appData.h"

bool ibValueMetaObjectInformationRegister::OnCreateMetaObject(ibMetaData* metaData, int flags)
{
	if (!ibValueMetaObjectRegisterData::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyModuleManager)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyModuleObject)->OnCreateMetaObject(metaData, flags);
}

bool ibValueMetaObjectInformationRegister::OnLoadMetaObject(ibMetaData* metaData)
{
	if (!(*m_propertyModuleManager)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyModuleObject)->OnLoadMetaObject(metaData))
		return false;

	return ibValueMetaObjectRegisterData::OnLoadMetaObject(metaData);
}

bool ibValueMetaObjectInformationRegister::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyModuleManager)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnSaveMetaObject(flags))
		return false;

#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
	if (GetWriteRegisterMode() == ibWriteRegisterMode::eSubordinateRecorder) {
		if (!((*m_propertyAttributeRecorder)->GetClsidCount() > 0)) {
			s_restructureInfo.AppendError(_("! Doesn't have any recorder ") + GetFullName());
			return false;
		}
	}
#endif

	return ibValueMetaObjectRegisterData::OnSaveMetaObject(flags);
}

bool ibValueMetaObjectInformationRegister::OnDeleteMetaObject()
{
	if (!(*m_propertyModuleManager)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnDeleteMetaObject())
		return false;

	return ibValueMetaObjectRegisterData::OnDeleteMetaObject();
}

bool ibValueMetaObjectInformationRegister::OnReloadMetaObject()
{
	ibValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		ibValueRecordSetObjectInformationRegister* recordSet = nullptr;
		if (moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), recordSet)) {
			if (!recordSet->InitializeObject())
				return false;
		}

		ibValueRecordManagerObjectInformationRegister* recordManager = nullptr;
		if (moduleManager->FindCompileModule(m_metaRecordManager, recordManager)) {
			if (!recordManager->InitializeObject())
				return false;
		}
	}

	return true;
}

#include "backend/objCtor.h"

bool ibValueMetaObjectInformationRegister::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyModuleManager)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnBeforeRunMetaObject(flags))
		return false;

	registerSelection();

	return ibValueMetaObjectRegisterData::OnBeforeRunMetaObject(flags);
}

bool ibValueMetaObjectInformationRegister::OnAfterRunMetaObject(int flags)
{
	if (!(*m_propertyModuleManager)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnAfterRunMetaObject(flags))
		return false;

	ibValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		if (ibValueMetaObjectRegisterData::OnAfterRunMetaObject(flags)) {

			if (!moduleManager->AddCompileModule(m_metaRecordManager, CreateRecordManagerObjectValue()))
				return false;

			if (!moduleManager->AddCompileModule(m_propertyModuleObject->GetMetaObject(), CreateRecordSetObjectValue()))
				return false;

			return true;
		}
	}

	return ibValueMetaObjectRegisterData::OnAfterRunMetaObject(flags);
}

bool ibValueMetaObjectInformationRegister::OnBeforeCloseMetaObject()
{
	if (!(*m_propertyModuleManager)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnBeforeCloseMetaObject())
		return false;

	ibValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		if (ibValueMetaObjectRegisterData::OnBeforeCloseMetaObject()) {

			if (!moduleManager->RemoveCompileModule(m_metaRecordManager))
				return false;

			if (!moduleManager->RemoveCompileModule(m_propertyModuleObject->GetMetaObject()))
				return false;

			return true;
		}
	}

	return ibValueMetaObjectRegisterData::OnBeforeCloseMetaObject();
}

bool ibValueMetaObjectInformationRegister::OnAfterCloseMetaObject()
{
	if (!(*m_propertyModuleManager)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnAfterCloseMetaObject())
		return false;

	unregisterSelection();

	return ibValueMetaObjectRegisterData::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                             form events                             *
//***********************************************************************

void ibValueMetaObjectInformationRegister::OnCreateFormObject(ibValueMetaObjectFormBase* metaForm)
{
	if (metaForm->GetTypeForm() == ibValueMetaObjectInformationRegister::eFormRecord
		&& m_propertyDefFormRecord->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormRecord->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == ibValueMetaObjectInformationRegister::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
}

void ibValueMetaObjectInformationRegister::OnRemoveMetaForm(ibValueMetaObjectFormBase* metaForm)
{
	if (metaForm->GetTypeForm() == ibValueMetaObjectInformationRegister::eFormRecord
		&& m_propertyDefFormRecord->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormRecord->SetValue(wxNOT_FOUND);
	}
	else if (metaForm->GetTypeForm() == ibValueMetaObjectInformationRegister::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
}

#include "informationRegisterManager.h"

ibValueManagerDataObject* ibValueMetaObjectInformationRegister::CreateManagerDataObjectValue()
{
	return ibValue::CreateAndPrepareValueRef<ibValueManagerDataObjectInformationRegister>(this);
}

ibValueRecordSetObject* ibValueMetaObjectInformationRegister::CreateRecordSetObjectRegValue(const ibUniqueKeyPair& uniqueKey)
{
	ibValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		ibValueRecordSetObject* pDataRef = nullptr;
		if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
			return ibValue::CreateAndPrepareValueRef<ibValueRecordSetObjectInformationRegister>(this, uniqueKey);
		}
		return pDataRef;
	}

	return ibValue::CreateAndPrepareValueRef<ibValueRecordSetObjectInformationRegister>(this, uniqueKey);
}

ibValueRecordManagerObject* ibValueMetaObjectInformationRegister::CreateRecordManagerObjectRegValue(const ibUniqueKeyPair& uniqueKey)
{
	ibValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		ibValueRecordManagerObject* pDataRef = nullptr;
		if (!moduleManager->FindCompileModule(m_metaRecordManager, pDataRef)) {
			return ibValue::CreateAndPrepareValueRef<ibValueRecordManagerObjectInformationRegister>(this, uniqueKey);
		}
		return pDataRef;
	}
	return ibValue::CreateAndPrepareValueRef<ibValueRecordManagerObjectInformationRegister>(this, uniqueKey);
}

ibSourceDataObject* ibValueMetaObjectInformationRegister::CreateSourceObject(ibValueMetaObjectFormBase* metaObject)
{
	switch (metaObject->GetTypeForm())
	{
	case eFormRecord:
		return CreateRecordManagerObjectValue();
	case eFormList:
		return ibValue::CreateAndPrepareValueRef<ibValueListRegisterObject>(this, metaObject->GetTypeForm());
	}

	return nullptr;
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

SYSTEM_TYPE_REGISTER(ibValueMetaObjectInformationRegister::ibValueMetaObjectRecordManager, "InformationRecordManager", string_to_clsid("MT_RCMG"));
METADATA_TYPE_REGISTER(ibValueMetaObjectInformationRegister, "InformationRegister", g_metaInformationRegisterCLSID);