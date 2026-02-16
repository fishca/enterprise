////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : constants - db
////////////////////////////////////////////////////////////////////////////

#include "constant.h"
#include "backend/metaData.h"
#include "backend/system/systemManager.h"

#include "backend/appData.h"

//***********************************************************************
//*                           constant value                            *
//***********************************************************************

CValueRecordDataObjectConstant* CValueMetaObjectConstant::CreateRecordDataObjectValue()
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	CValueRecordDataObjectConstant* pDataRef = nullptr;

	if (appData->DesignerMode()) {
		if (!moduleManager->FindCompileModule(m_propertyModule->GetMetaObject(), pDataRef))
			return CValue::CreateAndPrepareValueRef<CValueRecordDataObjectConstant>(this);
	}
	else {
		pDataRef = CValue::CreateAndPrepareValueRef<CValueRecordDataObjectConstant>(this);
	}

	return pDataRef;
}

//*********************************************************************************************
//*                                  CValueRecordDataObjectConstant                                     *
//*********************************************************************************************

bool CValueRecordDataObjectConstant::InitializeObject(const CValueRecordDataObjectConstant* source)
{
	IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);
	IValueModuleManager* moduleManager = metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!m_compileModule) {
		m_compileModule = new CCompileModule(m_metaObject->GetModuleObject());
		m_compileModule->SetParent(moduleManager->GetCompileModule());
		m_compileModule->AddContextVariable(wxT("ThisObject"), this);
	}

	try {
		m_constValue = GetConstValue();
	}
	catch (const CBackendException* err) {
		if (!appData->DesignerMode())
			throw(err);
		return false;
	}

	if (!appData->DesignerMode()) {
		m_procUnit = new CProcUnit();
		m_procUnit->SetParent(moduleManager->GetProcUnit());
		try {
			m_compileModule->Compile();
		}
		catch (const CBackendException* err) {
			if (!appData->DesignerMode())
				throw(err);
			return false;
		};
		m_procUnit->Execute(m_compileModule->m_cByteCode, true);
	}

	PrepareNames();
	//is Ok
	return true;
}

CValueRecordDataObjectConstant::CValueRecordDataObjectConstant(CValueMetaObjectConstant* metaObject)
	: m_metaObject(metaObject), m_objModified(false), m_methodHelper(new CMethodHelper())
{
	InitializeObject();
}

CValueRecordDataObjectConstant::CValueRecordDataObjectConstant(const CValueRecordDataObjectConstant& source)
	: m_metaObject(source.m_metaObject), m_objModified(false), m_methodHelper(new CMethodHelper())
{
	InitializeObject(&source);
}

CValueRecordDataObjectConstant::~CValueRecordDataObjectConstant()
{
	wxDELETE(m_methodHelper);
}

IBackendValueForm* CValueRecordDataObjectConstant::GetForm() const
{
	return IBackendValueForm::FindFormByUniqueKey(m_metaObject->GetGuid());
}

void CValueRecordDataObjectConstant::Modify(bool mod)
{
	IBackendValueForm* const foundedForm = GetForm();

	if (foundedForm != nullptr)
		foundedForm->Modify(mod);

	m_objModified = mod;
}

#include "backend/objCtor.h"

