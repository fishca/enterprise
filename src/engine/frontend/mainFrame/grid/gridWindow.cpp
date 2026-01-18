////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : grid window
////////////////////////////////////////////////////////////////////////////

#include "gridWindow.h"
#include "frontend/win/ctrls/grid/gridextprivate.h"

enum
{
	wxID_ROW_HEIGHT = wxID_HIGHEST + 1,
	wxID_COL_WIDTH,

	wxID_HIDE_CELL,
	wxID_SHOW_CELL
};

wxBEGIN_EVENT_TABLE(CGridExtCtrl, wxGridExt)
EVT_GRID_CELL_RIGHT_CLICK(CGridExtCtrl::OnMouseRightDown)
EVT_GRID_LABEL_RIGHT_CLICK(CGridExtCtrl::OnMouseRightDown)
EVT_KEY_DOWN(CGridExtCtrl::OnKeyDown)
EVT_GRID_SELECT_CELL(CGridExtCtrl::OnSelectCell)
EVT_GRID_RANGE_SELECT(CGridExtCtrl::OnSelectCells)
EVT_GRID_COL_SIZE(CGridExtCtrl::OnGridColSize)
EVT_GRID_ROW_SIZE(CGridExtCtrl::OnGridRowSize)
EVT_GRID_EDITOR_HIDDEN(CGridExtCtrl::OnGridEditorHidden)
EVT_SCROLLWIN(CGridExtCtrl::OnScroll)
EVT_IDLE(CGridExtCtrl::OnIdle)
EVT_SIZE(CGridExtCtrl::OnSize)
EVT_MENU(wxID_COPY, CGridExtCtrl::OnCopy)
EVT_MENU(wxID_PASTE, CGridExtCtrl::OnPaste)
EVT_MENU(wxID_DELETE, CGridExtCtrl::OnDelete)
EVT_MENU(wxID_ROW_HEIGHT, CGridExtCtrl::OnRowHeight)
EVT_MENU(wxID_COL_WIDTH, CGridExtCtrl::OnColWidth)
EVT_MENU(wxID_HIDE_CELL, CGridExtCtrl::OnHideCell)
EVT_MENU(wxID_SHOW_CELL, CGridExtCtrl::OnShowCell)
EVT_MENU(wxID_PROPERTIES, CGridExtCtrl::OnProperties)
wxEND_EVENT_TABLE()

////////////////////////////////////////////////////////////////////

#include "frontend/mainFrame/objinspect/objinspect.h"

void CGridExtCtrl::ActivateEditor()
{
	if (m_enableProperty)
		objectInspector->SelectObject(m_cellProperty, true);
}

// ctor and Create() create the grid window, as with the other controls
CGridExtCtrl::CGridExtCtrl() : wxGridExt(),
m_cellProperty(nullptr), m_enableProperty(true)
{
}

