////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : document metaData
////////////////////////////////////////////////////////////////////////////

#include "document.h"
#include "list/objectList.h"
#include "backend/metaData.h"
#include "backend/moduleManager/moduleManager.h"

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectDocument, IMetaObjectRecordDataMutableRef);

//********************************************************************************************

class CListDocumentDataObjectRef : public CListDataObjectRef {
public:
	CListDocumentDataObjectRef(CMetaObjectDocument* metaObject = nullptr, const form_identifier_t& formType = wxNOT_FOUND, bool choiceMode = false) :
		CListDataObjectRef(metaObject, formType, choiceMode)
	{
		IListDataObject::AppendSort(metaObject->GetDocumentNumber(), true, false);
		IListDataObject::AppendSort(metaObject->GetDocumentDate(), true, true);
		IListDataObject::AppendSort(metaObject->GetDataReference(), true, true, true);
	}
};

//********************************************************************************************
//*                                      metaData                                            *
//********************************************************************************************

CMetaObjectDocument::CMetaObjectDocument() : IMetaObjectRecordDataMutableRef()
{
	//set default proc
	(*m_propertyModuleObject)->SetDefaultProcedure("beforeWrite", eContentHelper::eProcedureHelper, { "cancel", "writeMode", "postingMode" });
	(*m_propertyModuleObject)->SetDefaultProcedure("onWrite", eContentHelper::eProcedureHelper, { "cancel" });
	(*m_propertyModuleObject)->SetDefaultProcedure("beforeDelete", eContentHelper::eProcedureHelper, { "cancel" });
	(*m_propertyModuleObject)->SetDefaultProcedure("onDelete", eContentHelper::eProcedureHelper, { "cancel" });

	(*m_propertyModuleObject)->SetDefaultProcedure("posting", eContentHelper::eProcedureHelper, { "cancel", "postingMode" });
	(*m_propertyModuleObject)->SetDefaultProcedure("undoPosting", eContentHelper::eProcedureHelper, { "cancel" });

	(*m_propertyModuleObject)->SetDefaultProcedure("filling", eContentHelper::eProcedureHelper, { "source", "standartProcessing" });
	(*m_propertyModuleObject)->SetDefaultProcedure("onCopy", eContentHelper::eProcedureHelper, { "source" });

	(*m_propertyModuleObject)->SetDefaultProcedure("setNewNumber", eContentHelper::eProcedureHelper, { "prefix", "standartProcessing" });
}

CMetaObjectDocument::~CMetaObjectDocument()
{
	//wxDELETE((*m_propertyAttributeNumber));
	//wxDELETE((*m_propertyAttributeDate));
	//wxDELETE((*m_propertyAttributePosted));
}

IMetaObjectForm* CMetaObjectDocument::GetDefaultFormByID(const form_identifier_t& id) const
{
	if (id == eFormObject && m_propertyDefFormObject->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormObject->GetValueAsInteger());
	}
	else if (id == eFormList && m_propertyDefFormList->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormList->GetValueAsInteger());
	}
	else if (id == eFormSelect && m_propertyDefFormSelect->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormSelect->GetValueAsInteger());
	}

	return nullptr;
}

ISourceDataObject* CMetaObjectDocument::CreateSourceObject(IMetaObjectForm* metaObject)
{
	switch (metaObject->GetTypeForm())
	{
	case eFormObject: return CreateObjectValue(); break;
	case eFormList:
		return CValue::CreateAndPrepareValueRef<CListDocumentDataObjectRef>(this, metaObject->GetTypeForm());
		break;
	case eFormSelect:
		return CValue::CreateAndPrepareValueRef<CListDocumentDataObjectRef>(this, metaObject->GetTypeForm(), true);
		break;
	}

	return nullptr;
}

#include "backend/appData.h"

IRecordDataObjectRef* CMetaObjectDocument::CreateObjectRefValue(const CGuid& objGuid)
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);
	CRecordDataObjectDocument* pDataRef = nullptr;
	if (appData->DesignerMode()) {
		if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef))
			return CValue::CreateAndPrepareValueRef<CRecordDataObjectDocument>(this, objGuid);
	}
	else {
		pDataRef = CValue::CreateAndPrepareValueRef<CRecordDataObjectDocument>(this, objGuid);
	}

	return pDataRef;
}

#pragma region _form_builder_h_
IBackendValueForm* CMetaObjectDocument::GetObjectForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CMetaObjectDocument::eFormObject,
		ownerControl, CreateObjectValue(),
		formGuid
	);
}

IBackendValueForm* CMetaObjectDocument::GetListForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CMetaObjectDocument::eFormList,
		ownerControl, CValue::CreateAndPrepareValueRef<CListDocumentDataObjectRef>(this, CMetaObjectDocument::eFormList),
		formGuid
	);
}

