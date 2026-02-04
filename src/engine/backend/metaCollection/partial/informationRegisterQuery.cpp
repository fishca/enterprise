#include "informationRegister.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/databaseLayer/databaseErrorCodes.h"
#include "backend/appData.h"

bool CValueMetaObjectInformationRegister::CreateAndUpdateSliceFirstTableDB(IMetaDataConfiguration* srcMetaData, IValueMetaObject* srcMetaObject, int flags)
{
	wxString tableName = GetTableNameDB();
	wxString viewName = GetTableNameDB() + wxT("SF");

	int retCode = DATABASE_LAYER_QUERY_RESULT_ERROR;

	if ((flags & createMetaTable) != 0) {

		IValueMetaObjectAttribute::sqlField_t sqlCol =
			IValueMetaObjectAttribute::GetSQLFieldData(m_propertyAttributePeriod->GetMetaObject());

		wxString sqlViewColumn =
			IValueMetaObjectAttribute::GetSQLFieldName(m_propertyAttributePeriod->GetMetaObject());
		for (const auto object : GetDimentionArrayObject()) {
			sqlViewColumn += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}
		for (const auto object : GetResourceArrayObject()) {
			sqlViewColumn += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}

		wxString sqlQuery = "CREATE VIEW %s (" + sqlViewColumn + ") AS";
		sqlQuery += " SELECT T2." + sqlCol.m_fieldTypeName;

		for (auto dataType : sqlCol.m_types) {
			if (dataType.m_type != IValueMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
				sqlQuery += ", T2." + dataType.m_field.m_fieldName;
			}
			else {
				sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
				sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
			}
		}

		for (const auto object : GetDimentionArrayObject()) {
			IValueMetaObjectAttribute::sqlField_t sqlDim = IValueMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += ", T2." + sqlDim.m_fieldTypeName;
			for (auto dataType : sqlDim.m_types) {
				if (dataType.m_type != IValueMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += ", T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		for (const auto object : GetResourceArrayObject()) {
			IValueMetaObjectAttribute::sqlField_t sqlRes = IValueMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " , T2." + sqlRes.m_fieldTypeName;
			for (auto dataType : sqlRes.m_types) {
				if (dataType.m_type != IValueMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += ", T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		sqlQuery += " FROM (SELECT ";
		sqlQuery += IValueMetaObjectAttribute::GetSQLFieldName(m_propertyAttributePeriod->GetMetaObject(), "MIN");
		for (const auto object : GetDimentionArrayObject()) {
			sqlQuery += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}
		for (const auto object : GetResourceArrayObject()) {
			sqlQuery += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}
		sqlQuery += " FROM " + tableName;
		sqlQuery += " GROUP BY ";
		sqlQuery += sqlCol.m_fieldTypeName;
		for (const auto object : GetDimentionArrayObject()) {
			sqlQuery += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}
		for (const auto object : GetResourceArrayObject()) {
			sqlQuery += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}
		sqlQuery += ") AS T1 "
			"INNER JOIN " + tableName + " AS T2 "
			" ON ";
		sqlQuery += " T1." + sqlCol.m_fieldTypeName + " = T2." + sqlCol.m_fieldTypeName;
		for (const auto object : GetDimentionArrayObject()) {
			IValueMetaObjectAttribute::sqlField_t sqlDim = IValueMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " AND T1." + sqlDim.m_fieldTypeName + " = T2." + sqlDim.m_fieldTypeName;
			for (auto dataType : sqlDim.m_types) {
				if (dataType.m_type != IValueMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldName + " = T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefType + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefName + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		for (const auto object : GetResourceArrayObject()) {
			IValueMetaObjectAttribute::sqlField_t sqlRes = IValueMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " AND T1." + sqlRes.m_fieldTypeName + " = T2." + sqlRes.m_fieldTypeName;
			for (auto dataType : sqlRes.m_types) {
				if (dataType.m_type != IValueMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldName + " = T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefType + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefName + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		retCode = db_query->RunQuery(sqlQuery, viewName, tableName);
	}
	else if ((flags & updateMetaTable) != 0) {

		IValueMetaObjectAttribute::sqlField_t sqlCol =
			IValueMetaObjectAttribute::GetSQLFieldData(m_propertyAttributePeriod->GetMetaObject());

		wxString sqlViewColumn =
			IValueMetaObjectAttribute::GetSQLFieldName(m_propertyAttributePeriod->GetMetaObject());
		for (const auto object : GetDimentionArrayObject()) {
			sqlViewColumn += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}
		for (const auto object : GetResourceArrayObject()) {
			sqlViewColumn += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}

		wxString sqlQuery = "CREATE OR ALTER VIEW %s (" + sqlViewColumn + ") AS";
		sqlQuery += " SELECT T2." + sqlCol.m_fieldTypeName;

		for (auto dataType : sqlCol.m_types) {
			if (dataType.m_type != IValueMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
				sqlQuery += ", T2." + dataType.m_field.m_fieldName;
			}
			else {
				sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
				sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
			}
		}

		for (const auto object : GetDimentionArrayObject()) {
			IValueMetaObjectAttribute::sqlField_t sqlDim = IValueMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += ", T2." + sqlDim.m_fieldTypeName;
			for (auto dataType : sqlDim.m_types) {
				if (dataType.m_type != IValueMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += ", T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		for (const auto object : GetResourceArrayObject()) {
			IValueMetaObjectAttribute::sqlField_t sqlRes = IValueMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " , T2." + sqlRes.m_fieldTypeName;
			for (auto dataType : sqlRes.m_types) {
				if (dataType.m_type != IValueMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += ", T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		sqlQuery += " FROM (SELECT ";
		sqlQuery += IValueMetaObjectAttribute::GetSQLFieldName(m_propertyAttributePeriod->GetMetaObject(), "MAX");
		for (const auto object : GetDimentionArrayObject()) {
			sqlQuery += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}
		for (const auto object : GetResourceArrayObject()) {
			sqlQuery += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}
		sqlQuery += " FROM " + tableName;
		sqlQuery += " GROUP BY ";
		sqlQuery += sqlCol.m_fieldTypeName;
		for (const auto object : GetDimentionArrayObject()) {
			sqlQuery += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}
		for (const auto object : GetResourceArrayObject()) {
			sqlQuery += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}
		sqlQuery += ") AS T1 "
			"INNER JOIN " + tableName + " AS T2 "
			" ON ";
		sqlQuery += " T1." + sqlCol.m_fieldTypeName + " = T2." + sqlCol.m_fieldTypeName;
		for (const auto object : GetDimentionArrayObject()) {
			IValueMetaObjectAttribute::sqlField_t sqlDim = IValueMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " AND T1." + sqlDim.m_fieldTypeName + " = T2." + sqlDim.m_fieldTypeName;
			for (auto dataType : sqlDim.m_types) {
				if (dataType.m_type != IValueMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldName + " = T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefType + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefName + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		for (const auto object : GetResourceArrayObject()) {
			IValueMetaObjectAttribute::sqlField_t sqlRes = IValueMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " AND T1." + sqlRes.m_fieldTypeName + " = T2." + sqlRes.m_fieldTypeName;
			for (auto dataType : sqlRes.m_types) {
				if (dataType.m_type != IValueMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldName + " = T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefType + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefName + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		retCode = db_query->RunQuery(sqlQuery, viewName, tableName);
	}
	else if ((flags & deleteMetaTable) != 0) {
		retCode = db_query->RunQuery("DROP VIEW %s", viewName);
	}

	if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
		return false;

	return true;
}

bool CValueMetaObjectInformationRegister::CreateAndUpdateSliceLastTableDB(IMetaDataConfiguration* srcMetaData, IValueMetaObject* srcMetaObject, int flags)
{
	wxString tableName = GetTableNameDB();
	wxString viewName = GetTableNameDB() + wxT("SL");

	int retCode = DATABASE_LAYER_QUERY_RESULT_ERROR;

	if ((flags & createMetaTable) != 0) {

		IValueMetaObjectAttribute::sqlField_t sqlCol =
			IValueMetaObjectAttribute::GetSQLFieldData(m_propertyAttributePeriod->GetMetaObject());

		wxString sqlViewColumn =
			IValueMetaObjectAttribute::GetSQLFieldName(m_propertyAttributePeriod->GetMetaObject());
		for (const auto object : GetDimentionArrayObject()) {
			sqlViewColumn += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}
		for (const auto object : GetResourceArrayObject()) {
			sqlViewColumn += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}

		wxString sqlQuery = "CREATE VIEW %s (" + sqlViewColumn + ") AS";
		sqlQuery += " SELECT T2." + sqlCol.m_fieldTypeName;

		for (auto dataType : sqlCol.m_types) {
			if (dataType.m_type != IValueMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
				sqlQuery += ", T2." + dataType.m_field.m_fieldName;
			}
			else {
				sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
				sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
			}
		}

		for (const auto object : GetDimentionArrayObject()) {
			IValueMetaObjectAttribute::sqlField_t sqlDim = IValueMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += ", T2." + sqlDim.m_fieldTypeName;
			for (auto dataType : sqlDim.m_types) {
				if (dataType.m_type != IValueMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += ", T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		for (const auto object : GetResourceArrayObject()) {
			IValueMetaObjectAttribute::sqlField_t sqlRes = IValueMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " , T2." + sqlRes.m_fieldTypeName;
			for (auto dataType : sqlRes.m_types) {
				if (dataType.m_type != IValueMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += ", T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		sqlQuery += " FROM (SELECT ";
		sqlQuery += IValueMetaObjectAttribute::GetSQLFieldName(m_propertyAttributePeriod->GetMetaObject(), "MAX");
		for (const auto object : GetDimentionArrayObject()) {
			sqlQuery += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}
		for (const auto object : GetResourceArrayObject()) {
			sqlQuery += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}
		sqlQuery += " FROM " + tableName;
		sqlQuery += " GROUP BY ";
		sqlQuery += sqlCol.m_fieldTypeName;
		for (const auto object : GetDimentionArrayObject()) {
			sqlQuery += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}
		for (const auto object : GetResourceArrayObject()) {
			sqlQuery += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}
		sqlQuery += ") AS T1 "
			"INNER JOIN " + tableName + " AS T2 "
			" ON ";
		sqlQuery += " T1." + sqlCol.m_fieldTypeName + " = T2." + sqlCol.m_fieldTypeName;
		for (const auto object : GetDimentionArrayObject()) {
			IValueMetaObjectAttribute::sqlField_t sqlDim = IValueMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " AND T1." + sqlDim.m_fieldTypeName + " = T2." + sqlDim.m_fieldTypeName;
			for (auto dataType : sqlDim.m_types) {
				if (dataType.m_type != IValueMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldName + " = T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefType + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefName + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		for (const auto object : GetResourceArrayObject()) {
			IValueMetaObjectAttribute::sqlField_t sqlRes = IValueMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " AND T1." + sqlRes.m_fieldTypeName + " = T2." + sqlRes.m_fieldTypeName;
			for (auto dataType : sqlRes.m_types) {
				if (dataType.m_type != IValueMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldName + " = T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefType + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefName + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		retCode = db_query->RunQuery(sqlQuery, viewName, tableName);
	}
	else if ((flags & updateMetaTable) != 0) {

		IValueMetaObjectAttribute::sqlField_t sqlCol =
			IValueMetaObjectAttribute::GetSQLFieldData(m_propertyAttributePeriod->GetMetaObject());

		wxString sqlViewColumn =
			IValueMetaObjectAttribute::GetSQLFieldName(m_propertyAttributePeriod->GetMetaObject());
		for (const auto object : GetDimentionArrayObject()) {
			sqlViewColumn += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}
		for (const auto object : GetResourceArrayObject()) {
			sqlViewColumn += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}

		wxString sqlQuery = "CREATE OR ALTER VIEW %s (" + sqlViewColumn + ") AS";
		sqlQuery += " SELECT T2." + sqlCol.m_fieldTypeName;

		for (auto dataType : sqlCol.m_types) {
			if (dataType.m_type != IValueMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
				sqlQuery += ", T2." + dataType.m_field.m_fieldName;
			}
			else {
				sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
				sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
			}
		}

		for (const auto object : GetDimentionArrayObject()) {
			IValueMetaObjectAttribute::sqlField_t sqlDim = IValueMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += ", T2." + sqlDim.m_fieldTypeName;
			for (auto dataType : sqlDim.m_types) {
				if (dataType.m_type != IValueMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += ", T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		for (const auto object : GetResourceArrayObject()) {
			IValueMetaObjectAttribute::sqlField_t sqlRes = IValueMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " , T2." + sqlRes.m_fieldTypeName;
			for (auto dataType : sqlRes.m_types) {
				if (dataType.m_type != IValueMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += ", T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += ", T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		sqlQuery += " FROM (SELECT ";
		sqlQuery += IValueMetaObjectAttribute::GetSQLFieldName(m_propertyAttributePeriod->GetMetaObject(), "MAX");
		for (const auto object : GetDimentionArrayObject()) {
			sqlQuery += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}
		for (const auto object : GetResourceArrayObject()) {
			sqlQuery += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}
		sqlQuery += " FROM " + tableName;
		sqlQuery += " GROUP BY ";
		sqlQuery += sqlCol.m_fieldTypeName;
		for (const auto object : GetDimentionArrayObject()) {
			sqlQuery += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}
		for (const auto object : GetResourceArrayObject()) {
			sqlQuery += "," + IValueMetaObjectAttribute::GetSQLFieldName(object);
		}
		sqlQuery += ") AS T1 "
			"INNER JOIN " + tableName + " AS T2 "
			" ON ";
		sqlQuery += " T1." + sqlCol.m_fieldTypeName + " = T2." + sqlCol.m_fieldTypeName;
		for (const auto object : GetDimentionArrayObject()) {
			IValueMetaObjectAttribute::sqlField_t sqlDim = IValueMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " AND T1." + sqlDim.m_fieldTypeName + " = T2." + sqlDim.m_fieldTypeName;
			for (auto dataType : sqlDim.m_types) {
				if (dataType.m_type != IValueMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldName + " = T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefType + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefName + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		for (const auto object : GetResourceArrayObject()) {
			IValueMetaObjectAttribute::sqlField_t sqlRes = IValueMetaObjectAttribute::GetSQLFieldData(object);
			sqlQuery += " AND T1." + sqlRes.m_fieldTypeName + " = T2." + sqlRes.m_fieldTypeName;
			for (auto dataType : sqlRes.m_types) {
				if (dataType.m_type != IValueMetaObjectAttribute::eFieldTypes::eFieldTypes_Reference) {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldName + " = T2." + dataType.m_field.m_fieldName;
				}
				else {
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefType + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefType;
					sqlQuery += " AND T1." + dataType.m_field.m_fieldRefName.m_fieldRefName + " = T2." + dataType.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
		}

		retCode = db_query->RunQuery(sqlQuery, viewName, tableName);
	}
	else if ((flags & deleteMetaTable) != 0) {
		retCode = db_query->RunQuery("DROP VIEW %s", viewName);
	}

	if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
		return false;

	return true;
}

bool CValueMetaObjectInformationRegister::CreateAndUpdateTableDB(IMetaDataConfiguration* srcMetaData, IValueMetaObject* srcMetaObject, int flags)
{
	//if (!CValueMetaObjectInformationRegister::CreateAndUpdateSliceFirstTableDB(srcMetaData, srcMetaObject, flags))
	//	return false;

	//if (!CValueMetaObjectInformationRegister::CreateAndUpdateSliceLastTableDB(srcMetaData, srcMetaObject, flags))
	//	return false;

	return IValueMetaObjectRegisterData::CreateAndUpdateTableDB(
		srcMetaData,
		srcMetaObject,
		flags
	);
}