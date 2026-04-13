#include "propertyFont.h"
#include "backend/system/value/valueFont.h"

//base property for "colour"
bool CPropertyFont::SetDataValue(const CValue& varPropVal)
{
	CValueFont *valueFont = varPropVal.ConvertToType<CValueFont>();
	if (valueFont == nullptr)
		return false;
	SetValue(valueFont->m_font);
	return true;
}

bool CPropertyFont::GetDataValue(CValue& pvarPropVal) const
{
	pvarPropVal = CValue::CreateObjectValue<CValueFont>(GetValueAsFont());
	return true;
}

bool CPropertyFont::LoadData(CMemoryReader& reader)
{
	CPropertyFont::SetValue(reader.r_stringZ());
	return true;
}

bool CPropertyFont::SaveData(CMemoryWriter& writer)
{
	writer.w_stringZ(CPropertyFont::GetValueAsString());
	return true;
}