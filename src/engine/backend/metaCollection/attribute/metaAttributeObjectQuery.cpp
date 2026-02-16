#include "metaAttributeObject.h"

#include "backend/appData.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/databaseLayer/databaseErrorCodes.h"

#include "backend/metaData.h"

#include "backend/objCtor.h"
#include "backend/metaCollection/partial/reference/reference.h"

wxString IValueMetaObjectAttribute::GetSQLTypeObject(const class_identifier_t& clsid) const
{
	const CTypeDescription& typeDesc = GetTypeDesc();

	switch (CValue::GetVTByID(clsid))
	{
	case eValueTypes::TYPE_BOOLEAN:
		return wxString::Format("SMALLINT");
	case eValueTypes::TYPE_NUMBER:
		if (typeDesc.GetScale() > 0)
			return wxString::Format("NUMERIC(%i,%i)", typeDesc.GetPrecision(), typeDesc.GetScale());
		else
			return wxString::Format("NUMERIC(%i)", typeDesc.GetPrecision());
	case eValueTypes::TYPE_DATE:
		if (typeDesc.GetDateFraction() == eDateFractions::eDateFractions_Date)
			return wxString::Format("DATE");
		else if (typeDesc.GetDateFraction() == eDateFractions::eDateFractions_DateTime)
			return wxString::Format("TIMESTAMP");
		else
			return wxString::Format("TIME");
	case eValueTypes::TYPE_STRING:
		if (typeDesc.GetAllowedLength() == eAllowedLength::eAllowedLength_Variable)
			return wxString::Format("VARCHAR(%i)", typeDesc.GetLength());
		else
			return wxString::Format("CHAR(%i)", typeDesc.GetLength());
	case eValueTypes::TYPE_ENUM:
		return wxString::Format("INTEGER");
	default:
		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
			return wxString::Format("BYTEA");
		return wxString::Format("BLOB");
	}

	return wxEmptyString;
}

unsigned short IValueMetaObjectAttribute::GetSQLFieldCount(const IValueMetaObjectAttribute* metaAttr)
{
	const wxString& fieldName = metaAttr->GetFieldNameDB(); unsigned short sqlField = 1;

	if (metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN)) {
		sqlField += 1;
	}
	if (metaAttr->ContainType(eValueTypes::TYPE_NUMBER)) {
		sqlField += 1;
	}
	if (metaAttr->ContainType(eValueTypes::TYPE_DATE)) {
		sqlField += 1;
	}
	if (metaAttr->ContainType(eValueTypes::TYPE_STRING)) {
		sqlField += 1;
	}
	if (metaAttr->ContainType(eValueTypes::TYPE_ENUM)) {
		sqlField += 1;
	}
	if (metaAttr->ContainMetaType(eCtorMetaType::eCtorMetaType_Reference)) {
		sqlField += 2;
	}

	return sqlField;
}

wxString IValueMetaObjectAttribute::GetSQLFieldName(const IValueMetaObjectAttribute* metaAttr, const wxString& aggr)
{
	const wxString& fieldName = metaAttr->GetFieldNameDB(); wxString sqlField = wxEmptyString;

	if (metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN)) {
		if (!sqlField.IsEmpty())
			sqlField += ",";
		sqlField += fieldName + "_B";
	}
	if (metaAttr->ContainType(eValueTypes::TYPE_NUMBER)) {
		if (!sqlField.IsEmpty())
			sqlField += ",";
		if (aggr.IsEmpty())
			sqlField += fieldName + "_N";
		else
			sqlField += aggr + "(" + fieldName + "_N" + ") AS " + fieldName + "_N";
	}
	if (metaAttr->ContainType(eValueTypes::TYPE_DATE)) {
		if (!sqlField.IsEmpty())
			sqlField += ",";
		if (aggr.IsEmpty())
			sqlField += fieldName + "_D";
		else
			sqlField += aggr + "(" + fieldName + "_D" + ") AS " + fieldName + "_D";
	}
	if (metaAttr->ContainType(eValueTypes::TYPE_STRING)) {
		if (!sqlField.IsEmpty())
			sqlField += ",";
		sqlField += fieldName + "_S";
	}
	if (metaAttr->ContainType(eValueTypes::TYPE_ENUM)) {
		if (!sqlField.IsEmpty())
			sqlField += ",";
		sqlField += fieldName + "_E";
	}
	if (metaAttr->ContainMetaType(eCtorMetaType::eCtorMetaType_Reference)) {
		if (!sqlField.IsEmpty())
			sqlField += ",";
		sqlField += fieldName + "_RTRef" + "," + fieldName + "_RRRef";
	}

	return fieldName + "_TYPE"
		+ (sqlField.Length() > 0 ? "," : "")
		+ sqlField;
}

wxString IValueMetaObjectAttribute::GetCompositeSQLFieldName(const IValueMetaObjectAttribute* metaAttr, const wxString& cmp)
{
	const wxString& fieldName = metaAttr->GetFieldNameDB(); wxString sqlField = wxEmptyString;

	if (metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN)) {
		if (!sqlField.IsEmpty())
			sqlField += " AND ";
		sqlField += fieldName + "_B " + cmp + " ? ";
	}
	if (metaAttr->ContainType(eValueTypes::TYPE_NUMBER)) {
		if (!sqlField.IsEmpty())
			sqlField += " AND ";
		sqlField += fieldName + "_N " + cmp + " ? ";
	}
	if (metaAttr->ContainType(eValueTypes::TYPE_DATE)) {
		if (!sqlField.IsEmpty())
			sqlField += " AND ";
		sqlField += fieldName + "_D " + cmp + " ? ";
	}
	if (metaAttr->ContainType(eValueTypes::TYPE_STRING)) {
		if (!sqlField.IsEmpty())
			sqlField += " AND ";
		sqlField += fieldName + "_S " + cmp + " ? ";
	}
	if (metaAttr->ContainType(eValueTypes::TYPE_ENUM)) {
		if (!sqlField.IsEmpty())
			sqlField += " AND ";
		sqlField += fieldName + "_E " + cmp + " ? ";
	}
	if (metaAttr->ContainMetaType(eCtorMetaType::eCtorMetaType_Reference)) {
		if (!sqlField.IsEmpty())
			sqlField += " AND ";
		sqlField += fieldName + "_RTRef = ? AND " + fieldName + "_RRRef " + cmp + " ? ";
	}

	return fieldName + "_TYPE = ? "
		+ (sqlField.Length() > 0 ? " AND " : "")
		+ sqlField;
}

