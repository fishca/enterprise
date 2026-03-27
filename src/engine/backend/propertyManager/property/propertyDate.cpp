#include "propertyDate.h"

//base property for "date"
bool ibPropertyDate::SetDataValue(const ibValue& varPropVal)
{
	ibPropertyDate::SetValue(varPropVal.GetDate());
	return true;
}

bool ibPropertyDate::GetDataValue(ibValue& pvarPropVal) const
{
	pvarPropVal = ibPropertyDate::GetValueAsDateTime();
	return true;
}

bool ibPropertyDate::LoadData(ibReaderMemory& reader)
{
	ibPropertyDate::SetValue((wxLongLong_t)reader.r_u64());
	return true;
}

bool ibPropertyDate::SaveData(ibWriterMemory& writer)
{
	writer.w_u64(ibPropertyDate::GetValueAsDateTime());
	return true;
}