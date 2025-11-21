#include "informationRegister.h"
#include "list/objectList.h"
#include "backend/metaData.h"
#include "backend/moduleManager/moduleManager.h"

//***********************************************************************
//*                         metaData                                    * 
//***********************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectInformationRegister::CMetaObjectRecordManager, IMetaObject);
wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectInformationRegister, IMetaObjectRegisterData);

/////////////////////////////////////////////////////////////////////////

CMetaObjectInformationRegister::CMetaObjectInformationRegister() : IMetaObjectRegisterData(),
m_metaRecordManager(new CMetaObjectRecordManager())
{
	//set default proc
	(*m_propertyModuleObject)->SetDefaultProcedure("beforeWrite", eContentHelper::eProcedureHelper, { "cancel" });
	(*m_propertyModuleObject)->SetDefaultProcedure("onWrite", eContentHelper::eProcedureHelper, { "cancel" });
}

CMetaObjectInformationRegister::~CMetaObjectInformationRegister()
{
	wxDELETE(m_metaRecordManager);
}

IMetaObjectForm* CMetaObjectInformationRegister::GetDefaultFormByID(const form_identifier_t& id)
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
IBackendValueForm* CMetaObjectInformationRegister::GetRecordForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniquePairKey& formGuid)
{
	return IMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CMetaObjectInformationRegister::eFormRecord,
		ownerControl, CreateRecordManagerObjectValue(),
		formGuid
	);
}

IBackendValueForm* CMetaObjectInformationRegister::GetListForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CMetaObjectInformationRegister::eFormList,
		ownerControl, CValue::CreateAndPrepareValueRef<CListRegisterObject>(this, CMetaObjectInformationRegister::eFormList),
		formGuid
	);
}
#pragma endregion

/////////////////////////////////////////////////////////////////////////////

bool CMetaObjectInformationRegister::GetFormRecord(CPropertyList* prop)
{
	prop->AppendItem(wxT("notSelected"), _("<not selected>"), wxNOT_FOUND);
	for (auto formObject : GetFormArrayObject()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormRecord == formObject->GetTypeForm()) {
			prop->AppendItem(formObject->GetName(), formObject->GetMetaID(), formObject);
		}
	}
	return true;
}

bool CMetaObjectInformationRegister::GetFormList(CPropertyList* prop)
{
	prop->AppendItem(wxT("notSelected"), _("<not selected>"), wxNOT_FOUND);
	for (auto formObject : GetFormArrayObject()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormList == formObject->GetTypeForm()) {
			prop->AppendItem(formObject->GetName(), formObject->GetMetaID(), formObject);
		}
	}
	return true;
}

//***************************************************************************
//*								form record								    *
//***************************************************************************

IBackendValueForm* CMetaObjectInformationRegister::GetRecordForm(const meta_identifier_t& id, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	const IMetaObjectForm* defList = FindFormObjectByFilter(id);

	return GetListForm(defList ? defList->GetName() : wxEmptyString,
		ownerControl, formGuid
	);
}

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool CMetaObjectInformationRegister::LoadData(CMemoryReader& dataReader)
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

	return IMetaObjectRegisterData::LoadData(dataReader);
}

bool CMetaObjectInformationRegister::SaveData(CMemoryWriter& dataWritter)
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
	return IMetaObjectRegisterData::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

#include "backend/appData.h"

bool CMetaObjectInformationRegister::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IMetaObjectRegisterData::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyModuleManager)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyModuleObject)->OnCreateMetaObject(metaData, flags);
}

bool CMetaObjectInformationRegister::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyModuleManager)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyModuleObject)->OnLoadMetaObject(metaData))
		return false;

	return IMetaObjectRegisterData::OnLoadMetaObject(metaData);
}

bool CMetaObjectInformationRegister::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyModuleManager)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnSaveMetaObject(flags))
		return false;

#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
	if (GetWriteRegisterMode() == eWriteRegisterMode::eSubordinateRecorder) {
		if (!((*m_propertyAttributeRecorder)->GetClsidCount() > 0)) {
			s_restructureInfo.AppendError(_("! Doesn't have any recorder ") + GetFullName());
			return false;
		}
	}
#endif

	return IMetaObjectRegisterData::OnSaveMetaObject(flags);
}

bool CMetaObjectInformationRegister::OnDeleteMetaObject()
{
	if (!(*m_propertyModuleManager)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnDeleteMetaObject())
		return false;

	return IMetaObjectRegisterData::OnDeleteMetaObject();
}

