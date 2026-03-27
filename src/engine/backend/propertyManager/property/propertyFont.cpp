#include "propertyFont.h"
#include "backend/system/value/valueFont.h"

//base property for "colour"
bool ibPropertyFont::SetDataValue(const ibValue& varPropVal)
{
	ibValueFont *valueFont = varPropVal.ConvertToType<ibValueFont>();
	if (valueFont == nullptr)
		return false;
	SetValue(valueFont->m_font);
	return true;
}

bool ibPropertyFont::GetDataValue(ibValue& pvarPropVal) const
{
	pvarPropVal = ibValue::CreateObjectValue<ibValueFont>(GetValueAsFont());
	return true;
}

bool ibPropertyFont::LoadData(ibReaderMemory& reader)
{
	ibPropertyFont::SetValue(reader.r_stringZ());
	return true;
}

bool ibPropertyFont::SaveData(ibWriterMemory& writer)
{
	writer.w_stringZ(ibPropertyFont::GetValueAsString());
	return true;
}