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

wxBEGIN_EVENT_TABLE(ibGridEditor, ibGrid)
EVT_GRID_CELL_LEFT_DCLICK(ibGridEditor::OnMouseLeftDown)
EVT_GRID_CELL_RIGHT_CLICK(ibGridEditor::OnMouseRightDown)
EVT_GRID_LABEL_RIGHT_CLICK(ibGridEditor::OnMouseRightDown)
EVT_KEY_DOWN(ibGridEditor::OnKeyDown)
EVT_GRID_ROW_MODIFIED(ibGridEditor::OnGridRowSize)
EVT_GRID_COL_MODIFIED(ibGridEditor::OnGridColSize)
EVT_GRID_TABLE_MODIFIED(ibGridEditor::OnGridTableModified)
EVT_GRID_TABLE_ATTR_MODIFIED(ibGridEditor::OnGridTableAttrModified)
EVT_GRID_ROW_BRAKE_ADD(ibGridEditor::OnGridRowBrake)
EVT_GRID_ROW_BRAKE_SET(ibGridEditor::OnGridRowBrake)
EVT_GRID_ROW_BRAKE_DELETE(ibGridEditor::OnGridRowBrake)
EVT_GRID_COL_BRAKE_ADD(ibGridEditor::OnGridColBrake)
EVT_GRID_COL_BRAKE_SET(ibGridEditor::OnGridColBrake)
EVT_GRID_COL_BRAKE_DELETE(ibGridEditor::OnGridRowBrake)
EVT_GRID_ROW_AREA_CREATE(ibGridEditor::OnGridRowArea)
EVT_GRID_ROW_AREA_DELETE(ibGridEditor::OnGridRowArea)
EVT_GRID_ROW_AREA_SIZE(ibGridEditor::OnGridRowArea)
EVT_GRID_ROW_AREA_NAME(ibGridEditor::OnGridRowArea)
EVT_GRID_COL_AREA_CREATE(ibGridEditor::OnGridColArea)
EVT_GRID_COL_AREA_DELETE(ibGridEditor::OnGridColArea)
EVT_GRID_COL_AREA_SIZE(ibGridEditor::OnGridColArea)
EVT_GRID_COL_AREA_NAME(ibGridEditor::OnGridColArea)
EVT_GRID_ROW_FREEZE(ibGridEditor::OnGridRowFreeze)
EVT_GRID_COL_FREEZE(ibGridEditor::OnGridColFreeze)
EVT_SCROLLWIN(ibGridEditor::OnScroll)
EVT_IDLE(ibGridEditor::OnIdle)
EVT_SIZE(ibGridEditor::OnSize)
EVT_MENU(wxID_COPY, ibGridEditor::OnCopy)
EVT_MENU(wxID_PASTE, ibGridEditor::OnPaste)
EVT_MENU(wxID_DELETE, ibGridEditor::OnDelete)
EVT_MENU(wxID_ROW_HEIGHT, ibGridEditor::OnRowHeight)
EVT_MENU(wxID_COL_WIDTH, ibGridEditor::OnColWidth)
EVT_MENU(wxID_HIDE_CELL, ibGridEditor::OnHideCell)
EVT_MENU(wxID_SHOW_CELL, ibGridEditor::OnShowCell)
EVT_MENU(wxID_PROPERTIES, ibGridEditor::OnProperties)
EVT_GRID_ZOOM(ibGridEditor::OnGridZoom)
wxEND_EVENT_TABLE()

////////////////////////////////////////////////////////////////////

#include "frontend/mainFrame/objinspect/objinspect.h"

void ibGridEditor::ActivateEditor()
{
	if (m_enableProperty)
		objectInspector->SelectObject(m_propertySpreadsheet, true);
}

// ctor and Create() create the grid window, as with the other controls
ibGridEditor::ibGridEditor() : ibGrid(),
m_propertySpreadsheet(nullptr), m_document(nullptr), m_spreadsheetObject(nullptr), m_enableProperty(true)
{
}

ibGridEditor::ibGridEditor(ibMetaDocument* document,
	wxWindow* parent,
	wxWindowID id, const wxPoint& pos,
	const wxSize& size) : ibGrid(parent, id, pos, size),
	m_propertySpreadsheet(nullptr), m_document(document), m_spreadsheetObject(nullptr), m_enableProperty(true)
{
	m_rowLabelWidth = s_rowLabelWidth;
	m_colLabelHeight = s_colLabelHeight;
	m_defaultColWidth = s_defaultColWidth;
	m_defaultRowHeight = s_defaultRowHeight;

	// Grid
	ibGrid::EnableEditing(true);
	ibGrid::EnableGridLines(true);
	ibGrid::EnableDragGridSize(false);
	ibGrid::SetScrollRate(15, 15);
	ibGrid::SetMargins(0, 0);

	// Create property
	m_propertySpreadsheet = new ibPropertyGridEditorSpreadsheet(this);

	// Native col 
	ibGrid::SetDoubleBuffered(true);

	// Cell Defaults
	ibGrid::SetDefaultCellAlignment(wxAlignment::wxALIGN_LEFT, wxAlignment::wxALIGN_BOTTOM);
	ibGrid::SetTable(new ibGridEditorStringTable, true);

	m_selectionBackground.Set(211, 217, 239);
	m_selectionForeground.Set(0, 0, 0);

	ibGrid::SetLabelFont(s_defaultSpreadsheetFont);
	ibGrid::SetDefaultCellFont(s_defaultSpreadsheetFont);

	ibGrid::SetDefaultEditor(new ibGridEditorCellTextEditor);

	wxAcceleratorEntry entries[5];
	entries[0].Set(wxACCEL_CTRL, (int)'A', wxID_SELECTALL);
	entries[1].Set(wxACCEL_CTRL, (int)'C', wxID_COPY);
	entries[2].Set(wxACCEL_CTRL, (int)'V', wxID_PASTE);

	entries[3].Set(wxACCEL_NORMAL, WXK_DELETE, wxID_DELETE);
	entries[4].Set(wxACCEL_NORMAL, WXK_NUMPAD_DELETE, wxID_DELETE);

	wxAcceleratorTable accel(5, entries);
	SetAcceleratorTable(accel);
}

