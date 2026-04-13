#include "backend/metaCollection/partial/reference/reference.h"
#include "backend/metaCollection/partial/list/objectList.h"

#include "objCtor.h"

//reference class 
wxClassInfo* CMetaValueRefTypeCtor::GetClassInfo() const
{
	return CLASSINFO(CValueReferenceDataObject);
}

CValue* CMetaValueRefTypeCtor::CreateObject() const
{
	return CValue::CreateAndPrepareValueRef<CValueReferenceDataObject>(m_metaObject);
}

//list class 
wxClassInfo* CMetaValueListRefTypeCtor::GetClassInfo() const
{
	IValueMetaObjectRecordDataHierarchyMutableRef* folderRef = nullptr;
	IValueMetaObjectRecordDataEnumRef* enumRef = nullptr;
	if (m_metaObject->ConvertToValue(folderRef)) {
		return CLASSINFO(CValueTreeDataObjectFolderRef);
	}
	else if (m_metaObject->ConvertToValue(enumRef)) {
		return CLASSINFO(CValueListDataObjectEnumRef);
	}
	return CLASSINFO(CValueListDataObjectRef);
}

CValue* CMetaValueListRefTypeCtor::CreateObject() const
{
	IValueMetaObjectRecordDataHierarchyMutableRef* folderRef = nullptr;
	IValueMetaObjectRecordDataEnumRef* enumRef = nullptr;

	if (m_metaObject->ConvertToValue(folderRef)) {
		return CValue::CreateAndPrepareValueRef<CValueTreeDataObjectFolderRef>(folderRef);
	}
	else if (m_metaObject->ConvertToValue(enumRef)) {
		return CValue::CreateAndPrepareValueRef<CValueListDataObjectEnumRef>(enumRef);
	}

	return CValue::CreateAndPrepareValueRef<CValueListDataObjectRef>((IValueMetaObjectRecordDataMutableRef*)m_metaObject);
}

wxClassInfo* CMetaValueListRegisterTypeCtor::GetClassInfo() const
{
	return CLASSINFO(CValueListRegisterObject);
}

CValue* CMetaValueListRegisterTypeCtor::CreateObject() const
{
	return CValue::CreateAndPrepareValueRef<CValueListRegisterObject>(m_metaObject);
}

//object class
wxClassInfo* CMetaValueObjectTypeCtor::GetClassInfo() const
{
	CValuePtr<IValueRecordDataObject> recordDataObjectValue = 
		m_metaObject->CreateRecordDataObjectValue();
	return recordDataObjectValue->GetClassInfo();
}

CValue* CMetaValueObjectTypeCtor::CreateObject() const
{
	return m_metaObject->CreateRecordDataObjectValue();
}

//manager class
wxClassInfo* CMetaValueManagerTypeCtor::GetClassInfo() const
{
	CValuePtr<IValueManagerDataObject> managerDataObject = 
		m_metaObject->CreateManagerDataObjectValue();
	return managerDataObject->GetClassInfo();
}

CValue* CMetaValueManagerTypeCtor::CreateObject() const
{
	return m_metaObject->CreateManagerDataObjectValue();
}

//object record key
wxClassInfo* CMetaValueRecordKeyTypeCtor::GetClassInfo() const
{
	return CLASSINFO(CValueRecordKeyObject);
}

CValue* CMetaValueRecordKeyTypeCtor::CreateObject() const
{
	return m_metaObject->CreateRecordKeyObjectValue();
}

//object record manager
wxClassInfo* CMetaValueRecordManagerTypeCtor::GetClassInfo() const
{
	CValuePtr<IValueRecordManagerObject> recordManagerObject = 
		m_metaObject->CreateRecordManagerObjectValue();
	return recordManagerObject->GetClassInfo();
}

CValue* CMetaValueRecordManagerTypeCtor::CreateObject() const
{
	return m_metaObject->CreateRecordManagerObjectValue();
}

//object record set
wxClassInfo* CMetaValueRecordSetTypeCtor::GetClassInfo() const
{
	CValuePtr<IValueRecordSetObject> recordSetObject = 
		m_metaObject->CreateRecordSetObjectValue();
	return recordSetObject->GetClassInfo();
}

CValue* CMetaValueRecordSetTypeCtor::CreateObject() const
{
	return m_metaObject->CreateRecordSetObjectValue();
}