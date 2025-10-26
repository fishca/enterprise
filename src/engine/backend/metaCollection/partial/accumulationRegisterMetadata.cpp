#include "accumulationRegister.h"
#include "list/objectList.h"
#include "backend/metadataConfiguration.h"
#include "backend/moduleManager/moduleManager.h"

//***********************************************************************
//*                         metaData                                    * 
//***********************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectAccumulationRegister, IMetaObjectRegisterData);

/////////////////////////////////////////////////////////////////////////

CMetaObjectAccumulationRegister::CMetaObjectAccumulationRegister() : IMetaObjectRegisterData()
{
	//set default proc
	(*m_propertyModuleObject)->SetDefaultProcedure("beforeWrite", eContentHelper::eProcedureHelper, { "cancel" });
	(*m_propertyModuleObject)->SetDefaultProcedure("onWrite", eContentHelper::eProcedureHelper, { "cancel" });
}

CMetaObjectAccumulationRegister::~CMetaObjectAccumulationRegister()
{
	//wxDELETE((*m_propertyAttributeRecordType));
}

CMetaObjectForm* CMetaObjectAccumulationRegister::GetDefaultFormByID(const form_identifier_t& id)
{
	if (id == eFormList && m_propertyDefFormList->GetValueAsInteger() != wxNOT_FOUND) {
		for (auto& obj : GetObjectForms()) {
			if (m_propertyDefFormList->GetValueAsInteger() == obj->GetMetaID()) {
				return obj;
			}
		}
	}

	return nullptr;
}

#pragma region _form_builder_h_
IBackendValueForm* CMetaObjectAccumulationRegister::GetListForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CMetaObjectAccumulationRegister::eFormList,
		ownerControl, m_metaData->CreateAndConvertObjectValueRef<CListRegisterObject>(this, CMetaObjectAccumulationRegister::eFormList),
		formGuid
	);
}
#pragma endregion

/////////////////////////////////////////////////////////////////////////////

bool CMetaObjectAccumulationRegister::GetFormList(CPropertyList* prop)
{
	prop->AppendItem(wxT("notSelected"), _("<not selected>"), wxNOT_FOUND);
	for (auto formObject : GetObjectForms()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormList == formObject->GetTypeForm()) {
			prop->AppendItem(formObject->GetName(), formObject->GetMetaID(), formObject);
		}
	}
	return true; 
}

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool CMetaObjectAccumulationRegister::LoadData(CMemoryReader& dataReader)
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

	return IMetaObjectRegisterData::LoadData(dataReader);
}

bool CMetaObjectAccumulationRegister::SaveData(CMemoryWriter& dataWritter)
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
	return IMetaObjectRegisterData::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

#include "backend/appData.h"

bool CMetaObjectAccumulationRegister::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IMetaObjectRegisterData::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyAttributeRecordType)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyModuleManager)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyModuleObject)->OnCreateMetaObject(metaData, flags);
}

bool CMetaObjectAccumulationRegister::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyAttributeRecordType)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyModuleManager)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyModuleObject)->OnLoadMetaObject(metaData))
		return false;

	return IMetaObjectRegisterData::OnLoadMetaObject(metaData);
}

bool CMetaObjectAccumulationRegister::OnSaveMetaObject(int flags)
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

	return IMetaObjectRegisterData::OnSaveMetaObject(flags);
}

bool CMetaObjectAccumulationRegister::OnDeleteMetaObject()
{
	if (!(*m_propertyAttributeRecordType)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyModuleManager)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnDeleteMetaObject())
		return false;

	return IMetaObjectRegisterData::OnDeleteMetaObject();
}

bool CMetaObjectAccumulationRegister::OnReloadMetaObject()
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		CRecordSetObjectAccumulationRegister* recordSet = nullptr;
		if (moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), recordSet)) {
			if (!recordSet->InitializeObject())
				return false;
		}
	}

	return true;
}

