#include "accumulationRegister.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/databaseLayer/databaseErrorCodes.h"
#include "backend/appData.h"

////////////////////////////////////////////////////////////////////////////////

bool CValueMetaObjectAccumulationRegister::CreateAndUpdateBalancesTableDB(IMetaDataConfiguration* srcMetaData, IValueMetaObject* srcMetaObject, int flags)
{
	wxString tableName = GetRegisterTableNameDB(eRegisterType::eBalances);

	int retCode = DATABASE_LAYER_QUERY_RESULT_ERROR;

	if ((flags & createMetaTable) != 0) {
		retCode = db_query->RunQuery("CREATE TABLE %s;", tableName);
		if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
			return false;

		retCode = ProcessAttribute(tableName,
			m_propertyAttributePeriod->GetMetaObject(), nullptr);

		for (const auto object : GetDimentionArrayObject()) {
			if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
				return false;
			retCode = ProcessDimension(tableName,
				object, nullptr);
		}

		if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
			return false;

		for (const auto object : GetResourceArrayObject()) {
			if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
				return false;
			retCode = ProcessResource(tableName,
				object, nullptr);
		}
	}
	else if ((flags & updateMetaTable) != 0) {

		//if src is null then delete
		IValueMetaObjectRegisterData* dstValue = nullptr;
		if (srcMetaObject->ConvertToValue(dstValue)) {
			if (!UpdateCurrentRecords(tableName, dstValue))
				return false;
			//dimensions from dst 
			for (const auto object : dstValue->GetDimentionArrayObject()) {
				IValueMetaObject* foundedMeta =
					IValueMetaObjectRegisterData::FindDimensionObjectByFilter(object->GetGuid());
				if (foundedMeta == nullptr) {
					retCode = ProcessDimension(tableName, nullptr, object);
					if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
						return false;
				}
			}

			//dimensions current
			for (const auto object : GetDimentionArrayObject()) {
				retCode = ProcessDimension(tableName,
					object, dstValue->FindDimensionObjectByFilter(object->GetGuid())
				);
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return false;
			}

			//resources from dst 
			for (const auto object : dstValue->GetResourceArrayObject()) {
				IValueMetaObject* foundedMeta =
					IValueMetaObjectRegisterData::FindResourceObjectByFilter(object->GetGuid());
				if (foundedMeta == nullptr) {
					retCode = ProcessResource(tableName, nullptr, object);
					if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
						return false;
				}
			}

			//resources current
			for (const auto object : GetResourceArrayObject()) {
				retCode = ProcessResource(tableName,
					object, dstValue->FindResourceObjectByFilter(object->GetGuid())
				);
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return false;
			}
		}
	}
	else if ((flags & deleteMetaTable) != 0) {
		retCode = db_query->RunQuery("DROP TABLE %s", tableName);
	}

	if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
		return false;

	return true;
}