CGridExtCtrl::CGridExtCtrl(wxWindow* parent,
	wxWindowID id, const wxPoint& pos,
	const wxSize& size) : wxGridExt(parent, id, pos, size),
	m_cellProperty(nullptr), m_enableProperty(true)
{
	m_rowLabelWidth = WXGRID_DEFAULT_ROW_LABEL_WIDTH - 42;
	m_colLabelHeight = WXGRID_MIN_COL_WIDTH;
	m_defaultColWidth = WXGRID_DEFAULT_ROW_LABEL_WIDTH - 12;
	m_defaultRowHeight = WXGRID_MIN_ROW_HEIGHT;

	// Grid
	wxGridExt::EnableEditing(true);
	wxGridExt::EnableGridLines(true);
	wxGridExt::EnableDragGridSize(false);
	wxGridExt::SetScrollRate(15, 15);
	wxGridExt::SetMargins(0, 0);

	// Create property
	m_cellProperty = new CGridExtCellProperty();
	m_cellProperty->SetView(this);

	// Native col 
	wxGridExt::SetUseNativeColLabels(false);
	wxGridExt::SetDoubleBuffered(true);

	// Cell Defaults
	wxGridExt::SetDefaultCellAlignment(wxAlignment::wxALIGN_LEFT, wxAlignment::wxALIGN_BOTTOM);
	wxGridExt::SetTable(new CGridExtStringTable, true);

	m_selectionBackground.Set(211, 217, 239);
	m_selectionForeground.Set(0, 0, 0);

	wxGridExt::SetDefaultCellFont(
		wxFont(8, wxFontFamily::wxFONTFAMILY_DEFAULT, wxFontStyle::wxFONTSTYLE_NORMAL, wxFontWeight::wxFONTWEIGHT_NORMAL));

	wxGridExt::SetDefaultEditor(new CGridExtCellTextEditor);

	wxAcceleratorEntry entries[4];
	entries[0].Set(wxACCEL_CTRL, (int)'A', wxID_SELECTALL);
	entries[1].Set(wxACCEL_CTRL, (int)'C', wxID_COPY);
	entries[2].Set(wxACCEL_CTRL, (int)'V', wxID_PASTE);

	entries[3].Set(wxACCEL_NORMAL, WXK_DELETE, wxID_DELETE);

	wxAcceleratorTable accel(4, entries);
	SetAcceleratorTable(accel);
}

CGridExtCtrl::~CGridExtCtrl()
{
	//delete property
	wxDELETE(m_cellProperty);
}

void CGridExtCtrl::AddArea()
{
	if (!wxGridExt::IsEditable())
		return;

	wxGridExt::AddArea();
}

void CGridExtCtrl::RemoveArea()
{
	if (!wxGridExt::IsEditable())
		return;

	wxGridExt::RemoveArea();
}

void CGridExtCtrl::MergeCells()
{
	if (!wxGridExt::IsEditable())
		return;

	const wxGridExtBlockCoords& cellRange = GetSelectedCellRange();
	wxGridExt::SetCellSize(
		cellRange.GetTopRow(), cellRange.GetLeftCol(),
		cellRange.GetBottomRow() - cellRange.GetTopRow() + 1, cellRange.GetRightCol() - cellRange.GetLeftCol() + 1
	);

	//wxGridExt::SendEvent(wxEVT_GRID_EDITOR_HIDDEN);
	//wxGridExt::ForceRefresh();
}

void CGridExtCtrl::DockTable()
{
	if (!wxGridExt::IsEditable())
		return;

	const wxGridExtBlockCoords& cellRange = GetSelectedCellRange();
	CGridExtCtrl::FreezeTo(cellRange.GetBottomRow(), cellRange.GetRightCol());
}

#include <wx/clipbrd.h>

void CGridExtCtrl::Copy()
{
	wxString copy_data;
	bool something_in_this_line;

	copy_data.Clear();

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

	if (wxTheClipboard->Open()) {
		wxTheClipboard->SetData(new wxTextDataObject(copy_data));
		wxTheClipboard->Close();
	}
}

void CGridExtCtrl::Paste()
{
	wxString copy_data, cur_field, cur_line;

	if (wxTheClipboard->Open()
		&& wxTheClipboard->IsSupported(wxDF_TEXT)) {
		wxTextDataObject textObj;
		if (wxTheClipboard->GetData(textObj)) {
			copy_data = textObj.GetText();
		}
		wxTheClipboard->Close();
	}

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

	wxGridExt::ForceRefresh();
}

#pragma region events

#include "frontend/artProvider/artProvider.h"

