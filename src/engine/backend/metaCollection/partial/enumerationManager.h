#ifndef _ENUMERATION_MANAGER_H__
#define _ENUMERATION_MANAGER_H__

#include "enumeration.h"

class CManagerDataObjectEnumeration : 
	public IManagerDataObject {
	wxDECLARE_DYNAMIC_CLASS(CManagerDataObjectEnumeration);
public:

	CManagerDataObjectEnumeration(CMetaObjectEnumeration *metaObject = nullptr);
	virtual ~CManagerDataObjectEnumeration();

	virtual CMetaObjectCommonModule *GetModuleManager() const;
	virtual CMetaObjectEnumeration* GetMetaObject() const { return m_metaObject; }

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}
	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);//method call

	virtual bool SetPropVal(const long lPropNum, CValue &varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

	//Get ref class 
	virtual class_identifier_t GetClassType() const;

	//types 
	virtual wxString GetClassName() const;
	virtual wxString GetString() const;

protected:
	//methods 
	CMetaObjectEnumeration* m_metaObject;
	CMethodHelper *m_methodHelper;
};

#endif 