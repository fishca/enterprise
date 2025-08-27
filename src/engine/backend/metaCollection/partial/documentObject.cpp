////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : document object
////////////////////////////////////////////////////////////////////////////

#include "document.h"
#include "backend/metaData.h"

#include "backend/appData.h"
#include "reference/reference.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/system/systemManager.h"

//*********************************************************************************************
//*                                  CRecorderRegisterDocument	                              *
//*********************************************************************************************

////////////////////////////////////////////////////////////////////////////////////////////////////
wxIMPLEMENT_DYNAMIC_CLASS(CRecordDataObjectDocument::CRecorderRegisterDocument, CValue);
////////////////////////////////////////////////////////////////////////////////////////////////////

void CRecordDataObjectDocument::CRecorderRegisterDocument::CreateRecordSet()
{
	CMetaObjectDocument* metaDocument = dynamic_cast<CMetaObjectDocument*>(m_document->GetMetaObject());
	wxASSERT(metaDocument);
	IMetaData* metaData = metaDocument->GetMetaData();
	wxASSERT(metaData);

	CRecorderRegisterDocument::ClearRecordSet();

	const CMetaDescription& metaDesc = metaDocument->GetRecordDescription();
	for (unsigned int idx = 0; idx < metaDesc.GetTypeCount(); idx++) {
		IMetaObjectRegisterData* metaObject = nullptr;
		if (!metaData->GetMetaObject(metaObject, metaDesc.GetByIdx(idx)) || !metaObject->IsAllowed())
			continue;
		CMetaObjectAttributeDefault* registerRecord = metaObject->GetRegisterRecorder();
		wxASSERT(registerRecord);

		CValuePtr<IRecordSetObject> recordSet(metaObject->CreateRecordSetObjectValue());
		recordSet->SetKeyValue(registerRecord->GetMetaID(), m_document->GetReference());
		m_records.insert_or_assign(metaObject->GetMetaID(), recordSet);
	}

	PrepareNames();
}

bool CRecordDataObjectDocument::CRecorderRegisterDocument::WriteRecordSet()
{
	for (auto& pair : m_records) {
		IRecordSetObject* record = pair.second;
		wxASSERT(record);
		try {
			if (!record->WriteRecordSet()) {
				return false;
			}
		}
		catch (...) {
			return false;
		}
	}

	return true;
}

bool CRecordDataObjectDocument::CRecorderRegisterDocument::DeleteRecordSet()
{
	for (auto& pair : m_records) {
		IRecordSetObject* record = pair.second;
		wxASSERT(record);
		try {
			if (!record->DeleteRecordSet()) {
				return false;
			}
		}
		catch (...) {
			return false;
		}
	}

	return true;
}

void CRecordDataObjectDocument::CRecorderRegisterDocument::ClearRecordSet()
{
	m_records.clear();
}

CRecordDataObjectDocument::CRecorderRegisterDocument::CRecorderRegisterDocument(CRecordDataObjectDocument* currentDoc) :
	CValue(eValueTypes::TYPE_VALUE), m_document(currentDoc), m_methodHelper(new CMethodHelper())
{
	CRecorderRegisterDocument::CreateRecordSet();
}

CRecordDataObjectDocument::CRecorderRegisterDocument::~CRecorderRegisterDocument()
{
	CRecorderRegisterDocument::ClearRecordSet();
	wxDELETE(m_methodHelper);
}

//*********************************************************************************************
//*                                  CRecordDataObjectDocument	                                      *
//*********************************************************************************************

CRecordDataObjectDocument::CRecordDataObjectDocument(CMetaObjectDocument* metaObject, const CGuid& objGuid) :
	IRecordDataObjectRef(metaObject, objGuid),
	m_registerRecords(new CRecorderRegisterDocument(this))
{
}

CRecordDataObjectDocument::CRecordDataObjectDocument(const CRecordDataObjectDocument& source) :
	IRecordDataObjectRef(source),
	m_registerRecords(new CRecorderRegisterDocument(this))
{
}

CRecordDataObjectDocument::~CRecordDataObjectDocument()
{
}

bool CRecordDataObjectDocument::IsPosted() const
{
	CMetaObjectDocument* metaDocRef = nullptr;
	if (m_metaObject->ConvertToValue(metaDocRef)) {
		return GetValueByMetaID(*metaDocRef->GetDocumentPosted()).GetBoolean();
	}
	return false;
}

void CRecordDataObjectDocument::SetDeletionMark(bool deletionMark)
{
	WriteObject(
		eDocumentWriteMode::eDocumentWriteMode_UndoPosting,
		eDocumentPostingMode::eDocumentPostingMode_Regular
	);

	IRecordDataObjectRef::SetDeletionMark(deletionMark);
}