wxString IValueMetaObjectAttribute::GetExcludeSQLFieldName(const IValueMetaObjectAttribute* metaAttr)
{
	const wxString& fieldName = metaAttr->GetFieldNameDB(); wxString sqlField = wxEmptyString;

	if (metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN)) {
		if (!sqlField.IsEmpty())
			sqlField += ", ";
		sqlField += fieldName + "_B = excluded." + fieldName + "_B";
	}
	if (metaAttr->ContainType(eValueTypes::TYPE_NUMBER)) {
		if (!sqlField.IsEmpty())
			sqlField += ", ";
		sqlField += fieldName + "_N = excluded." + fieldName + "_N";
	}
	if (metaAttr->ContainType(eValueTypes::TYPE_DATE)) {
		if (!sqlField.IsEmpty())
			sqlField += ", ";
		sqlField += fieldName + "_D = excluded." + fieldName + "_D";
	}
	if (metaAttr->ContainType(eValueTypes::TYPE_STRING)) {
		if (!sqlField.IsEmpty())
			sqlField += ", ";
		sqlField += fieldName + "_S = excluded." + fieldName + "_S";
	}
	if (metaAttr->ContainType(eValueTypes::TYPE_ENUM)) {
		if (!sqlField.IsEmpty())
			sqlField += ", ";
		sqlField += fieldName + "_E = excluded." + fieldName + "_E";
	}
	if (metaAttr->ContainMetaType(eCtorMetaType::eCtorMetaType_Reference)) {
		if (!sqlField.IsEmpty())
			sqlField += ", ";
		sqlField += fieldName + "_RTRef = excluded." + fieldName + "_RTRef, " + fieldName + "_RRRef = excluded." + fieldName + "_RRRef";
	}

	return fieldName + "_TYPE = excluded." + fieldName + "_TYPE"
		+ (sqlField.Length() > 0 ? ", " : "")
		+ sqlField;
}

IValueMetaObjectAttribute::sqlField_t IValueMetaObjectAttribute::GetSQLFieldData(const IValueMetaObjectAttribute* metaAttr)
{
	const wxString& fieldName = metaAttr->GetFieldNameDB(); sqlField_t sqlData(fieldName + "_TYPE");

	if (metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN)) {
		sqlData.AppendType(
			eFieldTypes::eFieldTypes_Boolean,
			fieldName + "_B"
		);
	}

	if (metaAttr->ContainType(eValueTypes::TYPE_NUMBER)) {
		sqlData.AppendType(
			eFieldTypes::eFieldTypes_Number,
			fieldName + "_N"
		);
	}

	if (metaAttr->ContainType(eValueTypes::TYPE_DATE)) {
		sqlData.AppendType(
			eFieldTypes::eFieldTypes_Date,
			fieldName + "_D"
		);
	}

	if (metaAttr->ContainType(eValueTypes::TYPE_STRING)) {
		sqlData.AppendType(
			eFieldTypes::eFieldTypes_String,
			fieldName + "_S"
		);
	}

	if (metaAttr->ContainType(eValueTypes::TYPE_ENUM)) {
		sqlData.AppendType(
			eFieldTypes::eFieldTypes_Enum,
			fieldName + "_E"
		);
	}

	if (metaAttr->ContainMetaType(eCtorMetaType::eCtorMetaType_Reference)) {
		sqlData.AppendType(
			eFieldTypes::eFieldTypes_Reference,
			fieldName + "_RTRef",
			fieldName + "_RRRef"
		);
	}

	return sqlData;
}

#include "backend/valueInfo.h"

