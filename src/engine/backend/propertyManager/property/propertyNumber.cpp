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
	return GetValueAsNumber().SetBuffer(reader);
}

bool ibPropertyNumber::SaveData(ibWriterMemory& writer)
{
	return GetValueAsNumber().GetBuffer(writer);
}