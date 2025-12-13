#ifndef _SINGLE_CLASS_H__
#define _SINGLE_CLASS_H__

#include "backend/compiler/typeCtor.h"

enum eCtorMetaType {
	eCtorMetaType_Reference = 1,
	eCtorMetaType_List,
	eCtorMetaType_Object,
	eCtorMetaType_Manager,
	eCtorMetaType_Selection,
	eCtorMetaType_TabularSection,
	eCtorMetaType_TabularSection_String,
	eCtorMetaType_RecordSet,
	eCtorMetaType_RecordSet_String,
	eCtorMetaType_RecordKey,
	eCtorMetaType_RecordManager
};

#define generate_class_name(prefix) GetClassName() + prefix + GetName()
#define generate_class_table_name(prefix) metaObject->GetClassName() + prefix + metaObject->GetName() + wxT(".") + GetName()

//record
#define prefixReference		wxT("Ref.")
#define prefixList			wxT("List.")
#define prefixObject		wxT("Object.")
#define prefixManager		wxT("Manager.")
#define prefixSelection		wxT("Selection.")

#define prefixTabSection	wxT("TabularSection.")
#define prefixTabSectionStr	wxT("TabularSectionString.")

//accum 
#define prefixRecordKey		wxT("RecordKey.")
#define prefixRecordManager wxT("RecordManager.")
#define prefixRecordSet		wxT("RecordSet.")

#define prefixRecordSetStr	wxT("RecordSetString.")

class IMetaValueTypeCtor : public IAbstractTypeCtor {
public:

	IMetaValueTypeCtor() {}
	virtual ~IMetaValueTypeCtor() {}

	virtual eCtorObjectType GetObjectTypeCtor() const final { return eCtorObjectType::eCtorObjectType_object_meta_value; }

	template <typename metaType>
	inline bool ConvertToMetaValue(metaType*& refValue) const {
		const IMetaObject* metaObject = GetMetaObject();
		return metaObject ?
			metaObject->ConvertToValue(refValue) : false;
	}

	virtual wxIcon GetClassIcon() const {
		const IMetaObject* metaObject = GetMetaObject();
		return metaObject ?
			metaObject->GetIcon() : wxNullIcon;
	}

	virtual eCtorMetaType GetMetaTypeCtor() const = 0;
	virtual IMetaObject* GetMetaObject() const = 0;
};

//reference class 
class CMetaValueRefTypeCtor : public IMetaValueTypeCtor {
	class_identifier_t m_classType;
public:

	CMetaValueRefTypeCtor(IMetaObjectRecordDataRef* recordRef) : IMetaValueTypeCtor(), m_metaObject(recordRef) {
		m_classType = string_to_clsid(wxT("R_") +
			stringUtils::IntToStr(m_metaObject->GetMetaID()));
	}

	virtual wxString GetClassName() const {
		return m_metaObject->GetClassName() + prefixReference + m_metaObject->GetName();
	}

	virtual class_identifier_t GetClassType() const { return m_classType; }
	virtual wxClassInfo* GetClassInfo() const;
	virtual CValue* CreateObject() const;
	virtual IMetaObject* GetMetaObject() const { return m_metaObject; }
	virtual eCtorMetaType GetMetaTypeCtor() const { return eCtorMetaType::eCtorMetaType_Reference; }

protected:
	IMetaObjectRecordDataRef* m_metaObject;
};

#define registerReference()\
	m_metaData->RegisterCtor(new CMetaValueRefTypeCtor(this))
#define unregisterReference()\
	m_metaData->UnRegisterCtor(generate_class_name(prefixReference))

//list object class 
class CMetaValueListRefTypeCtor : public IMetaValueTypeCtor {
	class_identifier_t m_classType;
public:

	CMetaValueListRefTypeCtor(IMetaObjectRecordDataRef* recordRef) : IMetaValueTypeCtor(), m_metaObject(recordRef) {
		m_classType = string_to_clsid(wxT("L_") +
			stringUtils::IntToStr(m_metaObject->GetMetaID()));
	}