int IValueMetaObjectAttribute::ProcessAttribute(const wxString& tableName,
	const IValueMetaObjectAttribute* srcAttr, const IValueMetaObjectAttribute* dstAttr)
{
	int retCode = 1;
	//is null - create
	if (dstAttr == nullptr) {
		const wxString& fieldName = srcAttr->GetFieldNameDB(); bool createReference = false; IMetaData* metaData = srcAttr->GetMetaData();
		retCode = db_query->RunQuery("ALTER TABLE %s ADD %s_TYPE %s DEFAULT 0 NOT NULL;", tableName, fieldName, "INTEGER");
		if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
			return retCode;
		const CTypeDescription& typeDesc = srcAttr->GetTypeDesc();
		for (auto clsid : typeDesc.GetClsidList()) {
			eValueTypes valType = CValue::GetVTByID(clsid);
			switch (valType) {
			case eValueTypes::TYPE_BOOLEAN:
				retCode = db_query->RunQuery("ALTER TABLE %s ADD %s_B %s;", tableName, fieldName, srcAttr->GetSQLTypeObject(clsid));
				break;
			case eValueTypes::TYPE_NUMBER:
				retCode = db_query->RunQuery("ALTER TABLE %s ADD %s_N %s;", tableName, fieldName, srcAttr->GetSQLTypeObject(clsid));
				break;
			case eValueTypes::TYPE_DATE:
				retCode = db_query->RunQuery("ALTER TABLE %s ADD %s_D %s;", tableName, fieldName, srcAttr->GetSQLTypeObject(clsid));
				break;
			case eValueTypes::TYPE_STRING:
				retCode = db_query->RunQuery("ALTER TABLE %s ADD %s_S %s;", tableName, fieldName, srcAttr->GetSQLTypeObject(clsid));
				break;
			case eValueTypes::TYPE_ENUM:
				retCode = db_query->RunQuery("ALTER TABLE %s ADD %s_E %s DEFAULT 0;", tableName, fieldName, srcAttr->GetSQLTypeObject(clsid));
				break;
			default:
				const IMetaValueTypeCtor* typeCtor = metaData->GetTypeCtor(clsid);
				wxASSERT(typeCtor);
				if (typeCtor != nullptr && eCtorMetaType::eCtorMetaType_Reference == typeCtor->GetMetaTypeCtor()) {
					createReference = true;
				}
			}
		}
		if (createReference) {
			retCode = db_query->RunQuery("ALTER TABLE %s ADD %s_RTRef %s;", tableName, fieldName, "BIGINT");
			if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
				return retCode;

			retCode = db_query->RunQuery("ALTER TABLE %s ADD %s_RRRef %s;", tableName, fieldName, db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL ? wxT("BYTEA") : wxString::Format(wxT("BINARY(%i)"), reference_size_t));
			if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
				return retCode;
		}
	}
	// update 
	else if (srcAttr != nullptr) {
		if (srcAttr->GetTypeDesc() != dstAttr->GetTypeDesc()) {
			const CTypeDescription& srcTypeDesc = srcAttr->GetTypeDesc();
			const wxString& fieldName = srcAttr->GetFieldNameDB(); std::set<class_identifier_t> createdRef, currentRef, removedRef; IMetaData* metaData = srcAttr->GetMetaData();
			for (auto clsid : srcTypeDesc.GetClsidList()) {
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return retCode;
				if (!dstAttr->ContainType(clsid)) {
					eValueTypes valType = CValue::GetVTByID(clsid);
					switch (valType) {
					case eValueTypes::TYPE_BOOLEAN:
						retCode = db_query->RunQuery("ALTER TABLE %s ADD %s_B %s;", tableName, fieldName, srcAttr->GetSQLTypeObject(clsid));
						break;
					case eValueTypes::TYPE_NUMBER:
						retCode = db_query->RunQuery("ALTER TABLE %s ADD %s_N %s;", tableName, fieldName, srcAttr->GetSQLTypeObject(clsid));
						break;
					case eValueTypes::TYPE_DATE:
						retCode = db_query->RunQuery("ALTER TABLE %s ADD %s_D %s;", tableName, fieldName, srcAttr->GetSQLTypeObject(clsid));
						break;
					case eValueTypes::TYPE_STRING:
						retCode = db_query->RunQuery("ALTER TABLE %s ADD %s_S %s;", tableName, fieldName, srcAttr->GetSQLTypeObject(clsid));
						break;
					case eValueTypes::TYPE_ENUM:
						retCode = db_query->RunQuery("ALTER TABLE %s ADD %s_E %s DEFAULT 0;", tableName, fieldName, srcAttr->GetSQLTypeObject(clsid));
						break;
					default:
						const IMetaValueTypeCtor* typeCtor = metaData->GetTypeCtor(clsid);
						//wxASSERT(typeCtor);
						if (typeCtor != nullptr && eCtorMetaType::eCtorMetaType_Reference == typeCtor->GetMetaTypeCtor()) {
							createdRef.insert(clsid);
						}
					}
				}
			}
			const CTypeDescription& dstTypeDesc = dstAttr->GetTypeDesc();
			for (auto clsid : dstTypeDesc.GetClsidList()) {
				eValueTypes valType = CValue::GetVTByID(clsid);
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return retCode;
				if (srcAttr->ContainType(clsid)) {
					switch (valType) {
					case eValueTypes::TYPE_BOOLEAN:
						if (!srcAttr->EqualType(clsid, dstAttr->GetTypeDesc()))
							retCode = db_query->RunQuery("ALTER TABLE %s ALTER COLUMN %s_B TYPE %s;", tableName, fieldName, srcAttr->GetSQLTypeObject(clsid));
						break;
					case eValueTypes::TYPE_NUMBER:
						if (!srcAttr->EqualType(clsid, dstAttr->GetTypeDesc()))
							retCode = db_query->RunQuery("ALTER TABLE %s ALTER COLUMN %s_N TYPE %s;", tableName, fieldName, srcAttr->GetSQLTypeObject(clsid));
						break;
					case eValueTypes::TYPE_DATE:
						if (!srcAttr->EqualType(clsid, dstAttr->GetTypeDesc())) {
							if (srcTypeDesc.GetDateFraction() != eDateFractions::eDateFractions_Time && dstTypeDesc.GetDateFraction() == eDateFractions::eDateFractions_Time) {
								retCode = db_query->RunQuery("ALTER TABLE %s DROP %s_D;", tableName, fieldName);
								if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
									return retCode;
								retCode = db_query->RunQuery("ALTER TABLE %s ADD %s_D %s;", tableName, fieldName, srcAttr->GetSQLTypeObject(clsid));
							}
							else {
								retCode = db_query->RunQuery("ALTER TABLE %s ALTER COLUMN %s_D TYPE %s;", tableName, fieldName, srcAttr->GetSQLTypeObject(clsid));
							}
						}
						break;
					case eValueTypes::TYPE_STRING:
						if (!srcAttr->EqualType(clsid, dstAttr->GetTypeDesc()))
							retCode = db_query->RunQuery("ALTER TABLE %s ALTER COLUMN %s_S TYPE %s;", tableName, fieldName, srcAttr->GetSQLTypeObject(clsid));
						break;
					case eValueTypes::TYPE_ENUM:
						if (!srcAttr->EqualType(clsid, dstAttr->GetTypeDesc()))
							retCode = db_query->RunQuery("ALTER TABLE %s ALTER COLUMN %s_E TYPE %s;", tableName, fieldName, srcAttr->GetSQLTypeObject(clsid));
						break;
					default:
						const IMetaValueTypeCtor* typeCtor = metaData->GetTypeCtor(clsid);
						//wxASSERT(typeCtor);
						if (typeCtor != nullptr && eCtorMetaType::eCtorMetaType_Reference == typeCtor->GetMetaTypeCtor()) {
							currentRef.insert(clsid);
						}
					}
				}
				else {
					switch (valType) {
					case eValueTypes::TYPE_BOOLEAN:
						retCode = db_query->RunQuery("ALTER TABLE %s DROP %s_B;", tableName, fieldName);
						if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
							return retCode;
						retCode = db_query->RunQuery("UPDATE %s SET %s_TYPE = 0 WHERE %s_TYPE = %i;", tableName, fieldName, fieldName, (int)eFieldTypes::eFieldTypes_Boolean);
						break;
					case eValueTypes::TYPE_NUMBER:
						retCode = db_query->RunQuery("ALTER TABLE %s DROP %s_N;", tableName, fieldName);
						if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
							return retCode;
						retCode = db_query->RunQuery("UPDATE %s SET %s_TYPE = 0 WHERE %s_TYPE = %i;", tableName, fieldName, fieldName, (int)eFieldTypes::eFieldTypes_Number);
						break;
					case eValueTypes::TYPE_DATE:
						retCode = db_query->RunQuery("ALTER TABLE %s DROP %s_D;", tableName, fieldName);
						if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
							return retCode;
						retCode = db_query->RunQuery("UPDATE %s SET %s_TYPE = 0 WHERE %s_TYPE = %i;", tableName, fieldName, fieldName, (int)eFieldTypes::eFieldTypes_Date);
						break;
					case eValueTypes::TYPE_STRING:
						retCode = db_query->RunQuery("ALTER TABLE %s DROP %s_S;", tableName, fieldName);
						if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
							return retCode;
						retCode = db_query->RunQuery("UPDATE %s SET %s_TYPE = 0 WHERE %s_TYPE = %i;", tableName, fieldName, fieldName, (int)eFieldTypes::eFieldTypes_String);
						break;
					case eValueTypes::TYPE_ENUM:
						retCode = db_query->RunQuery("ALTER TABLE %s DROP %s_E;", tableName, fieldName);
						if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
							return retCode;
						retCode = db_query->RunQuery("UPDATE %s SET %s_TYPE = 0 WHERE %s_TYPE = %i;", tableName, fieldName, fieldName, (int)eFieldTypes::eFieldTypes_Enum);
						break;
					default:
						const IMetaValueTypeCtor* typeCtor = metaData->GetTypeCtor(clsid);
						//wxASSERT(typeCtor);
						if (typeCtor != nullptr && eCtorMetaType::eCtorMetaType_Reference == typeCtor->GetMetaTypeCtor()) {
							removedRef.insert(clsid);
						}
					}
				}
			}
			if (createdRef.size() > 0 && currentRef.size() == 0 && removedRef.size() == 0) {
				retCode = db_query->RunQuery("ALTER TABLE %s ADD %s_RTRef %s;", tableName, fieldName, "BIGINT");
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return retCode;
				retCode = db_query->RunQuery("ALTER TABLE %s ADD %s_RRRef %s;", tableName, fieldName, db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL ? wxT("BYTEA") : wxString::Format(wxT("BINARY(%i)"), reference_size_t));
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return retCode;
			}
			if (currentRef.size() > 0) {
				for (auto clsid : removedRef) {
					wxString clsStr; clsStr << clsid;
					retCode = db_query->RunQuery("DELETE FROM %s WHERE %s_RTRef = " + clsStr, tableName, fieldName);
					if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
						return retCode;
				}
			}
			if (removedRef.size() > 0 && currentRef.size() == 0) {
				retCode = db_query->RunQuery("UPDATE %s SET %s_TYPE = 0 WHERE %s_TYPE = %i;", tableName, fieldName, fieldName, (int)eFieldTypes::eFieldTypes_Reference);
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return retCode;
				retCode = db_query->RunQuery("ALTER TABLE %s DROP %s_RTRef;", tableName, fieldName);
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return retCode;
				retCode = db_query->RunQuery("ALTER TABLE %s DROP %s_RRRef;", tableName, fieldName);
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return retCode;
			}
		}
	}
	//delete 
	else if (srcAttr == nullptr) {
		const wxString& fieldName = dstAttr->GetFieldNameDB(); bool removeReference = false; IMetaData* metaData = dstAttr->GetMetaData();
		retCode = db_query->RunQuery("ALTER TABLE %s DROP %s_TYPE;", tableName, fieldName);
		if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
			return retCode;
		const CTypeDescription& dstTypeDesc = dstAttr->GetTypeDesc();
		for (auto clsid : dstTypeDesc.GetClsidList()) {
			eValueTypes valType = CValue::GetVTByID(clsid);
			switch (valType) {
			case eValueTypes::TYPE_BOOLEAN:
				retCode = db_query->RunQuery("ALTER TABLE %s DROP %s_B;", tableName, fieldName);
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return retCode;
				break;
			case eValueTypes::TYPE_NUMBER:
				retCode = db_query->RunQuery("ALTER TABLE %s DROP %s_N;", tableName, fieldName);
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return retCode;
				break;
			case eValueTypes::TYPE_DATE:
				retCode = db_query->RunQuery("ALTER TABLE %s DROP %s_D;", tableName, fieldName);
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return retCode;
				break;
			case eValueTypes::TYPE_STRING:
				retCode = db_query->RunQuery("ALTER TABLE %s DROP %s_S;", tableName, fieldName);
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return retCode;
				break;
			case eValueTypes::TYPE_ENUM:
				retCode = db_query->RunQuery("ALTER TABLE %s DROP %s_E;", tableName, fieldName);
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return retCode;
				break;
			default:
				const IMetaValueTypeCtor* typeCtor = metaData->GetTypeCtor(clsid);
				//wxASSERT(typeCtor);
				if (typeCtor != nullptr && eCtorMetaType::eCtorMetaType_Reference == typeCtor->GetMetaTypeCtor()) {
					removeReference = true;
				}
				break;
			}
		}

		if (removeReference) {
			retCode = db_query->RunQuery("ALTER TABLE %s DROP %s_RTRef;", tableName, fieldName);
			if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
				return retCode;
			retCode = db_query->RunQuery("ALTER TABLE %s DROP %s_RRRef;", tableName, fieldName);
			if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
				return retCode;
		}
	}

	return retCode;
}

