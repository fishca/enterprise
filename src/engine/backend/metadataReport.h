#include "backend/metadataConfiguration.h"
#include "backend/metaCollection/partial/dataReport.h"
#include "backend/moduleManager/moduleManagerExt.h"

#define sign_dataReport 0x2355F6421261D

class BACKEND_API ibMetaDataReport :
	public ibMetaData {
public:

	ibMetaDataReport();
	ibMetaDataReport(ibMetaData* metaData, ibValueMetaObjectReport* report = nullptr);
	virtual ~ibMetaDataReport();

	virtual ibValueMetaObjectReport* GetReport() const { return m_commonObject; }
	virtual ibValueModuleManagerExternalReport* GetModuleManager() const { return m_moduleManager; }

	virtual void SetVersion(const ibVersionID& version) { m_version = version; }
	virtual ibVersionID GetVersion() const { return m_version; }

	virtual wxString GetFileName() const { return m_fullPath; }

	//runtime support:
	virtual ibValue* CreateObjectRef(const ibClassID& clsid, ibValue** paParams = nullptr, const long lSizeArray = 0) const;
	virtual ibValue* CreateObjectRef(const wxString& className, ibValue** paParams = nullptr, const long lSizeArray = 0) const {
		return CreateObjectRef(
			GetIDObjectFromString(className), paParams, lSizeArray
		);
	}

	virtual bool IsRegisterCtor(const wxString& className) const;
	virtual bool IsRegisterCtor(const wxString& className, ibCtorObjectType objectType) const;
	virtual bool IsRegisterCtor(const wxString& className, ibCtorObjectType objectType, enum ibCtorObjectMetaType refType) const;

	virtual bool IsRegisterCtor(const ibClassID& clsid) const;

	virtual ibClassID GetIDObjectFromString(const wxString& className) const;
	virtual wxString GetNameObjectFromID(const ibClassID& clsid, bool upper = false) const;

	virtual ibCtorMetaValueType* GetTypeCtor(const ibClassID& clsid) const;
	virtual ibCtorMetaValueType* GetTypeCtor(const ibValueMetaObject* metaValue, enum ibCtorObjectMetaType refType) const;

	virtual ibCtorAbstractType* GetAvailableCtor(const wxString& className) const;
	virtual ibCtorAbstractType* GetAvailableCtor(const ibClassID& clsid) const;

	virtual std::vector<ibCtorMetaValueType*> GetListCtorsByType() const;
	virtual std::vector<ibCtorMetaValueType*> GetListCtorsByType(const ibClassID& clsid, ibCtorObjectMetaType refType) const;
	virtual std::vector<ibCtorMetaValueType*> GetListCtorsByType(enum ibCtorObjectMetaType refType) const;

	//Get owner metadata 
	virtual bool GetOwner(ibMetaData*& metaData) const;

	//factory version 
	virtual unsigned int GetFactoryCountChanges() const {
		return m_factoryCtorCountChanges +
			activeMetaData != nullptr ? activeMetaData->GetFactoryCountChanges() : 0;
	}

	//metaData 
	virtual bool LoadDatabase();
	virtual bool SaveDatabase();
	virtual bool ClearDatabase();

	//get language code 
	virtual wxString GetLangCode() const;

	//run/close 
	virtual bool RunDatabase(int flags = defaultFlag);
	virtual bool CloseDatabase(int flags = defaultFlag);

	//load/save form file
	bool LoadFromFile(const wxString& strFileName);
	bool SaveToFile(const wxString& strFileName);

	virtual ibValueMetaObject* GetCommonMetaObject() const { return m_commonObject; }

	//start/exit module
	virtual bool StartMainModule() { return m_moduleManager ? m_moduleManager->StartMainModule() : false; }
	virtual bool ExitMainModule(bool force = false) { return m_moduleManager ? m_moduleManager->ExitMainModule(force) : false; }

protected:

	//header loader/saver 
	bool LoadHeader(ibReaderMemory& readerData);
	bool SaveHeader(ibWriterMemory& writerData);

	//loader/saver/deleter: 
	bool LoadCommonMetadata(const ibClassID& clsid, ibReaderMemory& readerData);
	bool LoadChildMetadata(const ibClassID& clsid, ibReaderMemory& readerData, ibValueMetaObject* object);
	bool SaveCommonMetadata(const ibClassID& clsid, ibWriterMemory& writerData, int flags = defaultFlag);
	bool SaveChildMetadata(const ibClassID& clsid, ibWriterMemory& writerData, ibValueMetaObject* object, int flags = defaultFlag);
	bool DeleteCommonMetadata(const ibClassID& clsid);
	bool DeleteChildMetadata(const ibClassID& clsid, ibValueMetaObject* object);

	//run/close recursively:
	bool RunChildMetadata(ibValueMetaObject* object, int flags, bool before);
	bool CloseChildMetadata(ibValueMetaObject* object, int flags, bool before);
	bool ClearChildMetadata(ibValueMetaObject* object);

private:

	wxString m_fullPath;

	ibMetaData* m_ownerMeta; //owner for saving/loading
	ibValueModuleManagerExternalReport* m_moduleManager;
	ibValueMetaObjectReport* m_commonObject; 	//common meta object
	bool m_configOpened;

	ibVersionID m_version;
};