void CGridExtCtrl::OnMouseRightDown(wxGridExtEvent& event)
{
	if (event.GetRow() == wxNOT_FOUND &&
		event.GetCol() == wxNOT_FOUND) {
	}
	else if (event.GetRow() == wxNOT_FOUND) {
		wxGridExt::SetCurrentCell(0, event.GetCol()); bool foundedCol = false;
		for (auto block : wxGridExt::GetSelectedBlocks()) {
			if (block.GetLeftCol() <= event.GetCol() && block.GetTopRow() == 0
				&& block.GetRightCol() >= event.GetCol() && block.GetBottomRow() == m_numRows - 1) {
				foundedCol = true; break;
			}
		}
		if (!foundedCol) {
			wxGridExt::SelectBlock(0, event.GetCol(), m_numRows - 1, event.GetCol());
		}
	}
	else if (event.GetCol() == wxNOT_FOUND) {
		wxGridExt::SetCurrentCell(event.GetRow(), 0); bool foundedRow = false;
		for (auto block : wxGridExt::GetSelectedBlocks()) {
			if (block.GetLeftCol() == 0 && block.GetTopRow() <= event.GetRow() &&
				block.GetRightCol() == m_numCols - 1 && block.GetBottomRow() >= event.GetRow()) {
				foundedRow = true; break;
			}
		}
		if (!foundedRow) {
			wxGridExt::SelectBlock(event.GetRow(), 0, event.GetRow(), m_numCols - 1);
		}
	}
	else {
		wxGridExt::SetCurrentCell(event.GetRow(), event.GetCol());
	}

	if (m_enableProperty) {

		wxMenuItem* item = nullptr; wxMenu menuPopup;
		item = menuPopup.Append(wxID_COPY, _("Copy"));
		item->SetBitmap(wxArtProvider::GetBitmap(wxART_COPY, wxART_MENU));
		item = menuPopup.Append(wxID_PASTE, _("Paste"));
		item->SetBitmap(wxArtProvider::GetBitmap(wxART_PASTE, wxART_MENU));

		if (!wxGridExt::CanEnableCellControl())
			menuPopup.Enable(wxID_PASTE, false);

		item = menuPopup.Append(wxID_DELETE, _("Delete"));
		item->SetBitmap(wxArtProvider::GetBitmap(wxART_DELETE, wxART_MENU));

		if (event.GetRow() == wxNOT_FOUND &&
			event.GetCol() != wxNOT_FOUND) {
			menuPopup.AppendSeparator();
			item = menuPopup.Append(wxID_HIDE_CELL, _("Hide"));
			item->SetBitmap(wxArtProvider::GetBitmap(wxART_MINUS, wxART_MENU));
			item = menuPopup.Append(wxID_SHOW_CELL, _("Display"));
			item->SetBitmap(wxArtProvider::GetBitmap(wxART_PLUS, wxART_MENU));
			item = menuPopup.Append(wxID_COL_WIDTH, _("Column width..."));
			item->SetBitmap(wxArtProvider::GetBitmap(wxART_FULL_SCREEN, wxART_MENU));
		}
		else if (event.GetRow() != wxNOT_FOUND &&
			event.GetCol() == wxNOT_FOUND) {
			menuPopup.AppendSeparator();
			item = menuPopup.Append(wxID_HIDE_CELL, _("Hide"));
			item->SetBitmap(wxArtProvider::GetBitmap(wxART_MINUS, wxART_MENU));
			item = menuPopup.Append(wxID_SHOW_CELL, _("Display"));
			item->SetBitmap(wxArtProvider::GetBitmap(wxART_PLUS, wxART_MENU));
			item = menuPopup.Append(wxID_ROW_HEIGHT, _("Row height..."));
			item->SetBitmap(wxArtProvider::GetBitmap(wxART_FULL_SCREEN, wxART_MENU));
		}

		menuPopup.AppendSeparator();
		item = menuPopup.Append(wxID_PROPERTIES, _("Properties"));
		item->SetBitmap(wxArtProvider::GetBitmap(wxART_PROPERTY, wxART_SERVICE));

		if (wxGridExt::PopupMenu(&menuPopup, event.GetPosition())) {
			event.Skip();
		}
	}
}