CSourceExplorer CRecordDataObjectDocument::GetSourceExplorer() const
{
	CSourceExplorer srcHelper(
		m_metaObject, GetClassType(),
		false
	);

	CMetaObjectDocument* metaRef = nullptr;

	if (m_metaObject->ConvertToValue(metaRef)) {
		srcHelper.AppendSource(metaRef->GetDocumentNumber(), false);
		srcHelper.AppendSource(metaRef->GetDocumentDate());
	}

	for (auto& obj : m_metaObject->GetObjectAttributes()) {
		srcHelper.AppendSource(obj);
	}

	for (auto& obj : m_metaObject->GetObjectTables()) {
		srcHelper.AppendSource(obj);
	}

	return srcHelper;
}

#pragma region _form_builder_h_
void CRecordDataObjectDocument::ShowFormValue(const wxString& strFormName, IBackendControlFrame* ownerControl)
{
	IBackendValueForm* const foundedForm = GetForm();

	if (foundedForm && foundedForm->IsShown()) {
		foundedForm->ActivateForm();
		return;
	}

	//if form is not initialized then generate  
	IBackendValueForm* valueForm = GetFormValue(strFormName, ownerControl);

	valueForm->Modify(m_objModified);
	valueForm->ShowForm();
}

IBackendValueForm* CRecordDataObjectDocument::GetFormValue(const wxString& strFormName, IBackendControlFrame* ownerControl)
{
	IBackendValueForm* const foundedForm = GetForm();

	if (foundedForm == nullptr) {

		IBackendValueForm* createdForm = m_metaObject->CreateAndBuildForm(
			strFormName,
			CMetaObjectDocument::eFormObject,
			ownerControl,
			this,
			m_objGuid
		);

		createdForm->CloseOnOwnerClose(false);
		return createdForm;
	}

	return foundedForm;
}
#pragma endregion

//***********************************************************************************************
//*                                   Document events                                            *
//***********************************************************************************************

#include "backend/backend_mainFrame.h"

bool CRecordDataObjectDocument::WriteObject(eDocumentWriteMode writeMode, eDocumentPostingMode postingMode)
{
	if (!appData->DesignerMode())
	{
		if (db_query != nullptr && !db_query->IsOpen())
			CBackendException::Error(_("database is not open!"));
		else if (db_query == nullptr)
			CBackendException::Error(_("database is not open!"));

		if (!CBackendException::IsEvalMode())
		{
			if (writeMode == eDocumentWriteMode::eDocumentWriteMode_Posting) {
				CValue deletionMark = false;
				CMetaObjectAttributeDefault* attributeDeletionMark = m_metaObject->GetDataDeletionMark();
				wxASSERT(attributeDeletionMark);
				IRecordDataObjectRef::GetValueByMetaID(*attributeDeletionMark, deletionMark);
				if (deletionMark.GetBoolean()) {
					CSystemFunction::Raise(_("failed to post object in db!"));
					return false;
				}
			}

			IBackendValueForm* const valueForm = GetForm();

			{
				db_query->BeginTransaction();

				{
					CValue cancel = false;
					m_procUnit->CallAsProc(wxT("BeforeWrite"), cancel, CValue::CreateEnumObject<CValueEnumDocumentWriteMode>(writeMode), CValue::CreateEnumObject<CValueEnumDocumentPostingMode>(postingMode));

					if (cancel.GetBoolean()) {
						db_query->RollBack(); CSystemFunction::Raise(_("failed to write object in db!"));
						return false;
					}

					CMetaObjectDocument* dataRef = nullptr;
					if (m_metaObject->ConvertToValue(dataRef)) {
						CMetaObjectAttributeDefault* metaPosted = dataRef->GetDocumentPosted();
						wxASSERT(metaPosted);
						if (writeMode == eDocumentWriteMode::eDocumentWriteMode_Posting)
							m_listObjectValue.insert_or_assign(metaPosted->GetMetaID(), true);
						else if (writeMode == eDocumentWriteMode::eDocumentWriteMode_UndoPosting)
							m_listObjectValue.insert_or_assign(metaPosted->GetMetaID(), false);
					}
				}

				bool newObject = CRecordDataObjectDocument::IsNewObject();

				//set current date if empty 
				CMetaObjectDocument* dataRef = nullptr;
				if (newObject && m_metaObject->ConvertToValue(dataRef)) {
					const CValue& docDate = GetValueByMetaID(*dataRef->GetDocumentDate());
					if (docDate.IsEmpty()) {
						SetValueByMetaID(*dataRef->GetDocumentDate(), CSystemFunction::CurrentDate());
					}
				}

				if (!CRecordDataObjectDocument::SaveData()) {
					db_query->RollBack();
					CSystemFunction::Raise(_("failed to write object in db!"));
					return false;
				}

				if (newObject) {
					m_registerRecords->CreateRecordSet();
				}

				if (writeMode == eDocumentWriteMode::eDocumentWriteMode_Posting) {
					CValue cancel = false;
					m_procUnit->CallAsProc(wxT("Posting"), cancel, CValue::CreateEnumObject<CValueEnumDocumentPostingMode>(postingMode));
					if (cancel.GetBoolean()) {
						db_query->RollBack();
						CSystemFunction::Raise(_("failed to write object in db!"));
						return false;
					}

					if (!m_registerRecords->WriteRecordSet()) {
						db_query->RollBack();
						CSystemFunction::Raise(_("failed to write object in db!"));
						return false;
					}
				}
				else if (writeMode == eDocumentWriteMode::eDocumentWriteMode_UndoPosting) {

					CValue cancel = false;
					m_procUnit->CallAsProc(wxT("UndoPosting"), cancel);
					if (cancel.GetBoolean()) {
						db_query->RollBack(); CSystemFunction::Raise(_("failed to write object in db!"));
						return false;
					}

					if (!m_registerRecords->DeleteRecordSet()) {
						db_query->RollBack();
						CSystemFunction::Raise(_("failed to write object in db!"));
						return false;
					}
				}
				{
					CValue cancel = false;
					m_procUnit->CallAsProc(wxT("OnWrite"), cancel);
					if (cancel.GetBoolean()) {
						db_query->RollBack();
						CSystemFunction::Raise(_("failed to write object in db!"));
						return false;
					}
				}

				db_query->Commit();

				if (newObject && valueForm != nullptr) valueForm->NotifyCreate(GetReference());
				else if (valueForm != nullptr) valueForm->NotifyChange(GetReference());

				if (backend_mainFrame != nullptr) backend_mainFrame->RefreshFrame();
			}

			m_objModified = false;
		}
	}

	return true;
}