	virtual wxString GetClassName() const {
		return m_metaObject->GetClassName() + prefixList + m_metaObject->GetName();
	}

	virtual class_identifier_t GetClassType() const { return m_classType; }
	virtual wxClassInfo* GetClassInfo() const;
	virtual CValue* CreateObject() const;
	virtual IMetaObject* GetMetaObject() const { return m_metaObject; }
	virtual eCtorMetaType GetMetaTypeCtor() const { return eCtorMetaType::eCtorMetaType_List; }

protected:
	IMetaObjectRecordDataRef* m_metaObject;
};

#define registerRefList()\
	m_metaData->RegisterCtor(new CMetaValueListRefTypeCtor(this))
#define unregisteRefList()\
	m_metaData->UnRegisterCtor(generate_class_name(prefixList))

//list register class
class CMetaValueListRegisterTypeCtor : public IMetaValueTypeCtor {
	class_identifier_t m_classType;
public:

	CMetaValueListRegisterTypeCtor(IMetaObjectRegisterData* recordRef) : IMetaValueTypeCtor(), m_metaObject(recordRef) {
		m_classType = string_to_clsid(wxT("J_") +
			stringUtils::IntToStr(m_metaObject->GetMetaID()));
	}

	virtual wxString GetClassName() const {
		return m_metaObject->GetClassName() + prefixList + m_metaObject->GetName();
	}

	virtual class_identifier_t GetClassType() const { return m_classType; }
	virtual wxClassInfo* GetClassInfo() const;
	virtual CValue* CreateObject() const;
	virtual IMetaObject* GetMetaObject() const { return m_metaObject; }
	virtual eCtorMetaType GetMetaTypeCtor() const { return eCtorMetaType::eCtorMetaType_List; }

protected:
	IMetaObjectRegisterData* m_metaObject;
};

#define registerRegList()\
	m_metaData->RegisterCtor(new CMetaValueListRegisterTypeCtor(this))
#define unregisterRegList()\
	m_metaData->UnRegisterCtor(generate_class_name(prefixList))

//object class
class CMetaValueObjectTypeCtor : public IMetaValueTypeCtor {
	IMetaObjectRecordData* m_metaObject;
public:

	CMetaValueObjectTypeCtor(IMetaObjectRecordData* recordRef) : IMetaValueTypeCtor(), m_metaObject(recordRef) {
		m_classType = string_to_clsid(wxT("O_") +
			stringUtils::IntToStr(m_metaObject->GetMetaID()));
	}

	virtual wxString GetClassName() const {
		return m_metaObject->GetClassName() + prefixObject +
			m_metaObject->GetName();
	}

	virtual class_identifier_t GetClassType() const { return m_classType; }
	virtual wxClassInfo* GetClassInfo() const;
	virtual CValue* CreateObject() const;
	virtual IMetaObject* GetMetaObject() const { return m_metaObject; }
	virtual eCtorMetaType GetMetaTypeCtor() const { return eCtorMetaType::eCtorMetaType_Object; }

protected:
	class_identifier_t m_classType;
};

class CMetaValueExternalObjectTypeCtor : public CMetaValueObjectTypeCtor {
	class_identifier_t m_classType;
public:

	CMetaValueExternalObjectTypeCtor(IMetaObjectRecordData* recordRef) : CMetaValueObjectTypeCtor(recordRef) {
		m_classType = string_to_clsid(wxT("EO_") +
			stringUtils::IntToStr(recordRef->GetMetaID()));
	}

	virtual class_identifier_t GetClassType() const { return m_classType; }
};

#define registerObject()\
	m_metaData->RegisterCtor(new CMetaValueObjectTypeCtor(this))
#define registerExternalObject()\
	m_metaData->RegisterCtor(new CMetaValueExternalObjectTypeCtor(this))
#define unregisterObject()\
	m_metaData->UnRegisterCtor(generate_class_name(prefixObject))

