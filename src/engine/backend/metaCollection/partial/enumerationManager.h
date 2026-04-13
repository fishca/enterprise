#ifndef _ENUMERATION_MANAGER_H__
#define _ENUMERATION_MANAGER_H__

#include "enumeration.h"

class CValueManagerDataObjectEnumeration :
	public IValueManagerDataObject {
public:

	CValueManagerDataObjectEnumeration(CValueMetaObjectEnumeration* metaObject = nullptr) : m_metaObject(metaObject) {}
	virtual ~CValueManagerDataObjectEnumeration() {}

	virtual CValueMetaObjectCommonModule* GetModuleManager() const;
	virtual CValueMetaObjectEnumeration* GetMetaObject() const { return m_metaObject; }

	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);//method call

	virtual bool SetPropVal(const long lPropNum, CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

protected:
	CValueMetaObjectEnumeration* m_metaObject;
private:
	wxDECLARE_DYNAMIC_CLASS(CValueManagerDataObjectEnumeration);
};

#endif 