bool CRecordDataObjectDocument::DeleteObject()
{
	if (!appData->DesignerMode())
	{
		if (db_query != nullptr && !db_query->IsOpen())
			CBackendException::Error(_("database is not open!"));
		else if (db_query == nullptr)
			CBackendException::Error(_("database is not open!"));

		if (!CBackendException::IsEvalMode())
		{
			IBackendValueForm* const valueForm = GetForm();

			{
				db_query->BeginTransaction();

				{
					CValue cancel = false;
					m_procUnit->CallAsProc(wxT("BeforeDelete"), cancel);
					if (cancel.GetBoolean()) {
						db_query->RollBack();
						CSystemFunction::Raise(_("failed to delete object in db!"));
						return false;
					}
				}

				if (!DeleteData()) {
					db_query->RollBack();
					CSystemFunction::Raise(_("failed to delete object in db!"));
					return false;
				}

				{
					CValue cancel = false;
					m_procUnit->CallAsProc(wxT("OnDelete"), cancel);
					if (cancel.GetBoolean()) {
						db_query->RollBack();
						CSystemFunction::Raise(_("failed to delete object in db!"));
						return false;
					}
				}

				db_query->Commit();

				if (valueForm != nullptr) valueForm->NotifyDelete(GetReference());
				if (backend_mainFrame != nullptr) backend_mainFrame->RefreshFrame();
			}
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////

enum Func {
	eIsNew,
	eCopy,
	eFill,
	eWrite,
	eDelete,
	eModified,
	eGetFormObject,
	eGetMetadata
};

enum Prop {
	eThisObject,
	eRegisterRecords
};

void CRecordDataObjectDocument::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	m_methodHelper->AppendFunc("isNew", "isNew()");
	m_methodHelper->AppendFunc("copy", "copy()");
	m_methodHelper->AppendFunc("fill", 1, "fill(object)");
	m_methodHelper->AppendFunc("write", "write()");
	m_methodHelper->AppendFunc("delete", "delete()");
	m_methodHelper->AppendFunc("modified", "modified()");
	m_methodHelper->AppendFunc("getFormObject", 2, "getFormObject(string, owner)");
	m_methodHelper->AppendFunc("getMetadata", "getMetadata()");

	m_methodHelper->AppendProp(wxT("thisObject"), true, false, eThisObject, eSystem);
	m_methodHelper->AppendProp(wxT("registerRecords"), true, false, eRegisterRecords, eSystem);

	//fill custom attributes 
	for (auto& obj : m_metaObject->GetGenericAttributes()) {
		if (obj->IsDeleted())
			continue;
		m_methodHelper->AppendProp(
			obj->GetName(),
			true,
			!m_metaObject->IsDataReference(obj->GetMetaID()),
			obj->GetMetaID(),
			eProperty
		);
	}

	//fill custom tables 
	for (auto& obj : m_metaObject->GetObjectTables()) {
		if (obj->IsDeleted())
			continue;
		m_methodHelper->AppendProp(
			obj->GetName(),
			true,
			false,
			obj->GetMetaID(),
			eTable
		);
	}

	if (m_procUnit != nullptr) {
		CByteCode* byteCode = m_procUnit->GetByteCode();
		if (byteCode != nullptr) {
			for (auto exportFunction : byteCode->m_listExportFunc) {
				m_methodHelper->AppendMethod(
					exportFunction.first,
					byteCode->GetNParams(exportFunction.second),
					byteCode->HasRetVal(exportFunction.second),
					exportFunction.second,
					eProcUnit
				);
			}
			for (auto exportVariable : byteCode->m_listExportVar) {
				m_methodHelper->AppendProp(
					exportVariable.first,
					exportVariable.second,
					eProcUnit
				);
			}
		}
	}

	m_registerRecords->PrepareNames();
}

bool CRecordDataObjectDocument::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	const long lPropAlias = m_methodHelper->GetPropAlias(lPropNum);
	if (lPropAlias == eProcUnit) {
		if (m_procUnit != nullptr) {
			return m_procUnit->SetPropVal(
				GetPropName(lPropNum), varPropVal
			);
		}
	}
	else if (lPropAlias == eProperty) {
		return SetValueByMetaID(
			m_methodHelper->GetPropData(lPropNum),
			varPropVal
		);
	}
	return false;
}

bool CRecordDataObjectDocument::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const long lPropAlias = m_methodHelper->GetPropAlias(lPropNum);
	if (lPropAlias == eProcUnit) {
		if (m_procUnit != nullptr) {
			return m_procUnit->GetPropVal(
				GetPropName(lPropNum), pvarPropVal
			);
		}
	}
	else if (lPropAlias == eProperty || lPropAlias == eTable) {
		const long lPropData = m_methodHelper->GetPropData(lPropNum);
		if (m_metaObject->IsDataReference(lPropData)) {
			pvarPropVal = GetReference();
			return true;
		}
		return GetValueByMetaID(lPropData, pvarPropVal);
	}
	else if (lPropAlias == eSystem) {
		switch (m_methodHelper->GetPropData(lPropNum))
		{
		case eRegisterRecords:
			pvarPropVal = m_registerRecords->GetValue();
			return true;
		case eThisObject:
			pvarPropVal = GetValue();
			return true;
		}
	}
	return false;
}

