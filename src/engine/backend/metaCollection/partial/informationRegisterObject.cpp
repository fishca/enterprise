#include "informationRegister.h"

#include "backend/appData.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/system/systemManager.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

#include "backend/backend_mainFrame.h"

bool CValueRecordSetObjectInformationRegister::WriteRecordSet(bool replace, bool clearTable)
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

			CTransactionGuard db_query_active_transaction = db_query;
			{
				db_query_active_transaction.BeginTransaction();

				{
					CValue cancel = false;
					m_procUnit->CallAsProc(wxT("BeforeWrite"), cancel);

					if (cancel.GetBoolean()) {
						db_query_active_transaction.RollBackTransaction();
						CBackendCoreException::Error(_("Failed to write object in db!"));
						return false;
					}
				}

				if (!SaveData(replace, clearTable)) {
					db_query_active_transaction.RollBackTransaction();
					CBackendCoreException::Error(_("Failed to write object in db!"));
					return false;
				}

				{
					CValue cancel = false;
					m_procUnit->CallAsProc(wxT("OnWrite"), cancel);
					if (cancel.GetBoolean()) {
						db_query_active_transaction.RollBackTransaction();
						CBackendCoreException::Error(_("Failed to write object in db!"));
						return false;
					}
				}

				db_query_active_transaction.CommitTransaction();
			}

			m_objModified = false;
		}
	}

	return true;
}

bool CValueRecordSetObjectInformationRegister::DeleteRecordSet()
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
				db_query_active_transaction.BeginTransaction();

				{
					CValue cancel = false;
					m_procUnit->CallAsProc(wxT("BeforeWrite"), cancel);

					if (cancel.GetBoolean()) {
						db_query_active_transaction.RollBackTransaction();
						CBackendCoreException::Error(_("Failed to write object in db!"));
						return false;
					}
				}

				if (!DeleteData()) {
					db_query_active_transaction.RollBackTransaction();
					CBackendCoreException::Error(_("Failed to write object in db!"));
					return false;
				}

				{
					CValue cancel = false;
					m_procUnit->CallAsProc(wxT("OnWrite"), cancel);
					if (cancel.GetBoolean()) {
						db_query_active_transaction.RollBackTransaction();
						CBackendCoreException::Error(_("Failed to write object in db!"));
						return false;
					}
				}

				db_query_active_transaction.CommitTransaction();
			}

			m_objModified = false;
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CSourceExplorer CValueRecordManagerObjectInformationRegister::GetSourceExplorer() const
{
	CSourceExplorer srcHelper(
		m_metaObject, GetClassType(),
		false
	);

	CValueMetaObjectInformationRegister* metaRef = nullptr;

	if (m_metaObject->ConvertToValue(metaRef)) {
		if (metaRef->GetPeriodicity() != ePeriodicity::eNonPeriodic) {
			srcHelper.AppendSource(metaRef->GetRegisterPeriod());
		}
	}

	for (const auto object : m_metaObject->GetDimentionArrayObject()) {
		srcHelper.AppendSource(object);
	}

	for (const auto object : m_metaObject->GetResourceArrayObject()) {
		srcHelper.AppendSource(object);
	}

	for (const auto object : m_metaObject->GetAttributeArrayObject()) {
		srcHelper.AppendSource(object);
	}

	return srcHelper;
}

#pragma region _form_builder_h_
void CValueRecordManagerObjectInformationRegister::ShowFormValue(const wxString& strFormName, IBackendControlFrame* ownerControl)
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
		valueForm->Modify(m_recordSet->IsModified());
		valueForm->ShowForm();
	}
}

