#include "backend/metaCollection/partial/reference/reference.h"
#include "backend/metaCollection/partial/list/objectList.h"
#include "backend/metaCollection/partial/constant.h"

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
	
	return CValue::CreateAndPrepareValueRef<CListDataObjectRef>((IMetaObjectRecordDataMutableRef *)m_metaObject);
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
	IRecordDataObject* dataObject = m_metaObject->CreateRecordDataObject();
	wxASSERT(dataObject); wxClassInfo* classInfo = dataObject->GetClassInfo();
	wxDELETE(dataObject); return classInfo;
}

CValue* CMetaValueObjectTypeCtor::CreateObject() const
{
	return m_metaObject->CreateRecordDataObject();
}

//const-object class
wxClassInfo* CMetaValueConstantObjectTypeCtor::GetClassInfo() const
{
	return CLASSINFO(CRecordDataObjectConstant);
}

CValue* CMetaValueConstantObjectTypeCtor::CreateObject() const
{
	CMetaObjectConstant* metaConstValue = nullptr;
	if (ConvertToMetaValue(metaConstValue))
		return metaConstValue->CreateObjectValue();
	wxASSERT(metaConstValue == nullptr);
	return nullptr;
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
	IRecordManagerObject* dataObject = m_metaObject->CreateRecordManagerObjectValue();
	wxASSERT(dataObject); wxClassInfo* classInfo = dataObject->GetClassInfo();
	wxDELETE(dataObject); return classInfo;
}

CValue* CMetaValueRecordManagerTypeCtor::CreateObject() const
{
	return m_metaObject->CreateRecordManagerObjectValue();
}

//object record set
wxClassInfo* CMetaValueRecordSetTypeCtor::GetClassInfo() const
{
	IRecordSetObject* dataObject = m_metaObject->CreateRecordSetObjectValue();
	wxASSERT(dataObject); wxClassInfo* classInfo = dataObject->GetClassInfo();
	wxDELETE(dataObject); return classInfo;
}

CValue* CMetaValueRecordSetTypeCtor::CreateObject() const
{
	return m_metaObject->CreateRecordSetObjectValue();
}