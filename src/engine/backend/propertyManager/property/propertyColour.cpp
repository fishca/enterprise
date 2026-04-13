#include "propertyColour.h"
#include "backend/system/value/valueColour.h"

//base property for "colour"
bool CPropertyColour::SetDataValue(const CValue& varPropVal)
{
	CValueColour *valueColour = varPropVal.ConvertToType<CValueColour>();
	if (valueColour == nullptr) 
		return false;
	SetValue(valueColour->m_colour);
	return true;
}

bool CPropertyColour::GetDataValue(CValue& pvarPropVal) const
{
	pvarPropVal = CValue::CreateObjectValue<CValueColour>(GetValueAsColour());
	return true;
}

bool CPropertyColour::LoadData(CMemoryReader& reader)
{
	SetValue(reader.r_stringZ());
	return true;
}

bool CPropertyColour::SaveData(CMemoryWriter& writer)
{
	writer.w_stringZ(CPropertyColour::GetValueAsString());
	return true;
}