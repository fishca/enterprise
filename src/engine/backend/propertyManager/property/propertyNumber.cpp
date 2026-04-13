#include "propertyNumber.h"
#include "backend/propertyManager/property/variant/variantNumber.h"

wxVariantData* CPropertyNumber::CreateVariantData(const number_t& val)
{
	return new wxVariantDataNumber(val);
}

////////////////////////////////////////////////////////////////////////

number_t& CPropertyNumber::GetValueAsNumber() const
{
	return get_cell_variant<wxVariantDataNumber>()->GetNumber();
}

void CPropertyNumber::SetValue(const number_t& val)
{
	m_propValue = CreateVariantData(val);
}

//base property for "number"
bool CPropertyNumber::SetDataValue(const CValue& varPropVal)
{
	SetValue(varPropVal.GetNumber());
	return true;
}

bool CPropertyNumber::GetDataValue(CValue& pvarPropVal) const
{
	pvarPropVal = CPropertyNumber::GetValueAsNumber();
	return true;
}

bool CPropertyNumber::LoadData(CMemoryReader& reader)
{
	number_t& value = GetValueAsNumber();
	reader.r(&value.exponent, sizeof(value.exponent));
	reader.r(&value.mantissa, sizeof(value.mantissa));
	reader.r(&value.info, sizeof(value.info));
	return true;
}

bool CPropertyNumber::SaveData(CMemoryWriter& writer)
{
	const number_t& value = GetValueAsNumber();
	writer.w(&value.exponent, sizeof(value.exponent));
	writer.w(&value.mantissa, sizeof(value.mantissa));
	writer.w(&value.info, sizeof(value.info));
	return true;
}