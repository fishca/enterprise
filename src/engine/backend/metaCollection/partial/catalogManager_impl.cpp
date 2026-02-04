////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : catalog manager
////////////////////////////////////////////////////////////////////////////

#include "catalogManager.h"
#include "backend/appData.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/metaCollection/attribute/metaAttributeObject.h"

CValueReferenceDataObject* CValueManagerDataObjectCatalog::FindByCode(const CValue& cParam) const 
{
	if (!appData->DesignerMode()) {

		if (db_query != nullptr && !db_query->IsOpen())
			CBackendCoreException::Error(_("Database is not open!"));
		else if (db_query == nullptr)
			CBackendCoreException::Error(_("Database is not open!"));

		if (!cParam.IsEmpty()) {
			const wxString& tableName = m_metaObject->GetTableNameDB();
			if (db_query->TableExists(tableName)) {
				CValueMetaObjectAttributePredefined* attributeCode = m_metaObject->GetDataCode();
				wxASSERT(attributeCode);
				wxString sqlQuery = "";
				if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
					sqlQuery = "SELECT uuid FROM %s WHERE " + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attributeCode, "LIKE") + " LIMIT 1";
				else
					sqlQuery = "SELECT FIRST 1 uuid FROM %s WHERE " + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attributeCode, "LIKE");
				IPreparedStatement* statement = db_query->PrepareStatement(sqlQuery, tableName);
				if (statement == nullptr)
					return CValueReferenceDataObject::Create(m_metaObject);
				int position = 1;
				IValueMetaObjectAttribute::SetValueAttribute(attributeCode, attributeCode->AdjustValue(cParam), statement, position);
				CValueReferenceDataObject* foundedReference = nullptr;
				IDatabaseResultSet* databaseResultSet = statement->RunQueryWithResults();
				wxASSERT(databaseResultSet);
				if (databaseResultSet->Next()) {
					const CGuid &foundedGuid = databaseResultSet->GetResultString(guidName);
					if (foundedGuid.isValid()) foundedReference = CValueReferenceDataObject::Create(m_metaObject, foundedGuid);		
				}
				db_query->CloseResultSet(databaseResultSet);
				if (foundedReference != nullptr) return foundedReference;		
			}
		}
	}
	return CValueReferenceDataObject::Create(m_metaObject);
}

CValueReferenceDataObject* CValueManagerDataObjectCatalog::FindByDescription(const CValue& cParam) const
{
	if (!appData->DesignerMode()) {

		if (db_query != nullptr && !db_query->IsOpen())
			CBackendCoreException::Error(_("Database is not open!"));
		else if (db_query == nullptr)
			CBackendCoreException::Error(_("Database is not open!"));

		if (!cParam.IsEmpty()) {
			const wxString tableName = m_metaObject->GetTableNameDB();
			if (db_query->TableExists(tableName)) {
				CValueMetaObjectAttributePredefined* attributeDescription = m_metaObject->GetDataDescription();
				wxASSERT(attributeDescription);
				wxString sqlQuery = "";
				if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
					sqlQuery = "SELECT uuid FROM %s WHERE " + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attributeDescription, "LIKE") + " LIMIT 1";
				else
					sqlQuery = "SELECT FIRST 1 uuid FROM %s WHERE " + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attributeDescription, "LIKE");
				IPreparedStatement* statement = db_query->PrepareStatement(sqlQuery, tableName);
				if (statement == nullptr) return CValueReferenceDataObject::Create(m_metaObject);
				int position = 1;
				IValueMetaObjectAttribute::SetValueAttribute(attributeDescription, attributeDescription->AdjustValue(cParam), statement, position);
				CValueReferenceDataObject* foundedReference = nullptr;
				IDatabaseResultSet* databaseResultSet = statement->RunQueryWithResults();
				wxASSERT(databaseResultSet);
				if (databaseResultSet->Next()) {
					const CGuid &foundedGuid = databaseResultSet->GetResultString(guidName);
					if (foundedGuid.isValid()) foundedReference = CValueReferenceDataObject::Create(m_metaObject, foundedGuid);			
				}
				db_query->CloseResultSet(databaseResultSet);
				if (foundedReference != nullptr) return foundedReference;		
			}
		}
	}	
	return CValueReferenceDataObject::Create(m_metaObject);
}