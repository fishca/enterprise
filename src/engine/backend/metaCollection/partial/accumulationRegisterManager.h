#ifndef _ACC_REGISTER_MANAGER_H__
#define _ACC_REGISTER_MANAGER_H__

#include "accumulationRegister.h"

class CValueManagerDataObjectAccumulationRegister : 
	public IValueManagerDataObject {
	wxDECLARE_DYNAMIC_CLASS(CValueManagerDataObjectAccumulationRegister);
public:

	CValue Balance(const CValue& cPeriod, const CValue& cFilter = CValue());
	CValue Turnovers(const CValue& cBeginOfPeriod, const CValue& cEndOfPeriod, const CValue& cFilter = CValue());

	CValueManagerDataObjectAccumulationRegister(CValueMetaObjectAccumulationRegister* metaObject = nullptr);
	virtual ~CValueManagerDataObjectAccumulationRegister();

	virtual CValueMetaObjectCommonModule* GetModuleManager() const;
	virtual CValueMetaObjectAccumulationRegister* GetMetaObject() const { return m_metaObject; }

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
	CValueMetaObjectAccumulationRegister* m_metaObject;
};

#endif 