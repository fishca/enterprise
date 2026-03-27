#ifndef _INFO_REGISTER_MANAGER_H__
#define _INFO_REGISTER_MANAGER_H__

#include "informationRegister.h"

class ibValueManagerDataObjectInformationRegister :
	public ibValueManagerDataObject {
public:

	ibValue Get(const ibValue& cFilter = ibValue());
	ibValue Get(const ibValue& cPeriod, const ibValue& cFilter);

	ibValue GetFirst(const ibValue& cPeriod, const ibValue& cFilter = ibValue());
	ibValue GetLast(const ibValue& cPeriod, const ibValue& cFilter = ibValue());

	ibValue SliceFirst(const ibValue& cPeriod, const ibValue& cFilter = ibValue());
	ibValue SliceLast(const ibValue& cPeriod, const ibValue& cFilter = ibValue());

	ibValueManagerDataObjectInformationRegister(ibValueMetaObjectInformationRegister* metaObject = nullptr) : m_metaObject(metaObject) {}
	virtual ~ibValueManagerDataObjectInformationRegister() {}

	virtual ibValueMetaObjectCommonModule* GetModuleManager() const;
	virtual ibValueMetaObjectInformationRegister* GetMetaObject() const { return m_metaObject; }

	virtual void PrepareNames() const; // this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray); //method call

protected:
	ibValueMetaObjectInformationRegister* m_metaObject;
private:
	wxDECLARE_DYNAMIC_CLASS(ibValueManagerDataObjectInformationRegister);
};

#endif 