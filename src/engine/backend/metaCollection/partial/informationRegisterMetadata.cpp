#include "informationRegister.h"
#include "list/objectList.h"
#include "backend/metaData.h"
#include "backend/moduleManager/moduleManager.h"

//***********************************************************************
//*                         metaData                                    * 
//***********************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectInformationRegister::CValueMetaObjectRecordManager, IValueMetaObject);
wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectInformationRegister, IValueMetaObjectRegisterData);

/////////////////////////////////////////////////////////////////////////

CValueMetaObjectInformationRegister::CValueMetaObjectInformationRegister() : IValueMetaObjectRegisterData(),
m_metaRecordManager(new CValueMetaObjectRecordManager())
{
	//set default proc
	(*m_propertyModuleObject)->SetDefaultProcedure(wxT("BeforeWrite"), eContentHelper::eProcedureHelper, { wxT("Cancel") });
	(*m_propertyModuleObject)->SetDefaultProcedure(wxT("OnWrite"), eContentHelper::eProcedureHelper, { wxT("Cancel") });
}

CValueMetaObjectInformationRegister::~CValueMetaObjectInformationRegister()
{
	wxDELETE(m_metaRecordManager);
}

IValueMetaObjectForm* CValueMetaObjectInformationRegister::GetDefaultFormByID(const form_identifier_t& id) const
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
IBackendValueForm* CValueMetaObjectInformationRegister::GetRecordForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IValueMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CValueMetaObjectInformationRegister::eFormRecord,
		ownerControl, CreateRecordManagerObjectValue(),
		formGuid
	);
}

IBackendValueForm* CValueMetaObjectInformationRegister::GetListForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IValueMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CValueMetaObjectInformationRegister::eFormList,
		ownerControl, CValue::CreateAndPrepareValueRef<CValueListRegisterObject>(this, CValueMetaObjectInformationRegister::eFormList),
		formGuid
	);
}
#pragma endregion

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool CValueMetaObjectInformationRegister::LoadData(CMemoryReader& dataReader)
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

	return IValueMetaObjectRegisterData::LoadData(dataReader);
}

bool CValueMetaObjectInformationRegister::SaveData(CMemoryWriter& dataWritter)
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
	return IValueMetaObjectRegisterData::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

#include "backend/appData.h"

bool CValueMetaObjectInformationRegister::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IValueMetaObjectRegisterData::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyModuleManager)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyModuleObject)->OnCreateMetaObject(metaData, flags);
}

bool CValueMetaObjectInformationRegister::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyModuleManager)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyModuleObject)->OnLoadMetaObject(metaData))
		return false;

	return IValueMetaObjectRegisterData::OnLoadMetaObject(metaData);
}

bool CValueMetaObjectInformationRegister::OnSaveMetaObject(int flags)
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

	return IValueMetaObjectRegisterData::OnSaveMetaObject(flags);
}

bool CValueMetaObjectInformationRegister::OnDeleteMetaObject()
{
	if (!(*m_propertyModuleManager)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnDeleteMetaObject())
		return false;

	return IValueMetaObjectRegisterData::OnDeleteMetaObject();
}

bool CValueMetaObjectInformationRegister::OnReloadMetaObject()
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		CValueRecordSetObjectInformationRegister* recordSet = nullptr;
		if (moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), recordSet)) {
			if (!recordSet->InitializeObject())
				return false;
		}

		CValueRecordManagerObjectInformationRegister* recordManager = nullptr;
		if (moduleManager->FindCompileModule(m_metaRecordManager, recordManager)) {
			if (!recordManager->InitializeObject())
				return false;
		}
	}

	return true;
}

#include "backend/objCtor.h"

bool CValueMetaObjectInformationRegister::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyModuleManager)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnBeforeRunMetaObject(flags))
		return false;

	registerSelection();

	return IValueMetaObjectRegisterData::OnBeforeRunMetaObject(flags);
}