bool CMetaObjectInformationRegister::OnReloadMetaObject()
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		CRecordSetObjectInformationRegister* recordSet = nullptr;
		if (moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), recordSet)) {
			if (!recordSet->InitializeObject())
				return false;
		}

		CRecordManagerObjectInformationRegister* recordManager = nullptr;
		if (moduleManager->FindCompileModule(m_metaRecordManager, recordManager)) {
			if (!recordManager->InitializeObject())
				return false;
		}
	}

	return true;
}

#include "backend/objCtor.h"

bool CMetaObjectInformationRegister::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyModuleManager)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnBeforeRunMetaObject(flags))
		return false;

	registerSelection();

	return IMetaObjectRegisterData::OnBeforeRunMetaObject(flags);
}

bool CMetaObjectInformationRegister::OnAfterRunMetaObject(int flags)
{
	if (!(*m_propertyModuleManager)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnAfterRunMetaObject(flags))
		return false;

	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		if (IMetaObjectRegisterData::OnAfterRunMetaObject(flags)) {

			if (!moduleManager->AddCompileModule(m_metaRecordManager, CreateRecordManagerObjectValue()))
				return false;

			if (!moduleManager->AddCompileModule(m_propertyModuleObject->GetMetaObject(), CreateRecordSetObjectValue()))
				return false;

			return true;
		}
	}

	return IMetaObjectRegisterData::OnAfterRunMetaObject(flags);
}

bool CMetaObjectInformationRegister::OnBeforeCloseMetaObject()
{
	if (!(*m_propertyModuleManager)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnBeforeCloseMetaObject())
		return false;

	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		if (IMetaObjectRegisterData::OnBeforeCloseMetaObject()) {

			if (!moduleManager->RemoveCompileModule(m_metaRecordManager))
				return false;

			if (!moduleManager->RemoveCompileModule(m_propertyModuleObject->GetMetaObject()))
				return false;

			return true;
		}
	}

	return IMetaObjectRegisterData::OnBeforeCloseMetaObject();
}

bool CMetaObjectInformationRegister::OnAfterCloseMetaObject()
{
	if (!(*m_propertyModuleManager)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnAfterCloseMetaObject())
		return false;

	unregisterSelection();

	return IMetaObjectRegisterData::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                             form events                             *
//***********************************************************************

void CMetaObjectInformationRegister::OnCreateFormObject(IMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CMetaObjectInformationRegister::eFormRecord
		&& m_propertyDefFormRecord->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormRecord->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CMetaObjectInformationRegister::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
}

void CMetaObjectInformationRegister::OnRemoveMetaForm(IMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CMetaObjectInformationRegister::eFormRecord
		&& m_propertyDefFormRecord->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormRecord->SetValue(wxNOT_FOUND);
	}
	else if (metaForm->GetTypeForm() == CMetaObjectInformationRegister::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
}

ISourceDataObject* CMetaObjectInformationRegister::CreateSourceObject(IMetaObjectForm* metaObject)
{
	switch (metaObject->GetTypeForm())
	{
	case eFormRecord:
		return CreateRecordManagerObjectValue();
	case eFormList:
		return CValue::CreateAndPrepareValueRef<CListRegisterObject>(this, metaObject->GetTypeForm());
	}

	return nullptr;
}

IRecordSetObject* CMetaObjectInformationRegister::CreateRecordSetObjectRegValue(const CUniquePairKey& uniqueKey)
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		IRecordSetObject* pDataRef = nullptr;
		if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
			return CValue::CreateAndPrepareValueRef<CRecordSetObjectInformationRegister>(this, uniqueKey);
		}
		return pDataRef;
	}

	return CValue::CreateAndPrepareValueRef<CRecordSetObjectInformationRegister>(this, uniqueKey);
}

IRecordManagerObject* CMetaObjectInformationRegister::CreateRecordManagerObjectRegValue(const CUniquePairKey& uniqueKey)
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		IRecordManagerObject* pDataRef = nullptr;
		if (!moduleManager->FindCompileModule(m_metaRecordManager, pDataRef)) {
			return CValue::CreateAndPrepareValueRef<CRecordManagerObjectInformationRegister>(this, uniqueKey);
		}
		return pDataRef;
	}
	return CValue::CreateAndPrepareValueRef<CRecordManagerObjectInformationRegister>(this, uniqueKey);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

SYSTEM_TYPE_REGISTER(CMetaObjectInformationRegister::CMetaObjectRecordManager, "recordManager", string_to_clsid("MT_RCMG"));
METADATA_TYPE_REGISTER(CMetaObjectInformationRegister, "informationRegister", g_metaInformationRegisterCLSID);