// This seems to be required for wxMotif/wxGTK otherwise the mouse
// cursor must be in the cell edit control to get key events
//
void CGridExtCtrl::OnKeyDown(wxKeyEvent& event)
{
	const int code = event.GetKeyCode();

	if (code == WXK_DOWN || code == WXK_UP || code == WXK_LEFT || code == WXK_RIGHT ||
		code == WXK_NUMPAD_DOWN || code == WXK_NUMPAD_UP || code == WXK_NUMPAD_LEFT || code == WXK_NUMPAD_RIGHT) {

		int ux, uy,
			sx, sy;

		wxGridExt::GetScrollPixelsPerUnit(&ux, &uy);
		wxGridExt::GetViewStart(&sx, &sy);

		sx *= ux; sy *= uy;

		int w, h;
		m_gridWin->GetClientSize(&w, &h);

		const int x0 = wxGridExt::XToCol(sx, true);
		const int y0 = wxGridExt::YToRow(sy, true);

		const int x1 = wxGridExt::XToCol(sx + w, true);
		const int y1 = wxGridExt::YToRow(sy + h, true);

		const short scroll = (code == WXK_UP || code == WXK_LEFT) ? -1 : 1;

		if (scroll > 0 && code == WXK_DOWN) {
			if (y1 == wxGridExt::GetNumberRows() - 1) {
				wxGridExt::AppendRows();
				wxGridExt::SetScrollPos(wxOrientation::wxVERTICAL, (sy + uy) / uy);
			}
		}
		else if (scroll > 0 && code == WXK_RIGHT) {
			if (x1 == wxGridExt::GetNumberCols() - 1) {
				wxGridExt::AppendCols();
				wxGridExt::SetScrollPos(wxOrientation::wxHORIZONTAL, (sx + ux) / ux);
			}
		}
		else if (scroll < 0 && code == WXK_UP) {
			if (y0 > GetMaxRowBrake() && y0 != 0) wxGridExt::DeleteRows(wxGridExt::GetNumberRows() - 1);
		}
		else if (scroll < 0 && code == WXK_LEFT) {
			if (x0 > GetMaxColBrake() && x0 != 0) wxGridExt::DeleteCols(wxGridExt::GetNumberCols() - 1);
		}
	}

	event.Skip();
}

void CGridExtCtrl::OnSelectCell(wxGridExtEvent& event)
{
	if (m_enableProperty) {
		if (event.Selecting()) {
			m_cellProperty->AddSelectedCell(
				{
				 event.GetRow(), event.GetCol(),
				 event.GetRow(), event.GetCol()
				}
			);
		}
		else {
			m_cellProperty->ClearSelectedCell();
		}
	}

	event.Skip();
}

void CGridExtCtrl::OnSelectCells(wxGridExtRangeSelectEvent& event)
{
	if (m_enableProperty) {
		if (event.Selecting()) {
			m_cellProperty->AddSelectedCell({
				event.GetTopRow(), event.GetLeftCol(),
				event.GetBottomRow(), event.GetRightCol()
				}
			);
		}
		else {
			m_cellProperty->ClearSelectedCell();
		}

	}

	event.Skip();
}

void CGridExtCtrl::OnGridColSize(wxGridExtSizeEvent& event)
{
	//CalcSection();
	wxGridExt::SendEvent(wxEVT_GRID_EDITOR_HIDDEN, wxGridExtCellCoords{ m_currentCellCoords.GetCol(), event.GetRowOrCol() });
	wxGridExt::ForceRefresh();
	event.Skip();
}

void CGridExtCtrl::OnGridRowSize(wxGridExtSizeEvent& event)
{
	//CalcSection();
	wxGridExt::SendEvent(wxEVT_GRID_EDITOR_HIDDEN, wxGridExtCellCoords{ event.GetRowOrCol(), m_currentCellCoords.GetCol() });
	wxGridExt::ForceRefresh();
	event.Skip();
}

void CGridExtCtrl::OnGridEditorHidden(wxGridExtEvent& event)
{
	int col = event.GetCol(),
		row = event.GetRow();

	int cell_rows = 0, cell_cols = 0;
	if (wxGridExt::GetCellSize(row, col, &cell_rows, &cell_cols) == wxGridExt::CellSpan_Main) {

		row += cell_rows - 1; col += cell_cols - 1;

	}
	else if (wxGridExt::GetCellSize(row, col, &cell_rows, &cell_cols) == wxGridExt::CellSpan_Inside) {

		row += cell_rows; col += cell_cols;

		if (GetCellSize(row, col, &cell_rows, &cell_cols) == wxGridExt::CellSpan_Main) {
			row += cell_rows; col += cell_cols;
		}
	}

	event.Skip();
}

