#include "accumulationRegister.h"
#include "list/objectList.h"
#include "backend/metadataConfiguration.h"
#include "backend/moduleManager/moduleManager.h"

//***********************************************************************
//*                         metaData                                    * 
//***********************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(ibValueMetaObjectAccumulationRegister, ibValueMetaObjectRegisterData);

/////////////////////////////////////////////////////////////////////////

ibValueMetaObjectAccumulationRegister::ibValueMetaObjectAccumulationRegister() : ibValueMetaObjectRegisterData()
{
	//set default proc
	(*m_propertyModuleObject)->SetDefaultProcedure(wxT("BeforeWrite"), ibContentHelper::eProcedureHelper, { wxT("Cancel") });
	(*m_propertyModuleObject)->SetDefaultProcedure(wxT("OnWrite"), ibContentHelper::eProcedureHelper, { wxT("Cancel") });
}

ibValueMetaObjectAccumulationRegister::~ibValueMetaObjectAccumulationRegister()
{
	//wxDELETE((*m_propertyAttributibRecordType));
}

ibValueMetaObjectFormBase* ibValueMetaObjectAccumulationRegister::GetDefaultFormByID(const ibFormID& id) const 
{
	if (id == eFormList && m_propertyDefFormList->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormList->GetValueAsInteger());
	}

	return nullptr;
}

#pragma region _form_builder_h_
ibBackendValueForm* ibValueMetaObjectAccumulationRegister::GetListForm(const wxString& strFormName, ibBackendControlFrame* ownerControl, const ibUniqueKey& formGuid)
{
	return ibValueMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		ibValueMetaObjectAccumulationRegister::eFormList,
		ownerControl, ibValue::CreateAndPrepareValueRef<ibValueListRegisterObject>(this, ibValueMetaObjectAccumulationRegister::eFormList),
		formGuid
	);
}
#pragma endregion

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool ibValueMetaObjectAccumulationRegister::LoadData(ibReaderMemory& dataReader)
{
	//load default attributes:
	(*m_propertyAttributibRecordType)->LoadMeta(dataReader);

	//load default form 
	m_propertyDefFormList->SetValue(GetIdByGuid(dataReader.r_stringZ()));

	//load data 
	m_propertyRegisterType->SetValue(dataReader.r_u16());

	//load object module
	(*m_propertyModuleObject)->LoadMeta(dataReader);
	(*m_propertyModuleManager)->LoadMeta(dataReader);

	return ibValueMetaObjectRegisterData::LoadData(dataReader);
}

bool ibValueMetaObjectAccumulationRegister::SaveData(ibWriterMemory& dataWritter)
{
	//save default attributes:
	(*m_propertyAttributibRecordType)->SaveMeta(dataWritter);

	//save default form 
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormList->GetValueAsInteger()));

	//save data
	dataWritter.w_u16(m_propertyRegisterType->GetValueAsInteger());

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

bool ibValueMetaObjectAccumulationRegister::OnCreateMetaObject(ibMetaData* metaData, int flags)
{
	if (!ibValueMetaObjectRegisterData::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyAttributibRecordType)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyModuleManager)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyModuleObject)->OnCreateMetaObject(metaData, flags);
}

bool ibValueMetaObjectAccumulationRegister::OnLoadMetaObject(ibMetaData* metaData)
{
	if (!(*m_propertyAttributibRecordType)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyModuleManager)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyModuleObject)->OnLoadMetaObject(metaData))
		return false;

	return ibValueMetaObjectRegisterData::OnLoadMetaObject(metaData);
}

bool ibValueMetaObjectAccumulationRegister::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyAttributibRecordType)->OnSaveMetaObject(flags))
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

	return ibValueMetaObjectRegisterData::OnSaveMetaObject(flags);
}

bool ibValueMetaObjectAccumulationRegister::OnDeleteMetaObject()
{
	if (!(*m_propertyAttributibRecordType)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyModuleManager)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnDeleteMetaObject())
		return false;

	return ibValueMetaObjectRegisterData::OnDeleteMetaObject();
}

