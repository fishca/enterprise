////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : list db 
////////////////////////////////////////////////////////////////////////////

#include "objectList.h"
#include "backend/appData.h"
#include "backend/databaseLayer/databaseLayer.h"

void CValueListDataObjectEnumRef::RefreshModel(const wxDataViewItem& topItem, const int countPerPage)
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	const wxString& tableName = GetMetaObject()->GetTableNameDB();

	wxString queryText = wxT("");

	if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
		queryText = wxString::Format("SELECT * FROM %s", tableName);
	else
		queryText = wxString::Format("SELECT FIRST " + stringUtils::IntToStr(countPerPage + 1) + " * FROM %s", tableName);

	wxString whereText; bool firstWhere = true;
	for (auto filter : m_filterRow.m_filters) {
		const wxString& operation = filter.m_filterComparison == eComparisonType::eComparisonType_Equal ? "=" : "<>";
		if (filter.m_filterUse) {
			const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(filter.m_filterModel);
			wxASSERT(attribute);
			if (!m_metaObject->IsDataReference(filter.m_filterModel)) {
				if (firstWhere)
					whereText += " WHERE ";
				whereText += (firstWhere ? " " : " AND ") + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attribute, operation);
				if (firstWhere)
					firstWhere = false;
			}
			else {
				if (firstWhere)
					whereText += " WHERE ";
				whereText += (firstWhere ? " " : " AND ") + wxString::Format("%s %s ?", guidName, operation);
				if (firstWhere)
					firstWhere = false;
			}
		}
	}
	wxString orderText; bool firstOrder = true;
	for (auto& sort : m_sortOrder.m_sorts) {
		if (sort.m_sortEnable) {
			const wxString& operation_sort = sort.m_sortAscending ? "ASC" : "DESC";
			if (m_metaObject->IsDataReference(sort.m_sortModel)) {
				if (firstOrder) orderText += " ORDER BY ";
				orderText += (firstOrder ? " " : ", ");
				orderText += "	CASE \n";
				for (const auto object : m_metaObject->GetEnumObjectArray()) {
					orderText += "	WHEN uuid = '" + object->GetGuid().str() + "' THEN " + stringUtils::IntToStr(GetMetaObject()->FindEnumObjectByFilter(object->GetGuid())->GetParentPosition()) + '\n';
				}
				orderText += "	END " + operation_sort + " \n";
				if (firstOrder) firstOrder = false;
			}
		}
	};

	if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
		queryText = queryText + whereText + orderText + " LIMIT " + stringUtils::IntToStr(countPerPage + 1);
	else
		queryText = queryText + whereText + orderText;

	CValueMetaObjectAttributePredefined* metaReference = m_metaObject->GetDataReference();
	CValueMetaObjectAttributePredefined* metaOrder = m_metaObject->GetDataOrder();
	IValueTable::Clear();
	IPreparedStatement* statement = db_query->PrepareStatement(queryText); int position = 1;
	for (auto filter : m_filterRow.m_filters) {
		if (filter.m_filterUse) {
			const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(filter.m_filterModel);
			wxASSERT(attribute);
			if (m_metaObject->IsDataReference(filter.m_filterModel)) {
				const CValue& filterValue = filter.m_filterValue; CValueReferenceDataObject* refData = nullptr;
				if (filterValue.ConvertToValue(refData))
					statement->SetParamString(position++, refData->GetGuid());
				else
					statement->SetParamString(position++, wxNullUniqueKey);
			}
			else {
				IValueMetaObjectAttribute::SetValueAttribute(attribute, filter.m_filterValue, statement, position);
			}
		}
	}
	/////////////////////////////////////////////////////////
	IValueTable::Reserve(countPerPage + 1);
	/////////////////////////////////////////////////////////
	IDatabaseResultSet* resultSet = statement->RunQueryWithResults();
	/////////////////////////////////////////////////////////
	while (resultSet->Next()) {
		CGuid enumRow = resultSet->GetResultString(guidName);
		wxValueTableEnumRow* rowData = new wxValueTableEnumRow(enumRow);
		rowData->AppendTableValue(metaReference->GetMetaID(), CValueReferenceDataObject::CreateFromResultSet(resultSet, m_metaObject, rowData->GetGuid()));
		rowData->AppendTableValue(metaOrder->GetMetaID(), GetMetaObject()->FindEnumObjectByFilter(enumRow)->GetParentPosition());
		IValueTable::Append(rowData, !CBackendException::IsEvalMode());
	};

	db_query->CloseResultSet(resultSet);
	db_query->CloseStatement(statement);
}

