#include "gridEditor.h"

class wxGridExtCommandClipboard : public wxGridExtCommandComposite
{
public:

	wxGridExtCommandClipboard(wxGridExt* view,
		const wxVector<CSpreadsheetCellDescription>& cells) : wxGridExtCommandComposite()
	{
		for (const auto& cell : cells) AppendCellCommand(view, cell.m_row, cell.m_col, cell);
	}

private:

	wxGridExtCellBorder GetCellGridBorder(const CSpreadsheetBorderDescription& rhs) const {
		wxGridExtCellBorder border;
		border.m_colour = rhs.m_colour;
		border.m_style = rhs.m_style;
		border.m_width = rhs.m_width;
		return border;
	}

	void AppendCellCommand(wxGridExt* view, int row, int col, const CSpreadsheetCellDescription& cell)
	{
		AppendCommand<wxGridExtCommandAttrBackgroundColour>(row, col, cell.m_backgroundColour,
			wxGridExtCommandAttrBackgroundColour::GetAttrValue(view->GetOrCreateCellAttrPtr(row, col)));

		AppendCommand<wxGridExtCommandAttrTextColour>(row, col, cell.m_textColour,
			wxGridExtCommandAttrTextColour::GetAttrValue(view->GetOrCreateCellAttrPtr(row, col)));

		AppendCommand<wxGridExtCommandAttrTextOrient>(row, col, cell.m_textOrient,
			wxGridExtCommandAttrTextOrient::GetAttrValue(view->GetOrCreateCellAttrPtr(row, col)));

		AppendCommand<wxGridExtCommandAttrFont>(row, col, cell.m_font,
			wxGridExtCommandAttrFont::GetAttrValue(view->GetOrCreateCellAttrPtr(row, col)));

		AppendCommand<wxGridExtCommandAttrAlignment>(row, col, wxSize(cell.m_alignHorz, cell.m_alignVert),
			wxGridExtCommandAttrAlignment::GetAttrValue(view->GetOrCreateCellAttrPtr(row, col)));

		AppendCommand<wxGridExtCommandAttrBorderLeft>(row, col, GetCellGridBorder(cell.m_borderAt[0]),
			wxGridExtCommandAttrBorderLeft::GetAttrValue(view->GetOrCreateCellAttrPtr(row, col)));

		AppendCommand<wxGridExtCommandAttrBorderRight>(row, col, GetCellGridBorder(cell.m_borderAt[1]),
			wxGridExtCommandAttrBorderRight::GetAttrValue(view->GetOrCreateCellAttrPtr(row, col)));

		AppendCommand<wxGridExtCommandAttrBorderTop>(row, col, GetCellGridBorder(cell.m_borderAt[2]),
			wxGridExtCommandAttrBorderTop::GetAttrValue(view->GetOrCreateCellAttrPtr(row, col)));

		AppendCommand<wxGridExtCommandAttrBorderBottom>(row, col, GetCellGridBorder(cell.m_borderAt[3]),
			wxGridExtCommandAttrBorderBottom::GetAttrValue(view->GetOrCreateCellAttrPtr(row, col)));

		AppendCommand<wxGridExtCommandAttrFitMode>(row, col, cell.m_fitMode == CSpreadsheetCellDescription::EFitMode::Mode_Overflow ? wxGridExtFitMode::Overflow() : wxGridExtFitMode::Clip(),
			wxGridExtCommandAttrFitMode::GetAttrValue(view->GetOrCreateCellAttrPtr(row, col)));

		AppendCommand<wxGridExtCommandAttrSize>(row, col, wxSize(cell.m_row_size, cell.m_col_size),
			wxGridExtCommandAttrSize::GetAttrValue(view->GetOrCreateCellAttrPtr(row, col)));

		AppendCommand<wxGridExtCommandAttrReadOnly>(row, col, cell.m_isReadOnly,
			wxGridExtCommandAttrReadOnly::GetAttrValue(view->GetOrCreateCellAttrPtr(row, col)));

		AppendCommand<wxGridExtCommandCellValue>(row, col, cell.m_value,
			view->GetCellValue(row, col));
	}

	wxVector<CSpreadsheetCellDescription> m_cells;
};

#include <wx/clipbrd.h>

