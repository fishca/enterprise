////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : tabular sections
////////////////////////////////////////////////////////////////////////////

#include "tabularSection.h"

#include "backend/appData.h"

#include "backend/databaseLayer/databaseLayer.h"
#include "backend/databaseLayer/databaseErrorCodes.h"

#include "backend/metaCollection/partial/commonObject.h"

bool CTabularSectionDataObjectRef::LoadData(const CGuid& srcGuid, bool createData)
{
	if (m_objectValue->IsNewObject() && !srcGuid.isValid()) {
		m_readAfter = true;
		return false;
	}

	IValueTable::Clear();
	IMetaObjectRecordData* metaObject = m_objectValue->GetMetaObject();
	wxASSERT(metaObject);
	const wxString& tableName = m_metaTable->GetTableNameDB();
	const wxString& sqlQuery = "SELECT * FROM " + tableName + " WHERE uuid = '" + srcGuid.str() + "'";
	IDatabaseResultSet* resultSet = db_query->RunQueryWithResults(sqlQuery);
	if (resultSet == nullptr)
		return false;
	while (resultSet->Next()) {
		wxValueTableRow* rowData = new wxValueTableRow();
		for (const auto object : m_metaTable->GetGenericAttributeArrayObject()) {
			if (m_metaTable->IsNumberLine(object->GetMetaID()))
				continue;
			IMetaObjectAttribute::GetValueAttribute(object, rowData->AppendTableValue(object->GetMetaID()), resultSet, createData);
		}
		IValueTable::Append(rowData, !CBackendException::IsEvalMode());

	}
	resultSet->Close();
	m_readAfter = true;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////

#include "backend/system/systemManager.h"

bool CTabularSectionDataObjectRef::SaveData()
{
	if (m_readOnly)
		return true;

	bool hasError = false;
	//check fill attributes 
	bool fillCheck = true; long currLine = 1;
	for (long row = 0; row < GetRowCount(); row++) {
		for (const auto object : m_metaTable->GetGenericAttributeArrayObject()) {
			if (object->FillCheck()) {
				wxValueTableRow* node = GetViewData<wxValueTableRow>(GetItem(row));
				wxASSERT(node);
				if (node->IsEmptyValue(object->GetMetaID())) {
					wxString fillError =
						wxString::Format(_("The %s is required on line %i of the %s list"), object->GetSynonym(), currLine, m_metaTable->GetSynonym());
					CSystemFunction::Message(fillError, eStatusMessage::eStatusMessage_Information);
					fillCheck = false;
				}
			}
		}
		currLine++;
	}

	if (!fillCheck)
		return false;

	if (!CTabularSectionDataObjectRef::DeleteData())
		return false;

	CMetaObjectAttributePredefined* numLine = m_metaTable->GetNumberLine();
	wxASSERT(numLine);
	IMetaObjectRecordData* metaObject = m_objectValue->GetMetaObject();
	wxASSERT(metaObject);
	reference_t* reference_impl = new reference_t(metaObject->GetMetaID(), m_objectValue->GetGuid());

	const wxString& tableName = m_metaTable->GetTableNameDB();
	wxString queryText = "INSERT INTO " + tableName + " (";
	queryText += "uuid";
	for (const auto object : m_metaTable->GetGenericAttributeArrayObject()) {
		queryText = queryText + ", " + IMetaObjectAttribute::GetSQLFieldName(object);
	}
	queryText += ") VALUES (?";
	for (const auto object : m_metaTable->GetGenericAttributeArrayObject()) {
		unsigned int fieldCount = IMetaObjectAttribute::GetSQLFieldCount(object);
		for (unsigned int i = 0; i < fieldCount; i++) {
			queryText += ", ?";
		}
	}
	queryText += ");";

	IPreparedStatement* statement = db_query->PrepareStatement(queryText);
	if (statement == nullptr)
		return false;

	number_t numberLine = 1;
	for (long row = 0; row < GetRowCount(); row++) {
		if (hasError)
			break;
		int position = 2;
		statement->SetParamString(1, m_objectValue->GetGuid());
		for (const auto object : m_metaTable->GetGenericAttributeArrayObject()) {
			if (!m_metaTable->IsNumberLine(object->GetMetaID())) {
				wxValueTableRow* node = GetViewData<wxValueTableRow>(GetItem(row));
				wxASSERT(node);
				IMetaObjectAttribute::SetValueAttribute(
					object,
					node->GetTableValue(object->GetMetaID()),
					statement,
					position
				);
			}
			else {
				IMetaObjectAttribute::SetValueAttribute(
					object,
					numberLine++,
					statement,
					position
				);
			}
		}

		hasError = statement->RunQuery() == DATABASE_LAYER_QUERY_RESULT_ERROR;
	}

	db_query->CloseStatement(statement);
	delete reference_impl;

	return !hasError;
}

bool CTabularSectionDataObjectRef::DeleteData()
{
	if (m_readOnly || m_objectValue->IsNewObject())
		return true;

	IMetaObjectRecordData* metaObject = m_objectValue->GetMetaObject();
	wxASSERT(metaObject);
	const wxString& tableName = m_metaTable->GetTableNameDB();
	db_query->RunQuery("DELETE FROM " + tableName + " WHERE uuid = '" + m_objectValue->GetGuid() + "';");
	return true;
}