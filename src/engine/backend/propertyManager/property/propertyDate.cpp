#include "propertyDate.h"

//base property for "date"
bool CPropertyDate::SetDataValue(const CValue& varPropVal)
{
	CPropertyDate::SetValue(varPropVal.GetDate());
	return true;
}

bool CPropertyDate::GetDataValue(CValue& pvarPropVal) const
{
	pvarPropVal = CPropertyDate::GetValueAsDateTime();
	return true;
}

bool CPropertyDate::LoadData(CMemoryReader& reader)
{
	CPropertyDate::SetValue((wxLongLong_t)reader.r_u64());
	return true;
}

bool CPropertyDate::SaveData(CMemoryWriter& writer)
{
	writer.w_u64(CPropertyDate::GetValueAsDateTime());
	return true;
}