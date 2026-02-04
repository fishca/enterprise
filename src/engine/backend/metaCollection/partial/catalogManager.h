#ifndef _MANAGER_CATALOG_H__
#define _MANAGER_CATALOG_H__

#include "catalog.h"

class CValueManagerDataObjectCatalog :
	public IValueManagerDataObject {
	wxDECLARE_DYNAMIC_CLASS(CValueManagerDataObjectCatalog);
public:

	CValueManagerDataObjectCatalog(CValueMetaObjectCatalog* metaObject = nullptr);
	virtual ~CValueManagerDataObjectCatalog();

	virtual CValueMetaObjectCommonModule* GetModuleManager() const;
	virtual CValueMetaObjectCatalog* GetMetaObject() const { return m_metaObject; }

	CValueReferenceDataObject* FindByCode(const CValue& vCode) const;
	CValueReferenceDataObject* FindByDescription(const CValue& cParam) const;

	CValueReferenceDataObject* EmptyRef() const;

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
	CValueMetaObjectCatalog* m_metaObject;
};

#endif 