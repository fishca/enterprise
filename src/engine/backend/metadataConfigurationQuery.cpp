#include "metadataConfiguration.h"

#include "backend/databaseLayer/databaseLayer.h"
#include "backend/databaseLayer/databaseErrorCodes.h"

#include "backend/utils/wxmd5.hpp"
#include "backend/appData.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
#include <wx/base64.h>
//////////////////////////////////////////////////////////////////////////////////////////////////////
#define config_table	  wxT("sys_config")
#define config_save_table wxT("sys_config_save")
//////////////////////////////////////////////////////////////////////////////////////////////////////
#define config_name       wxT("sys.database")
//////////////////////////////////////////////////////////////////////////////////////////////////////

inline wxString GetCommonConfigTable(eConfigType cfg_type) {

	switch (cfg_type)
	{
	case eConfigType::eConfigType_Load:
		return config_table;
	case eConfigType::eConfigType_Load_And_Save:
		return config_save_table;
	}

	return wxEmptyString;
}

//**************************************************************************************************
//*                                          ConfigMetadata                                        *
//**************************************************************************************************

bool CMetaDataConfiguration::LoadDatabase(int flags)
{
	//close if opened
	if (CMetaDataConfiguration::IsConfigOpen()) {
		if (!CloseDatabase(forceCloseFlag)) {
			return false;
		}
	}

	//clear data 
	if (!ClearDatabase()) {
		wxASSERT_MSG(false, "ClearDatabase() == false");
		return false;
	}

	// load config
	IDatabaseResultSet* resultSet = db_query->RunQueryWithResults("SELECT binary_data, file_guid FROM %s; ", GetCommonConfigTable(GetConfigType()));
	if (resultSet == nullptr)
		return false;

	//load metadata from DB 
	if (resultSet->Next()) {

		wxMemoryBuffer binaryData;
		resultSet->GetResultBlob(wxT("binary_data"), binaryData);
		CMemoryReader metaReader(binaryData.GetData(), binaryData.GetBufSize());

		//check is file empty
		if (metaReader.eof())
			return false;

		//check metadata 
		if (!LoadHeader(metaReader))
			return false;

		//load metadata 
		if (!LoadCommonMetadata(g_metaCommonMetadataCLSID, metaReader))
			return false;

		m_configNew = false;

		m_metaGuid = resultSet->GetResultString(wxT("file_guid"));
		m_md5Hash = wxMD5::ComputeMd5(wxBase64Encode(binaryData.GetData(), binaryData.GetDataLen()));
	}

	db_query->CloseResultSet(resultSet);
	return true;
}

//**************************************************************************************************
//*                                          ConfigSaveMetadata                                    *
//**************************************************************************************************

#include "metaCollection/partial/constant.h"

////////////////////////////////////////////////////////////////////////////////

bool CMetaDataConfigurationStorage::OnBeforeSaveDatabase(int flags)
{
	if (db_query->IsActiveTransaction())
		return false;

#if _USE_SAVE_METADATA_IN_TRANSACTION == 1	
	//begin transaction 
	db_query->BeginTransaction();
#endif 

	s_restructureInfo.ResetRestructureInfo();
	return db_query->IsActiveTransaction();
}

