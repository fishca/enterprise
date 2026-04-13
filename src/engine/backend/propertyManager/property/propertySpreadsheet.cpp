#include "propertySpreadsheet.h"
#include "backend/propertyManager/property/variant/variantSpreadsheet.h"

wxObject* (*ibPropertySpreadsheet::ms_propertySpreadsheet)(ibPropertyObject*, const wxString&, const wxString&, const wxVariant&) = nullptr;

////////////////////////////////////////////////////////////////////////

wxVariantData* ibPropertySpreadsheet::CreateVariantData(const ibSpreadsheetDescription& val)
{
	return new ibVariantDataSpreadsheet(val);
}

////////////////////////////////////////////////////////////////////////

ibSpreadsheetDescription& ibPropertySpreadsheet::GetValueAsSpreadsheetDesc() const
{
	return get_cell_variant<ibVariantDataSpreadsheet>()->GetSpreadsheetDesc();
}

void ibPropertySpreadsheet::SetValue(const ibSpreadsheetDescription& val)
{
	m_propValue = CreateVariantData(val);
}

////////////////////////////////////////////////////////////////////////

bool ibPropertySpreadsheet::IsEmptyProperty() const
{
	return get_cell_variant<ibVariantDataSpreadsheet>()->IsEmptySpreadsheet();
}

////////////////////////////////////////////////////////////////////////

#include "backend/system/value/valueSpreadsheet.h"

//base property for "template"
bool ibPropertySpreadsheet::SetDataValue(const ibValue& varPropVal)
{
	ibValueSpreadsheetDocument* valueType = varPropVal.ConvertToType<ibValueSpreadsheetDocument>();
	if (valueType != nullptr) {
		SetValue(valueType->GetSpreadsheetDesc());
		return true;
	}
	return false;
}

bool ibPropertySpreadsheet::GetDataValue(ibValue& pvarPropVal) const
{
	pvarPropVal = ibValue::CreateObjectValue<ibValueSpreadsheetDocument>(GetValueAsSpreadsheetDesc());
	return true;
}

bool ibPropertySpreadsheet::LoadData(ibReaderMemory& reader)
{
	return ibSpreadsheetDescriptionMemory::LoadData(reader, GetValueAsSpreadsheetDesc());
}

bool ibPropertySpreadsheet::SaveData(ibWriterMemory& writer)
{
	return ibSpreadsheetDescriptionMemory::SaveData(writer, GetValueAsSpreadsheetDesc());
}