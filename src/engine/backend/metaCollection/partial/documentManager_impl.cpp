////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : document manager
////////////////////////////////////////////////////////////////////////////

#include "documentManager.h"
#include "backend/appData.h"
#include "backend/databaseLayer/databaseLayer.h"

CReferenceDataObject* CDocumentManager::FindByNumber(const CValue& vNumber, const CValue& vPeriod)
{
	if (!appData->DesignerMode()) {
	
		if (db_query != nullptr && !db_query->IsOpen())
			CBackendException::Error(_("database is not open!"));
		else if (db_query == nullptr)
			CBackendException::Error(_("database is not open!"));
	
		const wxString& tableName = m_metaObject->GetTableNameDB();
		if (db_query->TableExists(tableName)) {
			CMetaObjectAttributeDefault* attributeNumber = m_metaObject->GetDocumentNumber();
			CMetaObjectAttributeDefault* attributeDate = m_metaObject->GetDocumentDate();
			wxASSERT(attributeNumber && attributeDate);
			wxString sqlQuery = "";
			if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL) {
				sqlQuery = "SELECT uuid FROM %s WHERE " + IMetaObjectAttribute::GetCompositeSQLFieldName(attributeNumber, "LIKE") + " LIMIT 1;";
				if (!vPeriod.IsEmpty()) {
					sqlQuery += IMetaObjectAttribute::GetCompositeSQLFieldName(attributeNumber, "<=");
				}
			}
			else {
				sqlQuery = "SELECT FIRST 1 uuid FROM %s WHERE " + IMetaObjectAttribute::GetCompositeSQLFieldName(attributeNumber, "LIKE") + ";";
				if (!vPeriod.IsEmpty()) {
					sqlQuery += IMetaObjectAttribute::GetCompositeSQLFieldName(attributeNumber, "<=");
				}
			}
			IPreparedStatement* statement = nullptr;
			if (!vPeriod.IsEmpty()) {
				statement = db_query->PrepareStatement(sqlQuery, tableName);
				if (statement == nullptr) return CReferenceDataObject::Create(m_metaObject);
				int position = 1;
				IMetaObjectAttribute::SetValueAttribute(attributeNumber, attributeNumber->AdjustValue(vNumber), statement, position);
				IMetaObjectAttribute::SetValueAttribute(attributeDate, attributeDate->AdjustValue(vPeriod), statement, position);
			}
			else {
				statement = db_query->PrepareStatement(sqlQuery, tableName);
				if (statement == nullptr) return CReferenceDataObject::Create(m_metaObject);		
				int position = 1;
				IMetaObjectAttribute::SetValueAttribute(attributeNumber, attributeNumber->AdjustValue(vNumber), statement, position);
			}
			CReferenceDataObject* foundedReference = nullptr;
			IDatabaseResultSet* databaseResultSet = statement->RunQueryWithResults();
			wxASSERT(databaseResultSet);
			if (databaseResultSet->Next()) {
				const CGuid& foundedGuid = databaseResultSet->GetResultString(guidName);
				if (foundedGuid.isValid()) foundedReference = CReferenceDataObject::Create(m_metaObject, foundedGuid);
			}
			db_query->CloseResultSet(databaseResultSet);
			db_query->CloseStatement(statement);
			if (foundedReference != nullptr) return foundedReference;		
		}
	}
	return CReferenceDataObject::Create(m_metaObject);
}
