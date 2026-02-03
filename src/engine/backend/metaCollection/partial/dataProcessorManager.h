#ifndef _MANAGER_DATAPROCESSOR_H__
#define _MANAGER_DATAPROCESSOR_H__

#include "dataProcessor.h"

class CManagerDataObjectDataProcessor :
	public IManagerDataObject {
	wxDECLARE_DYNAMIC_CLASS(CManagerDataObjectDataProcessor);
public:

	virtual CMetaObjectCommonModule* GetModuleManager() const;
	virtual CMetaObjectDataProcessor* GetMetaObject() const { return m_metaObject; }

	CManagerDataObjectDataProcessor(CMetaObjectDataProcessor* metaObject = nullptr);
	virtual ~CManagerDataObjectDataProcessor();

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

class CManagerDataObjectExternalDataProcessor :
	public IManagerDataObject {
	wxDECLARE_DYNAMIC_CLASS(CManagerDataObjectExternalDataProcessor);
public:
	
	CManagerDataObjectExternalDataProcessor();
	virtual ~CManagerDataObjectExternalDataProcessor();

	virtual CMetaObjectCommonModule* GetModuleManager() const { return nullptr; }
	virtual CMetaObjectDataProcessor* GetMetaObject() const { return nullptr; }

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