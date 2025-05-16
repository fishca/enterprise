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
	m_propertyModuleObject->GetMetaObject()->SetDefaultProcedure("beforeWrite", eContentHelper::eProcedureHelper, {"cancel", "writeMode", "postingMode"});
	m_propertyModuleObject->GetMetaObject()->SetDefaultProcedure("onWrite", eContentHelper::eProcedureHelper, { "cancel" });
	m_propertyModuleObject->GetMetaObject()->SetDefaultProcedure("beforeDelete", eContentHelper::eProcedureHelper, { "cancel" });
	m_propertyModuleObject->GetMetaObject()->SetDefaultProcedure("onDelete", eContentHelper::eProcedureHelper, { "cancel" });

	m_propertyModuleObject->GetMetaObject()->SetDefaultProcedure("posting", eContentHelper::eProcedureHelper, { "cancel", "postingMode" });
	m_propertyModuleObject->GetMetaObject()->SetDefaultProcedure("undoPosting", eContentHelper::eProcedureHelper, { "cancel" });

	m_propertyModuleObject->GetMetaObject()->SetDefaultProcedure("filling", eContentHelper::eProcedureHelper, { "source", "standartProcessing" });
	m_propertyModuleObject->GetMetaObject()->SetDefaultProcedure("onCopy", eContentHelper::eProcedureHelper, { "source" });
}

CMetaObjectDocument::~CMetaObjectDocument()
{
	wxDELETE(m_attributeNumber);
	wxDELETE(m_attributeDate);
	wxDELETE(m_attributePosted);
}

CMetaObjectForm* CMetaObjectDocument::GetDefaultFormByID(const form_identifier_t& id)
{
	if (id == eFormObject
		&& m_propertyDefFormObject->GetValueAsInteger() != wxNOT_FOUND) {
		for (auto& obj : GetObjectForms()) {
			if (m_propertyDefFormObject->GetValueAsInteger() == obj->GetMetaID()) {
				return obj;
			}
		}
	}
	else if (id == eFormList
		&& m_propertyDefFormList->GetValueAsInteger() != wxNOT_FOUND) {
		for (auto& obj : GetObjectForms()) {
			if (m_propertyDefFormList->GetValueAsInteger() == obj->GetMetaID()) {
				return obj;
			}
		}
	}
	else if (id == eFormSelect
		&& m_propertyDefFormSelect->GetValueAsInteger() != wxNOT_FOUND) {
		for (auto& obj : GetObjectForms()) {
			if (m_propertyDefFormSelect->GetValueAsInteger() == obj->GetMetaID()) {
				return obj;
			}
		}
	}

	return nullptr;
}

ISourceDataObject* CMetaObjectDocument::CreateObjectData(IMetaObjectForm* metaObject)
{
	switch (metaObject->GetTypeForm())
	{
	case eFormObject: return CreateObjectValue(); break;
	case eFormList:
		return m_metaData->CreateAndConvertObjectValueRef<CListDocumentDataObjectRef>(this, metaObject->GetTypeForm());
		break;
	case eFormSelect:
		return m_metaData->CreateAndConvertObjectValueRef<CListDocumentDataObjectRef>(this, metaObject->GetTypeForm(), true);
		break;
	}

	return nullptr;
}

#include "backend/appData.h"

IRecordDataObjectRef* CMetaObjectDocument::CreateObjectRefValue(const Guid& objGuid)
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);
	CRecordDataObjectDocument* pDataRef = nullptr;
	if (appData->DesignerMode()) {
		if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef))
			return m_metaData->CreateAndConvertObjectValueRef<CRecordDataObjectDocument>(this, objGuid);
	}
	else {
		pDataRef = m_metaData->CreateAndConvertObjectValueRef<CRecordDataObjectDocument>(this, objGuid);
	}

	return pDataRef;
}



IBackendValueForm* CMetaObjectDocument::GetObjectForm(const wxString& formName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	CMetaObjectForm* defList = nullptr;

	if (!formName.IsEmpty()) {
		for (auto metaForm : GetObjectForms()) {
			if (CMetaObjectDocument::eFormObject == metaForm->GetTypeForm()
				&& stringUtils::CompareString(formName, metaForm->GetName())) {
				defList = metaForm; break;
			}
		}
		wxASSERT(defList);
	}
	else {
		defList = GetDefaultFormByID(CMetaObjectDocument::eFormObject);
	}

	if (defList == nullptr) {
		IRecordDataObject* objectData = CreateObjectValue();
		IBackendValueForm* valueForm = IBackendValueForm::CreateNewForm(ownerControl, nullptr,
			objectData, formGuid
		);
		valueForm->BuildForm(CMetaObjectDocument::eFormObject);
		return valueForm;
	}

	return defList->GenerateFormAndRun(
		ownerControl, CreateObjectValue(), formGuid
	);
}

