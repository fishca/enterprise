#ifndef _MANAGER_REPORT_H__
#define _MANAGER_REPORT_H__

#include "dataReport.h"

class CManagerDataObjectReport :
	public IManagerDataObject {
	wxDECLARE_DYNAMIC_CLASS(CManagerDataObjectReport);
public:

	CManagerDataObjectReport(CMetaObjectReport* metaObject = nullptr);
	virtual ~CManagerDataObjectReport();

	virtual CMetaObjectCommonModule* GetModuleManager() const;
	virtual CMetaObjectReport* GetMetaObject() const { return m_metaObject; }

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
	CMetaObjectReport* m_metaObject;
	CMethodHelper* m_methodHelper;
};

class CManagerDataObjectExternalReport :
	public IManagerDataObject {
	wxDECLARE_DYNAMIC_CLASS(CManagerDataObjectExternalReport);
public:
	CManagerDataObjectExternalReport();
	virtual ~CManagerDataObjectExternalReport();

	virtual CMetaObjectCommonModule* GetModuleManager() const { return nullptr; }
	virtual CMetaObjectReport* GetMetaObject() const { return nullptr; }

	virtual CMethodHelper* GetPMethods() const {  // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}

	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);//method call

protected:
	CMethodHelper* m_methodHelper;
};

#endif 