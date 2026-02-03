////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : informationRegister manager
////////////////////////////////////////////////////////////////////////////

#include "informationRegister.h"
#include "informationRegisterManager.h"

#include "backend/system/value/valueMap.h"
#include "backend/system/value/valueTable.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/appData.h"

CValue CManagerDataObjectInformationRegister::Get(const CValue& cFilter)
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	CValueTable* retTable = CValue::CreateAndPrepareValueRef<CValueTable>();
	CValueTable::IValueModelColumnCollection* colCollection = retTable->GetColumnCollection();
	wxASSERT(colCollection);
	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		CValueTable::IValueModelColumnCollection::IValueModelColumnInfo* colInfo = colCollection->AddColumn(object->GetName(), object->GetTypeDesc(), object->GetSynonym());
		colInfo->SetColumnID(object->GetMetaID());
	}

	CValueStructure* valFilter = nullptr; std::map<IMetaObjectAttribute*, CValue> selFilter;
	if (cFilter.ConvertToValue(valFilter)) {
		for (const auto object : m_metaObject->GetDimentionArrayObject()) {
			CValue vSelValue;
			if (valFilter->Property(object->GetName(), vSelValue)) {
				selFilter.insert_or_assign(
					object, vSelValue
				);
			}
		}
	}

	wxString queryText = "SELECT * FROM %s "; bool firstWhere = true;

	for (auto filter : selFilter) {
		if (firstWhere) {
			queryText = queryText + " WHERE ";
		}
		queryText = queryText +
			(firstWhere ? " " : " AND ") + IMetaObjectAttribute::GetCompositeSQLFieldName(filter.first);
		if (firstWhere) {
			firstWhere = false;
		}
	}

	IPreparedStatement* statement = db_query->PrepareStatement(queryText, m_metaObject->GetTableNameDB());

	if (statement == nullptr)
		return retTable;

	int position = 1;
	for (auto filter : selFilter) {
		IMetaObjectAttribute::SetValueAttribute(filter.first, filter.second, statement, position);
	}

	IDatabaseResultSet* resultSet = statement->RunQueryWithResults();

	if (resultSet == nullptr)
		return retTable;

	while (resultSet->Next()) {
		CValueTable::CValueTableReturnLine* retLine = retTable->GetRowAt(retTable->AppendRow());
		wxASSERT(retLine);
		for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
			CValue retValue;
			if (IMetaObjectAttribute::GetValueAttribute(object, retValue, resultSet))
				retLine->SetValueByMetaID(object->GetMetaID(), retValue);
		}
		wxDELETE(retLine);
	}

	resultSet->Close();
	statement->Close();

	return retTable;
}

CValue CManagerDataObjectInformationRegister::Get(const CValue& cPeriod, const CValue& cFilter)
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	CValueTable* retTable = CValue::CreateAndPrepareValueRef<CValueTable>();
	CValueTable::IValueModelColumnCollection* colCollection = retTable->GetColumnCollection();
	wxASSERT(colCollection);
	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		CValueTable::IValueModelColumnCollection::IValueModelColumnInfo* colInfo =
			colCollection->AddColumn(
				object->GetName(),
				object->GetTypeDesc(),
				object->GetSynonym()
			);
		colInfo->SetColumnID(object->GetMetaID());
	}

	if (m_metaObject->GetPeriodicity() != ePeriodicity::eNonPeriodic ||
		m_metaObject->GetWriteRegisterMode() == eWriteRegisterMode::eSubordinateRecorder) {
		CValueStructure* valFilter = nullptr; std::map<IMetaObjectAttribute*, CValue> selFilter;
		if (cFilter.ConvertToValue(valFilter)) {
			for (const auto object : m_metaObject->GetDimentionArrayObject()) {
				CValue vSelValue;
				if (valFilter->Property(object->GetName(), vSelValue)) {
					selFilter.insert_or_assign(
						object, vSelValue
					);
				}
			}
		}

		wxString queryText = "SELECT * FROM %s WHERE " + IMetaObjectAttribute::GetCompositeSQLFieldName(m_metaObject->GetRegisterPeriod());

		for (auto filter : selFilter) {
			queryText = queryText +
				" AND " + IMetaObjectAttribute::GetCompositeSQLFieldName(filter.first);
		}

		IPreparedStatement* statement = db_query->PrepareStatement(queryText, m_metaObject->GetTableNameDB());

		if (statement == nullptr)
			return retTable;

		int position = 1;

		IMetaObjectAttribute::SetValueAttribute(m_metaObject->GetRegisterPeriod(), cPeriod.GetDate(), statement, position);

		for (auto filter : selFilter) {
			IMetaObjectAttribute::SetValueAttribute(filter.first, filter.second, statement, position);
		}

		IDatabaseResultSet* resultSet = statement->RunQueryWithResults();

		if (resultSet == nullptr)
			return retTable;

		while (resultSet->Next()) {
			CValueTable::CValueTableReturnLine* retLine = retTable->GetRowAt(retTable->AppendRow());
			wxASSERT(retLine);
			for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
				CValue retValue;
				if (IMetaObjectAttribute::GetValueAttribute(object, retValue, resultSet))
					retLine->SetValueByMetaID(object->GetMetaID(), retValue);
			}
			wxDELETE(retLine);
		}

		db_query->CloseResultSet(resultSet);
		db_query->CloseStatement(statement);
	}

	return retTable;
}

