#include "backend/metaCollection/partial/constant.h"
#include "backend/metaCollection/partial/constantManager.h"

#include "constantCtor.h"

//const-object class
wxClassInfo* ibCtorMetaValueTypeConstantObject::GetClassInfo() const
{
	return CLASSINFO(ibValueRecordDataObjectConstant);
}

ibValue* ibCtorMetaValueTypeConstantObject::CreateObject() const
{
	return m_metaObject->CreateRecordDataObjectValue();
}

//const-manager class
wxClassInfo* ibCtorMetaValueTypeConstantManager::GetClassInfo() const
{
	return CLASSINFO(ibValueManagerDataObjectConstant);
}

ibValue* ibCtorMetaValueTypeConstantManager::CreateObject() const
{
	return ibValue::CreateAndPrepareValueRef<ibValueManagerDataObjectConstant>(m_metaObject);
}