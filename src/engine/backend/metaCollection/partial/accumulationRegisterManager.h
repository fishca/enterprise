#ifndef _ACC_REGISTER_MANAGER_H__
#define _ACC_REGISTER_MANAGER_H__

#include "accumulationRegister.h"

class CManagerDataObjectAccumulationRegister : 
	public IManagerDataObject {
	wxDECLARE_DYNAMIC_CLASS(CManagerDataObjectAccumulationRegister);
public:

	CValue Balance(const CValue& cPeriod, const CValue& cFilter = CValue());
	CValue Turnovers(const CValue& cBeginOfPeriod, const CValue& cEndOfPeriod, const CValue& cFilter = CValue());

	CManagerDataObjectAccumulationRegister(CMetaObjectAccumulationRegister* metaObject = nullptr);
	virtual ~CManagerDataObjectAccumulationRegister();

	virtual CMetaObjectCommonModule* GetModuleManager() const;
	virtual CMetaObjectAccumulationRegister* GetMetaObject() const { return m_metaObject; }

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
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
	//methods 
	CMethodHelper* m_methodHelper;
	CMetaObjectAccumulationRegister* m_metaObject;
};

#endif 