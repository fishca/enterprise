#include "propertyColour.h"
#include "backend/system/value/valueColour.h"

//base property for "colour"
bool ibPropertyColour::SetDataValue(const ibValue& varPropVal)
{
	ibValueColour *valueColour = varPropVal.ConvertToType<ibValueColour>();
	if (valueColour == nullptr) 
		return false;
	SetValue(valueColour->m_colour);
	return true;
}

bool ibPropertyColour::GetDataValue(ibValue& pvarPropVal) const
{
	pvarPropVal = ibValue::CreateObjectValue<ibValueColour>(GetValueAsColour());
	return true;
}

bool ibPropertyColour::LoadData(ibReaderMemory& reader)
{
	SetValue(reader.r_stringZ());
	return true;
}

bool ibPropertyColour::SaveData(ibWriterMemory& writer)
{
	writer.w_stringZ(ibPropertyColour::GetValueAsString());
	return true;
}