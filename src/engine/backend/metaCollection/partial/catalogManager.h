#ifndef _MANAGER_CATALOG_H__
#define _MANAGER_CATALOG_H__

#include "catalog.h"

class CValueManagerDataObjectCatalog :
	public IValueManagerDataObjectPredefined {
public:

	CValueReferenceDataObject* FindByCode(const CValue& vCode) const;
	CValueReferenceDataObject* FindByDescription(const CValue& cParam) const;

	CValueReferenceDataObject* EmptyRef() const;

	CValueManagerDataObjectCatalog(CValueMetaObjectCatalog* metaObject = nullptr) : m_metaObject(metaObject) {}
	virtual ~CValueManagerDataObjectCatalog() {}

	virtual CValueMetaObjectCommonModule* GetModuleManager() const;
	virtual CValueMetaObjectCatalog* GetMetaObject() const { return m_metaObject; }

	virtual void PrepareNames() const; // this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);//method call

protected:
	CValueMetaObjectCatalog* m_metaObject;
private:
	wxDECLARE_DYNAMIC_CLASS(CValueManagerDataObjectCatalog);
};

#endif 