CValue CManagerDataObjectInformationRegister::GetFirst(const CValue& cPeriod, const CValue& cFilter)
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	CValueStructure* retTable = CValue::CreateAndPrepareValueRef<CValueStructure>();
	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		retTable->SetAt(object->GetName(), CValue());
	}

	if (m_metaObject->GetPeriodicity() != ePeriodicity::eNonPeriodic ||
		m_metaObject->GetWriteRegisterMode() == eWriteRegisterMode::eSubordinateRecorder) {
		CValueStructure* valFilter = nullptr; std::map<IMetaObjectAttribute*, CValue> selFilter;
		if (cFilter.ConvertToValue(valFilter)) {
			for (const auto object : m_metaObject->GetDimentionArrayObject()) {
				CValue vSelValue;
				if (valFilter->Property(object->GetName(), vSelValue)) {
					selFilter.insert_or_assign(
						object, vSelValue
					);
				}
			}
		}

		IMetaObjectAttribute::sqlField_t sqlCol =
			IMetaObjectAttribute::GetSQLFieldData(m_metaObject->GetRegisterPeriod());

		wxString sqlQuery = " SELECT * FROM ( SELECT T2." + sqlCol.m_fieldTypeName;

		for (auto dataType : sqlCol.m_types) {
			if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
				sqlQuery += ", T2." + dataType.m_field.m_fieldName;
			}
			else {
				sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
				sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
			}
		}

		for (const auto object : m_metaObject->GetDimentionArrayObject()) {
			IMetaObjectAttribute::sqlField_t sqlDim = IMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += ", T2." + sqlDim.m_fieldTypeName;
			for (auto dataType : sqlDim.m_types) {
				if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += ", T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		for (const auto object : m_metaObject->GetResourceArrayObject()) {
			IMetaObjectAttribute::sqlField_t sqlRes = IMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " , T2." + sqlRes.m_fieldTypeName;
			for (auto dataType : sqlRes.m_types) {
				if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += ", T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		sqlQuery += " FROM (SELECT ";
		sqlQuery += IMetaObjectAttribute::GetSQLFieldName(m_metaObject->GetRegisterPeriod(), "MIN");
		for (const auto object : m_metaObject->GetDimentionArrayObject()) {
			sqlQuery += "," + IMetaObjectAttribute::GetSQLFieldName(object);
		}
		for (const auto object : m_metaObject->GetResourceArrayObject()) {
			sqlQuery += "," + IMetaObjectAttribute::GetSQLFieldName(object);
		}
		sqlQuery += " FROM " + m_metaObject->GetTableNameDB();
		sqlQuery += " WHERE " + IMetaObjectAttribute::GetCompositeSQLFieldName(m_metaObject->GetRegisterActive());
		sqlQuery += " AND " + IMetaObjectAttribute::GetCompositeSQLFieldName(m_metaObject->GetRegisterPeriod(), ">=");
		sqlQuery += " GROUP BY ";
		sqlQuery += sqlCol.m_fieldTypeName;
		for (const auto object : m_metaObject->GetDimentionArrayObject()) {
			sqlQuery += "," + IMetaObjectAttribute::GetSQLFieldName(object);
		}
		for (const auto object : m_metaObject->GetResourceArrayObject()) {
			sqlQuery += "," + IMetaObjectAttribute::GetSQLFieldName(object);
		}
		sqlQuery += ") AS T1 "
			"INNER JOIN " + m_metaObject->GetTableNameDB() + " AS T2 "
			" ON ";

		sqlQuery += " T1." + sqlCol.m_fieldTypeName + " = T2." + sqlCol.m_fieldTypeName;

		for (auto dataType : sqlCol.m_types) {
			if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
				sqlQuery += " AND T1." + dataType.m_field.m_fieldName + " = T2." + dataType.m_field.m_fieldName;
			}
			else {
				sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefType + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
				sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefName + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
			}
		}

		for (const auto object : m_metaObject->GetDimentionArrayObject()) {
			IMetaObjectAttribute::sqlField_t sqlDim = IMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " AND T1." + sqlDim.m_fieldTypeName + " = T2." + sqlDim.m_fieldTypeName;
			for (auto dataType : sqlDim.m_types) {
				if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldName + " = T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefType + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefName + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		for (const auto object : m_metaObject->GetResourceArrayObject()) {
			IMetaObjectAttribute::sqlField_t sqlRes = IMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " AND T1." + sqlRes.m_fieldTypeName + " = T2." + sqlRes.m_fieldTypeName;
			for (auto dataType : sqlRes.m_types) {
				if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldName + " = T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefType + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefName + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		sqlQuery += ") AS lastTable"; bool firstWhere = true;

		for (auto filter : selFilter) {
			if (firstWhere) {
				sqlQuery = sqlQuery + " WHERE ";
			}
			sqlQuery = sqlQuery +
				(firstWhere ? " " : " AND ") + IMetaObjectAttribute::GetCompositeSQLFieldName(filter.first);
			if (firstWhere) {
				firstWhere = false;
			}
		}

		IPreparedStatement* statement = db_query->PrepareStatement(sqlQuery);

		if (statement == nullptr)
			return retTable;

		int position = 1;

		IMetaObjectAttribute::SetValueAttribute(m_metaObject->GetRegisterActive(), true, statement, position);
		IMetaObjectAttribute::SetValueAttribute(m_metaObject->GetRegisterPeriod(), cPeriod.GetDate(), statement, position);

		for (auto filter : selFilter) {
			IMetaObjectAttribute::SetValueAttribute(filter.first, filter.second, statement, position);
		}

		IDatabaseResultSet* resultSet = statement->RunQueryWithResults();

		if (resultSet == nullptr)
			return retTable;

		if (resultSet->Next()) {
			for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
				CValue retValue; 
				if(IMetaObjectAttribute::GetValueAttribute(object, retValue, resultSet))
					retTable->SetAt(object->GetName(), retValue);
			}
		}

		db_query->CloseResultSet(resultSet);
		db_query->CloseStatement(statement);
	}

	return retTable;
}

CValue CManagerDataObjectInformationRegister::GetLast(const CValue& cPeriod, const CValue& cFilter)
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	CValueStructure* retTable = CValue::CreateAndPrepareValueRef<CValueStructure>();
	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		retTable->SetAt(object->GetName(), CValue());
	}

	if (m_metaObject->GetPeriodicity() != ePeriodicity::eNonPeriodic ||
		m_metaObject->GetWriteRegisterMode() == eWriteRegisterMode::eSubordinateRecorder) {
		CValueStructure* valFilter = nullptr; std::map<IMetaObjectAttribute*, CValue> selFilter;
		if (cFilter.ConvertToValue(valFilter)) {
			for (const auto object : m_metaObject->GetDimentionArrayObject()) {
				CValue vSelValue;
				if (valFilter->Property(object->GetName(), vSelValue)) {
					selFilter.insert_or_assign(
						object, vSelValue
					);
				}
			}
		}

		IMetaObjectAttribute::sqlField_t sqlCol =
			IMetaObjectAttribute::GetSQLFieldData(m_metaObject->GetRegisterPeriod());

		wxString sqlQuery = " SELECT * FROM ( SELECT T2." + sqlCol.m_fieldTypeName;

		for (auto dataType : sqlCol.m_types) {
			if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
				sqlQuery += ", T2." + dataType.m_field.m_fieldName;
			}
			else {
				sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
				sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
			}
		}

		for (const auto object : m_metaObject->GetDimentionArrayObject()) {
			IMetaObjectAttribute::sqlField_t sqlDim = IMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += ", T2." + sqlDim.m_fieldTypeName;
			for (auto dataType : sqlDim.m_types) {
				if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += ", T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		for (const auto object : m_metaObject->GetResourceArrayObject()) {
			IMetaObjectAttribute::sqlField_t sqlRes = IMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " , T2." + sqlRes.m_fieldTypeName;
			for (auto dataType : sqlRes.m_types) {
				if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += ", T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		sqlQuery += " FROM (SELECT ";
		sqlQuery += IMetaObjectAttribute::GetSQLFieldName(m_metaObject->GetRegisterPeriod(), "MAX");
		for (const auto object : m_metaObject->GetDimentionArrayObject()) {
			sqlQuery += "," + IMetaObjectAttribute::GetSQLFieldName(object);
		}
		for (const auto object : m_metaObject->GetResourceArrayObject()) {
			sqlQuery += "," + IMetaObjectAttribute::GetSQLFieldName(object);
		}
		sqlQuery += " FROM " + m_metaObject->GetTableNameDB();
		sqlQuery += " WHERE " + IMetaObjectAttribute::GetCompositeSQLFieldName(m_metaObject->GetRegisterActive());
		sqlQuery += " AND " + IMetaObjectAttribute::GetCompositeSQLFieldName(m_metaObject->GetRegisterPeriod(), "<=");
		sqlQuery += " GROUP BY ";
		sqlQuery += sqlCol.m_fieldTypeName;
		for (const auto object : m_metaObject->GetDimentionArrayObject()) {
			sqlQuery += "," + IMetaObjectAttribute::GetSQLFieldName(object);
		}
		for (const auto object : m_metaObject->GetResourceArrayObject()) {
			sqlQuery += "," + IMetaObjectAttribute::GetSQLFieldName(object);
		}
		sqlQuery += ") AS T1 "
			"INNER JOIN " + m_metaObject->GetTableNameDB() + " AS T2 "
			" ON ";

		sqlQuery += " T1." + sqlCol.m_fieldTypeName + " = T2." + sqlCol.m_fieldTypeName;

		for (auto dataType : sqlCol.m_types) {
			if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
				sqlQuery += " AND T1." + dataType.m_field.m_fieldName + " = T2." + dataType.m_field.m_fieldName;
			}
			else {
				sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefType + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
				sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefName + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
			}
		}

		for (const auto object : m_metaObject->GetDimentionArrayObject()) {
			IMetaObjectAttribute::sqlField_t sqlDim = IMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " AND T1." + sqlDim.m_fieldTypeName + " = T2." + sqlDim.m_fieldTypeName;
			for (auto dataType : sqlDim.m_types) {
				if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldName + " = T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefType + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefName + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		for (const auto object : m_metaObject->GetResourceArrayObject()) {
			IMetaObjectAttribute::sqlField_t sqlRes = IMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " AND T1." + sqlRes.m_fieldTypeName + " = T2." + sqlRes.m_fieldTypeName;
			for (auto dataType : sqlRes.m_types) {
				if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldName + " = T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefType + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefName + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		sqlQuery += ") AS lastTable"; bool firstWhere = true;

		for (auto filter : selFilter) {
			if (firstWhere) {
				sqlQuery = sqlQuery + " WHERE ";
			}
			sqlQuery = sqlQuery +
				(firstWhere ? " " : " AND ") + IMetaObjectAttribute::GetCompositeSQLFieldName(filter.first);
			if (firstWhere) {
				firstWhere = false;
			}
		}

		IPreparedStatement* statement = db_query->PrepareStatement(sqlQuery);

		if (statement == nullptr)
			return retTable;

		int position = 1;

		IMetaObjectAttribute::SetValueAttribute(m_metaObject->GetRegisterActive(), true, statement, position);
		IMetaObjectAttribute::SetValueAttribute(m_metaObject->GetRegisterPeriod(), cPeriod.GetDate(), statement, position);

		for (auto filter : selFilter) {
			IMetaObjectAttribute::SetValueAttribute(filter.first, filter.second, statement, position);
		}

		IDatabaseResultSet* resultSet = statement->RunQueryWithResults();

		if (resultSet == nullptr)
			return retTable;

		if (resultSet->Next()) {
			for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
				CValue retValue;
				if (IMetaObjectAttribute::GetValueAttribute(object, retValue, resultSet))
					retTable->SetAt(object->GetName(), retValue);
			}
		}

		db_query->CloseResultSet(resultSet);
		db_query->CloseStatement(statement);
	}

	return retTable;
}

CValue CManagerDataObjectInformationRegister::SliceFirst(const CValue& cPeriod, const CValue& cFilter)
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	CValueTable* retTable = CValue::CreateAndPrepareValueRef<CValueTable>();
	CValueTable::IValueModelColumnCollection* colCollection = retTable->GetColumnCollection();
	wxASSERT(colCollection);
	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		CValueTable::IValueModelColumnCollection::IValueModelColumnInfo* colInfo =
			colCollection->AddColumn(
				object->GetName(),
				object->GetTypeDesc(),
				object->GetSynonym()
			);
		colInfo->SetColumnID(object->GetMetaID());
	}

	if (m_metaObject->GetPeriodicity() != ePeriodicity::eNonPeriodic ||
		m_metaObject->GetWriteRegisterMode() == eWriteRegisterMode::eSubordinateRecorder) {
		CValueStructure* valFilter = nullptr; std::map<IMetaObjectAttribute*, CValue> selFilter;
		if (cFilter.ConvertToValue(valFilter)) {
			for (const auto object : m_metaObject->GetDimentionArrayObject()) {
				CValue vSelValue;
				if (valFilter->Property(object->GetName(), vSelValue)) {
					selFilter.insert_or_assign(
						object, vSelValue
					);
				}
			}
		}

		IMetaObjectAttribute::sqlField_t sqlCol =
			IMetaObjectAttribute::GetSQLFieldData(m_metaObject->GetRegisterPeriod());

		wxString sqlQuery = " SELECT * FROM ( SELECT T2." + sqlCol.m_fieldTypeName;

		for (auto dataType : sqlCol.m_types) {
			if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
				sqlQuery += ", T2." + dataType.m_field.m_fieldName;
			}
			else {
				sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
				sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
			}
		}

		for (const auto object : m_metaObject->GetDimentionArrayObject()) {
			IMetaObjectAttribute::sqlField_t sqlDim = IMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += ", T2." + sqlDim.m_fieldTypeName;
			for (auto dataType : sqlDim.m_types) {
				if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += ", T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		for (const auto object : m_metaObject->GetResourceArrayObject()) {
			IMetaObjectAttribute::sqlField_t sqlRes = IMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " , T2." + sqlRes.m_fieldTypeName;
			for (auto dataType : sqlRes.m_types) {
				if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += ", T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		sqlQuery += " FROM (SELECT ";
		sqlQuery += IMetaObjectAttribute::GetSQLFieldName(m_metaObject->GetRegisterPeriod(), "MIN");
		for (const auto object : m_metaObject->GetDimentionArrayObject()) {
			sqlQuery += "," + IMetaObjectAttribute::GetSQLFieldName(object);
		}
		for (const auto object : m_metaObject->GetResourceArrayObject()) {
			sqlQuery += "," + IMetaObjectAttribute::GetSQLFieldName(object);
		}
		sqlQuery += " FROM " + m_metaObject->GetTableNameDB();
		sqlQuery += " WHERE " + IMetaObjectAttribute::GetCompositeSQLFieldName(m_metaObject->GetRegisterActive());
		sqlQuery += " AND " + IMetaObjectAttribute::GetCompositeSQLFieldName(m_metaObject->GetRegisterPeriod(), ">=");
		sqlQuery += " GROUP BY ";
		sqlQuery += sqlCol.m_fieldTypeName;
		for (const auto object : m_metaObject->GetDimentionArrayObject()) {
			sqlQuery += "," + IMetaObjectAttribute::GetSQLFieldName(object);
		}
		for (const auto object : m_metaObject->GetResourceArrayObject()) {
			sqlQuery += "," + IMetaObjectAttribute::GetSQLFieldName(object);
		}
		sqlQuery += ") AS T1 "
			"INNER JOIN " + m_metaObject->GetTableNameDB() + " AS T2 "
			" ON ";

		sqlQuery += " T1." + sqlCol.m_fieldTypeName + " = T2." + sqlCol.m_fieldTypeName;

		for (auto dataType : sqlCol.m_types) {
			if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
				sqlQuery += " AND T1." + dataType.m_field.m_fieldName + " = T2." + dataType.m_field.m_fieldName;
			}
			else {
				sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefType + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
				sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefName + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
			}
		}

		for (const auto object : m_metaObject->GetDimentionArrayObject()) {
			IMetaObjectAttribute::sqlField_t sqlDim = IMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " AND T1." + sqlDim.m_fieldTypeName + " = T2." + sqlDim.m_fieldTypeName;
			for (auto dataType : sqlDim.m_types) {
				if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldName + " = T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefType + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefName + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		for (const auto object : m_metaObject->GetResourceArrayObject()) {
			IMetaObjectAttribute::sqlField_t sqlRes = IMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " AND T1." + sqlRes.m_fieldTypeName + " = T2." + sqlRes.m_fieldTypeName;
			for (auto dataType : sqlRes.m_types) {
				if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldName + " = T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefType + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefName + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		sqlQuery += ") AS lastTable"; bool firstWhere = true;

		for (auto filter : selFilter) {
			if (firstWhere) {
				sqlQuery = sqlQuery + " WHERE ";
			}
			sqlQuery = sqlQuery +
				(firstWhere ? " " : " AND ") + IMetaObjectAttribute::GetCompositeSQLFieldName(filter.first);
			if (firstWhere) {
				firstWhere = false;
			}
		}

		IPreparedStatement* statement = db_query->PrepareStatement(sqlQuery);

		if (statement == nullptr)
			return retTable;

		int position = 1;

		IMetaObjectAttribute::SetValueAttribute(m_metaObject->GetRegisterActive(), true, statement, position);
		IMetaObjectAttribute::SetValueAttribute(m_metaObject->GetRegisterPeriod(), cPeriod.GetDate(), statement, position);

		for (auto filter : selFilter) {
			IMetaObjectAttribute::SetValueAttribute(filter.first, filter.second, statement, position);
		}

		IDatabaseResultSet* resultSet = statement->RunQueryWithResults();

		if (resultSet == nullptr)
			return retTable;

		while (resultSet->Next()) {
			CValueTable::CValueTableReturnLine* retLine = retTable->GetRowAt(retTable->AppendRow());
			wxASSERT(retLine);
			for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
				CValue retValue;
				if (IMetaObjectAttribute::GetValueAttribute(object, retValue, resultSet))
					retLine->SetValueByMetaID(object->GetMetaID(), retValue);
			}
			wxDELETE(retLine);
		}

		db_query->CloseResultSet(resultSet);
		db_query->CloseStatement(statement);
	}

	return retTable;
}

CValue CManagerDataObjectInformationRegister::SliceLast(const CValue& cPeriod, const CValue& cFilter)
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	CValueTable* retTable = CValue::CreateAndPrepareValueRef<CValueTable>();
	CValueTable::IValueModelColumnCollection* colCollection = retTable->GetColumnCollection();
	wxASSERT(colCollection);
	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		CValueTable::IValueModelColumnCollection::IValueModelColumnInfo* colInfo =
			colCollection->AddColumn(
				object->GetName(),
				object->GetTypeDesc(),
				object->GetSynonym()
			);
		colInfo->SetColumnID(object->GetMetaID());
	}

	if (m_metaObject->GetPeriodicity() != ePeriodicity::eNonPeriodic ||
		m_metaObject->GetWriteRegisterMode() == eWriteRegisterMode::eSubordinateRecorder) {
		CValueStructure* valFilter = nullptr; std::map<IMetaObjectAttribute*, CValue> selFilter;
		if (cFilter.ConvertToValue(valFilter)) {
			for (const auto object : m_metaObject->GetDimentionArrayObject()) {
				CValue vSelValue;
				if (valFilter->Property(object->GetName(), vSelValue)) {
					selFilter.insert_or_assign(
						object, vSelValue
					);
				}
			}
		}

		IMetaObjectAttribute::sqlField_t sqlCol =
			IMetaObjectAttribute::GetSQLFieldData(m_metaObject->GetRegisterPeriod());

		wxString sqlQuery = " SELECT * FROM ( SELECT T2." + sqlCol.m_fieldTypeName;

		for (auto dataType : sqlCol.m_types) {
			if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
				sqlQuery += ", T2." + dataType.m_field.m_fieldName;
			}
			else {
				sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
				sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
			}
		}

		for (const auto object : m_metaObject->GetDimentionArrayObject()) {
			IMetaObjectAttribute::sqlField_t sqlDim = IMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += ", T2." + sqlDim.m_fieldTypeName;
			for (auto dataType : sqlDim.m_types) {
				if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += ", T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		for (const auto object : m_metaObject->GetResourceArrayObject()) {
			IMetaObjectAttribute::sqlField_t sqlRes = IMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " , T2." + sqlRes.m_fieldTypeName;
			for (auto dataType : sqlRes.m_types) {
				if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += ", T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		sqlQuery += " FROM (SELECT ";
		sqlQuery += IMetaObjectAttribute::GetSQLFieldName(m_metaObject->GetRegisterPeriod(), "MAX");
		for (const auto object : m_metaObject->GetDimentionArrayObject()) {
			sqlQuery += "," + IMetaObjectAttribute::GetSQLFieldName(object);
		}
		for (const auto object : m_metaObject->GetResourceArrayObject()) {
			sqlQuery += "," + IMetaObjectAttribute::GetSQLFieldName(object);
		}
		sqlQuery += " FROM " + m_metaObject->GetTableNameDB();
		sqlQuery += " WHERE " + IMetaObjectAttribute::GetCompositeSQLFieldName(m_metaObject->GetRegisterActive());
		sqlQuery += " AND " + IMetaObjectAttribute::GetCompositeSQLFieldName(m_metaObject->GetRegisterPeriod(), "<=");
		sqlQuery += " GROUP BY ";
		sqlQuery += sqlCol.m_fieldTypeName;
		for (const auto object : m_metaObject->GetDimentionArrayObject()) {
			sqlQuery += "," + IMetaObjectAttribute::GetSQLFieldName(object);
		}
		for (const auto object : m_metaObject->GetResourceArrayObject()) {
			sqlQuery += "," + IMetaObjectAttribute::GetSQLFieldName(object);
		}
		sqlQuery += ") AS T1 "
			"INNER JOIN " + m_metaObject->GetTableNameDB() + " AS T2 "
			" ON ";

		sqlQuery += " T1." + sqlCol.m_fieldTypeName + " = T2." + sqlCol.m_fieldTypeName;

		for (auto dataType : sqlCol.m_types) {
			if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
				sqlQuery += " AND T1." + dataType.m_field.m_fieldName + " = T2." + dataType.m_field.m_fieldName;
			}
			else {
				sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefType + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
				sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefName + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
			}
		}

		for (const auto object : m_metaObject->GetDimentionArrayObject()) {
			IMetaObjectAttribute::sqlField_t sqlDim = IMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " AND T1." + sqlDim.m_fieldTypeName + " = T2." + sqlDim.m_fieldTypeName;
			for (auto dataType : sqlDim.m_types) {
				if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldName + " = T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefType + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefName + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		for (const auto object : m_metaObject->GetResourceArrayObject()) {
			IMetaObjectAttribute::sqlField_t sqlRes = IMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " AND T1." + sqlRes.m_fieldTypeName + " = T2." + sqlRes.m_fieldTypeName;
			for (auto dataType : sqlRes.m_types) {
				if (dataType.m_type != IMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldName + " = T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefType + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefName + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		sqlQuery += ") AS lastTable"; bool firstWhere = true;

		for (auto filter : selFilter) {
			if (firstWhere) {
				sqlQuery = sqlQuery + " WHERE ";
			}
			sqlQuery = sqlQuery +
				(firstWhere ? " " : " AND ") + IMetaObjectAttribute::GetCompositeSQLFieldName(filter.first);
			if (firstWhere) {
				firstWhere = false;
			}
		}

		IPreparedStatement* statement = db_query->PrepareStatement(sqlQuery);

		if (statement == nullptr)
			return retTable;

		int position = 1;

		IMetaObjectAttribute::SetValueAttribute(m_metaObject->GetRegisterActive(), true, statement, position);
		IMetaObjectAttribute::SetValueAttribute(m_metaObject->GetRegisterPeriod(), cPeriod.GetDate(), statement, position);

		for (auto filter : selFilter) {
			IMetaObjectAttribute::SetValueAttribute(filter.first, filter.second, statement, position);
		}

		IDatabaseResultSet* resultSet = statement->RunQueryWithResults();

		if (resultSet == nullptr)
			return retTable;

		while (resultSet->Next()) {
			CValueTable::CValueTableReturnLine* retLine = retTable->GetRowAt(retTable->AppendRow());
			wxASSERT(retLine);
			for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
				CValue retValue;
				if (IMetaObjectAttribute::GetValueAttribute(object, retValue, resultSet))
					retTable->SetAt(object->GetName(), retValue);
			}
			wxDELETE(retLine);
		}

		db_query->CloseResultSet(resultSet);
		db_query->CloseStatement(statement);
	}

	return retTable;
}
