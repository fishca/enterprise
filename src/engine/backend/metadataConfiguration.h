#ifndef _CONFIG_METADATA_H__
#define _CONFIG_METADATA_H__

#include "backend/metadata.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
#define activeMetaData		(IMetaDataConfiguration::Get())
//////////////////////////////////////////////////////////////////////////////////////////////////////
#define metaDataCreate(mode, f) (IMetaDataConfiguration::Initialize(mode, f))
#define metaDataDestroy()		(IMetaDataConfiguration::Destroy())
//////////////////////////////////////////////////////////////////////////////////////////////////////

enum eConfigType {
	eConfigType_File,
	eConfigType_Load,
	eConfigType_Load_And_Save,
};

class BACKEND_API IMetaDataConfiguration : public IMetaData {
public:

#pragma region access
	virtual bool AccessRight_Administration() const { return true; }
	virtual bool AccessRight_DataAdministration() const { return true; }
	virtual bool AccessRight_UpdateDatabaseConfiguration() const { return true; }
	virtual bool AccessRight_ActiveUsers() const { return true; }
	virtual bool AccessRight_ExclusiveMode() const { return true; }
	virtual bool AccessRight_ModeAllFunction() const { return true; }
#pragma endregion

	IMetaDataConfiguration() : IMetaData() {}

	virtual wxString GetConfigMD5() const = 0;
	virtual wxString GetConfigName() const = 0;

	virtual CGuid GetConfigGuid() const = 0;

	// if storage save in db 
	virtual bool IsConfigOpen() const { return false; }
	virtual bool IsConfigSave() const { return true; }

	virtual bool LoadDatabase(int flags = defaultFlag) { return true; }
	virtual bool SaveDatabase(int flags = defaultFlag) { return true; }

	//rollback to config db
	virtual bool RollbackDatabase() { return true; }

	//load/save config form buffer
	virtual bool LoadConfigFromBuffer(const wxMemoryBuffer& buffer) { return true; }
	virtual bool SaveConfigToBuffer(wxMemoryBuffer& buffer) { return true; }

	//load/save config form buffer
	virtual bool LoadDataFromBuffer(const wxMemoryBuffer& buffer) { return true; }
	virtual bool SaveDataToBuffer(wxMemoryBuffer& buffer) { return true; }

	//get common module 
	virtual CValueModuleManagerConfiguration* GetModuleManager() const = 0;
	virtual CValueMetaObjectConfiguration* GetCommonMetaObject() const = 0;

	//start/exit module 
	virtual bool StartMainModule(bool force = false) = 0;
	virtual bool ExitMainModule(bool force = false) = 0;

	// get config metadata in storage 
	virtual IMetaDataConfiguration* GetConfiguration() const { return nullptr; }

	//get config type 
	virtual eConfigType GetConfigType() const = 0;

	//special delete and create 
	virtual bool ReCreateDatabase() { return false; }

	//special save 
	virtual bool OnBeforeSaveDatabase(int flags) { return false; }
	virtual bool OnSaveDatabase(int flags) { return false; }
	virtual bool OnAfterSaveDatabase(bool roolback, int flags) { return false; }

	//load/save form file
	bool LoadConfigFromFile(const wxString& strFileName);
	bool SaveConfigToFile(const wxString& strFileName);

protected:

	virtual bool OnInitialize(const int flag) { return true; }
	virtual bool OnDestroy() { return true; }

public:

	static IMetaDataConfiguration* Get() { return ms_instance; }
	static bool Initialize(enum eRunMode mode, const int flag);
	static bool Destroy();

private:
	static IMetaDataConfiguration* ms_instance;
};

