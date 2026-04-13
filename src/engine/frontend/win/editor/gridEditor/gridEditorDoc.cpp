#include "gridEditor.h"

bool ibGridEditor::AssociatibDocument(const wxObjectDataPtr<ibBackendSpreadsheetObject>& doc)
{
	if (m_spreadsheetObject != doc) {

		if (m_spreadsheetObject != nullptr)
			m_spreadsheetObject->RemoveNotifier(m_notifier);

		m_spreadsheetObject = doc;
		m_notifier = m_spreadsheetObject->AddNotifier<ibGenericSpreadsheetNotifier>(this);
	}

	return true;
}

bool ibGridEditor::GetActivibDocument(wxObjectDataPtr<ibBackendSpreadsheetObject>& doc) const
{
	doc = m_spreadsheetObject;
	return true;
}

#pragma region file

bool ibGridEditor::LoadDocument(const ibSpreadsheetDescription& spreadsheetDesc)
{
	if (!LoadSpreadsheet(spreadsheetDesc))
		return false;

	wxObjectDataPtr<ibBackendSpreadsheetObject> doc(
		new ibBackendSpreadsheetObject(spreadsheetDesc));

	return AssociatibDocument(doc);
}

bool ibGridEditor::LoadDocument(const wxObjectDataPtr<ibBackendSpreadsheetObject>& doc)
{
	const ibSpreadsheetDescription& spreadsheetDesc = doc->GetSpreadsheetDesc();

	if (!spreadsheetDesc.IsEmptySpreadsheet())
	{
		ibGrid::SetTable(
			new ibGridEditorStringTable(spreadsheetDesc.GetNumberRows(), spreadsheetDesc.GetNumberCols()), true);

		ibGrid::SetEvtHandlerEnabled(false);

		for (int row = 0; row < spreadsheetDesc.GetNumberRows(); row++) {

			for (int col = 0; col < spreadsheetDesc.GetNumberCols(); col++) {

				const ibSpreadsheetCellDescription* cell = spreadsheetDesc.GetCell(row, col);
				if (cell == nullptr)
					continue;

				ibGridCellAttrPtr attr = GetOrCreateCellAttrPtr(row, col);
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

				attr->SetFitMode(cell->m_fitMode == ibSpreadsheetCellDescription::ibFitMode::Mode_Overflow ? ibGridFitMode::Overflow() : ibGridFitMode::Clip());
				attr->SetReadOnly(cell->m_isReadOnly);

				wxSharedPtr<wxString> ptr = wxSharedPtr<wxString>(new wxString(doc->ComputeStringValueFromParameters(cell->m_value, cell->m_fillSetType)));
				m_table->SetValueAsCustom(row, col, s_strTypeTextOrString, ptr.get());
			}
		}

		m_rowAreaAt.Clear();

		for (int idx = 0; idx < spreadsheetDesc.GetAreaNumberRows(); idx++) {

			const ibSpreadsheetAreaDescription* area = spreadsheetDesc.GetRowAreaByIdx(idx);

			if (area == nullptr)
				continue;

			//adding a new section
			ibGridCellArea entry;

			entry.m_start = area->m_start;
			entry.m_end = area->m_end;
			entry.m_areaLabel = area->m_label;

			m_rowAreaAt.push_back(entry);
		}

		m_colAreaAt.Clear();

		for (int idx = 0; idx < spreadsheetDesc.GetAreaNumberCols(); idx++) {

			const ibSpreadsheetAreaDescription* area = spreadsheetDesc.GetColAreaByIdx(idx);

			if (area == nullptr)
				continue;

			//adding a new section
			ibGridCellArea entry;

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
			const ibSpreadsheetRowSizeDescription* row_size = spreadsheetDesc.GetRowSizeByIdx(idx);
			if (row_size == nullptr)
				continue;

			if ((int)row_size->m_row >= ibGrid::GetNumberRows())
				ibGrid::AppendCols((int)row_size->m_row - ibGrid::GetNumberRows() + 1);

			ibGrid::SetRowSize(row_size->m_row, row_size->m_height, 1.0f, false);
		}

		for (int idx = 0; idx < spreadsheetDesc.GetSizeNumberCols(); idx++) {
			const ibSpreadsheetColSizeDescription* col_size = spreadsheetDesc.GetColSizeByIdx(idx);
			if (col_size == nullptr)
				continue;

			if ((int)col_size->m_col >= ibGrid::GetNumberCols())
				ibGrid::AppendCols((int)col_size->m_col - ibGrid::GetNumberCols() + 1);

			ibGrid::SetColSize(col_size->m_col, col_size->m_width, 1.0f, false);
		}

		FreezeTo(spreadsheetDesc.GetRowFreeze(), spreadsheetDesc.GetColFreeze());
		EnableEditing(doc->IsEditable());

		ibGrid::SetEvtHandlerEnabled(true);
	}

	return AssociatibDocument(doc);
}

