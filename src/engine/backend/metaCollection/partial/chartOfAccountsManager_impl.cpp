////////////////////////////////////////////////////////////////////////////
//	Author		: Tetracode Dev
//	Description : chart of accounts manager - FindByCode/FindByDescription
////////////////////////////////////////////////////////////////////////////

#include "chartOfAccountsManager.h"
#include "backend/appData.h"
#include "backend/session/session.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/metaCollection/attribute/metaAttributeObject.h"

ibValueReferenceDataObject* ibValueManagerDataObjectChartOfAccounts::FindByCode(const ibValue& cParam) const
{
	if (!appData->DesignerMode()) {
		if (ses_query != nullptr && !ses_query->IsOpen()) ibBackendCoreException::Error(_("Database is not open!"));
		else if (ses_query == nullptr) ibBackendCoreException::Error(_("Database is not open!"));
		if (!cParam.IsEmpty()) {
			const wxString& tableName = m_metaObject->GetTableNameDB();
			if (ses_query->TableExists(tableName)) {
				ibValueMetaObjectAttributePredefined* attributeCode = m_metaObject->GetDataCode();
				wxASSERT(attributeCode);
				wxString sqlQuery = "";
				if (ses_query->GetDatabaseLayerType() != DATABASELAYER_FIREBIRD)
					sqlQuery = "SELECT uuid FROM %s WHERE " + ibValueMetaObjectAttributeBase::GetCompositeSQLFieldName(attributeCode, "LIKE") + " LIMIT 1";
				else
					sqlQuery = "SELECT FIRST 1 uuid FROM %s WHERE " + ibValueMetaObjectAttributeBase::GetCompositeSQLFieldName(attributeCode, "LIKE");
				ibPreparedStatement* statement = ses_query->PrepareStatement(sqlQuery, tableName);
				if (statement == nullptr) return ibValueReferenceDataObject::Create(m_metaObject);
				int position = 1;
				ibValueMetaObjectAttributeBase::SetValueAttribute(attributeCode, attributeCode->AdjustValue(cParam), statement, position);
				ibValueReferenceDataObject* foundedReference = nullptr;
				ibDatabaseResultSet* databaseResultSet = statement->RunQueryWithResults();
				wxASSERT(databaseResultSet);
				if (databaseResultSet->Next()) {
					const ibGuid& foundedGuid = databaseResultSet->GetResultString(guidName);
					if (foundedGuid.isValid()) foundedReference = ibValueReferenceDataObject::Create(m_metaObject, foundedGuid);
				}
				ses_query->CloseResultSet(databaseResultSet);
				ses_query->CloseStatement(statement);
				if (foundedReference != nullptr) return foundedReference;
			}
		}
	}
	return ibValueReferenceDataObject::Create(m_metaObject);
}

ibValueReferenceDataObject* ibValueManagerDataObjectChartOfAccounts::FindByDescription(const ibValue& cParam) const
{
	if (!appData->DesignerMode()) {
		if (ses_query != nullptr && !ses_query->IsOpen()) ibBackendCoreException::Error(_("Database is not open!"));
		else if (ses_query == nullptr) ibBackendCoreException::Error(_("Database is not open!"));
		if (!cParam.IsEmpty()) {
			const wxString tableName = m_metaObject->GetTableNameDB();
			if (ses_query->TableExists(tableName)) {
				ibValueMetaObjectAttributePredefined* attributeDescription = m_metaObject->GetDataDescription();
				wxASSERT(attributeDescription);
				wxString sqlQuery = "";
				if (ses_query->GetDatabaseLayerType() != DATABASELAYER_FIREBIRD)
					sqlQuery = "SELECT uuid FROM %s WHERE " + ibValueMetaObjectAttributeBase::GetCompositeSQLFieldName(attributeDescription, "LIKE") + " LIMIT 1";
				else
					sqlQuery = "SELECT FIRST 1 uuid FROM %s WHERE " + ibValueMetaObjectAttributeBase::GetCompositeSQLFieldName(attributeDescription, "LIKE");
				ibPreparedStatement* statement = ses_query->PrepareStatement(sqlQuery, tableName);
				if (statement == nullptr) return ibValueReferenceDataObject::Create(m_metaObject);
				int position = 1;
				ibValueMetaObjectAttributeBase::SetValueAttribute(attributeDescription, attributeDescription->AdjustValue(cParam), statement, position);
				ibValueReferenceDataObject* foundedReference = nullptr;
				ibDatabaseResultSet* databaseResultSet = statement->RunQueryWithResults();
				wxASSERT(databaseResultSet);
				if (databaseResultSet->Next()) {
					const ibGuid& foundedGuid = databaseResultSet->GetResultString(guidName);
					if (foundedGuid.isValid()) foundedReference = ibValueReferenceDataObject::Create(m_metaObject, foundedGuid);
				}
				ses_query->CloseResultSet(databaseResultSet);
				ses_query->CloseStatement(statement);
				if (foundedReference != nullptr) return foundedReference;
			}
		}
	}
	return ibValueReferenceDataObject::Create(m_metaObject);
}
