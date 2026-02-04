#ifndef __MODULE_MANAGER_H__
#define __MODULE_MANAGER_H__

#include "backend/moduleInfo.h"

#include "backend/metaCollection/metaObjectMetadata.h"
#include "backend/metaCollection/metaModuleObject.h"
#include "backend/metaCollection/partial/commonObject.h"

class BACKEND_API IValueModuleManager :
	public IModuleDataObject, public CValue {
protected:
	enum helperAlias {
		eProcUnit
	};
public:

	class BACKEND_API CValueModuleUnit :
		public IModuleDataObject, public CValue {
		wxDECLARE_DYNAMIC_CLASS(CValueModuleUnit);
	protected:
		enum helperAlias {
			eProcUnit
		};
	public:

		CValueModuleUnit() {}
		CValueModuleUnit(IValueModuleManager* moduleManager, IValueMetaObjectModule* moduleObject, bool managerModule = false);
		virtual ~CValueModuleUnit();

		//initalize common module
		bool CreateCommonModule();
		bool DestroyCommonModule();

		//get common module 
		IValueMetaObjectModule* GetModuleObject() const {
			return m_moduleObject;
		}

		//Is global module?
		wxString GetModuleFullName() const {
			return m_moduleObject ? m_moduleObject->GetFullName() : wxEmptyString;
		}

		wxString GetModuleDocPath() const {
			return m_moduleObject ? m_moduleObject->GetDocPath() : wxEmptyString;
		}

		wxString GetModuleName() const {
			return m_moduleObject ? m_moduleObject->GetName() : wxEmptyString;
		}

		wxString GetModuleText() const {
			return m_moduleObject ? m_moduleObject->GetModuleText() : wxEmptyString;
		}

		bool IsGlobalModule() const {
			return m_moduleObject ? m_moduleObject->IsGlobalModule() : false;
		}

		//WORK AS AN AGGREGATE OBJECT

		// these methods need to be overridden in your aggregate objects:
		virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
			//PrepareNames();
			return m_methodHelper;
		}

		// this method is automatically called to initialize attribute and method names.
		virtual void PrepareNames() const;

		//method call
		virtual bool CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray) override;
		virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray) override;

		virtual wxString GetString() const override {
			return m_moduleObject->GetName();
		}

		//check is empty
		virtual bool IsEmpty() const {
			return false;
		}

		//operator '=='
		virtual bool CompareValueEQ(const CValue& cParam) const override
		{
			CValueModuleUnit* compareModule = dynamic_cast<CValueModuleUnit*>(cParam.GetRef());
			if (compareModule) {
				return m_moduleObject == compareModule->GetModuleObject();
			}

			return false;
		}

		//operator '!='
		virtual bool CompareValueNE(const CValue& cParam) const override
		{
			CValueModuleUnit* compareModule = dynamic_cast<CValueModuleUnit*>(cParam.GetRef());
			if (compareModule) {
				return m_moduleObject != compareModule->GetModuleObject();
			}

			return false;
		}

	protected:
		IValueModuleManager* m_moduleManager;
		IValueMetaObjectModule* m_moduleObject;
	private:
		CMethodHelper* m_methodHelper;
	};

	class BACKEND_API CMetadataUnit :
		public CValue {
		wxDECLARE_DYNAMIC_CLASS(CMetadataUnit);
	public:

		CMetadataUnit() {}
		CMetadataUnit(IMetaData* metaData);
		virtual ~CMetadataUnit();

		//get common module 
		IMetaData* GetMetaData() const { return m_metaData; }

		//check is empty
		virtual bool IsEmpty() const { return false; }

		//operator '=='
		virtual bool CompareValueEQ(const CValue& cParam) const override
		{
			CMetadataUnit* compareMetadata = dynamic_cast<CMetadataUnit*>(cParam.GetRef());
			if (compareMetadata) {
				return m_metaData == compareMetadata->GetMetaData();
			}

			return false;
		}

		//operator '!='
		virtual bool CompareValueNE(const CValue& cParam) const override {
			CMetadataUnit* compareMetadata = dynamic_cast<CMetadataUnit*>(cParam.GetRef());
			if (compareMetadata) {
				return m_metaData != compareMetadata->GetMetaData();
			}

			return false;
		}

		// these methods need to be overridden in your aggregate objects:
		virtual CMethodHelper* GetPMethods() const {
			//PrepareNames();  
			return m_methodHelper;
		}

		virtual void PrepareNames() const; // this method is automatically called to initialize attribute and method names.

		//****************************************************************************
		//*                              Override attribute                          *
		//****************************************************************************

		virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal) override;        //setting attribute
		virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal) override;                   //attribute value

	private:
		IMetaData* m_metaData;
		CMethodHelper* m_methodHelper;
	};

