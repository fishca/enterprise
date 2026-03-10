////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : tabular sections - model
////////////////////////////////////////////////////////////////////////////

#include "tabularSection.h"

//***********************************************************************************
//*                                  Model                                          *
//***********************************************************************************

void IValueTabularSectionDataObject::GetValueByRow(wxVariant& variant,
	const wxDataViewExtItem& row, unsigned int col) const
{
	wxValueTableRow* node = GetViewData<wxValueTableRow>(row);
	if (node == nullptr) return;

	if (m_metaTable->IsNumberLine(col))
		variant = new wxVariantDataValueNumberLine(GetRow(row) + 1);
	else if (node->HasColumnValue(col))
		node->GetValue(col, variant);
}

#include "backend/metaData.h"

bool IValueTabularSectionDataObject::SetValueByRow(const wxVariant& variant,
	const wxDataViewExtItem& row, unsigned int col)
{
	wxValueTableRow* node = GetViewData<wxValueTableRow>(row);
	if (node == nullptr) return false;

	if (!m_metaTable->IsNumberLine(col)) {
		IMetaData* metaData = m_metaTable->GetMetaData();
		wxASSERT(metaData);
		if (node->HasColumnValue(col)) {

			const wxString& strData = variant.GetString();

			const CValue& selValue = node->GetTableValue(col);
			const CValue& newValue = metaData->CreateObject(selValue.GetClassType());

			if (strData.Length() > 0) {
				std::vector<CValue> listValue;
				if (newValue.FindValue(strData, listValue)) {
					SetValueByMetaID(row, col, listValue.at(0));
				}
				else {
					return false;
				}
			}
			else {
				SetValueByMetaID(row, col, newValue);
			}
		}
		else {
			return false;
		}
	};

	RefreshTabularSection();
	return true;
}

void IValueTabularSectionDataObject::AddValue(unsigned int before)
{
	long row = GetRow(GetSelection());
	if (row > 0) AppendRow(row);
	else AppendRow();

	RefreshTabularSection();
}

void IValueTabularSectionDataObject::CopyValue()
{
	const wxDataViewExtItem& currentItem = GetSelection();
	if (!currentItem.IsOk())
		return;
	wxValueTableRow* node = GetViewData<wxValueTableRow>(currentItem);
	if (node == nullptr)
		return;
	wxValueTableRow* rowData = new wxValueTableRow();
	for (const auto object : m_metaTable->GetAttributeArrayObject()) {
		if (!m_metaTable->IsNumberLine(object->GetMetaID())) {
			rowData->AppendTableValue(object->GetMetaID(), node->GetTableValue(object->GetMetaID()));
		}
		else {
			rowData->AppendTableValue(object->GetMetaID(), CValue());
		}
	}
	long currentLine = GetRow(currentItem);
	if (currentLine != wxNOT_FOUND) {
		IValueTable::Insert(rowData, currentLine, !CBackendException::IsEvalMode());
	}
	else {
		IValueTable::Append(rowData, !CBackendException::IsEvalMode());
	}

	RefreshTabularSection();
}

void IValueTabularSectionDataObject::EditValue()
{
	const wxDataViewExtItem& currentItem = GetSelection();
	if (!currentItem.IsOk())
		return;

	if (m_modelProvider != nullptr) {

		if (m_metaTable->IsNumberLine(m_modelProvider->GetCurrentModelColumn()))
			return;

		IValueTable::RowValueStartEdit(currentItem, m_modelProvider->GetCurrentModelColumn());
	}
	else {
		IValueTable::RowValueStartEdit(currentItem);
	}

	RefreshTabularSection();
}

void IValueTabularSectionDataObject::DeleteValue()
{
	const wxDataViewExtItem& currentItem = GetSelection();
	if (!currentItem.IsOk())
		return;
	wxValueTableRow* node = GetViewData<wxValueTableRow>(currentItem);
	if (node == nullptr)
		return;

	RefreshTabularSection();

	if (!CBackendException::IsEvalMode()) {
		IValueTable::Remove(node);
	}
}

void CValueTabularSectionDataObjectRef::CopyValue()
{
	IValueTabularSectionDataObject::CopyValue();

	if (!CBackendException::IsEvalMode()) {
		IBackendValueForm* const foundedForm = IBackendValueForm::FindFormByUniqueKey(
			m_objectValue->GetGuid()
		);
		if (foundedForm != nullptr) {
			foundedForm->Modify(true);
		}
	}

	RefreshTabularSection();
}

void CValueTabularSectionDataObjectRef::DeleteValue()
{
	IValueTabularSectionDataObject::DeleteValue();

	if (!CBackendException::IsEvalMode()) {
		IBackendValueForm* const foundedForm = IBackendValueForm::FindFormByUniqueKey(m_objectValue->GetGuid());
		if (foundedForm != nullptr) foundedForm->Modify(true);
	}
}