IBackendValueForm* CMetaObjectDocument::GetSelectForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CMetaObjectDocument::eFormSelect,
		ownerControl, CValue::CreateAndPrepareValueRef<CListDocumentDataObjectRef>(this, CMetaObjectDocument::eFormSelect, true),
		formGuid
	);
}
#pragma endregion

bool CMetaObjectDocument::GetFormObject(CPropertyList* prop)
{
	prop->AppendItem(wxT("notSelected"), _("<not selected>"), wxNOT_FOUND);

	for (auto formObject : GetFormArrayObject()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormObject == formObject->GetTypeForm()) {
			prop->AppendItem(formObject->GetName(), formObject->GetMetaID(), formObject);
		}
	}
	return true;
}

bool CMetaObjectDocument::GetFormList(CPropertyList* prop)
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

bool CMetaObjectDocument::GetFormSelect(CPropertyList* prop)
{
	prop->AppendItem(wxT("notSelected"), _("<not selected>"), wxNOT_FOUND);

	for (auto formObject : GetFormArrayObject()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormSelect == formObject->GetTypeForm()) {
			prop->AppendItem(formObject->GetName(), formObject->GetMetaID(), formObject);
		}
	}
	return true;
}

wxString CMetaObjectDocument::GetDataPresentation(const IValueDataObject* objValue) const
{
	CValue vDate, vNumber;
	if (!objValue->GetValueByMetaID((*m_propertyAttributeDate)->GetMetaID(), vDate))
		return wxEmptyString;
	if (!objValue->GetValueByMetaID((*m_propertyAttributeNumber)->GetMetaID(), vNumber))
		return wxEmptyString;
	return GetSynonym() << wxT(" ") << vNumber.GetString() << wxT(" ") << vDate.GetString();
}

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool CMetaObjectDocument::LoadData(CMemoryReader& dataReader)
{
	//load default attributes:
	(*m_propertyAttributeNumber)->LoadMeta(dataReader);
	(*m_propertyAttributeDate)->LoadMeta(dataReader);
	(*m_propertyAttributePosted)->LoadMeta(dataReader);

	//load object module
	(*m_propertyModuleObject)->LoadMeta(dataReader);
	(*m_propertyModuleManager)->LoadMeta(dataReader);

	//load default form 
	m_propertyDefFormObject->SetValue(GetIdByGuid(dataReader.r_stringZ()));
	m_propertyDefFormList->SetValue(GetIdByGuid(dataReader.r_stringZ()));
	m_propertyDefFormSelect->SetValue(GetIdByGuid(dataReader.r_stringZ()));

	if (!m_propertyRegisterRecord->LoadData(dataReader))
		return false;

	return IMetaObjectRecordDataMutableRef::LoadData(dataReader);
}

bool CMetaObjectDocument::SaveData(CMemoryWriter& dataWritter)
{
	//save default attributes:
	(*m_propertyAttributeNumber)->SaveMeta(dataWritter);
	(*m_propertyAttributeDate)->SaveMeta(dataWritter);
	(*m_propertyAttributePosted)->SaveMeta(dataWritter);

	//save object module
	(*m_propertyModuleObject)->SaveMeta(dataWritter);
	(*m_propertyModuleManager)->SaveMeta(dataWritter);

	//save default form 
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormObject->GetValueAsInteger()));
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormList->GetValueAsInteger()));
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormSelect->GetValueAsInteger()));

	if (!m_propertyRegisterRecord->SaveData(dataWritter))
		return false;

	//create or update table:
	return IMetaObjectRecordDataMutableRef::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool CMetaObjectDocument::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IMetaObjectRecordDataMutableRef::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyAttributeNumber)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyAttributeDate)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyAttributePosted)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyModuleObject)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyModuleManager)->OnCreateMetaObject(metaData, flags);
}

bool CMetaObjectDocument::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyAttributeNumber)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyAttributeDate)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyAttributePosted)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyModuleObject)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyModuleManager)->OnLoadMetaObject(metaData))
		return false;

	return IMetaObjectRecordDataMutableRef::OnLoadMetaObject(metaData);
}

bool CMetaObjectDocument::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyAttributeNumber)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeDate)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyAttributePosted)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyModuleManager)->OnSaveMetaObject(flags))
		return false;

	return IMetaObjectRecordDataMutableRef::OnSaveMetaObject(flags);
}

bool CMetaObjectDocument::OnDeleteMetaObject()
{
	if (!(*m_propertyAttributeNumber)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyAttributeDate)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyAttributePosted)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyModuleManager)->OnDeleteMetaObject())
		return false;

	return IMetaObjectRecordDataMutableRef::OnDeleteMetaObject();
}

