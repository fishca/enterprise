#ifndef _ACC_REGISTER_MANAGER_H__
#define _ACC_REGISTER_MANAGER_H__

#include "accumulationRegister.h"

class CValueManagerDataObjectAccumulationRegister :
	public IValueManagerDataObject {
public:

	CValue Balance(const CValue& cPeriod, const CValue& cFilter = CValue());
	CValue Turnovers(const CValue& cBeginOfPeriod, const CValue& cEndOfPeriod, const CValue& cFilter = CValue());

	CValueManagerDataObjectAccumulationRegister(CValueMetaObjectAccumulationRegister* metaObject = nullptr) : m_metaObject(metaObject) {}
	virtual ~CValueManagerDataObjectAccumulationRegister() {}

	virtual CValueMetaObjectCommonModule* GetModuleManager() const;
	virtual CValueMetaObjectAccumulationRegister* GetMetaObject() const { return m_metaObject; }

	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);//method call

protected:
	CValueMetaObjectAccumulationRegister* m_metaObject;
private:
	wxDECLARE_DYNAMIC_CLASS(CValueManagerDataObjectAccumulationRegister);
};

#endif 