class_identifier_t CValueRecordDataObjectConstant::GetClassType() const
{
	const IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(m_metaObject, eCtorMetaType::eCtorMetaType_Object);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CValueRecordDataObjectConstant::GetClassName() const
{
	const IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(m_metaObject, eCtorMetaType::eCtorMetaType_Object);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CValueRecordDataObjectConstant::GetString() const
{
	const IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(m_metaObject, eCtorMetaType::eCtorMetaType_Object);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

CSourceExplorer CValueRecordDataObjectConstant::GetSourceExplorer() const
{
	CSourceExplorer srcHelper(
		m_metaObject, GetClassType(),
		false, true
	);

	srcHelper.AppendSource(m_metaObject);
	return srcHelper;
}

bool CValueRecordDataObjectConstant::GetModel(IValueModel*& tableValue, const meta_identifier_t& id)
{
	return false;
}

#pragma region _form_builder_h_
void CValueRecordDataObjectConstant::ShowFormValue()
{
	IBackendValueForm* const foundedForm = GetForm();

	if (foundedForm && foundedForm->IsShown()) {
		foundedForm->ActivateForm();
		return;
	}

	//if form is not initialized then generate  
	IBackendValueForm* const valueForm =
		GetFormValue();

	if (valueForm != nullptr) {
		valueForm->Modify(false);
		valueForm->ShowForm();
	}
}

IBackendValueForm* CValueRecordDataObjectConstant::GetFormValue()
{
	IBackendValueForm* const foundedForm = GetForm();

	if (foundedForm == nullptr)
		return IValueMetaObjectForm::CreateAndBuildForm(nullptr, nullptr, this, m_metaObject->GetGuid());

	return foundedForm;
}
#pragma endregion

bool CValueRecordDataObjectConstant::SetValueByMetaID(const meta_identifier_t& id, const CValue& varMetaVal)
{
	if (id == m_metaObject->GetMetaID()) {
		IBackendValueForm* const foundedForm = IBackendValueForm::FindFormByUniqueKey(m_metaObject->GetGuid());
		m_constValue = m_metaObject->AdjustValue(varMetaVal);
		if (foundedForm != nullptr) {
			foundedForm->Modify(true);
		}
		return true;
	}
	return false;
}

bool CValueRecordDataObjectConstant::GetValueByMetaID(const meta_identifier_t& id, CValue& pvarMetaVal) const
{
	if (id == m_metaObject->GetMetaID()) {
		pvarMetaVal = m_constValue;
		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void CValueRecordDataObjectConstant::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendProp(wxT("Value"),
		true, true, eValue, eSystem
	);

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

bool CValueRecordDataObjectConstant::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	const long lPropAlias = m_methodHelper->GetPropAlias(lPropNum);
	if (lPropAlias == eProcUnit) {
		if (m_procUnit != nullptr) {
			return m_procUnit->SetPropVal(
				GetPropName(lPropNum), varPropVal
			);
		}
	}
	else if (lPropAlias == eSystem) {
		m_constValue = varPropVal;
		return true;
	}
	return false;
}

bool CValueRecordDataObjectConstant::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const long lPropAlias = m_methodHelper->GetPropAlias(lPropNum);
	if (lPropAlias == eProcUnit) {
		if (m_procUnit != nullptr) {
			return m_procUnit->GetPropVal(
				GetPropName(lPropNum), pvarPropVal
			);
		}
	}
	else if (lPropAlias == eSystem) {
		switch (m_methodHelper->GetPropData(lPropNum))
		{
		case eValue:
			pvarPropVal = m_constValue;
			return true;
		}
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

#include "backend/databaseLayer/databaseLayer.h"

CValue CValueRecordDataObjectConstant::GetConstValue() const
{
	CValue ret;

	if (!appData->DesignerMode()) {

		if (db_query != nullptr && !db_query->IsOpen())
			CBackendCoreException::Error(_("Database is not open!"));
		else if (db_query == nullptr)
			CBackendCoreException::Error(_("Database is not open!"));

		if (!m_metaObject->AccessRight_Read()) {
			CBackendAccessException::Error();
			return false;
		}

		const wxString& tableName = m_metaObject->GetTableNameDB();
		const wxString& fieldName = m_metaObject->GetFieldNameDB();
		if (db_query->TableExists(tableName)) {
			IDatabaseResultSet* resultSet = nullptr;
			if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
				resultSet = db_query->RunQueryWithResults("SELECT %s FROM %s LIMIT 1", IValueMetaObjectAttribute::GetSQLFieldName(m_metaObject), tableName);
			else
				resultSet = db_query->RunQueryWithResults("SELECT FIRST 1 %s FROM %s", IValueMetaObjectAttribute::GetSQLFieldName(m_metaObject), tableName);
			if (resultSet == nullptr)
				return ret;
			if (resultSet->Next()) {
				if (IValueMetaObjectAttribute::GetValueAttribute(m_metaObject, ret, resultSet))
					ret = m_metaObject->AdjustValue(ret);
				else
					ret = m_metaObject->CreateValue();
			}
			else {
				ret = m_metaObject->CreateValue();
			}
			db_query->CloseResultSet(resultSet);
		}
	}
	else {
		ret = m_metaObject->AdjustValue();
	}

	return ret;
}

#include "backend/backend_mainFrame.h"
#include "backend/databaseLayer/databaseErrorCodes.h" 

bool CValueRecordDataObjectConstant::SetConstValue(const CValue& cValue)
{
	if (!appData->DesignerMode()) {

		const CValue& constValue = m_constValue;

		if (db_query != nullptr && !db_query->IsOpen())
			CBackendCoreException::Error(_("Database is not open!"));
		else if (db_query == nullptr)
			CBackendCoreException::Error(_("Database is not open!"));

		if (!m_metaObject->AccessRight_Write()) {
			CBackendAccessException::Error();
			return false;
		}

		const wxString& tableName = m_metaObject->GetTableNameDB();
		const wxString& fieldName = m_metaObject->GetFieldNameDB();

		if (db_query->TableExists(tableName)) {

			CTransactionGuard db_query_active_transaction = db_query;
			{
				IBackendValueForm* const valueForm = GetForm();

				db_query_active_transaction.BeginTransaction();
				{
					CValue cancel = false;

					m_procUnit->CallAsProc(wxT("BeforeWrite"), cancel);

					if (cancel.GetBoolean()) {
						db_query_active_transaction.RollBackTransaction();
						CBackendCoreException::Error(_("failed to write object in db!"));
						return false;
					}
				}

				m_constValue = m_metaObject->AdjustValue(cValue);

				//check fill attributes 
				bool fillCheck = true;

				if (m_metaObject->FillCheck()) {
					if (m_constValue.IsEmpty()) {
						wxString fillError =
							wxString::Format(_("""%s"" is a required field"), m_metaObject->GetSynonym());
						CSystemFunction::Message(fillError, eStatusMessage::eStatusMessage_Information);
						fillCheck = false;
					}
				}

				if (!fillCheck) {
					m_constValue = constValue;
					return false;
				}

				wxString sqlText = "";

				if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL) {
					sqlText = "INSERT INTO %s (%s, RECORD_KEY) VALUES(";
					for (unsigned int idx = 0; idx < IValueMetaObjectAttribute::GetSQLFieldCount(m_metaObject); idx++) {
						sqlText += "?,";
					}
					sqlText += "'6')";
					sqlText += " ON CONFLICT (RECORD_KEY) ";
					sqlText += " DO UPDATE SET " + IValueMetaObjectAttribute::GetExcludeSQLFieldName(m_metaObject) + ";";
				}
				else {
					sqlText = "UPDATE OR INSERT INTO %s (%s, RECORD_KEY) VALUES(";
					for (unsigned int idx = 0; idx < IValueMetaObjectAttribute::GetSQLFieldCount(m_metaObject); idx++) {
						sqlText += "?,";
					}
					sqlText += "'6') MATCHING(RECORD_KEY);";
				}

				IPreparedStatement* statement =
					db_query->PrepareStatement(sqlText, tableName, IValueMetaObjectAttribute::GetSQLFieldName(m_metaObject));

				if (statement == nullptr) {
					m_constValue = constValue;
					return false;
				}

				int position = 1;

				IValueMetaObjectAttribute::SetValueAttribute(
					m_metaObject,
					m_constValue,
					statement,
					position
				);

				bool hasError = statement->RunQuery() == DATABASE_LAYER_QUERY_RESULT_ERROR;
				db_query->CloseStatement(statement);

				if (hasError) {
					m_constValue = constValue;
					db_query_active_transaction.RollBackTransaction();
					CBackendCoreException::Error("failed to write object in db!"); return false;
				}

				{
					CValue cancel = false;
					m_procUnit->CallAsProc(wxT("OnWrite"), cancel);
					if (cancel.GetBoolean()) {
						db_query_active_transaction.RollBackTransaction();
						CBackendCoreException::Error("failed to write object in db!"); return false;
					}
				}

				db_query_active_transaction.CommitTransaction();

				if (valueForm != nullptr) valueForm->NotifyChange(GetValue());
			}
		}
	}

	return true;
}