#include "propertySpreadsheet.h"
#include "backend/propertyManager/property/variant/variantSpreadsheet.h"

////////////////////////////////////////////////////////////////////////

wxVariantData* CPropertySpreadsheet::CreateVariantData(const CSpreadsheetDescription& val)
{
	return new wxVariantDataSpreadsheet(val);
}

////////////////////////////////////////////////////////////////////////

CSpreadsheetDescription& CPropertySpreadsheet::GetValueAsSpreadsheetDesc() const
{
	return get_cell_variant<wxVariantDataSpreadsheet>()->GetSpreadsheetDesc();
}

void CPropertySpreadsheet::SetValue(const CSpreadsheetDescription& val)
{
	m_propValue = CreateVariantData(val);
}

////////////////////////////////////////////////////////////////////////

bool CPropertySpreadsheet::IsEmptyProperty() const
{
	return get_cell_variant<wxVariantDataSpreadsheet>()->IsEmptySpreadsheet();
}

////////////////////////////////////////////////////////////////////////

#include "backend/system/value/valueSpreadsheet.h"

//base property for "template"
bool CPropertySpreadsheet::SetDataValue(const CValue& varPropVal)
{
	CValueSpreadsheetDocument* valueType = varPropVal.ConvertToType<CValueSpreadsheetDocument>();
	if (valueType != nullptr) {
		SetValue(valueType->GetSpreadsheetDesc());
		return true;
	}
	return false;
}

bool CPropertySpreadsheet::GetDataValue(CValue& pvarPropVal) const
{
	pvarPropVal = CValue::CreateObjectValue<CValueSpreadsheetDocument>(GetValueAsSpreadsheetDesc());
	return true;
}

bool CPropertySpreadsheet::LoadData(CMemoryReader& reader)
{
	return CSpreadsheetDescriptionMemory::LoadData(reader, GetValueAsSpreadsheetDesc());
}

bool CPropertySpreadsheet::SaveData(CMemoryWriter& writer)
{
	return CSpreadsheetDescriptionMemory::SaveData(writer, GetValueAsSpreadsheetDesc());
}