IBackendValueForm* CValueRecordManagerObjectInformationRegister::GetFormValue(const wxString& strFormName, IBackendControlFrame* ownerControl)
{
	IBackendValueForm* const foundedForm = GetForm();

	if (foundedForm == nullptr) {

		IBackendValueForm* createdForm = m_metaObject->CreateAndBuildForm(
			strFormName,
			CValueMetaObjectInformationRegister::eFormRecord,
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

bool CValueRecordManagerObjectInformationRegister::WriteRegister(bool replace)
{
	if (!appData->DesignerMode())
	{
		if (db_query != nullptr && !db_query->IsOpen())
			CBackendCoreException::Error(_("Database is not open!"));
		else if (db_query == nullptr)
			CBackendCoreException::Error(_("Database is not open!"));

		if (!CBackendException::IsEvalMode())
		{
			CTransactionGuard db_query_active_transaction = db_query;
			{
				db_query_active_transaction.BeginTransaction();

				bool newObject = CValueRecordManagerObjectInformationRegister::IsNewObject();

				if (!SaveData()) {
					db_query_active_transaction.RollBackTransaction();
					CBackendCoreException::Error(_("failed to save object in db!"));
					return false;
				}

				IBackendValueForm::UpdateFormUniqueKey(m_objGuid);

				db_query_active_transaction.CommitTransaction();

				IBackendValueForm* const valueForm = GetForm();

				if (newObject && valueForm != nullptr) valueForm->NotifyCreate(GetValue());
				else if (valueForm != nullptr) valueForm->NotifyChange(GetValue());
			}

			m_recordSet->Modify(false);
		}
	}

	return true;
}

bool CValueRecordManagerObjectInformationRegister::DeleteRegister()
{
	if (!appData->DesignerMode())
	{
		if (db_query != nullptr && !db_query->IsOpen())
			CBackendCoreException::Error(_("Database is not open!"));
		else if (db_query == nullptr)
			CBackendCoreException::Error(_("Database is not open!"));

		if (!CBackendException::IsEvalMode())
		{
			CTransactionGuard db_query_active_transaction = db_query;
			{
				IBackendValueForm* const valueForm = GetForm();
				{
					db_query_active_transaction.BeginTransaction();

					if (!DeleteData()) {
						db_query_active_transaction.RollBackTransaction();
						CBackendCoreException::Error(_("Failed to delete object in db!"));
						return false;
					}

					db_query_active_transaction.CommitTransaction();

					if (valueForm != nullptr) valueForm->NotifyDelete(GetValue());
				}
				m_recordSet->Modify(false);
			}
		}
	}

	return true;
}

enum recordManager
{
	enCopyRecordManager,
	enWriteRecordManager,
	enDeleteRecordManager,
	enModifiedRecordManager,
	enReadRecordManager,
	enSelectedRecordManager,
	enGetFormRecord,
	enGetMetadataRecordManager
};

enum recordSet
{
	enAdd = 0,
	enCount,
	enClear,
	enLoad,
	enUnload,
	enWriteRecordSet,
	enModifiedRecordSet,
	enReadRecordSet,
	enSelectedRecordSet,
	enGetMetadataRecordSet,
};

enum prop
{
	eThisObject,
	eFilter
};

//****************************************************************************
//*                              Support methods                             *
//****************************************************************************

void CValueRecordSetObjectInformationRegister::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	m_methodHelper->AppendFunc("add", "add()");
	m_methodHelper->AppendFunc("count", "count()");
	m_methodHelper->AppendFunc("clear", "clear()");
	m_methodHelper->AppendFunc("write", 1, "write(replace)");
	m_methodHelper->AppendFunc("load", 1, "load(table)");
	m_methodHelper->AppendFunc("unload", "unload()");
	m_methodHelper->AppendFunc("modified", "modified()");
	m_methodHelper->AppendFunc("read", "read()");
	m_methodHelper->AppendFunc("selected", "selected()");
	m_methodHelper->AppendFunc("getMetadata", "getMetadata()");

	m_methodHelper->AppendProp(wxT("thisObject"), true, false, prop::eThisObject, wxNOT_FOUND);
	m_methodHelper->AppendProp(wxT("filter"), true, false, prop::eFilter, wxNOT_FOUND);
}

void CValueRecordManagerObjectInformationRegister::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	m_methodHelper->AppendFunc("copy", "copy()");
	m_methodHelper->AppendFunc("write", 1, "write(replace)");
	m_methodHelper->AppendFunc("delete", "delete()");
	m_methodHelper->AppendFunc("modified", "modified()");
	m_methodHelper->AppendFunc("read", "read()");
	m_methodHelper->AppendFunc("selected", "selected()");
	m_methodHelper->AppendFunc("getFormRecord", 3, "getFormRecord(string, owner, guid)");
	m_methodHelper->AppendFunc("getMetadata", "getMetadata()");

	//set object name 
	wxString objectName;

	//fill custom object 
	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		if (object->IsDeleted())
			continue;
		if (!object->GetObjectNameAsString(objectName))
			continue;
		m_methodHelper->AppendProp(
			objectName,
			object->GetMetaID()
		);
	}
}