IBackendValueForm* CMetaObjectDocument::GetListForm(const wxString& formName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	CMetaObjectForm* defList = nullptr;

	if (!formName.IsEmpty()) {
		for (auto metaForm : GetObjectForms()) {
			if (CMetaObjectDocument::eFormList == metaForm->GetTypeForm()
				&& stringUtils::CompareString(formName, metaForm->GetName())) {
				defList = metaForm; break;
			}
		}
		wxASSERT(defList);
	}
	else {
		defList = GetDefaultFormByID(CMetaObjectDocument::eFormList);
	}

	if (defList == nullptr) {
		IBackendValueForm* valueForm = IBackendValueForm::CreateNewForm(ownerControl, nullptr,
			m_metaData->CreateAndConvertObjectValueRef <CListDocumentDataObjectRef>(this, CMetaObjectDocument::eFormList), formGuid
		);
		valueForm->BuildForm(CMetaObjectDocument::eFormList);
		return valueForm;
	}

	return defList->GenerateFormAndRun(
		ownerControl, m_metaData->CreateAndConvertObjectValueRef<CListDocumentDataObjectRef>(this, defList->GetTypeForm()), formGuid
	);
}

IBackendValueForm* CMetaObjectDocument::GetSelectForm(const wxString& formName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	CMetaObjectForm* defList = nullptr;

	if (!formName.IsEmpty()) {
		for (auto metaForm : GetObjectForms()) {
			if (CMetaObjectDocument::eFormSelect == metaForm->GetTypeForm()
				&& stringUtils::CompareString(formName, metaForm->GetName())) {
				defList = metaForm; break;
			}
		}
		wxASSERT(defList);
	}
	else {
		defList = GetDefaultFormByID(CMetaObjectDocument::eFormSelect);
	}

	if (defList == nullptr) {
		IBackendValueForm* valueForm = IBackendValueForm::CreateNewForm(ownerControl, nullptr,
			m_metaData->CreateAndConvertObjectValueRef<CListDocumentDataObjectRef>(this, CMetaObjectDocument::eFormSelect, true), formGuid
		);
		valueForm->BuildForm(CMetaObjectDocument::eFormSelect);
		return valueForm;
	}

	return defList->GenerateFormAndRun(
		ownerControl, m_metaData->CreateAndConvertObjectValueRef<CListDocumentDataObjectRef>(this, defList->GetTypeForm()), formGuid
	);
}

bool CMetaObjectDocument::GetFormObject(CPropertyList* prop)
{
	prop->AppendItem(_("<not selected>"), wxNOT_FOUND, wxEmptyValue);
	for (auto formObject : GetObjectForms()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormObject == formObject->GetTypeForm()) {
			prop->AppendItem(formObject->GetName(), formObject->GetMetaID(), formObject);
		}
	}
	return true;
}

bool CMetaObjectDocument::GetFormList(CPropertyList* prop)
{
	prop->AppendItem(_("<not selected>"), wxNOT_FOUND, wxEmptyValue);
	for (auto formObject : GetObjectForms()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormList == formObject->GetTypeForm()) {
			prop->AppendItem(formObject->GetName(), formObject->GetMetaID(), formObject);
		}
	}
	return true;
}

bool CMetaObjectDocument::GetFormSelect(CPropertyList* prop)
{
	prop->AppendItem(_("<not selected>"), wxNOT_FOUND, wxEmptyValue);
	for (auto formObject : GetObjectForms()) {
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
	if (!objValue->GetValueByMetaID(m_attributeDate->GetMetaID(), vDate))
		return wxEmptyString;
	if (!objValue->GetValueByMetaID(m_attributeNumber->GetMetaID(), vNumber))
		return wxEmptyString;
	return GetSynonym() << wxT(" ") << vNumber.GetString() << wxT(" ") << vDate.GetString();
}

std::vector<IMetaObjectAttribute*> CMetaObjectDocument::GetDefaultAttributes() const
{
	std::vector<IMetaObjectAttribute*> attributes;
	attributes.push_back(m_attributeNumber);
	attributes.push_back(m_attributeDate);
	attributes.push_back(m_attributePosted);
	attributes.push_back(m_attributeReference);
	attributes.push_back(m_attributeDeletionMark);
	return attributes;
}

std::vector<IMetaObjectAttribute*> CMetaObjectDocument::GetSearchedAttributes() const
{
	std::vector<IMetaObjectAttribute*> attributes;

	attributes.push_back(m_attributeNumber);
	attributes.push_back(m_attributeDate);

	return attributes;
}

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool CMetaObjectDocument::LoadData(CMemoryReader& dataReader)
{
	//load default attributes:
	m_attributeNumber->LoadMeta(dataReader);
	m_attributeDate->LoadMeta(dataReader);
	m_attributePosted->LoadMeta(dataReader);
	
	//load object module
	m_propertyModuleObject->GetMetaObject()->LoadMeta(dataReader);
	m_propertyModuleManager->GetMetaObject()->LoadMeta(dataReader);

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
	m_attributeNumber->SaveMeta(dataWritter);
	m_attributeDate->SaveMeta(dataWritter);
	m_attributePosted->SaveMeta(dataWritter);

	//save object module
	m_propertyModuleObject->GetMetaObject()->SaveMeta(dataWritter);
	m_propertyModuleManager->GetMetaObject()->SaveMeta(dataWritter);

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

#include "backend/appData.h"

bool CMetaObjectDocument::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IMetaObjectRecordDataMutableRef::OnCreateMetaObject(metaData, flags))
		return false;

	return m_attributeNumber->OnCreateMetaObject(metaData, flags) &&
		m_attributeDate->OnCreateMetaObject(metaData, flags) &&
		m_attributePosted->OnCreateMetaObject(metaData, flags) &&
		m_propertyModuleObject->GetMetaObject()->OnCreateMetaObject(metaData, flags) &&
		m_propertyModuleManager->GetMetaObject()->OnCreateMetaObject(metaData, flags);
}