bool ibValueMetaObjectAccumulationRegister::OnReloadMetaObject()
{
	ibValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		ibValueRecordSetObjectAccumulationRegister* recordSet = nullptr;
		if (moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), recordSet)) {
			if (!recordSet->InitializeObject())
				return false;
		}
	}

	return true;
}

#include "backend/objCtor.h"

bool ibValueMetaObjectAccumulationRegister::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyAttributibRecordType)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleManager)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnBeforeRunMetaObject(flags))
		return false;

	registerSelection();

	return ibValueMetaObjectRegisterData::OnBeforeRunMetaObject(flags);
}

bool ibValueMetaObjectAccumulationRegister::OnAfterRunMetaObject(int flags)
{
	if (!(*m_propertyAttributibRecordType)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleManager)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnAfterRunMetaObject(flags))
		return false;

	ibValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		if (ibValueMetaObjectRegisterData::OnAfterRunMetaObject(flags)) {

			if (!moduleManager->AddCompileModule(m_propertyModuleObject->GetMetaObject(), CreateRecordSetObjectValue()))
				return false;

			return true;
		}
	}

	return ibValueMetaObjectRegisterData::OnAfterRunMetaObject(flags);
}

bool ibValueMetaObjectAccumulationRegister::OnBeforeCloseMetaObject()
{
	if (!(*m_propertyAttributibRecordType)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyModuleManager)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnBeforeCloseMetaObject())
		return false;

	ibValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		if (ibValueMetaObjectRegisterData::OnBeforeCloseMetaObject()) {

			if (!moduleManager->RemoveCompileModule(m_propertyModuleObject->GetMetaObject()))
				return false;

			return true;
		}
	}

	return ibValueMetaObjectRegisterData::OnBeforeCloseMetaObject();
}

bool ibValueMetaObjectAccumulationRegister::OnAfterCloseMetaObject()
{
	if (!(*m_propertyAttributibRecordType)->OnAfterCloseMetaObject())
		return false;

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

void ibValueMetaObjectAccumulationRegister::OnCreateFormObject(ibValueMetaObjectFormBase* metaForm)
{
	if (metaForm->GetTypeForm() == ibValueMetaObjectAccumulationRegister::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
}

void ibValueMetaObjectAccumulationRegister::OnRemoveMetaForm(ibValueMetaObjectFormBase* metaForm)
{
	if (metaForm->GetTypeForm() == ibValueMetaObjectAccumulationRegister::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
}
#include "accumulationRegisterManager.h"

ibValueManagerDataObject* ibValueMetaObjectAccumulationRegister::CreateManagerDataObjectValue()
{
	return ibValue::CreateAndPrepareValueRef<ibValueManagerDataObjectAccumulationRegister>(this);
}

ibValueRecordSetObject* ibValueMetaObjectAccumulationRegister::CreateRecordSetObjectRegValue(const ibUniqueKeyPair& uniqueKey)
{
	ibValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);
	if (appData->DesignerMode()) {
		ibValueRecordSetObject* pDataRef = nullptr;
		if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
			return ibValue::CreateAndPrepareValueRef<ibValueRecordSetObjectAccumulationRegister>(this, uniqueKey);
		}
		return pDataRef;
	}
	return ibValue::CreateAndPrepareValueRef<ibValueRecordSetObjectAccumulationRegister>(this, uniqueKey);
}

ibSourceDataObject* ibValueMetaObjectAccumulationRegister::CreateSourceObject(ibValueMetaObjectFormBase* metaObject)
{
	switch (metaObject->GetTypeForm())
	{
	case eFormList:
		return ibValue::CreateAndPrepareValueRef<ibValueListRegisterObject>(this, metaObject->GetTypeForm());
	}

	return nullptr;
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(ibValueMetaObjectAccumulationRegister, "AccumulationRegister", g_metaAccumulationRegisterCLSID);