ibGridEditor::~ibGridEditor()
{
	//delete property
	wxDELETE(m_propertySpreadsheet);
}

#include "gridPrintout.h"

ibGridEditorPrintout* ibGridEditor::CreatePrintout() const
{
	return new ibGridEditorPrintout(m_spreadsheetObject, wxGP_SHOW_NONE, m_document ? m_document->GetTitle() : _("Spreadsheet document"));
}

#pragma region commands

void ibGridEditor::AddArea()
{
	if (!ibGrid::IsEditable())
		return;

	ibGrid::CreateArea();
	ibGrid::ForceRefresh();
}

void ibGridEditor::DeleteArea()
{
	if (!ibGrid::IsEditable())
		return;

	ibGrid::DeleteArea();
	ibGrid::ForceRefresh();
}

void ibGridEditor::MergeCells()
{
	if (!ibGrid::IsEditable())
		return;

	const ibGridBlockCoords& cellRange = GetSelectedCellRange();
	ibGrid::SetCellSize(cellRange.GetTopRow(), cellRange.GetLeftCol(),
		cellRange.GetBottomRow() - cellRange.GetTopRow() + 1, cellRange.GetRightCol() - cellRange.GetLeftCol() + 1);

	ibGrid::ForceRefresh();
}

void ibGridEditor::DockTable()
{
	if (!ibGrid::IsEditable())
		return;

	const ibGridBlockCoords& cellRange = GetSelectedCellRange();
	ibGridEditor::FreezeTo(cellRange.GetBottomRow(), cellRange.GetRightCol());

	ibGrid::ForceRefresh();
}

#pragma endregion

#include "backend/metaCollection/metaSpreadsheetObject.h"

void ibGridEditor::GetCellDetailsParameter(int row, int col, wxString& s) const
{
	if (m_spreadsheetObject != nullptr)
		m_spreadsheetObject->GetCellDetailsParameter(row, col, s);
}

void ibGridEditor::SetCellDetailsParameter(int row, int col, const wxString& s)
{
	if (m_document != nullptr) {

		ibValueMetaObjectSpreadsheetBase* creator = m_document->ConvertMetaObjectToType<ibValueMetaObjectSpreadsheetBase>();

		if (creator != nullptr) {
			ibSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();
			spreadsheetDescription.SetCellDetailsParameter(row, col, s);
		}

		m_document->Modify(true);
	}

	if (m_spreadsheetObject != nullptr) 
		m_spreadsheetObject->SetCellDetailsParameter(row, col, s);

	// set new brake pos
	SetRowBrake(row);
	SetColBrake(col);
}

void ibGridEditor::SetCellDetailsParameter(const ibGridBlockCoords& coords, const wxString& s, bool sendUndoCommand)
{
	for (int row = coords.GetTopRow(); row <= coords.GetBottomRow(); row++)
	{
		for (int col = coords.GetLeftCol(); col <= coords.GetRightCol(); col++)
		{
			SetCellDetailsParameter(row, col, s);
		}
	}

	// set new brake pos
	SetRowBrake(coords.GetBottomRow());
	SetColBrake(coords.GetRightCol());
}

#pragma region events

#include "frontend/artProvider/artProvider.h"

void ibGridEditor::OnMouseLeftDown(ibGridEvent& event)
{
	if (m_spreadsheetObject != nullptr && !IsEditable())
		m_spreadsheetObject->OpenCellDetailsParameter(event.GetRow(), event.GetCol());

	event.Skip();
}

void ibGridEditor::OnMouseRightDown(ibGridEvent& event)
{
	if (event.GetRow() == wxNOT_FOUND &&
		event.GetCol() == wxNOT_FOUND) {
	}
	else if (event.GetRow() == wxNOT_FOUND) {
		ibGrid::SetCurrentCell(0, event.GetCol()); bool foundedCol = false;
		for (auto block : ibGrid::GetSelectedBlocks()) {
			if (block.GetLeftCol() <= event.GetCol() && block.GetTopRow() == 0
				&& block.GetRightCol() >= event.GetCol() && block.GetBottomRow() == m_numRows - 1) {
				foundedCol = true; break;
			}
		}
		if (!foundedCol) {
			ibGrid::SelectBlock(0, event.GetCol(), m_numRows - 1, event.GetCol());
		}
	}
	else if (event.GetCol() == wxNOT_FOUND) {
		ibGrid::SetCurrentCell(event.GetRow(), 0); bool foundedRow = false;
		for (auto block : ibGrid::GetSelectedBlocks()) {
			if (block.GetLeftCol() == 0 && block.GetTopRow() <= event.GetRow() &&
				block.GetRightCol() == m_numCols - 1 && block.GetBottomRow() >= event.GetRow()) {
				foundedRow = true; break;
			}
		}
		if (!foundedRow) {
			ibGrid::SelectBlock(event.GetRow(), 0, event.GetRow(), m_numCols - 1);
		}
	}
	else {
		ibGrid::SetCurrentCell(event.GetRow(), event.GetCol());
	}

	if (m_enableProperty) {

		wxMenuItem* item = nullptr; wxMenu menuPopup;
		item = menuPopup.Append(wxID_COPY, _("Copy"));
		item->SetBitmap(wxArtProvider::GetBitmap(wxART_COPY, wxART_MENU));
		item = menuPopup.Append(wxID_PASTE, _("Paste"));
		item->SetBitmap(wxArtProvider::GetBitmap(wxART_PASTE, wxART_MENU));

		if (!ibGrid::CanEnableCellControl())
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

		if (ibGrid::PopupMenu(&menuPopup, event.GetPosition())) {
			event.Skip();
		}
	}
}