bool CMetaObjectDocument::OnLoadMetaObject(IMetaData* metaData)
{
	if (!m_attributeNumber->OnLoadMetaObject(metaData))
		return false;

	if (!m_attributeDate->OnLoadMetaObject(metaData))
		return false;

	if (!m_attributePosted->OnLoadMetaObject(metaData))
		return false;

	if (!m_propertyModuleObject->GetMetaObject()->OnLoadMetaObject(metaData))
		return false;

	if (!m_propertyModuleManager->GetMetaObject()->OnLoadMetaObject(metaData))
		return false;

	return IMetaObjectRecordDataMutableRef::OnLoadMetaObject(metaData);
}

bool CMetaObjectDocument::OnSaveMetaObject()
{
	if (!m_attributeNumber->OnSaveMetaObject())
		return false;

	if (!m_attributeDate->OnSaveMetaObject())
		return false;

	if (!m_attributePosted->OnSaveMetaObject())
		return false;

	if (!m_propertyModuleObject->GetMetaObject()->OnSaveMetaObject())
		return false;

	if (!m_propertyModuleManager->GetMetaObject()->OnSaveMetaObject())
		return false;

	return IMetaObjectRecordDataMutableRef::OnSaveMetaObject();
}

bool CMetaObjectDocument::OnDeleteMetaObject()
{
	if (!m_attributeNumber->OnDeleteMetaObject())
		return false;

	if (!m_attributeDate->OnDeleteMetaObject())
		return false;

	if (!m_attributePosted->OnDeleteMetaObject())
		return false;

	if (!m_propertyModuleObject->GetMetaObject()->OnDeleteMetaObject())
		return false;

	if (!m_propertyModuleManager->GetMetaObject()->OnDeleteMetaObject())
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

	if (!m_attributeNumber->OnBeforeRunMetaObject(flags))
		return false;

	if (!m_attributeDate->OnBeforeRunMetaObject(flags))
		return false;

	if (!m_attributePosted->OnBeforeRunMetaObject(flags))
		return false;

	if (!m_propertyModuleObject->GetMetaObject()->OnBeforeRunMetaObject(flags))
		return false;

	if (!m_propertyModuleManager->GetMetaObject()->OnBeforeRunMetaObject(flags))
		return false;

	registerSelection();

	return IMetaObjectRecordDataMutableRef::OnBeforeRunMetaObject(flags);
}

bool CMetaObjectDocument::OnAfterRunMetaObject(int flags)
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

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
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

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
	if (!m_attributePosted->OnAfterCloseMetaObject())
		return false;

	if (!m_attributePosted->OnAfterCloseMetaObject())
		return false;

	if (!m_attributePosted->OnAfterCloseMetaObject())
		return false;

	if (!m_propertyModuleObject->GetMetaObject()->OnAfterCloseMetaObject())
		return false;

	if (!m_propertyModuleManager->GetMetaObject()->OnAfterCloseMetaObject())
		return false;

	const CMetaDescription& metaDesc = m_propertyRegisterRecord->GetValueAsMetaDesc();
	for (unsigned int idx = 0; idx < metaDesc.GetTypeCount(); idx++) {
		IMetaObjectRegisterData* registerData = nullptr;
		if (m_metaData->GetMetaObject(registerData, metaDesc.GetByIdx(idx))) {
			CMetaObjectAttributeDefault* infoRecorder = registerData->GetRegisterRecorder();
			wxASSERT(infoRecorder);
			infoRecorder->GetTypeDesc().ClearMetaType(m_attributeReference->GetTypeDesc());
		}
	}

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