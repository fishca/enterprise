////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : grid window
////////////////////////////////////////////////////////////////////////////

#include "gridEditor.h"

#include "frontend/docView/docView.h"

enum
{
	wxID_ROW_HEIGHT = wxID_HIGHEST + 1,
	wxID_COL_WIDTH,

	wxID_HIDE_CELL,
	wxID_SHOW_CELL
};

wxBEGIN_EVENT_TABLE(CGridEditor, wxGridExt)
EVT_GRID_CELL_RIGHT_CLICK(CGridEditor::OnMouseRightDown)
EVT_GRID_LABEL_RIGHT_CLICK(CGridEditor::OnMouseRightDown)
EVT_KEY_DOWN(CGridEditor::OnKeyDown)
EVT_GRID_SELECT_CELL(CGridEditor::OnSelectCell)
EVT_GRID_RANGE_SELECT(CGridEditor::OnSelectCells)
EVT_GRID_ROW_MODIFIED(CGridEditor::OnGridRowSize)
EVT_GRID_COL_MODIFIED(CGridEditor::OnGridColSize)
EVT_GRID_TABLE_MODIFIED(CGridEditor::OnGridTableModified)
EVT_GRID_TABLE_ATTR_MODIFIED(CGridEditor::OnGridTableAttrModified)
EVT_GRID_ROW_BRAKE_ADD(CGridEditor::OnGridRowBrake)
EVT_GRID_ROW_BRAKE_SET(CGridEditor::OnGridRowBrake)
EVT_GRID_ROW_BRAKE_DELETE(CGridEditor::OnGridRowBrake)
EVT_GRID_COL_BRAKE_ADD(CGridEditor::OnGridColBrake)
EVT_GRID_COL_BRAKE_SET(CGridEditor::OnGridColBrake)
EVT_GRID_COL_BRAKE_DELETE(CGridEditor::OnGridRowBrake)
EVT_GRID_ROW_AREA_CREATE(CGridEditor::OnGridRowArea)
EVT_GRID_ROW_AREA_DELETE(CGridEditor::OnGridRowArea)
EVT_GRID_ROW_AREA_SIZE(CGridEditor::OnGridRowArea)
EVT_GRID_ROW_AREA_NAME(CGridEditor::OnGridRowArea)
EVT_GRID_COL_AREA_CREATE(CGridEditor::OnGridColArea)
EVT_GRID_COL_AREA_DELETE(CGridEditor::OnGridColArea)
EVT_GRID_COL_AREA_SIZE(CGridEditor::OnGridColArea)
EVT_GRID_COL_AREA_NAME(CGridEditor::OnGridColArea)
EVT_GRID_ROW_FREEZE(CGridEditor::OnGridRowFreeze)
EVT_GRID_COL_FREEZE(CGridEditor::OnGridColFreeze)
EVT_SCROLLWIN(CGridEditor::OnScroll)
EVT_IDLE(CGridEditor::OnIdle)
EVT_SIZE(CGridEditor::OnSize)
EVT_MENU(wxID_COPY, CGridEditor::OnCopy)
EVT_MENU(wxID_PASTE, CGridEditor::OnPaste)
EVT_MENU(wxID_DELETE, CGridEditor::OnDelete)
EVT_MENU(wxID_ROW_HEIGHT, CGridEditor::OnRowHeight)
EVT_MENU(wxID_COL_WIDTH, CGridEditor::OnColWidth)
EVT_MENU(wxID_HIDE_CELL, CGridEditor::OnHideCell)
EVT_MENU(wxID_SHOW_CELL, CGridEditor::OnShowCell)
EVT_MENU(wxID_PROPERTIES, CGridEditor::OnProperties)
EVT_GRID_ZOOM(CGridEditor::OnGridZoom)
wxEND_EVENT_TABLE()

////////////////////////////////////////////////////////////////////

#include "frontend/mainFrame/objinspect/objinspect.h"

void CGridEditor::ActivateEditor()
{
	if (m_enableProperty)
		objectInspector->SelectObject(m_cellProperty, true);
}

// ctor and Create() create the grid window, as with the other controls
CGridEditor::CGridEditor() : wxGridExt(),
m_cellProperty(nullptr), m_document(nullptr), m_spreadsheetObject(nullptr), m_enableProperty(true)
{
}