bool CValueRecordManagerObjectInformationRegister::SetPropVal(const long lPropNum, const CValue& varPropVal)       //setting attribute
{
	return SetValueByMetaID(
		m_methodHelper->GetPropData(lPropNum), varPropVal
	);
}

bool CValueRecordManagerObjectInformationRegister::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	return GetValueByMetaID(
		m_methodHelper->GetPropData(lPropNum), pvarPropVal
	);
}

//////////////////////////////////////////////////////////////////////////

bool CValueRecordSetObjectInformationRegister::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	return false;
}

bool CValueRecordSetObjectInformationRegister::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	switch (lPropNum)
	{
	case prop::eThisObject:
		pvarPropVal = this;
		return true;
	case prop::eFilter:
		pvarPropVal = m_recordSetKeyValue;
		return true;
	}

	return false;
}

bool CValueRecordSetObjectInformationRegister::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);

	switch (lMethodNum)
	{
	case recordSet::enAdd:
		pvarRetValue = CValue::CreateAndPrepareValueRef<CValueRecordSetObjectRegisterReturnLine>(this, GetItem(AppendRow()));
		return true;
	case recordSet::enCount:
		pvarRetValue = (unsigned int)GetRowCount();
		return true;
	case recordSet::enClear:
		IValueTable::Clear();
		return true;
	case recordSet::enLoad:
		LoadDataFromTable(paParams[0]->ConvertToType<IValueTable>());
		return true;
	case recordSet::enUnload:
		pvarRetValue = SaveDataToTable();
		return true;
	case recordSet::enWriteRecordSet:
		WriteRecordSet(
			lSizeArray > 0 ?
			paParams[0]->GetBoolean() : true
		);
		return true;
	case recordSet::enModifiedRecordSet:
		pvarRetValue = m_objModified;
		return true;
	case recordSet::enReadRecordSet:
		Read();
		return true;
	case recordSet::enSelectedRecordSet:
		pvarRetValue = Selected();
		return true;
	case recordSet::enGetMetadataRecordSet:
		pvarRetValue = GetMetaObject();
		return true;
	}

	return false;
}

bool CValueRecordManagerObjectInformationRegister::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case recordManager::enCopyRecordManager:
		pvarRetValue = CopyRegister();
		return true;
	case recordManager::enWriteRecordManager:
		pvarRetValue = WriteRegister(
			lSizeArray > 0 ?
			paParams[0]->GetBoolean() : true
		);
		return true;
	case recordManager::enDeleteRecordManager:
		pvarRetValue = DeleteRegister();
		return true;
	case recordManager::enModifiedRecordManager:
		pvarRetValue = m_recordSet->IsModified();
		return true;
	case recordManager::enReadRecordManager:
		m_recordSet->Read();
		return true;
	case recordManager::enSelectedRecordManager:
		pvarRetValue = m_recordSet->Selected();
		return true;
	case recordManager::enGetFormRecord:
		pvarRetValue = GetFormValue(
			lSizeArray > 0 ? paParams[0]->GetString() : wxEmptyString,
			lSizeArray > 1 ? paParams[1]->ConvertToType<IBackendControlFrame>() : nullptr
		);
		return true;
	case recordManager::enGetMetadataRecordManager:
		pvarRetValue = m_metaObject;
		return true;
	}

	return false;
}