bool CMetaDataConfigurationStorage::OnSaveDatabase(int flags)
{
	if (!db_query->IsActiveTransaction())
		return false;

	//remove old tables (if need)
	if ((flags & saveConfigFlag) != 0) {

		//Delete common object
		if (!DeleteCommonMetadata(g_metaCommonMetadataCLSID)) {
#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
			db_query->RollBack(); return false;
#else 
			return false;
#endif
		}

		IMetaObject* commonObject = m_configMetadata->GetCommonMetaObject();
		wxASSERT(commonObject);

		for (unsigned int idx = 0; idx < commonObject->GetChildCount(); idx++) {

			auto child = commonObject->GetChild(idx);
			if (!commonObject->FilterChild(child->GetClassType()))
				continue;
			IMetaObject* foundedMeta =
				m_commonObject->FindChildByGuid(child->GetGuid());
			if (foundedMeta == nullptr) {
				bool ret = child->DeleteMetaTable(this);
				if (!ret) {
#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
					db_query->RollBack(); return false;
#else 
					return false;
#endif
				}
			}
		}

		//delete tables sql
		if (m_configNew) {

			s_restructureInfo.AppendInfo("Create new database");

			if (!CMetaObjectConstant::DeleteConstantSQLTable()) {
#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
				db_query->RollBack(); return false;
#else 
				return false;
#endif
			}

			if (!CMetaObjectConfiguration::ExecuteSystemSQLCommand()) {
#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
				db_query->RollBack(); return false;
#else 
				return false;
#endif
			}

			//create & update tables sql
			if (!CMetaObjectConstant::CreateConstantSQLTable()) {
#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
				db_query->RollBack(); return false;
#else 
				return false;
#endif
			}
		}

		for (unsigned int idx = 0; idx < m_commonObject->GetChildCount(); idx++) {

			auto child = m_commonObject->GetChild(idx);
			if (!m_commonObject->FilterChild(child->GetClassType()))
				continue;
			IMetaObject* foundedMeta =
				commonObject->FindChildByGuid(child->GetGuid());
			wxASSERT(child);
			bool ret = true;

			if (foundedMeta == nullptr) {
				ret = child->CreateMetaTable(m_configMetadata);
			}
			else {
				ret = child->UpdateMetaTable(m_configMetadata, foundedMeta);
			}

			if (!ret) {
#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
				db_query->RollBack(); return false;
#else
				return false;
#endif
			}
		}
	}

	//common data
	CMemoryWriter writterData;

	//Save header info 
	if (!SaveHeader(writterData)) {
#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
		db_query->RollBack(); return false;
#else 
		return false;
#endif
	}

	//Save common object
	if (!SaveCommonMetadata(g_metaCommonMetadataCLSID, writterData, flags)) {
#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
		db_query->RollBack(); return false;
#else 
		return false;
#endif
	}

	IPreparedStatement* prepStatement = nullptr;
	if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
		prepStatement = db_query->PrepareStatement("INSERT INTO %s (file_name, binary_data, file_guid) VALUES(?, ?, ?)"
			"ON CONFLICT (file_name) DO UPDATE SET file_name = EXCLUDED.file_name, binary_data = EXCLUDED.binary_data, file_guid = EXCLUDED.file_guid; ", config_save_table);
	else
		prepStatement = db_query->PrepareStatement("UPDATE OR INSERT INTO %s (file_name, binary_data, file_guid) VALUES(?, ?, ?) MATCHING (file_name); ", config_save_table);

	if (!prepStatement) return false;

	prepStatement->SetParamString(1, config_name);
	prepStatement->SetParamBlob(2, writterData.pointer(), writterData.size());
	prepStatement->SetParamString(3, m_metaGuid.str());

	if (prepStatement->RunQuery() == DATABASE_LAYER_QUERY_RESULT_ERROR) {
#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
		db_query->RollBack();
#else 
		return false;
#endif
	}

	if (!db_query->CloseStatement(prepStatement)) {
#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
		db_query->RollBack(); return false;
#else 
		return false;
#endif
	}

	m_md5Hash = wxMD5::ComputeMd5(
		wxBase64Encode(writterData.pointer(), writterData.size())
	);

	if ((flags & saveConfigFlag) != 0) {

		bool hasError =
			db_query->RunQuery("DELETE FROM %s;", config_table) == DATABASE_LAYER_QUERY_RESULT_ERROR;
		hasError = hasError ||
			db_query->RunQuery("INSERT INTO %s SELECT * FROM %s;", config_table, config_save_table) == DATABASE_LAYER_QUERY_RESULT_ERROR;
		if (hasError)
			return false;
	}

	return db_query->IsActiveTransaction();
}