CGridEditor::CGridEditor(CMetaDocument* document,
	wxWindow* parent,
	wxWindowID id, const wxPoint& pos,
	const wxSize& size) : wxGridExt(parent, id, pos, size),
	m_cellProperty(nullptr), m_document(document), m_spreadsheetObject(nullptr), m_enableProperty(true)
{
	m_rowLabelWidth = s_rowLabelWidth;
	m_colLabelHeight = s_colLabelHeight;
	m_defaultColWidth = s_defaultColWidth;
	m_defaultRowHeight = s_defaultRowHeight;

	// Grid
	wxGridExt::EnableEditing(true);
	wxGridExt::EnableGridLines(true);
	wxGridExt::EnableDragGridSize(false);
	wxGridExt::SetScrollRate(15, 15);
	wxGridExt::SetMargins(0, 0);

	// Create property
	m_cellProperty = new CGridEditorCellProperty;
	m_cellProperty->SetView(this);

	// Native col 
	wxGridExt::SetDoubleBuffered(true);

	// Cell Defaults
	wxGridExt::SetDefaultCellAlignment(wxAlignment::wxALIGN_LEFT, wxAlignment::wxALIGN_BOTTOM);
	wxGridExt::SetTable(new CGridEditorStringTable, true);

	m_selectionBackground.Set(211, 217, 239);
	m_selectionForeground.Set(0, 0, 0);

	wxGridExt::SetLabelFont(s_defaultSpreadsheetFont);
	wxGridExt::SetDefaultCellFont(s_defaultSpreadsheetFont);

	wxGridExt::SetDefaultEditor(new CGridEditorCellTextEditor);

	wxAcceleratorEntry entries[5];
	entries[0].Set(wxACCEL_CTRL, (int)'A', wxID_SELECTALL);
	entries[1].Set(wxACCEL_CTRL, (int)'C', wxID_COPY);
	entries[2].Set(wxACCEL_CTRL, (int)'V', wxID_PASTE);

	entries[3].Set(wxACCEL_NORMAL, WXK_DELETE, wxID_DELETE);
	entries[4].Set(wxACCEL_NORMAL, WXK_NUMPAD_DELETE, wxID_DELETE);

	wxAcceleratorTable accel(5, entries);
	SetAcceleratorTable(accel);
}

CGridEditor::~CGridEditor()
{
	//delete property
	wxDELETE(m_cellProperty);
}

#include "gridPrintout.h"

CGridEditorPrintout* CGridEditor::CreatePrintout() const
{
	return new CGridEditorPrintout(m_spreadsheetObject, wxGP_SHOW_NONE, m_document ? m_document->GetTitle() : _("Spreadsheet document"));
}

#pragma region commands

void CGridEditor::AddArea()
{
	if (!wxGridExt::IsEditable())
		return;

	wxGridExt::CreateArea();
}

void CGridEditor::DeleteArea()
{
	if (!wxGridExt::IsEditable())
		return;

	wxGridExt::DeleteArea();
}

void CGridEditor::MergeCells()
{
	if (!wxGridExt::IsEditable())
		return;

	const wxGridExtBlockCoords& cellRange = GetSelectedCellRange();
	wxGridExt::SetCellSize(cellRange.GetTopRow(), cellRange.GetLeftCol(),
		cellRange.GetBottomRow() - cellRange.GetTopRow() + 1, cellRange.GetRightCol() - cellRange.GetLeftCol() + 1);
}

void CGridEditor::DockTable()
{
	if (!wxGridExt::IsEditable())
		return;

	const wxGridExtBlockCoords& cellRange = GetSelectedCellRange();
	CGridEditor::FreezeTo(cellRange.GetBottomRow(), cellRange.GetRightCol());
}

#pragma endregion

#pragma region events

#include "frontend/artProvider/artProvider.h"

void CGridEditor::OnMouseRightDown(wxGridExtEvent& event)
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

#include "frontend/win/ctrls/grid/gridextprivate.h"

