////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : tabular sections
////////////////////////////////////////////////////////////////////////////

#include "tabularSection.h"

#include "backend/appData.h"

#include "backend/databaseLayer/databaseLayer.h"
#include "backend/databaseLayer/databaseErrorCodes.h"

#include "backend/metaCollection/partial/commonObject.h"

bool ibValueTabularSectionDataObjectRef::LoadData(const ibGuid& srcGuid, bool createData)
{
	if (m_objectValue->IsNewObject() && !srcGuid.isValid()) {
		m_readAfter = true;
		return false;
	}

	ibValueModelTable::Clear();
	ibValueMetaObjectRecordData* metaObject = m_objectValue->GetMetaObject();
	wxASSERT(metaObject);
	const wxString& tableName = m_metaTable->GetTableNameDB();
	const wxString& sqlQuery = "SELECT * FROM " + tableName + " WHERE uuid = '" + srcGuid.str() + "'";
	ibDatabaseResultSet* resultSet = db_query->RunQueryWithResults(sqlQuery);
	if (resultSet == nullptr)
		return false;
	while (resultSet->Next()) {
		wxValueTableRow* rowData = new wxValueTableRow();
		for (const auto object : m_metaTable->GetGenericAttributeArrayObject()) {
			if (m_metaTable->IsNumberLine(object->GetMetaID()))
				continue;
			ibValueMetaObjectAttributeBase::GetValueAttribute(object, rowData->AppendTableValue(object->GetMetaID()), resultSet, createData);
		}
		ibValueModelTable::Append(rowData, !ibBackendException::IsEvalMode());

	}

	db_query->CloseResultSet(resultSet);

	m_readAfter = true;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////

#include "backend/system/systemManager.h"

bool ibValueTabularSectionDataObjectRef::SaveData()
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
					ibValueSystemFunction::Message(fillError, ibStatusMessage::ibStatusMessage_Information);
					fillCheck = false;
				}
			}
		}
		currLine++;
	}

	if (!fillCheck)
		return false;

	if (!ibValueTabularSectionDataObjectRef::DeleteData())
		return false;

	ibValueMetaObjectAttributePredefined* numLine = m_metaTable->GetNumberLine();
	wxASSERT(numLine);
	ibValueMetaObjectRecordData* metaObject = m_objectValue->GetMetaObject();
	wxASSERT(metaObject);
	ibReference* reference_impl = new ibReference(metaObject->GetMetaID(), m_objectValue->GetGuid());

	const wxString& tableName = m_metaTable->GetTableNameDB();
	wxString queryText = "INSERT INTO " + tableName + " (";
	queryText += "uuid";
	for (const auto object : m_metaTable->GetGenericAttributeArrayObject()) {
		queryText = queryText + ", " + ibValueMetaObjectAttributeBase::GetSQLFieldName(object);
	}
	queryText += ") VALUES (?";
	for (const auto object : m_metaTable->GetGenericAttributeArrayObject()) {
		unsigned int fieldCount = ibValueMetaObjectAttributeBase::GetSQLFieldCount(object);
		for (unsigned int i = 0; i < fieldCount; i++) {
			queryText += ", ?";
		}
	}
	queryText += ");";

	ibPreparedStatement* statement = db_query->PrepareStatement(queryText);
	if (statement == nullptr)
		return false;

	ibNumber numberLine = 1;
	for (long row = 0; row < GetRowCount(); row++) {
		if (hasError)
			break;
		int position = 2;
		statement->SetParamString(1, m_objectValue->GetGuid());
		for (const auto object : m_metaTable->GetGenericAttributeArrayObject()) {
			if (!m_metaTable->IsNumberLine(object->GetMetaID())) {
				wxValueTableRow* node = GetViewData<wxValueTableRow>(GetItem(row));
				wxASSERT(node);
				ibValueMetaObjectAttributeBase::SetValueAttribute(
					object,
					node->GetTableValue(object->GetMetaID()),
					statement,
					position
				);
			}
			else {
				ibValueMetaObjectAttributeBase::SetValueAttribute(
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

bool ibValueTabularSectionDataObjectRef::DeleteData()
{
	if (m_readOnly || m_objectValue->IsNewObject())
		return true;

	ibValueMetaObjectRecordData* metaObject = m_objectValue->GetMetaObject();
	wxASSERT(metaObject);
	const wxString& tableName = m_metaTable->GetTableNameDB();
	db_query->RunQuery("DELETE FROM " + tableName + " WHERE uuid = '" + m_objectValue->GetGuid() + "';");
	return true;
}