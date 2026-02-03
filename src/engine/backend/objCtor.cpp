#include "backend/metaCollection/partial/reference/reference.h"
#include "backend/metaCollection/partial/list/objectList.h"

#include "objCtor.h"

//reference class 
wxClassInfo* CMetaValueRefTypeCtor::GetClassInfo() const
{
	return CLASSINFO(CReferenceDataObject);
}

CValue* CMetaValueRefTypeCtor::CreateObject() const
{
	return CValue::CreateAndPrepareValueRef<CReferenceDataObject>(m_metaObject);
}

//list class 
wxClassInfo* CMetaValueListRefTypeCtor::GetClassInfo() const
{
	IMetaObjectRecordDataHierarchyMutableRef* folderRef = nullptr;
	IMetaObjectRecordDataEnumRef* enumRef = nullptr;
	if (m_metaObject->ConvertToValue(folderRef)) {
		return CLASSINFO(CTreeDataObjectFolderRef);
	}
	else if (m_metaObject->ConvertToValue(enumRef)) {
		return CLASSINFO(CListDataObjectEnumRef);
	}
	return CLASSINFO(CListDataObjectRef);
}

CValue* CMetaValueListRefTypeCtor::CreateObject() const
{
	IMetaObjectRecordDataHierarchyMutableRef* folderRef = nullptr;
	IMetaObjectRecordDataEnumRef* enumRef = nullptr;

	if (m_metaObject->ConvertToValue(folderRef)) {
		return CValue::CreateAndPrepareValueRef<CTreeDataObjectFolderRef>(folderRef);
	}
	else if (m_metaObject->ConvertToValue(enumRef)) {
		return CValue::CreateAndPrepareValueRef<CListDataObjectEnumRef>(enumRef);
	}

	return CValue::CreateAndPrepareValueRef<CListDataObjectRef>((IMetaObjectRecordDataMutableRef*)m_metaObject);
}

wxClassInfo* CMetaValueListRegisterTypeCtor::GetClassInfo() const
{
	return CLASSINFO(CListRegisterObject);
}

CValue* CMetaValueListRegisterTypeCtor::CreateObject() const
{
	return CValue::CreateAndPrepareValueRef<CListRegisterObject>(m_metaObject);
}

//object class
wxClassInfo* CMetaValueObjectTypeCtor::GetClassInfo() const
{
	CValuePtr<IRecordDataObject> recordDataObjectValue = 
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
	CValuePtr<IManagerDataObject> managerDataObject = 
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
	return CLASSINFO(CRecordKeyObject);
}

CValue* CMetaValueRecordKeyTypeCtor::CreateObject() const
{
	return m_metaObject->CreateRecordKeyObjectValue();
}

//object record manager
wxClassInfo* CMetaValueRecordManagerTypeCtor::GetClassInfo() const
{
	CValuePtr<IRecordManagerObject> recordManagerObject = 
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
	CValuePtr<IRecordSetObject> recordSetObject = 
		m_metaObject->CreateRecordSetObjectValue();
	return recordSetObject->GetClassInfo();
}

CValue* CMetaValueRecordSetTypeCtor::CreateObject() const
{
	return m_metaObject->CreateRecordSetObjectValue();
}