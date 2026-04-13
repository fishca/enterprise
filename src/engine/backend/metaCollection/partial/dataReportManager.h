#ifndef _MANAGER_REPORT_H__
#define _MANAGER_REPORT_H__

#include "dataReport.h"

class CValueManagerDataObjectReport :
	public IValueManagerDataObject {
public:

	CValueManagerDataObjectReport(CValueMetaObjectReport* metaObject = nullptr) : m_metaObject(metaObject) {}
	virtual ~CValueManagerDataObjectReport() {}

	virtual CValueMetaObjectCommonModule* GetModuleManager() const;
	virtual CValueMetaObjectReport* GetMetaObject() const { return m_metaObject; }

	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);//method call

protected:
	CValueMetaObjectReport* m_metaObject;
private:
	wxDECLARE_DYNAMIC_CLASS(CValueManagerDataObjectReport);
};

class CValueManagerDataObjectExternalReport :
	public IValueManagerObject {
public:

	CValueManagerDataObjectExternalReport() {}
	virtual ~CValueManagerDataObjectExternalReport() {}

	virtual CValueMetaObjectReport* GetMetaObject() const { return nullptr; }

	virtual CMethodHelper* GetPMethods() const {  // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return &m_methodHelper;
	}

	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);//method call

protected:
	static CMethodHelper m_methodHelper;
private:
	wxDECLARE_DYNAMIC_CLASS(CValueManagerDataObjectExternalReport);
};

#endif 