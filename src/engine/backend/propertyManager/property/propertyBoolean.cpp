#include "propertyBoolean.h"

//base property for "bool"
bool CPropertyBoolean::SetDataValue(const CValue& varPropVal)
{
	SetValue(varPropVal.GetBoolean());
	return true;
}

bool CPropertyBoolean::GetDataValue(CValue& pvarPropVal) const
{
	pvarPropVal = CPropertyBoolean::GetValueAsBoolean();
	return true;
}

bool CPropertyBoolean::LoadData(CMemoryReader& reader)
{
	SetValue(reader.r_u8());
	return true;
}

bool CPropertyBoolean::SaveData(CMemoryWriter& writer)
{
	writer.w_u8(CPropertyBoolean::GetValueAsBoolean());
	return true;
}