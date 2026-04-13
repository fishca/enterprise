#ifndef _MANAGER_DOCUMENT_H__
#define _MANAGER_DOCUMENT_H__

#include "document.h"

class CValueManagerDataObjectDocument :
	public IValueManagerDataObject {
public:

	CValueReferenceDataObject* FindByNumber(const CValue& vCode, const CValue& vPeriod);
	CValueReferenceDataObject* EmptyRef();

	CValueManagerDataObjectDocument(CValueMetaObjectDocument* metaObject = nullptr) : m_metaObject(metaObject) {}
	virtual ~CValueManagerDataObjectDocument() {}

	virtual CValueMetaObjectCommonModule* GetModuleManager() const;
	virtual CValueMetaObjectDocument* GetMetaObject() const { return m_metaObject; }

	virtual void PrepareNames() const; // this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);//method call

protected:
	CValueMetaObjectDocument* m_metaObject;
private:
	wxDECLARE_DYNAMIC_CLASS(CValueManagerDataObjectDocument);
};

#endif 