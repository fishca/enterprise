#include "backend/metaCollection/partial/constant.h"
#include "backend/metaCollection/partial/constantManager.h"

#include "constantCtor.h"

//const-object class
wxClassInfo* CMetaValueConstantObjectTypeCtor::GetClassInfo() const
{
	return CLASSINFO(CValueRecordDataObjectConstant);
}

CValue* CMetaValueConstantObjectTypeCtor::CreateObject() const
{
	return m_metaObject->CreateRecordDataObjectValue();
}

//const-manager class
wxClassInfo* CMetaValueConstManagerTypeCtor::GetClassInfo() const
{
	return CLASSINFO(CValueManagerDataObjectConstant);
}

CValue* CMetaValueConstManagerTypeCtor::CreateObject() const
{
	return CValue::CreateAndPrepareValueRef<CValueManagerDataObjectConstant>(m_metaObject);
}