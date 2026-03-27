////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : tabular sections - model
////////////////////////////////////////////////////////////////////////////

#include "tabularSection.h"

//***********************************************************************************
//*                                  Model                                          *
//***********************************************************************************

void ibValueTabularSectionDataObjectBase::GetValueByRow(wxVariant& variant,
	const ibDataViewItem& row, unsigned int col) const
{
	wxValueTableRow* node = GetViewData<wxValueTableRow>(row);
	if (node == nullptr) return;

	if (m_metaTable->IsNumberLine(col))
		variant = new ibVariantDataValueNumberLine(GetRow(row) + 1);
	else if (node->HasColumnValue(col))
		node->GetValue(col, variant);
}

#include "backend/metaData.h"

bool ibValueTabularSectionDataObjectBase::SetValueByRow(const wxVariant& variant,
	const ibDataViewItem& row, unsigned int col)
{
	wxValueTableRow* node = GetViewData<wxValueTableRow>(row);
	if (node == nullptr) return false;

	if (!m_metaTable->IsNumberLine(col)) {
		ibMetaData* metaData = m_metaTable->GetMetaData();
		wxASSERT(metaData);
		if (node->HasColumnValue(col)) {

			const wxString& strData = variant.GetString();

			const ibValue& selValue = node->GetTableValue(col);
			const ibValue& newValue = metaData->CreateObject(selValue.GetClassType());

			if (strData.Length() > 0) {
				std::vector<ibValue> listValue;
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

void ibValueTabularSectionDataObjectBase::AddValue(unsigned int before)
{
	long row = GetRow(GetSelection());
	if (row > 0) AppendRow(row);
	else AppendRow();

	RefreshTabularSection();
}

void ibValueTabularSectionDataObjectBase::CopyValue()
{
	const ibDataViewItem& currentItem = GetSelection();
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
			rowData->AppendTableValue(object->GetMetaID(), ibValue());
		}
	}
	long currentLine = GetRow(currentItem);
	if (currentLine != wxNOT_FOUND) {
		ibValueModelTable::Insert(rowData, currentLine, !ibBackendException::IsEvalMode());
	}
	else {
		ibValueModelTable::Append(rowData, !ibBackendException::IsEvalMode());
	}

	RefreshTabularSection();
}

void ibValueTabularSectionDataObjectBase::EditValue()
{
	const ibDataViewItem& currentItem = GetSelection();
	if (!currentItem.IsOk())
		return;

	if (m_modelProvider != nullptr) {

		if (m_metaTable->IsNumberLine(m_modelProvider->GetCurrentModelColumn()))
			return;

		ibValueModelTable::RowValueStartEdit(currentItem, m_modelProvider->GetCurrentModelColumn());
	}
	else {
		ibValueModelTable::RowValueStartEdit(currentItem);
	}

	RefreshTabularSection();
}

void ibValueTabularSectionDataObjectBase::DeleteValue()
{
	const ibDataViewItem& currentItem = GetSelection();
	if (!currentItem.IsOk())
		return;
	wxValueTableRow* node = GetViewData<wxValueTableRow>(currentItem);
	if (node == nullptr)
		return;

	RefreshTabularSection();

	if (!ibBackendException::IsEvalMode()) {
		ibValueModelTable::Remove(node);
	}
}

void ibValueTabularSectionDataObjectRef::CopyValue()
{
	ibValueTabularSectionDataObjectBase::CopyValue();

	if (!ibBackendException::IsEvalMode()) {
		ibBackendValueForm* const foundedForm = ibBackendValueForm::FindFormByUniqueKey(
			m_objectValue->GetGuid()
		);
		if (foundedForm != nullptr) {
			foundedForm->Modify(true);
		}
	}

	RefreshTabularSection();
}

void ibValueTabularSectionDataObjectRef::DeleteValue()
{
	ibValueTabularSectionDataObjectBase::DeleteValue();

	if (!ibBackendException::IsEvalMode()) {
		ibBackendValueForm* const foundedForm = ibBackendValueForm::FindFormByUniqueKey(m_objectValue->GetGuid());
		if (foundedForm != nullptr) foundedForm->Modify(true);
	}
}