void IValueMetaObjectAttribute::SetValueAttribute(const IValueMetaObjectAttribute* metaAttr,
	const CValue& cValue, IPreparedStatement* statement, int& position)
{
	//write type & data
	if (cValue.GetType() == eValueTypes::TYPE_EMPTY) {

		statement->SetParamInt(position++, eFieldTypes_Empty); //TYPE

		if (metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN))
			statement->SetParamBool(position++, false); //DATA binary 
		if (metaAttr->ContainType(eValueTypes::TYPE_NUMBER))
			statement->SetParamNumber(position++, 0); //DATA number 
		if (metaAttr->ContainType(eValueTypes::TYPE_DATE))
			statement->SetParamDate(position++, emptyDate); //DATA date 
		if (metaAttr->ContainType(eValueTypes::TYPE_STRING))
			statement->SetParamString(position++, wxEmptyString); //DATA string 

		if (metaAttr->ContainType(eValueTypes::TYPE_ENUM))
			statement->SetParamInt(position++, wxNOT_FOUND); //DATA enum 

		if (metaAttr->ContainMetaType(eCtorMetaType::eCtorMetaType_Reference)) {
			statement->SetParamNumber(position++, 0); //TYPE REF
			statement->SetParamNull(position++); //DATA REF
		}
	}
	else if (cValue.GetType() == eValueTypes::TYPE_BOOLEAN) {

		statement->SetParamInt(position++, eFieldTypes_Boolean); //TYPE

		if (metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN))
			statement->SetParamBool(position++, cValue.GetBoolean()); //DATA binary 
		if (metaAttr->ContainType(eValueTypes::TYPE_NUMBER))
			statement->SetParamNumber(position++, 0); //DATA number 
		if (metaAttr->ContainType(eValueTypes::TYPE_DATE))
			statement->SetParamDate(position++, emptyDate); //DATA date 
		if (metaAttr->ContainType(eValueTypes::TYPE_STRING))
			statement->SetParamString(position++, wxEmptyString); //DATA string 

		if (metaAttr->ContainType(eValueTypes::TYPE_ENUM))
			statement->SetParamInt(position++, wxNOT_FOUND); //DATA enum 

		if (metaAttr->ContainMetaType(eCtorMetaType::eCtorMetaType_Reference)) {
			statement->SetParamNumber(position++, 0); //TYPE REF
			statement->SetParamNull(position++); //DATA REF
		}
	}
	else if (cValue.GetType() == eValueTypes::TYPE_NUMBER) {

		statement->SetParamInt(position++, eFieldTypes_Number); //TYPE

		if (metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN))
			statement->SetParamBool(position++, false); //DATA binary 
		if (metaAttr->ContainType(eValueTypes::TYPE_NUMBER))
			statement->SetParamNumber(position++, cValue.GetNumber()); //DATA number 
		if (metaAttr->ContainType(eValueTypes::TYPE_DATE))
			statement->SetParamDate(position++, emptyDate); //DATA date 
		if (metaAttr->ContainType(eValueTypes::TYPE_STRING))
			statement->SetParamString(position++, wxEmptyString); //DATA string 

		if (metaAttr->ContainType(eValueTypes::TYPE_ENUM))
			statement->SetParamInt(position++, wxNOT_FOUND); //DATA enum 

		if (metaAttr->ContainMetaType(eCtorMetaType::eCtorMetaType_Reference)) {
			statement->SetParamNumber(position++, 0); //TYPE REF
			statement->SetParamNull(position++); //DATA REF
		}
	}
	else if (cValue.GetType() == eValueTypes::TYPE_DATE) {

		statement->SetParamInt(position++, eFieldTypes_Date); //TYPE

		if (metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN))
			statement->SetParamBool(position++, false); //DATA binary 
		if (metaAttr->ContainType(eValueTypes::TYPE_NUMBER))
			statement->SetParamNumber(position++, 0); //DATA number 
		if (metaAttr->ContainType(eValueTypes::TYPE_DATE))
			statement->SetParamDate(position++, cValue.GetDate()); //DATA date 
		if (metaAttr->ContainType(eValueTypes::TYPE_STRING))
			statement->SetParamString(position++, wxEmptyString); //DATA string 

		if (metaAttr->ContainType(eValueTypes::TYPE_ENUM))
			statement->SetParamInt(position++, wxNOT_FOUND); //DATA enum 

		if (metaAttr->ContainMetaType(eCtorMetaType::eCtorMetaType_Reference)) {
			statement->SetParamNumber(position++, 0); //TYPE REF
			statement->SetParamNull(position++); //DATA REF
		}
	}
	else if (cValue.GetType() == eValueTypes::TYPE_STRING) {

		statement->SetParamInt(position++, eFieldTypes_String); //TYPE

		if (metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN))
			statement->SetParamBool(position++, false); //DATA binary 
		if (metaAttr->ContainType(eValueTypes::TYPE_NUMBER))
			statement->SetParamNumber(position++, 0); //DATA number 
		if (metaAttr->ContainType(eValueTypes::TYPE_DATE))
			statement->SetParamDate(position++, emptyDate); //DATA date 
		if (metaAttr->ContainType(eValueTypes::TYPE_STRING))
			statement->SetParamString(position++, cValue.GetString()); //DATA string 

		if (metaAttr->ContainType(eValueTypes::TYPE_ENUM))
			statement->SetParamInt(position++, wxNOT_FOUND); //DATA enum 

		if (metaAttr->ContainMetaType(eCtorMetaType::eCtorMetaType_Reference)) {
			statement->SetParamNumber(position++, 0); //TYPE REF
			statement->SetParamNull(position++); //DATA REF
		}
	}
	else if (cValue.GetType() == eValueTypes::TYPE_NULL) {

		statement->SetParamInt(position++, eFieldTypes_Null); //TYPE

		if (metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN))
			statement->SetParamBool(position++, false); //DATA binary 
		if (metaAttr->ContainType(eValueTypes::TYPE_NUMBER))
			statement->SetParamNumber(position++, 0); //DATA number 
		if (metaAttr->ContainType(eValueTypes::TYPE_DATE))
			statement->SetParamDate(position++, emptyDate); //DATA date 
		if (metaAttr->ContainType(eValueTypes::TYPE_STRING))
			statement->SetParamString(position++, wxEmptyString); //DATA string 

		if (metaAttr->ContainType(eValueTypes::TYPE_ENUM))
			statement->SetParamInt(position++, wxNOT_FOUND); //DATA enum 

		if (metaAttr->ContainMetaType(eCtorMetaType::eCtorMetaType_Reference)) {
			statement->SetParamNumber(position++, 0); //TYPE REF
			statement->SetParamNull(position++); //DATA REF
		}
	}
	else if (cValue.GetType() == eValueTypes::TYPE_ENUM) {

		statement->SetParamInt(position++, eFieldTypes_Enum); //TYPE

		if (metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN))
			statement->SetParamBool(position++, false); //DATA binary 
		if (metaAttr->ContainType(eValueTypes::TYPE_NUMBER))
			statement->SetParamNumber(position++, cValue.GetNumber()); //DATA number 
		if (metaAttr->ContainType(eValueTypes::TYPE_DATE))
			statement->SetParamDate(position++, emptyDate); //DATA date 	
		if (metaAttr->ContainType(eValueTypes::TYPE_STRING))
			statement->SetParamString(position++, wxEmptyString); //DATA string 

		if (metaAttr->ContainType(eValueTypes::TYPE_ENUM))
			statement->SetParamInt(position++, cValue.GetInteger()); //DATA enum 

		if (metaAttr->ContainMetaType(eCtorMetaType::eCtorMetaType_Reference)) {
			statement->SetParamNumber(position++, 0); //TYPE REF
			statement->SetParamNull(position++); //DATA REF
		}
	}
	else {

		statement->SetParamInt(position++, eFieldTypes_Reference); //TYPE

		if (metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN))
			statement->SetParamBool(position++, false); //DATA binary 
		if (metaAttr->ContainType(eValueTypes::TYPE_NUMBER))
			statement->SetParamNumber(position++, 0); //DATA number 
		if (metaAttr->ContainType(eValueTypes::TYPE_DATE))
			statement->SetParamDate(position++, emptyDate); //DATA date 
		if (metaAttr->ContainType(eValueTypes::TYPE_STRING))
			statement->SetParamString(position++, wxEmptyString); //DATA string 

		if (metaAttr->ContainType(eValueTypes::TYPE_ENUM))
			statement->SetParamInt(position++, wxNOT_FOUND); //DATA enum 

		const class_identifier_t& clsid = cValue.GetClassType();
		wxASSERT(clsid > 0);

		const IMetaData* metaData = metaAttr->GetMetaData();
		wxASSERT(metaData);

		const IMetaValueTypeCtor* typeCtor = metaData->GetTypeCtor(clsid);
		wxASSERT(typeCtor);

		if (typeCtor != nullptr && typeCtor->GetMetaTypeCtor() == eCtorMetaType::eCtorMetaType_Reference) {
			CValueReferenceDataObject* refData = nullptr;
			if (cValue.ConvertToValue(refData)) {
				statement->SetParamNumber(position++, clsid); //TYPE REF
				statement->SetParamBlob(position++, refData->GetReferenceData(), sizeof(reference_t)); //DATA REF
			}
			else {
				statement->SetParamNumber(position++, 0); //TYPE REF
				statement->SetParamNull(position++); //DATA REF
			}
		}
		else {
			statement->SetParamNumber(position++, 0); //TYPE REF
			statement->SetParamNull(position++); //DATA REF
		}
	}
}