bool CMetaObjectDocument::OnReloadMetaObject()
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		CRecordDataObjectDocument* pDataRef = nullptr;
		if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
			return true;
		}

		if (pDataRef->InitializeObject()) {
			if (IsDeleted()) pDataRef->ClearRecordSet();
			else pDataRef->UpdateRecordSet();
			return true;
		}

		return false;
	}

	return true;
}

#include "backend/objCtor.h"

bool CMetaObjectDocument::OnBeforeRunMetaObject(int flags)
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!(*m_propertyAttributeNumber)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeDate)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributePosted)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleManager)->OnBeforeRunMetaObject(flags))
		return false;

	registerSelection();
	return IMetaObjectRecordDataMutableRef::OnBeforeRunMetaObject(flags);
}

bool CMetaObjectDocument::OnAfterRunMetaObject(int flags)
{
	if (!(*m_propertyAttributeNumber)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeDate)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributePosted)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleManager)->OnAfterRunMetaObject(flags))
		return false;

	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	const CMetaDescription& metaDesc = m_propertyRegisterRecord->GetValueAsMetaDesc();
	for (unsigned int idx = 0; idx < metaDesc.GetTypeCount(); idx++) {
		IMetaObjectRegisterData* registerData = nullptr;
		if (m_metaData->GetMetaObject(registerData, metaDesc.GetByIdx(idx))) {
			CMetaObjectAttributePredefined* infoRecorder = registerData->GetRegisterRecorder();
			wxASSERT(infoRecorder);
			infoRecorder->GetTypeDesc().AppendMetaType((*m_propertyAttributeReference)->GetTypeDesc());
		}
	}

	if (appData->DesignerMode()) {

		if (IMetaObjectRecordDataMutableRef::OnAfterRunMetaObject(flags)) {
			return moduleManager->AddCompileModule(m_propertyModuleObject->GetMetaObject(), CreateObjectValue());
		}

		return false;
	}

	return IMetaObjectRecordDataMutableRef::OnAfterRunMetaObject(flags);
}

bool CMetaObjectDocument::OnBeforeCloseMetaObject()
{
	if (!(*m_propertyAttributePosted)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyAttributePosted)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyAttributePosted)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyModuleManager)->OnBeforeCloseMetaObject())
		return false;

	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	const CMetaDescription& metaDesc = m_propertyRegisterRecord->GetValueAsMetaDesc();
	for (unsigned int idx = 0; idx < metaDesc.GetTypeCount(); idx++) {
		IMetaObjectRegisterData* registerData = nullptr;
		if (m_metaData->GetMetaObject(registerData, metaDesc.GetByIdx(idx))) {
			CMetaObjectAttributePredefined* infoRecorder = registerData->GetRegisterRecorder();
			wxASSERT(infoRecorder);
			infoRecorder->GetTypeDesc().ClearMetaType((*m_propertyAttributeReference)->GetTypeDesc());
		}
	}

	if (appData->DesignerMode()) {

		if (IMetaObjectRecordDataMutableRef::OnBeforeCloseMetaObject()) {
			return moduleManager->RemoveCompileModule(m_propertyModuleObject->GetMetaObject());
		}

		return false;
	}

	return IMetaObjectRecordDataMutableRef::OnBeforeCloseMetaObject();
}

bool CMetaObjectDocument::OnAfterCloseMetaObject()
{
	if (!(*m_propertyAttributePosted)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyAttributePosted)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyAttributePosted)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyModuleManager)->OnAfterCloseMetaObject())
		return false;

	unregisterSelection();

	return IMetaObjectRecordDataMutableRef::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                             form events                             *
//***********************************************************************

void CMetaObjectDocument::OnCreateFormObject(IMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CMetaObjectDocument::eFormObject
		&& m_propertyDefFormObject->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormObject->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CMetaObjectDocument::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CMetaObjectDocument::eFormSelect
		&& m_propertyDefFormSelect->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormSelect->SetValue(metaForm->GetMetaID());
	}
}

void CMetaObjectDocument::OnRemoveMetaForm(IMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CMetaObjectDocument::eFormObject
		&& m_propertyDefFormObject->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormObject->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CMetaObjectDocument::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CMetaObjectDocument::eFormSelect
		&& m_propertyDefFormSelect->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormSelect->SetValue(metaForm->GetMetaID());
	}
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CMetaObjectDocument, "document", g_metaDocumentCLSID);
SYSTEM_TYPE_REGISTER(CRecordDataObjectDocument::CRecorderRegisterDocument, "recordRegister", string_to_clsid("VL_RECR"));