bool ibGridEditor::SaveDocument(ibSpreadsheetDescription& spreadsheetDesc) const
{
	return SaveSpreadsheet(spreadsheetDesc);
}

bool ibGridEditor::SaveDocument(wxObjectDataPtr<ibBackendSpreadsheetObject>& doc) const
{
	return SaveSpreadsheet(doc->GetSpreadsheetDesc());
}

void ibGridEditor::PutDocument(const wxObjectDataPtr<ibBackendSpreadsheetObject>& doc)
{
	ibGrid::SetEvtHandlerEnabled(false);

	if (m_table == nullptr) {
		ibGrid::SetTable(
			new ibGridEditorStringTable, true);
	}

	const int maxRowBrake = GetMaxRowBrake();
	const int maxColBrake = GetMaxColBrake();

	ibGrid::AppendRows(doc->GetNumberRows());
	if (doc->GetNumberCols() > m_table->GetNumberCols())
		ibGrid::AppendCols(doc->GetNumberCols() - m_table->GetNumberCols());

	m_numRows = m_table->GetNumberRows();
	m_numCols = m_table->GetNumberCols();

	for (int row = 0; row < doc->GetNumberRows(); row++) {

		for (int col = 0; col < doc->GetNumberCols(); col++) {

			const ibSpreadsheetCellDescription* cell = doc->GetSpreadsheetDesc().GetCell(row, col);
			if (cell == nullptr)
				continue;

			ibGridCellAttrPtr attr = GetOrCreateCellAttrPtr(maxRowBrake + row, col);
			attr->SetAlignment(cell->m_alignHorz, cell->m_alignVert);

			if (cell->m_row_size >= 0 && cell->m_col_size >= 0)
				SetCellSize(maxRowBrake + row, col, cell->m_row_size, cell->m_col_size, false);

			attr->SetTextOrient(cell->m_textOrient);
			attr->SetFont(cell->m_font);
			attr->SetBackgroundColour(cell->m_backgroundColour);
			attr->SetTextColour(cell->m_textColour);

			attr->SetBorderLeft(cell->m_borderAt[0].m_style, cell->m_borderAt[0].m_colour, cell->m_borderAt[0].m_width);
			attr->SetBorderRight(cell->m_borderAt[1].m_style, cell->m_borderAt[1].m_colour, cell->m_borderAt[1].m_width);
			attr->SetBorderTop(cell->m_borderAt[2].m_style, cell->m_borderAt[2].m_colour, cell->m_borderAt[2].m_width);
			attr->SetBorderBottom(cell->m_borderAt[3].m_style, cell->m_borderAt[3].m_colour, cell->m_borderAt[3].m_width);

			attr->SetFitMode(cell->m_fitMode == ibSpreadsheetCellDescription::ibFitMode::Mode_Overflow ? ibGridFitMode::Overflow() : ibGridFitMode::Clip());
			attr->SetReadOnly(cell->m_isReadOnly);

			wxSharedPtr<wxString> ptr = wxSharedPtr<wxString>(new wxString(doc->ComputeStringValueFromParameters(cell->m_value, cell->m_fillSetType)));
			m_table->SetValueAsCustom(maxRowBrake + row, col, s_strTypeTextOrString, ptr.get());
		}
	}

	for (int row = 0; row < doc->GetNumberRows(); row++)
		SetRowSize(maxRowBrake + row, doc->GetRowSize(row));

	for (int col = 0; col < doc->GetNumberCols(); col++)
		SetColSize(col, doc->GetColSize(col));

	SetRowBrake(maxRowBrake + doc->GetNumberRows());

	if (maxColBrake < doc->GetNumberCols())
		SetColBrake(maxColBrake + doc->GetNumberCols());

	ibGrid::EnableEditing(doc->IsEditable());
	ibGrid::SetEvtHandlerEnabled(true);
}

