#include "objectSelector.h"

#include "backend/metaCollection/partial/tabularSection/tabularSection.h"

#include "backend/databaseLayer/databaseLayer.h"
#include "backend/appData.h"

/////////////////////////////////////////////////////////////////////////

void CValueSelectorRecordDataObject::Reset()
{
	m_objGuid.reset(); m_newObject = false;
	if (!appData->DesignerMode()) {
		m_currentValues.clear();
		IPreparedStatement* statement = db_query->PrepareStatement("SELECT uuid FROM %s ORDER BY CAST(uuid AS VARCHAR(36)); ", m_metaObject->GetTableNameDB());
		IDatabaseResultSet* resultSet = statement->RunQueryWithResults();
		while (resultSet->Next()) {
			m_currentValues.push_back(
				resultSet->GetResultString(guidName)
			);
		};
		db_query->CloseResultSet(resultSet);
		db_query->CloseStatement(statement);
	}
	for (const auto object : m_metaObject->GetAttributeArrayObject()) {
		if (!appData->DesignerMode()) {
			m_listObjectValue.insert_or_assign(object->GetMetaID(), eValueTypes::TYPE_NULL);
		}
		else {
			m_listObjectValue.insert_or_assign(object->GetMetaID(), object->CreateValue());
		}
	}
	for (const auto object : m_metaObject->GetTableArrayObject()) {
		if (!appData->DesignerMode()) {
			m_listObjectValue.insert_or_assign(object->GetMetaID(), eValueTypes::TYPE_NULL);
		}
		else {
			m_listObjectValue.insert_or_assign(object->GetMetaID(), new CValueTabularSectionDataObjectRef(this, object));
		}
	}
}

bool CValueSelectorRecordDataObject::Read()
{
	if (!m_objGuid.isValid())
		return false;

	m_listObjectValue.clear();

	IPreparedStatement* statement = nullptr;
	if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
		statement = db_query->PrepareStatement("SELECT * FROM %s WHERE uuid = '%s' LIMIT 1; ", m_metaObject->GetTableNameDB(), m_objGuid.str());
	else
		statement = db_query->PrepareStatement("SELECT FIRST 1 * FROM %s WHERE uuid = '%s'; ", m_metaObject->GetTableNameDB(), m_objGuid.str());

	if (statement == nullptr)
		return false;
	bool isLoaded = false;
	IDatabaseResultSet* resultSet = statement->RunQueryWithResults();
	if (resultSet->Next()) {

		m_listObjectValue.insert_or_assign(m_metaObject->GetMetaID(), CValueReferenceDataObject::CreateFromResultSet(resultSet, m_metaObject, m_objGuid));

		//load attributes 
		for (const auto object : m_metaObject->GetAttributeArrayObject()) {
			if (m_metaObject->IsDataReference(object->GetMetaID()))
				continue;
			IValueMetaObjectAttribute::GetValueAttribute(
				object, m_listObjectValue[object->GetMetaID()], resultSet);
		}
		for (const auto object : m_metaObject->GetTableArrayObject()) {
			CValueTabularSectionDataObjectRef* tabularSection = CValue::CreateAndPrepareValueRef<CValueTabularSectionDataObjectRef>(this, object);
			if (!tabularSection->LoadData(m_objGuid))
				isLoaded = false;
			m_listObjectValue.insert_or_assign(object->GetMetaID(), tabularSection);
		}

		isLoaded = true;
	}
	db_query->CloseResultSet(resultSet);
	db_query->CloseStatement(statement);
	return isLoaded;
}

/////////////////////////////////////////////////////////////////////////

void CValueSelectorRegisterDataObject::Reset()
{
	m_keyValues.clear();
	if (!appData->DesignerMode()) {
		m_currentValues.clear();
		IPreparedStatement* statement = db_query->PrepareStatement("SELECT * FROM %s; ", m_metaObject->GetTableNameDB());
		IDatabaseResultSet* resultSet = statement->RunQueryWithResults();
		while (resultSet->Next()) {
			valueArray_t keyRow;
			if (m_metaObject->HasRecorder()) {
				CValueMetaObjectAttributePredefined* attributeRecorder = m_metaObject->GetRegisterRecorder();
				wxASSERT(attributeRecorder);
				IValueMetaObjectAttribute::GetValueAttribute(attributeRecorder, keyRow[attributeRecorder->GetMetaID()], resultSet);
				CValueMetaObjectAttributePredefined* attributeNumberLine = m_metaObject->GetRegisterLineNumber();
				wxASSERT(attributeNumberLine);
				IValueMetaObjectAttribute::GetValueAttribute(attributeNumberLine, keyRow[attributeNumberLine->GetMetaID()], resultSet);
			}
			else {
				for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
					IValueMetaObjectAttribute::GetValueAttribute(object, keyRow[object->GetMetaID()], resultSet);
				}
			}
			m_currentValues.push_back(keyRow);
		};
		db_query->CloseResultSet(resultSet);
		db_query->CloseStatement(statement);
	}
}

bool CValueSelectorRegisterDataObject::Read()
{
	if (m_keyValues.empty())
		return false;

	m_listObjectValue.clear(); 
	
	int position = 1;
	
	wxString queryText = ""; bool isLoaded = false;

	if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL) {
		queryText = "SELECT * FROM " + m_metaObject->GetTableNameDB() + " LIMIT 1"; bool firstWhere = true;
		for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
			if (firstWhere) {
				queryText = queryText + " WHERE ";
			}
			queryText = queryText +
				(firstWhere ? " " : " AND ") + IValueMetaObjectAttribute::GetCompositeSQLFieldName(object);
			if (firstWhere) {
				firstWhere = false;
			}
		}
	}
	else {
		queryText = "SELECT FIRST 1 * FROM " + m_metaObject->GetTableNameDB(); bool firstWhere = true;
		for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
			if (firstWhere) {
				queryText = queryText + " WHERE ";
			}
			queryText = queryText +
				(firstWhere ? " " : " AND ") + IValueMetaObjectAttribute::GetCompositeSQLFieldName(object);
			if (firstWhere) {
				firstWhere = false;
			}
		}
		queryText += " LIMIT 1 ";
	}

	IPreparedStatement* statement = db_query->PrepareStatement(queryText);

	if (statement == nullptr)
		return false;

	for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
		IValueMetaObjectAttribute::SetValueAttribute(
			object,
			m_keyValues.at(object->GetMetaID()),
			statement,
			position
		);
	}

	IDatabaseResultSet* resultSet = statement->RunQueryWithResults();

	if (resultSet->Next()) {
		isLoaded = true;
		//load attributes 
		valueArray_t keyTable, rowTable;
		for (const auto object : m_metaObject->GetGenericDimentionArrayObject())
			IValueMetaObjectAttribute::GetValueAttribute(object, keyTable[object->GetMetaID()], resultSet);
		for (const auto object : m_metaObject->GetGenericAttributeArrayObject())
			IValueMetaObjectAttribute::GetValueAttribute(object, rowTable[object->GetMetaID()], resultSet);
		m_listObjectValue.insert_or_assign(keyTable, rowTable);
	}
	db_query->CloseResultSet(resultSet);
	db_query->CloseStatement(statement);
	return isLoaded;
}
