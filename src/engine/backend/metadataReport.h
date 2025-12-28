#include "backend/metadataConfiguration.h"
#include "backend/metaCollection/partial/dataReport.h"
#include "backend/moduleManager/moduleManagerExt.h"

#define sign_dataReport 0x2355F6421261D

class BACKEND_API CMetaDataReport :
	public IMetaData {
public:

	CMetaDataReport();
	CMetaDataReport(IMetaData* metaData, CMetaObjectReport* report = nullptr);
	virtual ~CMetaDataReport();

	virtual CMetaObjectReport* GetReport() const { return m_commonObject; }
	virtual CModuleManagerExternalReport* GetModuleManager() const { return m_moduleManager; }

	virtual void SetVersion(const version_identifier_t& version) { m_version = version; }
	virtual version_identifier_t GetVersion() const { return m_version; }

	virtual wxString GetFileName() const { return m_fullPath; }

	//runtime support:
	virtual CValue* CreateObjectRef(const class_identifier_t& clsid, CValue** paParams = nullptr, const long lSizeArray = 0) const;
	virtual CValue* CreateObjectRef(const wxString& className, CValue** paParams = nullptr, const long lSizeArray = 0) const {
		return CreateObjectRef(
			GetIDObjectFromString(className), paParams, lSizeArray
		);
	}

	virtual bool IsRegisterCtor(const wxString& className) const;
	virtual bool IsRegisterCtor(const wxString& className, eCtorObjectType objectType) const;
	virtual bool IsRegisterCtor(const wxString& className, eCtorObjectType objectType, enum eCtorMetaType refType) const;

	virtual bool IsRegisterCtor(const class_identifier_t& clsid) const;

	virtual class_identifier_t GetIDObjectFromString(const wxString& className) const;
	virtual wxString GetNameObjectFromID(const class_identifier_t& clsid, bool upper = false) const;

	virtual IMetaValueTypeCtor* GetTypeCtor(const class_identifier_t& clsid) const;
	virtual IMetaValueTypeCtor* GetTypeCtor(const IMetaObject* metaValue, enum eCtorMetaType refType) const;

	virtual IAbstractTypeCtor* GetAvailableCtor(const wxString& className) const;
	virtual IAbstractTypeCtor* GetAvailableCtor(const class_identifier_t& clsid) const;

	virtual std::vector<IMetaValueTypeCtor*> GetListCtorsByType() const;
	virtual std::vector<IMetaValueTypeCtor*> GetListCtorsByType(const class_identifier_t& clsid, eCtorMetaType refType) const;
	virtual std::vector<IMetaValueTypeCtor*> GetListCtorsByType(enum eCtorMetaType refType) const;

	//Get owner metadata 
	virtual bool GetOwner(IMetaData*& metaData) const;

	//factory version 
	virtual unsigned int GetFactoryCountChanges() const {
		return m_factoryCtorCountChanges +
			activeMetaData != nullptr ? activeMetaData->GetFactoryCountChanges() : 0;
	}

	//metaData 
	virtual bool LoadDatabase();
	virtual bool SaveDatabase();
	virtual bool ClearDatabase();

	//run/close 
	virtual bool RunDatabase(int flags = defaultFlag);
	virtual bool CloseDatabase(int flags = defaultFlag);

	//load/save form file
	bool LoadFromFile(const wxString& strFileName);
	bool SaveToFile(const wxString& strFileName);

	virtual IMetaObject* GetCommonMetaObject() const { return m_commonObject; }

	//start/exit module 
	virtual bool StartMainModule() { return m_moduleManager ? m_moduleManager->StartMainModule() : false; }
	virtual bool ExitMainModule(bool force = false) { return m_moduleManager ? m_moduleManager->ExitMainModule(force) : false; }

protected:

	//header loader/saver 
	bool LoadHeader(CMemoryReader& readerData);
	bool SaveHeader(CMemoryWriter& writterData);

	//loader/saver/deleter: 
	bool LoadCommonMetadata(const class_identifier_t& clsid, CMemoryReader& readerData);
	bool LoadChildMetadata(const class_identifier_t& clsid, CMemoryReader& readerData, IMetaObject* object);
	bool SaveCommonMetadata(const class_identifier_t& clsid, CMemoryWriter& writterData, int flags = defaultFlag);
	bool SaveChildMetadata(const class_identifier_t& clsid, CMemoryWriter& writterData, IMetaObject* object, int flags = defaultFlag);
	bool DeleteCommonMetadata(const class_identifier_t& clsid);
	bool DeleteChildMetadata(const class_identifier_t& clsid, IMetaObject* object);

	//run/close recursively:
	bool RunChildMetadata(IMetaObject* object, int flags, bool before);
	bool CloseChildMetadata(IMetaObject* object, int flags, bool before);
	bool ClearChildMetadata(IMetaObject* object);

private:

	wxString m_fullPath;

	IMetaData* m_ownerMeta; //owner for saving/loading
	CModuleManagerExternalReport* m_moduleManager;
	CMetaObjectReport* m_commonObject; 	//common meta object
	bool m_configOpened;

	version_identifier_t m_version;
};