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
wxIMPLEMENT_DYNAMIC_CLASS(CValueRecordDataObjectDocument::CRecorderRegisterDocument, CValue);
////////////////////////////////////////////////////////////////////////////////////////////////////

void CValueRecordDataObjectDocument::CRecorderRegisterDocument::CreateRecordSet()
{
	CValueMetaObjectDocument* metaDocument = dynamic_cast<CValueMetaObjectDocument*>(m_document->GetMetaObject());
	wxASSERT(metaDocument);
	IMetaData* metaData = metaDocument->GetMetaData();
	wxASSERT(metaData);

	CRecorderRegisterDocument::ClearRecordSet();

	const CMetaDescription& metaDesc = metaDocument->GetRecordDescription();
	for (unsigned int idx = 0; idx < metaDesc.GetTypeCount(); idx++) {
		IValueMetaObjectRegisterData* metaObject = metaData->FindAnyObjectByFilter<IValueMetaObjectRegisterData>(metaDesc.GetByIdx(idx));
		if (metaObject == nullptr || !metaObject->IsAllowed())
			continue;
		CValueMetaObjectAttributePredefined* registerRecord = metaObject->GetRegisterRecorder();
		wxASSERT(registerRecord);
		CValuePtr<IValueRecordSetObject> recordSet(metaObject->CreateRecordSetObjectValue());
		recordSet->SetKeyValue(registerRecord->GetMetaID(), m_document->GetReference());
		m_records.insert_or_assign(metaObject->GetMetaID(), recordSet);
	}

	PrepareNames();
}