class BACKEND_API CMetaDataConfigurationFile : public IMetaDataConfiguration {
public:

#pragma region access
	virtual bool AccessRight_Administration() const { return m_commonObject->AccessRight_Administration(); }
	virtual bool AccessRight_DataAdministration() const { return m_commonObject->AccessRight_DataAdministration(); }
	virtual bool AccessRight_UpdateDatabaseConfiguration() const { return m_commonObject->AccessRight_UpdateDatabaseConfiguration(); }
	virtual bool AccessRight_ActiveUsers() const { return m_commonObject->AccessRight_ActiveUsers(); }
	virtual bool AccessRight_ExclusiveMode() const { return m_commonObject->AccessRight_ExclusiveMode(); }
	virtual bool AccessRight_ModeAllFunction() const { return m_commonObject->AccessRight_ModeAllFunction(); }
#pragma endregion

	virtual bool IsConfigOpen() const { return m_configOpened; }

	CMetaDataConfigurationFile();
	virtual ~CMetaDataConfigurationFile();

	virtual wxString GetConfigMD5() const { return m_md5Hash; }
	virtual wxString GetConfigName() const { return m_commonObject->GetName(); }

	virtual CGuid GetConfigGuid() const { return m_commonObject->GetDocPath(); }

	virtual void SetVersion(const version_identifier_t& version) { m_commonObject->SetVersion(version); }
	virtual version_identifier_t GetVersion() const { return m_commonObject->GetVersion(); }

	//compare metaData
	virtual bool CompareMetadata(CMetaDataConfigurationFile* dst) const {
		return m_md5Hash == dst->m_md5Hash;
	}

	//get language code 
	virtual wxString GetLangCode() const;

	//Check is full access 
	virtual bool IsFullAccess() const;

	//run/close 
	virtual bool RunDatabase(int flags = defaultFlag);
	virtual bool CloseDatabase(int flags = defaultFlag);

	virtual bool ClearDatabase();

	//load/save form file
	virtual bool LoadConfigFromBuffer(const wxMemoryBuffer& buffer);

	virtual CValueModuleManagerConfiguration* GetModuleManager() const { return m_moduleManager; }
	virtual CValueMetaObjectConfiguration* GetCommonMetaObject() const { return m_commonObject; }

	//start/exit module 
	virtual bool StartMainModule(bool force = false) {
		return m_moduleManager != nullptr ?
			m_moduleManager->StartMainModule() : false;
	}

	virtual bool ExitMainModule(bool force = false) {
		return m_moduleManager != nullptr ?
			m_moduleManager->ExitMainModule(force) : false;
	}

	//get config type 
	virtual eConfigType GetConfigType() const { return eConfigType::eConfigType_File; };

protected:

	//header loader/saver 
	bool LoadHeader(CMemoryReader& readerData);

	//loader/saver/deleter: 
	bool LoadCommonMetadata(const class_identifier_t& clsid, CMemoryReader& readerData);
	bool LoadDatabase(const class_identifier_t& clsid, CMemoryReader& readerData, IValueMetaObject* object);
	bool LoadChildMetadata(const class_identifier_t& clsid, CMemoryReader& readerData, IValueMetaObject* object);

	//run/close recursively:
	bool RunChildMetadata(IValueMetaObject* object, int flags, bool before);
	bool CloseChildMetadata(IValueMetaObject* object, int flags, bool before);

	//clear recursively:
	bool ClearChildMetadata(IValueMetaObject* object);

protected:

	bool m_configOpened;
	wxString m_md5Hash;
	//common meta object
	CValueMetaObjectConfiguration* m_commonObject;
	CValueModuleManagerConfiguration* m_moduleManager;
};

class BACKEND_API CMetaDataConfiguration : public CMetaDataConfigurationFile {
public:
	CMetaDataConfiguration();
	virtual bool LoadConfigFromFile(const wxString& strFileName) {
		if (CMetaDataConfigurationFile::LoadConfigFromFile(strFileName)) {
			Modify(true); //set modify for check metaData
			return RunDatabase();
		}
		return false;
	}