//const-object class
class CMetaValueConstantObjectTypeCtor : public IMetaValueTypeCtor {
	class_identifier_t m_classType;
public:

	CMetaValueConstantObjectTypeCtor(IMetaObject* recordRef) : IMetaValueTypeCtor(), m_metaObject(recordRef) {
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
	IMetaObject* m_metaObject;
};

#define registerConstObject()\
	m_metaData->RegisterCtor(new CMetaValueConstantObjectTypeCtor(this))
#define unregisterConstObject()\
	m_metaData->UnRegisterCtor(generate_class_name(prefixObject))

//manager class 
class CMetaValueManagerTypeCtor : public IMetaValueTypeCtor {
	IMetaObject* m_metaObject;
public:

	CMetaValueManagerTypeCtor(IMetaObject* recordRef) : IMetaValueTypeCtor(), m_metaObject(recordRef) {
		m_classType = string_to_clsid(wxT("M_") +
			stringUtils::IntToStr(m_metaObject->GetMetaID()));
	}

	virtual wxString GetClassName() const {
		return m_metaObject->GetClassName() + prefixManager + m_metaObject->GetName();
	}

	virtual class_identifier_t GetClassType() const { return m_classType; }
	virtual wxClassInfo* GetClassInfo() const { return nullptr; }
	virtual CValue* CreateObject() const { return nullptr; }
	virtual IMetaObject* GetMetaObject() const { return m_metaObject; }
	virtual eCtorMetaType GetMetaTypeCtor() const { return eCtorMetaType::eCtorMetaType_Manager; }

protected:
	class_identifier_t m_classType;
};

class CMetaValueExternalManagerTypeCtor : public CMetaValueManagerTypeCtor {
public:

	CMetaValueExternalManagerTypeCtor(IMetaObject* recordRef) :CMetaValueManagerTypeCtor(recordRef) {
		m_classType = string_to_clsid(wxT("EM_") +
			stringUtils::IntToStr(recordRef->GetMetaID()));
	}

	virtual class_identifier_t GetClassType() const { return m_classType; }
};

#define registerManager()\
	m_metaData->RegisterCtor(new CMetaValueManagerTypeCtor(this))
#define registerExternalManager()\
	m_metaData->RegisterCtor(new CMetaValueExternalManagerTypeCtor(this))
#define unregisterManager()\
	m_metaData->UnRegisterCtor(generate_class_name(prefixManager))

//selection class
class CMetaValueSelectionTypeCtor : public IMetaValueTypeCtor {
	class_identifier_t m_classType;
public:

	CMetaValueSelectionTypeCtor(IMetaObjectGenericData* recordRef) : IMetaValueTypeCtor(), m_metaObject(recordRef) {
		m_classType = string_to_clsid(wxT("S_") +
			stringUtils::IntToStr(m_metaObject->GetMetaID()));
	}

	virtual wxString GetClassName() const {
		return m_metaObject->GetClassName() + prefixSelection + m_metaObject->GetName();
	}

	virtual class_identifier_t GetClassType() const { return m_classType; }
	virtual wxClassInfo* GetClassInfo() const { return nullptr; }
	virtual CValue* CreateObject() const { return nullptr; }
	virtual IMetaObject* GetMetaObject() const { return m_metaObject; }
	virtual eCtorMetaType GetMetaTypeCtor() const { return eCtorMetaType::eCtorMetaType_Selection; }

protected:
	IMetaObjectGenericData* m_metaObject;
};

#define registerSelection()\
	m_metaData->RegisterCtor(new CMetaValueSelectionTypeCtor(this))
#define unregisterSelection()\
	m_metaData->UnRegisterCtor(generate_class_name(prefixSelection))

//tabular section class
class CMetaValueTabularSectionTypeCtor : public IMetaValueTypeCtor {
	class_identifier_t m_classType;
	CMetaObjectTableData* m_metaTable;
public:

