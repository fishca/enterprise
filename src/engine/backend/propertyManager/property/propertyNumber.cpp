#include "propertyNumber.h"
#include "backend/propertyManager/property/variant/variantNumber.h"

//get property for grid 
wxObject* (*ibPropertyNumber::ms_propertyNumber)(const wxString&, const wxString&, const ibNumber&) = nullptr;
wxObject* (*ibPropertyInteger::ms_propertyInteger)(const wxString&, const wxString&, const int&) = nullptr;
wxObject* (*ibPropertyUInteger::ms_propertyUInteger)(const wxString&, const wxString&, const unsigned int&) = nullptr;

////////////////////////////////////////////////////////////////////////

wxVariantData* ibPropertyNumber::CreateVariantData(const ibNumber& val)
{
	return new ibVariantDataNumber(val);
}

////////////////////////////////////////////////////////////////////////

ibNumber& ibPropertyNumber::GetValueAsNumber() const
{
	return get_cell_variant<ibVariantDataNumber>()->GetNumber();
}

void ibPropertyNumber::SetValue(const ibNumber& val)
{
	m_propValue = CreateVariantData(val);
}

//base property for "number"
bool ibPropertyNumber::SetDataValue(const ibValue& varPropVal)
{
	SetValue(varPropVal.GetNumber());
	return true;
}

bool ibPropertyNumber::GetDataValue(ibValue& pvarPropVal) const
{
	pvarPropVal = ibPropertyNumber::GetValueAsNumber();
	return true;
}

bool ibPropertyNumber::LoadData(ibReaderMemory& reader)
{
	ibNumber& value = GetValueAsNumber();
	reader.r(&value.exponent, sizeof(value.exponent));
	reader.r(&value.mantissa, sizeof(value.mantissa));
	reader.r(&value.info, sizeof(value.info));
	return true;
}

bool ibPropertyNumber::SaveData(ibWriterMemory& writer)
{
	const ibNumber& value = GetValueAsNumber();
	writer.w(&value.exponent, sizeof(value.exponent));
	writer.w(&value.mantissa, sizeof(value.mantissa));
	writer.w(&value.info, sizeof(value.info));
	return true;
}