void IValueMetaObjectAttribute::SetValueAttribute(const IValueMetaObjectAttribute* attribute, const CValue& cValue, IPreparedStatement* statement)
{
	int position = 1;
	SetValueAttribute(attribute, cValue, statement, position);
}

#include "backend/compiler/enumUnit.h"

bool IValueMetaObjectAttribute::GetValueAttribute(const wxString& fieldName,
	const eFieldTypes& fieldType, const IValueMetaObjectAttribute* metaAttr, CValue& retValue, IDatabaseResultSet* resultSet, bool createData)
{
	switch (fieldType)
	{
	case eFieldTypes_Boolean:
		retValue = resultSet->GetResultBool(fieldName);
		return true;
	case eFieldTypes_Number:
		retValue = resultSet->GetResultNumber(fieldName);
		return true;
	case eFieldTypes_Date:
		retValue = resultSet->GetResultDate(fieldName);
		return true;
	case eFieldTypes_String:
		retValue = resultSet->GetResultString(fieldName);
		return true;
	case eFieldTypes_Null:
		retValue = eValueTypes::TYPE_NULL;
		return true;
	case eFieldTypes_Enum:
	{
		const IMetaData* metaData = metaAttr->GetMetaData();
		wxASSERT(metaData);

		const CValue& defValue = metaAttr->CreateValue();
		const IAbstractTypeCtor* so = metaData->GetAvailableCtor(defValue.GetClassType());

		if (so != nullptr) {

			CValue enumVariant(resultSet->GetResultInt(fieldName));
			CValue* ppParams[] = { &enumVariant };

			try {
				CValuePtr<IEnumerationWrapper> creator(
					metaData->CreateAndConvertObjectRef<IEnumerationWrapper>(so->GetClassName(), ppParams, 1));
				retValue = creator->GetEnumVariantValue();
			}
			catch (...) {
				retValue = defValue;
				return false;
			}

			return true;
		}

		retValue = defValue;
		return false;
	}
	case eFieldTypes_Reference:
	{
		IMetaData* metaData = metaAttr->GetMetaData();
		wxASSERT(metaData);
		const class_identifier_t& refType = resultSet->GetResultLong(fieldName + wxT("_RTRef"));

		wxMemoryBuffer bufferData;
		resultSet->GetResultBlob(fieldName + wxT("_RRRef"), bufferData);
		if (!bufferData.IsEmpty()) {

			if (createData) {

				CValuePtr<CValueReferenceDataObject> created_reference =
					CValueReferenceDataObject::CreateFromPtr(metaData, bufferData.GetData());

				retValue = created_reference;
				return created_reference != nullptr;
			}

			CValuePtr<CValueReferenceDataObject> created_reference =
				CValueReferenceDataObject::Create(metaData, bufferData.GetData());

			retValue = created_reference;
			return created_reference != nullptr;
		}
		else if (refType > 0) {

			const IMetaValueTypeCtor* typeCtor = metaData->GetTypeCtor(refType);
			if (typeCtor != nullptr) {

				const IValueMetaObject* metaObject = typeCtor->GetMetaObject();
				wxASSERT(metaObject);

				CValuePtr<CValueReferenceDataObject> created_reference =
					CValueReferenceDataObject::Create(metaData, metaObject->GetMetaID());

				retValue = created_reference;
				return created_reference != nullptr;
			}

			return false;
		}

		break;
	}
	}

	return false;
}