#include "frontend/win/ctrls/grid/gridextprivate.h"

// This seems to be required for wxMotif/wxGTK otherwise the mouse
// cursor must be in the cell edit control to get key events
//
void ibGridEditor::OnKeyDown(wxKeyEvent& event)
{
	const int code = event.GetKeyCode();

	if (code == WXK_DOWN || code == WXK_UP || code == WXK_LEFT || code == WXK_RIGHT ||
		code == WXK_NUMPAD_DOWN || code == WXK_NUMPAD_UP || code == WXK_NUMPAD_LEFT || code == WXK_NUMPAD_RIGHT) {

		int ux, uy,
			sx, sy;

		ibGrid::GetScrollPixelsPerUnit(&ux, &uy);
		ibGrid::GetViewStart(&sx, &sy);

		sx *= ux; sy *= uy;

		int w, h;
		m_gridWin->GetClientSize(&w, &h);

		const int x0 = ibGrid::XToCol(sx, true);
		const int y0 = ibGrid::YToRow(sy, true);

		const int x1 = ibGrid::XToCol(sx + w, true);
		const int y1 = ibGrid::YToRow(sy + h, true);

		const short scroll = (code == WXK_UP || code == WXK_LEFT) ? -1 : 1;

		if (scroll > 0 && code == WXK_DOWN) {
			if (y1 >= ibGrid::GetNumberRows() - ibGrid::GetNumberFrozenRows() - 1) {
				ibGrid::AppendRows();
				ibGrid::SetScrollPos(wxOrientation::wxVERTICAL, (sy + uy) / uy);
			}
		}
		else if (scroll > 0 && code == WXK_RIGHT) {
			if (x1 >= ibGrid::GetNumberCols() - ibGrid::GetNumberFrozenCols() - 1) {
				ibGrid::AppendCols();
				ibGrid::SetScrollPos(wxOrientation::wxHORIZONTAL, (sx + ux) / ux);
			}
		}
		else if (scroll < 0 && code == WXK_UP) {
			if (y0 > GetMaxRowBrake() && y0 != 0) ibGrid::DeleteRows(ibGrid::GetNumberRows() - 1);
		}
		else if (scroll < 0 && code == WXK_LEFT) {
			if (x0 > GetMaxColBrake() && x0 != 0) ibGrid::DeleteCols(ibGrid::GetNumberCols() - 1);
		}
	}

	event.Skip();
}

void ibGridEditor::OnGridRowSize(ibGridSizeEvent& event)
{
	if (m_document != nullptr) {

		ibValueMetaObjectSpreadsheetBase* creator = m_document->ConvertMetaObjectToType<ibValueMetaObjectSpreadsheetBase>();

		if (creator != nullptr) {
			ibSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();
			spreadsheetDescription.SetRowSize(event.GetRowOrCol(), GetRowSize(event.GetRowOrCol()));
		}

		m_document->Modify(true);
	}

	ibSpreadsheetDescription& spreadsheetDescription = m_spreadsheetObject->GetSpreadsheetDesc();
	spreadsheetDescription.SetRowSize(event.GetRowOrCol(), GetRowSize(event.GetRowOrCol()));

	event.Skip();
}

void ibGridEditor::OnGridColSize(ibGridSizeEvent& event)
{
	if (m_document != nullptr) {

		ibValueMetaObjectSpreadsheetBase* creator = m_document->ConvertMetaObjectToType<ibValueMetaObjectSpreadsheetBase>();

		if (creator != nullptr) {
			ibSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();
			spreadsheetDescription.SetColSize(event.GetRowOrCol(), GetColSize(event.GetRowOrCol()));
		}

		m_document->Modify(true);
	}

	if (m_spreadsheetObject != nullptr) {
		ibSpreadsheetDescription& spreadsheetDescription = m_spreadsheetObject->GetSpreadsheetDesc();
		spreadsheetDescription.SetColSize(event.GetRowOrCol(), GetColSize(event.GetRowOrCol()));
	}

	event.Skip();
}