void CGridEditor::Copy()
{
	if (wxTheClipboard->Open()) {

		bool hasBlocks = false;

		CMemoryWriter dataWritter;
		CMemoryWriter cellWriter;

		for (const auto& coords : wxGridExt::GetSelectedBlocks())
		{
			for (int row = coords.GetTopRow(); row <= coords.GetBottomRow(); row++)
			{
				for (int col = coords.GetLeftCol(); col <= coords.GetRightCol(); col++)
				{
					CMemoryWriter bodyWriter;

					const CSpreadsheetCellDescription* cell =
						m_spreadsheetObject->GetSpreadsheetDesc().GetCell(row, col);

					bodyWriter.w_s32(row - coords.GetTopRow());
					bodyWriter.w_s32(col - coords.GetLeftCol());

					bodyWriter.w_u8(cell != nullptr);

					if (cell != nullptr && !CSpreadsheetCellDescriptionMemory::SaveData(bodyWriter, *cell))
						continue;

					wxString s;

					s << row - coords.GetTopRow()
						<< col - coords.GetLeftCol();

					cellWriter.w_chunk(stringUtils::StrToUInt(s), bodyWriter.buffer());
				}
			}

			hasBlocks = true;
		}

		if (!hasBlocks)
		{
			int start_row = m_numRows,
				start_col = m_numCols;

			if (start_row > wxGridExt::GetGridCursorRow() && wxGridExt::GetGridCursorRow() >= 0)
				start_row = wxGridExt::GetGridCursorRow(); else start_row = 0;

			if (start_col > wxGridExt::GetGridCursorCol() && wxGridExt::GetGridCursorCol() >= 0)
				start_col = wxGridExt::GetGridCursorCol(); else start_col = 0;

			CMemoryWriter bodyWriter;

			const CSpreadsheetCellDescription* cell =
				m_spreadsheetObject->GetSpreadsheetDesc().GetCell(start_row, start_col);

			bodyWriter.w_s32(0);
			bodyWriter.w_s32(0);

			bodyWriter.w_u8(cell != nullptr);

			wxString s;
			s << start_row << start_col;

			if (cell != nullptr) CSpreadsheetCellDescriptionMemory::SaveData(bodyWriter, *cell);

			cellWriter.w_chunk(stringUtils::StrToUInt(s), bodyWriter.buffer());
		}

		dataWritter.w_chunk(1, cellWriter.buffer());

		wxDataObjectComposite* composite = new wxDataObjectComposite();
		wxCustomDataObject* custom_object = new wxCustomDataObject(oes_clipboard_template);

		custom_object->SetData(dataWritter.size(), dataWritter.pointer()); // the +1 is used to force copy of the \0 character		

		wxString copy_data;
		bool something_in_this_line;

		for (int i = 0; i < m_numRows; i++) {
			something_in_this_line = false;
			for (int j = 0; j < m_numCols; j++) {
				if (IsInSelection(i, j)) {
					if (something_in_this_line == false) {
						if (copy_data.IsEmpty() == false) {
							copy_data.Append(wxT("\n"));
						}
						something_in_this_line = true;
					}
					else {
						copy_data.Append(wxT("\t"));
					}
					copy_data = copy_data + GetCellValue(i, j);
				}
			}
		}

		composite->Add(custom_object);
		composite->Add(new wxTextDataObject(copy_data), true);

		wxTheClipboard->SetData(composite);
		wxTheClipboard->Close();
	}
}