void CGridExtCtrl::OnScroll(wxScrollWinEvent& event)
{
	int ux, uy,
		sx, sy;

	wxGridExt::GetScrollPixelsPerUnit(&ux, &uy);
	wxGridExt::GetViewStart(&sx, &sy);

	sx *= ux; sy *= uy;

	int w, h;
	m_gridWin->GetClientSize(&w, &h);

	int scroll = 0, page_scroll_x = 0, page_scroll_y = 0;

	if (event.GetEventType() == wxEVT_SCROLLWIN_THUMBTRACK ||
		event.GetEventType() == wxEVT_SCROLLWIN_THUMBRELEASE) {
		const int position = wxGridExt::GetScrollPos(event.GetOrientation());
		if (event.GetOrientation() == wxOrientation::wxVERTICAL) {
			if (position < event.GetPosition())
				scroll = 0;
			else if (position > event.GetPosition())
				scroll = -(position - event.GetPosition());
		}
		else if (event.GetOrientation() == wxOrientation::wxHORIZONTAL) {
			if (position < event.GetPosition())
				scroll = 0;
			else if (position > event.GetPosition())
				scroll = -(position - event.GetPosition());
		}
	}
	else if (event.GetEventType() == wxEVT_SCROLLWIN_TOP ||
		event.GetEventType() == wxEVT_SCROLLWIN_LINEUP)
	{
		if (event.GetOrientation() == wxOrientation::wxVERTICAL)
			scroll = -uy;
		else if (event.GetOrientation() == wxOrientation::wxHORIZONTAL)
			scroll = -ux;
	}
	else if (event.GetEventType() == wxEVT_SCROLLWIN_PAGEUP)
	{
		if (event.GetOrientation() == wxOrientation::wxVERTICAL) {
			page_scroll_y = -wxGridExt::GetScrollPageSize(event.GetOrientation()) * uy;
			scroll = page_scroll_y;
		}
		else if (event.GetOrientation() == wxOrientation::wxHORIZONTAL) {
			page_scroll_x = -wxGridExt::GetScrollPageSize(event.GetOrientation()) * ux;
			scroll = page_scroll_x;
		}

		scroll = page_scroll_y;
	}
	else if (event.GetEventType() == wxEVT_SCROLLWIN_BOTTOM ||
		event.GetEventType() == wxEVT_SCROLLWIN_LINEDOWN)
	{
		if (event.GetOrientation() == wxOrientation::wxVERTICAL)
			scroll = uy;
		else if (event.GetOrientation() == wxOrientation::wxHORIZONTAL)
			scroll = ux;
	}
	else if (event.GetEventType() == wxEVT_SCROLLWIN_PAGEDOWN)
	{
		if (event.GetOrientation() == wxOrientation::wxVERTICAL) {
			page_scroll_y = wxGridExt::GetScrollPageSize(event.GetOrientation()) * uy;
			scroll = page_scroll_y;
		}
		else if (event.GetOrientation() == wxOrientation::wxHORIZONTAL) {
			page_scroll_x = wxGridExt::GetScrollPageSize(event.GetOrientation()) * ux;
			scroll = page_scroll_x;
		}
	}

	const int x0 = wxGridExt::XToCol(sx, true);
	const int y0 = wxGridExt::YToRow(sy, true);

	const int x1 = wxGridExt::XToCol(sx + w, true);
	const int y1 = wxGridExt::YToRow(sy + h, true);

	if (scroll > 0 && event.GetOrientation() == wxOrientation::wxVERTICAL) {
		if (y1 == wxGridExt::GetNumberRows() - 1) {
			wxGridExt::AppendRows();
			wxGridExt::SetScrollPos(wxOrientation::wxVERTICAL, (sy + scroll) / uy);
		}
	}
	else if (scroll > 0 && event.GetOrientation() == wxOrientation::wxHORIZONTAL) {
		if (x1 == wxGridExt::GetNumberCols() - 1) {
			wxGridExt::AppendCols();
			wxGridExt::SetScrollPos(wxOrientation::wxHORIZONTAL, (sx + scroll) / ux);
		}
	}
	else if (scroll < 0 && event.GetOrientation() == wxOrientation::wxVERTICAL) {
		const int y = wxGridExt::YToRow(sy + h + scroll, true) + 1;
		if (y0 > GetMaxRowBrake() && y0 != 0) {
			for (int row = wxGridExt::GetNumberRows(); row > y; row--)
				wxGridExt::DeleteRows(row - 1);
			wxGridExt::SetScrollPos(wxOrientation::wxVERTICAL, (sy + scroll) / uy);
		}
	}
	else if (scroll < 0 && event.GetOrientation() == wxOrientation::wxHORIZONTAL) {
		const int x = wxGridExt::XToCol(sx + w + scroll, true);
		if (x0 > GetMaxColBrake() && x0 != 0) {
			for (int col = wxGridExt::GetNumberCols(); col > x; col--)
				wxGridExt::DeleteCols(col - 1);
			wxGridExt::SetScrollPos(wxOrientation::wxHORIZONTAL, (sx + scroll) / ux);
		}
	}

	event.Skip();
}

