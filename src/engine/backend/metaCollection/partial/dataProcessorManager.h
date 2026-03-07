#ifndef _MANAGER_DATAPROCESSOR_H__
#define _MANAGER_DATAPROCESSOR_H__

#include "dataProcessor.h"

class CValueManagerDataObjectDataProcessor :
	public IValueManagerDataObject {
public:

	CValueManagerDataObjectDataProcessor(CValueMetaObjectDataProcessor* metaObject = nullptr) : m_metaObject(metaObject) {}
	virtual ~CValueManagerDataObjectDataProcessor() {}

	virtual CValueMetaObjectCommonModule* GetModuleManager() const;
	virtual CValueMetaObjectDataProcessor* GetMetaObject() const { return m_metaObject; }

	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);//method call

protected:
	CValueMetaObjectDataProcessor* m_metaObject;
private:
	wxDECLARE_DYNAMIC_CLASS(CValueManagerDataObjectDataProcessor);
};

class CValueManagerDataObjectExternalDataProcessor :
	public IValueManagerObject {
public:

	CValueManagerDataObjectExternalDataProcessor() {}
	virtual ~CValueManagerDataObjectExternalDataProcessor() {}

	virtual CValueMetaObjectDataProcessor* GetMetaObject() const { return nullptr; }

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames();
		return &m_methodHelper;
	}
	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);//method call

protected:
	static CMethodHelper m_methodHelper;
private:
	wxDECLARE_DYNAMIC_CLASS(CValueManagerDataObjectExternalDataProcessor);
};

#endif 