#include "gridEditor.h"

#pragma region file

bool CGridEditor::LoadDocument(const CBackendSpreadSheetDocument& doc)
{
	if (!doc.IsEmptySpreadsheet())
	{
		wxGridExt::SetTable(
			new CGridEditorStringTable(doc.GetNumberRows(), doc.GetNumberCols()), true);

		wxGridExt::SetEvtHandlerEnabled(false);

		for (int row = 0; row < doc.GetNumberRows(); row++)
		{
			for (int col = 0; col < doc.GetNumberCols(); col++)
			{
				const CSpreadsheetAttrChunk* cell = doc.GetCell(row, col);
				if (cell == nullptr)
					continue;

				wxGridExtCellAttrPtr attr = GetOrCreateCellAttrPtr(row, col);
				attr->SetAlignment(cell->m_alignHorz, cell->m_alignVert);

				SetCellValue(row, col, cell->m_value);

				if (cell->m_row_size >= 0 && cell->m_col_size >= 0)
					SetCellSize(row, col, cell->m_row_size, cell->m_col_size);

				attr->SetTextOrient(cell->m_textOrient);
				attr->SetFont(cell->m_font);
				attr->SetBackgroundColour(cell->m_backgroundColour);
				attr->SetTextColour(cell->m_textColour);

				attr->SetBorderLeft(cell->m_borderAt[0].m_style, cell->m_borderAt[0].m_colour, cell->m_borderAt[0].m_width);
				attr->SetBorderRight(cell->m_borderAt[1].m_style, cell->m_borderAt[1].m_colour, cell->m_borderAt[1].m_width);
				attr->SetBorderTop(cell->m_borderAt[2].m_style, cell->m_borderAt[2].m_colour, cell->m_borderAt[2].m_width);
				attr->SetBorderBottom(cell->m_borderAt[3].m_style, cell->m_borderAt[3].m_colour, cell->m_borderAt[3].m_width);
			}
		}

		m_rowAreaAt.Clear();

		for (int idx = 0; idx < doc.GetAreaNumberRows(); idx++)
		{
			const CSpreadsheetAreaChunk* area = doc.GetAreaRow(idx);

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

		for (int idx = 0; idx < doc.GetAreaNumberCols(); idx++)
		{
			const CSpreadsheetAreaChunk* area = doc.GetAreaCol(idx);

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

		for (int idx = 0; idx < doc.GetBrakeNumberRows(); idx++)
			m_rowBrakeAt.push_back(doc.GetBrakeRow(idx));

		m_colBrakeAt.Clear();

		for (int idx = 0; idx < doc.GetBrakeNumberCols(); idx++)
			m_colBrakeAt.push_back(doc.GetBrakeCol(idx));

		for (int idx = 0; idx < doc.GetSizeNumberRows(); idx++)
		{
			const CSpreadsheetRowSizeChunk* row_size = doc.GetSizeRow(idx);
			if (row_size == nullptr)
				continue;
			wxGridExt::SetRowSize(row_size->m_row, row_size->m_height);
		}

		for (int idx = 0; idx < doc.GetSizeNumberCols(); idx++)
		{
			const CSpreadsheetColSizeChunk* col_size = doc.GetSizeCol(idx);
			if (col_size == nullptr)
				continue;
			wxGridExt::SetColSize(col_size->m_col, col_size->m_width);
		}

		wxGridExt::SetEvtHandlerEnabled(true);

		FreezeTo(doc.GetFreezeRow(), doc.GetFreezeCol());
	}

	return true;
}

bool CGridEditor::SaveDocument(CBackendSpreadSheetDocument& doc) const
{
	doc.ResetSpreadsheet((GetMaxRowBrake() + 1) * (GetMaxColBrake() + 1));

	static wxString value;

	for (int row = 0; row < GetMaxRowBrake() + 1; row++)
	{
		for (int col = 0; col < GetMaxColBrake() + 1; col++)
		{
			wxGridExtCellAttrPtr attr = GetCellAttrPtr(row, col);

			int hAlign, vAlign;
			attr->GetAlignment(&hAlign, &vAlign);

			int num_rows, num_cols;
			attr->GetSize(&num_rows, &num_cols);

			CSpreadsheetBorderChunk borderAt[4] = {};

			const wxGridExtCellBorder& borderLeft = attr->GetBorderLeft();
			borderAt[0].m_colour = borderLeft.m_colour;
			borderAt[0].m_style = borderLeft.m_style;
			borderAt[0].m_width = borderLeft.m_width;

			const wxGridExtCellBorder& borderRight = attr->GetBorderRight();
			borderAt[1].m_colour = borderRight.m_colour;
			borderAt[1].m_style = borderRight.m_style;
			borderAt[1].m_width = borderRight.m_width;

			const wxGridExtCellBorder& borderTop = attr->GetBorderTop();
			borderAt[2].m_colour = borderTop.m_colour;
			borderAt[2].m_style = borderTop.m_style;
			borderAt[2].m_width = borderTop.m_width;

			const wxGridExtCellBorder& borderBottom = attr->GetBorderBottom();
			borderAt[3].m_colour = borderBottom.m_colour;
			borderAt[3].m_style = borderBottom.m_style;
			borderAt[3].m_width = borderBottom.m_width;

			void* tempval = m_table->GetValueAsCustom(row, col, wxT("translate"));
			if (tempval != nullptr) {
				const CBackendLocalizationEntryArray* array = static_cast<CBackendLocalizationEntryArray*>(tempval);
				if (array != nullptr) {
					CBackendLocalization::GetRawLocText(*array, value);
					wxDELETE(array);
				}
			}
			else {
				value.Clear();
			}

			doc.AppendCell(row, col, value,
				hAlign, vAlign, attr->GetTextOrient(),
				attr->GetFont(), attr->GetBackgroundColour(), attr->GetTextColour(),
				borderAt,
				num_rows, num_cols
			);
		}
	}

	for (unsigned int idx = 0; idx < m_rowAreaAt.Count(); idx++)
		doc.AppendAreaRow(m_rowAreaAt[idx].m_areaLabel, m_rowAreaAt[idx].m_start, m_rowAreaAt[idx].m_end);

	for (unsigned int idx = 0; idx < m_colAreaAt.Count(); idx++)
		doc.AppendAreaCol(m_colAreaAt[idx].m_areaLabel, m_colAreaAt[idx].m_start, m_colAreaAt[idx].m_end);

	for (unsigned int idx = 0; idx < m_rowBrakeAt.Count(); idx++)
		doc.AppendBrakeRow(m_rowBrakeAt[idx]);

	for (unsigned int idx = 0; idx < m_colBrakeAt.Count(); idx++)
		doc.AppendBrakeCol(m_colBrakeAt[idx]);

	for (int row = 0; row < GetMaxRowBrake() + 1; row++)
		doc.SetSizeRow(row, GetRowSize(row));

	for (int col = 0; col < GetMaxColBrake() + 1; col++)
		doc.SetSizeCol(col, GetColSize(col));

	doc.SetFreezeRow(m_numFrozenRows);
	doc.SetFreezeCol(m_numFrozenCols);
	return true;
}

#pragma endregion