#include "backend/objCtor.h"

bool CMetaObjectAccumulationRegister::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyAttributeRecordType)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleManager)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnBeforeRunMetaObject(flags))
		return false;

	registerSelection();

	return IMetaObjectRegisterData::OnBeforeRunMetaObject(flags);
}

bool CMetaObjectAccumulationRegister::OnAfterRunMetaObject(int flags)
{
	if (!(*m_propertyAttributeRecordType)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleManager)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnAfterRunMetaObject(flags))
		return false;

	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		if (IMetaObjectRegisterData::OnAfterRunMetaObject(flags)) {

			if (!moduleManager->AddCompileModule(m_propertyModuleObject->GetMetaObject(), CreateRecordSetObjectValue()))
				return false;

			return true;
		}
	}

	return IMetaObjectRegisterData::OnAfterRunMetaObject(flags);
}

bool CMetaObjectAccumulationRegister::OnBeforeCloseMetaObject()
{
	if (!(*m_propertyAttributeRecordType)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyModuleManager)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnBeforeCloseMetaObject())
		return false;

	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		if (IMetaObjectRegisterData::OnBeforeCloseMetaObject()) {

			if (!moduleManager->RemoveCompileModule(m_propertyModuleObject->GetMetaObject()))
				return false;

			return true;
		}
	}

	return IMetaObjectRegisterData::OnBeforeCloseMetaObject();
}

bool CMetaObjectAccumulationRegister::OnAfterCloseMetaObject()
{
	if (!(*m_propertyAttributeRecordType)->OnAfterCloseMetaObject())
		return false;

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

void CMetaObjectAccumulationRegister::OnCreateFormObject(IMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CMetaObjectAccumulationRegister::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
}

void CMetaObjectAccumulationRegister::OnRemoveMetaForm(IMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CMetaObjectAccumulationRegister::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
}

std::vector<IMetaObjectAttribute*> CMetaObjectAccumulationRegister::GetDefaultAttributes() const
{
	std::vector<IMetaObjectAttribute*> attributes;
	attributes.push_back(m_propertyAttributeLineActive->GetMetaObject());
	if (GetRegisterType() == eRegisterType::eBalances) {
		attributes.push_back(m_propertyAttributeRecordType->GetMetaObject());
	}
	attributes.push_back(m_propertyAttributePeriod->GetMetaObject());
	attributes.push_back(m_propertyAttributeRecorder->GetMetaObject());
	attributes.push_back(m_propertyAttributeLineNumber->GetMetaObject());
	return attributes;
}

std::vector<IMetaObjectAttribute*> CMetaObjectAccumulationRegister::GetGenericDimensions() const
{
	std::vector<IMetaObjectAttribute*> attributes;
	attributes.push_back(m_propertyAttributeRecorder->GetMetaObject());
	return attributes;
}

ISourceDataObject* CMetaObjectAccumulationRegister::CreateSourceObject(IMetaObjectForm* metaObject)
{
	switch (metaObject->GetTypeForm())
	{
	case eFormList:
		return 	m_metaData->CreateAndConvertObjectValueRef<CListRegisterObject>(this, metaObject->GetTypeForm());
	}

	return nullptr;
}

IRecordSetObject* CMetaObjectAccumulationRegister::CreateRecordSetObjectRegValue(const CUniquePairKey& uniqueKey)
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);
	if (appData->DesignerMode()) {
		IRecordSetObject* pDataRef = nullptr;
		if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
			return 	m_metaData->CreateAndConvertObjectValueRef<CRecordSetObjectAccumulationRegister>(this, uniqueKey);
		}
		return pDataRef;
	}
	return 	m_metaData->CreateAndConvertObjectValueRef<CRecordSetObjectAccumulationRegister>(this, uniqueKey);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CMetaObjectAccumulationRegister, "accumulationRegister", g_metaAccumulationRegisterCLSID);