	virtual wxString GetConfigName() const { return m_commonObject->GetName(); }
	virtual CGuid GetConfigGuid() const { return m_metaGuid; }

	//metaData 
	virtual bool LoadDatabase(int flags = defaultFlag);

	//get config type 
	virtual eConfigType GetConfigType() const { return eConfigType::eConfigType_Load; };

protected:

	virtual bool OnInitialize(const int flag);
	virtual bool OnDestroy();

protected:

	CGuid m_metaGuid;
	bool m_configNew;
};

class BACKEND_API CMetaDataConfigurationStorage : public CMetaDataConfiguration {

	struct CSequenceData {
		int m_interval;
		wxString m_strGuid;
		wxString m_strPrefix;
		int m_number;
	};

public:

	CMetaDataConfigurationStorage();
	virtual ~CMetaDataConfigurationStorage();

	//is config save
	virtual bool IsConfigSave() const {
		return CompareMetadata(m_configMetadata);
	}

	//metadata  
	virtual bool LoadDatabase(int flags = defaultFlag);
	virtual bool SaveDatabase(int flags = defaultFlag);

	//run/close 
	virtual bool RunDatabase(int flags = defaultFlag) {
		if (!CMetaDataConfiguration::RunDatabase(flags))
			return false;
		return m_configMetadata->RunDatabase(flags | loadConfigFlag);
	}

	virtual bool CloseDatabase(int flags = defaultFlag) {
		if (!CMetaDataConfiguration::CloseDatabase(flags))
			return false;
		return m_configMetadata->CloseDatabase(flags);
	}

	//Check is full access 
	virtual bool IsFullAccess() const { return true; }

	//rollback to config db
	virtual bool RollbackDatabase();

	//save form file
	virtual bool SaveConfigToBuffer(wxMemoryBuffer& buffer);

	//load/save config form buffer
	virtual bool LoadDataFromBuffer(const wxMemoryBuffer& buffer);
	virtual bool SaveDataToBuffer(wxMemoryBuffer& buffer);

	// get config metaData 
	virtual IMetaDataConfiguration* GetConfiguration() const { return m_configMetadata; }

	//get config type 
	virtual eConfigType GetConfigType() const { return eConfigType::eConfigType_Load_And_Save; };

	////////////////////////////////////////////////////////////////

	static bool TableAlreadyCreated();

	static void CreateConfigTable();
	static void CreateConfigSaveTable();
	
	static void CreateConfigSequence();

	static void ResetSequence();

	////////////////////////////////////////////////////////////////

	//special save 
	virtual bool OnBeforeSaveDatabase(int flags);
	virtual bool OnSaveDatabase(int flags);
	virtual bool OnAfterSaveDatabase(bool roolback, int flags);

	//special clear 
	virtual bool ReCreateDatabase();

protected:

	virtual bool OnInitialize(const int flag);
	virtual bool OnDestroy();

	//header saver 
	bool SaveHeader(CMemoryWriter& writerData);

	//loader/saver/deleter: 
	bool SaveCommonMetadata(const class_identifier_t& clsid, CMemoryWriter& writerData, int flags = defaultFlag);
	bool SaveDatabase(const class_identifier_t& clsid, CMemoryWriter& writerData, int flags = defaultFlag);
	bool SaveChildMetadata(const class_identifier_t& clsid, CMemoryWriter& writerData, IValueMetaObject* object, int flags = defaultFlag);
	bool DeleteCommonMetadata(const class_identifier_t& clsid);
	bool DeleteMetadata(const class_identifier_t& clsid);
	bool DeleteChildMetadata(const class_identifier_t& clsid, IValueMetaObject* object);

private:

	//load/save sequence form buffer
	bool LoadSequenceFromBuffer(const CMemoryReader& reader);
	bool SaveSequenceToBuffer(CMemoryWriter& writer);

	CMetaDataConfiguration* m_configMetadata;
};

#define sign_metadata 0x1236F362122FE

#endif