void CValueListDataObjectEnumRef::RefreshItemModel(const wxDataViewItem& topItem, const wxDataViewItem& currentItem, const int countPerPage, const short scroll)
{
	const long row_top = GetRow(topItem);
	const long row_current = GetRow(currentItem);

	if (row_top == 0 && countPerPage < GetRowCount() && scroll > 0) {

		wxValueTableEnumRow* valueTableListRow = GetViewData<wxValueTableEnumRow>(GetItem(0));
		const wxString& tableName = m_metaObject->GetTableNameDB();

		wxString queryText;
		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
			queryText = wxString::Format("SELECT * FROM %s ", tableName);
		else
			queryText = wxString::Format("SELECT FIRST 1 * FROM %s ", tableName);

		wxString whereText; bool firstWhere = true;
		for (auto& filter : m_filterRow.m_filters) {
			if (filter.m_filterUse) {
				const wxString& operation = filter.m_filterComparison == eComparisonType::eComparisonType_Equal ? "=" : "<>";
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(filter.m_filterModel);
				wxASSERT(attribute);
				if (!m_metaObject->IsDataReference(filter.m_filterModel)) {
					if (firstWhere)
						whereText += " WHERE ";
					whereText += (firstWhere ? " " : " AND ") + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attribute, operation);
					if (firstWhere)
						firstWhere = false;
				}
				else {
					if (firstWhere)
						whereText += " WHERE ";
					whereText += (firstWhere ? " " : " AND ") + wxString::Format("%s %s ?", guidName, operation);
					if (firstWhere)
						firstWhere = false;
				}
			}
		}
		wxString orderText; bool firstOrder = true;
		for (auto& sort : m_sortOrder.m_sorts) {
			if (sort.m_sortEnable) {
				const wxString& operation_sort = !sort.m_sortAscending ? "ASC" : "DESC";
				if (m_metaObject->IsDataReference(sort.m_sortModel)) {
					int pos_in_parent = 1, current_pos = 0;
					if (firstOrder) orderText += " ORDER BY ";
					if (firstWhere) whereText += " WHERE ";
					orderText += " CASE \n";
					whereText += " CASE \n";
					for (const auto object : m_metaObject->GetEnumObjectArray()) {
						const wxString& str_guid = object->GetGuid().str();
						orderText += " WHEN uuid = '" + str_guid + "' THEN " + stringUtils::IntToStr(pos_in_parent) + '\n';
						whereText += " WHEN uuid = '" + str_guid + "' THEN " + stringUtils::IntToStr(pos_in_parent) + '\n';
						if (object->GetGuid() == valueTableListRow->GetGuid()) current_pos = pos_in_parent;
						pos_in_parent++;
					}
					orderText += " END " + operation_sort + " \n";
					whereText += " END ";
					whereText += "  < " + stringUtils::IntToStr(current_pos) + " \n";
					if (firstOrder) firstOrder = false;
					if (firstWhere) firstWhere = false;
				}
			}
		};

		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
			queryText = queryText + whereText + orderText + " LIMIT " + stringUtils::IntToStr(1);
		else
			queryText = queryText + whereText + orderText;

		CValueMetaObjectAttributePredefined* metaReference = m_metaObject->GetDataReference();
		CValueMetaObjectAttributePredefined* metaOrder = m_metaObject->GetDataOrder();
		/////////////////////////////////////////////////////////
		IPreparedStatement* statement = db_query->PrepareStatement(queryText); int position = 1;
		for (auto filter : m_filterRow.m_filters) {
			if (filter.m_filterUse) {
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(filter.m_filterModel);
				wxASSERT(attribute);
				if (m_metaObject->IsDataReference(filter.m_filterModel)) {
					const CValue& filterValue = filter.m_filterValue; CValueReferenceDataObject* refData = nullptr;
					if (filterValue.ConvertToValue(refData))
						statement->SetParamString(position++, refData->GetGuid());
					else
						statement->SetParamString(position++, wxNullUniqueKey);
				}
				else {
					IValueMetaObjectAttribute::SetValueAttribute(attribute, filter.m_filterValue, statement, position);
				}
			}
		}
		/////////////////////////////////////////////////////////
		for (auto& sort : m_sortOrder.m_sorts) {
			if (sort.m_sortEnable && !m_metaObject->IsDataReference(sort.m_sortModel)) {
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(sort.m_sortModel);
				wxASSERT(attribute);
				IValueMetaObjectAttribute::SetValueAttribute(attribute, valueTableListRow->GetTableValue(sort.m_sortModel), statement, position);
			}
		}
		/////////////////////////////////////////////////////////
		IDatabaseResultSet* resultSet = statement->RunQueryWithResults();
		/////////////////////////////////////////////////////////
		bool insertedValue = false;
		/////////////////////////////////////////////////////////
		while (resultSet->Next()) {
			wxValueTableEnumRow* rowData = new wxValueTableEnumRow(resultSet->GetResultString(guidName));
			rowData->AppendTableValue(metaReference->GetMetaID(), CValueReferenceDataObject::CreateFromResultSet(resultSet, m_metaObject, rowData->GetGuid()));
			rowData->AppendTableValue(metaOrder->GetMetaID(), GetMetaObject()->FindEnumObjectByFilter(rowData->GetGuid())->GetParentPosition());
			IValueTable::Insert(rowData, 0, !CBackendException::IsEvalMode());
			/////////////////////////////////////////////////////////
			insertedValue = true;
			/////////////////////////////////////////////////////////
		};
		/////////////////////////////////////////////////////////
		if (insertedValue) IValueTable::ClearRange(IValueTable::GetRowCount() - 1, IValueTable::GetRowCount(), !CBackendException::IsEvalMode());
		/////////////////////////////////////////////////////////
		db_query->CloseResultSet(resultSet);
		db_query->CloseStatement(statement);
	}
	else if (row_top + countPerPage == GetRowCount() && scroll < 0) {

		wxValueTableEnumRow* valueTableListRow = GetViewData<wxValueTableEnumRow>(GetItem(GetRowCount() - 1));
		const wxString& tableName = m_metaObject->GetTableNameDB();

		wxString queryText;
		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
			queryText = wxString::Format("SELECT * FROM %s ", tableName);
		else
			queryText = wxString::Format("SELECT FIRST 1 * FROM %s ", tableName);

		wxString whereText; bool firstWhere = true;
		for (auto& filter : m_filterRow.m_filters) {
			if (filter.m_filterUse) {
				const wxString& operation = filter.m_filterComparison == eComparisonType::eComparisonType_Equal ? "=" : "<>";
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(filter.m_filterModel);
				wxASSERT(attribute);
				if (!m_metaObject->IsDataReference(filter.m_filterModel)) {
					if (firstWhere)
						whereText += " WHERE ";
					whereText += (firstWhere ? " " : " AND ") + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attribute, operation);
					if (firstWhere)
						firstWhere = false;
				}
				else {
					if (firstWhere)
						whereText += " WHERE ";
					whereText += (firstWhere ? " " : " AND ") + wxString::Format("%s %s ?", guidName, operation);
					if (firstWhere)
						firstWhere = false;
				}
			}
		}
		wxString orderText; bool firstOrder = true;
		for (auto& sort : m_sortOrder.m_sorts) {
			if (sort.m_sortEnable) {
				const wxString& operation_sort = sort.m_sortAscending ? "ASC" : "DESC";
				if (m_metaObject->IsDataReference(sort.m_sortModel)) {
					int pos_in_parent = 1, current_pos = 0;
					if (firstOrder) orderText += " ORDER BY ";
					if (firstWhere) whereText += " WHERE ";
					orderText += " CASE \n";
					whereText += " CASE \n";
					for (const auto object : m_metaObject->GetEnumObjectArray()) {
						const wxString& str_guid = object->GetGuid().str();
						orderText += " WHEN uuid = '" + str_guid + "' THEN " + stringUtils::IntToStr(pos_in_parent) + '\n';
						whereText += " WHEN uuid = '" + str_guid + "' THEN " + stringUtils::IntToStr(pos_in_parent) + '\n';
						if (object->GetGuid() == valueTableListRow->GetGuid()) current_pos = pos_in_parent;
						pos_in_parent++;
					}
					orderText += " END " + operation_sort + " \n";
					whereText += " END ";
					whereText += "  > " + stringUtils::IntToStr(current_pos) + " \n";
					if (firstOrder) firstOrder = false;
					if (firstWhere) firstWhere = false;
				}
			}
		};

		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
			queryText = queryText + whereText + orderText + " LIMIT " + stringUtils::IntToStr(1);
		else
			queryText = queryText + whereText + orderText;

		CValueMetaObjectAttributePredefined* metaReference = m_metaObject->GetDataReference();
		CValueMetaObjectAttributePredefined* metaOrder = m_metaObject->GetDataOrder();
		/////////////////////////////////////////////////////////
		IPreparedStatement* statement = db_query->PrepareStatement(queryText); int position = 1;
		for (auto filter : m_filterRow.m_filters) {
			if (filter.m_filterUse) {
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(filter.m_filterModel);
				wxASSERT(attribute);
				if (m_metaObject->IsDataReference(filter.m_filterModel)) {
					const CValue& filterValue = filter.m_filterValue; CValueReferenceDataObject* refData = nullptr;
					if (filterValue.ConvertToValue(refData))
						statement->SetParamString(position++, refData->GetGuid());
					else
						statement->SetParamString(position++, wxNullUniqueKey);
				}
				else {
					IValueMetaObjectAttribute::SetValueAttribute(attribute, filter.m_filterValue, statement, position);
				}
			}
		}
		/////////////////////////////////////////////////////////
		for (auto& sort : m_sortOrder.m_sorts) {
			if (sort.m_sortEnable && !m_metaObject->IsDataReference(sort.m_sortModel)) {
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(sort.m_sortModel);
				wxASSERT(attribute);
				IValueMetaObjectAttribute::SetValueAttribute(attribute, valueTableListRow->GetTableValue(sort.m_sortModel), statement, position);
			}
		}
		/////////////////////////////////////////////////////////
		IDatabaseResultSet* resultSet = statement->RunQueryWithResults();
		/////////////////////////////////////////////////////////
		bool insertedValue = false;
		/////////////////////////////////////////////////////////
		while (resultSet->Next()) {
			wxValueTableEnumRow* rowData = new wxValueTableEnumRow(resultSet->GetResultString(guidName));
			rowData->AppendTableValue(metaReference->GetMetaID(), CValueReferenceDataObject::CreateFromResultSet(resultSet, m_metaObject, rowData->GetGuid()));
			rowData->AppendTableValue(metaOrder->GetMetaID(), GetMetaObject()->FindEnumObjectByFilter(rowData->GetGuid())->GetParentPosition());
			IValueTable::Append(rowData, !CBackendException::IsEvalMode());
			/////////////////////////////////////////////////////////
			insertedValue = true;
			/////////////////////////////////////////////////////////
		};
		/////////////////////////////////////////////////////////
		if (insertedValue) IValueTable::ClearRange(0, 1, !CBackendException::IsEvalMode());
		/////////////////////////////////////////////////////////
		db_query->CloseResultSet(resultSet);
		db_query->CloseStatement(statement);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void CValueListDataObjectRef::RefreshModel(const wxDataViewItem& topItem, const int countPerPage)
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	const wxString& tableName = m_metaObject->GetTableNameDB();

	wxString queryText;
	if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
		queryText = wxString::Format("SELECT * FROM %s ", tableName);
	else
		queryText = wxString::Format("SELECT FIRST " + stringUtils::IntToStr(countPerPage + 1) + " * FROM % s ", tableName);

	wxString whereText; bool firstWhere = true;
	for (auto& filter : m_filterRow.m_filters) {
		if (filter.m_filterUse) {
			const wxString& operation = filter.m_filterComparison == eComparisonType::eComparisonType_Equal ? "=" : "<>";
			const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(filter.m_filterModel);
			wxASSERT(attribute);
			if (!m_metaObject->IsDataReference(filter.m_filterModel)) {
				if (firstWhere)
					whereText += " WHERE ";
				whereText += (firstWhere ? " " : " AND ") + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attribute, operation);
				if (firstWhere)
					firstWhere = false;
			}
			else {
				if (firstWhere)
					whereText += " WHERE ";
				whereText += (firstWhere ? " " : " AND ") + wxString::Format("%s %s ?", guidName, operation);
				if (firstWhere)
					firstWhere = false;
			}
		}
	}
	int compare_func = 0;
	wxString orderText; bool firstOrder = true;
	for (auto& sort : m_sortOrder.m_sorts) {
		if (sort.m_sortEnable) {
			const wxString& operation = sort.m_sortAscending ? "ASC" : "DESC";
			const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(sort.m_sortModel);
			wxASSERT(attribute);
			if (!m_metaObject->IsDataReference(sort.m_sortModel)) {
				if (firstOrder)
					orderText += " ORDER BY ";
				for (auto& field : IValueMetaObjectAttribute::GetSQLFieldData(attribute)) {
					if (field.m_type == IValueMetaObjectAttribute::eFieldTypes_Reference) {
						orderText += (firstOrder ? " " : ", ") + wxString::Format("%s %s", field.m_field.m_fieldRefName.m_fieldRefName, operation);
						if (firstOrder) firstOrder = false;
					}
					else {
						orderText += (firstOrder ? " " : ", ") + wxString::Format("%s %s", field.m_field.m_fieldName, operation);
						if (firstOrder) firstOrder = false;
					}
				}
			}
			if (compare_func == 0) {
				compare_func = sort.m_sortAscending ? 1 : -1;
			}
		}
	};

	for (auto& sort : m_sortOrder.m_sorts) {
		if (sort.m_sortEnable) {
			const wxString& operation_sort = (compare_func >= 0 ? sort.m_sortAscending : !sort.m_sortAscending) ? "ASC" : "DESC";
			if (m_metaObject->IsDataReference(sort.m_sortModel)) {
				if (firstOrder)
					orderText += " ORDER BY ";
				orderText += (firstOrder ? " " : ", ") + wxString::Format("%s %s", guidName, operation_sort);
				if (firstOrder)
					firstOrder = false;
			}
		}
	};

	const std::vector<IValueMetaObjectAttribute*>& vec_attr = m_metaObject->GetGenericAttributeArrayObject();

	if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
		queryText = queryText + whereText + orderText + " LIMIT " + stringUtils::IntToStr(countPerPage + 1);
	else
		queryText = queryText + whereText + orderText;

	CValueMetaObjectAttributePredefined* metaReference = m_metaObject->GetDataReference();
	/////////////////////////////////////////////////////////
	IValueTable::Clear();
	/////////////////////////////////////////////////////////
	IPreparedStatement* statement = db_query->PrepareStatement(queryText); int position = 1;
	for (auto filter : m_filterRow.m_filters) {
		if (filter.m_filterUse) {
			const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(filter.m_filterModel);
			wxASSERT(attribute);
			if (m_metaObject->IsDataReference(filter.m_filterModel)) {
				const CValue& filterValue = filter.m_filterValue; CValueReferenceDataObject* refData = nullptr;
				if (filterValue.ConvertToValue(refData))
					statement->SetParamString(position++, refData->GetGuid());
				else
					statement->SetParamString(position++, wxNullUniqueKey);
			}
			else {
				IValueMetaObjectAttribute::SetValueAttribute(attribute, filter.m_filterValue, statement, position);
			}
		}
	}
	/////////////////////////////////////////////////////////
	IValueTable::Reserve(countPerPage + 1);
	/////////////////////////////////////////////////////////
	IDatabaseResultSet* resultSet = statement->RunQueryWithResults();
	while (resultSet->Next()) {
		wxValueTableListRow* rowData = new wxValueTableListRow(resultSet->GetResultString(guidName));
		for (auto& attribute : vec_attr) {
			if (m_metaObject->IsDataReference(attribute->GetMetaID()))
				continue;
			IValueMetaObjectAttribute::GetValueAttribute(attribute, rowData->AppendTableValue(attribute->GetMetaID()), resultSet);
		}
		rowData->AppendTableValue(metaReference->GetMetaID(), CValueReferenceDataObject::CreateFromResultSet(resultSet, m_metaObject, rowData->GetGuid()));
		IValueTable::Append(rowData, !CBackendException::IsEvalMode());
	};

	db_query->CloseResultSet(resultSet);
	db_query->CloseStatement(statement);
}

