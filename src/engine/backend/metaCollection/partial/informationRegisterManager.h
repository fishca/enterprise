#ifndef _INFO_REGISTER_MANAGER_H__
#define _INFO_REGISTER_MANAGER_H__

#include "informationRegister.h"

class CManagerDataObjectInformationRegister :
	public IManagerDataObject {
	wxDECLARE_DYNAMIC_CLASS(CManagerDataObjectInformationRegister);
public:

	CValue Get(const CValue& cFilter = CValue());
	CValue Get(const CValue& cPeriod, const CValue& cFilter);

	CValue GetFirst(const CValue& cPeriod, const CValue& cFilter = CValue());
	CValue GetLast(const CValue& cPeriod, const CValue& cFilter = CValue());

	CValue SliceFirst(const CValue& cPeriod, const CValue& cFilter = CValue());
	CValue SliceLast(const CValue& cPeriod, const CValue& cFilter = CValue());

	CManagerDataObjectInformationRegister(CMetaObjectInformationRegister* metaObject = nullptr);
	virtual ~CManagerDataObjectInformationRegister();

	virtual CMetaObjectCommonModule* GetModuleManager() const;
	virtual CMetaObjectInformationRegister* GetMetaObject() const { return m_metaObject; }

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}
	virtual void PrepareNames() const; // this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray); //method call

	//Get ref class 
	virtual class_identifier_t GetClassType() const;

	//types 
	virtual wxString GetClassName() const;
	virtual wxString GetString() const;

protected:
	//methods 
	CMetaObjectInformationRegister* m_metaObject;
	CMethodHelper* m_methodHelper;
};

#endif 