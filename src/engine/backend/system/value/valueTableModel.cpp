////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : table models 
////////////////////////////////////////////////////////////////////////////

#include "valueTable.h"

//***********************************************************************************
//*                                  Model                                          *
//***********************************************************************************

void CValueTableMemory::GetValueByRow(wxVariant& variant,
	const wxDataViewItem& row, unsigned int col) const
{
	wxValueTableRow* node = GetViewData<wxValueTableRow>(row);
	if (node == nullptr)
		return;
	node->GetValue(col, variant);
}

bool CValueTableMemory::SetValueByRow(const wxVariant& variant,
	const wxDataViewItem& row, unsigned int col)
{
	const wxString& strData = variant.GetString();
	wxValueTableRow* node = GetViewData<wxValueTableRow>(row);
	if (node == nullptr)
		return false;
	const CTypeDescription& typeDescription =
		m_tableColumnCollection->GetColumnType(col);
	CValue cValue; node->GetValue(col, cValue);
	if (strData.Length() > 0) {
		std::vector<CValue> listValue;
		if (cValue.FindValue(strData, listValue)) {
			node->SetValue(
				col, CValueTypeDescription::AdjustValue(typeDescription, listValue.at(0))
			);
		}
		else {
			return false;
		}
	}
	else {
		node->SetValue(
			col, CValueTypeDescription::AdjustValue(typeDescription)
		);
	}

	return true;
}