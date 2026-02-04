#ifndef _MANAGER_DATAPROCESSOR_H__
#define _MANAGER_DATAPROCESSOR_H__

#include "dataProcessor.h"

class CValueManagerDataObjectDataProcessor :
	public IValueManagerDataObject {
	wxDECLARE_DYNAMIC_CLASS(CValueManagerDataObjectDataProcessor);
public:

	virtual CValueMetaObjectCommonModule* GetModuleManager() const;
	virtual CValueMetaObjectDataProcessor* GetMetaObject() const { return m_metaObject; }

	CValueManagerDataObjectDataProcessor(CValueMetaObjectDataProcessor* metaObject = nullptr);
	virtual ~CValueManagerDataObjectDataProcessor();

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
	CValueMetaObjectDataProcessor* m_metaObject;
};

class CValueManagerDataObjectExternalDataProcessor :
	public IValueManagerDataObject {
	wxDECLARE_DYNAMIC_CLASS(CValueManagerDataObjectExternalDataProcessor);
public:
	
	CValueManagerDataObjectExternalDataProcessor();
	virtual ~CValueManagerDataObjectExternalDataProcessor();

	virtual CValueMetaObjectCommonModule* GetModuleManager() const { return nullptr; }
	virtual CValueMetaObjectDataProcessor* GetMetaObject() const { return nullptr; }

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