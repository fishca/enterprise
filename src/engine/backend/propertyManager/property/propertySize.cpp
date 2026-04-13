#include "propertySize.h"
#include "backend/system/value/valueSize.h"

//base property for "size"
bool CPropertySize::SetDataValue(const CValue& varPropVal)
{
	CValueSize* valueSize = varPropVal.ConvertToType<CValueSize>();
	if (valueSize == nullptr)
		return false;
	SetValue(valueSize->m_size);
	return true;
}

bool CPropertySize::GetDataValue(CValue& pvarPropVal) const
{
	pvarPropVal = CValue::CreateObjectValue <CValueSize>(GetValueAsSize());
	return true;
}

bool CPropertySize::LoadData(CMemoryReader& reader)
{
	CPropertySize::SetValue(reader.r_stringZ());
	return true;
}

bool CPropertySize::SaveData(CMemoryWriter& writer)
{
	writer.w_stringZ(CPropertySize::GetValueAsString());
	return true;
}