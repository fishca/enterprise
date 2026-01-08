////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : base db reader/saver
////////////////////////////////////////////////////////////////////////////

#include "commonObject.h"

#include "backend/appData.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/databaseLayer/databaseErrorCodes.h"
#include "backend/metaCollection/partial/tabularSection/tabularSection.h"


//**********************************************************************************************************
//*                                          Common functions                                              *
//**********************************************************************************************************

wxString IMetaObjectRecordDataRef::GetTableNameDB() const
{
	const wxString& className = GetClassName();
	wxASSERT(m_metaId != 0);
	return wxString::Format("%s%i",
		className, GetMetaID());
}

int IMetaObjectRecordDataMutableRef::ProcessAttribute(const wxString& tableName, IMetaObjectAttribute* srcAttr, IMetaObjectAttribute* dstAttr)
{
	//is null - create
	if (dstAttr == nullptr) {
		s_restructureInfo.AppendInfo(_("Create attribute ") + srcAttr->GetFullName());
	}
	// update 
	else if (srcAttr != nullptr) {
		if (!srcAttr->CompareObject(dstAttr))
			s_restructureInfo.AppendInfo(_("Changing attribute ") + srcAttr->GetFullName());
	}
	//delete 
	else if (srcAttr == nullptr) {
		s_restructureInfo.AppendInfo(_("Removed attribute ") + dstAttr->GetFullName());
	}

	return IMetaObjectAttribute::ProcessAttribute(tableName, srcAttr, dstAttr);
}

int IMetaObjectRecordDataMutableRef::ProcessTable(const wxString& tabularName, CMetaObjectTableData* srcTable, CMetaObjectTableData* dstTable)
{
	int retCode = 1;
	//is null - create
	if (dstTable == nullptr) {

		s_restructureInfo.AppendInfo(_("Create tabular section ") + srcTable->GetFullName());

		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
			retCode = db_query->RunQuery("CREATE TABLE %s (uuid uuid NOT NULL);", tabularName);
		else
			retCode = db_query->RunQuery("CREATE TABLE %s (uuid VARCHAR(36) NOT NULL);", tabularName);

		if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
			return retCode;

		//default attributes
		for (const auto object : srcTable->GetGenericAttributeArrayObject()) {
			retCode = ProcessAttribute(tabularName,
				object, nullptr);
		}

		if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
			return retCode;

		retCode = db_query->RunQuery("CREATE INDEX %s_INDEX ON %s (uuid);", tabularName, tabularName);
	}
	// update 
	else if (srcTable != nullptr) {

		if (!srcTable->CompareObject(dstTable))
			s_restructureInfo.AppendInfo(_("Changing tabular section ") + srcTable->GetFullName());

		for (const auto object : srcTable->GetGenericAttributeArrayObject()) {
			retCode = ProcessAttribute(tabularName,
				object, dstTable->FindAnyAttributeObjectByFilter(object->GetGuid())
			);
			if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
				return retCode;
		}
	}
	//delete 
	else if (srcTable == nullptr) {

		s_restructureInfo.AppendInfo(_("Removed tabular section ") + dstTable->GetFullName());

		retCode = db_query->RunQuery("DROP TABLE %s", tabularName);
	}

	return retCode;
}