void CGridExtCtrl::OnIdle(wxIdleEvent& event)
{
	event.Skip();
}

void CGridExtCtrl::OnSize(wxSizeEvent& event)
{
	int ux, uy,
		sx, sy;

	wxGridExt::GetScrollPixelsPerUnit(&ux, &uy);
	wxGridExt::GetViewStart(&sx, &sy);

	sx *= ux; sy *= uy;

	const int w = event.m_size.x;
	const int h = event.m_size.y;

	int x0 = wxGridExt::XToCol(sx);
	int y0 = wxGridExt::YToRow(sy);
	int x1 = wxGridExt::XToCol(sx + w);
	int y1 = wxGridExt::YToRow(sy + h);

	if (m_table != nullptr && x0 == wxNOT_FOUND) {
		const int col = w / m_defaultColWidth;
		if ((col + 1) > m_table->GetColsCount())
			m_table->AppendCols(col + 1);
	}
	else if (m_table != nullptr && x1 == wxNOT_FOUND) {
		int col_size = 0;
		for (int col = x0; col < m_table->GetColsCount(); col++)
			col_size += wxGridExt::GetColSize(col);
		if (w > col_size) {
			const int col = ((w - col_size) / m_defaultColWidth);
			m_table->AppendCols(col + 1);
		}
	}

	if (m_table != nullptr && y0 == wxNOT_FOUND) {
		const int row = h / m_defaultRowHeight;
		if ((row + 1) > m_table->GetRowsCount())
			m_table->AppendRows(row + 1);
	}
	else if (m_table != nullptr && y1 == wxNOT_FOUND) {
		int row_size = 0;
		for (int row = y0; row < m_table->GetRowsCount(); row++)
			row_size += wxGridExt::GetRowSize(row);
		if (h > row_size) {
			const int row = ((h - row_size) / m_defaultRowHeight);
			m_table->AppendRows(row + 1);
		}
	}

	event.Skip();
}
#pragma endregion

#pragma region context_menu
void CGridExtCtrl::OnCopy(wxCommandEvent& event)
{
	Copy();
}

void CGridExtCtrl::OnPaste(wxCommandEvent& event)
{
	Paste();
}

