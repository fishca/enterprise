#include "propertyString.h"

//base property for "string"
bool ibPropertyStringBase::SetDataValue(const ibValue& varPropVal)
{
	ibPropertyStringBase::SetValue(varPropVal.GetString());
	return true;
}

bool ibPropertyStringBase::GetDataValue(ibValue& pvarPropVal) const
{
	pvarPropVal = ibPropertyStringBase::GetValueAsString();
	return true;
}

bool ibPropertyStringBase::LoadData(ibReaderMemory& reader)
{
	ibPropertyStringBase::SetValue(reader.r_stringZ());
	return true;
}

bool ibPropertyStringBase::SaveData(ibWriterMemory& writer)
{
	writer.w_stringZ(ibPropertyStringBase::GetValueAsString());
	return true;
}