// This seems to be required for wxMotif/wxGTK otherwise the mouse
// cursor must be in the cell edit control to get key events
//
void CGridEditor::OnKeyDown(wxKeyEvent& event)
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
			if (y1 >= wxGridExt::GetNumberRows() - wxGridExt::GetNumberFrozenRows() - 1) {
				wxGridExt::AppendRows();
				wxGridExt::SetScrollPos(wxOrientation::wxVERTICAL, (sy + uy) / uy);
			}
		}
		else if (scroll > 0 && code == WXK_RIGHT) {
			if (x1 >= wxGridExt::GetNumberCols() - wxGridExt::GetNumberFrozenCols() - 1) {
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

void CGridEditor::OnSelectCell(wxGridExtEvent& event)
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

void CGridEditor::OnSelectCells(wxGridExtRangeSelectEvent& event)
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

#include "backend/metaCollection/metaSpreadsheetObject.h"

void CGridEditor::OnGridRowSize(wxGridExtSizeEvent& event)
{
	if (m_document != nullptr) {

		IValueMetaObjectSpreadsheet* creator = m_document->ConvertMetaObjectToType<IValueMetaObjectSpreadsheet>();

		if (creator != nullptr) {
			CSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();
			spreadsheetDescription.SetRowSize(event.GetRowOrCol(), GetRowSize(event.GetRowOrCol()));
		}

		m_document->Modify(true);
	}

	CSpreadsheetDescription& spreadsheetDescription = m_spreadsheetObject->GetSpreadsheetDesc();
	spreadsheetDescription.SetRowSize(event.GetRowOrCol(), GetRowSize(event.GetRowOrCol()));

	event.Skip();
}

void CGridEditor::OnGridColSize(wxGridExtSizeEvent& event)
{
	if (m_document != nullptr) {

		IValueMetaObjectSpreadsheet* creator = m_document->ConvertMetaObjectToType<IValueMetaObjectSpreadsheet>();

		if (creator != nullptr) {
			CSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();
			spreadsheetDescription.SetColSize(event.GetRowOrCol(), GetColSize(event.GetRowOrCol()));
		}

		m_document->Modify(true);
	}

	if (m_spreadsheetObject != nullptr) {
		CSpreadsheetDescription& spreadsheetDescription = m_spreadsheetObject->GetSpreadsheetDesc();
		spreadsheetDescription.SetColSize(event.GetRowOrCol(), GetColSize(event.GetRowOrCol()));
	}

	event.Skip();
}

void CGridEditor::OnGridRowBrake(wxGridExtSizeEvent& event)
{
	if (m_document != nullptr) {

		IValueMetaObjectSpreadsheet* creator = m_document->ConvertMetaObjectToType<IValueMetaObjectSpreadsheet>();

		if (creator != nullptr) {
			CSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();
			if (event.GetEventType() == wxEVT_GRID_ROW_BRAKE_ADD)
				spreadsheetDescription.AddRowBrake(event.GetRowOrCol());
			else if (event.GetEventType() == wxEVT_GRID_ROW_BRAKE_SET)
				spreadsheetDescription.SetRowBrake(event.GetRowOrCol());
			else if (event.GetEventType() == wxEVT_GRID_ROW_BRAKE_DELETE)
				spreadsheetDescription.AddRowBrake(event.GetRowOrCol());
		}

		m_document->Modify(true);
	}

	if (m_spreadsheetObject != nullptr) {
		CSpreadsheetDescription& spreadsheetDescription = m_spreadsheetObject->GetSpreadsheetDesc();
		if (event.GetEventType() == wxEVT_GRID_ROW_BRAKE_ADD)
			spreadsheetDescription.AddRowBrake(event.GetRowOrCol());
		else if (event.GetEventType() == wxEVT_GRID_ROW_BRAKE_SET)
			spreadsheetDescription.SetRowBrake(event.GetRowOrCol());
		else if (event.GetEventType() == wxEVT_GRID_ROW_BRAKE_DELETE)
			spreadsheetDescription.AddRowBrake(event.GetRowOrCol());
	}

	event.Skip();
}

void CGridEditor::OnGridColBrake(wxGridExtSizeEvent& event)
{
	if (m_document != nullptr) {

		IValueMetaObjectSpreadsheet* creator = m_document->ConvertMetaObjectToType<IValueMetaObjectSpreadsheet>();

		if (creator != nullptr) {
			CSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();
			if (event.GetEventType() == wxEVT_GRID_COL_BRAKE_ADD)
				spreadsheetDescription.AddColBrake(event.GetRowOrCol());
			else if (event.GetEventType() == wxEVT_GRID_COL_BRAKE_SET)
				spreadsheetDescription.SetColBrake(event.GetRowOrCol());
			else if (event.GetEventType() == wxEVT_GRID_COL_BRAKE_DELETE)
				spreadsheetDescription.AddRowBrake(event.GetRowOrCol());
		}

		m_document->Modify(true);
	}

	if (m_spreadsheetObject != nullptr) {
		CSpreadsheetDescription& spreadsheetDescription = m_spreadsheetObject->GetSpreadsheetDesc();
		if (event.GetEventType() == wxEVT_GRID_COL_BRAKE_ADD)
			spreadsheetDescription.AddColBrake(event.GetRowOrCol());
		else if (event.GetEventType() == wxEVT_GRID_COL_BRAKE_SET)
			spreadsheetDescription.SetColBrake(event.GetRowOrCol());
		else if (event.GetEventType() == wxEVT_GRID_COL_BRAKE_DELETE)
			spreadsheetDescription.AddRowBrake(event.GetRowOrCol());
	}

	event.Skip();
}

void CGridEditor::OnGridRowArea(wxGridExtAreaEvent& event)
{
	if (m_document != nullptr) {

		IValueMetaObjectSpreadsheet* creator = m_document->ConvertMetaObjectToType<IValueMetaObjectSpreadsheet>();

		if (creator != nullptr) {
			CSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();
			if (event.GetEventType() == wxEVT_GRID_ROW_AREA_CREATE)
				spreadsheetDescription.AddRowArea(event.GetAreaLabel(), event.GetAreaStart(), event.GetAreaEnd());
			else if (event.GetEventType() == wxEVT_GRID_ROW_AREA_DELETE)
				spreadsheetDescription.DeleteRowArea(event.GetAreaLabel());
			else if (event.GetEventType() == wxEVT_GRID_ROW_AREA_SIZE)
				spreadsheetDescription.SetRowSizeArea(event.GetAreaLabel(), event.GetAreaStart(), event.GetAreaEnd());
			else if (event.GetEventType() == wxEVT_GRID_ROW_AREA_NAME)
				spreadsheetDescription.SetRowNameArea(event.GetRowOrColPos(), event.GetAreaLabel());
		}

		m_document->Modify(true);
	}

	if (m_spreadsheetObject != nullptr) {
		CSpreadsheetDescription& spreadsheetDescription = m_spreadsheetObject->GetSpreadsheetDesc();
		if (event.GetEventType() == wxEVT_GRID_ROW_AREA_CREATE)
			spreadsheetDescription.AddRowArea(event.GetAreaLabel(), event.GetAreaStart(), event.GetAreaEnd());
		else if (event.GetEventType() == wxEVT_GRID_ROW_AREA_DELETE)
			spreadsheetDescription.DeleteRowArea(event.GetAreaLabel());
		else if (event.GetEventType() == wxEVT_GRID_ROW_AREA_SIZE)
			spreadsheetDescription.SetRowSizeArea(event.GetAreaLabel(), event.GetAreaStart(), event.GetAreaEnd());
		else if (event.GetEventType() == wxEVT_GRID_ROW_AREA_NAME)
			spreadsheetDescription.SetRowNameArea(event.GetRowOrColPos(), event.GetAreaLabel());
	}

	event.Skip();
}

void CGridEditor::OnGridColArea(wxGridExtAreaEvent& event)
{
	if (m_document != nullptr) {

		IValueMetaObjectSpreadsheet* creator = m_document->ConvertMetaObjectToType<IValueMetaObjectSpreadsheet>();

		if (creator != nullptr) {
			CSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();
			if (event.GetEventType() == wxEVT_GRID_COL_AREA_CREATE)
				spreadsheetDescription.AddColArea(event.GetAreaLabel(), event.GetAreaStart(), event.GetAreaEnd());
			else if (event.GetEventType() == wxEVT_GRID_COL_AREA_DELETE)
				spreadsheetDescription.DeleteColArea(event.GetAreaLabel());
			else if (event.GetEventType() == wxEVT_GRID_COL_AREA_SIZE)
				spreadsheetDescription.SetColSizeArea(event.GetAreaLabel(), event.GetAreaStart(), event.GetAreaEnd());
			else if (event.GetEventType() == wxEVT_GRID_COL_AREA_NAME)
				spreadsheetDescription.SetColNameArea(event.GetRowOrColPos(), event.GetAreaLabel());
		}

		m_document->Modify(true);
	}

	if (m_spreadsheetObject != nullptr) {
		CSpreadsheetDescription& spreadsheetDescription = m_spreadsheetObject->GetSpreadsheetDesc();
		if (event.GetEventType() == wxEVT_GRID_COL_AREA_CREATE)
			spreadsheetDescription.AddColArea(event.GetAreaLabel(), event.GetAreaStart(), event.GetAreaEnd());
		else if (event.GetEventType() == wxEVT_GRID_COL_AREA_DELETE)
			spreadsheetDescription.DeleteColArea(event.GetAreaLabel());
		else if (event.GetEventType() == wxEVT_GRID_COL_AREA_SIZE)
			spreadsheetDescription.SetColSizeArea(event.GetAreaLabel(), event.GetAreaStart(), event.GetAreaEnd());
		else if (event.GetEventType() == wxEVT_GRID_COL_AREA_NAME)
			spreadsheetDescription.SetColNameArea(event.GetRowOrColPos(), event.GetAreaLabel());
	}

	event.Skip();
}

void CGridEditor::OnGridRowFreeze(wxGridExtSizeEvent& event)
{
	if (m_document != nullptr) {

		IValueMetaObjectSpreadsheet* creator = m_document->ConvertMetaObjectToType<IValueMetaObjectSpreadsheet>();

		if (creator != nullptr) {
			CSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();
			spreadsheetDescription.SetRowFreeze(event.GetRowOrCol());
		}

		m_document->Modify(true);
	}

	if (m_spreadsheetObject != nullptr) {
		CSpreadsheetDescription& spreadsheetDescription = m_spreadsheetObject->GetSpreadsheetDesc();
		spreadsheetDescription.SetRowFreeze(event.GetRowOrCol());
	}

	event.Skip();
}

void CGridEditor::OnGridColFreeze(wxGridExtSizeEvent& event)
{
	if (m_document != nullptr) {

		IValueMetaObjectSpreadsheet* creator = m_document->ConvertMetaObjectToType<IValueMetaObjectSpreadsheet>();

		if (creator != nullptr) {
			CSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();
			spreadsheetDescription.SetColFreeze(event.GetRowOrCol());
		}

		m_document->Modify(true);
	}

	if (m_spreadsheetObject != nullptr) {
		CSpreadsheetDescription& spreadsheetDescription = m_spreadsheetObject->GetSpreadsheetDesc();
		spreadsheetDescription.SetColFreeze(event.GetRowOrCol());
	}

	event.Skip();
}

void CGridEditor::OnGridTableModified(wxGridExtEvent& event)
{
	wxString value;

	void* tempval = m_table->GetValueAsCustom(event.GetRow(), event.GetCol(), wxT("translate"));
	if (tempval != nullptr) {
		const CBackendLocalizationEntryArray* array = static_cast<CBackendLocalizationEntryArray*>(tempval);
		if (array != nullptr) {
			CBackendLocalization::GetRawLocText(*array, value);
			wxDELETE(array);
		}
	}

	if (m_document != nullptr) {

		IValueMetaObjectSpreadsheet* creator = m_document->ConvertMetaObjectToType<IValueMetaObjectSpreadsheet>();

		if (creator != nullptr) {
			CSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();
			spreadsheetDescription.SetCellValue(event.GetRow(), event.GetCol(), value);
		}

		m_document->Modify(true);
	}

	if (m_spreadsheetObject != nullptr) {
		CSpreadsheetDescription& spreadsheetDescription = m_spreadsheetObject->GetSpreadsheetDesc();
		spreadsheetDescription.SetCellValue(event.GetRow(), event.GetCol(), value);
	}

	event.Skip();
}

void CGridEditor::OnGridTableAttrModified(wxGridExtEvent& event)
{
	int horiz, vert;
	GetCellAlignment(event.GetRow(), event.GetCol(), &horiz, &vert);

	int num_rows, num_cols;
	GetCellSize(event.GetRow(), event.GetCol(), &num_rows, &num_cols);

	if (m_document != nullptr) {

		IValueMetaObjectSpreadsheet* creator = m_document->ConvertMetaObjectToType<IValueMetaObjectSpreadsheet>();

		if (creator != nullptr) {

			CSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();

			spreadsheetDescription.SetCellBackgroundColour(event.GetRow(), event.GetCol(), GetCellBackgroundColour(event.GetRow(), event.GetCol()));
			spreadsheetDescription.SetCellTextColour(event.GetRow(), event.GetCol(), GetCellTextColour(event.GetRow(), event.GetCol()));
			spreadsheetDescription.SetCellTextOrient(event.GetRow(), event.GetCol(), GetCellTextOrient(event.GetRow(), event.GetCol()));
			spreadsheetDescription.SetCellFont(event.GetRow(), event.GetCol(), GetCellFont(event.GetRow(), event.GetCol()));
			spreadsheetDescription.SetCellAlignment(event.GetRow(), event.GetCol(), horiz, vert);

			//border 
			const wxGridExtCellBorder& borderLeft = GetCellBorderLeft(event.GetRow(), event.GetCol());
			spreadsheetDescription.SetCellBorderLeft(event.GetRow(), event.GetCol(), { borderLeft.m_style, borderLeft.m_colour, borderLeft.m_width });
			const wxGridExtCellBorder& borderRight = GetCellBorderLeft(event.GetRow(), event.GetCol());
			spreadsheetDescription.SetCellBorderRight(event.GetRow(), event.GetCol(), { borderRight.m_style, borderRight.m_colour, borderRight.m_width });
			const wxGridExtCellBorder& borderTop = GetCellBorderLeft(event.GetRow(), event.GetCol());
			spreadsheetDescription.SetCellBorderTop(event.GetRow(), event.GetCol(), { borderTop.m_style, borderTop.m_colour, borderTop.m_width });
			const wxGridExtCellBorder& borderBottom = GetCellBorderLeft(event.GetRow(), event.GetCol());
			spreadsheetDescription.SetCellBorderBottom(event.GetRow(), event.GetCol(), { borderBottom.m_style, borderBottom.m_colour, borderBottom.m_width });

			//size 
			spreadsheetDescription.SetCellSize(event.GetRow(), event.GetCol(), num_rows, num_cols);

			//cell
			wxGridExtFitMode fitMode =
				GetCellFitMode(event.GetRow(), event.GetCol());

			spreadsheetDescription.SetCellFitMode(event.GetRow(), event.GetCol(), fitMode.IsOverflow() ? CSpreadsheetCellDescription::EFitMode::Mode_Overflow : CSpreadsheetCellDescription::EFitMode::Mode_Clip);
			spreadsheetDescription.SetCellReadOnly(event.GetRow(), event.GetCol(), IsCellReadOnly(event.GetRow(), event.GetCol()));
		}

		m_document->Modify(true);
	}

	if (m_spreadsheetObject != nullptr) {

		CSpreadsheetDescription& spreadsheetDescription = m_spreadsheetObject->GetSpreadsheetDesc();

		spreadsheetDescription.SetCellBackgroundColour(event.GetRow(), event.GetCol(), GetCellBackgroundColour(event.GetRow(), event.GetCol()));
		spreadsheetDescription.SetCellTextColour(event.GetRow(), event.GetCol(), GetCellTextColour(event.GetRow(), event.GetCol()));
		spreadsheetDescription.SetCellTextOrient(event.GetRow(), event.GetCol(), GetCellTextOrient(event.GetRow(), event.GetCol()));
		spreadsheetDescription.SetCellFont(event.GetRow(), event.GetCol(), GetCellFont(event.GetRow(), event.GetCol()));
		spreadsheetDescription.SetCellAlignment(event.GetRow(), event.GetCol(), horiz, vert);

		//border 
		const wxGridExtCellBorder& borderLeft = GetCellBorderLeft(event.GetRow(), event.GetCol());
		spreadsheetDescription.SetCellBorderLeft(event.GetRow(), event.GetCol(), { borderLeft.m_style, borderLeft.m_colour, borderLeft.m_width });
		const wxGridExtCellBorder& borderRight = GetCellBorderLeft(event.GetRow(), event.GetCol());
		spreadsheetDescription.SetCellBorderRight(event.GetRow(), event.GetCol(), { borderRight.m_style, borderRight.m_colour, borderRight.m_width });
		const wxGridExtCellBorder& borderTop = GetCellBorderLeft(event.GetRow(), event.GetCol());
		spreadsheetDescription.SetCellBorderTop(event.GetRow(), event.GetCol(), { borderTop.m_style, borderTop.m_colour, borderTop.m_width });
		const wxGridExtCellBorder& borderBottom = GetCellBorderLeft(event.GetRow(), event.GetCol());
		spreadsheetDescription.SetCellBorderBottom(event.GetRow(), event.GetCol(), { borderBottom.m_style, borderBottom.m_colour, borderBottom.m_width });

		//size 
		spreadsheetDescription.SetCellSize(event.GetRow(), event.GetCol(), num_rows, num_cols);

		//cell
		wxGridExtFitMode fitMode =
			GetCellFitMode(event.GetRow(), event.GetCol());

		spreadsheetDescription.SetCellFitMode(event.GetRow(), event.GetCol(), fitMode.IsOverflow() ? CSpreadsheetCellDescription::EFitMode::Mode_Overflow : CSpreadsheetCellDescription::EFitMode::Mode_Clip);
		spreadsheetDescription.SetCellReadOnly(event.GetRow(), event.GetCol(), IsCellReadOnly(event.GetRow(), event.GetCol()));
	}

	event.Skip();
}

void CGridEditor::OnScroll(wxScrollWinEvent& event)
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
		if (y1 >= wxGridExt::GetNumberRows() - wxGridExt::GetNumberFrozenRows() - 1) {
			wxGridExt::AppendRows();
			wxGridExt::SetScrollPos(wxOrientation::wxVERTICAL, (sy + scroll) / uy);
		}
	}
	else if (scroll > 0 && event.GetOrientation() == wxOrientation::wxHORIZONTAL) {
		if (x1 >= wxGridExt::GetNumberCols() - wxGridExt::GetNumberFrozenCols() - 1) {
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

void CGridEditor::OnIdle(wxIdleEvent& event)
{
	event.Skip();
}

void CGridEditor::OnSize(wxSizeEvent& event)
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
		const int col = w / wxCalcGridScale(m_defaultColWidth, GetGridZoom());
		if ((col + 1) > m_table->GetColsCount())
			m_table->AppendCols(col + 1);
	}
	else if (m_table != nullptr && x1 == wxNOT_FOUND) {
		int col_size = 0;
		for (int col = x0; col < m_table->GetColsCount(); col++)
			col_size += wxGridExt::GetColWidth(col, GetGridZoom());
		if (w > col_size) {
			const int col = ((w - col_size) / wxCalcGridScale(m_defaultColWidth, GetGridZoom()));
			m_table->AppendCols(col + 1);
		}
	}

	if (m_table != nullptr && y0 == wxNOT_FOUND) {
		const int row = h / wxCalcGridScale(m_defaultRowHeight, GetGridZoom());
		if ((row + 1) > m_table->GetRowsCount())
			m_table->AppendRows(row + 1);
	}
	else if (m_table != nullptr && y1 == wxNOT_FOUND) {
		int row_size = 0;
		for (int row = y0; row < m_table->GetRowsCount(); row++)
			row_size += wxGridExt::GetRowHeight(row, GetGridZoom());
		if (h > row_size) {
			const int row = ((h - row_size) / wxCalcGridScale(m_defaultRowHeight, GetGridZoom()));
			m_table->AppendRows(row + 1);
		}
	}

	event.Skip();
}

void CGridEditor::OnGridZoom(wxGridExtEvent& event)
{
	int ux, uy,
		sx, sy;

	wxGridExt::GetScrollPixelsPerUnit(&ux, &uy);
	wxGridExt::GetViewStart(&sx, &sy);

	sx *= ux; sy *= uy;

	int w, h;
	wxGridExt::GetSize(&w, &h);

	int x0 = wxGridExt::XToCol(sx);
	int y0 = wxGridExt::YToRow(sy);
	int x1 = wxGridExt::XToCol(sx + w);
	int y1 = wxGridExt::YToRow(sy + h);

	if (m_table != nullptr && x0 == wxNOT_FOUND) {
		const int col = w / wxCalcGridScale(m_defaultColWidth, GetGridZoom());
		if ((col + 1) > m_table->GetColsCount())
			m_table->AppendCols(col + 1);
	}
	else if (m_table != nullptr && x1 == wxNOT_FOUND) {
		int col_size = 0;
		for (int col = x0; col < m_table->GetColsCount(); col++)
			col_size += wxGridExt::GetColWidth(col, GetGridZoom());
		if (w > col_size) {
			const int col = ((w - col_size) / wxCalcGridScale(m_defaultColWidth, GetGridZoom()));
			m_table->AppendCols(col + 1);
		}
	}

	if (m_table != nullptr && y0 == wxNOT_FOUND) {
		const int row = h / wxCalcGridScale(m_defaultRowHeight, GetGridZoom());
		if ((row + 1) > m_table->GetRowsCount())
			m_table->AppendRows(row + 1);
	}
	else if (m_table != nullptr && y1 == wxNOT_FOUND) {
		int row_size = 0;
		for (int row = y0; row < m_table->GetRowsCount(); row++)
			row_size += wxGridExt::GetRowHeight(row, GetGridZoom());
		if (h > row_size) {
			const int row = ((h - row_size) / wxCalcGridScale(m_defaultRowHeight, GetGridZoom()));
			m_table->AppendRows(row + 1);
		}
	}

	event.Skip();
}

#pragma endregion

#pragma region context_menu
void CGridEditor::OnCopy(wxCommandEvent& event)
{
	Copy();
}

void CGridEditor::OnPaste(wxCommandEvent& event)
{
	Paste();
}

void CGridEditor::OnDelete(wxCommandEvent& event)
{
	for (auto cell : wxGridExt::GetSelectedBlocks()) {
		for (int col = cell.GetLeftCol(); col <= cell.GetRightCol(); col++) {
			for (int row = cell.GetTopRow(); row <= cell.GetBottomRow(); row++) {

				SetAttr(row, col, nullptr);
				SetCellValue(row, col, wxEmptyString);
			}
		}

		DeleteArea();

		if (cell.GetLeftCol() <= GetMaxColBrake() && cell.GetRightCol() >= GetMaxColBrake() && cell.GetLeftCol() > 0) {
			for (int col = cell.GetRightCol(); col >= cell.GetLeftCol(); col--) {
				wxGridExt::SetColSize(col, WXGRID_DEFAULT_ROW_LABEL_WIDTH - 12);
			}
			m_colBrakeAt.Last() = cell.GetLeftCol() - 1;
		}

		if (cell.GetTopRow() <= GetMaxRowBrake() && cell.GetBottomRow() >= GetMaxRowBrake() && cell.GetTopRow() > 0) {
			for (int row = cell.GetBottomRow(); row >= cell.GetTopRow(); row--) {
				wxGridExt::SetRowSize(row, WXGRID_MIN_ROW_HEIGHT);
			}
			m_rowBrakeAt.Last() = cell.GetTopRow() - 1;
		}

		if (!cell.GetLeftCol() &&
			!cell.GetTopRow()) {
			for (int col = cell.GetRightCol(); col >= 0; col--)
				wxGridExt::SetColSize(col, WXGRID_DEFAULT_ROW_LABEL_WIDTH - 12);
			for (int row = cell.GetBottomRow(); row >= 0; row--)
				wxGridExt::SetRowSize(row, WXGRID_MIN_ROW_HEIGHT);
			m_colBrakeAt.clear(); m_rowBrakeAt.clear();
		}
	}
}

#include "frontend/win/dlgs/rowHeight.h"

void CGridEditor::OnRowHeight(wxCommandEvent& event)
{
	std::shared_ptr <CDialogRowHeight> rowHeight(new CDialogRowHeight(this));
	const int result = rowHeight->ShowModal();
	if (result == wxID_OK) {
		for (auto cell : wxGridExt::GetSelectedBlocks()) {
			for (int row = cell.GetTopRow(); row <= cell.GetBottomRow(); row++) {
				const int size = wxGridExt::GetRowSize(row);
				wxGridExt::SetRowSize(row, rowHeight->GetHeight());
			}
		}
	}
}

#include "frontend/win/dlgs/colHeight.h"

void CGridEditor::OnColWidth(wxCommandEvent& event)
{
	std::shared_ptr<CDialogColWidth> colWidth(new CDialogColWidth(this));
	const int result = colWidth->ShowModal();
	if (result == wxID_OK) {
		for (auto cell : wxGridExt::GetSelectedBlocks()) {
			for (int col = cell.GetLeftCol(); col <= cell.GetRightCol(); col++) {
				const int size = wxGridExt::GetColSize(col);
				wxGridExt::SetColSize(col, colWidth->GetWidth());
			}
		}
	}
}

void CGridEditor::OnHideCell(wxCommandEvent& event)
{
	for (auto cell : wxGridExt::GetSelectedBlocks()) {
		if (cell.GetLeftCol() > 0 &&
			cell.GetTopRow() == 0) {
			for (int col = cell.GetLeftCol(); col <= cell.GetRightCol(); col++) {
				const int size = wxGridExt::GetColSize(col);
				wxGridExt::SetColSize(col, 0);
			}
		}
		else if (cell.GetTopRow() > 0 &&
			cell.GetLeftCol() == 0) {
			for (int row = cell.GetTopRow(); row <= cell.GetBottomRow(); row++) {
				const int size = wxGridExt::GetRowSize(row);
				wxGridExt::SetRowSize(row, 0);
			}
		}
		else if (cell.GetLeftCol() == 0 &&
			cell.GetTopRow() == 0) {
			if (cell.GetBottomRow() == m_numRows - 1) {
				for (int col = cell.GetLeftCol(); col <= cell.GetRightCol(); col++) {
					const int size = wxGridExt::GetColSize(col);
					wxGridExt::SetColSize(col, 0);
				}
			}
			else if (cell.GetRightCol() == m_numCols - 1) {
				for (int row = cell.GetTopRow(); row <= cell.GetBottomRow(); row++) {
					const int size = wxGridExt::GetRowSize(row);
					wxGridExt::SetRowSize(row, 0);
				}
			}
		}
	}
}

void CGridEditor::OnShowCell(wxCommandEvent& event)
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
				const int size = wxGridExt::GetColSize(col);
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
				const int size = wxGridExt::GetRowSize(row);
				wxGridExt::SetRowSize(row, wxNOT_FOUND);
			}
		}
		else if (cell.GetLeftCol() == 0 &&
			cell.GetTopRow() == 0) {
			if (cell.GetBottomRow() == m_numRows - 1) {
				for (int col = cell.GetLeftCol(); col <= cell.GetRightCol(); col++) {
					const int size = wxGridExt::GetColSize(col);
					wxGridExt::SetColSize(col, wxNOT_FOUND);
				}
			}
			else if (cell.GetRightCol() == m_numCols - 1) {
				for (int row = cell.GetTopRow(); row <= cell.GetBottomRow(); row++) {
					const int size = wxGridExt::GetRowSize(row);
					wxGridExt::SetRowSize(row, wxNOT_FOUND);
				}
			}
		}
	}
}

void CGridEditor::OnProperties(wxCommandEvent& event)
{
	m_cellProperty->ShowProperty();
}
#pragma endregion  