void ibGridEditor::JoinDocument(const wxObjectDataPtr<ibBackendSpreadsheetObject>& doc)
{
	ibGrid::SetEvtHandlerEnabled(false);

	if (m_table == nullptr) {
		ibGrid::SetTable(
			new ibGridEditorStringTable, true);
	}

	const int maxRowBrake = GetMaxRowBrake();
	const int maxColBrake = GetMaxColBrake();

	if (doc->GetNumberRows() > m_table->GetNumberRows())
		ibGrid::AppendRows(doc->GetNumberRows() - m_table->GetNumberRows());
	ibGrid::AppendCols(doc->GetNumberCols());

	m_numRows = m_table->GetNumberRows();
	m_numCols = m_table->GetNumberCols();

	for (int row = 0; row < doc->GetNumberRows(); row++) {

		for (int col = 0; col < doc->GetNumberCols(); col++) {

			const ibSpreadsheetCellDescription* cell = doc->GetSpreadsheetDesc().GetCell(row, col);
			if (cell == nullptr)
				continue;

			ibGridCellAttrPtr attr = GetOrCreateCellAttrPtr(row, maxColBrake + col);
			attr->SetAlignment(cell->m_alignHorz, cell->m_alignVert);

			if (cell->m_row_size >= 0 && cell->m_col_size >= 0)
				SetCellSize(row, maxColBrake + col, cell->m_row_size, cell->m_col_size, false);

			attr->SetTextOrient(cell->m_textOrient);
			attr->SetFont(cell->m_font);
			attr->SetBackgroundColour(cell->m_backgroundColour);
			attr->SetTextColour(cell->m_textColour);

			attr->SetBorderLeft(cell->m_borderAt[0].m_style, cell->m_borderAt[0].m_colour, cell->m_borderAt[0].m_width);
			attr->SetBorderRight(cell->m_borderAt[1].m_style, cell->m_borderAt[1].m_colour, cell->m_borderAt[1].m_width);
			attr->SetBorderTop(cell->m_borderAt[2].m_style, cell->m_borderAt[2].m_colour, cell->m_borderAt[2].m_width);
			attr->SetBorderBottom(cell->m_borderAt[3].m_style, cell->m_borderAt[3].m_colour, cell->m_borderAt[3].m_width);

			attr->SetFitMode(cell->m_fitMode == ibSpreadsheetCellDescription::ibFitMode::Mode_Overflow ? ibGridFitMode::Overflow() : ibGridFitMode::Clip());
			attr->SetReadOnly(cell->m_isReadOnly);

			wxSharedPtr<wxString> ptr = wxSharedPtr<wxString>(new wxString(doc->ComputeStringValueFromParameters(cell->m_value, cell->m_fillSetType)));
			m_table->SetValueAsCustom(row, maxColBrake + col, s_strTypeTextOrString, ptr.get());
		}
	}

	for (int row = 0; row < doc->GetNumberRows(); row++)
		SetRowSize(row, doc->GetRowSize(row));

	for (int col = 0; col < doc->GetNumberCols(); col++)
		SetColSize(maxColBrake + col, doc->GetColSize(col));

	if (maxRowBrake < doc->GetNumberRows())
		SetRowBrake(maxRowBrake + doc->GetNumberRows());

	SetColBrake(maxColBrake + doc->GetNumberCols());

	ibGrid::SetEvtHandlerEnabled(true);
}