void CGridExtCtrl::OnDelete(wxCommandEvent& event)
{
	//for (auto cell : wxGridExt::GetSelectedBlocks()) {
	//	for (int col = cell.GetLeftCol(); col <= cell.GetRightCol(); col++) {
	//		for (int row = cell.GetTopRow(); row <= cell.GetBottomRow(); row++) {
	//			wxGridExt::SetCellBackgroundColour(row, col, m_defaultCellAttr->GetBackgroundColour());
	//			wxGridExt::SetCellFont(row, col, m_defaultCellAttr->GetFont());
	//			wxGridExt::SetCellTextColour(row, col, m_defaultCellAttr->GetTextColour());
	//			wxGridExt::SetCellAlignment(row, col, wxAlignment::wxALIGN_LEFT, wxAlignment::wxALIGN_BOTTOM);
	//			int num_rows, num_cols;
	//			if (wxGridExt::GetCellSize(row, col, &num_rows, &num_cols) == CellSpan::CellSpan_Main) {
	//				wxGridExt::SetCellSize(row, col, 1, 1);
	//			}
	//			else if (wxGridExt::GetCellSize(row, col, &num_rows, &num_cols) == CellSpan::CellSpan_Inside) {
	//				int insideRow = row + num_rows,
	//					insideCol = col + num_cols;
	//				wxGridExt::GetCellSize(insideRow, insideCol, &num_rows, &num_cols);
	//				wxGridExt::SetCellSize(insideRow, insideCol,
	//					1,
	//					1
	//				);
	//			}
	//			else {
	//				wxGridExt::SetCellSize(row, col, 1, 1);
	//			}

	//			CGridExtCtrl::SetCellTextOrientation(row, col, wxOrientation::wxHORIZONTAL);
	//			CGridExtCtrl::SetCellBorder(row, col,
	//				wxPenStyle::wxPENSTYLE_TRANSPARENT,
	//				wxPenStyle::wxPENSTYLE_TRANSPARENT,
	//				wxPenStyle::wxPENSTYLE_TRANSPARENT,
	//				wxPenStyle::wxPENSTYLE_TRANSPARENT,
	//				*wxBLACK
	//			);

	//			wxGridExt::SetCellValue(row, col, wxEmptyString);
	//		}
	//	}

	//	int rangeFrom = 0, rangeTo = 0; CGridExtArea* gridSection = nullptr;
	//	if (GetWorkSection(rangeFrom, rangeTo, gridSection)) {
	//		gridSection->Remove(rangeFrom, rangeTo);
	//	}

	//	if (cell.GetLeftCol() <= m_colBrake &&
	//		cell.GetRightCol() >= m_colBrake &&
	//		cell.GetLeftCol() > 0) {
	//		for (int col = cell.GetRightCol(); col >= cell.GetLeftCol(); col--) {
	//			wxGridExt::SetColSize(col, WXGRID_DEFAULT_ROW_LABEL_WIDTH - 12);
	//		}
	//		m_rowBrake = cell.GetLeftCol() - 1;
	//	}

	//	if (cell.GetTopRow() <= m_rowBrake &&
	//		cell.GetBottomRow() >= m_rowBrake &&
	//		cell.GetTopRow() > 0) {
	//		for (int row = cell.GetBottomRow(); row >= cell.GetTopRow(); row--) {
	//			wxGridExt::SetRowSize(row, WXGRID_MIN_ROW_HEIGHT);
	//		}
	//		m_colBrake = cell.GetTopRow() - 1;
	//	}

	//	if (!cell.GetLeftCol() &&
	//		!cell.GetTopRow()) {
	//		for (int col = cell.GetRightCol(); col >= 0; col--) {
	//			wxGridExt::SetColSize(col, WXGRID_DEFAULT_ROW_LABEL_WIDTH - 12);
	//		}
	//		for (int row = cell.GetBottomRow(); row >= 0; row--) {
	//			wxGridExt::SetRowSize(row, WXGRID_MIN_ROW_HEIGHT);
	//		}
	//		m_colBrake = 0; m_rowBrake = 0;
	//	}
	//}
}

#include "frontend/win/dlgs/rowHeight.h"