bool CValueRecordDataObjectDocument::CRecorderRegisterDocument::WriteRecordSet()
{
	for (auto& pair : m_records) {
		IValueRecordSetObject* record = pair.second;
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

bool CValueRecordDataObjectDocument::CRecorderRegisterDocument::DeleteRecordSet()
{
	for (auto& pair : m_records) {
		IValueRecordSetObject* record = pair.second;
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

void CValueRecordDataObjectDocument::CRecorderRegisterDocument::ClearRecordSet()
{
	m_records.clear();
}

void CValueRecordDataObjectDocument::CRecorderRegisterDocument::RefreshRecordSet()
{
	for (auto& pair : m_records) {
		IValueRecordSetObject* record = pair.second;
		wxASSERT(record);

		IValueMetaObjectRegisterData* object = record->GetMetaObject();
		wxASSERT(object);
		IBackendValueForm* backendFrame = IBackendValueForm::FindFormBySourceUniqueKey(object->GetGuid());
		if (backendFrame != nullptr) backendFrame->UpdateForm();
	}
}

CValueRecordDataObjectDocument::CRecorderRegisterDocument::CRecorderRegisterDocument(CValueRecordDataObjectDocument* currentDoc) :
	CValue(eValueTypes::TYPE_VALUE), m_document(currentDoc), m_methodHelper(new CMethodHelper())
{
	CRecorderRegisterDocument::CreateRecordSet();
}

CValueRecordDataObjectDocument::CRecorderRegisterDocument::~CRecorderRegisterDocument()
{
	CRecorderRegisterDocument::ClearRecordSet();
	wxDELETE(m_methodHelper);
}

//*********************************************************************************************
//*                                  CValueRecordDataObjectDocument	                                      *
//*********************************************************************************************

CValueRecordDataObjectDocument::CValueRecordDataObjectDocument(CValueMetaObjectDocument* metaObject, const CGuid& objGuid) :
	IValueRecordDataObjectRef(metaObject, objGuid),
	m_registerRecords(new CRecorderRegisterDocument(this))
{
}

CValueRecordDataObjectDocument::CValueRecordDataObjectDocument(const CValueRecordDataObjectDocument& source) :
	IValueRecordDataObjectRef(source),
	m_registerRecords(new CRecorderRegisterDocument(this))
{
}

CValueRecordDataObjectDocument::~CValueRecordDataObjectDocument()
{
}

bool CValueRecordDataObjectDocument::IsPosted() const
{
	CValueMetaObjectDocument* metaDocRef = nullptr;
	if (m_metaObject->ConvertToValue(metaDocRef)) {
		return GetValueByMetaID(*metaDocRef->GetDocumentPosted()).GetBoolean();
	}
	return false;
}

void CValueRecordDataObjectDocument::SetDeletionMark(bool deletionMark)
{
	WriteObject(
		eDocumentWriteMode::eDocumentWriteMode_UndoPosting,
		eDocumentPostingMode::eDocumentPostingMode_Regular
	);

	IValueRecordDataObjectRef::SetDeletionMark(deletionMark);
}

CSourceExplorer CValueRecordDataObjectDocument::GetSourceExplorer() const
{
	CSourceExplorer srcHelper(
		m_metaObject, GetClassType(),
		false
	);

	CValueMetaObjectDocument* metaRef = nullptr;

	if (m_metaObject->ConvertToValue(metaRef)) {
		srcHelper.AppendSource(metaRef->GetDocumentNumber(), false);
		srcHelper.AppendSource(metaRef->GetDocumentDate());
	}

	for (const auto object : m_metaObject->GetAttributeArrayObject()) {
		srcHelper.AppendSource(object);
	}

	for (const auto object : m_metaObject->GetTableArrayObject()) {
		srcHelper.AppendSource(object);
	}

	return srcHelper;
}

#pragma region _form_builder_h_
void CValueRecordDataObjectDocument::ShowFormValue(const wxString& strFormName, IBackendControlFrame* ownerControl)
{
	IBackendValueForm* const foundedForm = GetForm();

	if (foundedForm && foundedForm->IsShown()) {
		foundedForm->ActivateForm();
		return;
	}

	//if form is not initialized then generate  
	IBackendValueForm* const valueForm = 
		GetFormValue(strFormName, ownerControl);

	if (valueForm != nullptr) {
		valueForm->Modify(m_objModified);
		valueForm->ShowForm();
	}
}

IBackendValueForm* CValueRecordDataObjectDocument::GetFormValue(const wxString& strFormName, IBackendControlFrame* ownerControl)
{
	IBackendValueForm* const foundedForm = GetForm();

	if (foundedForm == nullptr) {

		IBackendValueForm* createdForm = m_metaObject->CreateAndBuildForm(
			strFormName,
			CValueMetaObjectDocument::eFormObject,
			ownerControl,
			this,
			m_objGuid
		);

		if (createdForm != nullptr)
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

bool CValueRecordDataObjectDocument::WriteObject(eDocumentWriteMode writeMode, eDocumentPostingMode postingMode)
{
	if (!appData->DesignerMode())
	{
		if (db_query != nullptr && !db_query->IsOpen())
			CBackendCoreException::Error(_("Database is not open!"));
		else if (db_query == nullptr)
			CBackendCoreException::Error(_("Database is not open!"));

		if (!CBackendException::IsEvalMode())
		{
			if (!m_metaObject->AccessRight_Write()) {
				CBackendAccessException::Error();
				return false;
			}

			if (writeMode == eDocumentWriteMode::eDocumentWriteMode_Posting) {
				CValue deletionMark = false;
				CValueMetaObjectAttributePredefined* attributeDeletionMark = m_metaObject->GetDataDeletionMark();
				wxASSERT(attributeDeletionMark);
				IValueRecordDataObjectRef::GetValueByMetaID(*attributeDeletionMark, deletionMark);
				if (deletionMark.GetBoolean()) {
					CBackendCoreException::Error(_("Failed to post object in db!"));
					return false;
				}
			}

			CTransactionGuard db_query_active_transaction = db_query;
			{
				IBackendValueForm* const valueForm = GetForm();
				{
					db_query_active_transaction.BeginTransaction();

					{
						CValue cancel = false;
						m_procUnit->CallAsProc(wxT("BeforeWrite"), cancel,
							CValue::CreateEnumObject<CValueEnumDocumentWriteMode>(writeMode),
							CValue::CreateEnumObject<CValueEnumDocumentPostingMode>(postingMode)
						);

						if (cancel.GetBoolean()) {
							db_query_active_transaction.RollBackTransaction();
							CBackendCoreException::Error(_("Failed to write object in db!"));
							return false;
						}

						CValueMetaObjectDocument* dataRef = nullptr;
						if (m_metaObject->ConvertToValue(dataRef)) {
							CValueMetaObjectAttributePredefined* metaPosted = dataRef->GetDocumentPosted();
							wxASSERT(metaPosted);
							if (writeMode == eDocumentWriteMode::eDocumentWriteMode_Posting)
								m_listObjectValue.insert_or_assign(metaPosted->GetMetaID(), true);
							else if (writeMode == eDocumentWriteMode::eDocumentWriteMode_UndoPosting)
								m_listObjectValue.insert_or_assign(metaPosted->GetMetaID(), false);
						}
					}

					bool newObject = CValueRecordDataObjectDocument::IsNewObject();
					bool generateUniqueIdentifier = false;

					if (!IsSetUniqueIdentifier()) {
						CValue prefix = "", standartProcessing = true;
						m_procUnit->CallAsProc(wxT("SetNewNumber"), prefix, standartProcessing);
						if (standartProcessing.GetBoolean()) {
							generateUniqueIdentifier =
								CValueRecordDataObjectDocument::GenerateUniqueIdentifier(prefix.GetString());
						}
					}

					//set current date if empty 
					CValueMetaObjectDocument* dataRef = nullptr;
					if (newObject && m_metaObject->ConvertToValue(dataRef)) {
						const CValue& docDate = GetValueByMetaID(*dataRef->GetDocumentDate());
						if (docDate.IsEmpty()) {
							SetValueByMetaID(*dataRef->GetDocumentDate(), CSystemFunction::CurrentDate());
						}
					}

					if (!CValueRecordDataObjectDocument::SaveData()) {
						if (generateUniqueIdentifier)
							CValueRecordDataObjectDocument::ResetUniqueIdentifier();
						db_query_active_transaction.RollBackTransaction();
						CBackendCoreException::Error(_("Failed to write object in db!"));
						return false;
					}

					if (newObject) {
						m_registerRecords->CreateRecordSet();
					}

					if (writeMode == eDocumentWriteMode::eDocumentWriteMode_Posting) {

						CValue cancel = false;
						m_procUnit->CallAsProc(wxT("Posting"), cancel,
							CValue::CreateEnumObject<CValueEnumDocumentPostingMode>(postingMode)
						);

						if (cancel.GetBoolean()) {
							if (generateUniqueIdentifier)
								CValueRecordDataObjectDocument::ResetUniqueIdentifier();
							db_query_active_transaction.RollBackTransaction();
							CBackendCoreException::Error(_("Failed to write object in db!"));
							return false;
						}

						if (!m_registerRecords->WriteRecordSet()) {
							if (generateUniqueIdentifier)
								CValueRecordDataObjectDocument::ResetUniqueIdentifier();
							db_query_active_transaction.RollBackTransaction();
							CBackendCoreException::Error(_("Failed to write object in db!"));
							return false;
						}
					}
					else if (writeMode == eDocumentWriteMode::eDocumentWriteMode_UndoPosting) {

						CValue cancel = false;
						m_procUnit->CallAsProc(wxT("UndoPosting"), cancel);
						if (cancel.GetBoolean()) {
							if (generateUniqueIdentifier)
								CValueRecordDataObjectDocument::ResetUniqueIdentifier();
							db_query_active_transaction.RollBackTransaction();
							CBackendCoreException::Error(_("Failed to write object in db!"));
							return false;
						}

						if (!m_registerRecords->DeleteRecordSet()) {
							if (generateUniqueIdentifier)
								CValueRecordDataObjectDocument::ResetUniqueIdentifier();
							db_query_active_transaction.RollBackTransaction();
							CBackendCoreException::Error(_("Failed to write object in db!"));
							return false;
						}
					}
					{
						CValue cancel = false;
						m_procUnit->CallAsProc(wxT("OnWrite"), cancel);
						if (cancel.GetBoolean()) {
							if (generateUniqueIdentifier)
								CValueRecordDataObjectDocument::ResetUniqueIdentifier();
							db_query_active_transaction.RollBackTransaction();
							CBackendCoreException::Error(_("Failed to write object in db!"));
							return false;
						}
					}

					db_query_active_transaction.CommitTransaction();

					if (newObject && valueForm != nullptr) valueForm->NotifyCreate(GetReference());
					else if (valueForm != nullptr) valueForm->NotifyChange(GetReference());

					m_registerRecords->RefreshRecordSet();
				}

				m_objModified = false;
			}
		}
	}

	return true;
}

bool CValueRecordDataObjectDocument::DeleteObject()
{
	if (!appData->DesignerMode())
	{
		if (db_query != nullptr && !db_query->IsOpen())
			CBackendCoreException::Error(_("Database is not open!"));
		else if (db_query == nullptr)
			CBackendCoreException::Error(_("Database is not open!"));

		if (!CBackendException::IsEvalMode())
		{
			if (!m_metaObject->AccessRight_Delete()) {
				CBackendAccessException::Error();
				return false;
			}

			CTransactionGuard db_query_active_transaction = db_query;
			{
				IBackendValueForm* const valueForm = GetForm();
				{
					db_query_active_transaction.BeginTransaction();
					{
						CValue cancel = false;
						m_procUnit->CallAsProc(wxT("BeforeDelete"), cancel);
						if (cancel.GetBoolean()) {
							db_query_active_transaction.RollBackTransaction();
							CBackendCoreException::Error(_("Failed to delete object in db!"));
							return false;
						}
					}

					if (!m_registerRecords->DeleteRecordSet()) {
						db_query_active_transaction.RollBackTransaction();
						CBackendCoreException::Error(_("Failed to write object in db!"));
						return false;
					}

					{
						CValue cancel = false;
						m_procUnit->CallAsProc(wxT("OnDelete"), cancel);

						if (cancel.GetBoolean()) {
							db_query_active_transaction.RollBackTransaction();
							CBackendCoreException::Error(_("Failed to delete object in db!"));
							return false;
						}
					}

					if (!DeleteData()) {
						db_query_active_transaction.RollBackTransaction();
						CBackendCoreException::Error(_("Failed to delete object in db!"));
						return false;
					}

					db_query_active_transaction.CommitTransaction();

					if (valueForm != nullptr) valueForm->NotifyDelete(GetReference());

					m_registerRecords->RefreshRecordSet();
				}
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

void CValueRecordDataObjectDocument::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	m_methodHelper->AppendFunc("isNew", "isNew()");
	m_methodHelper->AppendFunc("copy", "copy()");
	m_methodHelper->AppendFunc("fill", 1, "fill(object)");
	m_methodHelper->AppendFunc("write", 2, "write(writeMode, postingMode)");
	m_methodHelper->AppendFunc("delete", "delete()");
	m_methodHelper->AppendFunc("modified", "modified()");
	m_methodHelper->AppendFunc("getFormObject", 2, "getFormObject(string, owner)");
	m_methodHelper->AppendFunc("getMetadata", "getMetadata()");

	m_methodHelper->AppendProp(wxT("thisObject"), true, false, eThisObject, eSystem);
	m_methodHelper->AppendProp(wxT("registerRecords"), true, false, eRegisterRecords, eSystem);

	//set object name 
	wxString objectName;

	//fill custom attributes 
	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		if (object->IsDeleted())
			continue;
		if (!object->GetObjectNameAsString(objectName))
			continue;
		m_methodHelper->AppendProp(
			objectName,
			true,
			!m_metaObject->IsDataReference(object->GetMetaID()),
			object->GetMetaID(),
			eProperty
		);
	}

	//fill custom tables 
	for (const auto object : m_metaObject->GetTableArrayObject()) {
		if (object->IsDeleted())
			continue;
		if (!object->GetObjectNameAsString(objectName))
			continue;
		m_methodHelper->AppendProp(
			objectName,
			true,
			false,
			object->GetMetaID(),
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

bool CValueRecordDataObjectDocument::SetPropVal(const long lPropNum, const CValue& varPropVal)
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

bool CValueRecordDataObjectDocument::GetPropVal(const long lPropNum, CValue& pvarPropVal)
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

bool CValueRecordDataObjectDocument::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
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
			lSizeArray > 0 ? paParams[0]->GetString() : wxEmptyString,
			lSizeArray > 1 ? paParams[1]->ConvertToType<IBackendControlFrame>() : nullptr
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

void CValueRecordDataObjectDocument::CRecorderRegisterDocument::PrepareNames() const {
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendFunc("write", "write()");
	for (auto& pair : m_records) {
		IValueRecordSetObject* record = pair.second;
		wxASSERT(record);
		IValueMetaObjectRegisterData* metaObject =
			record->GetMetaObject();
		wxASSERT(metaObject);
		m_methodHelper->AppendProp(
			metaObject->GetName(), true, false, pair.first
		);
	}
}

bool CValueRecordDataObjectDocument::CRecorderRegisterDocument::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	return false;
}

bool CValueRecordDataObjectDocument::CRecorderRegisterDocument::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	auto& it = m_records.find(m_methodHelper->GetPropData(lPropNum));
	if (it != m_records.end()) {
		pvarPropVal = it->second;
		return true;
	}
	return false;
}

bool CValueRecordDataObjectDocument::CRecorderRegisterDocument::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
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

SYSTEM_TYPE_REGISTER(CValueRecordDataObjectDocument::CRecorderRegisterDocument, "recorderRegister", string_to_clsid("VL_RGST"));