	CMetaValueTabularSectionTypeCtor(IMetaObjectRecordData* recordRef, CMetaObjectTableData* recordTable) : IMetaValueTypeCtor(), m_metaObject(recordRef), m_metaTable(recordTable) {
		m_classType = string_to_clsid(wxT("T_") +
			stringUtils::IntToStr(m_metaTable->GetMetaID()));
	}

	virtual wxString GetClassName() const {
		return m_metaObject->GetClassName() + prefixTabSection + m_metaObject->GetName() + wxT(".") + m_metaTable->GetName();
	}

	virtual class_identifier_t GetClassType() const { return m_classType; }
	virtual wxClassInfo* GetClassInfo() const { return nullptr; }
	virtual CValue* CreateObject() const { return nullptr; }
	virtual IMetaObject* GetMetaObject() const { return m_metaTable; }
	virtual eCtorMetaType GetMetaTypeCtor() const { return eCtorMetaType::eCtorMetaType_TabularSection; }

protected:
	IMetaObjectRecordData* m_metaObject;
};

#define registerTabularSection()\
	m_metaData->RegisterCtor(new CMetaValueTabularSectionTypeCtor(metaObject, this))
#define unregisterTabularSection()\
	m_metaData->UnRegisterCtor(generate_class_table_name(prefixTabSection))

//tabular section string class
class CMetaValueTabularSectionStringTypeCtor : public IMetaValueTypeCtor {
	class_identifier_t m_classType;
public:

	CMetaValueTabularSectionStringTypeCtor(IMetaObjectRecordData* recordRef, CMetaObjectTableData* recordTable) : IMetaValueTypeCtor(), m_metaObject(recordRef), m_metaTable(recordTable) {
		m_classType = string_to_clsid(wxT("B_") +
			stringUtils::IntToStr(m_metaTable->GetMetaID()));
	}

	virtual wxString GetClassName() const {
		return m_metaObject->GetClassName() + prefixTabSectionStr + m_metaObject->GetName() + wxT(".") + m_metaTable->GetName();
	}

	virtual class_identifier_t GetClassType() const { return m_classType; }
	virtual wxClassInfo* GetClassInfo() const { return nullptr; }
	virtual CValue* CreateObject() const { return nullptr; }
	virtual IMetaObject* GetMetaObject() const { return m_metaTable; }
	virtual eCtorMetaType GetMetaTypeCtor() const { return eCtorMetaType::eCtorMetaType_TabularSection_String; }

protected:
	IMetaObjectRecordData* m_metaObject;
	CMetaObjectTableData* m_metaTable;
};

#define registerTabularSection_String()\
	m_metaData->RegisterCtor(new CMetaValueTabularSectionStringTypeCtor(metaObject, this))
#define unregisterTabularSection_String()\
	m_metaData->UnRegisterCtor(generate_class_table_name(prefixTabSectionStr))

//record key class
class CMetaValueRecordKeyTypeCtor : public IMetaValueTypeCtor {
	class_identifier_t m_classType;
public:

	CMetaValueRecordKeyTypeCtor(IMetaObjectRegisterData* recordRef) : IMetaValueTypeCtor(), m_metaObject(recordRef) {
		m_classType = string_to_clsid(wxT("A_") +
			stringUtils::IntToStr(m_metaObject->GetMetaID()));
	}

	virtual wxString GetClassName() const {
		return m_metaObject->GetClassName() + prefixRecordKey + m_metaObject->GetName();
	}

	virtual class_identifier_t GetClassType() const { return m_classType; }
	virtual wxClassInfo* GetClassInfo() const;
	virtual CValue* CreateObject() const;
	virtual IMetaObject* GetMetaObject() const { return m_metaObject; }
	virtual eCtorMetaType GetMetaTypeCtor() const { return eCtorMetaType::eCtorMetaType_RecordKey; }

protected:
	IMetaObjectRegisterData* m_metaObject;
};

#define registerRecordKey()\
	m_metaData->RegisterCtor(new CMetaValueRecordKeyTypeCtor(this))
#define unregisterRecordKey()\
	m_metaData->UnRegisterCtor(generate_class_name(prefixRecordKey))

//record manager class
class CMetaValueRecordManagerTypeCtor : public IMetaValueTypeCtor {
	class_identifier_t m_classType;
public:

	CMetaValueRecordManagerTypeCtor(IMetaObjectRegisterData* recordRef) : IMetaValueTypeCtor(), m_metaObject(recordRef) {
		m_classType = string_to_clsid(wxT("D_") +
			stringUtils::IntToStr(m_metaObject->GetMetaID()));
	}

	virtual wxString GetClassName() const {
		return m_metaObject->GetClassName() + prefixRecordManager + m_metaObject->GetName();
	}

	virtual class_identifier_t GetClassType() const { return m_classType; }
	virtual wxClassInfo* GetClassInfo() const;
	virtual CValue* CreateObject() const;
	virtual IMetaObject* GetMetaObject() const { return m_metaObject; }
	virtual eCtorMetaType GetMetaTypeCtor() const { return eCtorMetaType::eCtorMetaType_RecordManager; }

protected:
	IMetaObjectRegisterData* m_metaObject;
};

#define registerRecordManager()\
	m_metaData->RegisterCtor(new CMetaValueRecordManagerTypeCtor(this))
#define unregisterRecordManager()\
	m_metaData->UnRegisterCtor(generate_class_name(prefixRecordManager))

//record set class
class CMetaValueRecordSetTypeCtor : public IMetaValueTypeCtor {
	class_identifier_t m_classType;
public:

	CMetaValueRecordSetTypeCtor(IMetaObjectRegisterData* recordRef) : IMetaValueTypeCtor(), m_metaObject(recordRef) {
		m_classType = string_to_clsid(wxT("H_") +
			stringUtils::IntToStr(m_metaObject->GetMetaID()));
	}

	virtual wxString GetClassName() const {
		return m_metaObject->GetClassName() + prefixRecordSet + m_metaObject->GetName();
	}

	virtual class_identifier_t GetClassType() const { return m_classType; }
	virtual wxClassInfo* GetClassInfo() const;
	virtual CValue* CreateObject() const;
	virtual IMetaObject* GetMetaObject() const { return m_metaObject; }
	virtual eCtorMetaType GetMetaTypeCtor() const { return eCtorMetaType::eCtorMetaType_RecordSet; }

protected:
	IMetaObjectRegisterData* m_metaObject;
};

#define registerRecordSet()\
	m_metaData->RegisterCtor(new CMetaValueRecordSetTypeCtor(this))
#define unregisterRecordSet()\
	m_metaData->UnRegisterCtor(generate_class_name(prefixRecordSet))

//record set string class
class CMetaValueRecordSetStringTypeCtor : public IMetaValueTypeCtor {
	class_identifier_t m_classType;
public:

	CMetaValueRecordSetStringTypeCtor(IMetaObjectRegisterData* recordRef) : IMetaValueTypeCtor(), m_metaObject(recordRef) {
		m_classType = string_to_clsid(wxT("P_") +
			stringUtils::IntToStr(m_metaObject->GetMetaID()));
	}

	virtual wxString GetClassName() const {
		return m_metaObject->GetClassName() + prefixRecordSetStr + m_metaObject->GetName();
	}

	virtual class_identifier_t GetClassType() const { return m_classType; }
	virtual wxClassInfo* GetClassInfo() const { return nullptr; }
	virtual CValue* CreateObject() const { return nullptr; }
	virtual IMetaObject* GetMetaObject() const { return m_metaObject; }
	virtual eCtorMetaType GetMetaTypeCtor() const { return eCtorMetaType::eCtorMetaType_RecordSet_String; }

protected:
	IMetaObjectRegisterData* m_metaObject;
};

#define registerRecordSet_String()\
	m_metaData->RegisterCtor(new CMetaValueRecordSetStringTypeCtor(this))
#define unregisterRecordSet_String()\
	m_metaData->UnRegisterCtor(generate_class_name(prefixRecordSetStr))

#endif 