bool IValueMetaObjectAttribute::GetValueAttribute(const wxString& fieldName,
	const IValueMetaObjectAttribute* metaAttr, CValue& retValue, IDatabaseResultSet* resultSet, bool createData)
{
	eFieldTypes fieldType =
		static_cast<eFieldTypes>(resultSet->GetResultInt(fieldName + wxT("_TYPE")));

	switch (fieldType)
	{
	case eFieldTypes_Boolean:
		return IValueMetaObjectAttribute::GetValueAttribute(fieldName + wxT("_B"), eFieldTypes_Boolean, metaAttr, retValue, resultSet, createData);
	case eFieldTypes_Number:
		return IValueMetaObjectAttribute::GetValueAttribute(fieldName + wxT("_N"), eFieldTypes_Number, metaAttr, retValue, resultSet, createData);
	case eFieldTypes_Date:
		return IValueMetaObjectAttribute::GetValueAttribute(fieldName + wxT("_D"), eFieldTypes_Date, metaAttr, retValue, resultSet, createData);
	case eFieldTypes_String:
		return IValueMetaObjectAttribute::GetValueAttribute(fieldName + wxT("_S"), eFieldTypes_String, metaAttr, retValue, resultSet, createData);
	case eFieldTypes_Null:
		retValue = eValueTypes::TYPE_NULL;
		return true;
	case eFieldTypes_Enum:
		return IValueMetaObjectAttribute::GetValueAttribute(fieldName + wxT("_E"), eFieldTypes_Enum, metaAttr, retValue, resultSet, createData);
	case eFieldTypes_Reference:
		return IValueMetaObjectAttribute::GetValueAttribute(fieldName, eFieldTypes_Reference, metaAttr, retValue, resultSet, createData);
	default:
		retValue = metaAttr->CreateValue(); // if attribute was updated after 
		return true;
	}

	return false;
}