void CValueListDataObjectRef::RefreshItemModel(const wxDataViewItem& topItem, const wxDataViewItem& currentItem, const int countPerPage, const short scroll)
{
	const long row_top = GetRow(topItem);
	const long row_current = GetRow(currentItem);

	if (row_top == 0 && countPerPage < GetRowCount() && scroll > 0) {

		wxValueTableListRow* valueTableListRow = GetViewData<wxValueTableListRow>(GetItem(0));
		const wxString& tableName = m_metaObject->GetTableNameDB();

		wxString queryText;
		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
			queryText = wxString::Format("SELECT * FROM %s ", tableName);
		else
			queryText = wxString::Format("SELECT FIRST 1 * FROM %s ", tableName);

		wxString whereText; bool firstWhere = true;
		for (auto& filter : m_filterRow.m_filters) {
			if (filter.m_filterUse) {
				const wxString& operation = filter.m_filterComparison == eComparisonType::eComparisonType_Equal ? "=" : "<>";
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(filter.m_filterModel);
				wxASSERT(attribute);
				if (!m_metaObject->IsDataReference(filter.m_filterModel)) {
					if (firstWhere)
						whereText += " WHERE ";
					whereText += (firstWhere ? " " : " AND ") + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attribute, operation);
					if (firstWhere)
						firstWhere = false;
				}
				else {
					if (firstWhere)
						whereText += " WHERE ";
					whereText += (firstWhere ? " " : " AND ") + wxString::Format("%s %s ?", guidName, operation);
					if (firstWhere)
						firstWhere = false;
				}
			}
		}
		wxString orderText; bool firstOrder = true; wxString sortText;
		int compare_func = 0;
		for (auto& sort : m_sortOrder.m_sorts) {
			if (sort.m_sortEnable) {
				const wxString& operation_sort = !sort.m_sortAscending ? "ASC" : "DESC";
				const wxString& operation_compare = !sort.m_sortAscending ? ">=" : "<=";
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(sort.m_sortModel);
				wxASSERT(attribute);
				if (!m_metaObject->IsDataReference(sort.m_sortModel)) {
					if (firstOrder)
						orderText += " ORDER BY ";
					for (auto& field : IValueMetaObjectAttribute::GetSQLFieldData(attribute)) {
						if (field.m_type == IValueMetaObjectAttribute::eFieldTypes_Reference) {
							orderText += (firstOrder ? " " : ", ") + wxString::Format("%s %s", field.m_field.m_fieldRefName.m_fieldRefName, operation_sort);
							if (firstOrder)
								firstOrder = false;

						}
						else {
							orderText += (firstOrder ? " " : ", ") + wxString::Format("%s %s", field.m_field.m_fieldName, operation_sort);
							if (firstOrder)
								firstOrder = false;
						}

						if (firstWhere) whereText += " WHERE ";
						whereText += (firstWhere ? " " : " AND ") + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attribute, operation_compare);
						sortText += (sortText.IsEmpty() ? " " : " AND ") + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attribute, '=');
						if (firstWhere) firstWhere = false;
					}
				}
				if (compare_func == 0) {
					compare_func = sort.m_sortAscending ? 1 : -1;
				}
			}
		};
		for (auto& sort : m_sortOrder.m_sorts) {
			if (sort.m_sortEnable) {
				const wxString& operation_sort = (compare_func >= 0 ? !sort.m_sortAscending : sort.m_sortAscending) ? "ASC" : "DESC";
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(sort.m_sortModel);
				wxASSERT(attribute);
				if (m_metaObject->IsDataReference(sort.m_sortModel)) {
					if (firstOrder)
						orderText += " ORDER BY ";
					orderText += (firstOrder ? " " : ", ") + wxString::Format("%s %s", guidName, operation_sort);
					if (firstOrder)
						firstOrder = false;

					if (firstWhere)
						whereText += " WHERE ";

					whereText += (firstWhere ? " " : " AND ") + wxString::Format((compare_func >= 0 ? !sort.m_sortAscending : sort.m_sortAscending) ? "case when %s then %s > '%s' else true end" : "case when %s then %s < '%s' else true end", sortText, guidName, valueTableListRow->GetGuid().str());
					if (firstWhere)
						firstWhere = false;
				}
			}
		};
		const std::vector<IValueMetaObjectAttribute*>& vec_attr = m_metaObject->GetGenericAttributeArrayObject();

		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
			queryText = queryText + whereText + orderText + " LIMIT " + stringUtils::IntToStr(1);
		else
			queryText = queryText + whereText + orderText;

		CValueMetaObjectAttributePredefined* metaReference = m_metaObject->GetDataReference();
		/////////////////////////////////////////////////////////
		IPreparedStatement* statement = db_query->PrepareStatement(queryText); int position = 1;
		for (auto filter : m_filterRow.m_filters) {
			if (filter.m_filterUse) {
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(filter.m_filterModel);
				wxASSERT(attribute);
				if (m_metaObject->IsDataReference(filter.m_filterModel)) {
					const CValue& filterValue = filter.m_filterValue; CValueReferenceDataObject* refData = nullptr;
					if (filterValue.ConvertToValue(refData))
						statement->SetParamString(position++, refData->GetGuid());
					else
						statement->SetParamString(position++, wxNullUniqueKey);
				}
				else {
					IValueMetaObjectAttribute::SetValueAttribute(attribute, filter.m_filterValue, statement, position);
				}
			}
		}
		/////////////////////////////////////////////////////////
		for (auto& sort : m_sortOrder.m_sorts) {
			if (sort.m_sortEnable && !m_metaObject->IsDataReference(sort.m_sortModel)) {
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(sort.m_sortModel);
				wxASSERT(attribute);
				IValueMetaObjectAttribute::SetValueAttribute(attribute, valueTableListRow->GetTableValue(sort.m_sortModel), statement, position);
			}
		}
		/////////////////////////////////////////////////////////
		for (auto& sort : m_sortOrder.m_sorts) {
			if (sort.m_sortEnable && !m_metaObject->IsDataReference(sort.m_sortModel)) {
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(sort.m_sortModel);
				wxASSERT(attribute);
				IValueMetaObjectAttribute::SetValueAttribute(attribute, valueTableListRow->GetTableValue(sort.m_sortModel), statement, position);
			}
		}
		/////////////////////////////////////////////////////////
		IDatabaseResultSet* resultSet = statement->RunQueryWithResults();
		/////////////////////////////////////////////////////////
		bool insertedValue = false;
		/////////////////////////////////////////////////////////
		while (resultSet->Next()) {
			wxValueTableListRow* rowData = new wxValueTableListRow(resultSet->GetResultString(guidName));
			for (auto& attribute : vec_attr) {
				if (m_metaObject->IsDataReference(attribute->GetMetaID()))
					continue;
				IValueMetaObjectAttribute::GetValueAttribute(attribute, rowData->AppendTableValue(attribute->GetMetaID()), resultSet);
			}
			rowData->AppendTableValue(metaReference->GetMetaID(), CValueReferenceDataObject::CreateFromResultSet(resultSet, m_metaObject, rowData->GetGuid()));
			IValueTable::Insert(rowData, 0, !CBackendException::IsEvalMode());
			/////////////////////////////////////////////////////////
			insertedValue = true;
			/////////////////////////////////////////////////////////
		};
		/////////////////////////////////////////////////////////
		if (insertedValue) IValueTable::ClearRange(IValueTable::GetRowCount() - 1, IValueTable::GetRowCount(), !CBackendException::IsEvalMode());
		/////////////////////////////////////////////////////////
		db_query->CloseResultSet(resultSet);
		db_query->CloseStatement(statement);
	}
	else if (row_top + countPerPage == GetRowCount() && scroll < 0) {

		wxValueTableListRow* valueTableListRow = GetViewData<wxValueTableListRow>(GetItem(GetRowCount() - 1));
		const wxString& tableName = m_metaObject->GetTableNameDB();

		wxString queryText;
		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
			queryText = wxString::Format("SELECT * FROM %s ", tableName);
		else
			queryText = wxString::Format("SELECT FIRST 1 * FROM %s ", tableName);

		wxString whereText; bool firstWhere = true;
		for (auto& filter : m_filterRow.m_filters) {
			if (filter.m_filterUse) {
				const wxString& operation = filter.m_filterComparison == eComparisonType::eComparisonType_Equal ? "=" : "<>";
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(filter.m_filterModel);
				wxASSERT(attribute);
				if (!m_metaObject->IsDataReference(filter.m_filterModel)) {
					if (firstWhere)
						whereText += " WHERE ";
					whereText += (firstWhere ? " " : " AND ") + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attribute, operation);
					if (firstWhere)
						firstWhere = false;
				}
				else {
					if (firstWhere)
						whereText += " WHERE ";
					whereText += (firstWhere ? " " : " AND ") + wxString::Format("%s %s ?", guidName, operation);
					if (firstWhere)
						firstWhere = false;
				}
			}
		}
		wxString orderText; bool firstOrder = true; wxString sortText;
		int compare_func = 0;
		for (auto& sort : m_sortOrder.m_sorts) {
			if (sort.m_sortEnable) {
				const wxString& operation_sort = sort.m_sortAscending ? "ASC" : "DESC";
				const wxString& operation_compare = sort.m_sortAscending ? ">=" : "<=";
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(sort.m_sortModel);
				wxASSERT(attribute);
				if (!m_metaObject->IsDataReference(sort.m_sortModel)) {
					if (firstOrder) orderText += " ORDER BY ";
					for (auto& field : IValueMetaObjectAttribute::GetSQLFieldData(attribute)) {
						if (field.m_type == IValueMetaObjectAttribute::eFieldTypes_Reference) {
							orderText += (firstOrder ? " " : ", ") + wxString::Format("%s %s", field.m_field.m_fieldRefName.m_fieldRefName, operation_sort);
							if (firstOrder) firstOrder = false;
						}
						else {
							orderText += (firstOrder ? " " : ", ") + wxString::Format("%s %s", field.m_field.m_fieldName, operation_sort);
							if (firstOrder) firstOrder = false;
						}
						if (firstWhere) whereText += " WHERE ";
						whereText += (firstWhere ? " " : " AND ") + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attribute, operation_compare);
						sortText += (sortText.IsEmpty() ? " " : " AND ") + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attribute, '=');
						if (firstWhere) firstWhere = false;
					}
				}
				if (compare_func == 0) {
					compare_func = sort.m_sortAscending ? 1 : -1;
				}
			}
		};
		for (auto& sort : m_sortOrder.m_sorts) {
			if (sort.m_sortEnable) {
				const wxString& operation_sort = (compare_func >= 0 ? sort.m_sortAscending : !sort.m_sortAscending) ? "ASC" : "DESC";
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(sort.m_sortModel);
				wxASSERT(attribute);
				if (m_metaObject->IsDataReference(sort.m_sortModel)) {
					if (firstOrder)
						orderText += " ORDER BY ";
					orderText += (firstOrder ? " " : ", ") + wxString::Format("%s %s", guidName, operation_sort);
					if (firstOrder)
						firstOrder = false;

					if (firstWhere)
						whereText += " WHERE ";
					whereText += (firstWhere ? " " : " AND ") + wxString::Format((compare_func >= 0 ? sort.m_sortAscending : !sort.m_sortAscending) ? "case when %s then %s > '%s' else true end" : "case when %s then %s < '%s' else true end", sortText, guidName, valueTableListRow->GetGuid().str());
					if (firstWhere)
						firstWhere = false;
				}
			}
		};
		const std::vector<IValueMetaObjectAttribute*>& vec_attr = m_metaObject->GetGenericAttributeArrayObject();

		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
			queryText = queryText + whereText + orderText + " LIMIT " + stringUtils::IntToStr(1);
		else
			queryText = queryText + whereText + orderText;

		CValueMetaObjectAttributePredefined* metaReference = m_metaObject->GetDataReference();
		/////////////////////////////////////////////////////////
		IPreparedStatement* statement = db_query->PrepareStatement(queryText); int position = 1;
		for (auto filter : m_filterRow.m_filters) {
			if (filter.m_filterUse) {
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(filter.m_filterModel);
				wxASSERT(attribute);
				if (m_metaObject->IsDataReference(filter.m_filterModel)) {
					const CValue& filterValue = filter.m_filterValue; CValueReferenceDataObject* refData = nullptr;
					if (filterValue.ConvertToValue(refData))
						statement->SetParamString(position++, refData->GetGuid());
					else
						statement->SetParamString(position++, wxNullUniqueKey);
				}
				else {
					IValueMetaObjectAttribute::SetValueAttribute(attribute, filter.m_filterValue, statement, position);
				}
			}
		}
		/////////////////////////////////////////////////////////
		for (auto& sort : m_sortOrder.m_sorts) {
			if (sort.m_sortEnable && !m_metaObject->IsDataReference(sort.m_sortModel)) {
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(sort.m_sortModel);
				wxASSERT(attribute);
				IValueMetaObjectAttribute::SetValueAttribute(attribute, valueTableListRow->GetTableValue(sort.m_sortModel), statement, position);
			}
		}
		/////////////////////////////////////////////////////////
		for (auto& sort : m_sortOrder.m_sorts) {
			if (sort.m_sortEnable && !m_metaObject->IsDataReference(sort.m_sortModel)) {
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(sort.m_sortModel);
				wxASSERT(attribute);
				IValueMetaObjectAttribute::SetValueAttribute(attribute, valueTableListRow->GetTableValue(sort.m_sortModel), statement, position);
			}
		}
		/////////////////////////////////////////////////////////
		IDatabaseResultSet* resultSet = statement->RunQueryWithResults();
		/////////////////////////////////////////////////////////
		bool insertedValue = false;
		/////////////////////////////////////////////////////////
		while (resultSet->Next()) {
			wxValueTableListRow* rowData = new wxValueTableListRow(resultSet->GetResultString(guidName));
			for (auto& attribute : vec_attr) {
				if (m_metaObject->IsDataReference(attribute->GetMetaID()))
					continue;
				IValueMetaObjectAttribute::GetValueAttribute(attribute, rowData->AppendTableValue(attribute->GetMetaID()), resultSet);
			}
			rowData->AppendTableValue(metaReference->GetMetaID(), CValueReferenceDataObject::CreateFromResultSet(resultSet, m_metaObject, rowData->GetGuid()));
			IValueTable::Append(rowData, !CBackendException::IsEvalMode());
			/////////////////////////////////////////////////////////
			insertedValue = true;
			/////////////////////////////////////////////////////////
		};
		/////////////////////////////////////////////////////////
		if (insertedValue) IValueTable::ClearRange(0, 1, !CBackendException::IsEvalMode());
		/////////////////////////////////////////////////////////
		db_query->CloseResultSet(resultSet);
		db_query->CloseStatement(statement);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void CValueListRegisterObject::RefreshModel(const wxDataViewItem& topItem, const int countPerPage)
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	const wxString& tableName = m_metaObject->GetTableNameDB();

	wxString queryText;
	if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
		queryText = "SELECT * FROM " + tableName;
	else
		queryText = "SELECT FIRST " + stringUtils::IntToStr(countPerPage + 1) + " * FROM " + tableName;

	wxString whereText; bool firstWhere = true;
	for (auto filter : m_filterRow.m_filters) {
		if (filter.m_filterUse) {
			if (firstWhere)
				whereText += " WHERE ";
			const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(filter.m_filterModel);
			wxASSERT(attribute);
			whereText += (firstWhere ? " " : " AND ") + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attribute, filter.m_filterComparison == eComparisonType::eComparisonType_Equal ? "=" : "<>");
			if (firstWhere)
				firstWhere = false;
		}
	}
	const std::vector<IValueMetaObjectAttribute*>& vec_attr = m_metaObject->GetGenericAttributeArrayObject(), & vec_dim = m_metaObject->GetGenericDimentionArrayObject();
	/////////////////////////////////////////////////////////
	wxString orderText; bool firstOrder = true;
	/////////////////////////////////////////////////////////
	for (auto& sort : m_sortOrder.m_sorts) {
		if (sort.m_sortEnable) {
			const wxString& operation = sort.m_sortAscending ? "ASC" : "DESC";
			const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(sort.m_sortModel);
			wxASSERT(attribute);
			const CTypeDescription& typeDesc = attribute->GetTypeDesc();
			if (firstOrder)
				orderText += " ORDER BY ";
			for (auto& field : IValueMetaObjectAttribute::GetSQLFieldData(attribute)) {
				if (field.m_type == IValueMetaObjectAttribute::eFieldTypes_Reference) {
					orderText += (firstOrder ? " " : ", ") + wxString::Format("%s %s", field.m_field.m_fieldRefName.m_fieldRefName, operation);
					if (firstOrder) firstOrder = false;
				}
				else if (field.m_type == IValueMetaObjectAttribute::eFieldTypes_String) {
					orderText += (firstOrder ? " " : ", ") + wxString::Format("LPAD(%s, %i, ' ') %s", field.m_field.m_fieldName, typeDesc.GetStringQualifier().m_length, operation);
					if (firstOrder) firstOrder = false;
				}
				else {
					orderText += (firstOrder ? " " : ", ") + wxString::Format("%s %s", field.m_field.m_fieldName, operation);
					if (firstOrder) firstOrder = false;
				}
			}
		}
	};
	/////////////////////////////////////////////////////////
	if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
		queryText = queryText + whereText + orderText + " LIMIT " + stringUtils::IntToStr(countPerPage + 1);
	else
		queryText = queryText + whereText + orderText;

	IValueTable::Clear();
	IPreparedStatement* statement = db_query->PrepareStatement(queryText); int position = 1;
	for (auto& filter : m_filterRow.m_filters) {
		if (filter.m_filterUse) {
			const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(filter.m_filterModel);
			wxASSERT(attribute);
			IValueMetaObjectAttribute::SetValueAttribute(attribute, filter.m_filterValue, statement, position);
		}
	}
	/////////////////////////////////////////////////////////
	IValueTable::Reserve(countPerPage + 1);
	/////////////////////////////////////////////////////////
	IDatabaseResultSet* resultSet = statement->RunQueryWithResults();
	while (resultSet->Next()) {
		wxValueTableKeyRow* rowData = new wxValueTableKeyRow;
		if (m_metaObject->HasRecorder()) {
			CValueMetaObjectAttributePredefined* attributeRecorder = m_metaObject->GetRegisterRecorder();
			wxASSERT(attributeRecorder);
			IValueMetaObjectAttribute::GetValueAttribute(attributeRecorder, rowData->AppendNodeValue(attributeRecorder->GetMetaID()), resultSet);
			CValueMetaObjectAttributePredefined* attributeNumberLine = m_metaObject->GetRegisterLineNumber();
			wxASSERT(attributeNumberLine);
			IValueMetaObjectAttribute::GetValueAttribute(attributeNumberLine, rowData->AppendNodeValue(attributeNumberLine->GetMetaID()), resultSet);
		}
		else {
			for (auto& dimension : vec_dim) {
				IValueMetaObjectAttribute::GetValueAttribute(dimension, rowData->AppendNodeValue(dimension->GetMetaID()), resultSet);
			}
		}
		for (auto& attribute : vec_attr) {
			IValueMetaObjectAttribute::GetValueAttribute(attribute, rowData->AppendTableValue(attribute->GetMetaID()), resultSet);
		}
		IValueTable::Append(
			rowData, !CBackendException::IsEvalMode()
		);
	};

	db_query->CloseResultSet(resultSet);
	db_query->CloseStatement(statement);
}

