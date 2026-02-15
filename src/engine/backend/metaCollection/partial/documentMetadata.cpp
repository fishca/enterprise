////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : document metaData
////////////////////////////////////////////////////////////////////////////

#include "document.h"
#include "list/objectList.h"
#include "backend/metaData.h"
#include "backend/moduleManager/moduleManager.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectDocument, IValueMetaObjectRecordDataMutableRef);

//********************************************************************************************

class CListDocumentDataObjectRef : public CValueListDataObjectRef {
public:
	CListDocumentDataObjectRef(CValueMetaObjectDocument* metaObject = nullptr, const form_identifier_t& formType = wxNOT_FOUND, bool choiceMode = false) :
		CValueListDataObjectRef(metaObject, formType, choiceMode)
	{
		IValueListDataObject::AppendSort(metaObject->GetDocumentNumber(), true, false);
		IValueListDataObject::AppendSort(metaObject->GetDocumentDate(), true, true);
		IValueListDataObject::AppendSort(metaObject->GetDataReference(), true, true, true);
	}
};

//********************************************************************************************
//*                                      metaData                                            *
//********************************************************************************************

CValueMetaObjectDocument::CValueMetaObjectDocument() : IValueMetaObjectRecordDataMutableRef()
{
	//set default proc
	(*m_propertyModuleObject)->SetDefaultProcedure(wxT("BeforeWrite"), eContentHelper::eProcedureHelper, { wxT("Cancel"), wxT("WriteMode"), wxT("PostingMode") });
	(*m_propertyModuleObject)->SetDefaultProcedure(wxT("OnWrite"), eContentHelper::eProcedureHelper, { wxT("Cancel") });
	(*m_propertyModuleObject)->SetDefaultProcedure(wxT("BeforeDelete"), eContentHelper::eProcedureHelper, { wxT("Cancel") });
	(*m_propertyModuleObject)->SetDefaultProcedure(wxT("OnDelete"), eContentHelper::eProcedureHelper, { wxT("Cancel") });

	(*m_propertyModuleObject)->SetDefaultProcedure(wxT("Posting"), eContentHelper::eProcedureHelper, { wxT("Cancel"), wxT("PostingMode") });
	(*m_propertyModuleObject)->SetDefaultProcedure(wxT("UndoPosting"), eContentHelper::eProcedureHelper, { wxT("Cancel") });

	(*m_propertyModuleObject)->SetDefaultProcedure(wxT("Filling"), eContentHelper::eProcedureHelper, { wxT("Source"), wxT("StandartProcessing") });
	(*m_propertyModuleObject)->SetDefaultProcedure(wxT("OnCopy"), eContentHelper::eProcedureHelper, { wxT("Source") });

	(*m_propertyModuleObject)->SetDefaultProcedure(wxT("SetNewNumber"), eContentHelper::eProcedureHelper, { wxT("Prefix"), wxT("StandartProcessing") });
}

CValueMetaObjectDocument::~CValueMetaObjectDocument()
{
	//wxDELETE((*m_propertyAttributeNumber));
	//wxDELETE((*m_propertyAttributeDate));
	//wxDELETE((*m_propertyAttributePosted));
}

IValueMetaObjectForm* CValueMetaObjectDocument::GetDefaultFormByID(const form_identifier_t& id) const
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

#include "documentManager.h"

IValueManagerDataObject* CValueMetaObjectDocument::CreateManagerDataObjectValue()
{
	return CValue::CreateAndPrepareValueRef<CValueManagerDataObjectDocument>(this);
}

#include "backend/appData.h"

IValueRecordDataObjectRef* CValueMetaObjectDocument::CreateObjectRefValue(const CGuid& objGuid)
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);
	CValueRecordDataObjectDocument* pDataRef = nullptr;
	if (appData->DesignerMode()) {
		if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef))
			return CValue::CreateAndPrepareValueRef<CValueRecordDataObjectDocument>(this, objGuid);
	}
	else {
		pDataRef = CValue::CreateAndPrepareValueRef<CValueRecordDataObjectDocument>(this, objGuid);
	}

	return pDataRef;
}

ISourceDataObject* CValueMetaObjectDocument::CreateSourceObject(IValueMetaObjectForm* metaObject)
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

#pragma region _form_builder_h_
IBackendValueForm* CValueMetaObjectDocument::GetObjectForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IValueMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CValueMetaObjectDocument::eFormObject,
		ownerControl, CreateObjectValue(),
		formGuid
	);
}

IBackendValueForm* CValueMetaObjectDocument::GetListForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IValueMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CValueMetaObjectDocument::eFormList,
		ownerControl, CValue::CreateAndPrepareValueRef<CListDocumentDataObjectRef>(this, CValueMetaObjectDocument::eFormList),
		formGuid
	);
}

IBackendValueForm* CValueMetaObjectDocument::GetSelectForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IValueMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CValueMetaObjectDocument::eFormSelect,
		ownerControl, CValue::CreateAndPrepareValueRef<CListDocumentDataObjectRef>(this, CValueMetaObjectDocument::eFormSelect, true),
		formGuid
	);
}
#pragma endregion

wxString CValueMetaObjectDocument::GetDataPresentation(const IValueDataObject* objValue) const
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