private:

	void Clear();

protected:

	//metaData and external variant
	IValueModuleManager(IMetaData* metaData, CValueMetaObjectModule* metaObject);

public:

	virtual ~IValueModuleManager();

	//Create common module
	virtual bool CreateMainModule() = 0;

	//destroy common module
	virtual bool DestroyMainModule() = 0;

	//start common module
	virtual bool StartMainModule(bool force = false) = 0;

	//exit common module
	virtual bool ExitMainModule(bool force = false) = 0;

	// these methods need to be overridden in your aggregate objects:
	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}

	// this method is automatically called to initialize attribute and method names.
	virtual void PrepareNames() const;

	//method call
	virtual bool CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray);
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value
	virtual long FindProp(const wxString& strName) const;

	//check is empty
	virtual bool IsEmpty() const { return false; }

	//compile modules:
	bool AddCompileModule(const IValueMetaObject* moduleObject, CValue* object);
	bool RemoveCompileModule(const IValueMetaObject* moduleObject);

	//templates:
	template <class T> inline bool FindCompileModule(const IValueMetaObject* moduleObject, T*& objValue) const {
		auto& it = m_listCommonModuleValue.find(moduleObject);
		if (it != m_listCommonModuleValue.end()) {
			objValue = dynamic_cast<T*>(&(*it->second));
			return objValue != nullptr;
		}
		objValue = nullptr;
		return false;
	}

	template <class T> inline bool FindParentCompileModule(const IValueMetaObject* moduleObject, T*& objValue) const {
		IValueMetaObject* parentMetadata = moduleObject ? moduleObject->GetParent() : nullptr;
		if (parentMetadata != nullptr)
			return FindCompileModule(parentMetadata, objValue);
		return false;
	}

	//common modules:
	bool AddCommonModule(CValueMetaObjectCommonModule* commonModule, bool managerModule = false, bool runModule = false);

	CValueModuleUnit* FindCommonModule(CValueMetaObjectCommonModule* commonModule) const;
	bool RenameCommonModule(CValueMetaObjectCommonModule* commonModule, const wxString& newName);
	bool RemoveCommonModule(CValueMetaObjectCommonModule* commonModule);

	//system object:
	CValue* GetObjectManager() const { return m_objectManager; }
	CMetadataUnit* GetMetaManager() const { return m_metaManager; }

	virtual std::vector<CValuePtr<CValueModuleUnit>>& GetCommonModules() { return m_listCommonModuleManager; }

	//associated map
	virtual std::map<wxString, CValuePtr<CValue>>& GetGlobalVariables() { return m_listGlConstValue; }
	virtual std::map<wxString, CValue*>& GetContextVariables() { return m_compileModule->m_listContextValue; }

	//return external module
	virtual CValue* GetObjectValue() const { return nullptr; }

protected:

	bool m_initialized;

	//global manager
	CValuePtr<CValue> m_objectManager;

	// global metamanager
	CValuePtr<CMetadataUnit> m_metaManager;

	//map with compile data
	std::map<const IValueMetaObject*, CValuePtr<CValue>> m_listCommonModuleValue;

	//array of common modules
	std::vector<CValuePtr<CValueModuleUnit>> m_listCommonModuleManager;

	//array of global variables
	std::map<wxString, CValuePtr<CValue>> m_listGlConstValue;

	friend class CMetaDataConfiguration;
	friend class CMetaDataDataProcessor;

	friend class CValueModuleUnit;

	CMethodHelper* m_methodHelper;
};

class BACKEND_API CValueModuleManagerConfiguration :
	public IValueModuleManager {
	//system events:
	bool BeforeStart();
	void OnStart();
	bool BeforeExit();
	void OnExit();
public:

	//metaData and external variant
	CValueModuleManagerConfiguration(IMetaData* metaData = nullptr, CValueMetaObjectConfiguration* metaObject = nullptr);

	//Create common module
	virtual bool CreateMainModule();

	//destroy common module
	virtual bool DestroyMainModule();

	//start common module
	virtual bool StartMainModule(bool force = false);

	//exit common module
	virtual bool ExitMainModule(bool force = false);
};

#endif

