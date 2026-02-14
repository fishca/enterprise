#include "gridEditor.h"

bool CGridEditor::AssociateDocument(const wxObjectDataPtr<CBackendSpreadsheetObject>& doc)
{
	if (m_spreadsheetObject != doc) {

		if (m_spreadsheetObject != nullptr)
			m_spreadsheetObject->RemoveNotifier(m_notifier);

		m_spreadsheetObject = doc;
		m_notifier = m_spreadsheetObject->AddNotifier<CGenericSpreadsheetNotifier>(this);
	}

	return true;
}

bool CGridEditor::GetActiveDocument(wxObjectDataPtr<CBackendSpreadsheetObject>& doc) const
{
	doc = m_spreadsheetObject;
	return true;
}

#pragma region file

bool CGridEditor::LoadDocument(const CSpreadsheetDescription& spreadsheetDesc)
{
	if (!LoadSpreadsheet(spreadsheetDesc))
		return false;

	wxObjectDataPtr<CBackendSpreadsheetObject> doc(
		new CBackendSpreadsheetObject(spreadsheetDesc));

	return AssociateDocument(doc);
}

bool CGridEditor::LoadDocument(const wxObjectDataPtr<CBackendSpreadsheetObject>& doc)
{
	const CSpreadsheetDescription& spreadsheetDesc = doc->GetSpreadsheetDesc();

	if (!spreadsheetDesc.IsEmptySpreadsheet())
	{
		wxGridExt::SetTable(
			new CGridEditorStringTable(spreadsheetDesc.GetNumberRows(), spreadsheetDesc.GetNumberCols()), true);

		wxGridExt::SetEvtHandlerEnabled(false);

		for (int row = 0; row < spreadsheetDesc.GetNumberRows(); row++) {

			for (int col = 0; col < spreadsheetDesc.GetNumberCols(); col++) {

				const CSpreadsheetCellDescription* cell = spreadsheetDesc.GetCell(row, col);
				if (cell == nullptr)
					continue;

				wxGridExtCellAttrPtr attr = GetOrCreateCellAttrPtr(row, col);
				attr->SetAlignment(cell->m_alignHorz, cell->m_alignVert);

				if (cell->m_row_size >= 0 && cell->m_col_size >= 0)
					SetCellSize(row, col, cell->m_row_size, cell->m_col_size, false);

				attr->SetTextOrient(cell->m_textOrient);
				attr->SetFont(cell->m_font);
				attr->SetBackgroundColour(cell->m_backgroundColour);
				attr->SetTextColour(cell->m_textColour);

				attr->SetBorderLeft(cell->m_borderAt[0].m_style, cell->m_borderAt[0].m_colour, cell->m_borderAt[0].m_width);
				attr->SetBorderRight(cell->m_borderAt[1].m_style, cell->m_borderAt[1].m_colour, cell->m_borderAt[1].m_width);
				attr->SetBorderTop(cell->m_borderAt[2].m_style, cell->m_borderAt[2].m_colour, cell->m_borderAt[2].m_width);
				attr->SetBorderBottom(cell->m_borderAt[3].m_style, cell->m_borderAt[3].m_colour, cell->m_borderAt[3].m_width);

				attr->SetFitMode(cell->m_fitMode == CSpreadsheetCellDescription::EFitMode::Mode_Overflow ? wxGridExtFitMode::Overflow() : wxGridExtFitMode::Clip());
				attr->SetReadOnly(cell->m_isReadOnly);

				wxSharedPtr<wxString> ptr = wxSharedPtr<wxString>(new wxString(doc->ComputeStringValueFromParameters(cell->m_value, cell->m_fillSetType)));
				m_table->SetValueAsCustom(row, col, s_strTypeTextOrString, ptr.get());
			}
		}

		m_rowAreaAt.Clear();

		for (int idx = 0; idx < spreadsheetDesc.GetAreaNumberRows(); idx++) {

			const CSpreadsheetAreaDescription* area = spreadsheetDesc.GetRowAreaByIdx(idx);

			if (area == nullptr)
				continue;

			//adding a new section
			wxGridExtCellArea entry;

			entry.m_start = area->m_start;
			entry.m_end = area->m_end;
			entry.m_areaLabel = area->m_label;

			m_rowAreaAt.push_back(entry);
		}

		m_colAreaAt.Clear();

		for (int idx = 0; idx < spreadsheetDesc.GetAreaNumberCols(); idx++) {

			const CSpreadsheetAreaDescription* area = spreadsheetDesc.GetColAreaByIdx(idx);

			if (area == nullptr)
				continue;

			//adding a new section
			wxGridExtCellArea entry;

			entry.m_start = area->m_start;
			entry.m_end = area->m_end;
			entry.m_areaLabel = area->m_label;

			m_colAreaAt.push_back(entry);
		}

		m_rowBrakeAt.Clear();

		for (int idx = 0; idx < spreadsheetDesc.GetBrakeNumberRows(); idx++)
			m_rowBrakeAt.push_back(spreadsheetDesc.GetRowBrakeByIdx(idx));

		m_colBrakeAt.Clear();

		for (int idx = 0; idx < spreadsheetDesc.GetBrakeNumberCols(); idx++)
			m_colBrakeAt.push_back(spreadsheetDesc.GetColBrakeByIdx(idx));

		for (int idx = 0; idx < spreadsheetDesc.GetSizeNumberRows(); idx++) {
			const CSpreadsheetRowSizeDescription* row_size = spreadsheetDesc.GetRowSizeByIdx(idx);
			if (row_size == nullptr)
				continue;

			wxGridExt::SetRowSize(row_size->m_row, row_size->m_height, 1.0f, false);
		}

		for (int idx = 0; idx < spreadsheetDesc.GetSizeNumberCols(); idx++) {
			const CSpreadsheetColSizeDescription* col_size = spreadsheetDesc.GetColSizeByIdx(idx);
			if (col_size == nullptr)
				continue;

			wxGridExt::SetColSize(col_size->m_col, col_size->m_width, 1.0f, false);
		}

		FreezeTo(spreadsheetDesc.GetRowFreeze(), spreadsheetDesc.GetColFreeze());

		wxGridExt::SetEvtHandlerEnabled(true);
	}

	return AssociateDocument(doc);
}