void ibGridEditor::OnGridRowBrake(ibGridSizeEvent& event)
{
	if (m_document != nullptr) {

		ibValueMetaObjectSpreadsheetBase* creator = m_document->ConvertMetaObjectToType<ibValueMetaObjectSpreadsheetBase>();

		if (creator != nullptr) {
			ibSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();
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
		ibSpreadsheetDescription& spreadsheetDescription = m_spreadsheetObject->GetSpreadsheetDesc();
		if (event.GetEventType() == wxEVT_GRID_ROW_BRAKE_ADD)
			spreadsheetDescription.AddRowBrake(event.GetRowOrCol());
		else if (event.GetEventType() == wxEVT_GRID_ROW_BRAKE_SET)
			spreadsheetDescription.SetRowBrake(event.GetRowOrCol());
		else if (event.GetEventType() == wxEVT_GRID_ROW_BRAKE_DELETE)
			spreadsheetDescription.AddRowBrake(event.GetRowOrCol());
	}

	event.Skip();
}

void ibGridEditor::OnGridColBrake(ibGridSizeEvent& event)
{
	if (m_document != nullptr) {

		ibValueMetaObjectSpreadsheetBase* creator = m_document->ConvertMetaObjectToType<ibValueMetaObjectSpreadsheetBase>();

		if (creator != nullptr) {
			ibSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();
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
		ibSpreadsheetDescription& spreadsheetDescription = m_spreadsheetObject->GetSpreadsheetDesc();
		if (event.GetEventType() == wxEVT_GRID_COL_BRAKE_ADD)
			spreadsheetDescription.AddColBrake(event.GetRowOrCol());
		else if (event.GetEventType() == wxEVT_GRID_COL_BRAKE_SET)
			spreadsheetDescription.SetColBrake(event.GetRowOrCol());
		else if (event.GetEventType() == wxEVT_GRID_COL_BRAKE_DELETE)
			spreadsheetDescription.AddRowBrake(event.GetRowOrCol());
	}

	event.Skip();
}

void ibGridEditor::OnGridRowArea(ibGridAreaEvent& event)
{
	if (m_document != nullptr) {

		ibValueMetaObjectSpreadsheetBase* creator = m_document->ConvertMetaObjectToType<ibValueMetaObjectSpreadsheetBase>();

		if (creator != nullptr) {
			ibSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();
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
		ibSpreadsheetDescription& spreadsheetDescription = m_spreadsheetObject->GetSpreadsheetDesc();
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

void ibGridEditor::OnGridColArea(ibGridAreaEvent& event)
{
	if (m_document != nullptr) {

		ibValueMetaObjectSpreadsheetBase* creator = m_document->ConvertMetaObjectToType<ibValueMetaObjectSpreadsheetBase>();

		if (creator != nullptr) {
			ibSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();
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
		ibSpreadsheetDescription& spreadsheetDescription = m_spreadsheetObject->GetSpreadsheetDesc();
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

void ibGridEditor::OnGridRowFreeze(ibGridSizeEvent& event)
{
	if (m_document != nullptr) {

		ibValueMetaObjectSpreadsheetBase* creator = m_document->ConvertMetaObjectToType<ibValueMetaObjectSpreadsheetBase>();

		if (creator != nullptr) {
			ibSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();
			spreadsheetDescription.SetRowFreeze(event.GetRowOrCol());
		}

		m_document->Modify(true);
	}

	if (m_spreadsheetObject != nullptr) {
		ibSpreadsheetDescription& spreadsheetDescription = m_spreadsheetObject->GetSpreadsheetDesc();
		spreadsheetDescription.SetRowFreeze(event.GetRowOrCol());
	}

	event.Skip();
}

void ibGridEditor::OnGridColFreeze(ibGridSizeEvent& event)
{
	if (m_document != nullptr) {

		ibValueMetaObjectSpreadsheetBase* creator = m_document->ConvertMetaObjectToType<ibValueMetaObjectSpreadsheetBase>();

		if (creator != nullptr) {
			ibSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();
			spreadsheetDescription.SetColFreeze(event.GetRowOrCol());
		}

		m_document->Modify(true);
	}

	if (m_spreadsheetObject != nullptr) {
		ibSpreadsheetDescription& spreadsheetDescription = m_spreadsheetObject->GetSpreadsheetDesc();
		spreadsheetDescription.SetColFreeze(event.GetRowOrCol());
	}

	event.Skip();
}

void ibGridEditor::OnGridTableModified(ibGridEvent& event)
{
	ibSpreadsheetFillType type = ibSpreadsheetFillType::ibSpreadsheetFillType_StrText;

	if (m_table->CanGetValueAs(event.GetRow(), event.GetCol(), s_strTypeTextOrString))
		type = ibSpreadsheetFillType::ibSpreadsheetFillType_StrText;
	else if (m_table->CanGetValueAs(event.GetRow(), event.GetCol(), s_strTypeTemplate))
		type = ibSpreadsheetFillType::ibSpreadsheetFillType_StrTemplate;
	else if (m_table->CanGetValueAs(event.GetRow(), event.GetCol(), s_strTypeParameter))
		type = ibSpreadsheetFillType::ibSpreadsheetFillType_StrParameter;

	wxSharedPtr<wxString> value;

	if (type == ibSpreadsheetFillType::ibSpreadsheetFillType_StrText) {
		value = static_cast<wxString*>(
			m_table->GetValueAsCustom(event.GetRow(), event.GetCol(), s_strTypeTextOrString));
	}
	else if (type == ibSpreadsheetFillType::ibSpreadsheetFillType_StrTemplate) {
		value = static_cast<wxString*>(
			m_table->GetValueAsCustom(event.GetRow(), event.GetCol(), s_strTypeTemplate));
	}
	else if (type == ibSpreadsheetFillType::ibSpreadsheetFillType_StrParameter) {
		value = static_cast<wxString*>(
			m_table->GetValueAsCustom(event.GetRow(), event.GetCol(), s_strTypeParameter));
	}

	if (m_document != nullptr) {

		ibValueMetaObjectSpreadsheetBase* creator = m_document->ConvertMetaObjectToType<ibValueMetaObjectSpreadsheetBase>();

		if (creator != nullptr) {
			ibSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();
			spreadsheetDescription.SetCellFillType(event.GetRow(), event.GetCol(), type);
			spreadsheetDescription.SetCellValue(event.GetRow(), event.GetCol(), *value);
		}

		m_document->Modify(true);
	}

	if (m_spreadsheetObject != nullptr) {
		ibSpreadsheetDescription& spreadsheetDescription = m_spreadsheetObject->GetSpreadsheetDesc();
		spreadsheetDescription.SetCellFillType(event.GetRow(), event.GetCol(), type);
		spreadsheetDescription.SetCellValue(event.GetRow(), event.GetCol(), *value);
	}

	event.Skip();
}

void ibGridEditor::OnGridTableAttrModified(ibGridEvent& event)
{
	int horiz, vert;
	GetCellAlignment(event.GetRow(), event.GetCol(), &horiz, &vert);

	int num_rows, num_cols;
	GetCellSize(event.GetRow(), event.GetCol(), &num_rows, &num_cols);

	if (m_document != nullptr) {

		ibValueMetaObjectSpreadsheetBase* creator = m_document->ConvertMetaObjectToType<ibValueMetaObjectSpreadsheetBase>();

		if (creator != nullptr) {

			ibSpreadsheetDescription& spreadsheetDescription = creator->GetSpreadsheetDesc();

			spreadsheetDescription.SetCellBackgroundColour(event.GetRow(), event.GetCol(), GetCellBackgroundColour(event.GetRow(), event.GetCol()));
			spreadsheetDescription.SetCellTextColour(event.GetRow(), event.GetCol(), GetCellTextColour(event.GetRow(), event.GetCol()));
			spreadsheetDescription.SetCellTextOrient(event.GetRow(), event.GetCol(), GetCellTextOrient(event.GetRow(), event.GetCol()));
			spreadsheetDescription.SetCellFont(event.GetRow(), event.GetCol(), GetCellFont(event.GetRow(), event.GetCol()));
			spreadsheetDescription.SetCellAlignment(event.GetRow(), event.GetCol(), horiz, vert);

			//border 
			const ibGridCellBorder& borderLeft = GetCellBorderLeft(event.GetRow(), event.GetCol());
			spreadsheetDescription.SetCellBorderLeft(event.GetRow(), event.GetCol(), { borderLeft.m_style, borderLeft.m_colour, borderLeft.m_width });
			const ibGridCellBorder& borderRight = GetCellBorderLeft(event.GetRow(), event.GetCol());
			spreadsheetDescription.SetCellBorderRight(event.GetRow(), event.GetCol(), { borderRight.m_style, borderRight.m_colour, borderRight.m_width });
			const ibGridCellBorder& borderTop = GetCellBorderLeft(event.GetRow(), event.GetCol());
			spreadsheetDescription.SetCellBorderTop(event.GetRow(), event.GetCol(), { borderTop.m_style, borderTop.m_colour, borderTop.m_width });
			const ibGridCellBorder& borderBottom = GetCellBorderLeft(event.GetRow(), event.GetCol());
			spreadsheetDescription.SetCellBorderBottom(event.GetRow(), event.GetCol(), { borderBottom.m_style, borderBottom.m_colour, borderBottom.m_width });

			//size 
			spreadsheetDescription.SetCellSize(event.GetRow(), event.GetCol(), num_rows, num_cols);

			//cell
			ibGridFitMode fitMode =
				GetCellFitMode(event.GetRow(), event.GetCol());

			spreadsheetDescription.SetCellFitMode(event.GetRow(), event.GetCol(), fitMode.IsOverflow() ? ibSpreadsheetCellDescription::ibFitMode::Mode_Overflow : ibSpreadsheetCellDescription::ibFitMode::Mode_Clip);
			spreadsheetDescription.SetCellReadOnly(event.GetRow(), event.GetCol(), IsCellReadOnly(event.GetRow(), event.GetCol()));
		}

		m_document->Modify(true);
	}

	if (m_spreadsheetObject != nullptr) {

		ibSpreadsheetDescription& spreadsheetDescription = m_spreadsheetObject->GetSpreadsheetDesc();

		spreadsheetDescription.SetCellBackgroundColour(event.GetRow(), event.GetCol(), GetCellBackgroundColour(event.GetRow(), event.GetCol()));
		spreadsheetDescription.SetCellTextColour(event.GetRow(), event.GetCol(), GetCellTextColour(event.GetRow(), event.GetCol()));
		spreadsheetDescription.SetCellTextOrient(event.GetRow(), event.GetCol(), GetCellTextOrient(event.GetRow(), event.GetCol()));
		spreadsheetDescription.SetCellFont(event.GetRow(), event.GetCol(), GetCellFont(event.GetRow(), event.GetCol()));
		spreadsheetDescription.SetCellAlignment(event.GetRow(), event.GetCol(), horiz, vert);

		//border 
		const ibGridCellBorder& borderLeft = GetCellBorderLeft(event.GetRow(), event.GetCol());
		spreadsheetDescription.SetCellBorderLeft(event.GetRow(), event.GetCol(), { borderLeft.m_style, borderLeft.m_colour, borderLeft.m_width });
		const ibGridCellBorder& borderRight = GetCellBorderLeft(event.GetRow(), event.GetCol());
		spreadsheetDescription.SetCellBorderRight(event.GetRow(), event.GetCol(), { borderRight.m_style, borderRight.m_colour, borderRight.m_width });
		const ibGridCellBorder& borderTop = GetCellBorderLeft(event.GetRow(), event.GetCol());
		spreadsheetDescription.SetCellBorderTop(event.GetRow(), event.GetCol(), { borderTop.m_style, borderTop.m_colour, borderTop.m_width });
		const ibGridCellBorder& borderBottom = GetCellBorderLeft(event.GetRow(), event.GetCol());
		spreadsheetDescription.SetCellBorderBottom(event.GetRow(), event.GetCol(), { borderBottom.m_style, borderBottom.m_colour, borderBottom.m_width });

		//size 
		spreadsheetDescription.SetCellSize(event.GetRow(), event.GetCol(), num_rows, num_cols);

		//cell
		ibGridFitMode fitMode =
			GetCellFitMode(event.GetRow(), event.GetCol());

		spreadsheetDescription.SetCellFitMode(event.GetRow(), event.GetCol(), fitMode.IsOverflow() ? ibSpreadsheetCellDescription::ibFitMode::Mode_Overflow : ibSpreadsheetCellDescription::ibFitMode::Mode_Clip);
		spreadsheetDescription.SetCellReadOnly(event.GetRow(), event.GetCol(), IsCellReadOnly(event.GetRow(), event.GetCol()));
	}

	event.Skip();
}

void ibGridEditor::OnScroll(wxScrollWinEvent& event)
{
	int ux, uy,
		sx, sy;

	ibGrid::GetScrollPixelsPerUnit(&ux, &uy);
	ibGrid::GetViewStart(&sx, &sy);

	sx *= ux; sy *= uy;

	int w, h;
	m_gridWin->GetClientSize(&w, &h);

	int scroll = 0, page_scroll_x = 0, page_scroll_y = 0;

	if (event.GetEventType() == wxEVT_SCROLLWIN_THUMBTRACK ||
		event.GetEventType() == wxEVT_SCROLLWIN_THUMBRELEASE) {
		const int position = ibGrid::GetScrollPos(event.GetOrientation());
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
			page_scroll_y = -ibGrid::GetScrollPageSize(event.GetOrientation()) * uy;
			scroll = page_scroll_y;
		}
		else if (event.GetOrientation() == wxOrientation::wxHORIZONTAL) {
			page_scroll_x = -ibGrid::GetScrollPageSize(event.GetOrientation()) * ux;
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
			page_scroll_y = ibGrid::GetScrollPageSize(event.GetOrientation()) * uy;
			scroll = page_scroll_y;
		}
		else if (event.GetOrientation() == wxOrientation::wxHORIZONTAL) {
			page_scroll_x = ibGrid::GetScrollPageSize(event.GetOrientation()) * ux;
			scroll = page_scroll_x;
		}
	}

	const int x0 = ibGrid::XToCol(sx, true);
	const int y0 = ibGrid::YToRow(sy, true);

	const int x1 = ibGrid::XToCol(sx + w, true);
	const int y1 = ibGrid::YToRow(sy + h, true);

	if (scroll > 0 && event.GetOrientation() == wxOrientation::wxVERTICAL) {
		if (y1 >= ibGrid::GetNumberRows() - ibGrid::GetNumberFrozenRows() - 1) {
			ibGrid::AppendRows();
			ibGrid::SetScrollPos(wxOrientation::wxVERTICAL, (sy + scroll) / uy);
		}
	}
	else if (scroll > 0 && event.GetOrientation() == wxOrientation::wxHORIZONTAL) {
		if (x1 >= ibGrid::GetNumberCols() - ibGrid::GetNumberFrozenCols() - 1) {
			ibGrid::AppendCols();
			ibGrid::SetScrollPos(wxOrientation::wxHORIZONTAL, (sx + scroll) / ux);
		}
	}
	else if (scroll < 0 && event.GetOrientation() == wxOrientation::wxVERTICAL) {
		const int y = ibGrid::YToRow(sy + h + scroll, true) + 1;
		if (y0 > GetMaxRowBrake() && y0 != 0) {
			for (int row = ibGrid::GetNumberRows(); row > y; row--)
				ibGrid::DeleteRows(row - 1);
			ibGrid::SetScrollPos(wxOrientation::wxVERTICAL, (sy + scroll) / uy);
		}
	}
	else if (scroll < 0 && event.GetOrientation() == wxOrientation::wxHORIZONTAL) {
		const int x = ibGrid::XToCol(sx + w + scroll, true);
		if (x0 > GetMaxColBrake() && x0 != 0) {
			for (int col = ibGrid::GetNumberCols(); col > x; col--)
				ibGrid::DeleteCols(col - 1);
			ibGrid::SetScrollPos(wxOrientation::wxHORIZONTAL, (sx + scroll) / ux);
		}
	}

	event.Skip();
}

void ibGridEditor::OnIdle(wxIdleEvent& event)
{
	event.Skip();
}

void ibGridEditor::OnSize(wxSizeEvent& event)
{
	int ux, uy,
		sx, sy;

	ibGrid::GetScrollPixelsPerUnit(&ux, &uy);
	ibGrid::GetViewStart(&sx, &sy);

	sx *= ux; sy *= uy;

	const int w = event.m_size.x;
	const int h = event.m_size.y;

	int x0 = ibGrid::XToCol(sx);
	int y0 = ibGrid::YToRow(sy);
	int x1 = ibGrid::XToCol(sx + w);
	int y1 = ibGrid::YToRow(sy + h);

	if (m_table != nullptr && x0 == wxNOT_FOUND) {
		const int col = w / ibCalcGridScale(m_defaultColWidth, GetGridZoom());
		if ((col + 1) > m_table->GetColsCount())
			m_table->AppendCols(col + 1);
	}
	else if (m_table != nullptr && x1 == wxNOT_FOUND) {
		int col_size = 0;
		for (int col = x0; col < m_table->GetColsCount(); col++)
			col_size += ibGrid::GetColWidth(col, GetGridZoom());
		if (w > col_size) {
			const int col = ((w - col_size) / ibCalcGridScale(m_defaultColWidth, GetGridZoom()));
			m_table->AppendCols(col + 1);
		}
	}

	if (m_table != nullptr && y0 == wxNOT_FOUND) {
		const int row = h / ibCalcGridScale(m_defaultRowHeight, GetGridZoom());
		if ((row + 1) > m_table->GetRowsCount())
			m_table->AppendRows(row + 1);
	}
	else if (m_table != nullptr && y1 == wxNOT_FOUND) {
		int row_size = 0;
		for (int row = y0; row < m_table->GetRowsCount(); row++)
			row_size += ibGrid::GetRowHeight(row, GetGridZoom());
		if (h > row_size) {
			const int row = ((h - row_size) / ibCalcGridScale(m_defaultRowHeight, GetGridZoom()));
			m_table->AppendRows(row + 1);
		}
	}

	event.Skip();
}

void ibGridEditor::OnGridZoom(ibGridEvent& event)
{
	int ux, uy,
		sx, sy;

	ibGrid::GetScrollPixelsPerUnit(&ux, &uy);
	ibGrid::GetViewStart(&sx, &sy);

	sx *= ux; sy *= uy;

	int w, h;
	ibGrid::GetSize(&w, &h);

	int x0 = ibGrid::XToCol(sx);
	int y0 = ibGrid::YToRow(sy);
	int x1 = ibGrid::XToCol(sx + w);
	int y1 = ibGrid::YToRow(sy + h);

	if (m_table != nullptr && x0 == wxNOT_FOUND) {
		const int col = w / ibCalcGridScale(m_defaultColWidth, GetGridZoom());
		if ((col + 1) > m_table->GetColsCount())
			m_table->AppendCols(col + 1);
	}
	else if (m_table != nullptr && x1 == wxNOT_FOUND) {
		int col_size = 0;
		for (int col = x0; col < m_table->GetColsCount(); col++)
			col_size += ibGrid::GetColWidth(col, GetGridZoom());
		if (w > col_size) {
			const int col = ((w - col_size) / ibCalcGridScale(m_defaultColWidth, GetGridZoom()));
			m_table->AppendCols(col + 1);
		}
	}

	if (m_table != nullptr && y0 == wxNOT_FOUND) {
		const int row = h / ibCalcGridScale(m_defaultRowHeight, GetGridZoom());
		if ((row + 1) > m_table->GetRowsCount())
			m_table->AppendRows(row + 1);
	}
	else if (m_table != nullptr && y1 == wxNOT_FOUND) {
		int row_size = 0;
		for (int row = y0; row < m_table->GetRowsCount(); row++)
			row_size += ibGrid::GetRowHeight(row, GetGridZoom());
		if (h > row_size) {
			const int row = ((h - row_size) / ibCalcGridScale(m_defaultRowHeight, GetGridZoom()));
			m_table->AppendRows(row + 1);
		}
	}

	event.Skip();
}

#pragma endregion

#pragma region context_menu
void ibGridEditor::OnCopy(wxCommandEvent& event)
{
	Copy();
}

void ibGridEditor::OnPaste(wxCommandEvent& event)
{
	Paste();
}

void ibGridEditor::OnDelete(wxCommandEvent& event)
{
	for (auto cell : ibGrid::GetSelectedBlocks()) {
		for (int col = cell.GetLeftCol(); col <= cell.GetRightCol(); col++) {
			for (int row = cell.GetTopRow(); row <= cell.GetBottomRow(); row++) {

				SetAttr(row, col, nullptr);
				SetCellValue(row, col, wxEmptyString);
			}
		}

		DeleteArea();

		if (cell.GetLeftCol() <= GetMaxColBrake() && cell.GetRightCol() >= GetMaxColBrake() && cell.GetLeftCol() > 0) {
			for (int col = cell.GetRightCol(); col >= cell.GetLeftCol(); col--) {
				ibGrid::SetColSize(col, WXGRID_DEFAULT_ROW_LABEL_WIDTH - 12);
			}
			m_colBrakeAt.Last() = cell.GetLeftCol() - 1;
		}

		if (cell.GetTopRow() <= GetMaxRowBrake() && cell.GetBottomRow() >= GetMaxRowBrake() && cell.GetTopRow() > 0) {
			for (int row = cell.GetBottomRow(); row >= cell.GetTopRow(); row--) {
				ibGrid::SetRowSize(row, WXGRID_MIN_ROW_HEIGHT);
			}
			m_rowBrakeAt.Last() = cell.GetTopRow() - 1;
		}

		if (!cell.GetLeftCol() &&
			!cell.GetTopRow()) {
			for (int col = cell.GetRightCol(); col >= 0; col--)
				ibGrid::SetColSize(col, WXGRID_DEFAULT_ROW_LABEL_WIDTH - 12);
			for (int row = cell.GetBottomRow(); row >= 0; row--)
				ibGrid::SetRowSize(row, WXGRID_MIN_ROW_HEIGHT);
			m_colBrakeAt.clear(); m_rowBrakeAt.clear();
		}
	}
}

#include "frontend/win/dlgs/rowHeight.h"

void ibGridEditor::OnRowHeight(wxCommandEvent& event)
{
	std::shared_ptr <ibDialogRowHeight> rowHeight(new ibDialogRowHeight(this));
	const int result = rowHeight->ShowModal();
	if (result == wxID_OK) {
		for (auto cell : ibGrid::GetSelectedBlocks()) {
			for (int row = cell.GetTopRow(); row <= cell.GetBottomRow(); row++) {
				const int size = ibGrid::GetRowSize(row);
				ibGrid::SetRowSize(row, rowHeight->GetHeight());
			}
		}
	}
}

#include "frontend/win/dlgs/colHeight.h"

void ibGridEditor::OnColWidth(wxCommandEvent& event)
{
	std::shared_ptr<ibDialogColWidth> colWidth(new ibDialogColWidth(this));
	const int result = colWidth->ShowModal();
	if (result == wxID_OK) {
		for (auto cell : ibGrid::GetSelectedBlocks()) {
			for (int col = cell.GetLeftCol(); col <= cell.GetRightCol(); col++) {
				const int size = ibGrid::GetColSize(col);
				ibGrid::SetColSize(col, colWidth->GetWidth());
			}
		}
	}
}

void ibGridEditor::OnHideCell(wxCommandEvent& event)
{
	for (auto cell : ibGrid::GetSelectedBlocks()) {
		if (cell.GetLeftCol() > 0 &&
			cell.GetTopRow() == 0) {
			for (int col = cell.GetLeftCol(); col <= cell.GetRightCol(); col++) {
				const int size = ibGrid::GetColSize(col);
				ibGrid::SetColSize(col, 0);
			}
		}
		else if (cell.GetTopRow() > 0 &&
			cell.GetLeftCol() == 0) {
			for (int row = cell.GetTopRow(); row <= cell.GetBottomRow(); row++) {
				const int size = ibGrid::GetRowSize(row);
				ibGrid::SetRowSize(row, 0);
			}
		}
		else if (cell.GetLeftCol() == 0 &&
			cell.GetTopRow() == 0) {
			if (cell.GetBottomRow() == m_numRows - 1) {
				for (int col = cell.GetLeftCol(); col <= cell.GetRightCol(); col++) {
					const int size = ibGrid::GetColSize(col);
					ibGrid::SetColSize(col, 0);
				}
			}
			else if (cell.GetRightCol() == m_numCols - 1) {
				for (int row = cell.GetTopRow(); row <= cell.GetBottomRow(); row++) {
					const int size = ibGrid::GetRowSize(row);
					ibGrid::SetRowSize(row, 0);
				}
			}
		}
	}
}

void ibGridEditor::OnShowCell(wxCommandEvent& event)
{
	for (auto cell : ibGrid::GetSelectedBlocks()) {
		if (cell.GetLeftCol() > 0 &&
			cell.GetTopRow() == 0) {
			bool hiddenCol = false;
			for (int col = 0; col < cell.GetLeftCol(); col++) {
				int size = ibGrid::GetColSize(col);
				hiddenCol = size == 0;
			}
			for (int col = hiddenCol ? 0 : cell.GetLeftCol(); col <= cell.GetRightCol(); col++) {
				const int size = ibGrid::GetColSize(col);
				ibGrid::SetColSize(col, wxNOT_FOUND);
			}
		}
		else if (cell.GetTopRow() > 0 &&
			cell.GetLeftCol() == 0) {
			bool hiddenRow = false;
			for (int row = 0; row < cell.GetTopRow(); row++) {
				int size = ibGrid::GetRowSize(row);
				hiddenRow = size == 0;
			}
			for (int row = hiddenRow ? 0 : cell.GetTopRow(); row <= cell.GetBottomRow(); row++) {
				const int size = ibGrid::GetRowSize(row);
				ibGrid::SetRowSize(row, wxNOT_FOUND);
			}
		}
		else if (cell.GetLeftCol() == 0 &&
			cell.GetTopRow() == 0) {
			if (cell.GetBottomRow() == m_numRows - 1) {
				for (int col = cell.GetLeftCol(); col <= cell.GetRightCol(); col++) {
					const int size = ibGrid::GetColSize(col);
					ibGrid::SetColSize(col, wxNOT_FOUND);
				}
			}
			else if (cell.GetRightCol() == m_numCols - 1) {
				for (int row = cell.GetTopRow(); row <= cell.GetBottomRow(); row++) {
					const int size = ibGrid::GetRowSize(row);
					ibGrid::SetRowSize(row, wxNOT_FOUND);
				}
			}
		}
	}
}

void ibGridEditor::OnProperties(wxCommandEvent& event)
{
	m_propertySpreadsheet->ShowInspector();
}
#pragma endregion  