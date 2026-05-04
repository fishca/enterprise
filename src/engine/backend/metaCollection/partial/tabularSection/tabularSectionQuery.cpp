////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : tabular sections
////////////////////////////////////////////////////////////////////////////

#include "tabularSection.h"

#include "backend/appData.h"
#include "backend/session/session.h"

#include "backend/databaseLayer/databaseLayer.h"
#include "backend/databaseLayer/databaseErrorCodes.h"

#include "backend/metaCollection/partial/commonObject.h"

bool ibValueTabularSectionDataObjectRef::LoadData(const ibGuid& srcGuid, bool createData)
{
	if (m_objectValue->IsNewObject() && !srcGuid.isValid()) {
		m_readAfter = true;
		return false;
	}

	const auto db = ses_query;
	ibValueModelTableBase::Clear();
	ibValueMetaObjectRecordData* metaObject = m_objectValue->GetMetaObject();
	wxASSERT(metaObject);
	const wxString& tableName = m_metaTable->GetTableNameDB();
	ibStatementGuard sel(db, db->PrepareStatement("SELECT * FROM " + tableName + " WHERE uuid = ?;"));
	if (!sel) return false;
	sel->SetParamString(1, srcGuid.str());
	ibDatabaseResultSet* resultSet = sel->RunQueryWithResults();
	if (resultSet == nullptr)
		return false;
	while (resultSet->Next()) {
		ibValueTableRow* rowData = new ibValueTableRow();
		for (const auto object : m_metaTable->GetGenericAttributeArrayObject()) {
			if (m_metaTable->IsNumberLine(object->GetMetaID()))
				continue;
			ibValueMetaObjectAttributeBase::GetValueAttribute(object, rowData->AppendTableValue(object->GetMetaID()), resultSet, createData);
		}
		ibValueModelTableBase::Append(rowData, !ibBackendException::IsEvalMode());

	}

	db->CloseResultSet(resultSet);

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
				ibValueTableRow* node = GetViewData<ibValueTableRow>(GetItem(row));
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

	const auto db = ses_query;
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

	ibStatementGuard statement(db, db->PrepareStatement(queryText));
	if (!statement) {
		delete reference_impl;
		return false;
	}

	ibNumber numberLine = 1;
	for (long row = 0; row < GetRowCount(); row++) {
		if (hasError)
			break;
		int position = 2;
		statement->SetParamString(1, m_objectValue->GetGuid());
		for (const auto object : m_metaTable->GetGenericAttributeArrayObject()) {
			if (!m_metaTable->IsNumberLine(object->GetMetaID())) {
				ibValueTableRow* node = GetViewData<ibValueTableRow>(GetItem(row));
				wxASSERT(node);
				ibValueMetaObjectAttributeBase::SetValueAttribute(
					object,
					node->GetTableValue(object->GetMetaID()),
					statement.get(),
					position
				);
			}
			else {
				ibValueMetaObjectAttributeBase::SetValueAttribute(
					object,
					numberLine++,
					statement.get(),
					position
				);
			}
		}

		hasError = statement->RunQuery() == DATABASE_LAYER_QUERY_RESULT_ERROR;
	}

	delete reference_impl;
	return !hasError;
}

bool ibValueTabularSectionDataObjectRef::DeleteData()
{
	if (m_readOnly || m_objectValue->IsNewObject())
		return true;

	const auto db = ses_query;
	ibValueMetaObjectRecordData* metaObject = m_objectValue->GetMetaObject();
	wxASSERT(metaObject);
	const wxString& tableName = m_metaTable->GetTableNameDB();
	ibStatementGuard del(db, db->PrepareStatement("DELETE FROM " + tableName + " WHERE uuid = ?;"));
	if (del) {
		del->SetParamString(1, m_objectValue->GetGuid());
		del->RunQuery();
	}
	return true;
}