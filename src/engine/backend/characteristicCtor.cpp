#include "backend/metaCollection/partial/chartOfCharacteristicTypes.h"

#include "characteristicCtor.h"

//characteristic class
wxClassInfo* ibCtorMetaValueTypeCharacteristic::GetClassInfo() const
{
	return CLASSINFO(ibValueRecordDataObjectChartOfCharacteristicTypes);
}

ibValue* ibCtorMetaValueTypeCharacteristic::CreateObject() const
{
	return m_metaObject->CreateValueRef();
}