bool CMetaDataConfigurationStorage::OnAfterSaveDatabase(bool roolback, int flags)
{
	//s_restructureInfo.ResetRestructureInfo();

	if (roolback) {

#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
		if (db_query->IsActiveTransaction())
			db_query->RollBack();
#endif 

		return !db_query->IsActiveTransaction();
	}
	else {

		if (!db_query->IsActiveTransaction())
			return false;

		if ((flags & saveConfigFlag) != 0) {
#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
			if (db_query->GetDatabaseLayerType() == DATABASELAYER_FIREBIRD) db_query->Commit();
#endif 

#if _USE_SAVE_METADATA_IN_TRANSACTION == 1	

			if (db_query->GetDatabaseLayerType() == DATABASELAYER_FIREBIRD) {

				CRestructureInfo restructureInfo;

				IMetaObject* commonObject = m_configMetadata->GetCommonMetaObject();
				wxASSERT(commonObject);
				for (unsigned int idx = 0; idx < m_commonObject->GetChildCount(); idx++) {
					auto child = m_commonObject->GetChild(idx);
					if (!m_commonObject->FilterChild(child->GetClassType()))
						continue;
					IMetaObject* foundedMeta =
						commonObject->FindChildByGuid(child->GetGuid());
					wxASSERT(child);
					if (foundedMeta == nullptr) {
						bool ret = child->CreateMetaTable(m_configMetadata, repairMetaTable);
						if (!ret) {
#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
							db_query->RollBack(); return false;
#else
							return false;
#endif
						}
					}
				}
			}
#endif

#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
			if (db_query->GetDatabaseLayerType() == DATABASELAYER_FIREBIRD) db_query->BeginTransaction();
#endif 
			if (!m_configMetadata->LoadDatabase(onlyLoadFlag)) {
#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
				db_query->RollBack();
#else 
				return false;
#endif
			}

			if (!m_configNew && !m_configMetadata->RunDatabase()) {
#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
				db_query->RollBack();
#else 
				return false;
#endif
			}

#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
			db_query->Commit();
#endif 
			Modify(false);
		}
		else {

			Modify(false);

#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
			db_query->Commit();
#endif 
			Modify(true);
		}

		return !db_query->IsActiveTransaction();
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool CMetaDataConfigurationStorage::SaveDatabase(int flags)
{
	if (OnBeforeSaveDatabase(flags)) {
		bool succes = OnSaveDatabase(flags);
		OnAfterSaveDatabase(!succes, flags);
		return succes;
	}

	return false;
}

bool CMetaDataConfigurationStorage::RoolbackDatabase()
{
	bool hasError = db_query->RunQuery("DELETE FROM %s;", config_save_table) == DATABASE_LAYER_QUERY_RESULT_ERROR;
	hasError = hasError || db_query->RunQuery("INSERT INTO %s SELECT * FROM %s;", config_save_table, config_table) == DATABASE_LAYER_QUERY_RESULT_ERROR;
	if (hasError) return false;
	//close data 
	if (CMetaDataConfiguration::IsConfigOpen()) {
		if (!CloseDatabase(forceCloseFlag)) {
			wxASSERT_MSG(false, "CloseDatabase() == false");
			return false;
		}
	}

	//clear data 
	if (!ClearDatabase()) {
		wxASSERT_MSG(false, "ClearDatabase() == false");
		return false;
	}

	if (LoadDatabase())
		return RunDatabase();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

bool CMetaDataConfigurationStorage::TableAlreadyCreated()
{
	return db_query->TableExists(config_table) &&
		db_query->TableExists(config_save_table);
}

void CMetaDataConfigurationStorage::CreateConfigTable() {

	//db for enterprise - TODO
	if (!db_query->TableExists(config_table)) {
		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL) {
			db_query->RunQuery("CREATE TABLE %s ("
				"file_name VARCHAR(128) NOT NULL PRIMARY KEY,"
				"file_guid uuid NOT NULL,"
				"binary_data BYTEA NOT NULL);", config_table);         	//size of binary medatadata
		}
		else
		{
			db_query->RunQuery("CREATE TABLE %s ("
				"file_name VARCHAR(128) NOT NULL PRIMARY KEY,"
				"file_guid VARCHAR(36) NOT NULL,"
				"binary_data BLOB NOT NULL);", config_table);         	//size of binary medatadata
		}

		db_query->RunQuery("CREATE INDEX s_config_index ON %s (file_name);", config_table);
	}
}

void CMetaDataConfigurationStorage::CreateConfigSaveTable() {

	//db for designer 
	if (!db_query->TableExists(config_save_table)) {
		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL) {
			db_query->RunQuery("CREATE TABLE %s ("
				"file_name VARCHAR(128) NOT NULL PRIMARY KEY,"
				"file_guid uuid NOT NULL,"
				"binary_data BYTEA NOT NULL);", config_save_table);         	//size of binary medatadata
		}
		else {
			db_query->RunQuery("CREATE TABLE %s ("
				"file_name VARCHAR(128) NOT NULL PRIMARY KEY,"
				"file_guid VARCHAR(36) NOT NULL,"
				"binary_data BLOB NOT NULL);", config_save_table);         	//size of binary medatadata
		}
		db_query->RunQuery("CREATE INDEX s_config_save_index ON %s (file_name);", config_save_table);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////