bool IMetaObjectRecordDataMutableRef::CreateAndUpdateTableDB(IMetaDataConfiguration* srcMetaData, IMetaObject* srcMetaObject, int flags)
{
	const wxString& tableName = GetTableNameDB(); int retCode = 1;

	if ((flags & createMetaTable) != 0) {

		s_restructureInfo.AppendInfo(_("Create ") + GetFullName());

		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
			retCode = db_query->RunQuery("CREATE TABLE %s (uuid uuid NOT NULL PRIMARY KEY);", tableName);
		else
			retCode = db_query->RunQuery("CREATE TABLE %s (uuid VARCHAR(36) NOT NULL PRIMARY KEY);", tableName);

		if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
			return false;

		for (const auto object : GetPredefinedAttributeArrayObject()) {
			if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
				return false;
			if (IMetaObjectRecordDataRef::IsDataReference(object->GetMetaID()))
				continue;
			retCode = ProcessAttribute(tableName,
				object, nullptr);
		}

		if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
			return false;

		for (const auto object : GetAttributeArrayObject()) {
			if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
				return false;
			retCode = ProcessAttribute(tableName,
				object, nullptr);
		}

		if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
			return false;

		for (const auto object : GetTableArrayObject()) {
			if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
				return false;
			retCode = ProcessTable(object->GetTableNameDB(),
				object, nullptr);
		}

		if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
			return false;

		retCode = db_query->RunQuery("CREATE INDEX %s_INDEX ON %s (uuid);", tableName, tableName);

	}
	else if ((flags & updateMetaTable) != 0) {

		//if src is null then delete
		IMetaObjectRecordDataRef* dstValue = nullptr;
		if (srcMetaObject->ConvertToValue(dstValue)) {

			if (!dstValue->CompareObject(this))
				s_restructureInfo.AppendInfo(_("Changed ") + GetFullName());

			//attributes from dst 
			for (const auto object : dstValue->GetPredefinedAttributeArrayObject()) {
				IMetaObject* foundedMeta =
					IMetaObjectRecordDataRef::FindPredefinedAttributeObjectByFilter(object->GetGuid());
				if (dstValue->IsDataReference(object->GetMetaID()))
					continue;
				if (foundedMeta == nullptr) {
					retCode = ProcessAttribute(tableName, nullptr, object);
					if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
						return false;
				}
			}

			//attributes current
			for (const auto object : GetPredefinedAttributeArrayObject()) {
				if (IMetaObjectRecordDataRef::IsDataReference(object->GetMetaID()))
					continue;
				retCode = ProcessAttribute(tableName,
					object, dstValue->FindPredefinedAttributeObjectByFilter(object->GetGuid())
				);
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return false;
			}

			//attributes from dst 
			for (const auto object : dstValue->GetAttributeArrayObject()) {
				IMetaObject* foundedMeta =
					IMetaObjectRecordDataRef::FindAttributeObjectByFilter(object->GetGuid());
				if (foundedMeta == nullptr) {
					retCode = ProcessAttribute(tableName, nullptr, object);
					if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
						return false;
				}
			}

			//attributes current
			for (const auto object : GetAttributeArrayObject()) {
				retCode = ProcessAttribute(tableName,
					object, dstValue->FindAttributeObjectByFilter(object->GetGuid())
				);
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return false;
			}

			//tables from dst 
			for (const auto object : dstValue->GetTableArrayObject()) {
				IMetaObject* foundedMeta =
					IMetaObjectRecordDataRef::FindTableObjectByFilter(object->GetGuid());
				if (foundedMeta == nullptr) {
					retCode = ProcessTable(object->GetTableNameDB(), nullptr, object);
					if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
						return false;
				}
			}

			//tables current 
			for (const auto object : GetTableArrayObject()) {
				retCode = ProcessTable(object->GetTableNameDB(),
					object, dstValue->FindTableObjectByFilter(object->GetGuid())
				);
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return false;
			}
		}
	}
	else if ((flags & deleteMetaTable) != 0) {

		s_restructureInfo.AppendInfo(_("Removed ") + GetFullName());

		if (db_query->TableExists(tableName)) {
			retCode = db_query->RunQuery("DROP TABLE %s", tableName);
		}
		if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
			return false;

		for (const auto object : GetTableArrayObject()) {
			if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
				return false;
			const wxString& tabularName = object->GetTableNameDB();
			if (db_query->TableExists(tabularName)) {
				retCode = db_query->RunQuery("DROP TABLE %s", tabularName);
			}
		}
	}

	if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
		return false;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

int IMetaObjectRecordDataEnumRef::ProcessEnumeration(const wxString& tableName, CMetaObjectEnum* srcEnum, CMetaObjectEnum* dstEnum)
{
	int retCode = 1;

	//is null - create
	if (dstEnum == nullptr) {

		s_restructureInfo.AppendInfo(_("Create enumeration ") + srcEnum->GetFullName());

		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
			retCode = db_query->RunQuery("INSERT INTO %s (uuid) VALUES ('%s') ON CONFLICT(uuid) DO UPDATE SET uuid = excluded.uuid;", tableName, srcEnum->GetGuid().str());
		else
			retCode = db_query->RunQuery("UPDATE OR INSERT INTO %s (uuid) VALUES ('%s') MATCHING(uuid);", tableName, srcEnum->GetGuid().str());
	}
	// update 
	else if (srcEnum != nullptr) {

		if (!srcEnum->CompareObject(dstEnum))
			s_restructureInfo.AppendInfo(_("Changing enumeration ") + srcEnum->GetFullName());

		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
			retCode = db_query->RunQuery("INSERT INTO %s (uuid) VALUES ('%s') ON CONFLICT(uuid) DO UPDATE SET uuid = excluded.uuid;", tableName, srcEnum->GetGuid().str());
		else
			retCode = db_query->RunQuery("UPDATE OR INSERT INTO %s (uuid) VALUES ('%s') MATCHING(uuid);", tableName, srcEnum->GetGuid().str());
	}
	//delete 
	else if (srcEnum == nullptr) {

		s_restructureInfo.AppendInfo(_("Removed enumeration ") + dstEnum->GetFullName());

		retCode = db_query->RunQuery("DELETE FROM %s WHERE uuid = '%s' ;", tableName, dstEnum->GetGuid().str());
	}

	return retCode;
}

bool IMetaObjectRecordDataEnumRef::CreateAndUpdateTableDB(IMetaDataConfiguration* srcMetaData, IMetaObject* srcMetaObject, int flags)
{
	const wxString& tableName = GetTableNameDB(); int retCode = 1;

	if ((flags & createMetaTable) != 0 || (flags & repairMetaTable) != 0) {

		if ((flags & createMetaTable) != 0) {

			s_restructureInfo.AppendInfo(_("Create ") + GetFullName());

			if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
				retCode = db_query->RunQuery("CREATE TABLE %s (uuid uuid NOT NULL PRIMARY KEY);", tableName);
			else
				retCode = db_query->RunQuery("CREATE TABLE %s (uuid VARCHAR(36) NOT NULL PRIMARY KEY);", tableName);

			if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
				return false;

			if (db_query->GetDatabaseLayerType() != DATABASELAYER_FIREBIRD) {

				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return false;

				for (const auto object : GetEnumObjectArray()) {
					if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
						return false;
					retCode = ProcessEnumeration(tableName,
						object, nullptr);
				}
			}

			if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
				return false;

			retCode = db_query->RunQuery("CREATE INDEX %s_INDEX ON %s (uuid);", tableName, tableName);
		}
		else if ((flags & repairMetaTable) != 0) {

			if (db_query->GetDatabaseLayerType() == DATABASELAYER_FIREBIRD) {
				retCode = 1;
				for (const auto object : GetEnumObjectArray()) {
					if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
						return false;
					retCode = ProcessEnumeration(tableName,
						object, nullptr);
				}
			}
		}
	}
	else if ((flags & updateMetaTable) != 0) {

		//if src is null then delete
		IMetaObjectRecordDataEnumRef* dstValue = nullptr;
		if (srcMetaObject->ConvertToValue(dstValue)) {

			if (!dstValue->CompareObject(this))
				s_restructureInfo.AppendInfo(_("Changed ") + GetFullName());

			//enums from dst 
			for (const auto object : dstValue->GetEnumObjectArray()) {
				IMetaObject* foundedMeta =
					IMetaObjectRecordDataEnumRef::FindEnumObjectByFilter(object->GetGuid());
				if (foundedMeta == nullptr) {
					retCode = ProcessEnumeration(tableName,
						nullptr, object);
					if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
						return false;
				}
			}
			//enums current
			for (const auto object : GetEnumObjectArray()) {
				retCode = ProcessEnumeration(tableName,
					object, dstValue->FindEnumObjectByFilter(object->GetGuid())
				);
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return false;
			}
		}
	}
	else if ((flags & deleteMetaTable) != 0) {

		s_restructureInfo.AppendInfo(_("Removed ") + GetFullName());

		if (db_query->TableExists(tableName)) {
			retCode = db_query->RunQuery("DROP TABLE %s", tableName);
		}

		if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
			return false;
	}

	if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
		return false;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

wxString IMetaObjectRegisterData::GetTableNameDB() const
{
	const wxString& className = GetClassName();
	wxASSERT(m_metaId != 0);
	return wxString::Format("%s%i",
		className, GetMetaID());
}

int IMetaObjectRegisterData::ProcessDimension(const wxString& tableName, IMetaObjectAttribute* srcAttr, IMetaObjectAttribute* dstAttr)
{
	//is null - create
	if (dstAttr == nullptr) {
		s_restructureInfo.AppendInfo(_("Create dimension ") + srcAttr->GetFullName());
	}
	// update 
	else if (srcAttr != nullptr) {
		if (!srcAttr->CompareObject(dstAttr))
			s_restructureInfo.AppendInfo(_("Changing dimension ") + srcAttr->GetFullName());
	}
	//delete 
	else if (srcAttr == nullptr) {
		s_restructureInfo.AppendInfo(_("Removed dimension ") + dstAttr->GetFullName());
	}

	return IMetaObjectAttribute::ProcessAttribute(tableName, srcAttr, dstAttr);
}

int IMetaObjectRegisterData::ProcessResource(const wxString& tableName, IMetaObjectAttribute* srcAttr, IMetaObjectAttribute* dstAttr)
{
	//is null - create
	if (dstAttr == nullptr) {
		s_restructureInfo.AppendInfo(_("Create resource ") + srcAttr->GetFullName());
	}
	// update 
	else if (srcAttr != nullptr) {
		if (!srcAttr->CompareObject(dstAttr))
			s_restructureInfo.AppendInfo(_("Changing resource ") + srcAttr->GetFullName());
	}
	//delete 
	else if (srcAttr == nullptr) {
		s_restructureInfo.AppendInfo(_("Removed resource ") + dstAttr->GetFullName());
	}

	return IMetaObjectAttribute::ProcessAttribute(tableName, srcAttr, dstAttr);
}

int IMetaObjectRegisterData::ProcessAttribute(const wxString& tableName, IMetaObjectAttribute* srcAttr, IMetaObjectAttribute* dstAttr)
{
	//is null - create
	if (dstAttr == nullptr) {
		s_restructureInfo.AppendInfo(_("Create attribute ") + srcAttr->GetFullName());
	}
	// update 
	else if (srcAttr != nullptr) {
		if (!srcAttr->CompareObject(dstAttr))
			s_restructureInfo.AppendInfo(_("Changing attribute ") + srcAttr->GetFullName());
	}
	//delete 
	else if (srcAttr == nullptr) {
		s_restructureInfo.AppendInfo(_("Removed attribute ") + dstAttr->GetFullName());
	}

	return IMetaObjectAttribute::ProcessAttribute(tableName, srcAttr, dstAttr);
}

////////////////////////////////////////////////////////////////////////////////////////

bool IMetaObjectRegisterData::UpdateCurrentRecords(const wxString& tableName, IMetaObjectRegisterData* dst)
{
	if (HasRecorder()) {
		CMetaObjectAttributePredefined* metaRec = dst->GetRegisterRecorder();
		if (metaRec == nullptr)
			return false;
		const CTypeDescription& typeDesc = metaRec->GetTypeDesc();
		for (auto& clsid : typeDesc.GetClsidList()) {
			if (!(*m_propertyAttributeRecorder)->ContainType(clsid)) {
				int retCode = DATABASE_LAYER_QUERY_RESULT_ERROR; wxString clsStr; clsStr << wxLongLong_t(clsid);
				retCode = db_query->RunQuery("DELETE FROM %s WHERE %s_RTRef = " + clsStr, tableName, metaRec->GetFieldNameDB());
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return false;
			}
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////

bool IMetaObjectRegisterData::CreateAndUpdateTableDB(IMetaDataConfiguration* srcMetaData, IMetaObject* srcMetaObject, int flags)
{
	const wxString& tableName = GetTableNameDB();

	int retCode = 1;

	if ((flags & createMetaTable) != 0) {

		s_restructureInfo.AppendInfo(_("Append register ") + GetFullName());

		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
			retCode = db_query->RunQuery("CREATE TABLE %s (rowData BYTEA);", tableName);
		else
			retCode = db_query->RunQuery("CREATE TABLE %s (rowData BLOB);", tableName);

		for (const auto object : GetPredefinedAttributeArrayObject()) {
			if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
				return false;
			retCode = ProcessAttribute(tableName,
				object, nullptr);
		}

		if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
			return false;

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

		if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
			return false;

		for (const auto object : GetAttributeArrayObject()) {
			if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
				return false;
			retCode = ProcessAttribute(tableName,
				object, nullptr);
		}

		if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
			return false;

		wxString queryText;
		if (HasRecorder()) {
			CMetaObjectAttributePredefined* attributeRecorder = GetRegisterRecorder();
			wxASSERT(attributeRecorder);
			queryText += IMetaObjectAttribute::GetSQLFieldName(attributeRecorder);
			CMetaObjectAttributePredefined* attributeNumberLine = GetRegisterLineNumber();
			wxASSERT(attributeNumberLine);
			queryText += "," + IMetaObjectAttribute::GetSQLFieldName(attributeNumberLine);
		}
		else {
			bool firstMatching = true;
			for (const auto object : GetGenericDimentionArrayObject()) {
				queryText += (firstMatching ? "" : ",") + IMetaObjectAttribute::GetSQLFieldName(object);
				if (firstMatching) {
					firstMatching = false;
				}
			}
		}

		if (!queryText.IsEmpty()) {
			retCode = db_query->RunQuery(
				wxT("CREATE INDEX %s_INDEX ON %s (") + queryText + wxT(");"), tableName, tableName);
		}
	}
	else if ((flags & updateMetaTable) != 0) {

		//if src is null then delete
		IMetaObjectRegisterData* dstValue = nullptr;
		if (srcMetaObject->ConvertToValue(dstValue)) {

			if (!UpdateCurrentRecords(tableName, dstValue))
				return false;

			if (!dstValue->CompareObject(this))
				s_restructureInfo.AppendInfo(_("Changed register ") + GetFullName());

			//attributes from dst 
			for (const auto object : dstValue->GetPredefinedAttributeArrayObject()) {
				IMetaObject* foundedMeta =
					IMetaObjectRegisterData::FindPredefinedAttributeObjectByFilter(object->GetGuid());
				if (foundedMeta == nullptr) {
					retCode = ProcessAttribute(tableName, nullptr, object);
					if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
						return false;
				}
			}

			//attributes current
			for (const auto object : GetPredefinedAttributeArrayObject()) {
				retCode = ProcessAttribute(tableName,
					object, dstValue->FindPredefinedAttributeObjectByFilter(object->GetGuid())
				);
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return false;
			}

			//dimensions from dst 
			for (const auto object : dstValue->GetDimentionArrayObject()) {
				IMetaObject* foundedMeta =
					IMetaObjectRegisterData::FindDimensionObjectByFilter(object->GetGuid());
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
				IMetaObject* foundedMeta =
					IMetaObjectRegisterData::FindResourceObjectByFilter(object->GetGuid());
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

			//attributes from dst 
			for (const auto object : dstValue->GetAttributeArrayObject()) {
				IMetaObject* foundedMeta =
					IMetaObjectRegisterData::FindAttributeObjectByFilter(object->GetGuid());
				if (foundedMeta == nullptr) {
					retCode = ProcessAttribute(tableName, nullptr, object);
					if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
						return false;
				}
			}

			//attributes current
			for (const auto object : GetAttributeArrayObject()) {
				retCode = ProcessAttribute(tableName,
					object, dstValue->FindAttributeObjectByFilter(object->GetGuid())
				);
				if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
					return false;
			}
		}
	}
	else if ((flags & deleteMetaTable) != 0) {

		s_restructureInfo.AppendInfo(_("Remove register ") + GetFullName());

		retCode = db_query->RunQuery("DROP TABLE %s", tableName);
	}

	if (retCode == DATABASE_LAYER_QUERY_RESULT_ERROR)
		return false;

	return true;
}

//**********************************************************************************************************
//*                                          Query functions                                               *
//**********************************************************************************************************

#include "backend/system/systemManager.h"

bool IRecordDataObjectRef::ReadData()
{
	return ReadData(m_objGuid);
}

bool IRecordDataObjectRef::ReadData(const CGuid& srcGuid)
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	if (m_newObject && !srcGuid.isValid())
		return false;
	wxASSERT(m_metaObject);
	wxString tableName = m_metaObject->GetTableNameDB();
	if (db_query->TableExists(tableName)) {

		IDatabaseResultSet* resultSet = nullptr;
		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
			resultSet = db_query->RunQueryWithResults("SELECT * FROM " + tableName + " WHERE uuid = '" + srcGuid.str() + "' LIMIT 1;");
		else
			resultSet = db_query->RunQueryWithResults("SELECT FIRST 1 * FROM " + tableName + " WHERE uuid = '" + srcGuid.str() + "';");

		if (resultSet == nullptr) return false;
		bool succes = false;
		if (resultSet->Next()) {
			succes = true;
			//load other attributes 
			for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
				if (!m_metaObject->IsDataReference(object->GetMetaID())) {
					IMetaObjectAttribute::GetValueAttribute(object, m_listObjectValue[object->GetMetaID()], resultSet);
				}
			}
			for (const auto object : m_metaObject->GetTableArrayObject()) {
				CTabularSectionDataObjectRef* tabularSection = new CTabularSectionDataObjectRef(this, object);
				if (!tabularSection->LoadData(srcGuid)) succes = false;
				m_listObjectValue.insert_or_assign(object->GetMetaID(), tabularSection);
			}
		}
		db_query->CloseResultSet(resultSet);
		return succes;
	}
	return false;
}

bool IRecordDataObjectRef::SaveData()
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	//check fill attributes 
	bool fillCheck = true;
	wxASSERT(m_metaObject);
	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		if (object->FillCheck()) {
			if (m_listObjectValue[object->GetMetaID()].IsEmpty()) {
				wxString fillError =
					wxString::Format(_("""%s"" is a required field"), object->GetSynonym());
				CSystemFunction::Message(fillError, eStatusMessage::eStatusMessage_Information);
				fillCheck = false;
			}
		}
	}

	if (!fillCheck)
		return false;

	if (!IRecordDataObjectRef::DeleteData())
		return false;

	const wxString& tableName = m_metaObject->GetTableNameDB();
	wxString queryText = "INSERT INTO " + tableName + " (";
	queryText += "uuid";
	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		if (m_metaObject->IsDataReference(object->GetMetaID()))
			continue;
		queryText = queryText + ", " + IMetaObjectAttribute::GetSQLFieldName(object);
	}
	queryText += ") VALUES (?";
	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		if (m_metaObject->IsDataReference(object->GetMetaID()))
			continue;
		unsigned int fieldCount = IMetaObjectAttribute::GetSQLFieldCount(object);
		for (unsigned int i = 0; i < fieldCount; i++) {
			queryText += ", ?";
		}
	}
	queryText += ");";

	IPreparedStatement* statement = db_query->PrepareStatement(queryText);
	if (statement == nullptr)
		return false;

	m_objGuid = m_reference_impl->m_guid;
	statement->SetParamString(1, m_objGuid.str());

	int position = 2;

	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		if (m_metaObject->IsDataReference(object->GetMetaID()))
			continue;
		IMetaObjectAttribute::SetValueAttribute(
			object,
			m_listObjectValue.at(object->GetMetaID()),
			statement,
			position
		);
	}

	bool hasError =
		statement->RunQuery() == DATABASE_LAYER_QUERY_RESULT_ERROR;

	db_query->CloseStatement(statement);

	//table parts
	if (!hasError) {
		for (const auto object : m_metaObject->GetTableArrayObject()) {
			ITabularSectionDataObject* tabularSection = nullptr;
			if (m_listObjectValue[object->GetMetaID()].ConvertToValue(tabularSection)) {
				if (!tabularSection->SaveData()) {
					hasError = true;
					break;
				}
			}
			else if (m_listObjectValue[object->GetMetaID()].GetType() != TYPE_NULL) {
				hasError = true;
				break;
			}
		}
	}

	if (!hasError) {
		m_newObject = false;
	}

	return !hasError;
}

bool IRecordDataObjectRef::DeleteData()
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	if (m_newObject)
		return true;
	const wxString& tableName = m_metaObject->GetTableNameDB();
	//table parts
	for (const auto object : m_metaObject->GetTableArrayObject()) {
		ITabularSectionDataObject* tabularSection = nullptr;
		if (m_listObjectValue[object->GetMetaID()].ConvertToValue(tabularSection)) {
			if (!tabularSection->DeleteData())
				return false;
		}
		else if (m_listObjectValue[object->GetMetaID()].GetType() != TYPE_NULL)
			return false;

	}
	db_query->RunQuery("DELETE FROM " + tableName + " WHERE uuid = '" + m_objGuid.str() + "';");
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool IRecordDataObjectRef::IsSetUniqueIdentifier() const
{
	const auto object = m_metaObject->GetAttributeForCode();
	if (object != nullptr)
		return !m_listObjectValue.at(object->GetMetaID()).IsEmpty();
	return false;
}

bool IRecordDataObjectRef::GenerateUniqueIdentifier(const wxString& strPrefix)
{
	const auto object = m_metaObject->GetAttributeForCode();
	if (object != nullptr && !IsSetUniqueIdentifier()) {
		m_listObjectValue.insert_or_assign(object->GetMetaID(),
			GenerateNextIdentifier(object, strPrefix));
		return true;
	}
	return false;
}

bool IRecordDataObjectRef::ResetUniqueIdentifier()
{
	const auto object = m_metaObject->GetAttributeForCode();
	if (object != nullptr && IsSetUniqueIdentifier()) {
		m_listObjectValue.insert_or_assign(object->GetMetaID(), object->CreateValue());
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool IRecordDataObjectFolderRef::ReadData()
{
	if (IRecordDataObjectRef::ReadData()) {
		IMetaObjectRecordDataFolderMutableRef* metaFolder = GetMetaObject();
		wxASSERT(metaFolder);
		CValue isFolder; IRecordDataObjectFolderRef::GetValueByMetaID(*metaFolder->GetDataIsFolder(), isFolder);
		if (isFolder.GetBoolean())
			m_objMode = eObjectMode::OBJECT_FOLDER;
		else
			m_objMode = eObjectMode::OBJECT_ITEM;
		return true;
	}
	return false;
}

bool IRecordDataObjectFolderRef::ReadData(const CGuid& srcGuid)
{
	if (IRecordDataObjectRef::ReadData(srcGuid)) {
		IMetaObjectRecordDataFolderMutableRef* metaFolder = GetMetaObject();
		wxASSERT(metaFolder);
		CValue isFolder; IRecordDataObjectFolderRef::GetValueByMetaID(*metaFolder->GetDataIsFolder(), isFolder);
		if (isFolder.GetBoolean())
			m_objMode = eObjectMode::OBJECT_FOLDER;
		else
			m_objMode = eObjectMode::OBJECT_ITEM;
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void IRecordDataObjectRef::SetDeletionMark(bool deletionMark)
{
	if (m_newObject)
		return;

	if (m_metaObject != nullptr) {
		CMetaObjectAttributePredefined* attributeDeletionMark = m_metaObject->GetDataDeletionMark();
		wxASSERT(attributeDeletionMark);
		IRecordDataObjectRef::SetValueByMetaID(*attributeDeletionMark, deletionMark);
	}

	SaveModify();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool IRecordManagerObject::ExistData()
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	bool success = false;

	if (m_recordLine != nullptr) {

		db_query->BeginTransaction();

		wxString tableName = m_metaObject->GetTableNameDB(); int position = 1;
		wxString queryText = "SELECT * FROM " + tableName; bool firstWhere = true;

		for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
			if (firstWhere) {
				queryText = queryText + " WHERE ";
			}
			queryText = queryText +
				(firstWhere ? " " : " AND ") + IMetaObjectAttribute::GetCompositeSQLFieldName(object);
			if (firstWhere) {
				firstWhere = false;
			}
		}

		IPreparedStatement* statement = db_query->PrepareStatement(queryText);

		if (statement != nullptr) {
			for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
				CValue retValue; m_recordLine->GetValueByMetaID(object->GetMetaID(), retValue);
				IMetaObjectAttribute::SetValueAttribute(
					object,
					retValue,
					statement,
					position
				);
			}

			IDatabaseResultSet* resultSet = statement->RunQueryWithResults();
			if (resultSet != nullptr) {
				success = resultSet->Next();
				db_query->CloseResultSet(resultSet);
			}

			db_query->CloseStatement(statement);
		}

		db_query->Commit();
	}

	return success;
}

bool IRecordManagerObject::ReadData(const CUniquePairKey& key)
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	if (m_recordSet->ReadData(key)) {
		if (m_recordLine == nullptr) {
			m_recordLine = m_recordSet->GetRowAt(
				m_recordSet->GetItem(0)
			);
		}
		return true;
	}

	return false;
}

bool IRecordManagerObject::SaveData(bool replace)
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	if (m_recordSet->Selected()
		&& !DeleteData())
		return false;

	if (ExistData()) {
		wxString fillError =
			wxString::Format(_("This entry already exists. It is not possible to write a new value!"));
		CSystemFunction::Message(fillError, eStatusMessage::eStatusMessage_Information);
		return false;
	}

	m_recordSet->m_keyValues.clear();
	wxASSERT(m_recordLine);
	for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
		CValue retValue; m_recordLine->GetValueByMetaID(object->GetMetaID(), retValue);
		m_recordSet->m_keyValues.insert_or_assign(
			object->GetMetaID(), retValue
		);
	}
	if (m_recordSet->WriteRecordSet(replace, false)) {
		m_objGuid.SetKeyPair(m_metaObject, m_recordSet->m_keyValues);
		return true;
	}
	return false;
}

bool IRecordManagerObject::DeleteData()
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));
	return m_recordSet->DeleteRecordSet();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool IRecordSetObject::ExistData()
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	wxString tableName = m_metaObject->GetTableNameDB(); int position = 1;
	wxString queryText = "SELECT * FROM " + tableName; bool firstWhere = true;

	for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
		if (!IRecordSetObject::FindKeyValue(object->GetMetaID()))
			continue;
		if (firstWhere) {
			queryText = queryText + " WHERE ";
		}
		queryText = queryText +
			(firstWhere ? " " : " AND ") + IMetaObjectAttribute::GetCompositeSQLFieldName(object);
		if (firstWhere) {
			firstWhere = false;
		}
	}

	IPreparedStatement* statement = db_query->PrepareStatement(queryText);
	if (statement == nullptr)
		return false;

	for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
		if (!IRecordSetObject::FindKeyValue(object->GetMetaID()))
			continue;
		IMetaObjectAttribute::SetValueAttribute(
			object,
			m_keyValues.at(object->GetMetaID()),
			statement,
			position
		);
	}

	IDatabaseResultSet* resultSet = statement->RunQueryWithResults();
	if (resultSet == nullptr)
		return false;
	bool founded = false;
	if (resultSet->Next())
		founded = true;
	db_query->CloseResultSet(resultSet);
	db_query->CloseStatement(statement);
	return founded;
}

bool IRecordSetObject::ExistData(number_t& lastNum)
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	wxString tableName = m_metaObject->GetTableNameDB(); int position = 1;
	wxString queryText = "SELECT * FROM " + tableName; bool firstWhere = true;

	for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
		if (!IRecordSetObject::FindKeyValue(object->GetMetaID()))
			continue;
		if (firstWhere) {
			queryText = queryText + " WHERE ";
		}
		queryText = queryText +
			(firstWhere ? " " : " AND ") + IMetaObjectAttribute::GetCompositeSQLFieldName(object);
		if (firstWhere) {
			firstWhere = false;
		}
	}

	IPreparedStatement* statement = db_query->PrepareStatement(queryText);
	if (statement == nullptr)
		return false;

	for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
		if (!IRecordSetObject::FindKeyValue(object->GetMetaID()))
			continue;
		IMetaObjectAttribute::SetValueAttribute(
			object,
			m_keyValues.at(object->GetMetaID()),
			statement,
			position
		);
	}

	IDatabaseResultSet* resultSet = statement->RunQueryWithResults();
	if (resultSet == nullptr)
		return false;
	bool founded = false; lastNum = 1;
	while (resultSet->Next()) {
		CValue numLine; IMetaObjectAttribute::GetValueAttribute(m_metaObject->GetRegisterLineNumber(), numLine, resultSet);
		if (numLine > lastNum) {
			lastNum = numLine.GetNumber();
		}
		founded = true;
	}
	db_query->CloseResultSet(resultSet);
	db_query->CloseStatement(statement);
	return founded;
}

bool IRecordSetObject::ReadData(const CUniquePairKey& key)
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	IValueTable::Clear(); int position = 1;

	wxString tableName = m_metaObject->GetTableNameDB();
	wxString queryText = "SELECT * FROM " + tableName; bool firstWhere = true;

	for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
		if (!key.FindKey(object->GetMetaID()))
			continue;
		if (firstWhere) {
			queryText = queryText + " WHERE ";
		}
		queryText = queryText +
			(firstWhere ? " " : " AND ") + IMetaObjectAttribute::GetCompositeSQLFieldName(object);
		if (firstWhere) {
			firstWhere = false;
		}
	}
	IPreparedStatement* statement = db_query->PrepareStatement(queryText);
	if (statement == nullptr)
		return false;
	for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
		if (!key.FindKey(object->GetMetaID()))
			continue;
		IMetaObjectAttribute::SetValueAttribute(
			object,
			key.GetKey(object->GetMetaID()),
			statement,
			position
		);
	}
	IDatabaseResultSet* resultSet = statement->RunQueryWithResults();
	if (resultSet == nullptr)
		return false;
	while (resultSet->Next()) {
		wxValueTableRow* rowData = new wxValueTableRow();
		for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
			IMetaObjectAttribute::GetValueAttribute(object, rowData->AppendTableValue(object->GetMetaID()), resultSet);
		}
		for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
			IMetaObjectAttribute::GetValueAttribute(object, rowData->AppendTableValue(object->GetMetaID()), resultSet);
		}
		IValueTable::Append(rowData, !CBackendException::IsEvalMode());
		m_selected = true;
	}

	resultSet->Close();
	db_query->CloseResultSet(resultSet);
	db_query->CloseStatement(statement);
	return GetRowCount() > 0;
}

bool IRecordSetObject::ReadData()
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	IValueTable::Clear(); int position = 1;

	wxString tableName = m_metaObject->GetTableNameDB();
	wxString queryText = "SELECT * FROM " + tableName; bool firstWhere = true;

	for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
		if (!IRecordSetObject::FindKeyValue(object->GetMetaID()))
			continue;
		if (firstWhere) {
			queryText = queryText + " WHERE ";
		}
		queryText = queryText +
			(firstWhere ? " " : " AND ") + IMetaObjectAttribute::GetCompositeSQLFieldName(object);
		if (firstWhere) {
			firstWhere = false;
		}
	}
	IPreparedStatement* statement = db_query->PrepareStatement(queryText);
	if (statement == nullptr)
		return false;
	for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
		if (!IRecordSetObject::FindKeyValue(object->GetMetaID()))
			continue;
		IMetaObjectAttribute::SetValueAttribute(
			object,
			m_keyValues.at(object->GetMetaID()),
			statement,
			position
		);
	}
	IDatabaseResultSet* resultSet = statement->RunQueryWithResults();
	if (resultSet == nullptr)
		return false;
	while (resultSet->Next()) {
		wxValueTableRow* rowData = new wxValueTableRow();
		for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
			IMetaObjectAttribute::GetValueAttribute(object, rowData->AppendTableValue(object->GetMetaID()), resultSet);
		}
		for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
			IMetaObjectAttribute::GetValueAttribute(object, rowData->AppendTableValue(object->GetMetaID()), resultSet);
		}
		IValueTable::Append(rowData, !CBackendException::IsEvalMode());
		m_selected = true;
	}

	resultSet->Close();
	db_query->CloseResultSet(resultSet);
	db_query->CloseStatement(statement);
	return GetRowCount() > 0;
}

bool IRecordSetObject::SaveData(bool replace, bool clearTable)
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	//check fill attributes 
	bool fillCheck = true; long currLine = 1;
	for (long row = 0; row < GetRowCount(); row++) {
		for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
			if (object->FillCheck()) {
				wxValueTableRow* node = GetViewData<wxValueTableRow>(GetItem(row));
				wxASSERT(node);
				if (node->IsEmptyValue(object->GetMetaID())) {
					wxString fillError =
						wxString::Format(_("The %s is required on line %i of the %s"), object->GetSynonym(), currLine, m_metaObject->GetSynonym());
					CSystemFunction::Message(fillError, eStatusMessage::eStatusMessage_Information);
					fillCheck = false;
				}
			}
		}
		currLine++;
	}

	if (!fillCheck)
		return false;

	number_t numberLine = 1, oldNumberLine = 1;

	if (m_metaObject->HasRecorder() &&
		IRecordSetObject::ExistData(oldNumberLine)) {
		if (replace && !IRecordSetObject::DeleteData())
			return false;
		if (!replace) {
			numberLine = oldNumberLine;
		}
	}
	else if (IRecordSetObject::ExistData()) {
		if (replace && !IRecordSetObject::DeleteData())
			return false;
		if (!replace) {
			numberLine = oldNumberLine;
		}
	}

	wxString tableName = m_metaObject->GetTableNameDB(); wxString queryText; bool firstUpdate = true;
	if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL) {
		queryText = "INSERT INTO " + tableName + " (";
	}
	else {
		queryText = "UPDATE OR INSERT INTO " + tableName + " (";
	}
	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		queryText += (firstUpdate ? "" : ",") + IMetaObjectAttribute::GetSQLFieldName(object);
		if (firstUpdate) {
			firstUpdate = false;
		}
	}
	queryText += ") VALUES ("; bool firstInsert = true;
	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		unsigned int fieldCount = IMetaObjectAttribute::GetSQLFieldCount(object);
		for (unsigned int i = 0; i < fieldCount; i++) {
			queryText += (firstInsert ? "?" : ",?");
			if (firstInsert) {
				firstInsert = false;
			}
		}
	}

	if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL) {
		queryText += ")";
	}
	else {
		queryText += ") MATCHING (";
		if (m_metaObject->HasRecorder()) {
			CMetaObjectAttributePredefined* attributeRecorder = m_metaObject->GetRegisterRecorder();
			wxASSERT(attributeRecorder);
			queryText += IMetaObjectAttribute::GetSQLFieldName(attributeRecorder);
			CMetaObjectAttributePredefined* attributeNumberLine = m_metaObject->GetRegisterLineNumber();
			wxASSERT(attributeNumberLine);
			queryText += "," + IMetaObjectAttribute::GetSQLFieldName(attributeNumberLine);
		}
		else
		{
			bool firstMatching = true;
			for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
				queryText += (firstMatching ? "" : ",") + IMetaObjectAttribute::GetSQLFieldName(object);
				if (firstMatching) {
					firstMatching = false;
				}
			}
		}
		queryText += ");";
	}

	IPreparedStatement* statement = db_query->PrepareStatement(queryText);
	if (statement == nullptr)
		return false;

	bool hasError = false;

	for (long row = 0; row < GetRowCount(); row++) {
		if (hasError)
			break;
		int position = 1;
		for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
			auto foundedKey = m_keyValues.find(object->GetMetaID());
			if (foundedKey != m_keyValues.end()) {
				IMetaObjectAttribute::SetValueAttribute(
					object,
					foundedKey->second,
					statement,
					position
				);
			}
			else if (m_metaObject->IsRegisterLineNumber(object->GetMetaID())) {
				IMetaObjectAttribute::SetValueAttribute(
					object,
					numberLine++,
					statement,
					position
				);
			}
			else {
				wxValueTableRow* node = GetViewData< wxValueTableRow>(GetItem(row));
				wxASSERT(node);
				IMetaObjectAttribute::SetValueAttribute(
					object,
					node->GetTableValue(object->GetMetaID()),
					statement,
					position
				);
			}
		}

		hasError = statement->RunQuery() == DATABASE_LAYER_QUERY_RESULT_ERROR;
	}

	db_query->CloseStatement(statement);

	if (!hasError && !SaveVirtualTable())
		return false;

	if (!hasError && clearTable)
		IValueTable::Clear();
	else if (!clearTable)
		m_selected = true;

	return !hasError;
}

bool IRecordSetObject::DeleteData()
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	wxString tableName = m_metaObject->GetTableNameDB();
	wxString queryText = "DELETE FROM " + tableName; bool firstWhere = true;
	for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
		if (!IRecordSetObject::FindKeyValue(object->GetMetaID()))
			continue;
		if (firstWhere) {
			queryText = queryText + " WHERE ";
		}
		queryText = queryText +
			(firstWhere ? " " : " AND ") + IMetaObjectAttribute::GetCompositeSQLFieldName(object);
		if (firstWhere) {
			firstWhere = false;
		}
	}

	IPreparedStatement* statement = db_query->PrepareStatement(queryText); int position = 1;

	if (statement == nullptr)
		return false;

	for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
		if (!IRecordSetObject::FindKeyValue(object->GetMetaID()))
			continue;
		IMetaObjectAttribute::SetValueAttribute(
			object,
			m_keyValues.at(object->GetMetaID()),
			statement,
			position
		);
	}

	statement->RunQuery();
	db_query->CloseStatement(statement);
	return DeleteVirtualTable();
}

//**********************************************************************************************************
//*                                          Code generator												   *
//**********************************************************************************************************

CValue IRecordDataObjectRef::GenerateNextIdentifier(IMetaObjectAttribute* attribute, const wxString& strPrefix)
{
	if (db_query != nullptr && !db_query->IsOpen())
		CBackendCoreException::Error(_("Database is not open!"));
	else if (db_query == nullptr)
		CBackendCoreException::Error(_("Database is not open!"));

	wxASSERT(attribute);

	number_t resultCode = 1;

	const CTypeDescription& typeDesc = attribute->GetTypeDesc();

	if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL) {

		IDatabaseResultSet* resultSet = db_query->RunQueryWithResults(
			wxT("SELECT meta_guid, prefix, number FROM %s WHERE meta_guid = '%s' AND prefix = '%s' FOR UPDATE;"),
			sequence_table,
			m_metaObject->GetDocPath(),
			strPrefix
		);

		if (resultSet == nullptr)
			return attribute->CreateValue();

		if (resultSet->Next())
			resultCode = resultSet->GetResultNumber(wxT("number")) + 1;

		db_query->RunQuery(
			wxT("INSERT INTO %s (meta_guid, prefix, number) VALUES ('%s', '%s', %s) ON CONFLICT(meta_guid, prefix) DO UPDATE SET meta_guid = excluded.meta_guid, prefix = excluded.prefix, number = excluded.number;"),
			sequence_table,
			m_metaObject->GetDocPath(),
			strPrefix, //prefix
			resultCode.ToString()
		);

		db_query->CloseResultSet(resultSet);
	}
	else {

		IDatabaseResultSet* resultSet = db_query->RunQueryWithResults(
			wxT("SELECT meta_guid, prefix, number FROM %s WHERE meta_guid = '%s' AND prefix = '%s' FOR UPDATE WITH LOCK;"),
			sequence_table,
			m_metaObject->GetDocPath(),
			strPrefix
		);

		if (resultSet == nullptr)
			return attribute->CreateValue();

		if (resultSet->Next())
			resultCode = resultSet->GetResultNumber(wxT("number")) + 1;

		db_query->RunQuery(
			wxT("UPDATE OR INSERT INTO %s (meta_guid, prefix, number) VALUES ('%s', '%s', %s) MATCHING (meta_guid, prefix);"),
			sequence_table,
			m_metaObject->GetDocPath(),
			strPrefix, //prefix
			resultCode.ToString()
		);

		db_query->CloseResultSet(resultSet);
	}

	if (attribute->ContainType(eValueTypes::TYPE_NUMBER)) {
		return resultCode;
	}
	else if (attribute->ContainType(eValueTypes::TYPE_STRING)) {

		wxString strNumber;

		std::stringstream strStreamNumber;
		if (strPrefix.Length() < typeDesc.GetLength()) {
			strStreamNumber << strPrefix <<
				std::setw(typeDesc.GetLength() - strPrefix.Length()) << std::setfill('0') << resultCode;
		}
		else {
			strStreamNumber <<
				std::setw(strPrefix.Length()) << std::setfill('0') << resultCode;
		}

		strNumber = strStreamNumber.str();
		return strNumber;
	}

	wxASSERT_MSG(false, "m_metaAttribute->GetClsidList() != eValueTypes::TYPE_NUMBER"
		"|| m_metaAttribute->GetClsidList() != eValueTypes::TYPE_STRING");

	return wxEmptyValue;
}