bool CRecordDataObjectDocument::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case eIsNew:
		pvarRetValue = m_newObject;
		return true;
	case eCopy:
		pvarRetValue = CopyObject();
		return true;
	case eFill:
		FillObject(*paParams[0]);
		return true;
	case eWrite:
		WriteObject(
			lSizeArray > 0 ? paParams[0]->ConvertToEnumValue<eDocumentWriteMode>() : eDocumentWriteMode::eDocumentWriteMode_Write,
			lSizeArray > 1 ? paParams[1]->ConvertToEnumValue<eDocumentPostingMode>() : eDocumentPostingMode::eDocumentPostingMode_RealTime
		);
		return true;
	case eDelete:
		DeleteObject();
		return true;
	case eModified:
		pvarRetValue = m_objModified;
		return true;
	case Func::eGetFormObject:
		pvarRetValue = GetFormValue(
			paParams[0]->GetString(),
			paParams[1]->ConvertToType<IBackendControlFrame>()
		);
		return true;
	case Func::eGetMetadata:
		pvarRetValue = m_metaObject;
		return true;
	}

	return IModuleDataObject::ExecuteFunc(
		GetMethodName(lMethodNum), pvarRetValue, paParams, lSizeArray
	);
}

enum {
	enWriteRegister = 0
};

void CRecordDataObjectDocument::CRecorderRegisterDocument::PrepareNames() const {
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendFunc("write", "write()");
	for (auto& pair : m_records) {
		IRecordSetObject* record = pair.second;
		wxASSERT(record);
		IMetaObjectRegisterData* metaObject =
			record->GetMetaObject();
		wxASSERT(metaObject);
		m_methodHelper->AppendProp(
			metaObject->GetName(), true, false, pair.first
		);
	}
}

bool CRecordDataObjectDocument::CRecorderRegisterDocument::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	return false;
}

bool CRecordDataObjectDocument::CRecorderRegisterDocument::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	auto& it = m_records.find(m_methodHelper->GetPropData(lPropNum));
	if (it != m_records.end()) {
		pvarPropVal = it->second;
		return true;
	}
	return false;
}

bool CRecordDataObjectDocument::CRecorderRegisterDocument::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enWriteRegister:
		WriteRecordSet();
		return true;
	}

	return false;
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

SYSTEM_TYPE_REGISTER(CRecordDataObjectDocument::CRecorderRegisterDocument, "recorderRegister", string_to_clsid("VL_RGST"));