bool ibGridEditor::LoadSpreadsheet(const ibSpreadsheetDescription& spreadsheetDesc)
{
	if (!spreadsheetDesc.IsEmptySpreadsheet())
	{
		ibGrid::SetTable(
			new ibGridEditorStringTable(spreadsheetDesc.GetNumberRows(), spreadsheetDesc.GetNumberCols()), true);

		ibGrid::SetEvtHandlerEnabled(false);

		for (int row = 0; row < spreadsheetDesc.GetNumberRows(); row++) {

			for (int col = 0; col < spreadsheetDesc.GetNumberCols(); col++) {

				const ibSpreadsheetCellDescription* cell = spreadsheetDesc.GetCell(row, col);
				if (cell == nullptr)
					continue;

				ibGridCellAttrPtr attr = GetOrCreateCellAttrPtr(row, col);
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

				attr->SetFitMode(cell->m_fitMode == ibSpreadsheetCellDescription::ibFitMode::Mode_Overflow ? ibGridFitMode::Overflow() : ibGridFitMode::Clip());
				attr->SetReadOnly(cell->m_isReadOnly);

				if (cell->m_fillSetType == ibSpreadsheetFillType::ibSpreadsheetFillType_StrText) {
					wxSharedPtr<wxString> ptr = wxSharedPtr<wxString>(new wxString(cell->m_value));
					m_table->SetValueAsCustom(row, col, s_strTypeTextOrString, ptr.get());
				}
				else if (cell->m_fillSetType == ibSpreadsheetFillType::ibSpreadsheetFillType_StrTemplate) {
					wxSharedPtr<wxString> ptr = wxSharedPtr<wxString>(new wxString(cell->m_value));
					m_table->SetValueAsCustom(row, col, s_strTypeTemplate, ptr.get());
				}
				else if (cell->m_fillSetType == ibSpreadsheetFillType::ibSpreadsheetFillType_StrParameter) {
					wxSharedPtr<wxString> ptr = wxSharedPtr<wxString>(new wxString(cell->m_value));
					m_table->SetValueAsCustom(row, col, s_strTypeParameter, ptr.get());
				}
			}
		}

		m_rowAreaAt.Clear();

		for (int idx = 0; idx < spreadsheetDesc.GetAreaNumberRows(); idx++) {

			const ibSpreadsheetAreaDescription* area = spreadsheetDesc.GetRowAreaByIdx(idx);

			if (area == nullptr)
				continue;

			//adding a new section
			ibGridCellArea entry;

			entry.m_start = area->m_start;
			entry.m_end = area->m_end;
			entry.m_areaLabel = area->m_label;

			m_rowAreaAt.push_back(entry);
		}

		m_colAreaAt.Clear();

		for (int idx = 0; idx < spreadsheetDesc.GetAreaNumberCols(); idx++) {

			const ibSpreadsheetAreaDescription* area = spreadsheetDesc.GetColAreaByIdx(idx);

			if (area == nullptr)
				continue;

			//adding a new section
			ibGridCellArea entry;

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
			const ibSpreadsheetRowSizeDescription* row_size = spreadsheetDesc.GetRowSizeByIdx(idx);
			if (row_size == nullptr)
				continue;

			if ((int)row_size->m_row >= ibGrid::GetNumberRows())
				ibGrid::AppendCols((int)row_size->m_row - ibGrid::GetNumberRows() + 1);

			ibGrid::SetRowSize((int)row_size->m_row, row_size->m_height, 1.0f, false);
		}

		for (int idx = 0; idx < spreadsheetDesc.GetSizeNumberCols(); idx++) {
			const ibSpreadsheetColSizeDescription* col_size = spreadsheetDesc.GetColSizeByIdx(idx);
			if (col_size == nullptr)
				continue;

			if ((int)col_size->m_col >= ibGrid::GetNumberCols())
				ibGrid::AppendCols((int)col_size->m_col - ibGrid::GetNumberCols() + 1);

			ibGrid::SetColSize(col_size->m_col, col_size->m_width, 1.0f, false);
		}

		FreezeTo(spreadsheetDesc.GetRowFreeze(), spreadsheetDesc.GetColFreeze());

		ibGrid::SetEvtHandlerEnabled(true);
	}

	return true;
}

bool ibGridEditor::SaveSpreadsheet(ibSpreadsheetDescription& spreadsheetDesc) const
{
	spreadsheetDesc = m_spreadsheetObject->GetSpreadsheetDesc();
	return true;
}

#pragma endregion