bool IValueMetaObjectAttribute::GetValueAttribute(const IValueMetaObjectAttribute* metaAttr, CValue& retValue, IDatabaseResultSet* resultSet, bool createData)
{
	return IValueMetaObjectAttribute::GetValueAttribute(
		metaAttr->GetFieldNameDB(),
		metaAttr, retValue, resultSet, createData
	);
}

///////////////////////////////////////////////////

void IValueMetaObjectAttribute::SetBinaryData(const IValueMetaObjectAttribute* metaAttr, const CMemoryReader& reader, IPreparedStatement* statement,
	int& position)
{
	const int fieldType = reader.r_s32();

	//write type & data
	if (fieldType == eFieldTypes_Empty || fieldType == eFieldTypes_Null) {

		statement->SetParamInt(position++, fieldType); //TYPE

		if (metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN))
			statement->SetParamBool(position++, false); //DATA binary 

		if (metaAttr->ContainType(eValueTypes::TYPE_NUMBER))
			statement->SetParamNumber(position++, 0); //DATA number 

		if (metaAttr->ContainType(eValueTypes::TYPE_DATE))
			statement->SetParamDate(position++, emptyDate); //DATA date 

		if (metaAttr->ContainType(eValueTypes::TYPE_STRING))
			statement->SetParamString(position++, wxEmptyString); //DATA string 

		if (metaAttr->ContainType(eValueTypes::TYPE_ENUM))
			statement->SetParamInt(position++, wxNOT_FOUND); //DATA enum 

		if (metaAttr->ContainMetaType(eCtorMetaType::eCtorMetaType_Reference)) {
			statement->SetParamNumber(position++, 0); //TYPE REF
			statement->SetParamNull(position++); //DATA REF
		}
	}
	else if (fieldType == eFieldTypes_Boolean) {

		statement->SetParamInt(position++, fieldType); //TYPE

		if (metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN))
			statement->SetParamBool(position++, reader.r_u8()); //DATA binary 

		if (metaAttr->ContainType(eValueTypes::TYPE_NUMBER))
			statement->SetParamNumber(position++, 0); //DATA number 

		if (metaAttr->ContainType(eValueTypes::TYPE_DATE))
			statement->SetParamDate(position++, emptyDate); //DATA date 

		if (metaAttr->ContainType(eValueTypes::TYPE_STRING))
			statement->SetParamString(position++, wxEmptyString); //DATA string 

		if (metaAttr->ContainType(eValueTypes::TYPE_ENUM))
			statement->SetParamInt(position++, wxNOT_FOUND); //DATA enum 

		if (metaAttr->ContainMetaType(eCtorMetaType::eCtorMetaType_Reference)) {
			statement->SetParamNumber(position++, 0); //TYPE REF
			statement->SetParamNull(position++); //DATA REF
		}
	}
	else if (fieldType == eFieldTypes_Number) {

		statement->SetParamInt(position++, fieldType); //TYPE

		if (metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN))
			statement->SetParamBool(position++, false); //DATA binary 

		if (metaAttr->ContainType(eValueTypes::TYPE_NUMBER)) {

			number_t value;
			reader.r(&value.exponent, sizeof(value.exponent));
			reader.r(&value.mantissa, sizeof(value.mantissa));
			reader.r(&value.info, sizeof(value.info));

			statement->SetParamNumber(position++, value); //DATA number 
		}

		if (metaAttr->ContainType(eValueTypes::TYPE_DATE))
			statement->SetParamDate(position++, emptyDate); //DATA date 

		if (metaAttr->ContainType(eValueTypes::TYPE_STRING))
			statement->SetParamString(position++, wxEmptyString); //DATA string 

		if (metaAttr->ContainType(eValueTypes::TYPE_ENUM))
			statement->SetParamInt(position++, wxNOT_FOUND); //DATA enum 

		if (metaAttr->ContainMetaType(eCtorMetaType::eCtorMetaType_Reference)) {
			statement->SetParamNumber(position++, 0); //TYPE REF
			statement->SetParamNull(position++); //DATA REF
		}
	}
	else if (fieldType == eFieldTypes_Date) {

		statement->SetParamInt(position++, fieldType); //TYPE

		if (metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN))
			statement->SetParamBool(position++, false); //DATA binary 

		if (metaAttr->ContainType(eValueTypes::TYPE_NUMBER))
			statement->SetParamNumber(position++, 0); //DATA number 

		if (metaAttr->ContainType(eValueTypes::TYPE_DATE))
			statement->SetParamDate(position++, wxLongLong(reader.r_u64())); //DATA date 

		if (metaAttr->ContainType(eValueTypes::TYPE_STRING))
			statement->SetParamString(position++, wxEmptyString); //DATA string 

		if (metaAttr->ContainType(eValueTypes::TYPE_ENUM))
			statement->SetParamInt(position++, wxNOT_FOUND); //DATA enum 

		if (metaAttr->ContainMetaType(eCtorMetaType::eCtorMetaType_Reference)) {
			statement->SetParamNumber(position++, 0); //TYPE REF
			statement->SetParamNull(position++); //DATA REF
		}
	}
	else if (fieldType == eFieldTypes_String) {

		statement->SetParamInt(position++, fieldType); //TYPE

		if (metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN))
			statement->SetParamBool(position++, false); //DATA binary 

		if (metaAttr->ContainType(eValueTypes::TYPE_NUMBER))
			statement->SetParamNumber(position++, 0); //DATA number 

		if (metaAttr->ContainType(eValueTypes::TYPE_DATE))
			statement->SetParamDate(position++, emptyDate); //DATA date 

		if (metaAttr->ContainType(eValueTypes::TYPE_STRING)) {
			statement->SetParamString(position++, reader.r_stringZ()); //DATA string 
		}

		if (metaAttr->ContainType(eValueTypes::TYPE_ENUM))
			statement->SetParamInt(position++, wxNOT_FOUND); //DATA enum 

		if (metaAttr->ContainMetaType(eCtorMetaType::eCtorMetaType_Reference)) {
			statement->SetParamNumber(position++, 0); //TYPE REF
			statement->SetParamNull(position++); //DATA REF
		}
	}
	else if (fieldType == eFieldTypes_Enum) {

		statement->SetParamInt(position++, fieldType); //TYPE

		if (metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN))
			statement->SetParamBool(position++, false); //DATA binary 

		if (metaAttr->ContainType(eValueTypes::TYPE_NUMBER))
			statement->SetParamNumber(position++, 0); //DATA number 

		if (metaAttr->ContainType(eValueTypes::TYPE_DATE))
			statement->SetParamDate(position++, emptyDate); //DATA date 	

		if (metaAttr->ContainType(eValueTypes::TYPE_STRING))
			statement->SetParamString(position++, wxEmptyString); //DATA string 

		if (metaAttr->ContainType(eValueTypes::TYPE_ENUM))
			statement->SetParamInt(position++, reader.r_s32()); //DATA enum 

		if (metaAttr->ContainMetaType(eCtorMetaType::eCtorMetaType_Reference)) {
			statement->SetParamNumber(position++, 0); //TYPE REF
			statement->SetParamNull(position++); //DATA REF
		}
	}
	else {

		wxMemoryBuffer typeRRBuffer;

		statement->SetParamInt(position++, fieldType); //TYPE

		if (metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN))
			statement->SetParamBool(position++, false); //DATA binary 
		if (metaAttr->ContainType(eValueTypes::TYPE_NUMBER))
			statement->SetParamNumber(position++, 0); //DATA number 
		if (metaAttr->ContainType(eValueTypes::TYPE_DATE))
			statement->SetParamDate(position++, emptyDate); //DATA date 
		if (metaAttr->ContainType(eValueTypes::TYPE_STRING))
			statement->SetParamString(position++, wxEmptyString); //DATA string 

		if (metaAttr->ContainType(eValueTypes::TYPE_ENUM))
			statement->SetParamInt(position++, wxNOT_FOUND); //DATA enum 

		statement->SetParamNumber(position++, reader.r_u64()); //TYPE REF
		reader.r_chunk(rt_ref_chunk, typeRRBuffer);
		statement->SetParamBlob(position++, typeRRBuffer.GetData(), typeRRBuffer.GetDataLen()); //DATA REF
	}
}

