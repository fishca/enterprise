////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : catalog manager
////////////////////////////////////////////////////////////////////////////

#include "catalogManager.h"
#include "backend/appData.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/metaCollection/attribute/metaAttributeObject.h"

CReferenceDataObject* CCatalogManager::FindByCode(const CValue& cParam) const 
{
	if (!appData->DesignerMode()) {

		if (db_query != nullptr && !db_query->IsOpen())
			CBackendException::Error(_("database is not open!"));
		else if (db_query == nullptr)
			CBackendException::Error(_("database is not open!"));

		if (!cParam.IsEmpty()) {
			const wxString& tableName = m_metaObject->GetTableNameDB();
			if (db_query->TableExists(tableName)) {
				CMetaObjectAttributePredefined* attributeCode = m_metaObject->GetDataCode();
				wxASSERT(attributeCode);
				wxString sqlQuery = "";
				if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
					sqlQuery = "SELECT uuid FROM %s WHERE " + IMetaObjectAttribute::GetCompositeSQLFieldName(attributeCode, "LIKE") + " LIMIT 1";
				else
					sqlQuery = "SELECT FIRST 1 uuid FROM %s WHERE " + IMetaObjectAttribute::GetCompositeSQLFieldName(attributeCode, "LIKE");
				IPreparedStatement* statement = db_query->PrepareStatement(sqlQuery, tableName);
				if (statement == nullptr)
					return CReferenceDataObject::Create(m_metaObject);
				int position = 1;
				IMetaObjectAttribute::SetValueAttribute(attributeCode, attributeCode->AdjustValue(cParam), statement, position);
				CReferenceDataObject* foundedReference = nullptr;
				IDatabaseResultSet* databaseResultSet = statement->RunQueryWithResults();
				wxASSERT(databaseResultSet);
				if (databaseResultSet->Next()) {
					const CGuid &foundedGuid = databaseResultSet->GetResultString(guidName);
					if (foundedGuid.isValid()) foundedReference = CReferenceDataObject::Create(m_metaObject, foundedGuid);		
				}
				db_query->CloseResultSet(databaseResultSet);
				if (foundedReference != nullptr) return foundedReference;		
			}
		}
	}
	return CReferenceDataObject::Create(m_metaObject);
}

CReferenceDataObject* CCatalogManager::FindByDescription(const CValue& cParam) const
{
	if (!appData->DesignerMode()) {

		if (db_query != nullptr && !db_query->IsOpen())
			CBackendException::Error(_("database is not open!"));
		else if (db_query == nullptr)
			CBackendException::Error(_("database is not open!"));

		if (!cParam.IsEmpty()) {
			const wxString tableName = m_metaObject->GetTableNameDB();
			if (db_query->TableExists(tableName)) {
				CMetaObjectAttributePredefined* attributeDescription = m_metaObject->GetDataDescription();
				wxASSERT(attributeDescription);
				wxString sqlQuery = "";
				if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
					sqlQuery = "SELECT uuid FROM %s WHERE " + IMetaObjectAttribute::GetCompositeSQLFieldName(attributeDescription, "LIKE") + " LIMIT 1";
				else
					sqlQuery = "SELECT FIRST 1 uuid FROM %s WHERE " + IMetaObjectAttribute::GetCompositeSQLFieldName(attributeDescription, "LIKE");
				IPreparedStatement* statement = db_query->PrepareStatement(sqlQuery, tableName);
				if (statement == nullptr) return CReferenceDataObject::Create(m_metaObject);
				int position = 1;
				IMetaObjectAttribute::SetValueAttribute(attributeDescription, attributeDescription->AdjustValue(cParam), statement, position);
				CReferenceDataObject* foundedReference = nullptr;
				IDatabaseResultSet* databaseResultSet = statement->RunQueryWithResults();
				wxASSERT(databaseResultSet);
				if (databaseResultSet->Next()) {
					const CGuid &foundedGuid = databaseResultSet->GetResultString(guidName);
					if (foundedGuid.isValid()) foundedReference = CReferenceDataObject::Create(m_metaObject, foundedGuid);			
				}
				db_query->CloseResultSet(databaseResultSet);
				if (foundedReference != nullptr) return foundedReference;		
			}
		}
	}	
	return CReferenceDataObject::Create(m_metaObject);
}