bool CValueMetaObjectInformationRegister::OnAfterRunMetaObject(int flags)
{
	if (!(*m_propertyModuleManager)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnAfterRunMetaObject(flags))
		return false;

	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		if (IValueMetaObjectRegisterData::OnAfterRunMetaObject(flags)) {

			if (!moduleManager->AddCompileModule(m_metaRecordManager, CreateRecordManagerObjectValue()))
				return false;

			if (!moduleManager->AddCompileModule(m_propertyModuleObject->GetMetaObject(), CreateRecordSetObjectValue()))
				return false;

			return true;
		}
	}

	return IValueMetaObjectRegisterData::OnAfterRunMetaObject(flags);
}

bool CValueMetaObjectInformationRegister::OnBeforeCloseMetaObject()
{
	if (!(*m_propertyModuleManager)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnBeforeCloseMetaObject())
		return false;

	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		if (IValueMetaObjectRegisterData::OnBeforeCloseMetaObject()) {

			if (!moduleManager->RemoveCompileModule(m_metaRecordManager))
				return false;

			if (!moduleManager->RemoveCompileModule(m_propertyModuleObject->GetMetaObject()))
				return false;

			return true;
		}
	}

	return IValueMetaObjectRegisterData::OnBeforeCloseMetaObject();
}

bool CValueMetaObjectInformationRegister::OnAfterCloseMetaObject()
{
	if (!(*m_propertyModuleManager)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnAfterCloseMetaObject())
		return false;

	unregisterSelection();

	return IValueMetaObjectRegisterData::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                             form events                             *
//***********************************************************************

void CValueMetaObjectInformationRegister::OnCreateFormObject(IValueMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CValueMetaObjectInformationRegister::eFormRecord
		&& m_propertyDefFormRecord->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormRecord->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CValueMetaObjectInformationRegister::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
}

void CValueMetaObjectInformationRegister::OnRemoveMetaForm(IValueMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CValueMetaObjectInformationRegister::eFormRecord
		&& m_propertyDefFormRecord->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormRecord->SetValue(wxNOT_FOUND);
	}
	else if (metaForm->GetTypeForm() == CValueMetaObjectInformationRegister::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
}

#include "informationRegisterManager.h"

IValueManagerDataObject* CValueMetaObjectInformationRegister::CreateManagerDataObjectValue()
{
	return CValue::CreateAndPrepareValueRef<CValueManagerDataObjectInformationRegister>(this);
}

IValueRecordSetObject* CValueMetaObjectInformationRegister::CreateRecordSetObjectRegValue(const CUniquePairKey& uniqueKey)
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		IValueRecordSetObject* pDataRef = nullptr;
		if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
			return CValue::CreateAndPrepareValueRef<CValueRecordSetObjectInformationRegister>(this, uniqueKey);
		}
		return pDataRef;
	}

	return CValue::CreateAndPrepareValueRef<CValueRecordSetObjectInformationRegister>(this, uniqueKey);
}

IValueRecordManagerObject* CValueMetaObjectInformationRegister::CreateRecordManagerObjectRegValue(const CUniquePairKey& uniqueKey)
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		IValueRecordManagerObject* pDataRef = nullptr;
		if (!moduleManager->FindCompileModule(m_metaRecordManager, pDataRef)) {
			return CValue::CreateAndPrepareValueRef<CValueRecordManagerObjectInformationRegister>(this, uniqueKey);
		}
		return pDataRef;
	}
	return CValue::CreateAndPrepareValueRef<CValueRecordManagerObjectInformationRegister>(this, uniqueKey);
}

ISourceDataObject* CValueMetaObjectInformationRegister::CreateSourceObject(IValueMetaObjectForm* metaObject)
{
	switch (metaObject->GetTypeForm())
	{
	case eFormRecord:
		return CreateRecordManagerObjectValue();
	case eFormList:
		return CValue::CreateAndPrepareValueRef<CValueListRegisterObject>(this, metaObject->GetTypeForm());
	}

	return nullptr;
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

SYSTEM_TYPE_REGISTER(CValueMetaObjectInformationRegister::CValueMetaObjectRecordManager, "InformationRecordManager", string_to_clsid("MT_RCMG"));
METADATA_TYPE_REGISTER(CValueMetaObjectInformationRegister, "InformationRegister", g_metaInformationRegisterCLSID);