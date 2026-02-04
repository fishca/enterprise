#ifndef _MANAGER_REPORT_H__
#define _MANAGER_REPORT_H__

#include "dataReport.h"

class CValueManagerDataObjectReport :
	public IValueManagerDataObject {
	wxDECLARE_DYNAMIC_CLASS(CValueManagerDataObjectReport);
public:

	CValueManagerDataObjectReport(CValueMetaObjectReport* metaObject = nullptr);
	virtual ~CValueManagerDataObjectReport();

	virtual CValueMetaObjectCommonModule* GetModuleManager() const;
	virtual CValueMetaObjectReport* GetMetaObject() const { return m_metaObject; }

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
	CValueMetaObjectReport* m_metaObject;
	CMethodHelper* m_methodHelper;
};

class CValueManagerDataObjectExternalReport :
	public IValueManagerDataObject {
	wxDECLARE_DYNAMIC_CLASS(CValueManagerDataObjectExternalReport);
public:
	CValueManagerDataObjectExternalReport();
	virtual ~CValueManagerDataObjectExternalReport();

	virtual CValueMetaObjectCommonModule* GetModuleManager() const { return nullptr; }
	virtual CValueMetaObjectReport* GetMetaObject() const { return nullptr; }

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