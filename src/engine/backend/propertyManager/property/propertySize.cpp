#include "propertySize.h"
#include "backend/system/value/valueSize.h"

wxObject* (*ibPropertySize::ms_propertySize)(const wxString&, const wxString&, const wxSize&) = nullptr;

//base property for "size"
bool ibPropertySize::SetDataValue(const ibValue& varPropVal)
{
	ibValueSize* valueSize = varPropVal.ConvertToType<ibValueSize>();
	if (valueSize == nullptr)
		return false;
	SetValue(valueSize->m_size);
	return true;
}

bool ibPropertySize::GetDataValue(ibValue& pvarPropVal) const
{
	pvarPropVal = ibValue::CreateObjectValue <ibValueSize>(GetValueAsSize());
	return true;
}

bool ibPropertySize::LoadData(ibReaderMemory& reader)
{
	ibPropertySize::SetValue(reader.r_stringZ());
	return true;
}

bool ibPropertySize::SaveData(ibWriterMemory& writer)
{
	writer.w_stringZ(ibPropertySize::GetValueAsString());
	return true;
}