bool CGridEditor::SaveDocument(CSpreadsheetDescription& spreadsheetDesc) const
{
	return SaveSpreadsheet(spreadsheetDesc);
}

bool CGridEditor::SaveDocument(wxObjectDataPtr<CBackendSpreadsheetObject>& doc) const
{
	return SaveSpreadsheet(doc->GetSpreadsheetDesc());
}

bool CGridEditor::LoadSpreadsheet(const CSpreadsheetDescription& spreadsheetDesc)
{
	if (!spreadsheetDesc.IsEmptySpreadsheet())
	{
		wxGridExt::SetTable(
			new CGridEditorStringTable(spreadsheetDesc.GetNumberRows(), spreadsheetDesc.GetNumberCols()), true);

		wxGridExt::SetEvtHandlerEnabled(false);

		for (int row = 0; row < spreadsheetDesc.GetNumberRows(); row++) {

			for (int col = 0; col < spreadsheetDesc.GetNumberCols(); col++) {

				const CSpreadsheetCellDescription* cell = spreadsheetDesc.GetCell(row, col);
				if (cell == nullptr)
					continue;

				wxGridExtCellAttrPtr attr = GetOrCreateCellAttrPtr(row, col);
				attr->SetAlignment(cell->m_alignHorz, cell->m_alignVert);

				if (cell->m_row_size >= 0 && cell->m_col_size >= 0)
					SetCellSize(row, col, cell->m_row_size, cell->m_col_size, false);

				attr->SetTextOrient(cell->m_textOrient);
				attr->SetFont(cell->m_font);
				attr->SetBackgroundColour(cell->m_backgroundColour);
				attr->SetTextColour(cell->m_textColour);

				attr->SetBorderLeft(cell->m_borderAt[0].m_style, cell->m_borderAt[0].m_colour, cell->m_borderAt[0].m_width);
				attr->SetBorderRight(cell->m_borderAt[1].m_style, cell->m_borderAt[1].m_colour, cell->m_borderAt[1].m_width);
				attr->SetBorderTop(cell->m_borderAt[2].m_style, cell->m_borderAt[2].m_colour, cell->m_borderAt[2].m_width);
				attr->SetBorderBottom(cell->m_borderAt[3].m_style, cell->m_borderAt[3].m_colour, cell->m_borderAt[3].m_width);

				attr->SetFitMode(cell->m_fitMode == CSpreadsheetCellDescription::EFitMode::Mode_Overflow ? wxGridExtFitMode::Overflow() : wxGridExtFitMode::Clip());
				attr->SetReadOnly(cell->m_isReadOnly);

				if (cell->m_fillSetType == enSpreadsheetFillType::enSpreadsheetFillType_StrText) {
					wxSharedPtr<wxString> ptr = wxSharedPtr<wxString>(new wxString(cell->m_value));
					m_table->SetValueAsCustom(row, col, s_strTypeTextOrString, ptr.get());
				}
				else if (cell->m_fillSetType == enSpreadsheetFillType::enSpreadsheetFillType_StrTemplate) {
					wxSharedPtr<wxString> ptr = wxSharedPtr<wxString>(new wxString(cell->m_value));
					m_table->SetValueAsCustom(row, col, s_strTypeTemplate, ptr.get());
				}
				else if (cell->m_fillSetType == enSpreadsheetFillType::enSpreadsheetFillType_StrParameter) {
					wxSharedPtr<wxString> ptr = wxSharedPtr<wxString>(new wxString(cell->m_value));
					m_table->SetValueAsCustom(row, col, s_strTypeParameter, ptr.get());
				}
			}
		}

		m_rowAreaAt.Clear();

		for (int idx = 0; idx < spreadsheetDesc.GetAreaNumberRows(); idx++) {

			const CSpreadsheetAreaDescription* area = spreadsheetDesc.GetRowAreaByIdx(idx);

			if (area == nullptr)
				continue;

			//adding a new section
			wxGridExtCellArea entry;

			entry.m_start = area->m_start;
			entry.m_end = area->m_end;
			entry.m_areaLabel = area->m_label;

			m_rowAreaAt.push_back(entry);
		}

		m_colAreaAt.Clear();

		for (int idx = 0; idx < spreadsheetDesc.GetAreaNumberCols(); idx++) {

			const CSpreadsheetAreaDescription* area = spreadsheetDesc.GetColAreaByIdx(idx);

			if (area == nullptr)
				continue;

			//adding a new section
			wxGridExtCellArea entry;

			entry.m_start = area->m_start;
			entry.m_end = area->m_end;
			entry.m_areaLabel = area->m_label;

			m_colAreaAt.push_back(entry);
		}

		m_rowBrakeAt.Clear();

		for (int idx = 0; idx < spreadsheetDesc.GetBrakeNumberRows(); idx++)
			m_rowBrakeAt.push_back(spreadsheetDesc.GetRowBrakeByIdx(idx));

		m_colBrakeAt.Clear();

		for (int idx = 0; idx < spreadsheetDesc.GetBrakeNumberCols(); idx++)
			m_colBrakeAt.push_back(spreadsheetDesc.GetColBrakeByIdx(idx));

		for (int idx = 0; idx < spreadsheetDesc.GetSizeNumberRows(); idx++) {
			const CSpreadsheetRowSizeDescription* row_size = spreadsheetDesc.GetRowSizeByIdx(idx);
			if (row_size == nullptr)
				continue;

			wxGridExt::SetRowSize(row_size->m_row, row_size->m_height, 1.0f, false);
		}

		for (int idx = 0; idx < spreadsheetDesc.GetSizeNumberCols(); idx++) {
			const CSpreadsheetColSizeDescription* col_size = spreadsheetDesc.GetColSizeByIdx(idx);
			if (col_size == nullptr)
				continue;

			wxGridExt::SetColSize(col_size->m_col, col_size->m_width, 1.0f, false);
		}

		FreezeTo(spreadsheetDesc.GetRowFreeze(), spreadsheetDesc.GetColFreeze());

		wxGridExt::SetEvtHandlerEnabled(true);
	}

	return true;
}

bool CGridEditor::SaveSpreadsheet(CSpreadsheetDescription& spreadsheetDesc) const
{
	spreadsheetDesc = m_spreadsheetObject->GetSpreadsheetDesc();
	return true;
}

#pragma endregion