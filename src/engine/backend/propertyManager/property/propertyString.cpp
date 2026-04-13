#include "propertyString.h"

//base property for "string"
bool IPropertyString::SetDataValue(const CValue& varPropVal)
{
	IPropertyString::SetValue(varPropVal.GetString());
	return true;
}

bool IPropertyString::GetDataValue(CValue& pvarPropVal) const
{
	pvarPropVal = IPropertyString::GetValueAsString();
	return true;
}

bool IPropertyString::LoadData(CMemoryReader& reader)
{
	IPropertyString::SetValue(reader.r_stringZ());
	return true;
}

bool IPropertyString::SaveData(CMemoryWriter& writer)
{
	writer.w_stringZ(IPropertyString::GetValueAsString());
	return true;
}
