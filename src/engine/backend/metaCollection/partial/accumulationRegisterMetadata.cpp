#include "accumulationRegister.h"
#include "list/objectList.h"
#include "backend/metadataConfiguration.h"
#include "backend/moduleManager/moduleManager.h"

//***********************************************************************
//*                         metaData                                    * 
//***********************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectAccumulationRegister, IValueMetaObjectRegisterData);

/////////////////////////////////////////////////////////////////////////

CValueMetaObjectAccumulationRegister::CValueMetaObjectAccumulationRegister() : IValueMetaObjectRegisterData()
{
	//set default proc
	(*m_propertyModuleObject)->SetDefaultProcedure("beforeWrite", eContentHelper::eProcedureHelper, { "cancel" });
	(*m_propertyModuleObject)->SetDefaultProcedure("onWrite", eContentHelper::eProcedureHelper, { "cancel" });
}

CValueMetaObjectAccumulationRegister::~CValueMetaObjectAccumulationRegister()
{
	//wxDELETE((*m_propertyAttributeRecordType));
}

IValueMetaObjectForm* CValueMetaObjectAccumulationRegister::GetDefaultFormByID(const form_identifier_t& id) const 
{
	if (id == eFormList && m_propertyDefFormList->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormList->GetValueAsInteger());
	}

	return nullptr;
}

#pragma region _form_builder_h_
IBackendValueForm* CValueMetaObjectAccumulationRegister::GetListForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IValueMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CValueMetaObjectAccumulationRegister::eFormList,
		ownerControl, CValue::CreateAndPrepareValueRef<CValueListRegisterObject>(this, CValueMetaObjectAccumulationRegister::eFormList),
		formGuid
	);
}
#pragma endregion

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool CValueMetaObjectAccumulationRegister::LoadData(CMemoryReader& dataReader)
{
	//load default attributes:
	(*m_propertyAttributeRecordType)->LoadMeta(dataReader);

	//load default form 
	m_propertyDefFormList->SetValue(GetIdByGuid(dataReader.r_stringZ()));

	//load data 
	m_propertyRegisterType->SetValue(dataReader.r_u16());

	//load object module
	(*m_propertyModuleObject)->LoadMeta(dataReader);
	(*m_propertyModuleManager)->LoadMeta(dataReader);

	return IValueMetaObjectRegisterData::LoadData(dataReader);
}

bool CValueMetaObjectAccumulationRegister::SaveData(CMemoryWriter& dataWritter)
{
	//save default attributes:
	(*m_propertyAttributeRecordType)->SaveMeta(dataWritter);

	//save default form 
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormList->GetValueAsInteger()));

	//save data
	dataWritter.w_u16(m_propertyRegisterType->GetValueAsInteger());

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

bool CValueMetaObjectAccumulationRegister::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IValueMetaObjectRegisterData::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyAttributeRecordType)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyModuleManager)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyModuleObject)->OnCreateMetaObject(metaData, flags);
}

bool CValueMetaObjectAccumulationRegister::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyAttributeRecordType)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyModuleManager)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyModuleObject)->OnLoadMetaObject(metaData))
		return false;

	return IValueMetaObjectRegisterData::OnLoadMetaObject(metaData);
}

bool CValueMetaObjectAccumulationRegister::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyAttributeRecordType)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyModuleManager)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnSaveMetaObject(flags))
		return false;

#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
	if (!((*m_propertyAttributeRecorder)->GetClsidCount() > 0)) {
		s_restructureInfo.AppendError(_("! Doesn't have any recorder ") + GetFullName());
		return false;
	}
#endif 

	return IValueMetaObjectRegisterData::OnSaveMetaObject(flags);
}

bool CValueMetaObjectAccumulationRegister::OnDeleteMetaObject()
{
	if (!(*m_propertyAttributeRecordType)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyModuleManager)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnDeleteMetaObject())
		return false;

	return IValueMetaObjectRegisterData::OnDeleteMetaObject();
}

bool CValueMetaObjectAccumulationRegister::OnReloadMetaObject()
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		CValueRecordSetObjectAccumulationRegister* recordSet = nullptr;
		if (moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), recordSet)) {
			if (!recordSet->InitializeObject())
				return false;
		}
	}

	return true;
}

#include "backend/objCtor.h"

bool CValueMetaObjectAccumulationRegister::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyAttributeRecordType)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleManager)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnBeforeRunMetaObject(flags))
		return false;

	registerSelection();

	return IValueMetaObjectRegisterData::OnBeforeRunMetaObject(flags);
}

bool CValueMetaObjectAccumulationRegister::OnAfterRunMetaObject(int flags)
{
	if (!(*m_propertyAttributeRecordType)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleManager)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnAfterRunMetaObject(flags))
		return false;

	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		if (IValueMetaObjectRegisterData::OnAfterRunMetaObject(flags)) {

			if (!moduleManager->AddCompileModule(m_propertyModuleObject->GetMetaObject(), CreateRecordSetObjectValue()))
				return false;

			return true;
		}
	}

	return IValueMetaObjectRegisterData::OnAfterRunMetaObject(flags);
}

bool CValueMetaObjectAccumulationRegister::OnBeforeCloseMetaObject()
{
	if (!(*m_propertyAttributeRecordType)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyModuleManager)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnBeforeCloseMetaObject())
		return false;

	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		if (IValueMetaObjectRegisterData::OnBeforeCloseMetaObject()) {

			if (!moduleManager->RemoveCompileModule(m_propertyModuleObject->GetMetaObject()))
				return false;

			return true;
		}
	}

	return IValueMetaObjectRegisterData::OnBeforeCloseMetaObject();
}

bool CValueMetaObjectAccumulationRegister::OnAfterCloseMetaObject()
{
	if (!(*m_propertyAttributeRecordType)->OnAfterCloseMetaObject())
		return false;

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

void CValueMetaObjectAccumulationRegister::OnCreateFormObject(IValueMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CValueMetaObjectAccumulationRegister::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
}

void CValueMetaObjectAccumulationRegister::OnRemoveMetaForm(IValueMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CValueMetaObjectAccumulationRegister::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
}
#include "accumulationRegisterManager.h"

IValueManagerDataObject* CValueMetaObjectAccumulationRegister::CreateManagerDataObjectValue()
{
	return CValue::CreateAndPrepareValueRef<CValueManagerDataObjectAccumulationRegister>(this);
}

IValueRecordSetObject* CValueMetaObjectAccumulationRegister::CreateRecordSetObjectRegValue(const CUniquePairKey& uniqueKey)
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);
	if (appData->DesignerMode()) {
		IValueRecordSetObject* pDataRef = nullptr;
		if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
			return CValue::CreateAndPrepareValueRef<CValueRecordSetObjectAccumulationRegister>(this, uniqueKey);
		}
		return pDataRef;
	}
	return CValue::CreateAndPrepareValueRef<CValueRecordSetObjectAccumulationRegister>(this, uniqueKey);
}

ISourceDataObject* CValueMetaObjectAccumulationRegister::CreateSourceObject(IValueMetaObjectForm* metaObject)
{
	switch (metaObject->GetTypeForm())
	{
	case eFormList:
		return CValue::CreateAndPrepareValueRef<CValueListRegisterObject>(this, metaObject->GetTypeForm());
	}

	return nullptr;
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CValueMetaObjectAccumulationRegister, "accumulationRegister", g_metaAccumulationRegisterCLSID);