void IValueMetaObjectAttribute::SetBinaryData(const IValueMetaObjectAttribute* metaAttr, const CMemoryReader& reader, IPreparedStatement* statement)
{
	int position = 1;
	SetBinaryData(metaAttr, reader, statement, position);
}

void IValueMetaObjectAttribute::GetBinaryData(const IValueMetaObjectAttribute* metaAttr, CMemoryWriter& writer, IDatabaseResultSet* resultSet)
{
	const wxString& fieldName = metaAttr->GetFieldNameDB();
	const int fieldType = resultSet->GetResultInt(fieldName + wxT("_TYPE"));

	writer.w_s32(fieldType);

	//DATA boolean 
	if (fieldType == eFieldTypes_Boolean
		&& metaAttr->ContainType(eValueTypes::TYPE_BOOLEAN)
		&& resultSet != nullptr) {
		writer.w_u8(resultSet->GetResultBool(fieldName + wxT("_B")));
	}
	else if (fieldType == eFieldTypes_Boolean) {
		writer.w_u8(false);
	}

	//DATA number 
	if (fieldType == eFieldTypes_Number
		&& metaAttr->ContainType(eValueTypes::TYPE_NUMBER)
		&& resultSet != nullptr) {
		const number_t& value = resultSet->GetResultNumber(fieldName + wxT("_N"));
		writer.w(&value.exponent, sizeof(value.exponent));
		writer.w(&value.mantissa, sizeof(value.mantissa));
		writer.w(&value.info, sizeof(value.info));
	}
	else if (fieldType == eFieldTypes_Number) {
		const number_t& value = 0;
		writer.w(&value.exponent, sizeof(value.exponent));
		writer.w(&value.mantissa, sizeof(value.mantissa));
		writer.w(&value.info, sizeof(value.info));
	}

	//DATA date 
	if (fieldType == eFieldTypes_Date
		&& metaAttr->ContainType(eValueTypes::TYPE_DATE)
		&& resultSet != nullptr) {
		const wxDateTime& dt = resultSet->GetResultDate(fieldName + wxT("_D"));
		const wxLongLong& llData = dt.GetValue();
		writer.w_u64(llData.GetValue());
	}
	else if (fieldType == eFieldTypes_Date) {
		writer.w_u64(emptyDate);
	}

	//DATA string 
	if (fieldType == eFieldTypes_String
		&& metaAttr->ContainType(eValueTypes::TYPE_STRING)
		&& resultSet != nullptr) {
		writer.w_stringZ(resultSet->GetResultString(fieldName + wxT("_S")));
	}
	else if (fieldType == eFieldTypes_String) {
		writer.w_stringZ(wxEmptyString);
	}

	//DATA enum 
	if (fieldType == eFieldTypes_Enum
		&& metaAttr->ContainType(eValueTypes::TYPE_ENUM)
		&& resultSet != nullptr) {
		writer.w_s32(resultSet->GetResultInt(fieldName + wxT("_E")));
	}
	else if (fieldType == eFieldTypes_Enum) {
		writer.w_s32(wxNOT_FOUND);
	}

	//DATA reference 
	if (fieldType == eFieldTypes_Reference
		&& metaAttr->ContainMetaType(eCtorMetaType::eCtorMetaType_Reference)
		&& resultSet != nullptr) {
		wxMemoryBuffer bufferData;
		resultSet->GetResultBlob(fieldName + wxT("_RRRef"), bufferData);
		writer.w_u64(resultSet->GetResultLong(fieldName + wxT("_RTRef")));
		writer.w_chunk(rt_ref_chunk, bufferData);
	}
	else if (fieldType == eFieldTypes_Reference) {
		writer.w_u64(0);
		writer.w_chunk(rt_ref_chunk, wxMemoryBuffer());
	}
}

///////////////////////////////////////////////////