void CGridEditor::Paste()
{
	if (wxTheClipboard->Open() &&
		wxTheClipboard->IsSupported(oes_clipboard_template))
	{
		wxCustomDataObject data(oes_clipboard_template);

		if (wxTheClipboard->GetData(data)) {

			int start_row = m_numRows,
				start_col = m_numCols;

			bool hasBlocks = false;

			for (const auto& coords : wxGridExt::GetSelectedBlocks()) {

				if (start_row > coords.GetTopRow())
					start_row = coords.GetTopRow();

				if (start_col > coords.GetLeftCol())
					start_col = coords.GetLeftCol();

				hasBlocks = true;
			}

			if (!hasBlocks) {

				if (start_row > wxGridExt::GetGridCursorRow() && wxGridExt::GetGridCursorRow() >= 0)
					start_row = wxGridExt::GetGridCursorRow(); else start_row = 0;

				if (start_col > wxGridExt::GetGridCursorCol() && wxGridExt::GetGridCursorCol() >= 0)
					start_col = wxGridExt::GetGridCursorCol(); else start_col = 0;
			}

			CMemoryReader* prevReaderCellMemory = nullptr;
			CMemoryReader reader(data.GetData(), data.GetDataSize());

			wxVector<CSpreadsheetCellDescription> cells;

			wxMemoryBuffer buf;
			if (reader.r_chunk(1, buf)) {

				int i = GetGridCursorRow(), j = GetGridCursorCol();

				CMemoryReader readerData(buf);

				while (!readerData.eof()) {

					u64 id = 0;

					CMemoryReader* readerCellMemory = readerData.open_chunk_iterator(id, prevReaderCellMemory);
					if (!readerCellMemory)
						break;

					const int row = start_row + readerCellMemory->r_s32();
					const int col = start_col + readerCellMemory->r_s32();

					if (readerCellMemory->r_u8()) {
						CSpreadsheetCellDescription cell(row, col);
						if (CSpreadsheetCellDescriptionMemory::LoadData(*readerCellMemory, cell))
							cells.push_back(cell);
					}

					prevReaderCellMemory = readerCellMemory;
				}

				PushCommand<wxGridExtCommandClipboard>(this, cells);

				for (const auto& cell : cells) {

					SetCellAlignment(cell.m_row, cell.m_col, cell.m_alignHorz, cell.m_alignVert, false);

					if (cell.m_row_size >= 0 && cell.m_col_size >= 0)
						SetCellSize(cell.m_row, cell.m_col, cell.m_row_size, cell.m_col_size, false);

					SetCellTextOrient(cell.m_row, cell.m_col, cell.m_textOrient, false);
					SetCellFont(cell.m_row, cell.m_col, cell.m_font, false);
					SetCellBackgroundColour(cell.m_row, cell.m_col, cell.m_backgroundColour, false);
					SetCellTextColour(cell.m_row, cell.m_col, cell.m_textColour, false);

					SetCellBorderLeft(cell.m_row, cell.m_col, cell.m_borderAt[0].m_style, cell.m_borderAt[0].m_colour, cell.m_borderAt[0].m_width, false);
					SetCellBorderRight(cell.m_row, cell.m_col, cell.m_borderAt[1].m_style, cell.m_borderAt[1].m_colour, cell.m_borderAt[1].m_width, false);
					SetCellBorderTop(cell.m_row, cell.m_col, cell.m_borderAt[2].m_style, cell.m_borderAt[2].m_colour, cell.m_borderAt[2].m_width, false);
					SetCellBorderBottom(cell.m_row, cell.m_col, cell.m_borderAt[3].m_style, cell.m_borderAt[3].m_colour, cell.m_borderAt[3].m_width, false);

					SetCellFitMode(cell.m_row, cell.m_col, cell.m_fitMode == CSpreadsheetCellDescription::EFitMode::Mode_Overflow ? wxGridExtFitMode::Overflow() : wxGridExtFitMode::Clip(), false);
					SetCellReadOnly(cell.m_row, cell.m_col, cell.m_isReadOnly, false);

					if (cell.m_fillSetType == enSpreadsheetFillType::enSpreadsheetFillType_StrText) {
						wxSharedPtr<wxString> ptr = wxSharedPtr<wxString>(new wxString(cell.m_value));
						m_table->SetValueAsCustom(cell.m_row, cell.m_col, s_strTypeTextOrString, ptr.get());
					}
					else if (cell.m_fillSetType == enSpreadsheetFillType::enSpreadsheetFillType_StrTemplate) {
						wxSharedPtr<wxString> ptr = wxSharedPtr<wxString>(new wxString(cell.m_value));
						m_table->SetValueAsCustom(cell.m_row, cell.m_col, s_strTypeTemplate, ptr.get());
					}
					else if (cell.m_fillSetType == enSpreadsheetFillType::enSpreadsheetFillType_StrParameter) {
						wxSharedPtr<wxString> ptr = wxSharedPtr<wxString>(new wxString(cell.m_value));
						m_table->SetValueAsCustom(cell.m_row, cell.m_col, s_strTypeParameter, ptr.get());
					}

					SetCellValue(cell.m_row, cell.m_col, cell.m_value, false);
				}
			}
		}

		wxTheClipboard->Close();

	}
	else if (wxTheClipboard->Open()
		&& wxTheClipboard->IsSupported(wxDF_TEXT)) {

		wxString copy_data, cur_field, cur_line;

		wxTextDataObject textObj;
		if (wxTheClipboard->GetData(textObj)) {
			copy_data = textObj.GetText();
		}
		wxTheClipboard->Close();

		int i = GetGridCursorRow(), j = GetGridCursorCol();
		int k = j;

		while (!copy_data.IsEmpty()) {
			cur_line = copy_data.BeforeFirst('\n');
			while (!cur_line.IsEmpty()) {
				cur_field = cur_line.BeforeFirst('\t');
				SetCellValue(i, j, cur_field);
				j++;
				cur_line = cur_line.AfterFirst('\t');
			}
			i++;
			j = k;
			copy_data = copy_data.AfterFirst('\n');
		}
	}

	wxGridExt::ForceRefresh();
}
