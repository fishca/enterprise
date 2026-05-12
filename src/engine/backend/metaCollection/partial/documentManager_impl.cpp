////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : document manager
////////////////////////////////////////////////////////////////////////////

#include "documentManager.h"
#include "backend/appData.h"
#include "backend/session/session.h"
#include "backend/databaseLayer/databaseLayer.h"

ibValueReferenceDataObject* ibValueManagerDataObjectDocument::FindByNumber(const ibValue& vNumber, const ibValue& vPeriod)
{
	if (!appData->DesignerMode()) {
	
		if (ses_query != nullptr && !ses_query->IsOpen())
			ibBackendCoreException::Error(_("Database is not open!"));
		else if (ses_query == nullptr)
			ibBackendCoreException::Error(_("Database is not open!"));
	
		const wxString& tableName = m_metaObject->GetTableNameDB();
		if (ses_query->TableExists(tableName)) {
			ibValueMetaObjectAttributePredefined* attributeNumber = m_metaObject->GetDocumentNumber();
			ibValueMetaObjectAttributePredefined* attributeDate = m_metaObject->GetDocumentDate();
			wxASSERT(attributeNumber && attributeDate);
			wxString sqlQuery = "";
			if (ses_query->GetDatabaseLayerType() != DATABASELAYER_FIREBIRD) {
				sqlQuery = "SELECT uuid FROM %s WHERE " + ibValueMetaObjectAttributeBase::GetCompositeSQLFieldName(attributeNumber, "LIKE") + " LIMIT 1;";
				if (!vPeriod.IsEmpty()) {
					sqlQuery += ibValueMetaObjectAttributeBase::GetCompositeSQLFieldName(attributeNumber, "<=");
				}
			}
			else {
				sqlQuery = "SELECT FIRST 1 uuid FROM %s WHERE " + ibValueMetaObjectAttributeBase::GetCompositeSQLFieldName(attributeNumber, "LIKE") + ";";
				if (!vPeriod.IsEmpty()) {
					sqlQuery += ibValueMetaObjectAttributeBase::GetCompositeSQLFieldName(attributeNumber, "<=");
				}
			}
			ibPreparedStatement* statement = nullptr;
			if (!vPeriod.IsEmpty()) {
				statement = ses_query->PrepareStatement(sqlQuery, tableName);
				if (statement == nullptr) return ibValueReferenceDataObject::Create(m_metaObject);
				int position = 1;
				ibValueMetaObjectAttributeBase::SetValueAttribute(attributeNumber, attributeNumber->AdjustValue(vNumber), statement, position);
				ibValueMetaObjectAttributeBase::SetValueAttribute(attributeDate, attributeDate->AdjustValue(vPeriod), statement, position);
			}
			else {
				statement = ses_query->PrepareStatement(sqlQuery, tableName);
				if (statement == nullptr) return ibValueReferenceDataObject::Create(m_metaObject);		
				int position = 1;
				ibValueMetaObjectAttributeBase::SetValueAttribute(attributeNumber, attributeNumber->AdjustValue(vNumber), statement, position);
			}
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
	return ibValueReferenceDataObject::Create(m_metaObject);
}
