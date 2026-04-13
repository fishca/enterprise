#include "propertyBoolean.h"

//get property for grid	
wxObject* (*ibPropertyBoolean::ms_propertyBoolean)(const wxString&, const wxString&, const bool&) = nullptr;

//base property for "bool"
bool ibPropertyBoolean::SetDataValue(const ibValue& varPropVal)
{
	SetValue(varPropVal.GetBoolean());
	return true;
}

bool ibPropertyBoolean::GetDataValue(ibValue& pvarPropVal) const
{
	pvarPropVal = ibPropertyBoolean::GetValueAsBoolean();
	return true;
}

bool ibPropertyBoolean::LoadData(ibReaderMemory& reader)
{
	SetValue(reader.r_u8());
	return true;
}

bool ibPropertyBoolean::SaveData(ibWriterMemory& writer)
{
	writer.w_u8(ibPropertyBoolean::GetValueAsBoolean());
	return true;
}