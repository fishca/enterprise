////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : catalog object
////////////////////////////////////////////////////////////////////////////

#include "catalog.h"
#include "backend/metaData.h"

#include "backend/appData.h"
#include "reference/reference.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/system/systemManager.h"

#include "backend/fileSystem/fs.h"
//*********************************************************************************************
//*                                  ObjectCatalogValue                                       *
//*********************************************************************************************

CRecordDataObjectCatalog::CRecordDataObjectCatalog(CMetaObjectCatalog* metaObject, const CGuid& objGuid, eObjectMode objMode) :
	IRecordDataObjectFolderRef(metaObject, objGuid, objMode)
{
}

CRecordDataObjectCatalog::CRecordDataObjectCatalog(const CRecordDataObjectCatalog& source) :
	IRecordDataObjectFolderRef(source)
{
}

CSourceExplorer CRecordDataObjectCatalog::GetSourceExplorer() const
{
	CSourceExplorer srcHelper(
		m_metaObject, GetClassType(),
		false
	);

	CMetaObjectCatalog* metaRef = nullptr;

	if (m_metaObject->ConvertToValue(metaRef)) {
		srcHelper.AppendSource(metaRef->GetDataCode(), false);
		srcHelper.AppendSource(metaRef->GetDataDescription());
		CMetaObjectAttributePredefined* defOwner = metaRef->GetCatalogOwner();
		if (defOwner != nullptr && defOwner->GetClsidCount() > 0) {
			srcHelper.AppendSource(metaRef->GetCatalogOwner());
		}
		srcHelper.AppendSource(metaRef->GetDataParent());
	}

	for (const auto object : m_metaObject->GetAttributeArrayObject()) {
		eItemMode attrUse = object->GetItemMode();
		if (m_objMode == eObjectMode::OBJECT_ITEM) {
			if (attrUse == eItemMode::eItemMode_Item
				|| attrUse == eItemMode::eItemMode_Folder_Item) {
				if (!m_metaObject->IsDataReference(object->GetMetaID())) {
					srcHelper.AppendSource(object);
				}
			}
		}
		else {
			if (attrUse == eItemMode::eItemMode_Folder ||
				attrUse == eItemMode::eItemMode_Folder_Item) {
				if (!m_metaObject->IsDataReference(object->GetMetaID())) {
					srcHelper.AppendSource(object);
				}
			}
		}
	}

	for (const auto object : m_metaObject->GetTableArrayObject()) {
		eItemMode tableUse = object->GetTableUse();
		if (m_objMode == eObjectMode::OBJECT_ITEM) {
			if (tableUse == eItemMode::eItemMode_Item
				|| tableUse == eItemMode::eItemMode_Folder_Item) {
				srcHelper.AppendSource(object);
			}
		}
		else {
			if (tableUse == eItemMode::eItemMode_Folder ||
				tableUse == eItemMode::eItemMode_Folder_Item) {
				srcHelper.AppendSource(object);
			}
		}
	}

	return srcHelper;
}

#pragma region _form_builder_h_
void CRecordDataObjectCatalog::ShowFormValue(const wxString& strFormName, IBackendControlFrame* ownerControl)
{
	IBackendValueForm* const foundedForm = GetForm();

	if (foundedForm && foundedForm->IsShown()) {
		foundedForm->ActivateForm();
		return;
	}

	//if form is not initialized then generate  
	IBackendValueForm* valueForm =
		GetFormValue(strFormName, ownerControl);

	valueForm->Modify(m_objModified);
	valueForm->ShowForm();
}