void CValueListRegisterObject::RefreshItemModel(const wxDataViewItem& topItem, const wxDataViewItem& currentItem, const int countPerPage, const short scroll)
{
	const long row_top = GetRow(topItem);
	const long row_current = GetRow(currentItem);

	if (row_top == 0 && countPerPage < GetRowCount() && scroll > 0) {

		wxValueTableKeyRow* valueTableListRow = GetViewData<wxValueTableKeyRow>(GetItem(0));
		const wxString& tableName = m_metaObject->GetTableNameDB();

		wxString queryText;
		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
			queryText = wxString::Format("SELECT * FROM %s ", tableName);
		else
			queryText = wxString::Format("SELECT FIRST 1 * FROM %s ", tableName);

		wxString whereText; bool firstWhere = true;
		for (auto& filter : m_filterRow.m_filters) {
			if (filter.m_filterUse) {
				const wxString& operation = filter.m_filterComparison == eComparisonType::eComparisonType_Equal ? "=" : "<>";
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(filter.m_filterModel);
				wxASSERT(attribute);
				if (firstWhere)
					whereText += " WHERE ";
				whereText += (firstWhere ? " " : " AND ") + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attribute, operation);
				if (firstWhere)
					firstWhere = false;
			}
		}
		wxString orderText; bool firstOrder = true; bool firstOr = true; wxString dimText;
		for (auto& sort : m_sortOrder.m_sorts) {
			if (sort.m_sortEnable) {
				const wxString& operation_sort = (!sort.m_sortAscending) ? "ASC" : "DESC";
				const wxString& operation_compare = (!sort.m_sortAscending) ? ">=" : "<=";
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(sort.m_sortModel);
				wxASSERT(attribute);
				const CTypeDescription& typeDesc = attribute->GetTypeDesc();
				if (firstOrder)
					orderText += " ORDER BY ";
				for (auto& field : IValueMetaObjectAttribute::GetSQLFieldData(attribute)) {
					if (field.m_type == IValueMetaObjectAttribute::eFieldTypes_Reference) {
						orderText += (firstOrder ? " " : ", ") + wxString::Format("%s %s", field.m_field.m_fieldRefName.m_fieldRefName, operation_sort);
						if (firstOrder)
							firstOrder = false;
					}
					else if (field.m_type == IValueMetaObjectAttribute::eFieldTypes_String) {
						orderText += (firstOrder ? " " : ", ") + wxString::Format("LPAD(%s, %i, ' ') %s", field.m_field.m_fieldName, typeDesc.GetStringQualifier().m_length, operation_sort);
						if (firstOrder) firstOrder = false;
					}
					else {
						orderText += (firstOrder ? " " : ", ") + wxString::Format("%s %s", field.m_field.m_fieldName, operation_sort);
						if (firstOrder) firstOrder = false;
					}
				}
				dimText += firstOr ? " (" + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attribute, "<>") + ") " : " OR (" + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attribute, "<>") + ") ";
				if (firstWhere) whereText += " WHERE ";
				whereText += (firstWhere ? " " : " AND ") + attribute->GetFieldNameDB() + "_TYPE = ? ";
				if (firstWhere) firstWhere = false;
				for (auto& field : IValueMetaObjectAttribute::GetSQLFieldData(attribute)) {
					if (field.m_type == IValueMetaObjectAttribute::eFieldTypes_Reference) {
						whereText += (firstWhere ? " " : " AND ") + wxString::Format("%s %s ?", field.m_field.m_fieldRefName.m_fieldRefType, operation_compare);
						whereText += (firstWhere ? " " : " AND ") + wxString::Format("%s %s ?", field.m_field.m_fieldRefName.m_fieldRefName, operation_compare);
					}
					else if (field.m_type == IValueMetaObjectAttribute::eFieldTypes_String) {
						whereText += (firstWhere ? " " : " AND ") + wxString::Format("LPAD(%s, %i, ' ') %s LPAD(cast(? as varchar(1024)), %i, ' ')", field.m_field.m_fieldName, typeDesc.GetStringQualifier().m_length, operation_compare, typeDesc.GetStringQualifier().m_length);
					}
					else {
						whereText += (firstWhere ? " " : " AND ") + wxString::Format("%s %s ?", field.m_field.m_fieldName, operation_compare);
					}
				}
				firstOr = false;
			}
		};

		const std::vector<IValueMetaObjectAttribute*> vec_attr = m_metaObject->GetGenericAttributeArrayObject(),
			vec_dim = m_metaObject->GetGenericDimentionArrayObject();

		/////////////////////////////////////////////////////////
		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
			queryText = queryText + whereText + " AND (" + dimText + ") " + orderText + " LIMIT " + stringUtils::IntToStr(1);
		else
			queryText = queryText + whereText + " AND (" + dimText + ") " + orderText;
		/////////////////////////////////////////////////////////
		IPreparedStatement* statement = db_query->PrepareStatement(queryText); int position = 1;
		for (auto& filter : m_filterRow.m_filters) {
			if (filter.m_filterUse) {
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(filter.m_filterModel);
				wxASSERT(attribute);
				IValueMetaObjectAttribute::SetValueAttribute(attribute, filter.m_filterValue, statement, position);
			}
		}
		/////////////////////////////////////////////////////////
		for (auto& sort : m_sortOrder.m_sorts) {
			if (sort.m_sortEnable) {
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(sort.m_sortModel);
				wxASSERT(attribute);
				IValueMetaObjectAttribute::SetValueAttribute(attribute, valueTableListRow->GetTableValue(sort.m_sortModel), statement, position);
			}
		}
		/////////////////////////////////////////////////////////
		for (auto& sort : m_sortOrder.m_sorts) {
			if (sort.m_sortEnable) {
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(sort.m_sortModel);
				wxASSERT(attribute);
				IValueMetaObjectAttribute::SetValueAttribute(attribute, valueTableListRow->GetTableValue(sort.m_sortModel), statement, position);
			}
		}
		/////////////////////////////////////////////////////////
		IDatabaseResultSet* resultSet = statement->RunQueryWithResults();
		/////////////////////////////////////////////////////////
		bool insertedValue = false;
		/////////////////////////////////////////////////////////
		while (resultSet->Next()) {
			wxValueTableKeyRow* rowData = new wxValueTableKeyRow;
			if (m_metaObject->HasRecorder()) {
				CValueMetaObjectAttributePredefined* attributeRecorder = m_metaObject->GetRegisterRecorder();
				wxASSERT(attributeRecorder);
				IValueMetaObjectAttribute::GetValueAttribute(attributeRecorder, rowData->AppendNodeValue(attributeRecorder->GetMetaID()), resultSet);
				CValueMetaObjectAttributePredefined* attributeNumberLine = m_metaObject->GetRegisterLineNumber();
				wxASSERT(attributeNumberLine);
				IValueMetaObjectAttribute::GetValueAttribute(attributeNumberLine, rowData->AppendNodeValue(attributeNumberLine->GetMetaID()), resultSet);
			}
			else {
				for (auto& dimension : vec_dim) {
					IValueMetaObjectAttribute::GetValueAttribute(dimension, rowData->AppendNodeValue(dimension->GetMetaID()), resultSet);
				}
			}
			for (auto& attribute : vec_attr) {
				IValueMetaObjectAttribute::GetValueAttribute(attribute, rowData->AppendTableValue(attribute->GetMetaID()), resultSet);
			}
			IValueTable::Insert(rowData, 0, !CBackendException::IsEvalMode());
			/////////////////////////////////////////////////////////
			insertedValue = true;
			/////////////////////////////////////////////////////////
		};
		/////////////////////////////////////////////////////////
		if (insertedValue) IValueTable::ClearRange(IValueTable::GetRowCount() - 1, IValueTable::GetRowCount(), !CBackendException::IsEvalMode());
		/////////////////////////////////////////////////////////
		db_query->CloseResultSet(resultSet);
		db_query->CloseStatement(statement);
	}
	else if (row_top + countPerPage == GetRowCount() && scroll < 0) {

		wxValueTableKeyRow* valueTableListRow = GetViewData<wxValueTableKeyRow>(GetItem(GetRowCount() - 1));
		const wxString& tableName = m_metaObject->GetTableNameDB();

		wxString queryText;
		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
			queryText = wxString::Format("SELECT * FROM %s ", tableName);
		else
			queryText = wxString::Format("SELECT FIRST 1 * FROM %s ", tableName);

		wxString whereText; bool firstWhere = true;
		for (auto& filter : m_filterRow.m_filters) {
			if (filter.m_filterUse) {
				const wxString& operation = filter.m_filterComparison == eComparisonType::eComparisonType_Equal ? "=" : "<>";
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(filter.m_filterModel);
				wxASSERT(attribute);
				if (firstWhere)
					whereText += " WHERE ";
				whereText += (firstWhere ? " " : " AND ") + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attribute, operation);
				if (firstWhere)
					firstWhere = false;
			}
		}
		wxString orderText; bool firstOrder = true; bool firstOr = true; wxString dimText;
		for (auto& sort : m_sortOrder.m_sorts) {
			if (sort.m_sortEnable) {
				const wxString& operation_sort = (sort.m_sortAscending) ? "ASC" : "DESC";
				const wxString& operation_compare = (sort.m_sortAscending) ? ">=" : "<=";
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(sort.m_sortModel);
				wxASSERT(attribute);
				const CTypeDescription& typeDesc = attribute->GetTypeDesc();
				if (firstOrder)
					orderText += " ORDER BY ";
				for (auto& field : IValueMetaObjectAttribute::GetSQLFieldData(attribute)) {
					if (field.m_type == IValueMetaObjectAttribute::eFieldTypes_Reference) {
						orderText += (firstOrder ? " " : ", ") + wxString::Format("%s %s", field.m_field.m_fieldRefName.m_fieldRefName, operation_sort);
						if (firstOrder)
							firstOrder = false;
					}
					else if (field.m_type == IValueMetaObjectAttribute::eFieldTypes_String) {
						orderText += (firstOrder ? " " : ", ") + wxString::Format("LPAD(%s, %i, ' ') %s", field.m_field.m_fieldName, typeDesc.GetStringQualifier().m_length, operation_sort);
						if (firstOrder) firstOrder = false;
					}
					else {
						orderText += (firstOrder ? " " : ", ") + wxString::Format("%s %s", field.m_field.m_fieldName, operation_sort);
						if (firstOrder) firstOrder = false;
					}
				}
				dimText += firstOr ? " (" + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attribute, "<>") + ") " : " OR (" + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attribute, "<>") + ") ";
				if (firstWhere) whereText += " WHERE ";
				whereText += (firstWhere ? " " : " AND ") + attribute->GetFieldNameDB() + "_TYPE = ? ";
				if (firstWhere) firstWhere = false;
				for (auto& field : IValueMetaObjectAttribute::GetSQLFieldData(attribute)) {
					if (field.m_type == IValueMetaObjectAttribute::eFieldTypes_Reference) {
						whereText += (firstWhere ? " " : " AND ") + wxString::Format("%s %s ?", field.m_field.m_fieldRefName.m_fieldRefType, operation_compare);
						whereText += (firstWhere ? " " : " AND ") + wxString::Format("%s %s ?", field.m_field.m_fieldRefName.m_fieldRefName, operation_compare);
					}
					else if (field.m_type == IValueMetaObjectAttribute::eFieldTypes_String) {
						whereText += (firstWhere ? " " : " AND ") + wxString::Format("LPAD(%s, %i, ' ') %s LPAD(cast(? as varchar(1024)), %i, ' ')", field.m_field.m_fieldName, typeDesc.GetStringQualifier().m_length, operation_compare, typeDesc.GetStringQualifier().m_length);
					}
					else {
						whereText += (firstWhere ? " " : " AND ") + wxString::Format("%s %s ?", field.m_field.m_fieldName, operation_compare);
					}
				}
				firstOr = false;
			}
		};

		const std::vector<IValueMetaObjectAttribute*> vec_attr = m_metaObject->GetGenericAttributeArrayObject(),
			vec_dim = m_metaObject->GetGenericDimentionArrayObject();

		/////////////////////////////////////////////////////////
		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
			queryText = queryText + whereText + " AND (" + dimText + ") " + orderText + " LIMIT " + stringUtils::IntToStr(1);
		else
			queryText = queryText + whereText + " AND (" + dimText + ") " + orderText;
		/////////////////////////////////////////////////////////
		IPreparedStatement* statement = db_query->PrepareStatement(queryText); int position = 1;
		for (auto filter : m_filterRow.m_filters) {
			if (filter.m_filterUse) {
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(filter.m_filterModel);
				wxASSERT(attribute);
				IValueMetaObjectAttribute::SetValueAttribute(attribute, filter.m_filterValue, statement, position);
			}
		}
		/////////////////////////////////////////////////////////
		for (auto& sort : m_sortOrder.m_sorts) {
			if (sort.m_sortEnable) {
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(sort.m_sortModel);
				wxASSERT(attribute);
				IValueMetaObjectAttribute::SetValueAttribute(attribute, valueTableListRow->GetTableValue(sort.m_sortModel), statement, position);
			}
		}
		/////////////////////////////////////////////////////////
		for (auto& sort : m_sortOrder.m_sorts) {
			if (sort.m_sortEnable) {
				const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(sort.m_sortModel);
				wxASSERT(attribute);
				IValueMetaObjectAttribute::SetValueAttribute(attribute, valueTableListRow->GetTableValue(sort.m_sortModel), statement, position);
			}
		}
		/////////////////////////////////////////////////////////
		IDatabaseResultSet* resultSet = statement->RunQueryWithResults();
		/////////////////////////////////////////////////////////
		bool insertedValue = false;
		/////////////////////////////////////////////////////////
		while (resultSet->Next()) {
			wxValueTableKeyRow* rowData = new wxValueTableKeyRow;
			if (m_metaObject->HasRecorder()) {
				CValueMetaObjectAttributePredefined* attributeRecorder = m_metaObject->GetRegisterRecorder();
				wxASSERT(attributeRecorder);
				IValueMetaObjectAttribute::GetValueAttribute(attributeRecorder, rowData->AppendNodeValue(attributeRecorder->GetMetaID()), resultSet);
				CValueMetaObjectAttributePredefined* attributeNumberLine = m_metaObject->GetRegisterLineNumber();
				wxASSERT(attributeNumberLine);
				IValueMetaObjectAttribute::GetValueAttribute(attributeNumberLine, rowData->AppendNodeValue(attributeNumberLine->GetMetaID()), resultSet);
			}
			else {
				for (auto& dimension : vec_dim) {
					IValueMetaObjectAttribute::GetValueAttribute(dimension, rowData->AppendNodeValue(dimension->GetMetaID()), resultSet);
				}
			}
			for (auto& attribute : vec_attr) {
				IValueMetaObjectAttribute::GetValueAttribute(attribute, rowData->AppendTableValue(attribute->GetMetaID()), resultSet);
			}
			IValueTable::Append(rowData, !CBackendException::IsEvalMode());
			/////////////////////////////////////////////////////////
			insertedValue = true;
			/////////////////////////////////////////////////////////
		};
		/////////////////////////////////////////////////////////
		if (insertedValue) IValueTable::ClearRange(0, 1, !CBackendException::IsEvalMode());
		/////////////////////////////////////////////////////////
		db_query->CloseResultSet(resultSet);
		db_query->CloseStatement(statement);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void CValueTreeDataObjectFolderRef::RefreshModel(const wxDataViewItem& topItem, const int countPerPage)
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	CValue isFolder, parent;
	const wxString& tableName = m_metaObject->GetTableNameDB();
	wxString queryText = "SELECT * FROM " + tableName;
	wxString whereText; bool firstWhere = true;
	for (auto& filter : m_filterRow.m_filters) {
		const wxString& operation = filter.m_filterComparison == eComparisonType::eComparisonType_Equal ? "=" : "<>";
		if (filter.m_filterUse) {
			const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(filter.m_filterModel);
			wxASSERT(attribute);
			if (!m_metaObject->IsDataReference(filter.m_filterModel)) {
				if (firstWhere)
					whereText += " WHERE ";
				whereText += (firstWhere ? " " : " AND ") + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attribute, operation);
				if (firstWhere)
					firstWhere = false;
			}
			else {
				if (firstWhere)
					whereText += " WHERE ";
				whereText += (firstWhere ? " " : " AND ") + wxString::Format("%s %s ?", guidName, operation);
				if (firstWhere)
					firstWhere = false;
			}
		}
	}
	const std::vector<IValueMetaObjectAttribute*>& vec_attr = m_metaObject->GetGenericAttributeArrayObject();
	CValueMetaObjectAttributePredefined* metaReference = m_metaObject->GetDataReference();
	CValueMetaObjectAttributePredefined* metaParent = m_metaObject->GetDataParent();
	CValueMetaObjectAttributePredefined* metaIsFolder = m_metaObject->GetDataIsFolder();
	wxASSERT(metaReference);
	queryText = queryText + whereText;
	IValueTree::Clear();
	IPreparedStatement* statement = db_query->PrepareStatement(queryText); int position = 1;
	for (auto filter : m_filterRow.m_filters) {
		if (filter.m_filterUse) {
			const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(filter.m_filterModel);
			wxASSERT(attribute);
			if (m_metaObject->IsDataReference(filter.m_filterModel)) {
				const CValue& filterValue = filter.m_filterValue; CValueReferenceDataObject* refData = nullptr;
				if (filterValue.ConvertToValue(refData))
					statement->SetParamString(position++, refData->GetGuid());
				else
					statement->SetParamString(position++, wxNullUniqueKey);
			}
			else {
				IValueMetaObjectAttribute::SetValueAttribute(attribute, filter.m_filterValue, statement, position);
			}
		}
	}
	//////////////////////////////////////////////////
	std::vector<wxValueTreeListNode*> arrTree;
	//////////////////////////////////////////////////
	
	IDatabaseResultSet* resultSet = statement->RunQueryWithResults();
	while (resultSet->Next()) {

		if (!IValueMetaObjectAttribute::GetValueAttribute(metaIsFolder, isFolder, resultSet)) continue;
		if (m_listMode == CValueTreeDataObjectFolderRef::LIST_FOLDER && !isFolder.GetBoolean()) continue;
		if (!IValueMetaObjectAttribute::GetValueAttribute(metaParent, parent, resultSet)) continue;

		wxValueTreeListNode* rowData = new wxValueTreeListNode(nullptr, resultSet->GetResultString(guidName), this, isFolder.GetBoolean());
		for (auto& attribute : vec_attr) {
			if (m_metaObject->IsDataReference(attribute->GetMetaID()))
				continue;
			IValueMetaObjectAttribute::GetValueAttribute(attribute, rowData->AppendTableValue(attribute->GetMetaID()), resultSet);
		}
		rowData->AppendTableValue(metaReference->GetMetaID(), CValueReferenceDataObject::CreateFromResultSet(resultSet, m_metaObject, rowData->GetGuid()));
		
		//////////////////////////////////////////////////
		arrTree.push_back(rowData);
		//////////////////////////////////////////////////
	};

	/* wxDataViewModel::*/ m_modelProvider->BeforeReset();

	static CValue cReference;
	for (const auto node : arrTree) {
		
		CValueReferenceDataObject* reference = NULL;
		if (node->GetValue(*m_metaObject->GetDataParent(), cReference)) {
			if (cReference.ConvertToValue(reference)) {
				
				auto iterator = std::find_if(arrTree.begin(), arrTree.end(),
					[reference](wxValueTreeListNode* node) { return reference->GetGuid() == node->GetGuid(); });

				if (iterator != arrTree.end())
					node->SetParent(*iterator);
				else
					node->SetParent(GetRoot());
			}
		}
	}

	/* wxDataViewModel:: */ m_modelProvider->AfterReset();

	db_query->CloseResultSet(resultSet);
	db_query->CloseStatement(statement);
}

/////////////////////////////////////////////////////////////////////////////////////////////////