void CGridExtCtrl::OnRowHeight(wxCommandEvent& event)
{
	std::shared_ptr <CDialogRowHeight> rowHeight(new CDialogRowHeight(this));
	const int result = rowHeight->ShowModal();
	if (result == wxID_OK) {
		for (auto cell : wxGridExt::GetSelectedBlocks()) {
			for (int row = cell.GetTopRow(); row <= cell.GetBottomRow(); row++) {
				wxGridExt::SetRowSize(row, rowHeight->GetHeight());
			}
		}
	}
}

#include "frontend/win/dlgs/colHeight.h"

void CGridExtCtrl::OnColWidth(wxCommandEvent& event)
{
	std::shared_ptr<CDialogColWidth> colWidth(new CDialogColWidth(this));
	const int result = colWidth->ShowModal();
	if (result == wxID_OK) {
		for (auto cell : wxGridExt::GetSelectedBlocks()) {
			for (int col = cell.GetLeftCol(); col <= cell.GetRightCol(); col++) {
				wxGridExt::SetColSize(col, colWidth->GetWidth());
			}
		}
	}
}

void CGridExtCtrl::OnHideCell(wxCommandEvent& event)
{
	for (auto cell : wxGridExt::GetSelectedBlocks()) {
		if (cell.GetLeftCol() > 0 &&
			cell.GetTopRow() == 0) {
			for (int col = cell.GetLeftCol(); col <= cell.GetRightCol(); col++) {
				wxGridExt::SetColSize(col, 0);
			}
		}
		else if (cell.GetTopRow() > 0 &&
			cell.GetLeftCol() == 0) {
			for (int row = cell.GetTopRow(); row <= cell.GetBottomRow(); row++) {
				wxGridExt::SetRowSize(row, 0);
			}
		}
		else if (cell.GetLeftCol() == 0 &&
			cell.GetTopRow() == 0) {
			if (cell.GetBottomRow() == m_numRows - 1) {
				for (int col = cell.GetLeftCol(); col <= cell.GetRightCol(); col++) {
					wxGridExt::SetColSize(col, 0);
				}
			}
			else if (cell.GetRightCol() == m_numCols - 1) {
				for (int row = cell.GetTopRow(); row <= cell.GetBottomRow(); row++) {
					wxGridExt::SetRowSize(row, 0);
				}
			}
		}
	}
}

void CGridExtCtrl::OnShowCell(wxCommandEvent& event)
{
	for (auto cell : wxGridExt::GetSelectedBlocks()) {
		if (cell.GetLeftCol() > 0 &&
			cell.GetTopRow() == 0) {
			bool hiddenCol = false;
			for (int col = 0; col < cell.GetLeftCol(); col++) {
				int size = wxGridExt::GetColSize(col);
				hiddenCol = size == 0;
			}
			for (int col = hiddenCol ? 0 : cell.GetLeftCol(); col <= cell.GetRightCol(); col++) {
				wxGridExt::SetColSize(col, wxNOT_FOUND);
			}
		}
		else if (cell.GetTopRow() > 0 &&
			cell.GetLeftCol() == 0) {
			bool hiddenRow = false;
			for (int row = 0; row < cell.GetTopRow(); row++) {
				int size = wxGridExt::GetRowSize(row);
				hiddenRow = size == 0;
			}
			for (int row = hiddenRow ? 0 : cell.GetTopRow(); row <= cell.GetBottomRow(); row++) {
				wxGridExt::SetRowSize(row, wxNOT_FOUND);
			}
		}
		else if (cell.GetLeftCol() == 0 &&
			cell.GetTopRow() == 0) {
			if (cell.GetBottomRow() == m_numRows - 1) {
				for (int col = cell.GetLeftCol(); col <= cell.GetRightCol(); col++) {
					wxGridExt::SetColSize(col, wxNOT_FOUND);
				}
			}
			else if (cell.GetRightCol() == m_numCols - 1) {
				for (int row = cell.GetTopRow(); row <= cell.GetBottomRow(); row++) {
					wxGridExt::SetRowSize(row, wxNOT_FOUND);
				}
			}
		}
	}
}

void CGridExtCtrl::OnProperties(wxCommandEvent& event)
{
	m_cellProperty->ShowProperty();
}
#pragma endregion  