IBackendValueForm* CRecordDataObjectCatalog::GetFormValue(const wxString& strFormName, IBackendControlFrame* ownerControl)
{
	IBackendValueForm* const foundedForm = GetForm();

	if (foundedForm == nullptr) {

		IBackendValueForm* createdForm = m_metaObject->CreateAndBuildForm(
			strFormName,
			m_objMode == eObjectMode::OBJECT_ITEM ? CMetaObjectCatalog::eFormObject : CMetaObjectCatalog::eFormFolder,
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
//*                                   Catalog events                                            *
//***********************************************************************************************

#include "backend/backend_mainFrame.h"

bool CRecordDataObjectCatalog::WriteObject()
{
	if (!appData->DesignerMode())
	{
		if (db_query != nullptr && !db_query->IsOpen())
			CBackendException::Error(_("Database is not open!"));
		else if (db_query == nullptr)
			CBackendException::Error(_("Database is not open!"));

		if (!CBackendException::IsEvalMode()) 
		{
			if (!m_metaObject->AccessRight_Write()) {
				CSystemFunction::Raise(_("Not enough access rights for this user!"));
				return false;
			}

			CTransactionGuard db_query_active_transaction = db_query;
			{
				IBackendValueForm* const valueForm = GetForm();
				{
					db_query_active_transaction.BeginTransaction();

					{
						CValue cancel = false;
						m_procUnit->CallAsProc(wxT("BeforeWrite"), cancel);

						if (cancel.GetBoolean()) {
							db_query_active_transaction.RollBackTransaction();
							CSystemFunction::Raise(_("Failed to write object in db!"));
							return false;
						}
					}

					bool newObject = CRecordDataObjectCatalog::IsNewObject();
					bool generateUniqueIdentifier = false;
					
					if (!IsSetUniqueIdentifier()) {
						CValue prefix = "", standartProcessing = true;
						m_procUnit->CallAsProc(wxT("SetNewCode"), prefix, standartProcessing);
						if (standartProcessing.GetBoolean()) {
							generateUniqueIdentifier = 
								CRecordDataObjectCatalog::GenerateUniqueIdentifier(prefix.GetString());
						}
					}

					if (!SaveData()) {
						if (generateUniqueIdentifier)
							CRecordDataObjectCatalog::ResetUniqueIdentifier();
						db_query_active_transaction.RollBackTransaction();
						CSystemFunction::Raise(_("Failed to write object in db!"));
						return false;
					}

					{
						CValue cancel = false;
						m_procUnit->CallAsProc(wxT("OnWrite"), cancel);
						if (cancel.GetBoolean()) {
							if (generateUniqueIdentifier)
								CRecordDataObjectCatalog::ResetUniqueIdentifier();
							db_query_active_transaction.RollBackTransaction();
							CSystemFunction::Raise(_("Failed to write object in db!"));
							return false;
						}
					}

					db_query_active_transaction.CommitTransaction();

					if (newObject && valueForm != nullptr) valueForm->NotifyCreate(GetReference());
					else if (valueForm != nullptr) valueForm->NotifyChange(GetReference());
				}
				m_objModified = false;
			}
		}
	}

	return true;
}

bool CRecordDataObjectCatalog::DeleteObject()
{
	if (!appData->DesignerMode())
	{
		if (db_query != nullptr && !db_query->IsOpen())
			CBackendException::Error(_("Database is not open!"));
		else if (db_query == nullptr)
			CBackendException::Error(_("Database is not open!"));

		if (!CBackendException::IsEvalMode())
		{
			if (!m_metaObject->AccessRight_Delete()) {
				CSystemFunction::Raise(_("Not enough access rights for this user!"));
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
							CSystemFunction::Raise(_("Failed to delete object in db!"));
							return false;
						}
					}

					if (!DeleteData()) {
						db_query_active_transaction.RollBackTransaction();
						CSystemFunction::Raise(_("Failed to delete object in db!")); 
						return false;
					}

					{
						CValue cancel = false;
						m_procUnit->CallAsProc(wxT("OnDelete"), cancel);
						if (cancel.GetBoolean()) {
							db_query_active_transaction.RollBackTransaction();
							CSystemFunction::Raise(_("Failed to delete object in db!")); 
							return false;
						}
					}

					db_query_active_transaction.CommitTransaction();

					if (valueForm != nullptr) valueForm->NotifyDelete(GetReference());
				}
			}
		}
	}

	return true;
}

enum Func {
	enIsNew = 0,
	enCopy,
	enFill,
	enWrite,
	enDelete,
	enModified,
	enGetForm,
	enGetMetadata
};

//****************************************************************************
//*                              Support methods                             *
//****************************************************************************

void CRecordDataObjectCatalog::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	m_methodHelper->AppendFunc("isNew", "isNew()");
	m_methodHelper->AppendFunc("copy", "copy()");
	m_methodHelper->AppendFunc("fill", 1, "fill(object)");
	m_methodHelper->AppendFunc("write", "write()");
	m_methodHelper->AppendFunc("delete", "delete()");
	m_methodHelper->AppendFunc("modified", "modified()");
	m_methodHelper->AppendFunc("getFormObject", 3, "getFormObject(string, owner, guid)");
	m_methodHelper->AppendFunc("getMetadata", "getMetadata()");

	m_methodHelper->AppendProp(wxT("thisObject"), true, false, eThisObject, eSystem);

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
}

bool CRecordDataObjectCatalog::SetPropVal(const long lPropNum, const CValue& varPropVal)
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

bool CRecordDataObjectCatalog::GetPropVal(const long lPropNum, CValue& pvarPropVal)
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
		case eThisObject:
			pvarPropVal = GetValue();
			return true;
		}
	}
	return false;
}

bool CRecordDataObjectCatalog::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enIsNew:
		pvarRetValue = m_newObject;
		return true;
	case enCopy:
		pvarRetValue = CopyObject();
		return true;
	case enFill:
		FillObject(*paParams[0]);
		return true;
	case enWrite:
		WriteObject();
		return true;
	case enDelete:
		DeleteObject();
		return true;
	case enModified:
		pvarRetValue = m_objModified;
		return true;
	case enGetForm:
		pvarRetValue = GetFormValue(
			lSizeArray > 0 ? paParams[0]->GetString() : wxEmptyString,
			lSizeArray > 1 ? paParams[1]->ConvertToType<IBackendControlFrame>() : nullptr
		);
		return true;
	case enGetMetadata:
		pvarRetValue = m_metaObject;
		return true;
	}

	return IModuleDataObject::ExecuteFunc(
		GetMethodName(lMethodNum), pvarRetValue, paParams, lSizeArray
	);
}