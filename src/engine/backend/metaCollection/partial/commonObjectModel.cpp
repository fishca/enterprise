////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : tabular sections - model
////////////////////////////////////////////////////////////////////////////

#include "commonObject.h"

//***********************************************************************************
//*                                  Model                                          *
//***********************************************************************************

void IValueRecordSetObject::GetValueByRow(wxVariant& variant,
	const wxDataViewItem& row, unsigned int col) const
{
	wxValueTableRow* node = GetViewData<wxValueTableRow>(row);
	if (node == nullptr)
		return;
	node->GetValue(col, variant);
}

#include "backend/metaData.h"

bool IValueRecordSetObject::SetValueByRow(const wxVariant& variant,
	const wxDataViewItem& row, unsigned int col)
{
	const wxString &strData = variant.GetString();
	wxValueTableRow* node = GetViewData<wxValueTableRow>(row);
	if (node == nullptr)
		return false;

	IValueMetaObjectRegisterData* metaObject = GetMetaObject();
	wxASSERT(metaObject);
	IValueMetaObjectAttribute* metaObjectAttribute = metaObject->FindAnyAttributeObjectByFilter(col);
	wxASSERT(metaObject);
	CValue newValue = metaObjectAttribute->CreateValue();
	if (strData.Length() > 0) {
		std::vector<CValue> listValue;
		if (newValue.FindValue(strData, listValue)) {
			return node->SetValue(col, listValue.at(0));
		}
		else {
			return false;
		}
	}
	else {
		return node->SetValue(col, newValue);
	}


	return false;
}