bool CValueMetaObjectAccumulationRegister::CreateAndUpdateTurnoverTableDB(IMetaDataConfiguration* srcMetaData, IValueMetaObject* srcMetaObject, int flags)
{
	wxString tableName = GetRegisterTableNameDB(eRegisterType::eTurnovers);

	int retCode = DATABASE_LAYER_QUERY_RESULT_ERROR;

	if ((flags & createMetaTable) != 0) {
		
		retCode = db_query->RunQuery("CREATE TABLE %s;", tableName);
	
		if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
			return false;

		retCode = ProcessAttribute(tableName,
			m_propertyAttributePeriod->GetMetaObject(), nullptr);

		for (const auto object : GetDimentionArrayObject()) {
			if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
				return false;
			retCode = ProcessDimension(tableName,
				object, nullptr);
		}

		if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
			return false;

		for (const auto object : GetResourceArrayObject()) {
			if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
				return false;
			retCode = ProcessResource(tableName,
				object, nullptr);
		}
	}
	else if ((flags & updateMetaTable) != 0) {

		//if src is null then delete
		IValueMetaObjectRegisterData* dstValue = nullptr;
		if (srcMetaObject->ConvertToValue(dstValue)) {
			if (!UpdateCurrentRecords(tableName, dstValue))
				return false;
			//dimensions from dst 
			for (const auto object : dstValue->GetDimentionArrayObject()) {
				IValueMetaObject* foundedMeta =
					IValueMetaObjectRegisterData::FindDimensionObjectByFilter(object->GetGuid());
				if (foundedMeta == nullptr) {
					retCode = ProcessDimension(tableName, nullptr, object);
					if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
						return false;
				}
			}

			//dimensions current
			for (const auto object : GetDimentionArrayObject()) {
				retCode = ProcessDimension(tableName,
					object, dstValue->FindDimensionObjectByFilter(object->GetGuid())
				);
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return false;
			}

			//resources from dst 
			for (const auto object : dstValue->GetResourceArrayObject()) {
				IValueMetaObject* foundedMeta =
					IValueMetaObjectRegisterData::FindResourceObjectByFilter(object->GetGuid());
				if (foundedMeta == nullptr) {
					retCode = ProcessResource(tableName, nullptr, object);
					if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
						return false;
				}
			}

			//resources current
			for (const auto object : GetResourceArrayObject()) {
				retCode = ProcessResource(tableName,
					object, dstValue->FindResourceObjectByFilter(object->GetGuid())
				);
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return false;
			}
		}
	}
	else if ((flags & deleteMetaTable) != 0) {
		retCode = db_query->RunQuery("DROP TABLE %s", tableName);
	}

	if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
		return false;

	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool CValueMetaObjectAccumulationRegister::CreateAndUpdateTableDB(IMetaDataConfiguration* srcMetaData, IValueMetaObject* srcMetaObject, int flags)
{
	CValueMetaObjectAccumulationRegister* dstValue = nullptr; int tableBalancesFlags = flags, tableTurnoverFlags = flags;

	if (srcMetaObject != nullptr &&
		srcMetaObject->ConvertToValue(dstValue)) {
		eRegisterType dstRegType = dstValue->GetRegisterType();
		if (dstRegType != GetRegisterType()
			&& GetRegisterType() == eRegisterType::eBalances) {
			tableBalancesFlags = createMetaTable;
			tableTurnoverFlags = deleteMetaTable;
		}
		else if (dstRegType != GetRegisterType()
			&& GetRegisterType() == eRegisterType::eTurnovers) {
			tableBalancesFlags = deleteMetaTable;
			tableTurnoverFlags = createMetaTable;
		}
		else if (GetRegisterType() == eRegisterType::eTurnovers) {
			tableBalancesFlags = defaultFlag;
		}
		else if (GetRegisterType() == eRegisterType::eBalances) {
			tableTurnoverFlags = defaultFlag;
		}
	}
	else if (srcMetaObject == nullptr
		&& flags == createMetaTable) {
		if (GetRegisterType() == eRegisterType::eBalances) {
			tableBalancesFlags = createMetaTable;
			tableTurnoverFlags = defaultFlag;
		}
		else {
			tableBalancesFlags = defaultFlag;
			tableTurnoverFlags = createMetaTable;
		}
	}
	else if (srcMetaObject == nullptr
		&& flags == deleteMetaTable) {
		if (GetRegisterType() == eRegisterType::eBalances) {
			tableBalancesFlags = deleteMetaTable;
			tableTurnoverFlags = defaultFlag;
		}
		else {
			tableBalancesFlags = defaultFlag;
			tableTurnoverFlags = deleteMetaTable;
		}
	}

	//if (tableBalancesFlags != defaultFlag
	//	&& !CValueMetaObjectAccumulationRegister::CreateAndUpdateBalancesTableDB(srcMetaData, srcMetaObject, tableBalancesFlags))
	//	return false;

	//if (tableTurnoverFlags != defaultFlag
	//	&& !CValueMetaObjectAccumulationRegister::CreateAndUpdateTurnoverTableDB(srcMetaData, srcMetaObject, tableTurnoverFlags))
	//	return false;

	return IValueMetaObjectRegisterData::CreateAndUpdateTableDB(
		srcMetaData,
		srcMetaObject,
		flags
	);
}

////////////////////////////////////////////////////////////////////////////////

bool CValueRecordSetObjectAccumulationRegister::SaveVirtualTable()
{
	/*CValueMetaObjectAccumulationRegister* metaObject = nullptr;

	if (!m_metaObject->ConvertToValue(metaObject))
		return false;

	CValueMetaObjectAttributePredefined* attributePeriod = metaObject->GetRegisterPeriod();
	wxASSERT(attributePeriod);

	wxString tableName = metaObject->GetRegisterTableNameDB(); bool firstUpdate = true;
	wxString queryText = "UPDATE OR INSERT INTO " + tableName + "(" + IValueMetaObjectAttribute::GetSQLFieldName(attributePeriod);

	for (const auto object : metaObject->GetDimentionArrayObject()) {
		queryText += "," + IValueMetaObjectAttribute::GetSQLFieldName(attribute);
	}

	queryText += ") VALUES ("; bool firstInsert = true;

	unsigned int fieldCount = IValueMetaObjectAttribute::GetSQLFieldCount(attributePeriod);
	for (unsigned int i = 0; i < fieldCount; i++) {
		queryText += (firstInsert ? "?" : ",?");
		if (firstInsert) {
			firstInsert = false;
		}
	}

	for (const auto object : metaObject->GetDimentionArrayObject()) {
		for (unsigned int i = 0; i < IValueMetaObjectAttribute::GetSQLFieldCount(attribute); i++) {
			queryText += ",?";
		}
	}

	queryText += ") MATCHING ( " + IValueMetaObjectAttribute::GetSQLFieldName(attributePeriod);

	for (const auto object : metaObject->GetDimentionArrayObject()) {
		queryText += "," + IValueMetaObjectAttribute::GetSQLFieldName(attribute);
	}

	queryText += ");";

	IPreparedStatement* statement = db_query->PrepareStatement(queryText);

	if (statement == nullptr)
		return false;

	bool hasError = false;

	for (auto objectValue : m_listObjectValue) {

		if (hasError)
			break;

		int position = 1;

		IValueMetaObjectAttribute::SetValueAttribute(
			attributePeriod,
			objectValue.at(attributePeriod->GetMetaID()),
			statement,
			position
		);

		for (const auto object : metaObject->GetDimentionArrayObject()) {
			auto foundedKey = m_keyValues.find(attribute->GetMetaID());
			if (foundedKey != m_keyValues.end()) {
				IValueMetaObjectAttribute::SetValueAttribute(
					attribute,
					foundedKey->second,
					statement,
					position
				);
			}
			else {
				IValueMetaObjectAttribute::SetValueAttribute(
					attribute,
					objectValue.at(attribute->GetMetaID()),
					statement,
					position
				);
			}
		}

		hasError = statement->DoRunQuery() == DATABASE_LAYER_QUERY_RESULT_ERROR;
	}

	db_query->CloseStatement(statement);
	return !hasError;*/
	return true;
}

bool CValueRecordSetObjectAccumulationRegister::DeleteVirtualTable()
{
	/*CValueMetaObjectAccumulationRegister* metaObject = nullptr;

	if (!m_metaObject->ConvertToValue(metaObject))
		return false;

	CValueMetaObjectAttributePredefined* attributePeriod = metaObject->GetRegisterPeriod();
	wxASSERT(attributePeriod);

	wxString tableName = metaObject->GetRegisterTableNameDB();
	wxString queryText = "DELETE FROM " + tableName; bool firstWhere = true;

	for (const auto object : metaObject->GetDimentionArrayObject()) {
		if (!IValueRecordSetObject::FindKeyValue(attribute->GetMetaID()))
			continue;
		if (firstWhere) {
			queryText = queryText + " WHERE ";
		}
		queryText = queryText +
			(firstWhere ? " " : " AND ") + IValueMetaObjectAttribute::GetCompositeSQLFieldName(attribute);
		if (firstWhere) {
			firstWhere = false;
		}
	}

	IPreparedStatement* statement = db_query->PrepareStatement(queryText); int position = 1;

	if (statement == nullptr)
		return false;

	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		if (!IValueRecordSetObject::FindKeyValue(attribute->GetMetaID()))
			continue;
		IValueMetaObjectAttribute::SetValueAttribute(
			attribute,
			m_keyValues.at(attribute->GetMetaID()),
			statement,
			position
		);
	}

	statement->DoRunQuery();
	statement->Close();*/
	return true;
}