#ifndef __CONST_CTOR_H__
#define __CONST_CTOR_H__

#include "objCtor.h"

//const-object class
class CMetaValueConstantObjectTypeCtor :
	public IMetaValueTypeCtor {
public:

	CMetaValueConstantObjectTypeCtor(CMetaObjectConstant* recordRef) : IMetaValueTypeCtor(), m_metaObject(recordRef) {
		m_classType = string_to_clsid(wxT("C_") +
			stringUtils::IntToStr(m_metaObject->GetMetaID()));
	}

	wxString CMetaValueConstantObjectTypeCtor::GetClassName() const {
		return m_metaObject->GetClassName() + prefixObject +
			m_metaObject->GetName();
	}

	virtual class_identifier_t CMetaValueConstantObjectTypeCtor::GetClassType() const { return m_classType; }
	virtual wxClassInfo* GetClassInfo() const;
	virtual CValue* CreateObject() const;
	virtual IMetaObject* GetMetaObject() const { return m_metaObject; }
	virtual eCtorMetaType GetMetaTypeCtor() const { return eCtorMetaType::eCtorMetaType_Object; }

protected:
	class_identifier_t m_classType;
	CMetaObjectConstant* m_metaObject;
};

#define registerConstObject()\
	m_metaData->RegisterCtor(new CMetaValueConstantObjectTypeCtor(this))
#define unregisterConstObject()\
	m_metaData->UnRegisterCtor(generate_class_name(prefixObject))

//const-manager class 
class CMetaValueConstManagerTypeCtor :
	public IMetaValueTypeCtor {
public:

	CMetaValueConstManagerTypeCtor(class CMetaObjectConstant* recordRef) : IMetaValueTypeCtor(), m_metaObject(recordRef) {
		m_classType = string_to_clsid(wxT("G_") +
			stringUtils::IntToStr(m_metaObject->GetMetaID()));
	}

	virtual wxString GetClassName() const {
		return m_metaObject->GetClassName() + prefixManager + m_metaObject->GetName();
	}

	virtual class_identifier_t GetClassType() const { return m_classType; }
	virtual wxClassInfo* GetClassInfo() const;
	virtual CValue* CreateObject() const;
	virtual IMetaObject* GetMetaObject() const { return m_metaObject; }
	virtual eCtorMetaType GetMetaTypeCtor() const { return eCtorMetaType::eCtorMetaType_Manager; }

protected:
	class_identifier_t m_classType;
	CMetaObjectConstant* m_metaObject;
};

#define registerConstManager()\
	m_metaData->RegisterCtor(new CMetaValueConstManagerTypeCtor(this))
#define unregisterConstManager()\
	m_metaData->UnRegisterCtor(generate_class_name(prefixManager))

#endif 