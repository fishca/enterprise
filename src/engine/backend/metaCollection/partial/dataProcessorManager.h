#ifndef _MANAGER_DATAPROCESSOR_H__
#define _MANAGER_DATAPROCESSOR_H__

#include "dataProcessor.h"
#include "backend/managerInfo.h"

class CDataProcessorManager : public CValue,
	public IMetaManagerInfo {
	wxDECLARE_DYNAMIC_CLASS(CDataProcessorManager);
public:

	virtual CMetaObjectCommonModule* GetModuleManager() const;

	CDataProcessorManager(CMetaObjectDataProcessor* metaObject = nullptr);
	virtual ~CDataProcessorManager();

	virtual CMethodHelper* GetPMethods() const {  // get a reference to the class helper for parsing attribute and method names
		//PrepareNames();
		return m_methodHelper;
	}
	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);//method call

	//Get ref class 
	virtual class_identifier_t GetClassType() const;

	//types 
	virtual wxString GetClassName() const;
	virtual wxString GetString() const;

protected:

	CMethodHelper* m_methodHelper;
	CMetaObjectDataProcessor* m_metaObject;
};

class CDataProcessorExternalManager : public CValue {
	wxDECLARE_DYNAMIC_CLASS(CDataProcessorExternalManager);
public:
	CDataProcessorExternalManager();
	virtual ~CDataProcessorExternalManager();

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames();
		return m_methodHelper; 
	} 
	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);//method call

protected:
	CMethodHelper* m_methodHelper;
};

#endif 