bool CValueMetaObjectDocument::LoadData(CMemoryReader& dataReader)
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

	return IValueMetaObjectRecordDataMutableRef::LoadData(dataReader);
}

bool CValueMetaObjectDocument::SaveData(CMemoryWriter& dataWritter)
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
	return IValueMetaObjectRecordDataMutableRef::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool CValueMetaObjectDocument::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IValueMetaObjectRecordDataMutableRef::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyAttributeNumber)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyAttributeDate)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyAttributePosted)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyModuleObject)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyModuleManager)->OnCreateMetaObject(metaData, flags);
}

bool CValueMetaObjectDocument::OnLoadMetaObject(IMetaData* metaData)
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

	return IValueMetaObjectRecordDataMutableRef::OnLoadMetaObject(metaData);
}

bool CValueMetaObjectDocument::OnSaveMetaObject(int flags)
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

	return IValueMetaObjectRecordDataMutableRef::OnSaveMetaObject(flags);
}

bool CValueMetaObjectDocument::OnDeleteMetaObject()
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

	return IValueMetaObjectRecordDataMutableRef::OnDeleteMetaObject();
}

bool CValueMetaObjectDocument::OnReloadMetaObject()
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		CValueRecordDataObjectDocument* pDataRef = nullptr;
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

bool CValueMetaObjectDocument::OnBeforeRunMetaObject(int flags)
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
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
	return IValueMetaObjectRecordDataMutableRef::OnBeforeRunMetaObject(flags);
}

bool CValueMetaObjectDocument::OnAfterRunMetaObject(int flags)
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

	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	const CMetaDescription& metaDesc = m_propertyRegisterRecord->GetValueAsMetaDesc();
	for (unsigned int idx = 0; idx < metaDesc.GetTypeCount(); idx++) {
		const IValueMetaObjectRegisterData* registerData = m_metaData->FindAnyObjectByFilter<IValueMetaObjectRegisterData>(metaDesc.GetByIdx(idx));
		if (registerData != nullptr) {
			CValueMetaObjectAttributePredefined* infoRecorder = registerData->GetRegisterRecorder();
			wxASSERT(infoRecorder);
			infoRecorder->GetTypeDesc().AppendMetaType((*m_propertyAttributeReference)->GetTypeDesc());
		}
	}

	if (appData->DesignerMode()) {

		if (IValueMetaObjectRecordDataMutableRef::OnAfterRunMetaObject(flags)) {
			return moduleManager->AddCompileModule(m_propertyModuleObject->GetMetaObject(), CreateObjectValue());
		}

		return false;
	}

	return IValueMetaObjectRecordDataMutableRef::OnAfterRunMetaObject(flags);
}

bool CValueMetaObjectDocument::OnBeforeCloseMetaObject()
{
	if (!(*m_propertyAttributeNumber)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeDate)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyAttributePosted)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyModuleManager)->OnBeforeCloseMetaObject())
		return false;

	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	const CMetaDescription& metaDesc = m_propertyRegisterRecord->GetValueAsMetaDesc();
	for (unsigned int idx = 0; idx < metaDesc.GetTypeCount(); idx++) {
		const IValueMetaObjectRegisterData* registerData = m_metaData->FindAnyObjectByFilter<IValueMetaObjectRegisterData>(metaDesc.GetByIdx(idx));
		if (registerData != nullptr) {
			CValueMetaObjectAttributePredefined* infoRecorder = registerData->GetRegisterRecorder();
			wxASSERT(infoRecorder);
			infoRecorder->GetTypeDesc().ClearMetaType((*m_propertyAttributeReference)->GetTypeDesc());
		}
	}

	if (appData->DesignerMode()) {

		if (IValueMetaObjectRecordDataMutableRef::OnBeforeCloseMetaObject()) {
			return moduleManager->RemoveCompileModule(m_propertyModuleObject->GetMetaObject());
		}

		return false;
	}

	return IValueMetaObjectRecordDataMutableRef::OnBeforeCloseMetaObject();
}

bool CValueMetaObjectDocument::OnAfterCloseMetaObject()
{
	if (!(*m_propertyAttributeNumber)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeDate)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyAttributePosted)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyModuleManager)->OnAfterCloseMetaObject())
		return false;

	unregisterSelection();

	return IValueMetaObjectRecordDataMutableRef::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                             form events                             *
//***********************************************************************

void CValueMetaObjectDocument::OnCreateFormObject(IValueMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CValueMetaObjectDocument::eFormObject
		&& m_propertyDefFormObject->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormObject->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CValueMetaObjectDocument::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CValueMetaObjectDocument::eFormSelect
		&& m_propertyDefFormSelect->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormSelect->SetValue(metaForm->GetMetaID());
	}
}

void CValueMetaObjectDocument::OnRemoveMetaForm(IValueMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CValueMetaObjectDocument::eFormObject
		&& m_propertyDefFormObject->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormObject->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CValueMetaObjectDocument::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CValueMetaObjectDocument::eFormSelect
		&& m_propertyDefFormSelect->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormSelect->SetValue(metaForm->GetMetaID());
	}
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CValueMetaObjectDocument, "Document", g_metaDocumentCLSID);
SYSTEM_TYPE_REGISTER(CValueRecordDataObjectDocument::CRecorderRegisterDocument, "RecordRegister", string_to_clsid("VL_RECR"));