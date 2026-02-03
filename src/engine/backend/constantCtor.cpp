#include "backend/metaCollection/partial/constant.h"
#include "backend/metaCollection/partial/constantManager.h"

#include "constantCtor.h"

//const-object class
wxClassInfo* CMetaValueConstantObjectTypeCtor::GetClassInfo() const
{
	return CLASSINFO(CRecordDataObjectConstant);
}

CValue* CMetaValueConstantObjectTypeCtor::CreateObject() const
{
	return m_metaObject->CreateRecordDataObjectValue();
}

//const-manager class
wxClassInfo* CMetaValueConstManagerTypeCtor::GetClassInfo() const
{
	return CLASSINFO(CManagerDataObjectConstant);
}

CValue* CMetaValueConstManagerTypeCtor::CreateObject() const
{
	return CValue::CreateAndPrepareValueRef<CManagerDataObjectConstant>(m_metaObject);
}