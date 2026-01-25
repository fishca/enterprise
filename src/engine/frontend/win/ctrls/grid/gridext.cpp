///////////////////////////////////////////////////////////////////////////
// Name:        src/generic/grid.cpp
// Purpose:     wxGridExt and related classes
// Author:      Michael Bedward (based on code by Julian Smart, Robin Dunn)
// Modified by: Robin Dunn, Vadim Zeitlin, Santiago Palacios
// Created:     1/08/1999
// Copyright:   (c) Michael Bedward (mbedward@ozemail.com.au)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

/*
	TODO:

	- Make Begin/EndBatch() the same as the generic Freeze/Thaw()
	- Review the column reordering code, it's a mess.
 */

 // For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#if wxUSE_GRID

#include "gridext.h"

#ifndef WX_PRECOMP
#include "wx/utils.h"
#include "wx/dcclient.h"
#include "wx/settings.h"
#include "wx/log.h"
#include "wx/textctrl.h"
#include "wx/checkbox.h"
#include "wx/combobox.h"
#include "wx/valtext.h"
#include "wx/intl.h"
#include "wx/math.h"
#include "wx/listbox.h"
#endif

#include "wx/dcbuffer.h"
#include "wx/textfile.h"
#include "wx/spinctrl.h"
#include "wx/tokenzr.h"
#include "wx/renderer.h"
#include "wx/headerctrl.h"
#include "wx/hashset.h"

#if wxUSE_CLIPBOARD
#include "wx/clipbrd.h"
#endif // wxUSE_CLIPBOARD

#include "gridextsel.h"
#include "gridextctrl.h"
#include "gridexteditors.h"
#include "gridextprivate.h"

#include "wx/arrimpl.cpp"

// Required for wxIs... functions
#include <ctype.h>

const char wxGridExtNameStr[] = "gridext";

WX_DECLARE_HASH_SET_WITH_DECL_PTR(int, wxIntegerHash, wxIntegerEqual,
	wxGridExtFixedIndicesSet, class);

// ----------------------------------------------------------------------------
// globals
// ----------------------------------------------------------------------------

namespace
{

	//#define DEBUG_ATTR_CACHE
#ifdef DEBUG_ATTR_CACHE
	static size_t gs_nAttrCacheHits = 0;
	static size_t gs_nAttrCacheMisses = 0;
#endif

	// this struct simply combines together the default header renderers
	//
	// as the renderers ctors are trivial, there is no problem with making them
	// globals
	struct DefaultHeaderRenderers
	{
		wxGridExtColumnHeaderRendererDefault colRenderer;
		wxGridExtRowHeaderRendererDefault rowRenderer;
		wxGridExtCornerHeaderRendererDefault cornerRenderer;
	} gs_defaultHeaderRenderers;

} // anonymous namespace

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

const wxArrayString wxEmptyArrayGridString;

wxGridExtCellCoords wxGridExtNoCellCoords(-1, -1);
wxGridExtBlockCoords wxGridExtNoBlockCoords(-1, -1, -1, -1);
wxRect wxGridExtNoCellRect(-1, -1, -1, -1);

namespace
{

	// scroll line size
	const size_t GRID_SCROLL_LINE_X = 15;
	const size_t GRID_SCROLL_LINE_Y = GRID_SCROLL_LINE_X;

	// the size of hash tables used a bit everywhere (the max number of elements
	// in these hash tables is the number of rows/columns)
	const int GRID_HASH_SIZE = 100;

	// the minimal distance in pixels the mouse needs to move to start a drag
	// operation
	const int DRAG_SENSITIVITY = 3;

	// the space between the cell edge and the checkbox mark
	const int GRID_CELL_CHECKBOX_MARGIN = 2;

	// the margin between a cell vertical line and a cell text
	const int GRID_TEXT_MARGIN = 1;

} // anonymous namespace


WX_DEFINE_OBJARRAY(wxGridExtCellCoordsArray)
WX_DEFINE_OBJARRAY(wxGridExtCellAreaArray)

// ----------------------------------------------------------------------------
// events
// ----------------------------------------------------------------------------

wxDEFINE_EVENT(wxEVT_GRID_CELL_LEFT_CLICK, wxGridExtEvent);
wxDEFINE_EVENT(wxEVT_GRID_CELL_RIGHT_CLICK, wxGridExtEvent);
wxDEFINE_EVENT(wxEVT_GRID_CELL_LEFT_DCLICK, wxGridExtEvent);
wxDEFINE_EVENT(wxEVT_GRID_CELL_RIGHT_DCLICK, wxGridExtEvent);
wxDEFINE_EVENT(wxEVT_GRID_CELL_BEGIN_DRAG, wxGridExtEvent);
wxDEFINE_EVENT(wxEVT_GRID_LABEL_LEFT_CLICK, wxGridExtEvent);
wxDEFINE_EVENT(wxEVT_GRID_LABEL_RIGHT_CLICK, wxGridExtEvent);
wxDEFINE_EVENT(wxEVT_GRID_LABEL_LEFT_DCLICK, wxGridExtEvent);
wxDEFINE_EVENT(wxEVT_GRID_LABEL_RIGHT_DCLICK, wxGridExtEvent);
wxDEFINE_EVENT(wxEVT_GRID_ROW_SIZE, wxGridExtSizeEvent);
wxDEFINE_EVENT(wxEVT_GRID_ROW_AUTO_SIZE, wxGridExtSizeEvent);
wxDEFINE_EVENT(wxEVT_GRID_COL_SIZE, wxGridExtSizeEvent);
wxDEFINE_EVENT(wxEVT_GRID_COL_AUTO_SIZE, wxGridExtSizeEvent);
wxDEFINE_EVENT(wxEVT_GRID_ROW_MOVE, wxGridExtEvent);
wxDEFINE_EVENT(wxEVT_GRID_COL_MOVE, wxGridExtEvent);
wxDEFINE_EVENT(wxEVT_GRID_COL_SORT, wxGridExtEvent);
wxDEFINE_EVENT(wxEVT_GRID_RANGE_SELECTING, wxGridExtRangeSelectEvent);
wxDEFINE_EVENT(wxEVT_GRID_RANGE_SELECTED, wxGridExtRangeSelectEvent);
wxDEFINE_EVENT(wxEVT_GRID_CELL_CHANGING, wxGridExtEvent);
wxDEFINE_EVENT(wxEVT_GRID_CELL_CHANGED, wxGridExtEvent);
wxDEFINE_EVENT(wxEVT_GRID_SELECT_CELL, wxGridExtEvent);
wxDEFINE_EVENT(wxEVT_GRID_EDITOR_SHOWN, wxGridExtEvent);
wxDEFINE_EVENT(wxEVT_GRID_EDITOR_HIDDEN, wxGridExtEvent);
wxDEFINE_EVENT(wxEVT_GRID_EDITOR_CREATED, wxGridExtEditorCreatedEvent);
wxDEFINE_EVENT(wxEVT_GRID_CHANGED, wxGridExtEvent);
wxDEFINE_EVENT(wxEVT_GRID_TABBING, wxGridExtEvent);

// ============================================================================
// implementation
// ============================================================================

namespace
{

	// Helper function for consistent cell span determination based on cell size.
	wxGridExt::CellSpan GetCellSpan(int numRows, int numCols)
	{
		if (numRows == 1 && numCols == 1)
			return wxGridExt::CellSpan_None; // just a normal cell

		if (numRows < 0 || numCols < 0)
			return wxGridExt::CellSpan_Inside; // covered by a multi-span cell

		// this cell spans multiple cells to its right/bottom
		return wxGridExt::CellSpan_Main;
	}

} // anonymous namespace

wxIMPLEMENT_ABSTRACT_CLASS(wxGridExtCellEditorEvtHandler, wxEvtHandler);

wxBEGIN_EVENT_TABLE(wxGridExtCellEditorEvtHandler, wxEvtHandler)
EVT_KILL_FOCUS(wxGridExtCellEditorEvtHandler::OnKillFocus)
EVT_KEY_DOWN(wxGridExtCellEditorEvtHandler::OnKeyDown)
EVT_CHAR(wxGridExtCellEditorEvtHandler::OnChar)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(wxGridExtHeaderCtrl, wxHeaderCtrl)
EVT_HEADER_CLICK(wxID_ANY, wxGridExtHeaderCtrl::OnClick)
EVT_HEADER_DCLICK(wxID_ANY, wxGridExtHeaderCtrl::OnDoubleClick)
EVT_HEADER_RIGHT_CLICK(wxID_ANY, wxGridExtHeaderCtrl::OnRightClick)

EVT_HEADER_BEGIN_RESIZE(wxID_ANY, wxGridExtHeaderCtrl::OnBeginResize)
EVT_HEADER_RESIZING(wxID_ANY, wxGridExtHeaderCtrl::OnResizing)
EVT_HEADER_END_RESIZE(wxID_ANY, wxGridExtHeaderCtrl::OnEndResize)

EVT_HEADER_BEGIN_REORDER(wxID_ANY, wxGridExtHeaderCtrl::OnBeginReorder)
EVT_HEADER_END_REORDER(wxID_ANY, wxGridExtHeaderCtrl::OnEndReorder)
wxEND_EVENT_TABLE()

wxGridExtOperations& wxGridExtRowOperations::Dual() const
{
	static wxGridExtColumnOperations s_colOper;

	return s_colOper;
}

wxGridExtOperations& wxGridExtColumnOperations::Dual() const
{
	static wxGridExtRowOperations s_rowOper;

	return s_rowOper;
}

int wxGridExtRowOperations::GetNumberOfLines(const wxGridExt* grid, wxGridExtWindow* gridWindow) const
{
	if (!gridWindow)
		return grid->GetNumberRows();

	if (gridWindow->GetType() & wxGridExtWindow::wxGridExtWindowFrozenRow)
		return grid->GetNumberFrozenRows();

	return grid->GetNumberRows() - grid->GetNumberFrozenRows();
}

int wxGridExtColumnOperations::GetNumberOfLines(const wxGridExt* grid, wxGridExtWindow* gridWindow) const
{
	if (!gridWindow)
		return grid->GetNumberCols();

	if (gridWindow->GetType() & wxGridExtWindow::wxGridExtWindowFrozenCol)
		return grid->GetNumberFrozenCols();

	return grid->GetNumberCols() - grid->GetNumberFrozenCols();
}

int wxGridExtRowOperations::GetFirstLine(const wxGridExt* grid, wxGridExtWindow* gridWindow) const
{
	if (!gridWindow || gridWindow->GetType() & wxGridExtWindow::wxGridExtWindowFrozenRow)
		return 0;

	return grid->GetNumberFrozenRows();
}

int wxGridExtColumnOperations::GetFirstLine(const wxGridExt* grid, wxGridExtWindow* gridWindow) const
{
	if (!gridWindow || gridWindow->GetType() & wxGridExtWindow::wxGridExtWindowFrozenCol)
		return 0;

	return grid->GetNumberFrozenCols();
}

void wxGridExtOperations::PrepareDCForLabels(wxGridExt* grid, wxDC& dc) const
{
	// The grid can be scrolled in both directions and so grid->DoPrepareDC
	// could offset the device context in both directions.
	// For row and col labels, we want only one direction and so
	// we reset the other direction to the original, unscrolled value.
	wxPoint dcOriginBefore = dc.GetDeviceOrigin();
	grid->DoPrepareDC(dc);
	wxPoint dcOrigin = dc.GetDeviceOrigin();

	if (GetOrientation() == wxVERTICAL)
		dcOrigin.x = dcOriginBefore.x;
	else if (GetOrientation() == wxHORIZONTAL)
		dcOrigin.y = dcOriginBefore.y;

	dc.SetDeviceOrigin(dcOrigin.x, dcOrigin.y);
}

// ----------------------------------------------------------------------------
// wxGridExtCellWorker is an (almost) empty common base class for
// wxGridExtCellRenderer and wxGridExtCellEditor managing ref counting
// ----------------------------------------------------------------------------

wxGridExtCellWorker::wxGridExtCellWorker(const wxGridExtCellWorker& other)
	: wxRefCounter()
{
	CopyClientDataContainer(other);
}

void wxGridExtCellWorker::SetParameters(const wxString& WXUNUSED(params))
{
	// nothing to do
}

wxGridExtCellWorker::~wxGridExtCellWorker()
{
}

// ----------------------------------------------------------------------------
// wxGridExtHeaderLabelsRenderer and related classes
// ----------------------------------------------------------------------------

void wxGridExtHeaderLabelsRenderer::DrawLabel(const wxGridExt& grid,
	wxDC& dc,
	const wxString& value,
	const wxRect& rect,
	int horizAlign,
	int vertAlign,
	int textOrientation) const
{
	dc.SetBackgroundMode(wxBRUSHSTYLE_TRANSPARENT);
	dc.SetFont(grid.GetLabelFont());

	// Draw text in a different colour and with a shadow when the control
	// is disabled.
	//
	// Note that the colours used here are consistent with wxGenericStaticText
	// rather than our own wxGridExtCellStringRenderer::SetTextColoursAndFont()
	// because this results in a better disabled appearance for the default
	// bold font used for the labels.
	wxColour colText;
	if (!grid.IsEnabled())
	{
		colText = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT);
		dc.SetTextForeground(colText);

		wxRect rectShadow = rect;
		rectShadow.Offset(1, 1);
		grid.DrawTextRectangle(dc, value, rectShadow,
			horizAlign, vertAlign, textOrientation);

		colText = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
	}
	else
	{
		colText = grid.GetLabelTextColour();
	}

	dc.SetTextForeground(colText);

	grid.DrawTextRectangle(dc, value, rect, horizAlign, vertAlign, textOrientation);
}


void wxGridExtRowHeaderRendererDefault::DrawBorder(const wxGridExt& grid,
	wxDC& dc,
	wxRect& rect) const
{
	dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW)));
	dc.DrawLine(rect.GetRight(), rect.GetTop(),
		rect.GetRight(), rect.GetBottom());

	dc.DrawLine(rect.GetLeft(), rect.GetBottom(),
		rect.GetRight() + 1, rect.GetBottom());

	// Only draw the external borders when the containing control doesn't have
	// any border, otherwise they would compound with the outer border which
	// looks bad.
	int ofs = 0;
	if (grid.GetBorder() == wxBORDER_NONE)
	{
		dc.DrawLine(rect.GetLeft(), rect.GetTop(),
			rect.GetLeft(), rect.GetBottom());

		ofs = 1;
	}

	dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DLIGHT)));
	dc.DrawLine(rect.GetLeft() + ofs, rect.GetTop(),
		rect.GetLeft() + ofs, rect.GetBottom());
	dc.DrawLine(rect.GetLeft() + ofs, rect.GetTop(),
		rect.GetRight(), rect.GetTop());

	rect.Deflate(1 + ofs);
}

void wxGridExtColumnHeaderRendererDefault::DrawBorder(const wxGridExt& grid,
	wxDC& dc,
	wxRect& rect) const
{
	dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW)));
	dc.DrawLine(rect.GetRight(), rect.GetTop(),
		rect.GetRight(), rect.GetBottom());
	dc.DrawLine(rect.GetLeft(), rect.GetBottom(),
		rect.GetRight() + 1, rect.GetBottom());

	// As above, don't draw the outer border if the control has its own one.
	int ofs = 0;
	if (grid.GetBorder() == wxBORDER_NONE)
	{
		dc.DrawLine(rect.GetLeft(), rect.GetTop(),
			rect.GetRight(), rect.GetTop());

		ofs = 1;
	}

	dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DLIGHT)));
	dc.DrawLine(rect.GetLeft(), rect.GetTop() + ofs,
		rect.GetLeft(), rect.GetBottom());
	dc.DrawLine(rect.GetLeft(), rect.GetTop() + ofs,
		rect.GetRight(), rect.GetTop() + ofs);

	rect.Deflate(1 + ofs);
}

void wxGridExtCornerHeaderRendererDefault::DrawBorder(const wxGridExt& grid,
	wxDC& dc,
	wxRect& rect) const
{
	dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW)));
	dc.DrawLine(rect.GetRight() - 1, rect.GetBottom() - 1,
		rect.GetRight() - 1, rect.GetTop());
	dc.DrawLine(rect.GetRight() - 1, rect.GetBottom() - 1,
		rect.GetLeft(), rect.GetBottom() - 1);

	// As above, don't draw either of outer border if there is already a border
	// around the entire window.
	int ofs = 0;
	if (grid.GetBorder() == wxBORDER_NONE)
	{
		dc.DrawLine(rect.GetLeft(), rect.GetTop(),
			rect.GetRight(), rect.GetTop());
		dc.DrawLine(rect.GetLeft(), rect.GetTop(),
			rect.GetLeft(), rect.GetBottom());

		ofs = 1;
	}

	dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DLIGHT)));
	dc.DrawLine(rect.GetLeft() + 1, rect.GetTop() + ofs,
		rect.GetRight() - 1, rect.GetTop() + ofs);
	dc.DrawLine(rect.GetLeft() + ofs, rect.GetTop() + ofs,
		rect.GetLeft() + ofs, rect.GetBottom() - 1);

	rect.Deflate(1 + ofs);
}

// ----------------------------------------------------------------------------
// wxGridExtCellAttr
// ----------------------------------------------------------------------------

void wxGridExtCellAttr::Init(wxGridExtCellAttr* attrDefault)
{
	m_isReadOnly = Unset;

	m_renderer = NULL;
	m_editor = NULL;

	m_attrkind = wxGridExtCellAttr::Cell;

	m_orientText = -1;
	m_sizeRows = m_sizeCols = 1;

	SetDefAttr(attrDefault);
}

wxGridExtCellAttr* wxGridExtCellAttr::Clone() const
{
	wxGridExtCellAttr* attr = new wxGridExtCellAttr(m_defGridAttr);

	if (HasTextColour())
		attr->SetTextColour(GetTextColour());
	if (HasTextOrient())
		attr->SetTextOrient(GetTextOrient());
	if (HasBackgroundColour())
		attr->SetBackgroundColour(GetBackgroundColour());
	if (HasFont())
		attr->SetFont(GetFont());
	if (HasAlignment())
		attr->SetAlignment(m_hAlign, m_vAlign);

	if (HasBorderLeft())
		attr->SetBorderLeft(m_borderLeft.m_style, m_borderLeft.m_colour, m_borderLeft.m_width);
	if (HasBorderRight())
		attr->SetBorderRight(m_borderRight.m_style, m_borderRight.m_colour, m_borderRight.m_width);
	if (HasBorderTop())
		attr->SetBorderTop(m_borderTop.m_style, m_borderTop.m_colour, m_borderTop.m_width);
	if (HasBorderBottom())
		attr->SetBorderBottom(m_borderTop.m_style, m_borderTop.m_colour, m_borderBottom.m_width);

	attr->SetSize(m_sizeRows, m_sizeCols);

	if (m_renderer)
	{
		attr->SetRenderer(m_renderer);
		m_renderer->IncRef();
	}
	if (m_editor)
	{
		attr->SetEditor(m_editor);
		m_editor->IncRef();
	}

	attr->CopyClientDataContainer(*this);

	if (IsReadOnly())
		attr->SetReadOnly();

	attr->m_fitMode = m_fitMode;
	attr->SetKind(m_attrkind);

	return attr;
}

void wxGridExtCellAttr::MergeWith(wxGridExtCellAttr* mergefrom)
{
	if (!HasTextColour() && mergefrom->HasTextColour())
		SetTextColour(mergefrom->GetTextColour());
	if (!HasTextOrient() && mergefrom->HasTextOrient())
		SetTextOrient(mergefrom->GetTextOrient());
	if (!HasBackgroundColour() && mergefrom->HasBackgroundColour())
		SetBackgroundColour(mergefrom->GetBackgroundColour());
	if (!HasFont() && mergefrom->HasFont())
		SetFont(mergefrom->GetFont());
	if (!HasAlignment() && mergefrom->HasAlignment())
	{
		int hAlign, vAlign;
		mergefrom->GetAlignment(&hAlign, &vAlign);
		SetAlignment(hAlign, vAlign);
	}

	if (!HasBorderLeft() && mergefrom->HasBorderLeft())
		SetBorderLeft(mergefrom->m_borderLeft.m_style, mergefrom->m_borderLeft.m_colour, mergefrom->m_borderLeft.m_width);
	if (!HasBorderRight() && mergefrom->HasBorderRight())
		SetBorderRight(mergefrom->m_borderRight.m_style, mergefrom->m_borderRight.m_colour, mergefrom->m_borderRight.m_width);
	if (!HasBorderTop() && mergefrom->HasBorderTop())
		SetBorderTop(mergefrom->m_borderTop.m_style, mergefrom->m_borderTop.m_colour, mergefrom->m_borderTop.m_width);
	if (!HasBorderBottom() && mergefrom->HasBorderBottom())
		SetBorderBottom(mergefrom->m_borderTop.m_style, mergefrom->m_borderTop.m_colour, mergefrom->m_borderBottom.m_width);

	if (!HasSize() && mergefrom->HasSize())
		mergefrom->GetSize(&m_sizeRows, &m_sizeCols);

	// Directly access member functions as GetRender/Editor don't just return
	// m_renderer/m_editor
	//
	// Maybe add support for merge of Render and Editor?
	if (!HasRenderer() && mergefrom->HasRenderer())
	{
		m_renderer = mergefrom->m_renderer;
		m_renderer->IncRef();
	}
	if (!HasEditor() && mergefrom->HasEditor())
	{
		m_editor = mergefrom->m_editor;
		m_editor->IncRef();
	}
	if (!HasClientDataContainer() && mergefrom->HasClientDataContainer())
	{
		CopyClientDataContainer(*mergefrom);
	}
	if (!HasReadWriteMode() && mergefrom->HasReadWriteMode())
		SetReadOnly(mergefrom->IsReadOnly());

	if (!HasOverflowMode() && mergefrom->HasOverflowMode())
		SetOverflow(mergefrom->GetOverflow());

	SetDefAttr(mergefrom->m_defGridAttr);
}

void wxGridExtCellAttr::SetSize(int num_rows, int num_cols)
{
	// The size of a cell is normally 1,1

	// If this cell is larger (2,2) then this is the top left cell
	// the other cells that will be covered (lower right cells) must be
	// set to negative or zero values such that
	// row + num_rows of the covered cell points to the larger cell (this cell)
	// same goes for the col + num_cols.

	// Size of 0,0 is NOT valid, neither is <=0 and any positive value

	wxASSERT_MSG((!((num_rows > 0) && (num_cols <= 0)) ||
		!((num_rows <= 0) && (num_cols > 0)) ||
		!((num_rows == 0) && (num_cols == 0))),
		wxT("wxGridExtCellAttr::SetSize only takes two positive values or negative/zero values"));

	m_sizeRows = num_rows;
	m_sizeCols = num_cols;
}

void wxGridExtCellAttr::SetBorderLeft(wxPenStyle style, const wxColour& colour, int width)
{
	m_borderLeft.m_style = style;
	m_borderLeft.m_colour = colour;
	m_borderLeft.m_width = width;
}

void wxGridExtCellAttr::SetBorderRight(wxPenStyle style, const wxColour& colour, int width)
{
	m_borderRight.m_style = style;
	m_borderRight.m_colour = colour;
	m_borderRight.m_width = width;
}

void wxGridExtCellAttr::SetBorderTop(wxPenStyle style, const wxColour& colour, int width)
{
	m_borderTop.m_style = style;
	m_borderTop.m_colour = colour;
	m_borderTop.m_width = width;
}

void wxGridExtCellAttr::SetBorderBottom(wxPenStyle style, const wxColour& colour, int width)
{
	m_borderBottom.m_style = style;
	m_borderBottom.m_colour = colour;
	m_borderBottom.m_width = width;
}

const wxColour& wxGridExtCellAttr::GetTextColour() const
{
	if (HasTextColour())
	{
		return m_colText;
	}
	else if (m_defGridAttr && m_defGridAttr != this)
	{
		return m_defGridAttr->GetTextColour();
	}
	else
	{
		wxFAIL_MSG(wxT("Missing default cell attribute"));
		return wxNullColour;
	}
}

const int wxGridExtCellAttr::GetTextOrient() const
{
	if (HasTextOrient())
	{
		return m_orientText;
	}
	else if (m_defGridAttr && m_defGridAttr != this)
	{
		return m_defGridAttr->GetTextOrient();
	}
	else
	{
		wxFAIL_MSG(wxT("Missing default cell attribute"));
		return -1;
	}
}

const wxColour& wxGridExtCellAttr::GetBackgroundColour() const
{
	if (HasBackgroundColour())
	{
		return m_colBack;
	}
	else if (m_defGridAttr && m_defGridAttr != this)
	{
		return m_defGridAttr->GetBackgroundColour();
	}
	else
	{
		wxFAIL_MSG(wxT("Missing default cell attribute"));
		return wxNullColour;
	}
}

const wxFont& wxGridExtCellAttr::GetFont() const
{
	if (HasFont())
	{
		return m_font;
	}
	else if (m_defGridAttr && m_defGridAttr != this)
	{
		return m_defGridAttr->GetFont();
	}
	else
	{
		wxFAIL_MSG(wxT("Missing default cell attribute"));
		return wxNullFont;
	}
}

void wxGridExtCellAttr::GetAlignment(int* hAlign, int* vAlign) const
{
	if (HasAlignment())
	{
		if (hAlign)
			*hAlign = m_hAlign;
		if (vAlign)
			*vAlign = m_vAlign;
	}
	else if (m_defGridAttr && m_defGridAttr != this)
	{
		m_defGridAttr->GetAlignment(hAlign, vAlign);
	}
	else
	{
		wxFAIL_MSG(wxT("Missing default cell attribute"));
	}
}

void wxGridExtCellAttr::GetNonDefaultAlignment(int* hAlign, int* vAlign) const
{
	// The logic here is tricky but necessary to handle all the cases: if we
	// have non-default alignment on input, we should only override it if this
	// attribute specifies a non-default alignment. However if the input
	// alignment is invalid, we need to always initialize it, using the default
	// attribute if necessary.

	// First of all, never dereference null pointer.
	if (hAlign)
	{
		if (this != m_defGridAttr && m_hAlign != wxALIGN_INVALID)
		{
			// This attribute has its own alignment, which should always
			// override the input alignment value.
			*hAlign = m_hAlign;
		}
		else if (*hAlign == wxALIGN_INVALID)
		{
			// No input alignment specified, fill it with the default alignment
			// (note that we know that this attribute itself doesn't have any
			// specific alignment or is the same as the default one anyhow in
			// in this "else" branch, so we don't need to check m_hAlign here).
			*hAlign = m_defGridAttr->m_hAlign;
		}
		//else: Input alignment is valid but ours one isn't, nothing to do.
	}

	// This is exactly the same logic as above.
	if (vAlign)
	{
		if (this != m_defGridAttr && m_vAlign != wxALIGN_INVALID)
			*vAlign = m_vAlign;
		else if (*vAlign == wxALIGN_INVALID)
			*vAlign = m_defGridAttr->m_vAlign;
	}
}

wxGridExtCellBorder wxGridExtCellAttr::GetBorderLeft() const
{
	if (HasBorderLeft())
	{
		return m_borderLeft;
	}
	else if (m_defGridAttr && m_defGridAttr != this)
	{
		return m_defGridAttr->GetBorderLeft();
	}
	else
	{
		wxFAIL_MSG(wxT("Missing default cell attribute"));
		return wxGridExtCellBorder();
	}
}

wxGridExtCellBorder wxGridExtCellAttr::GetBorderRight() const
{
	if (HasBorderRight())
	{
		return m_borderRight;
	}
	else if (m_defGridAttr && m_defGridAttr != this)
	{
		return m_defGridAttr->GetBorderRight();
	}
	else
	{
		wxFAIL_MSG(wxT("Missing default cell attribute"));
		return wxGridExtCellBorder();
	}
}

wxGridExtCellBorder wxGridExtCellAttr::GetBorderTop() const
{
	if (HasBorderTop())
	{
		return m_borderTop;
	}
	else if (m_defGridAttr && m_defGridAttr != this)
	{
		return m_defGridAttr->GetBorderTop();
	}
	else
	{
		wxFAIL_MSG(wxT("Missing default cell attribute"));
		return wxGridExtCellBorder();
	}
}

wxGridExtCellBorder wxGridExtCellAttr::GetBorderBottom() const
{
	if (HasBorderBottom())
	{
		return m_borderBottom;
	}
	else if (m_defGridAttr && m_defGridAttr != this)
	{
		return m_defGridAttr->GetBorderBottom();
	}
	else
	{
		wxFAIL_MSG(wxT("Missing default cell attribute"));
		return wxGridExtCellBorder();
	}
}

void wxGridExtCellAttr::GetSize(int* num_rows, int* num_cols) const
{
	if (num_rows)
		*num_rows = m_sizeRows;
	if (num_cols)
		*num_cols = m_sizeCols;
}

wxGridExtFitMode wxGridExtCellAttr::GetFitMode() const
{
	if (m_fitMode.IsSpecified())
	{
		return m_fitMode;
	}
	else if (m_defGridAttr && m_defGridAttr != this)
	{
		return m_defGridAttr->GetFitMode();
	}
	else
	{
		wxFAIL_MSG(wxT("Missing default cell attribute"));
		return wxGridExtFitMode();
	}
}

bool wxGridExtCellAttr::CanOverflow() const
{
	// If overflow is disabled anyhow, we definitely can't overflow.
	if (!GetOverflow())
		return false;

	// But if it is enabled, we still don't use it for right-aligned or
	// centered cells because it's not really clear how it should work for
	// them.
	int hAlign = wxALIGN_LEFT;
	GetNonDefaultAlignment(&hAlign, NULL);

	return hAlign == wxALIGN_LEFT;
}

// GetRenderer and GetEditor use a slightly different decision path about
// which attribute to use.  If a non-default attr object has one then it is
// used, otherwise the default editor or renderer is fetched from the grid and
// used.  It should be the default for the data type of the cell.  If it is
// NULL (because the table has a type that the grid does not have in its
// registry), then the grid's default editor or renderer is used.

wxGridExtCellRenderer* wxGridExtCellAttr::GetRenderer(const wxGridExt* grid, int row, int col) const
{
	wxGridExtCellRenderer* renderer = NULL;

	if (m_renderer && this != m_defGridAttr)
	{
		// use the cells renderer if it has one
		renderer = m_renderer;
		renderer->IncRef();
	}
	else // no non-default cell renderer
	{
		// get default renderer for the data type
		if (grid)
		{
			// GetDefaultRendererForCell() will do IncRef() for us
			renderer = grid->GetDefaultRendererForCell(row, col);
		}

		if (renderer == NULL)
		{
			if ((m_defGridAttr != NULL) && (m_defGridAttr != this))
			{
				// if we still don't have one then use the grid default
				// (no need for IncRef() here either)
				renderer = m_defGridAttr->GetRenderer(NULL, 0, 0);
			}
			else // default grid attr
			{
				// use m_renderer which we had decided not to use initially
				renderer = m_renderer;
				if (renderer)
					renderer->IncRef();
			}
		}
	}

	// we're supposed to always find something
	wxASSERT_MSG(renderer, wxT("Missing default cell renderer"));

	return renderer;
}

// same as above, except for s/renderer/editor/g
wxGridExtCellEditor* wxGridExtCellAttr::GetEditor(const wxGridExt* grid, int row, int col) const
{
	wxGridExtCellEditor* editor = NULL;

	if (m_editor && this != m_defGridAttr)
	{
		// use the cells editor if it has one
		editor = m_editor;
		editor->IncRef();
	}
	else // no non default cell editor
	{
		// get default editor for the data type
		if (grid)
		{
			// GetDefaultEditorForCell() will do IncRef() for us
			editor = grid->GetDefaultEditorForCell(row, col);
		}

		if (editor == NULL)
		{
			if ((m_defGridAttr != NULL) && (m_defGridAttr != this))
			{
				// if we still don't have one then use the grid default
				// (no need for IncRef() here either)
				editor = m_defGridAttr->GetEditor(NULL, 0, 0);
			}
			else // default grid attr
			{
				// use m_editor which we had decided not to use initially
				editor = m_editor;
				if (editor)
					editor->IncRef();
			}
		}
	}

	// we're supposed to always find something
	wxASSERT_MSG(editor, wxT("Missing default cell editor"));

	return editor;
}

// ----------------------------------------------------------------------------
// wxGridExtCellAttrData
// ----------------------------------------------------------------------------

namespace
{

	// Helper functions to convert grid coords to a key for the attr map, and
	// vice versa.

	wxGridExtCoordsToAttrMap::key_type CoordsToKey(int row, int col)
	{
		// Treat both row and col as unsigned to not cause havoc with (unsupported)
		// negative coords.
		return (static_cast<wxULongLong_t>(row) << 32) + static_cast<wxUint32>(col);
	}

	void KeyToCoords(wxGridExtCoordsToAttrMap::key_type key, int* pRow, int* pCol)
	{
		*pRow = key >> 32;
		*pCol = key & wxUINT32_MAX;
	}

} // anonymous namespace

wxGridExtCellAttrData::~wxGridExtCellAttrData()
{
	for (wxGridExtCoordsToAttrMap::iterator it = m_attrs.begin();
		it != m_attrs.end();
		++it)
	{
		it->second->DecRef();
	}

	m_attrs.clear();
}

void wxGridExtCellAttrData::SetAttr(wxGridExtCellAttr* attr, int row, int col)
{
	wxGridExtCoordsToAttrMap::iterator it = FindIndex(row, col);
	if (it == m_attrs.end())
	{
		if (attr)
		{
			// add the attribute
			m_attrs[CoordsToKey(row, col)] = attr;
		}
		//else: nothing to do
	}
	else // we already have an attribute for this cell
	{
		// See note near DecRef() in wxGridExtRowOrColAttrData::SetAttr for why
		// this also works when old and new attribute are the same.
		it->second->DecRef();

		// Change or remove the attribute.
		if (attr)
			it->second = attr;
		else
			m_attrs.erase(it);
	}
}

wxGridExtCellAttr* wxGridExtCellAttrData::GetAttr(int row, int col) const
{
	wxGridExtCellAttr* attr = NULL;

	wxGridExtCoordsToAttrMap::iterator it = FindIndex(row, col);
	if (it != m_attrs.end())
	{
		attr = it->second;
		attr->IncRef();
	}

	return attr;
}

namespace
{

	void UpdateCellAttrRowsOrCols(wxGridExtCoordsToAttrMap& attrs, int editPos,
		int editRowCount, int editColCount)
	{
		wxASSERT(!editRowCount || !editColCount);

		const bool isEditingRows = (editRowCount != 0);
		const int editCount = (isEditingRows ? editRowCount : editColCount);

		// Copy updated attributes to a new map instead of attempting to edit attrs
		// map in-place. This requires more memory but greatly simplifies the code
		// and any attempt with in-place editing would likely also require a lot
		// more attr lookups.
		// The updated copy will contain an attrs map with, if applicable: adjusted
		// coords and cell size, newly inserted attributes (for multicells), and
		// without now deleted attributes.
		wxGridExtCoordsToAttrMap newAttrs;

		for (wxGridExtCoordsToAttrMap::iterator it = attrs.begin();
			it != attrs.end();
			++it)
		{
			const wxGridExtCoordsToAttrMap::key_type oldCoords = it->first;
			wxGridExtCellAttr* cellAttr = it->second;

			int cellRows, cellCols;
			cellAttr->GetSize(&cellRows, &cellCols);

			int cellRow, cellCol;
			KeyToCoords(oldCoords, &cellRow, &cellCol);

			const int cellPos = isEditingRows ? cellRow : cellCol;

			if (cellPos < editPos)
			{
				// This cell's coords aren't influenced by the editing, however
				// do adjust a multicell's main size, if needed.
				if (GetCellSpan(cellRows, cellCols) == wxGridExt::CellSpan_Main)
				{
					int mainSize = isEditingRows ? cellRows : cellCols;
					if (cellPos + mainSize > editPos)
					{
						// Multicell is within affected range:
						// Adjust its size.

						if (editCount >= 0)
						{
							mainSize += editCount;
						}
						else
						{
							// Reduce multicell size by number of deletions, but
							// never more than the multicell's size minus one:
							// cellPos (the main cell) is always less than editPos
							// at this point, then with the below code a multicell
							// with size 7 is at most reduced by:
							// cellPos + 7 - (cellPos + 1) = 7 - 1 = 6.
							mainSize -= wxMin(-editCount,
								cellPos + mainSize - editPos);
							/*
							The above was derived from:
							first_del = edit
							last_del = min(edit - count - 1, cell + size - 1)
							size -= (last_del + 1 - first_del)

							eliminating the 1's:

							first_del = edit
							last_del = min(edit - count, cell + size)
							size -= (last_del - first_del)

							reducing each by edit:

							first_del = 0
							last_del_plus_1 = min(0 - count, cell + size - edit)
							size -= (last_del_plus_1 - 0)

							after eliminating the 0's and substitution, leaving:

							size -= min(-count, cell + size - edit)

							E.g. with a multicell of size 7 and at 2 positions
							after the main cell 100 positions are deleted then
							the size will not (/can't) be reduced by 100 cells
							but by:

							cellPos + 7 - editPos =       # editPos = cellPos + 2
							cellPos + 7 - (cellPos + 2) = # eliminate cellPos
							7 - 2 =
							5 cells, making the final size 7 - 5 = 2.
							*/
						}

						cellAttr->SetSize(isEditingRows ? mainSize : cellRows,
							isEditingRows ? cellCols : mainSize);
					}
				}

				// Set attribute at old/unmodified coords.
				newAttrs[oldCoords] = cellAttr;

				continue;
			}

			if (editCount < 0 && cellPos < editPos - editCount)
			{
				// This row/col is deleted and the cell doesn't exist any longer:
				// Remove the attribute.
				cellAttr->DecRef();

				continue;
			}

			const wxGridExtCoordsToAttrMap::key_type newCoords
				= CoordsToKey(cellRow + editRowCount, cellCol + editColCount);

			if (GetCellSpan(cellRows, cellCols) != wxGridExt::CellSpan_Inside)
			{
				// Rows/cols inserted or deleted (and this cell still exists):
				// Adjust cell coords.
				newAttrs[newCoords] = cellAttr;

				// Nothing more to do: cell is not an inside cell of a multicell.
				continue;
			}

			// Handle inside cell's existence, coords, and size.

			const int mainPos = cellPos + (isEditingRows ? cellRows : cellCols);

			if (editCount < 0
				&& mainPos >= editPos && mainPos < editPos - editCount)
			{
				// On a position that still exists after deletion but main cell
				// of multicell is within deletion range so the multicell is gone:
				// Remove the attribute.
				cellAttr->DecRef();

				continue;
			}

			// Rows/cols inserted or deleted (and this inside cell still exists):
			// Adjust (inside) cell coords.
			newAttrs[newCoords] = cellAttr;

			if (mainPos >= editPos)
			{
				// Nothing more to do: the multicell that this inside cell is part
				// of is moving its main cell as well so offsets to the main cell
				// don't change and there are no edits changing the multicell size.
				continue;
			}

			if (editCount > 0 && cellPos == editPos)
			{
				// At an (old) position that is newly inserted: this is the only
				// opportunity to add required inside cells that point to
				// the main cell. E.g. with a 2x1 multicell that increases in size
				// there's only one inside cell that will be visited while there
				// can be multiple insertions.
				for (int i = 0; i < editCount; ++i)
				{
					const int adjustRows = i * isEditingRows,
						adjustCols = i * !isEditingRows;

					wxGridExtCellAttr* attr = new wxGridExtCellAttr;
					attr->SetSize(cellRows - adjustRows, cellCols - adjustCols);

					const int row = cellRow + adjustRows,
						col = cellCol + adjustCols;
					newAttrs[CoordsToKey(row, col)] = attr;
				}
			}

			// Let this inside cell's size point to the main cell of the multicell.
			cellAttr->SetSize(cellRows - editRowCount, cellCols - editColCount);
		}

		attrs = newAttrs;
	}

} // anonymous namespace

void wxGridExtCellAttrData::UpdateAttrRows(size_t pos, int numRows)
{
	UpdateCellAttrRowsOrCols(m_attrs, static_cast<int>(pos), numRows, 0);
}

void wxGridExtCellAttrData::UpdateAttrCols(size_t pos, int numCols)
{
	UpdateCellAttrRowsOrCols(m_attrs, static_cast<int>(pos), 0, numCols);
}

wxGridExtCoordsToAttrMap::iterator
wxGridExtCellAttrData::FindIndex(int row, int col) const
{
	return m_attrs.find(CoordsToKey(row, col));
}

// ----------------------------------------------------------------------------
// wxGridExtRowOrColAttrData
// ----------------------------------------------------------------------------

wxGridExtRowOrColAttrData::~wxGridExtRowOrColAttrData()
{
	size_t count = m_attrs.GetCount();
	for (size_t n = 0; n < count; n++)
	{
		m_attrs[n]->DecRef();
	}
}

wxGridExtCellAttr* wxGridExtRowOrColAttrData::GetAttr(int rowOrCol) const
{
	wxGridExtCellAttr* attr = NULL;

	int n = m_rowsOrCols.Index(rowOrCol);
	if (n != wxNOT_FOUND)
	{
		attr = m_attrs[(size_t)n];
		attr->IncRef();
	}

	return attr;
}

void wxGridExtRowOrColAttrData::SetAttr(wxGridExtCellAttr* attr, int rowOrCol)
{
	int i = m_rowsOrCols.Index(rowOrCol);
	if (i == wxNOT_FOUND)
	{
		if (attr)
		{
			// store the new attribute, taking its ownership
			m_rowsOrCols.Add(rowOrCol);
			m_attrs.Add(attr);
		}
		// nothing to remove
	}
	else // we have an attribute for this row or column
	{
		size_t n = (size_t)i;

		// notice that this code works correctly even when the old attribute is
		// the same as the new one: as we own of it, we must call DecRef() on
		// it in any case and this won't result in destruction of the new
		// attribute if it's the same as old one because it must have ref count
		// of at least 2 to be passed to us while we keep a reference to it too
		m_attrs[n]->DecRef();

		if (attr)
		{
			// replace the attribute with the new one
			m_attrs[n] = attr;
		}
		else // remove the attribute
		{
			m_rowsOrCols.RemoveAt(n);
			m_attrs.RemoveAt(n);
		}
	}
}

void wxGridExtRowOrColAttrData::UpdateAttrRowsOrCols(size_t pos, int numRowsOrCols)
{
	size_t count = m_attrs.GetCount();
	for (size_t n = 0; n < count; n++)
	{
		int& rowOrCol = m_rowsOrCols[n];
		if ((size_t)rowOrCol >= pos)
		{
			if (numRowsOrCols > 0)
			{
				// If rows or cols inserted, increment row/col counter where necessary
				rowOrCol += numRowsOrCols;
			}
			else if (numRowsOrCols < 0)
			{
				// If rows/cols deleted, either decrement row/col counter (if row/col still exists)
				if ((size_t)rowOrCol >= pos - numRowsOrCols)
					rowOrCol += numRowsOrCols;
				else
				{
					m_rowsOrCols.RemoveAt(n);
					m_attrs[n]->DecRef();
					m_attrs.RemoveAt(n);
					n--;
					count--;
				}
			}
		}
	}
}

// ----------------------------------------------------------------------------
// wxGridExtCellAttrProvider
// ----------------------------------------------------------------------------

wxGridExtCellAttrProvider::wxGridExtCellAttrProvider()
{
	m_data = NULL;
}

wxGridExtCellAttrProvider::~wxGridExtCellAttrProvider()
{
	delete m_data;
}

void wxGridExtCellAttrProvider::InitData()
{
	m_data = new wxGridExtCellAttrProviderData;
}

wxGridExtCellAttr* wxGridExtCellAttrProvider::GetAttr(int row, int col,
	wxGridExtCellAttr::wxAttrKind  kind) const
{
	wxGridExtCellAttr* attr = NULL;
	if (m_data)
	{
		switch (kind)
		{
		case (wxGridExtCellAttr::Any):
			// Get cached merge attributes.
			// Currently not used as no cache implemented as not mutable
			// attr = m_data->m_mergeAttr.GetAttr(row, col);
			if (!attr)
			{
				// Basically implement old version.
				// Also check merge cache, so we don't have to re-merge every time..
				wxGridExtCellAttr* attrcell = m_data->m_cellAttrs.GetAttr(row, col);
				wxGridExtCellAttr* attrrow = m_data->m_rowAttrs.GetAttr(row);
				wxGridExtCellAttr* attrcol = m_data->m_colAttrs.GetAttr(col);

				if ((attrcell != attrrow) && (attrrow != attrcol) && (attrcell != attrcol))
				{
					// Two or more are non NULL
					attr = new wxGridExtCellAttr;
					attr->SetKind(wxGridExtCellAttr::Merged);

					// Order is important..
					if (attrcell)
					{
						attr->MergeWith(attrcell);
						attrcell->DecRef();
					}
					if (attrcol)
					{
						attr->MergeWith(attrcol);
						attrcol->DecRef();
					}
					if (attrrow)
					{
						attr->MergeWith(attrrow);
						attrrow->DecRef();
					}

					// store merge attr if cache implemented
					//attr->IncRef();
					//m_data->m_mergeAttr.SetAttr(attr, row, col);
				}
				else
				{
					// one or none is non null return it or null.
					if (attrrow)
						attr = attrrow;
					if (attrcol)
					{
						if (attr)
							attr->DecRef();
						attr = attrcol;
					}
					if (attrcell)
					{
						if (attr)
							attr->DecRef();
						attr = attrcell;
					}
				}
			}
			break;

		case (wxGridExtCellAttr::Cell):
			attr = m_data->m_cellAttrs.GetAttr(row, col);
			break;

		case (wxGridExtCellAttr::Col):
			attr = m_data->m_colAttrs.GetAttr(col);
			break;

		case (wxGridExtCellAttr::Row):
			attr = m_data->m_rowAttrs.GetAttr(row);
			break;

		default:
			// unused as yet...
			// (wxGridExtCellAttr::Default):
			// (wxGridExtCellAttr::Merged):
			break;
		}
	}

	return attr;
}

void wxGridExtCellAttrProvider::SetAttr(wxGridExtCellAttr* attr,
	int row, int col)
{
	if (!m_data)
		InitData();

	m_data->m_cellAttrs.SetAttr(attr, row, col);
}

void wxGridExtCellAttrProvider::SetRowAttr(wxGridExtCellAttr* attr, int row)
{
	if (!m_data)
		InitData();

	m_data->m_rowAttrs.SetAttr(attr, row);
}

void wxGridExtCellAttrProvider::SetColAttr(wxGridExtCellAttr* attr, int col)
{
	if (!m_data)
		InitData();

	m_data->m_colAttrs.SetAttr(attr, col);
}

void wxGridExtCellAttrProvider::UpdateAttrRows(size_t pos, int numRows)
{
	if (m_data)
	{
		m_data->m_cellAttrs.UpdateAttrRows(pos, numRows);

		m_data->m_rowAttrs.UpdateAttrRowsOrCols(pos, numRows);
	}
}

void wxGridExtCellAttrProvider::UpdateAttrCols(size_t pos, int numCols)
{
	if (m_data)
	{
		m_data->m_cellAttrs.UpdateAttrCols(pos, numCols);

		m_data->m_colAttrs.UpdateAttrRowsOrCols(pos, numCols);
	}
}

const wxGridExtColumnHeaderRenderer&
wxGridExtCellAttrProvider::GetColumnHeaderRenderer(int WXUNUSED(col))
{
	return gs_defaultHeaderRenderers.colRenderer;
}

const wxGridExtRowHeaderRenderer&
wxGridExtCellAttrProvider::GetRowHeaderRenderer(int WXUNUSED(row))
{
	return gs_defaultHeaderRenderers.rowRenderer;
}

const wxGridExtCornerHeaderRenderer& wxGridExtCellAttrProvider::GetCornerRenderer()
{
	return gs_defaultHeaderRenderers.cornerRenderer;
}

// ----------------------------------------------------------------------------
// wxGridExtBlockCoords
// ----------------------------------------------------------------------------

wxGridExtBlockDiffResult
wxGridExtBlockCoords::Difference(const wxGridExtBlockCoords& other,
	int splitOrientation) const
{
	wxGridExtBlockDiffResult result;

	// Check whether the blocks intersect.
	if (!Intersects(other))
	{
		result.m_parts[0] = *this;
		return result;
	}

	// Split the block in up to 4 new parts, that don't contain the other
	// block, like this (for wxHORIZONTAL):
	// |-----------------------------|
	// |                             |
	// |           part[0]           |
	// |                             |
	// |-----------------------------|
	// | part[2] |  other  | part[3] |
	// |-----------------------------|
	// |                             |
	// |           part[1]           |
	// |                             |
	// |-----------------------------|
	// And for wxVERTICAL:
	// |-----------------------------|
	// |         |         |         |
	// |         | part[2] |         |
	// |         |         |         |
	// |         |---------|         |
	// | part[0] |  other  | part[1] |
	// |         |---------|         |
	// |         |         |         |
	// |         | part[3] |         |
	// |         |         |         |
	// |-----------------------------|

	if (splitOrientation == wxHORIZONTAL)
	{
		// Part[0].
		if (m_topRow < other.m_topRow)
			result.m_parts[0] =
			wxGridExtBlockCoords(m_topRow, m_leftCol,
				other.m_topRow - 1, m_rightCol);

		// Part[1].
		if (m_bottomRow > other.m_bottomRow)
			result.m_parts[1] =
			wxGridExtBlockCoords(other.m_bottomRow + 1, m_leftCol,
				m_bottomRow, m_rightCol);

		const int maxTopRow = wxMax(m_topRow, other.m_topRow);
		const int minBottomRow = wxMin(m_bottomRow, other.m_bottomRow);

		// Part[2].
		if (m_leftCol < other.m_leftCol)
			result.m_parts[2] =
			wxGridExtBlockCoords(maxTopRow, m_leftCol,
				minBottomRow, other.m_leftCol - 1);

		// Part[3].
		if (m_rightCol > other.m_rightCol)
			result.m_parts[3] =
			wxGridExtBlockCoords(maxTopRow, other.m_rightCol + 1,
				minBottomRow, m_rightCol);
	}
	else // wxVERTICAL
	{
		// Part[0].
		if (m_leftCol < other.m_leftCol)
			result.m_parts[0] =
			wxGridExtBlockCoords(m_topRow, m_leftCol,
				m_bottomRow, other.m_leftCol - 1);

		// Part[1].
		if (m_rightCol > other.m_rightCol)
			result.m_parts[1] =
			wxGridExtBlockCoords(m_topRow, other.m_rightCol + 1,
				m_bottomRow, m_rightCol);

		const int maxLeftCol = wxMax(m_leftCol, other.m_leftCol);
		const int minRightCol = wxMin(m_rightCol, other.m_rightCol);

		// Part[2].
		if (m_topRow < other.m_topRow)
			result.m_parts[2] =
			wxGridExtBlockCoords(m_topRow, maxLeftCol,
				other.m_topRow - 1, minRightCol);

		// Part[3].
		if (m_bottomRow > other.m_bottomRow)
			result.m_parts[3] =
			wxGridExtBlockCoords(other.m_bottomRow + 1, maxLeftCol,
				m_bottomRow, minRightCol);
	}

	return result;
}

wxGridExtBlockDiffResult
wxGridExtBlockCoords::SymDifference(const wxGridExtBlockCoords& other) const
{
	wxGridExtBlockDiffResult result;

	// Check whether the blocks intersect.
	if (!Intersects(other))
	{
		result.m_parts[0] = *this;
		result.m_parts[1] = other;
		return result;
	}

	// Possible result blocks:
	// |------------------|
	// |                  |            minUpper->m_topRow
	// |      part[0]     |
	// |                  |            maxUpper->m_topRow - 1
	// |-----------------------------|
	// |         |         |         | maxUpper->m_topRow
	// | part[2] |    x    | part[3] |
	// |         |         |         | minLower->m_bottomRow
	// |-----------------------------|
	//           |                   | minLower->m_bottomRow + 1
	//           |      part[1]      |
	//           |                   | maxLower->m_bottomRow
	//           |-------------------|
	//
	// The x marks the intersection of the blocks, which is not a part
	// of the result.

	// Part[0].
	int maxUpperRow;
	if (m_topRow != other.m_topRow)
	{
		const bool block1Min = m_topRow < other.m_topRow;
		const wxGridExtBlockCoords& minUpper = block1Min ? *this : other;
		const wxGridExtBlockCoords& maxUpper = block1Min ? other : *this;
		maxUpperRow = maxUpper.m_topRow;

		result.m_parts[0] = wxGridExtBlockCoords(minUpper.m_topRow,
			minUpper.m_leftCol,
			maxUpper.m_topRow - 1,
			minUpper.m_rightCol);
	}
	else
	{
		maxUpperRow = m_topRow;
	}

	// Part[1].
	int minLowerRow;
	if (m_bottomRow != other.m_bottomRow)
	{
		const bool block1Min = m_bottomRow < other.m_bottomRow;
		const wxGridExtBlockCoords& minLower = block1Min ? *this : other;
		const wxGridExtBlockCoords& maxLower = block1Min ? other : *this;
		minLowerRow = minLower.m_bottomRow;

		result.m_parts[1] = wxGridExtBlockCoords(minLower.m_bottomRow + 1,
			maxLower.m_leftCol,
			maxLower.m_bottomRow,
			maxLower.m_rightCol);
	}
	else
	{
		minLowerRow = m_bottomRow;
	}

	// Part[2].
	if (m_leftCol != other.m_leftCol)
	{
		result.m_parts[2] = wxGridExtBlockCoords(maxUpperRow,
			wxMin(m_leftCol,
				other.m_leftCol),
			minLowerRow,
			wxMax(m_leftCol,
				other.m_leftCol) - 1);
	}

	// Part[3].
	if (m_rightCol != other.m_rightCol)
	{
		result.m_parts[3] = wxGridExtBlockCoords(maxUpperRow,
			wxMin(m_rightCol,
				other.m_rightCol) + 1,
			minLowerRow,
			wxMax(m_rightCol,
				other.m_rightCol));
	}

	return result;
}

// ----------------------------------------------------------------------------
// wxGridExtTableBase
// ----------------------------------------------------------------------------

wxIMPLEMENT_ABSTRACT_CLASS(wxGridExtTableBase, wxObject);

wxGridExtTableBase::wxGridExtTableBase()
{
	m_view = NULL;
	m_attrProvider = NULL;
}

wxGridExtTableBase::~wxGridExtTableBase()
{
	delete m_attrProvider;
}

void wxGridExtTableBase::SetAttrProvider(wxGridExtCellAttrProvider* attrProvider)
{
	delete m_attrProvider;
	m_attrProvider = attrProvider;
}

bool wxGridExtTableBase::CanHaveAttributes()
{
	if (!GetAttrProvider())
	{
		// use the default attr provider by default
		SetAttrProvider(new wxGridExtCellAttrProvider);
	}

	return true;
}

wxGridExtCellAttr* wxGridExtTableBase::GetAttr(int row, int col, wxGridExtCellAttr::wxAttrKind  kind)
{
	if (m_attrProvider)
		return m_attrProvider->GetAttr(row, col, kind);
	else
		return NULL;
}

void wxGridExtTableBase::SetAttr(wxGridExtCellAttr* attr, int row, int col)
{
	if (m_attrProvider)
	{
		if (attr)
			attr->SetKind(wxGridExtCellAttr::Cell);
		m_attrProvider->SetAttr(attr, row, col);
	}
	else
	{
		// as we take ownership of the pointer and don't store it, we must
		// free it now
		wxSafeDecRef(attr);
	}
}

void wxGridExtTableBase::SetRowAttr(wxGridExtCellAttr* attr, int row)
{
	if (m_attrProvider)
	{
		if (attr)
			attr->SetKind(wxGridExtCellAttr::Row);
		m_attrProvider->SetRowAttr(attr, row);
	}
	else
	{
		// as we take ownership of the pointer and don't store it, we must
		// free it now
		wxSafeDecRef(attr);
	}
}

void wxGridExtTableBase::SetColAttr(wxGridExtCellAttr* attr, int col)
{
	if (m_attrProvider)
	{
		if (attr)
			attr->SetKind(wxGridExtCellAttr::Col);
		m_attrProvider->SetColAttr(attr, col);
	}
	else
	{
		// as we take ownership of the pointer and don't store it, we must
		// free it now
		wxSafeDecRef(attr);
	}
}

bool wxGridExtTableBase::InsertRows(size_t WXUNUSED(pos),
	size_t WXUNUSED(numRows))
{
	wxFAIL_MSG(wxT("Called grid table class function InsertRows\nbut your derived table class does not override this function"));

	return false;
}

bool wxGridExtTableBase::AppendRows(size_t WXUNUSED(numRows))
{
	wxFAIL_MSG(wxT("Called grid table class function AppendRows\nbut your derived table class does not override this function"));

	return false;
}

bool wxGridExtTableBase::DeleteRows(size_t WXUNUSED(pos),
	size_t WXUNUSED(numRows))
{
	wxFAIL_MSG(wxT("Called grid table class function DeleteRows\nbut your derived table class does not override this function"));

	return false;
}

bool wxGridExtTableBase::InsertCols(size_t WXUNUSED(pos),
	size_t WXUNUSED(numCols))
{
	wxFAIL_MSG(wxT("Called grid table class function InsertCols\nbut your derived table class does not override this function"));

	return false;
}

bool wxGridExtTableBase::AppendCols(size_t WXUNUSED(numCols))
{
	wxFAIL_MSG(wxT("Called grid table class function AppendCols\nbut your derived table class does not override this function"));

	return false;
}

bool wxGridExtTableBase::DeleteCols(size_t WXUNUSED(pos),
	size_t WXUNUSED(numCols))
{
	wxFAIL_MSG(wxT("Called grid table class function DeleteCols\nbut your derived table class does not override this function"));

	return false;
}

wxString wxGridExtTableBase::GetRowLabelValue(int row)
{
	wxString s;

	// RD: Starting the rows at zero confuses users,
	// no matter how much it makes sense to us geeks.
	s << row + 1;

	return s;
}

wxString wxGridExtTableBase::GetColLabelValue(int col)
{
	// default col labels are:
	//   cols 0 to 25   : A-Z
	//   cols 26 to 675 : AA-ZZ
	//   etc.

	wxString s;
	unsigned int i, n;
	for (n = 1; ; n++)
	{
		s += (wxChar)(wxT('A') + (wxChar)(col % 26));
		col = col / 26 - 1;
		if (col < 0)
			break;
	}

	// reverse the string...
	wxString s2;
	for (i = 0; i < n; i++)
	{
		s2 += s[n - i - 1];
	}

	return s2;
}

wxString wxGridExtTableBase::GetCornerLabelValue() const
{
	return wxString();
}

bool wxGridExtTableBase::GetTypeName(int WXUNUSED(row), int WXUNUSED(col), wxString& typeName)
{
	static wxString typeCacheName = wxGRID_VALUE_STRING;
	if (!typeCacheName.IsSameAs(typeName))
		typeName = typeCacheName;
	return true;
}

bool wxGridExtTableBase::CanGetValueAs(int WXUNUSED(row), int WXUNUSED(col),
	const wxString& typeName)
{
	return typeName == wxGRID_VALUE_STRING;
}

bool wxGridExtTableBase::CanSetValueAs(int row, int col, const wxString& typeName)
{
	return CanGetValueAs(row, col, typeName);
}

long wxGridExtTableBase::GetValueAsLong(int WXUNUSED(row), int WXUNUSED(col))
{
	return 0;
}

double wxGridExtTableBase::GetValueAsDouble(int WXUNUSED(row), int WXUNUSED(col))
{
	return 0.0;
}

bool wxGridExtTableBase::GetValueAsBool(int WXUNUSED(row), int WXUNUSED(col))
{
	return false;
}

void wxGridExtTableBase::SetValueAsLong(int WXUNUSED(row), int WXUNUSED(col),
	long WXUNUSED(value))
{
}

void wxGridExtTableBase::SetValueAsDouble(int WXUNUSED(row), int WXUNUSED(col),
	double WXUNUSED(value))
{
}

void wxGridExtTableBase::SetValueAsBool(int WXUNUSED(row), int WXUNUSED(col),
	bool WXUNUSED(value))
{
}

void* wxGridExtTableBase::GetValueAsCustom(int WXUNUSED(row), int WXUNUSED(col),
	const wxString& WXUNUSED(typeName))
{
	return NULL;
}

void  wxGridExtTableBase::SetValueAsCustom(int WXUNUSED(row), int WXUNUSED(col),
	const wxString& WXUNUSED(typeName),
	void* WXUNUSED(value))
{
}

//////////////////////////////////////////////////////////////////////
//
// Message class for the grid table to send requests and notifications
// to the grid view
//

wxGridExtTableMessage::wxGridExtTableMessage()
{
	m_table = NULL;
	m_id = -1;
	m_comInt1 = -1;
	m_comInt2 = -1;
}

wxGridExtTableMessage::wxGridExtTableMessage(wxGridExtTableBase* table, int id,
	int commandInt1, int commandInt2)
{
	m_table = table;
	m_id = id;
	m_comInt1 = commandInt1;
	m_comInt2 = commandInt2;
}

//////////////////////////////////////////////////////////////////////
//
// A basic grid table for string data. An object of this class will
// created by wxGridExt if you don't specify an alternative table class.
//

WX_DEFINE_OBJARRAY(wxGridExtStringArray)

wxIMPLEMENT_DYNAMIC_CLASS(wxGridExtStringTable, wxGridExtTableBase);

wxGridExtStringTable::wxGridExtStringTable()
	: wxGridExtTableBase()
{
	m_numCols = 0;
}

wxGridExtStringTable::wxGridExtStringTable(int numRows, int numCols)
	: wxGridExtTableBase()
{
	m_numCols = numCols;

	m_data.Alloc(numRows);

	wxArrayString sa;
	sa.Alloc(numCols);
	sa.Add(wxEmptyString, numCols);

	m_data.Add(sa, numRows);
}

wxString wxGridExtStringTable::GetValue(int row, int col)
{
	wxCHECK_MSG((row >= 0 && row < GetNumberRows()) &&
		(col >= 0 && col < GetNumberCols()),
		wxEmptyString,
		wxT("invalid row or column index in wxGridExtStringTable"));

	return m_data[row][col];
}

void wxGridExtStringTable::SetValue(int row, int col, const wxString& value)
{
	wxCHECK_RET((row >= 0 && row < GetNumberRows()) &&
		(col >= 0 && col < GetNumberCols()),
		wxT("invalid row or column index in wxGridExtStringTable"));

	m_data[row][col] = value;
}

void wxGridExtStringTable::Clear()
{
	int numRows;
	numRows = m_data.GetCount();
	if (numRows > 0)
	{
		int numCols;
		numCols = m_data[0].GetCount();

		int row;
		for (row = 0; row < numRows; row++)
		{
			int col;
			for (col = 0; col < numCols; col++)
			{
				m_data[row][col].clear();
			}
		}
	}
}

bool wxGridExtStringTable::InsertRows(size_t pos, size_t numRows)
{
	if (pos >= m_data.size())
	{
		return AppendRows(numRows);
	}

	wxArrayString sa;
	sa.Alloc(m_numCols);
	sa.Add(wxEmptyString, m_numCols);
	m_data.Insert(sa, pos, numRows);

	if (GetView())
	{
		wxGridExtTableMessage msg(this,
			wxGRIDTABLE_NOTIFY_ROWS_INSERTED,
			pos,
			numRows);

		GetView()->ProcessTableMessage(msg);
	}

	return true;
}

bool wxGridExtStringTable::AppendRows(size_t numRows)
{
	wxArrayString sa;
	if (m_numCols > 0)
	{
		sa.Alloc(m_numCols);
		sa.Add(wxEmptyString, m_numCols);
	}

	m_data.Add(sa, numRows);

	if (GetView())
	{
		wxGridExtTableMessage msg(this,
			wxGRIDTABLE_NOTIFY_ROWS_APPENDED,
			numRows);

		GetView()->ProcessTableMessage(msg);
	}

	return true;
}

bool wxGridExtStringTable::DeleteRows(size_t pos, size_t numRows)
{
	size_t curNumRows = m_data.GetCount();

	if (pos >= curNumRows)
	{
		wxFAIL_MSG(wxString::Format
		(
			wxT("Called wxGridExtStringTable::DeleteRows(pos=%lu, N=%lu)\nPos value is invalid for present table with %lu rows"),
			(unsigned long)pos,
			(unsigned long)numRows,
			(unsigned long)curNumRows
		));

		return false;
	}

	if (numRows > curNumRows - pos)
	{
		numRows = curNumRows - pos;
	}

	if (numRows >= curNumRows)
	{
		m_data.Clear();
	}
	else
	{
		m_data.RemoveAt(pos, numRows);
	}

	if (GetView())
	{
		wxGridExtTableMessage msg(this,
			wxGRIDTABLE_NOTIFY_ROWS_DELETED,
			pos,
			numRows);

		GetView()->ProcessTableMessage(msg);
	}

	return true;
}

bool wxGridExtStringTable::InsertCols(size_t pos, size_t numCols)
{
	if (pos >= static_cast<size_t>(m_numCols))
	{
		return AppendCols(numCols);
	}

	if (!m_colLabels.IsEmpty())
	{
		m_colLabels.Insert(wxEmptyString, pos, numCols);

		for (size_t i = pos; i < pos + numCols; i++)
			m_colLabels[i] = wxGridExtTableBase::GetColLabelValue(i);
	}

	for (size_t row = 0; row < m_data.size(); row++)
	{
		for (size_t col = pos; col < pos + numCols; col++)
		{
			m_data[row].Insert(wxEmptyString, col);
		}
	}

	m_numCols += numCols;

	if (GetView())
	{
		wxGridExtTableMessage msg(this,
			wxGRIDTABLE_NOTIFY_COLS_INSERTED,
			pos,
			numCols);

		GetView()->ProcessTableMessage(msg);
	}

	return true;
}

bool wxGridExtStringTable::AppendCols(size_t numCols)
{
	for (size_t row = 0; row < m_data.size(); row++)
	{
		m_data[row].Add(wxEmptyString, numCols);
	}

	m_numCols += numCols;

	if (GetView())
	{
		wxGridExtTableMessage msg(this,
			wxGRIDTABLE_NOTIFY_COLS_APPENDED,
			numCols);

		GetView()->ProcessTableMessage(msg);
	}

	return true;
}

bool wxGridExtStringTable::DeleteCols(size_t pos, size_t numCols)
{
	size_t row;

	size_t curNumRows = m_data.GetCount();
	size_t curNumCols = m_numCols;

	if (pos >= curNumCols)
	{
		wxFAIL_MSG(wxString::Format
		(
			wxT("Called wxGridExtStringTable::DeleteCols(pos=%lu, N=%lu)\nPos value is invalid for present table with %lu cols"),
			(unsigned long)pos,
			(unsigned long)numCols,
			(unsigned long)curNumCols
		));
		return false;
	}

	int colID;
	if (GetView())
		colID = GetView()->GetColAt(pos);
	else
		colID = pos;

	if (numCols > curNumCols - colID)
	{
		numCols = curNumCols - colID;
	}

	if (!m_colLabels.IsEmpty())
	{
		// m_colLabels stores just as many elements as it needs, e.g. if only
		// the label of the first column had been set it would have only one
		// element and not numCols, so account for it
		int numRemaining = m_colLabels.size() - colID;
		if (numRemaining > 0)
			m_colLabels.RemoveAt(colID, wxMin(numCols, numRemaining));
	}

	if (numCols >= curNumCols)
	{
		for (row = 0; row < curNumRows; row++)
		{
			m_data[row].Clear();
		}

		m_numCols = 0;
	}
	else // something will be left
	{
		for (row = 0; row < curNumRows; row++)
		{
			m_data[row].RemoveAt(colID, numCols);
		}

		m_numCols -= numCols;
	}

	if (GetView())
	{
		wxGridExtTableMessage msg(this,
			wxGRIDTABLE_NOTIFY_COLS_DELETED,
			pos,
			numCols);

		GetView()->ProcessTableMessage(msg);
	}

	return true;
}

wxString wxGridExtStringTable::GetRowLabelValue(int row)
{
	if (row > (int)(m_rowLabels.GetCount()) - 1)
	{
		// using default label
		//
		return wxGridExtTableBase::GetRowLabelValue(row);
	}
	else
	{
		return m_rowLabels[row];
	}
}

wxString wxGridExtStringTable::GetColLabelValue(int col)
{
	if (col > (int)(m_colLabels.GetCount()) - 1)
	{
		// using default label
		//
		return wxGridExtTableBase::GetColLabelValue(col);
	}
	else
	{
		return m_colLabels[col];
	}
}

void wxGridExtStringTable::SetRowLabelValue(int row, const wxString& value)
{
	if (row > (int)(m_rowLabels.GetCount()) - 1)
	{
		int n = m_rowLabels.GetCount();
		int i;

		for (i = n; i <= row; i++)
		{
			m_rowLabels.Add(wxGridExtTableBase::GetRowLabelValue(i));
		}
	}

	m_rowLabels[row] = value;
}

void wxGridExtStringTable::SetColLabelValue(int col, const wxString& value)
{
	if (col > (int)(m_colLabels.GetCount()) - 1)
	{
		int n = m_colLabels.GetCount();
		int i;

		for (i = n; i <= col; i++)
		{
			m_colLabels.Add(wxGridExtTableBase::GetColLabelValue(i));
		}
	}

	m_colLabels[col] = value;
}

void wxGridExtStringTable::SetCornerLabelValue(const wxString& value)
{
	m_cornerLabel = value;
}

wxString wxGridExtStringTable::GetCornerLabelValue() const
{
	return m_cornerLabel;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

wxBEGIN_EVENT_TABLE(wxGridExtSubwindow, wxWindow)
EVT_MOUSE_CAPTURE_LOST(wxGridExtSubwindow::OnMouseCaptureLost)
wxEND_EVENT_TABLE()

void wxGridExtSubwindow::OnMouseCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(event))
{
	m_owner->CancelMouseCapture();
}

wxBEGIN_EVENT_TABLE(wxGridExtRowAreaWindow, wxGridExtSubwindow)
EVT_PAINT(wxGridExtRowAreaWindow::OnPaint)
EVT_MOUSEWHEEL(wxGridExtRowAreaWindow::OnMouseWheel)
EVT_MOUSE_EVENTS(wxGridExtRowAreaWindow::OnMouseEvent)
wxEND_EVENT_TABLE()

void wxGridExtRowAreaWindow::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	wxPaintDC dc(this);

	// NO - don't do this because it will set both the x and y origin
	// coords to match the parent scrolled window and we just want to
	// set the y coord  - MB
	//
	// m_owner->PrepareDC( dc );

	int x, y;
	wxGridExtWindow* gridWindow = IsFrozen() ? m_owner->m_frozenRowGridWin :
		m_owner->m_gridWin;
	m_owner->GetGridWindowOffset(gridWindow, x, y);
	m_owner->CalcGridWindowUnscrolledPosition(x, y, &x, &y, gridWindow);
	wxPoint pt = dc.GetDeviceOrigin();
	dc.SetDeviceOrigin(pt.x, pt.y - y);

	wxArrayInt rows = m_owner->CalcRowLabelsExposed(GetUpdateRegion(), gridWindow);
	m_owner->DrawRowAreas(dc, rows);
}

void wxGridExtRowAreaWindow::OnMouseEvent(wxMouseEvent& event)
{
	m_owner->ProcessRowColAreaMouseEvent(wxGridExtRowOperations(), event, this);
}

void wxGridExtRowAreaWindow::OnMouseWheel(wxMouseEvent& event)
{
	if (!m_owner->ProcessWindowEvent(event))
		event.Skip();
}

//////////////////////////////////////////////////////////////////////

wxBEGIN_EVENT_TABLE(wxGridExtRowLabelWindow, wxGridExtSubwindow)
EVT_PAINT(wxGridExtRowLabelWindow::OnPaint)
EVT_MOUSEWHEEL(wxGridExtRowLabelWindow::OnMouseWheel)
EVT_MOUSE_EVENTS(wxGridExtRowLabelWindow::OnMouseEvent)
wxEND_EVENT_TABLE()

void wxGridExtRowLabelWindow::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	wxPaintDC dc(this);

	// NO - don't do this because it will set both the x and y origin
	// coords to match the parent scrolled window and we just want to
	// set the y coord  - MB
	//
	// m_owner->PrepareDC( dc );

	int x, y;
	wxGridExtWindow* gridWindow = IsFrozen() ? m_owner->m_frozenRowGridWin :
		m_owner->m_gridWin;
	m_owner->GetGridWindowOffset(gridWindow, x, y);
	m_owner->CalcGridWindowUnscrolledPosition(x, y, &x, &y, gridWindow);
	wxPoint pt = dc.GetDeviceOrigin();
	dc.SetDeviceOrigin(pt.x, pt.y - y);

	wxArrayInt rows = m_owner->CalcRowLabelsExposed(GetUpdateRegion(), gridWindow);
	m_owner->DrawRowLabels(dc, rows);

	if (IsFrozen())
		m_owner->DrawLabelFrozenBorder(dc, this, true);
}

void wxGridExtRowLabelWindow::OnMouseEvent(wxMouseEvent& event)
{
	m_owner->ProcessRowColLabelMouseEvent(wxGridExtRowOperations(), event, this);
}

void wxGridExtRowLabelWindow::OnMouseWheel(wxMouseEvent& event)
{
	if (!m_owner->ProcessWindowEvent(event))
		event.Skip();
}

//////////////////////////////////////////////////////////////////////

wxBEGIN_EVENT_TABLE(wxGridExtColAreaWindow, wxGridExtSubwindow)
EVT_PAINT(wxGridExtColAreaWindow::OnPaint)
EVT_MOUSEWHEEL(wxGridExtColAreaWindow::OnMouseWheel)
EVT_MOUSE_EVENTS(wxGridExtColAreaWindow::OnMouseEvent)
wxEND_EVENT_TABLE()

void wxGridExtColAreaWindow::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	wxPaintDC dc(this);

	// NO - don't do this because it will set both the x and y origin
	// coords to match the parent scrolled window and we just want to
	// set the x coord  - MB
	//
	// m_owner->PrepareDC( dc );

	// Column indices become invalid when the grid is empty, so avoid doing
	// anything at all in this case.
	if (m_owner->GetNumberCols() == 0)
		return;

	int x, y;
	wxGridExtWindow* gridWindow = IsFrozen() ? m_owner->m_frozenColGridWin :
		m_owner->m_gridWin;
	m_owner->GetGridWindowOffset(gridWindow, x, y);
	m_owner->CalcGridWindowUnscrolledPosition(x, y, &x, &y, gridWindow);
	wxPoint pt = dc.GetDeviceOrigin();
	dc.SetDeviceOrigin(pt.x - x, pt.y);

	wxArrayInt cols = m_owner->CalcColLabelsExposed(GetUpdateRegion(), gridWindow);
	m_owner->DrawColAreas(dc, cols);

	if (IsFrozen())
		m_owner->DrawLabelFrozenBorder(dc, this, false);
}

void wxGridExtColAreaWindow::OnMouseEvent(wxMouseEvent& event)
{
	m_owner->ProcessRowColAreaMouseEvent(wxGridExtColumnOperations(), event, this);
}

void wxGridExtColAreaWindow::OnMouseWheel(wxMouseEvent& event)
{
	if (!m_owner->ProcessWindowEvent(event))
		event.Skip();
}

wxBEGIN_EVENT_TABLE(wxGridExtColLabelWindow, wxGridExtSubwindow)
EVT_PAINT(wxGridExtColLabelWindow::OnPaint)
EVT_MOUSEWHEEL(wxGridExtColLabelWindow::OnMouseWheel)
EVT_MOUSE_EVENTS(wxGridExtColLabelWindow::OnMouseEvent)
wxEND_EVENT_TABLE()

void wxGridExtColLabelWindow::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	wxPaintDC dc(this);

	// NO - don't do this because it will set both the x and y origin
	// coords to match the parent scrolled window and we just want to
	// set the x coord  - MB
	//
	// m_owner->PrepareDC( dc );

	// Column indices become invalid when the grid is empty, so avoid doing
	// anything at all in this case.
	if (m_owner->GetNumberCols() == 0)
		return;

	int x, y;
	wxGridExtWindow* gridWindow = IsFrozen() ? m_owner->m_frozenColGridWin :
		m_owner->m_gridWin;
	m_owner->GetGridWindowOffset(gridWindow, x, y);
	m_owner->CalcGridWindowUnscrolledPosition(x, y, &x, &y, gridWindow);
	wxPoint pt = dc.GetDeviceOrigin();
	dc.SetDeviceOrigin(pt.x - x, pt.y);

	wxArrayInt cols = m_owner->CalcColLabelsExposed(GetUpdateRegion(), gridWindow);
	m_owner->DrawColLabels(dc, cols);

	if (IsFrozen())
		m_owner->DrawLabelFrozenBorder(dc, this, false);
}

void wxGridExtColLabelWindow::OnMouseEvent(wxMouseEvent& event)
{
	m_owner->ProcessRowColLabelMouseEvent(wxGridExtColumnOperations(), event, this);
}

void wxGridExtColLabelWindow::OnMouseWheel(wxMouseEvent& event)
{
	if (!m_owner->ProcessWindowEvent(event))
		event.Skip();
}

//////////////////////////////////////////////////////////////////////

wxBEGIN_EVENT_TABLE(wxGridExtCornerLabelWindow, wxGridExtSubwindow)
EVT_MOUSEWHEEL(wxGridExtCornerLabelWindow::OnMouseWheel)
EVT_MOUSE_EVENTS(wxGridExtCornerLabelWindow::OnMouseEvent)
EVT_PAINT(wxGridExtCornerLabelWindow::OnPaint)
wxEND_EVENT_TABLE()

void wxGridExtCornerLabelWindow::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	wxPaintDC dc(this);

	m_owner->DrawCornerLabel(dc);
}

void wxGridExtCornerLabelWindow::OnMouseEvent(wxMouseEvent& event)
{
	m_owner->ProcessCornerLabelMouseEvent(event);
}

void wxGridExtCornerLabelWindow::OnMouseWheel(wxMouseEvent& event)
{
	if (!m_owner->ProcessWindowEvent(event))
		event.Skip();
}

//////////////////////////////////////////////////////////////////////

wxBEGIN_EVENT_TABLE(wxGridExtWindow, wxGridExtSubwindow)
EVT_PAINT(wxGridExtWindow::OnPaint)
EVT_MOUSEWHEEL(wxGridExtWindow::OnMouseWheel)
EVT_MOUSE_EVENTS(wxGridExtWindow::OnMouseEvent)
EVT_KEY_DOWN(wxGridExtWindow::OnKeyDown)
EVT_KEY_UP(wxGridExtWindow::OnKeyUp)
EVT_CHAR(wxGridExtWindow::OnChar)
EVT_SET_FOCUS(wxGridExtWindow::OnFocus)
EVT_KILL_FOCUS(wxGridExtWindow::OnFocus)
wxEND_EVENT_TABLE()

void wxGridExtWindow::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	wxAutoBufferedPaintDC dc(this);
	m_owner->PrepareDCFor(dc, this);
	wxRegion reg = GetUpdateRegion();

	wxGridExtCellCoordsArray dirtyCells = m_owner->CalcCellsExposed(reg, this);

	m_owner->DrawGridSpace(dc, this);
	m_owner->DrawGridCellArea(dc, dirtyCells);
	m_owner->DrawAllGridWindowLines(dc, reg, this);

	if (m_type != wxGridExtWindow::wxGridExtWindowNormal)
		m_owner->DrawFrozenBorder(dc, this);

	m_owner->DrawBorder(dc, dirtyCells);
	m_owner->DrawHighlight(dc, dirtyCells);
}

void wxGridExt::Render(wxDC& dc,
	const wxPoint& position,
	const wxSize& size,
	const wxGridExtCellCoords& topLeft,
	const wxGridExtCellCoords& bottomRight,
	int style)
{
	wxCHECK_RET(bottomRight.GetCol() < GetNumberCols(),
		"Invalid right column");
	wxCHECK_RET(bottomRight.GetRow() < GetNumberRows(),
		"Invalid bottom row");

	// store user settings and reset later

	// remove grid selection, don't paint selection colour
	// unless we have wxGRID_DRAW_SELECTION
	wxGridExtSelection* selectionOrig = NULL;
	if (m_selection && !(style & wxGRID_DRAW_SELECTION))
	{
		// remove the selection temporarily, it will be restored below
		selectionOrig = m_selection;
		m_selection = NULL;
	}

	// store user device origin
	wxCoord userOriginX, userOriginY;
	dc.GetDeviceOrigin(&userOriginX, &userOriginY);

	// store user scale
	double scaleUserX, scaleUserY;
	dc.GetUserScale(&scaleUserX, &scaleUserY);

	// set defaults if necessary
	wxGridExtCellCoords leftTop(topLeft), rightBottom(bottomRight);
	if (leftTop.GetCol() < 0)
		leftTop.SetCol(0);
	if (leftTop.GetRow() < 0)
		leftTop.SetRow(0);
	if (rightBottom.GetCol() < 0)
		rightBottom.SetCol(GetNumberCols() - 1);
	if (rightBottom.GetRow() < 0)
		rightBottom.SetRow(GetNumberRows() - 1);

	// get grid offset, size and cell parameters
	wxPoint pointOffSet;
	wxSize sizeGrid;
	wxGridExtCellCoordsArray renderCells;
	wxArrayInt arrayCols;
	wxArrayInt arrayRows;

	GetRenderSizes(leftTop, rightBottom,
		pointOffSet, sizeGrid,
		renderCells,
		arrayCols, arrayRows);

	// add headers/labels to dimensions
	if (style & wxGRID_DRAW_ROWS_HEADER)
		sizeGrid.x += GetRowLabelSize();
	if (style & wxGRID_DRAW_COLS_HEADER)
		sizeGrid.y += GetColLabelSize();

	// get render start position in logical units
	wxPoint positionRender = GetRenderPosition(dc, position);

	wxCoord originX = dc.LogicalToDeviceX(positionRender.x);
	wxCoord originY = dc.LogicalToDeviceY(positionRender.y);

	dc.SetDeviceOrigin(originX, originY);

	SetRenderScale(dc, positionRender, size, sizeGrid);

	// draw row headers at specified origin
	if (GetRowLabelSize() > 0 && (style & wxGRID_DRAW_ROWS_HEADER))
	{
		if (style & wxGRID_DRAW_COLS_HEADER)
		{
			DrawCornerLabel(dc); // do only if both col and row labels drawn
			originY += dc.LogicalToDeviceYRel(GetColLabelSize());
		}

		originY -= dc.LogicalToDeviceYRel(pointOffSet.y);
		dc.SetDeviceOrigin(originX, originY);

		DrawRowLabels(dc, arrayRows);

		// reset for columns
		if (style & wxGRID_DRAW_COLS_HEADER)
			originY -= dc.LogicalToDeviceYRel(GetColLabelSize());

		originY += dc.LogicalToDeviceYRel(pointOffSet.y);
		// X offset so we don't overwrite row labels
		originX += dc.LogicalToDeviceXRel(GetRowLabelSize());
	}

	// subtract col offset where startcol > 0
	originX -= dc.LogicalToDeviceXRel(pointOffSet.x);
	// no y offset for col labels, they are at the Y origin

	// draw column labels
	if (style & wxGRID_DRAW_COLS_HEADER)
	{
		dc.SetDeviceOrigin(originX, originY);
		DrawColLabels(dc, arrayCols);
		// don't overwrite the labels, increment originY
		originY += dc.LogicalToDeviceYRel(GetColLabelSize());
	}

	// set device origin to draw grid cells and lines
	originY -= dc.LogicalToDeviceYRel(pointOffSet.y);
	dc.SetDeviceOrigin(originX, originY);

	// draw cell area background
	dc.SetBrush(GetDefaultCellBackgroundColour());
	dc.SetPen(*wxTRANSPARENT_PEN);
	// subtract headers from grid area dimensions
	wxSize sizeCells(sizeGrid);
	if (style & wxGRID_DRAW_ROWS_HEADER)
		sizeCells.x -= GetRowLabelSize();
	if (style & wxGRID_DRAW_COLS_HEADER)
		sizeCells.y -= GetColLabelSize();

	dc.DrawRectangle(pointOffSet, sizeCells);

	// draw cells
	{
		wxDCClipper clipper(dc, wxRect(pointOffSet, sizeCells));
		DrawGridCellArea(dc, renderCells);
	}

	// draw grid lines
	if (style & wxGRID_DRAW_CELL_LINES)
	{
		wxRegion regionClip(pointOffSet.x, pointOffSet.y,
			sizeCells.x, sizeCells.y);

		DrawRangeGridLines(dc, regionClip, renderCells[0], renderCells.Last());
	}

	// draw render rectangle bounding lines
	DoRenderBox(dc, style,
		pointOffSet, sizeCells,
		leftTop, rightBottom);

	// restore user setings
	dc.SetDeviceOrigin(userOriginX, userOriginY);
	dc.SetUserScale(scaleUserX, scaleUserY);

	if (selectionOrig)
	{
		m_selection = selectionOrig;
	}
}

void
wxGridExt::SetRenderScale(wxDC& dc,
	const wxPoint& pos, const wxSize& size,
	const wxSize& sizeGrid)
{
	double scaleX, scaleY;
	wxSize sizeTemp;

	if (size.GetWidth() != wxDefaultSize.GetWidth()) // size.x was specified
		sizeTemp.SetWidth(size.GetWidth());
	else
		sizeTemp.SetWidth(dc.DeviceToLogicalXRel(dc.GetSize().GetWidth())
			- pos.x);

	if (size.GetHeight() != wxDefaultSize.GetHeight()) // size.y was specified
		sizeTemp.SetHeight(size.GetHeight());
	else
		sizeTemp.SetHeight(dc.DeviceToLogicalYRel(dc.GetSize().GetHeight())
			- pos.y);

	scaleX = (double)((double)sizeTemp.GetWidth() / (double)sizeGrid.GetWidth());
	scaleY = (double)((double)sizeTemp.GetHeight() / (double)sizeGrid.GetHeight());

	dc.SetUserScale(wxMin(scaleX, scaleY), wxMin(scaleX, scaleY));
}

// get grid rendered size, origin offset and fill cell arrays
void wxGridExt::GetRenderSizes(const wxGridExtCellCoords& topLeft,
	const wxGridExtCellCoords& bottomRight,
	wxPoint& pointOffSet, wxSize& sizeGrid,
	wxGridExtCellCoordsArray& renderCells,
	wxArrayInt& arrayCols, wxArrayInt& arrayRows) const
{
	pointOffSet.x = 0;
	pointOffSet.y = 0;
	sizeGrid.SetWidth(0);
	sizeGrid.SetHeight(0);

	int col, row;

	wxGridExtSizesInfo sizeinfo = GetColSizes();
	for (col = 0; col <= bottomRight.GetCol(); col++)
	{
		if (col < topLeft.GetCol())
		{
			pointOffSet.x += sizeinfo.GetSize(col);
		}
		else
		{
			for (row = topLeft.GetRow(); row <= bottomRight.GetRow(); row++)
			{
				renderCells.Add(wxGridExtCellCoords(row, col));
				arrayRows.Add(row); // column labels rendered in DrawColLabels
			}
			arrayCols.Add(col); // row labels rendered in DrawRowLabels
			sizeGrid.x += sizeinfo.GetSize(col);
		}
	}

	sizeinfo = GetRowSizes();
	for (row = 0; row <= bottomRight.GetRow(); row++)
	{
		if (row < topLeft.GetRow())
			pointOffSet.y += sizeinfo.GetSize(row);
		else
			sizeGrid.y += sizeinfo.GetSize(row);
	}
}

// get render start position
// if position not specified use dc draw extents MaxX and MaxY
wxPoint wxGridExt::GetRenderPosition(wxDC& dc, const wxPoint& position)
{
	wxPoint positionRender(position);

	if (!positionRender.IsFullySpecified())
	{
		if (positionRender.x == wxDefaultPosition.x)
			positionRender.x = dc.MaxX();

		if (positionRender.y == wxDefaultPosition.y)
			positionRender.y = dc.MaxY();
	}

	return positionRender;
}

// draw render rectangle bounding lines
// useful where there is multi cell row or col clipping and no cell border
void wxGridExt::DoRenderBox(wxDC& dc, const int& style,
	const wxPoint& pointOffSet,
	const wxSize& sizeCells,
	const wxGridExtCellCoords& topLeft,
	const wxGridExtCellCoords& bottomRight)
{
	if (!(style & wxGRID_DRAW_BOX_RECT))
		return;

	int bottom = pointOffSet.y + sizeCells.GetY(),
		right = pointOffSet.x + sizeCells.GetX() - 1;

	// horiz top line if we are not drawing column header/labels
	if (!(style & wxGRID_DRAW_COLS_HEADER))
	{
		int left = pointOffSet.x;
		left += (style & wxGRID_DRAW_COLS_HEADER)
			? -GetRowLabelSize() : 0;
		dc.SetPen(GetRowGridLinePen(topLeft.GetRow()));
		dc.DrawLine(left,
			pointOffSet.y,
			right,
			pointOffSet.y);
	}

	// horiz bottom line
	dc.SetPen(GetRowGridLinePen(bottomRight.GetRow()));
	dc.DrawLine(pointOffSet.x, bottom - 1, right, bottom - 1);

	// left vertical line if we are not drawing row header/labels
	if (!(style & wxGRID_DRAW_ROWS_HEADER))
	{
		int top = pointOffSet.y;
		top += (style & wxGRID_DRAW_COLS_HEADER)
			? -GetColLabelSize() : 0;
		dc.SetPen(GetColGridLinePen(topLeft.GetCol()));
		dc.DrawLine(pointOffSet.x - 1,
			top,
			pointOffSet.x - 1,
			bottom - 1);
	}

	// right vertical line
	dc.SetPen(GetColGridLinePen(bottomRight.GetCol()));
	dc.DrawLine(right, pointOffSet.y, right, bottom - 1);
}

void wxGridExtWindow::ScrollWindow(int dx, int dy, const wxRect* rect)
{
	m_owner->ScrollWindow(dx, dy, rect);
}

void wxGridExt::ScrollWindow(int dx, int dy, const wxRect* rect)
{
	// We must explicitly call wxWindow version to avoid infinite recursion as
	// wxGridExtWindow::ScrollWindow() calls this method back.
	m_gridWin->wxWindow::ScrollWindow(dx, dy, rect);

	if (m_frozenColGridWin)
		m_frozenColGridWin->wxWindow::ScrollWindow(0, dy, rect);
	if (m_frozenRowGridWin)
		m_frozenRowGridWin->wxWindow::ScrollWindow(dx, 0, rect);

	m_rowAreaWin->ScrollWindow(0, dy, rect);
	m_rowLabelWin->ScrollWindow(0, dy, rect);
	m_colAreaWin->ScrollWindow(dx, 0, rect);
	m_colLabelWin->ScrollWindow(dx, 0, rect);
}

void wxGridExtWindow::OnMouseEvent(wxMouseEvent& event)
{
	if (event.ButtonDown(wxMOUSE_BTN_LEFT) && FindFocus() != this)
		SetFocus();

	m_owner->ProcessGridCellMouseEvent(event, this);
}

void wxGridExtWindow::OnMouseWheel(wxMouseEvent& event)
{
	if (!m_owner->ProcessWindowEvent(event))
		event.Skip();
}

// This seems to be required for wxMotif/wxGTK otherwise the mouse
// cursor must be in the cell edit control to get key events
//
void wxGridExtWindow::OnKeyDown(wxKeyEvent& event)
{
	if (!m_owner->ProcessWindowEvent(event))
		event.Skip();
}

void wxGridExtWindow::OnKeyUp(wxKeyEvent& event)
{
	if (!m_owner->ProcessWindowEvent(event))
		event.Skip();
}

void wxGridExtWindow::OnChar(wxKeyEvent& event)
{
	if (!m_owner->ProcessWindowEvent(event))
		event.Skip();
}

void wxGridExtWindow::OnFocus(wxFocusEvent& event)
{
	// and if we have any selection, it has to be repainted, because it
	// uses different colour when the grid is not focused:
	if (m_owner->IsSelection())
	{
		Refresh();
	}
	else
	{
		// NB: Note that this code is in "else" branch only because the other
		//     branch refreshes everything and so there's no point in calling
		//     Refresh() again, *not* because it should only be done if
		//     !IsSelection(). If the above code is ever optimized to refresh
		//     only selected area, this needs to be moved out of the "else"
		//     branch so that it's always executed.

		// current cell cursor {dis,re}appears on focus change:
		const wxGridExtCellCoords cursorCoords(m_owner->GetGridCursorRow(),
			m_owner->GetGridCursorCol());
		const wxRect cursor =
			m_owner->BlockToDeviceRect(cursorCoords, cursorCoords, this);
		if (cursor != wxGridExtNoCellRect)
			Refresh(true, &cursor);
	}

	if (!m_owner->ProcessWindowEvent(event))
		event.Skip();
}

// Unlike XToCol() and YToRow() these macros always return a valid column/row,
// so their results don't need to be checked, while the results of the public
// functions always must be.
#define internalXToCol(x, gridWindowPtr) XToCol(x, true, gridWindowPtr)
#define internalYToRow(y, gridWindowPtr) YToRow(y, true, gridWindowPtr)

/////////////////////////////////////////////////////////////////////

wxBEGIN_EVENT_TABLE(wxGridExt, wxScrolledCanvas)
EVT_SIZE(wxGridExt::OnSize)
EVT_DPI_CHANGED(wxGridExt::OnDPIChanged)
EVT_KEY_DOWN(wxGridExt::OnKeyDown)
EVT_CHAR(wxGridExt::OnChar)
wxEND_EVENT_TABLE()

bool wxGridExt::Create(wxWindow* parent, wxWindowID id,
	const wxPoint& pos, const wxSize& size,
	long style, const wxString& name)
{
	if (!wxScrolledCanvas::Create(parent, id, pos, size,
		style | wxWANTS_CHARS, name))
		return false;

	m_colMinWidths = wxLongToLongHashMap(GRID_HASH_SIZE);
	m_rowMinHeights = wxLongToLongHashMap(GRID_HASH_SIZE);

	Create();
	SetInitialSize(size);
	CalcDimensions();

	return true;
}

wxGridExt::~wxGridExt()
{
	if (m_winCapture)
		m_winCapture->ReleaseMouse();

	// Ensure that the editor control is destroyed before the grid is,
	// otherwise we crash later when the editor tries to do something with the
	// half destroyed grid
	HideCellEditControl();

	// Must do this or ~wxScrollHelper will pop the wrong event handler
	SetTargetWindow(this);
	ClearAttrCache();
	wxSafeDecRef(m_defaultCellAttr);

#ifdef DEBUG_ATTR_CACHE
	size_t total = gs_nAttrCacheHits + gs_nAttrCacheMisses;
	wxPrintf(wxT("wxGridExt attribute cache statistics: "
		"total: %u, hits: %u (%u%%)\n"),
		total, gs_nAttrCacheHits,
		total ? (gs_nAttrCacheHits * 100) / total : 0);
#endif

	// if we own the table, just delete it, otherwise at least don't leave it
	// with dangling view pointer
	if (m_ownTable)
		delete m_table;
	else if (m_table && m_table->GetView() == this)
		m_table->SetView(NULL);

	delete m_typeRegistry;
	delete m_selection;

	delete m_setFixedRows;
	delete m_setFixedCols;
}

//
// ----- internal init and update functions
//

// NOTE: If using the default visual attributes works everywhere then this can
// be removed as well as the #else cases below.
#define _USE_VISATTR 0

void wxGridExt::Create()
{
	// create the type registry
	m_typeRegistry = new wxGridExtTypeRegistry;

	m_cellEditCtrlEnabled = false;

	m_defaultCellAttr = new wxGridExtCellAttr();

	// Set default cell attributes
	m_defaultCellAttr->SetDefAttr(m_defaultCellAttr);
	m_defaultCellAttr->SetKind(wxGridExtCellAttr::Default);
	m_defaultCellAttr->SetFont(GetFont());
	m_defaultCellAttr->SetAlignment(wxALIGN_LEFT, wxALIGN_TOP);
	m_defaultCellAttr->SetRenderer(new wxGridExtCellStringRenderer);
	m_defaultCellAttr->SetEditor(new wxGridExtCellTextEditor);
	m_defaultCellAttr->SetFitMode(wxGridExtFitMode::Overflow());

#if _USE_VISATTR
	wxVisualAttributes gva = wxListBox::GetClassDefaultAttributes();
	wxVisualAttributes lva = wxPanel::GetClassDefaultAttributes();

	m_defaultCellAttr->SetTextColour(gva.colFg);
	m_defaultCellAttr->SetBackgroundColour(gva.colBg);

#else
	m_defaultCellAttr->SetTextColour(
		wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
	m_defaultCellAttr->SetBackgroundColour(
		wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
#endif

	m_defaultCellAttr->SetTextOrient(wxHORIZONTAL);

	m_numRows = 0;
	m_numCols = 0;
	m_numFrozenRows = 0;
	m_numFrozenCols = 0;
	m_currentCellCoords = wxGridExtNoCellCoords;

	// subwindow components that make up the wxGridExt
#pragma region area
	m_rowAreaWin = new wxGridExtRowAreaWindow(this);
#pragma endregion 
	m_rowLabelWin = new wxGridExtRowLabelWindow(this);

	CreateColumnWindow();
	m_cornerLabelWin = new wxGridExtCornerLabelWindow(this);
	m_gridWin = new wxGridExtWindow(this, wxGridExtWindow::wxGridExtWindowNormal);

	SetTargetWindow(m_gridWin);

#if _USE_VISATTR
	wxColour gfg = gva.colFg;
	wxColour gbg = gva.colBg;
	wxColour lfg = lva.colFg;
	wxColour lbg = lva.colBg;
#else
	wxColour gfg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	wxColour gbg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
	wxColour lfg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	wxColour lbg = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
#endif

	m_cornerLabelWin->SetOwnForegroundColour(lfg);
	m_cornerLabelWin->SetOwnBackgroundColour(lbg);

	m_rowAreaWin->SetOwnForegroundColour(lfg);
	m_rowAreaWin->SetOwnBackgroundColour(lbg);
	m_rowLabelWin->SetOwnForegroundColour(lfg);
	m_rowLabelWin->SetOwnBackgroundColour(lbg);
	m_colAreaWin->SetOwnForegroundColour(lfg);
	m_colAreaWin->SetOwnBackgroundColour(lbg);
	m_colLabelWin->SetOwnForegroundColour(lfg);
	m_colLabelWin->SetOwnBackgroundColour(lbg);

	m_gridWin->SetOwnForegroundColour(gfg);
	m_gridWin->SetOwnBackgroundColour(gbg);

	m_labelBackgroundColour = m_rowLabelWin->GetBackgroundColour();
	m_labelTextColour = m_rowLabelWin->GetForegroundColour();

	InitPixelFields();
}

void wxGridExt::InitPixelFields()
{
	m_defaultRowHeight = m_gridWin->GetCharHeight();
#if defined(__WXMOTIF__) || defined(__WXGTK__) || defined(__WXQT__)  // see also text ctrl sizing in ShowCellEditControl()
	m_defaultRowHeight += 8;
#else
	m_defaultRowHeight += 4;
#endif

	// Don't change the value when called from OnDPIChanged() later if the
	// corresponding label window is hidden, these values should remain zeroes
	// then.

	if (m_rowAreaWin->IsShown())
		m_rowAreaWidth = FromDIP(WXGRID_DEFAULT_ROW_LABEL_WIDTH);

	if (m_rowLabelWin->IsShown())
		m_rowLabelWidth = FromDIP(WXGRID_DEFAULT_ROW_LABEL_WIDTH);

	if (m_colAreaWin->IsShown())
		m_colAreaHeight = FromDIP(WXGRID_DEFAULT_COL_LABEL_HEIGHT);

	if (m_colLabelWin->IsShown())
		m_colLabelHeight = FromDIP(WXGRID_DEFAULT_COL_LABEL_HEIGHT);

	m_defaultColWidth = FromDIP(WXGRID_DEFAULT_COL_WIDTH);

	m_minAcceptableColWidth = FromDIP(WXGRID_MIN_COL_WIDTH);
	m_minAcceptableRowHeight = FromDIP(WXGRID_MIN_ROW_HEIGHT);
}

void wxGridExt::CreateColumnWindow()
{
	if (m_useNativeHeader)
	{
		m_colAreaWin = new wxGridExtColAreaWindow(this);
		m_colAreaHeight = m_colAreaWin->GetBestSize().y;
		m_colLabelWin = new wxGridExtHeaderCtrl(this);
		m_colLabelHeight = m_colLabelWin->GetBestSize().y;
	}
	else // draw labels ourselves
	{
		m_colAreaWin = new wxGridExtColAreaWindow(this);
		m_colAreaHeight = FromDIP(WXGRID_DEFAULT_COL_LABEL_HEIGHT);
		m_colLabelWin = new wxGridExtColLabelWindow(this);
		m_colLabelHeight = FromDIP(WXGRID_DEFAULT_COL_LABEL_HEIGHT);
	}
}

bool wxGridExt::CreateGrid(int numRows, int numCols,
	wxGridExtSelectionModes selmode)
{
	wxCHECK_MSG(!m_created,
		false,
		wxT("wxGridExt::CreateGrid or wxGridExt::SetTable called more than once"));

	return SetTable(new wxGridExtStringTable(numRows, numCols), true, selmode);
}

void wxGridExt::SetSelectionMode(wxGridExtSelectionModes selmode)
{
	wxCHECK_RET(m_created,
		wxT("Called wxGridExt::SetSelectionMode() before calling CreateGrid()"));

	m_selection->SetSelectionMode(selmode);
}

wxGridExt::wxGridExtSelectionModes wxGridExt::GetSelectionMode() const
{
	wxCHECK_MSG(m_created, wxGridExtSelectCells,
		wxT("Called wxGridExt::GetSelectionMode() before calling CreateGrid()"));

	return m_selection->GetSelectionMode();
}

bool
wxGridExt::SetTable(wxGridExtTableBase* table,
	bool takeOwnership,
	wxGridExt::wxGridExtSelectionModes selmode)
{
	if (m_created)
	{
		// stop all processing
		m_created = false;

		if (m_table)
		{
			// We can't leave the in-place control editing the data of the
			// table alive, as it would try to use the table object that we
			// don't have any more later otherwise, so hide it manually.
			//
			// Notice that we can't call DisableCellEditControl() from here
			// which would try to save the current editor value into the table
			// which might be half-deleted by now, so we have to manually mark
			// the edit control as being disabled.
			HideCellEditControl();
			m_cellEditCtrlEnabled = false;

			// Don't hold on to attributes cached from the old table
			ClearAttrCache();

			m_table->SetView(0);
			if (m_ownTable)
				delete m_table;
			m_table = NULL;
		}

		wxDELETE(m_selection);

		m_ownTable = false;
		m_numRows = 0;
		m_numCols = 0;
		m_numFrozenRows = 0;
		m_numFrozenCols = 0;

		// kill row and column size arrays
		m_colWidths.Empty();
		m_colRights.Empty();
		m_rowHeights.Empty();
		m_rowBottoms.Empty();
	}

	if (table)
	{
		m_numRows = table->GetNumberRows();
		m_numCols = table->GetNumberCols();

		m_table = table;
		m_table->SetView(this);
		m_ownTable = takeOwnership;

		// Notice that this must be called after setting m_table as it uses it
		// indirectly, via wxGridExt::GetColLabelValue().
		if (m_useNativeHeader)
			SetNativeHeaderColCount();

		m_selection = new wxGridExtSelection(this, selmode);
		CalcDimensions();

		m_created = true;
	}

	InvalidateBestSize();

	UpdateCurrentCellOnRedim();

	return m_created;
}

void wxGridExt::AssignTable(wxGridExtTableBase* table, wxGridExtSelectionModes selmode)
{
	wxCHECK_RET(table, wxS("Table pointer must be valid"));
	wxCHECK_RET(!m_created, wxS("wxGridExt already has a table"));

	SetTable(table, true /* take ownership */, selmode);
}

void wxGridExt::Init()
{
	m_created = false;

	m_cornerLabelWin = NULL;
	m_rowAreaWin = NULL;
	m_rowFrozenAreaWin = NULL;
	m_rowLabelWin = NULL;
	m_rowFrozenLabelWin = NULL;
	m_colAreaWin = NULL;
	m_colFrozenAreaWin = NULL;
	m_colLabelWin = NULL;
	m_colFrozenLabelWin = NULL;
	m_gridWin = NULL;
	m_frozenColGridWin = NULL;
	m_frozenRowGridWin = NULL;
	m_frozenCornerGridWin = NULL;

	m_table = NULL;
	m_ownTable = false;

	m_selection = NULL;
	m_defaultCellAttr = NULL;
	m_typeRegistry = NULL;

	m_setFixedRows =
		m_setFixedCols = NULL;

	// init attr cache
	m_attrCache.row = -1;
	m_attrCache.col = -1;
	m_attrCache.attr = NULL;

	m_labelFont = GetFont();
	m_labelFont.SetWeight(wxFONTWEIGHT_BOLD);

	m_rowLabelHorizAlign = wxALIGN_CENTRE;
	m_rowLabelVertAlign = wxALIGN_CENTRE;

	m_colLabelHorizAlign = wxALIGN_CENTRE;
	m_colLabelVertAlign = wxALIGN_CENTRE;
	m_colLabelTextOrientation = wxHORIZONTAL;

	m_cornerLabelHorizAlign = wxALIGN_CENTRE;
	m_cornerLabelVertAlign = wxALIGN_CENTRE;
	m_cornerLabelTextOrientation = wxHORIZONTAL;

	// All these fields require a valid window, so are initialized in Create().
	m_rowAreaWidth =
		m_rowLabelWidth = m_colAreaHeight = m_colLabelHeight = 0;

	m_defaultColWidth =
		m_defaultRowHeight = 0;

	m_minAcceptableColWidth =
		m_minAcceptableRowHeight = 0;

	m_gridLineColour = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
	m_gridLinesEnabled = true;
	m_gridLinesClipHorz =
		m_gridLinesClipVert = true;
	m_cellHighlightColour = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	m_cellHighlightPenWidth = 2;
	m_cellHighlightROPenWidth = 1;
	m_gridFrozenBorderColour = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE); //wxSystemSettings::SelectLightDark(*wxBLACK, *wxWHITE);
	m_gridFrozenBorderPenWidth = 0;

	m_canDragRowMove = false;
	m_canDragColMove = false;
	m_canHideColumns = true;

	m_cursorMode = WXGRID_CURSOR_SELECT_CELL;
	m_winCapture = NULL;
	m_canDragRowSize = true;
	m_canDragColSize = true;
	m_canDragGridSize = true;
	m_canDragCell = false;
	m_dragMoveRowOrCol = -1;
	m_dragLastPos = -1;
	m_dragLastColour = NULL;
	m_dragRowOrCol = -1;
	m_dragRowOrColOldSize = -1;
	m_isDragging = false;
	m_cancelledDragging = false;
	m_startDragPos = wxDefaultPosition;
	m_lastMousePos = wxDefaultPosition;

	m_sortCol = wxNOT_FOUND;
	m_sortIsAscending = true;

	m_useNativeHeader =
		m_nativeColumnLabels = false;

	m_waitForSlowClick = false;

	m_rowResizeCursor = wxCursor(wxCURSOR_SIZENS);
	m_colResizeCursor = wxCursor(wxCURSOR_SIZEWE);

	m_currentCellCoords = wxGridExtNoCellCoords;

	m_selectionBackground = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
	m_selectionForeground = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);

	m_editable = true;  // default for whole grid
	m_areaEnabled = true;  // default for area grid

	m_batchCount = 0;

	m_extraWidth =
		m_extraHeight = 0;

	// we can't call SetScrollRate() as the window isn't created yet but OTOH
	// we don't need to call it either as the scroll position is (0, 0) right
	// now anyhow, so just set the parameters directly
	m_xScrollPixelsPerLine = GRID_SCROLL_LINE_X;
	m_yScrollPixelsPerLine = GRID_SCROLL_LINE_Y;

	m_tabBehaviour = Tab_Stop;
}

// ----------------------------------------------------------------------------
// the idea is to call these functions only when necessary because they create
// quite big arrays which eat memory mostly unnecessary - in particular, if
// default widths/heights are used for all rows/columns, we may not use these
// arrays at all
//
// with some extra code, it should be possible to only store the widths/heights
// different from default ones (resulting in space savings for huge grids) but
// this is not done currently
// ----------------------------------------------------------------------------

void wxGridExt::InitRowHeights()
{
	m_rowHeights.Empty();
	m_rowBottoms.Empty();

	m_rowHeights.Alloc(m_numRows);
	m_rowBottoms.Alloc(m_numRows);

	m_rowHeights.Add(m_defaultRowHeight, m_numRows);

	for (int i = 0; i < m_numRows; i++)
	{
		int rowBottom = (GetRowPos(i) + 1) * m_defaultRowHeight;;
		m_rowBottoms.Add(rowBottom);
	}
}

void wxGridExt::InitColWidths()
{
	m_colWidths.Empty();
	m_colRights.Empty();

	m_colWidths.Alloc(m_numCols);
	m_colRights.Alloc(m_numCols);

	m_colWidths.Add(m_defaultColWidth, m_numCols);

	for (int i = 0; i < m_numCols; i++)
	{
		int colRight = (GetColPos(i) + 1) * m_defaultColWidth;
		m_colRights.Add(colRight);
	}
}

int wxGridExt::GetColWidth(int col) const
{
	if (m_colWidths.IsEmpty())
		return m_defaultColWidth;

	// a negative width indicates a hidden column
	return m_colWidths[col] > 0 ? m_colWidths[col] : 0;
}

int wxGridExt::GetColLeft(int col) const
{
	if (m_colRights.IsEmpty())
		return GetColPos(col) * m_defaultColWidth;

	return m_colRights[col] - GetColWidth(col);
}

int wxGridExt::GetColRight(int col) const
{
	return m_colRights.IsEmpty() ? (GetColPos(col) + 1) * m_defaultColWidth
		: m_colRights[col];
}

int wxGridExt::GetRowHeight(int row) const
{
	// no custom heights / hidden rows
	if (m_rowHeights.IsEmpty())
		return m_defaultRowHeight;

	// a negative height indicates a hidden row
	return m_rowHeights[row] > 0 ? m_rowHeights[row] : 0;
}

int wxGridExt::GetRowTop(int row) const
{
	if (m_rowBottoms.IsEmpty())
		return GetRowPos(row) * m_defaultRowHeight;

	return m_rowBottoms[row] - GetRowHeight(row);
}

int wxGridExt::GetRowBottom(int row) const
{
	return m_rowBottoms.IsEmpty() ? (GetRowPos(row) + 1) * m_defaultRowHeight
		: m_rowBottoms[row];
}

void wxGridExt::CalcDimensions()
{
	// Wait until the window is thawed if it's currently frozen.
	if (GetBatchCount())
		return;

	// if our OnSize() hadn't been called (it would if we have scrollbars), we
	// still must reposition the children
	CalcWindowSizes();

	// compute the size of the scrollable area
	int w = m_numCols > 0 ? GetColRight(GetColAt(m_numCols - 1)) : 0;
	int h = m_numRows > 0 ? GetRowBottom(m_numRows - 1) : 0;

	w += m_extraWidth;
	h += m_extraHeight;

	// take into account editor if shown
	if (IsCellEditControlShown())
	{
		const wxRect rect = GetCurrentCellEditorPtr()->GetWindow()->GetRect();
		if (rect.GetRight() > w)
			w = rect.GetRight();
		if (rect.GetBottom() > h)
			h = rect.GetBottom();
	}

	wxPoint offset = GetGridWindowOffset(m_gridWin);
	w -= offset.x;
	h -= offset.y;

	// preserve (more or less) the previous position
	int x, y;
	GetViewStart(&x, &y);

	// ensure the position is valid for the new scroll ranges
	if (x >= w)
		x = wxMax(w - 1, 0);
	if (y >= h)
		y = wxMax(h - 1, 0);

	// update the virtual size and refresh the scrollbars to reflect it
	m_gridWin->SetVirtualSize(w, h);
	Scroll(x, y);
	AdjustScrollbars();
}

wxSize wxGridExt::GetSizeAvailableForScrollTarget(const wxSize& size)
{
	wxPoint offset = GetGridWindowOffset(m_gridWin);
	wxSize sizeGridWin(size);
	sizeGridWin.x -= (GridRowAreaEnabled() ? m_rowAreaWidth : 0) + m_rowLabelWidth + offset.x;
	sizeGridWin.y -= (GridColAreaEnabled() ? m_colAreaHeight : 0) + m_colLabelHeight + offset.y;

	return sizeGridWin;
}

void wxGridExt::CalcWindowSizes()
{
	// escape if the window is has not been fully created yet
	if (m_cornerLabelWin == NULL)
		return;

	int cw, ch;
	GetClientSize(&cw, &ch);

	// frozen rows and cols windows size
	int fgw = 0, fgh = 0;

	for (int i = 0; i < m_numFrozenRows; i++)
		fgh += GetRowHeight(i);

	for (int i = 0; i < m_numFrozenCols; i++)
		fgw += GetColWidth(i);

	const int rowAreaWidth =
		(GridRowAreaEnabled() ? m_rowAreaWidth : 0);

	const int colAreaHeight =
		(GridColAreaEnabled() ? m_colAreaHeight : 0);

	// the grid may be too small to have enough space for the labels yet, don't
	// size the windows to negative sizes in this case
	int gw = cw - m_rowLabelWidth - fgw;
	int gh = ch - m_colLabelHeight - fgh;

	if (gw < 0)
		gw = 0;
	if (gh < 0)
		gh = 0;

	if (m_frozenCornerGridWin && m_frozenCornerGridWin->IsShown())
		m_frozenCornerGridWin->SetSize(rowAreaWidth + m_rowLabelWidth, colAreaHeight + m_colLabelHeight, fgw, fgh);

	if (m_cornerLabelWin && m_cornerLabelWin->IsShown())
		m_cornerLabelWin->SetSize(rowAreaWidth, colAreaHeight, m_rowLabelWidth, m_colLabelHeight);

	if (m_colAreaWin && m_colAreaWin->IsShown())
		m_colAreaWin->SetSize(rowAreaWidth + m_rowLabelWidth + fgw, 0, gw, colAreaHeight);

	if (m_colFrozenAreaWin && m_colFrozenAreaWin->IsShown())
		m_colFrozenAreaWin->SetSize(rowAreaWidth + m_rowLabelWidth, 0, rowAreaWidth + m_rowLabelWidth + fgw, colAreaHeight);

	if (m_colFrozenLabelWin && m_colFrozenLabelWin->IsShown())
		m_colFrozenLabelWin->SetSize(rowAreaWidth + m_rowLabelWidth, colAreaHeight, fgw, m_colLabelHeight);

	if (m_frozenColGridWin && m_frozenColGridWin->IsShown())
		m_frozenColGridWin->SetSize(rowAreaWidth + m_rowLabelWidth, colAreaHeight + m_colLabelHeight + fgh, fgw, gh);

	if (m_colLabelWin && m_colLabelWin->IsShown())
		m_colLabelWin->SetSize(rowAreaWidth + m_rowLabelWidth + fgw, colAreaHeight, gw, m_colLabelHeight);

	if (m_rowAreaWin && m_rowAreaWin->IsShown())
		m_rowAreaWin->SetSize(0, colAreaHeight + m_colLabelHeight + fgh, rowAreaWidth, gh);

	if (m_rowFrozenAreaWin && m_rowFrozenAreaWin->IsShown())
		m_rowFrozenAreaWin->SetSize(0, colAreaHeight + m_colLabelHeight, rowAreaWidth, fgh);

	if (m_rowFrozenLabelWin && m_rowFrozenLabelWin->IsShown())
		m_rowFrozenLabelWin->SetSize(rowAreaWidth, colAreaHeight + m_colLabelHeight, m_rowLabelWidth, fgh);

	if (m_rowLabelWin && m_rowLabelWin->IsShown())
		m_rowLabelWin->SetSize(rowAreaWidth, colAreaHeight + m_colLabelHeight + fgh, m_rowLabelWidth, gh);

	if (m_frozenRowGridWin && m_frozenRowGridWin->IsShown())
		m_frozenRowGridWin->SetSize(rowAreaWidth + m_rowLabelWidth + fgw, colAreaHeight + m_colLabelHeight, gw, fgh);

	if (m_gridWin && m_gridWin->IsShown())
		m_gridWin->SetSize(rowAreaWidth + m_rowLabelWidth + fgw, colAreaHeight + m_colLabelHeight + fgh, gw, gh);
}

// this is called when the grid table sends a message
// to indicate that it has been redimensioned
//
bool wxGridExt::Redimension(wxGridExtTableMessage& msg)
{
	int i;
	bool result = false;

	// Clear the attribute cache as the attribute might refer to a different
	// cell than stored in the cache after adding/removing rows/columns.
	ClearAttrCache();

	// By the same reasoning, the editor should be dismissed if columns are
	// added or removed. And for consistency, it should IMHO always be
	// removed, not only if the cell "underneath" it actually changes.
	// For now, I intentionally do not save the editor's content as the
	// cell it might want to save that stuff to might no longer exist.
	HideCellEditControl();

	switch (msg.GetId())
	{
	case wxGRIDTABLE_NOTIFY_ROWS_INSERTED:
	{
		size_t pos = msg.GetCommandInt();
		int numRows = msg.GetCommandInt2();
		m_numRows += numRows;

		if (!m_rowAt.IsEmpty())
		{
			//Shift the row IDs
			for (i = 0; i < m_numRows - numRows; i++)
			{
				if (m_rowAt[i] >= (int)pos)
					m_rowAt[i] += numRows;
			}

			m_rowAt.Insert(pos, pos, numRows);

			//Set the new rows' positions
			for (i = pos + 1; i < (int)pos + numRows; i++)
			{
				m_rowAt[i] = i;
			}
		}

		if (!m_rowHeights.IsEmpty())
		{
			m_rowHeights.Insert(m_defaultRowHeight, pos, numRows);
			m_rowBottoms.Insert(0, pos, numRows);

			int bottom = 0;
			if (pos > 0)
				bottom = m_rowBottoms[GetRowAt(pos - 1)];

			int rowPos;
			for (rowPos = pos; rowPos < m_numRows; rowPos++)
			{
				i = GetRowAt(rowPos);

				bottom += GetRowHeight(i);
				m_rowBottoms[i] = bottom;
			}
		}

		UpdateCurrentCellOnRedim();

		if (m_selection)
			m_selection->UpdateRows(pos, numRows);
		wxGridExtCellAttrProvider* attrProvider = m_table->GetAttrProvider();
		if (attrProvider)
			attrProvider->UpdateAttrRows(pos, numRows);

		CalcDimensions();

		if (ShouldRefresh()) {
			m_rowAreaWin->Refresh();
			m_rowLabelWin->Refresh();
		}
	}
	result = true;
	break;

	case wxGRIDTABLE_NOTIFY_ROWS_APPENDED:
	{
		int numRows = msg.GetCommandInt();
		int oldNumRows = m_numRows;
		m_numRows += numRows;

		if (!m_rowAt.IsEmpty())
		{
			m_rowAt.Add(0, numRows);

			//Set the new rows' positions
			for (i = oldNumRows; i < m_numRows; i++)
			{
				m_rowAt[i] = i;
			}
		}

		if (!m_rowHeights.IsEmpty())
		{
			m_rowHeights.Add(m_defaultRowHeight, numRows);
			m_rowBottoms.Add(0, numRows);

			int bottom = 0;
			if (oldNumRows > 0)
				bottom = m_rowBottoms[oldNumRows - 1];

			int rowPos;
			for (rowPos = oldNumRows; rowPos < m_numRows; rowPos++)
			{
				i = GetRowAt(rowPos);

				bottom += GetRowHeight(i);
				m_rowBottoms[i] = bottom;
			}
		}

		UpdateCurrentCellOnRedim();

		CalcDimensions();

		if (ShouldRefresh()) {
			m_rowAreaWin->Refresh();
			m_rowLabelWin->Refresh();
		}
	}
	result = true;
	break;

	case wxGRIDTABLE_NOTIFY_ROWS_DELETED:
	{
		size_t pos = msg.GetCommandInt();
		int numRows = msg.GetCommandInt2();
		m_numRows -= numRows;

		if (!m_rowAt.IsEmpty())
		{
			int rowID = GetRowAt(pos);

			m_rowAt.RemoveAt(pos, numRows);

			//Shift the row IDs
			int rowPos;
			for (rowPos = 0; rowPos < m_numRows; rowPos++)
			{
				if (m_rowAt[rowPos] > rowID)
					m_rowAt[rowPos] -= numRows;
			}
		}

		if (!m_rowHeights.IsEmpty())
		{
			m_rowHeights.RemoveAt(pos, numRows);
			m_rowBottoms.RemoveAt(pos, numRows);

			int h = 0;
			int rowPos;
			for (rowPos = 0; rowPos < m_numRows; rowPos++)
			{
				i = GetRowAt(rowPos);

				h += GetRowHeight(i);
				m_rowBottoms[i] = h;
			}
		}

		UpdateCurrentCellOnRedim();

		if (m_selection)
			m_selection->UpdateRows(pos, -((int)numRows));
		wxGridExtCellAttrProvider* attrProvider = m_table->GetAttrProvider();
		if (attrProvider)
		{
			attrProvider->UpdateAttrRows(pos, -((int)numRows));

			// ifdef'd out following patch from Paul Gammans
#if 0
				// No need to touch column attributes, unless we
				// removed _all_ rows, in this case, we remove
				// all column attributes.
				// I hate to do this here, but the
				// needed data is not available inside UpdateAttrRows.
			if (!GetNumberRows())
				attrProvider->UpdateAttrCols(0, -GetNumberCols());
#endif
		}

		CalcDimensions();

		if (ShouldRefresh()) {
			m_rowAreaWin->Refresh();
			m_rowLabelWin->Refresh();
		}
	}
	result = true;
	break;

	case wxGRIDTABLE_NOTIFY_COLS_INSERTED:
	{
		size_t pos = msg.GetCommandInt();
		int numCols = msg.GetCommandInt2();
		m_numCols += numCols;

		if (!m_colAt.IsEmpty())
		{
			//Shift the column IDs
			for (i = 0; i < m_numCols - numCols; i++)
			{
				if (m_colAt[i] >= (int)pos)
					m_colAt[i] += numCols;
			}

			m_colAt.Insert(pos, pos, numCols);

			//Set the new columns' positions
			for (i = pos + 1; i < (int)pos + numCols; i++)
			{
				m_colAt[i] = i;
			}
		}

		if (!m_colWidths.IsEmpty())
		{
			m_colWidths.Insert(m_defaultColWidth, pos, numCols);
			m_colRights.Insert(0, pos, numCols);

			int right = 0;
			if (pos > 0)
				right = m_colRights[GetColAt(pos - 1)];

			int colPos;
			for (colPos = pos; colPos < m_numCols; colPos++)
			{
				i = GetColAt(colPos);

				right += GetColWidth(i);
				m_colRights[i] = right;
			}
		}

		// See comment for wxGRIDTABLE_NOTIFY_COLS_APPENDED case explaining
		// why this has to be done here and not before.
		if (m_useNativeHeader)
			GetGridColHeader()->SetColumnCount(m_numCols);

		UpdateCurrentCellOnRedim();

		if (m_selection)
			m_selection->UpdateCols(pos, numCols);
		wxGridExtCellAttrProvider* attrProvider = m_table->GetAttrProvider();
		if (attrProvider)
			attrProvider->UpdateAttrCols(pos, numCols);

		CalcDimensions();

		if (ShouldRefresh()) {
			m_colAreaWin->Refresh();
			m_colLabelWin->Refresh();
		}
	}
	result = true;
	break;

	case wxGRIDTABLE_NOTIFY_COLS_APPENDED:
	{
		int numCols = msg.GetCommandInt();
		int oldNumCols = m_numCols;
		m_numCols += numCols;

		if (!m_colAt.IsEmpty())
		{
			m_colAt.Add(0, numCols);

			//Set the new columns' positions
			for (i = oldNumCols; i < m_numCols; i++)
			{
				m_colAt[i] = i;
			}
		}

		if (!m_colWidths.IsEmpty())
		{
			m_colWidths.Add(m_defaultColWidth, numCols);
			m_colRights.Add(0, numCols);

			int right = 0;
			if (oldNumCols > 0)
				right = m_colRights[GetColAt(oldNumCols - 1)];

			int colPos;
			for (colPos = oldNumCols; colPos < m_numCols; colPos++)
			{
				i = GetColAt(colPos);

				right += GetColWidth(i);
				m_colRights[i] = right;
			}
		}

		// Notice that this must be called after updating m_colWidths above
		// as the native grid control will check whether the new columns
		// are shown which results in accessing m_colWidths array.
		if (m_useNativeHeader)
			GetGridColHeader()->SetColumnCount(m_numCols);

		UpdateCurrentCellOnRedim();

		CalcDimensions();

		if (ShouldRefresh()) {
			m_colAreaWin->Refresh();
			m_colLabelWin->Refresh();
		}
	}
	result = true;
	break;

	case wxGRIDTABLE_NOTIFY_COLS_DELETED:
	{
		size_t pos = msg.GetCommandInt();
		int numCols = msg.GetCommandInt2();
		m_numCols -= numCols;

		if (!m_colAt.IsEmpty())
		{
			int colID = GetColAt(pos);

			m_colAt.RemoveAt(pos, numCols);

			//Shift the column IDs
			int colPos;
			for (colPos = 0; colPos < m_numCols; colPos++)
			{
				if (m_colAt[colPos] > colID)
					m_colAt[colPos] -= numCols;
			}
		}

		if (!m_colWidths.IsEmpty())
		{
			m_colWidths.RemoveAt(pos, numCols);
			m_colRights.RemoveAt(pos, numCols);

			int w = 0;
			int colPos;
			for (colPos = 0; colPos < m_numCols; colPos++)
			{
				i = GetColAt(colPos);

				w += GetColWidth(i);
				m_colRights[i] = w;
			}
		}

		// See comment for wxGRIDTABLE_NOTIFY_COLS_APPENDED case explaining
		// why this has to be done here and not before.
		if (m_useNativeHeader)
			GetGridColHeader()->SetColumnCount(m_numCols);

		UpdateCurrentCellOnRedim();

		if (m_selection)
			m_selection->UpdateCols(pos, -((int)numCols));
		wxGridExtCellAttrProvider* attrProvider = m_table->GetAttrProvider();
		if (attrProvider)
		{
			attrProvider->UpdateAttrCols(pos, -((int)numCols));

			// ifdef'd out following patch from Paul Gammans
#if 0
				// No need to touch row attributes, unless we
				// removed _all_ columns, in this case, we remove
				// all row attributes.
				// I hate to do this here, but the
				// needed data is not available inside UpdateAttrCols.
			if (!GetNumberCols())
				attrProvider->UpdateAttrRows(0, -GetNumberRows());
#endif
		}

		CalcDimensions();

		if (ShouldRefresh()) {
			m_colAreaWin->Refresh();
			m_colLabelWin->Refresh();
		}
	}
	result = true;
	break;
	}

	InvalidateBestSize();

	if (result && ShouldRefresh())
		Refresh();

	return result;
}

wxArrayInt wxGridExt::CalcRowLabelsExposed(const wxRegion& reg, wxGridExtWindow* gridWindow) const
{
	wxRect r;

	wxArrayInt  rowlabels;

	int top, bottom;
	for (wxRegionIterator iter(reg); iter; ++iter)
	{
		r = iter.GetRect();
		r.Offset(GetGridWindowOffset(gridWindow));

		// TODO: remove this when we can...
		// There is a bug in wxMotif that gives garbage update
		// rectangles if you jump-scroll a long way by clicking the
		// scrollbar with middle button.  This is a work-around
		//
#if defined(__WXMOTIF__)
		int cw, ch;
		m_gridWin->GetClientSize(&cw, &ch);
		if (r.GetTop() > ch)
			r.SetTop(0);
		r.SetBottom(wxMin(r.GetBottom(), ch));
#endif

		// logical bounds of update region
		//
		int dummy;
		CalcGridWindowUnscrolledPosition(0, r.GetTop(), &dummy, &top, gridWindow);
		CalcGridWindowUnscrolledPosition(0, r.GetBottom(), &dummy, &bottom, gridWindow);

		// find the row labels within these bounds
		const int rowFirst = internalYToRow(top, gridWindow);
		if (rowFirst == wxNOT_FOUND)
			continue;

		for (int rowPos = GetRowPos(rowFirst); rowPos < m_numRows; rowPos++)
		{
			int row;
			row = GetRowAt(rowPos);

			if (GetRowBottom(row) < top)
				continue;

			if (GetRowTop(row) > bottom)
				break;

			rowlabels.Add(row);
		}
	}

	return rowlabels;
}

wxArrayInt wxGridExt::CalcColLabelsExposed(const wxRegion& reg, wxGridExtWindow* gridWindow) const
{
	wxRect r;

	wxArrayInt colLabels;

	int left, right;
	for (wxRegionIterator iter(reg); iter; ++iter)
	{
		r = iter.GetRect();
		r.Offset(GetGridWindowOffset(gridWindow));

		// TODO: remove this when we can...
		// There is a bug in wxMotif that gives garbage update
		// rectangles if you jump-scroll a long way by clicking the
		// scrollbar with middle button.  This is a work-around
		//
#if defined(__WXMOTIF__)
		int cw, ch;
		m_gridWin->GetClientSize(&cw, &ch);
		if (r.GetLeft() > cw)
			r.SetLeft(0);
		r.SetRight(wxMin(r.GetRight(), cw));
#endif

		// logical bounds of update region
		//
		int dummy;
		CalcGridWindowUnscrolledPosition(r.GetLeft(), 0, &left, &dummy, gridWindow);
		CalcGridWindowUnscrolledPosition(r.GetRight(), 0, &right, &dummy, gridWindow);

		// find the cells within these bounds
		//
		const int colFirst = internalXToCol(left, gridWindow);
		if (colFirst == wxNOT_FOUND)
			continue;

		for (int colPos = GetColPos(colFirst); colPos < m_numCols; colPos++)
		{
			int col;
			col = GetColAt(colPos);

			if (GetColRight(col) < left)
				continue;

			if (GetColLeft(col) > right)
				break;

			colLabels.Add(col);
		}
	}

	return colLabels;
}

wxGridExtCellCoordsArray wxGridExt::CalcCellsExposed(const wxRegion& reg,
	wxGridExtWindow* gridWindow) const
{
	wxRect r;

	wxGridExtCellCoordsArray  cellsExposed;

	int left, top, right, bottom;
	for (wxRegionIterator iter(reg); iter; ++iter)
	{
		r = iter.GetRect();
		r.Offset(GetGridWindowOffset(gridWindow));

		// Skip 0-height cells, they're invisible anyhow, don't waste time
		// getting their rectangles and so on.
		if (!r.GetHeight())
			continue;

		// TODO: remove this when we can...
		// There is a bug in wxMotif that gives garbage update
		// rectangles if you jump-scroll a long way by clicking the
		// scrollbar with middle button.  This is a work-around
		//
#if defined(__WXMOTIF__)
		if (gridWindow)
		{
			int cw, ch;
			gridWindow->GetClientSize(&cw, &ch);
			if (r.GetTop() > ch) r.SetTop(0);
			if (r.GetLeft() > cw) r.SetLeft(0);
			r.SetRight(wxMin(r.GetRight(), cw));
			r.SetBottom(wxMin(r.GetBottom(), ch));
		}
#endif

		// logical bounds of update region
		//
		CalcGridWindowUnscrolledPosition(r.GetLeft(), r.GetTop(), &left, &top, gridWindow);
		CalcGridWindowUnscrolledPosition(r.GetRight(), r.GetBottom(), &right, &bottom, gridWindow);

		// find the cells within these bounds
		//
		const int rowFirst = internalYToRow(top, gridWindow);
		if (rowFirst == wxNOT_FOUND)
			continue;

		wxArrayInt cols;

		for (int rowPos = GetRowPos(rowFirst); rowPos < m_numRows; rowPos++)
		{
			const int row = GetRowAt(rowPos);

			if (GetRowBottom(row) <= top)
				continue;

			if (GetRowTop(row) > bottom)
				break;

			// add all dirty cells in this row: notice that the columns which
			// are dirty don't depend on the row so we compute them only once
			// for the first dirty row and then reuse for all the next ones
			if (cols.empty())
			{
				// do determine the dirty columns
				for (int pos = XToPos(left, gridWindow); pos <= XToPos(right, gridWindow); pos++)
					cols.push_back(GetColAt(pos));

				// if there are no dirty columns at all, nothing to do
				if (cols.empty())
					break;
			}

			const size_t count = cols.size();
			cellsExposed.Alloc(count + cellsExposed.size());
			for (size_t n = 0; n < count; n++)
				cellsExposed.Add(wxGridExtCellCoords(row, cols[n]));
		}
	}

	return cellsExposed;
}

void wxGridExt::PrepareDCFor(wxDC& dc, wxGridExtWindow* gridWindow)
{
	wxScrolledCanvas::PrepareDC(dc);

	wxPoint dcOrigin = dc.GetDeviceOrigin() - GetGridWindowOffset(gridWindow);

	if (gridWindow->GetType() & wxGridExtWindow::wxGridExtWindowFrozenCol)
		dcOrigin.x = 0;
	if (gridWindow->GetType() & wxGridExtWindow::wxGridExtWindowFrozenRow)
		dcOrigin.y = 0;

	dc.SetDeviceOrigin(dcOrigin.x, dcOrigin.y);
}

void wxGridExt::CheckDoDragScroll(wxGridExtSubwindow* eventGridWindow, wxGridExtSubwindow* gridWindow,
	wxPoint posEvent, int direction)
{
	// helper for Process{Row|Col}LabelMouseEvent, ProcessGridCellMouseEvent:
	//  scroll when at the edges or outside the window w. respect to direction
	// eventGridWindow: the window that received the mouse event
	// gridWindow: the same or the corresponding non-frozen window

	if (!m_isDragging)
	{
		// drag is just starting
		m_lastMousePos = posEvent;
		return;
	}

	int w, h;
	eventGridWindow->GetSize(&w, &h);
	// ViewStart is scroll position in scroll units
	wxPoint scrollPos = GetViewStart();
	wxPoint newScrollPos = wxPoint(wxDefaultCoord, wxDefaultCoord);

	if (direction & wxHORIZONTAL)
	{
		// check x direction
		if (eventGridWindow->IsFrozen() && posEvent.x < w)
		{
			// in the frozen window, moving left?
			if (scrollPos.x > 0 && posEvent.x < m_lastMousePos.x)
				newScrollPos.x = scrollPos.x - 1;
		}
		else if (eventGridWindow->IsFrozen() && posEvent.x >= w)
		{
			// frozen window was left, add the width of the non-frozen window
			w += gridWindow->GetSize().x;
		}

		if (posEvent.x < 0 && scrollPos.x > 0)
			newScrollPos.x = scrollPos.x - 1;
		else if (posEvent.x >= w)
			newScrollPos.x = scrollPos.x + 1;
	}

	if (direction & wxVERTICAL)
	{
		// check y direction
		if (eventGridWindow->IsFrozen() && posEvent.y < h)
		{
			// in the frozen window, moving upward?
			if (scrollPos.y && posEvent.y < m_lastMousePos.y)
				newScrollPos.y = scrollPos.y - 1;
		}
		else if (eventGridWindow->IsFrozen() && posEvent.y >= h)
		{
			// frozen window was left, add the height of the non-frozen window
			h += gridWindow->GetSize().y;
		}

		if (posEvent.y < 0 && scrollPos.y > 0)
			newScrollPos.y = scrollPos.y - 1;
		else if (posEvent.y >= h)
			newScrollPos.y = scrollPos.y + 1;
	}

	if (newScrollPos.x != wxDefaultCoord || newScrollPos.y != wxDefaultCoord)
		Scroll(newScrollPos);

	m_lastMousePos = posEvent;
}

bool wxGridExt::CheckIfDragCancelled(wxMouseEvent* event)
{
	// helper for Process{Row|Col}LabelMouseEvent, ProcessGridCellMouseEvent:
	// block re-triggering m_isDragging
	if (!m_cancelledDragging)
		return false;

	if (event->LeftIsDown())
		return true;

	m_cancelledDragging = false;

	return false;
}

bool wxGridExt::CheckIfAtDragSourceLine(const wxGridExtOperations& oper, int coord)
{
	// check whether coord on the dragged line or at max half of a line away
	// (the sizes of the lines before/after can be 0, if they are hidden)
	int minCoord = oper.GetLineStartPos(this, m_dragMoveRowOrCol);
	int maxCoord = minCoord + oper.GetLineSize(this, m_dragMoveRowOrCol);

	int lineBefore = oper.GetLineBefore(this, m_dragMoveRowOrCol);
	if (lineBefore == -1 && coord < maxCoord)
		return true;
	if (lineBefore != -1)
		minCoord -= oper.GetLineSize(this, lineBefore) / 2;

	int posAfter = oper.GetLinePos(this, m_dragMoveRowOrCol) + 1;
	int lineAfter = posAfter < oper.GetTotalNumberOfLines(this) ?
		oper.GetLineAt(this, posAfter) : -1;

	if (lineAfter == -1 && coord >= minCoord)
		return true;
	if (lineAfter != -1)
		maxCoord += oper.GetLineSize(this, lineAfter) / 2;

	if (coord >= minCoord && coord < maxCoord)
		return true;
	return false;
}

void wxGridExt::ProcessRowColLabelMouseEvent(const wxGridExtOperations& oper, wxMouseEvent& event, wxGridExtSubwindow* labelWin)
{
	const wxGridExtOperations& dual = oper.Dual();

	wxGridExtSubwindow* headerWin = (wxGridExtSubwindow*)oper.GetHeaderWindow(this);
	wxGridExtWindow* gridWindow = labelWin->IsFrozen() ? oper.GetFrozenGrid(this) :
		m_gridWin;

	// store position, before it's modified in the next step
	const wxPoint posEvent = event.GetPosition();

	event.SetPosition(posEvent + GetGridWindowOffset(gridWindow));

	// for drag, we could be moving from the window sending the event to the other
	if (labelWin->IsFrozen() &&
		dual.Select(event.GetPosition()) > dual.Select(labelWin->GetClientSize()))
		gridWindow = m_gridWin;

	const wxPoint unscrolledPos = CalcGridWindowUnscrolledPosition(event.GetPosition(), gridWindow);

	// find y or x mouse coordinate for row / col label window
	int coord = dual.Select(unscrolledPos);
	// index into data rows/cols;  wxNOT_FOUND if outside label window
	int line = oper.PosToLine(this, coord, NULL);

	// wxNOT_FOUND if not near a line edge;  otherwise index into data rows/cols
	int lineEdge = PosToEdgeOfLine(coord, oper);


	// these are always valid, even if the mouse is outside the row/col range:
	// index into displayed rows/cols
	int posAt = PosToLinePos(coord, true /* clip */, oper, NULL);
	// index into data rows/cols
	int lineAt = oper.GetLineAt(this, posAt);

	if (CheckIfDragCancelled(&event))
		return;

	if (event.Dragging() && (m_winCapture == labelWin))
	{
		// scroll when at the edges or outside the window
		CheckDoDragScroll(labelWin, headerWin, posEvent, oper.GetOrientation());
	}

	if (event.Dragging())
	{
		if (!m_isDragging)
		{
			m_isDragging = true;

			if (m_cursorMode == oper.GetCursorModeMove() && line != -1)
				DoStartMoveRowOrCol(line);
		}

		if (event.LeftIsDown())
		{
			if (m_cursorMode == oper.GetCursorModeResize())
			{
				DoGridDragResize(event.GetPosition(), oper, gridWindow);
			}
			else if (m_cursorMode == oper.GetCursorModeSelect() && line >= 0)
			{
				// We can't extend the selection from non-selected row / col,
				// which may happen if we Ctrl-clicked it initially.
				// Therefore check m_selection->IsInSelection(m_currentCellCoords)

				if (m_selection && m_numRows && m_numCols &&
					m_selection->IsInSelection(m_currentCellCoords))
				{
					oper.SelectionExtendCurrentBlock(this, line, event, wxEVT_GRID_RANGE_SELECTING);
				}
			}
			else if (m_cursorMode == oper.GetCursorModeMove())
			{
				// determine the y or x position of the drop marker
				int markerPos;
				if (coord >= oper.GetLineStartPos(this, lineAt) + (oper.GetLineSize(this, lineAt) / 2))
					markerPos = oper.GetLineEndPos(this, lineAt);
				else
					markerPos = oper.GetLineStartPos(this, lineAt);

				const wxColour* markerColour;
				// Moving to the same place? Draw a grey marker.
				if (CheckIfAtDragSourceLine(oper, coord))
					markerColour = wxLIGHT_GREY;
				else
					markerColour = wxBLUE;

				if (markerPos != m_dragLastPos || markerColour != m_dragLastColour)
				{
					wxClientDC dc(headerWin);
					oper.PrepareDCForLabels(this, dc);

					int markerLength = oper.Select(headerWin->GetClientSize());

					// Clean up the last indicator if position has changed
					if (m_dragLastPos >= 0 && markerPos != m_dragLastPos)
					{
						wxPen pen(headerWin->GetBackgroundColour(), 2);
						dc.SetPen(pen);
						oper.DrawParallelLine(dc, 0, markerLength, m_dragLastPos + 1);
						dc.SetPen(wxNullPen);

						int lastLine = oper.PosToLine(this, m_dragLastPos, NULL, false);

						if (lastLine != -1)
							oper.DrawLineLabel(this, dc, lastLine);
					}

					// Draw the marker
					wxPen pen(*markerColour, 2);
					dc.SetPen(pen);
					oper.DrawParallelLine(dc, 0, markerLength, markerPos + 1);
					dc.SetPen(wxNullPen);

					m_dragLastPos = markerPos;
					m_dragLastColour = markerColour;
				}
			}
		}
		return;
	}

	if (m_isDragging && (event.Entering() || event.Leaving()))
		return;

	// ------------ Entering or leaving the window
	//
	if (event.Entering() || event.Leaving())
	{
		ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL, labelWin);
	}

	// ------------ Left button pressed
	//
	else if (event.LeftDown())
	{
		if (lineEdge != wxNOT_FOUND && oper.CanDragLineSize(this, lineEdge))
		{
			DoStartResizeRowOrCol(lineEdge, oper.GetLineSize(this, lineEdge));
			ChangeCursorMode(oper.GetCursorModeResize(), labelWin);
		}
		else
		{
			// not a request to start resizing
			if (line >= 0 &&
				oper.SendEvent(this, wxEVT_GRID_LABEL_LEFT_CLICK, line, event) == Event_Unhandled)
			{
				if (oper.CanDragMove(this))
				{
					ChangeCursorMode(oper.GetCursorModeMove(), headerWin);

					// Show button as pressed
					m_dragMoveRowOrCol = line;
					wxClientDC dc(headerWin);
					oper.PrepareDCForLabels(this, dc);
					oper.DrawLineLabel(this, dc, line);
				}
				else
				{
					// Check if row/col selection is possible and allowed, before doing
					// anything else, including changing the cursor mode to "select
					// row"/"select col".
					if (m_selection && m_numRows > 0 && m_numCols > 0 &&
						m_selection->GetSelectionMode() != dual.GetSelectionMode())
					{
						bool selectNewLine = false,
							makeLineCurrent = false;

						if (event.ShiftDown() && !event.CmdDown())
						{
							// Continue editing the current selection and don't
							// move the grid cursor.
							oper.SelectionExtendCurrentBlock(this, line, event);
							oper.MakeLineVisible(this, line);
						}
						else if (event.CmdDown() && !event.ShiftDown())
						{
							if (oper.IsLineInSelection(this, line))
							{
								oper.DeselectLine(this, line);
								makeLineCurrent = true;
							}
							else
							{
								makeLineCurrent =
									selectNewLine = true;
							}
						}
						else
						{
							ClearSelection();
							makeLineCurrent =
								selectNewLine = true;
						}

						if (selectNewLine)
							oper.SelectLine(this, line, event);

						if (makeLineCurrent)
							oper.MakeLineCurrent(this, line);

						ChangeCursorMode(oper.GetCursorModeSelect(), labelWin);
					}
				}
			}
		}
	}

	// ------------ Left double click
	//
	else if (event.LeftDClick())
	{
		if (lineEdge != wxNOT_FOUND && oper.CanDragLineSize(this, line))
		{
			// adjust row or column size depending on label text
			oper.HandleLineAutosize(this, lineEdge, event);

			ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL, labelWin);
			m_dragLastPos = -1;
		}
		else // not on line separator or it's not resizable
		{
			if (line >= 0 &&
				oper.SendEvent(this, wxEVT_GRID_LABEL_LEFT_DCLICK, line, event) == Event_Unhandled)
			{
				// no default action at the moment
			}
		}
	}

	// ------------ Left button released
	//
	else if (event.LeftUp())
	{
		if (m_cursorMode == oper.GetCursorModeResize())
		{
			oper.DoEndLineResize(this, event, gridWindow);
		}
		else if (m_cursorMode == oper.GetCursorModeMove())
		{
			if (CheckIfAtDragSourceLine(oper, coord))
			{
				// the line didn't actually move anywhere, "unpress" the label
				if (oper.GetOrientation() == wxVERTICAL && line != -1)
					DoColHeaderClick(line);
				oper.GetHeaderWindow(this)->Refresh();
			}
			else
			{
				// find the target position: start with current mouse position
				// (posAt and lineAt are always valid indices, not wxNOT_FOUND)
				int posNew = posAt;

				// adjust for the row being dragged itself
				if (posAt < oper.GetLinePos(this, m_dragMoveRowOrCol))
					posNew++;

				// if mouse is on the near part of the target row,
				// insert it before it, not after
				if (posNew > 0 &&
					(coord <= oper.GetLineStartPos(this, lineAt) +
						oper.GetLineSize(this, lineAt) / 2))
					posNew--;

				oper.DoEndMove(this, posNew);
			}
		}
		else if (oper.GetOrientation() == wxHORIZONTAL && line != -1 &&
			(m_cursorMode == WXGRID_CURSOR_SELECT_CELL ||
				m_cursorMode == WXGRID_CURSOR_SELECT_COL))
		{
			DoColHeaderClick(line);
		}
		EndDraggingIfNecessary();
	}

	// ------------ Right button down
	//
	else if (event.RightDown())
	{
		if (line < 0 ||
			oper.SendEvent(this, wxEVT_GRID_LABEL_RIGHT_CLICK, line, event) == Event_Unhandled)
		{
			// no default action at the moment
			event.Skip();
		}
	}

	// ------------ Right double click
	//
	else if (event.RightDClick())
	{
		if (line < 0 ||
			oper.SendEvent(this, wxEVT_GRID_LABEL_RIGHT_DCLICK, line, event) == Event_Unhandled)
		{
			// no default action at the moment
			event.Skip();
		}
	}

	// ------------ No buttons down and mouse moving
	//
	else if (event.Moving())
	{
		if (lineEdge != wxNOT_FOUND)
		{
			if (m_cursorMode == WXGRID_CURSOR_SELECT_CELL)
			{
				if (oper.CanDragLineSize(this, lineEdge))
				{
					ChangeCursorMode(oper.GetCursorModeResize(), labelWin, false);
				}
			}
		}
		else if (m_cursorMode != WXGRID_CURSOR_SELECT_CELL)
		{
			ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL, labelWin, false);
		}
	}

	// Don't consume the remaining events (e.g. right up).
	else
	{
		event.Skip();
	}
}

void wxGridExt::UpdateCurrentCellOnRedim()
{
	if (m_currentCellCoords == wxGridExtNoCellCoords)
	{
		// We didn't have any valid selection before, which can only happen
		// if the grid was empty.
		// Check if this is still the case and ensure we do have valid
		// selection if the grid is not empty any more.
		if (m_numCols > 0 && m_numRows > 0)
		{
			SetCurrentCell(0, 0);
		}
	}
	else
	{
		if (m_numCols == 0 || m_numRows == 0)
		{
			// We have to reset the selection, as it must either use validate
			// coordinates otherwise, but there are no valid coordinates for
			// the grid cells any more now that it is empty.
			m_currentCellCoords = wxGridExtNoCellCoords;
		}
		else
		{
			// Check if the current cell coordinates are still valid.
			wxGridExtCellCoords updatedCoords = m_currentCellCoords;
			if (updatedCoords.GetCol() >= m_numCols)
				updatedCoords.SetCol(m_numCols - 1);
			if (updatedCoords.GetRow() >= m_numRows)
				updatedCoords.SetRow(m_numRows - 1);

			// And change them if they're not.
			if (updatedCoords != m_currentCellCoords)
			{
				// Prevent SetCurrentCell() from redrawing the previous current
				// cell whose coordinates are invalid now.
				m_currentCellCoords = wxGridExtNoCellCoords;
				SetCurrentCell(updatedCoords);
			}
		}
	}
}

void wxGridExt::UpdateColumnSortingIndicator(int col)
{
	wxCHECK_RET(col != wxNOT_FOUND, "invalid column index");

	if (m_useNativeHeader) {
		m_colAreaWin->Refresh();
		GetGridColHeader()->UpdateColumn(col);
	}
	else if (m_nativeColumnLabels) {
		m_colAreaWin->Refresh();
		m_colLabelWin->Refresh();
	}
	//else: sorting indicator display not yet implemented in grid version
}

void wxGridExt::SetSortingColumn(int col, bool ascending)
{
	if (col == m_sortCol)
	{
		// we are already using this column for sorting (or not sorting at all)
		// but we might still change the sorting order, check for it
		if (m_sortCol != wxNOT_FOUND && ascending != m_sortIsAscending)
		{
			m_sortIsAscending = ascending;

			UpdateColumnSortingIndicator(m_sortCol);
		}
	}
	else // we're changing the column used for sorting
	{
		const int sortColOld = m_sortCol;

		// change it before updating the column as we want GetSortingColumn()
		// to return the correct new value
		m_sortCol = col;

		if (sortColOld != wxNOT_FOUND)
			UpdateColumnSortingIndicator(sortColOld);

		if (m_sortCol != wxNOT_FOUND)
		{
			m_sortIsAscending = ascending;
			UpdateColumnSortingIndicator(m_sortCol);
		}
	}
}

void wxGridExt::DoColHeaderClick(int col)
{
	// we consider that the grid was resorted if this event is processed and
	// not vetoed
	if (SendEvent(wxEVT_GRID_COL_SORT, -1, col) == Event_Handled)
	{
		SetSortingColumn(col, IsSortingBy(col) ? !m_sortIsAscending : true);
		Refresh();
	}
}

void wxGridExt::DoStartResizeRowOrCol(int col, int size)
{
	// Hide the editor if it's currently shown to avoid any weird interactions
	// with it while dragging the row/column separator.
	AcceptCellEditControlIfShown();

	m_dragRowOrCol = col;
	m_dragRowOrColOldSize = size;
}

void wxGridExt::ProcessRowColAreaMouseEvent(const wxGridExtOperations& oper, wxMouseEvent& event, wxGridExtSubwindow* areaWin)
{
	const wxGridExtOperations& dual = oper.Dual();

	wxGridExtSubwindow* headerWin = (wxGridExtSubwindow*)oper.GetHeaderWindow(this);
	wxGridExtWindow* gridWindow = areaWin->IsFrozen() ? oper.GetFrozenGrid(this) :
		m_gridWin;

	// store position, before it's modified in the next step
	const wxPoint posEvent = event.GetPosition();

	event.SetPosition(posEvent + GetGridWindowOffset(gridWindow));

	// for drag, we could be moving from the window sending the event to the other
	if (areaWin->IsFrozen() &&
		dual.Select(event.GetPosition()) > dual.Select(areaWin->GetClientSize()))
		gridWindow = m_gridWin;

	const wxPoint unscrolledPos = CalcGridWindowUnscrolledPosition(event.GetPosition(), gridWindow);

	// find y or x mouse coordinate for row / col label window
	int coord = dual.Select(unscrolledPos);

	// index into data rows/cols;  wxNOT_FOUND if outside label window
	int line = oper.PosToLine(this, coord, NULL);

	if (event.LeftDown())
	{
		// indicate corner label by having both row and
		// col args == -1
		//
		if (SendEvent(wxEVT_GRID_LABEL_LEFT_CLICK, -1, -1, event) == Event_Unhandled) {
			oper.SelectArea(this, line, event);
		}
	}
	else if (event.LeftDClick())
	{
		SendEvent(wxEVT_GRID_LABEL_LEFT_DCLICK, -1, -1, event);
		oper.MakeAreaName(this, line, event);
	}
	else if (event.RightDown())
	{
		if (SendEvent(wxEVT_GRID_LABEL_RIGHT_CLICK, -1, -1, event) == Event_Unhandled)
		{
			// no default action at the moment
			event.Skip();
		}
	}
	else if (event.RightDClick())
	{
		if (SendEvent(wxEVT_GRID_LABEL_RIGHT_DCLICK, -1, -1, event) == Event_Unhandled)
		{
			// no default action at the moment
			event.Skip();
		}
	}
	else
	{
		event.Skip();
	}
}

void wxGridExt::ProcessCornerLabelMouseEvent(wxMouseEvent& event)
{
	if (event.LeftDown())
	{
		// indicate corner label by having both row and
		// col args == -1
		//
		if (SendEvent(wxEVT_GRID_LABEL_LEFT_CLICK, -1, -1, event) == Event_Unhandled)
		{
			SelectAll();
		}
	}
	else if (event.LeftDClick())
	{
		SendEvent(wxEVT_GRID_LABEL_LEFT_DCLICK, -1, -1, event);
	}
	else if (event.RightDown())
	{
		if (SendEvent(wxEVT_GRID_LABEL_RIGHT_CLICK, -1, -1, event) == Event_Unhandled)
		{
			// no default action at the moment
			event.Skip();
		}
	}
	else if (event.RightDClick())
	{
		if (SendEvent(wxEVT_GRID_LABEL_RIGHT_DCLICK, -1, -1, event) == Event_Unhandled)
		{
			// no default action at the moment
			event.Skip();
		}
	}
	else
	{
		event.Skip();
	}
}

void wxGridExt::HandleRowAutosize(int row, const wxMouseEvent& event)
{
	// adjust row height depending on label text
	//
	// TODO: generate RESIZING event, see #10754
	if (!SendGridSizeEvent(wxEVT_GRID_ROW_AUTO_SIZE, row, event))
		AutoSizeRowLabelSize(row);

	SendGridSizeEvent(wxEVT_GRID_ROW_SIZE, row, event);
}

void wxGridExt::HandleColumnAutosize(int col, const wxMouseEvent& event)
{
	// adjust column width depending on label text
	//
	// TODO: generate RESIZING event, see #10754
	if (!SendGridSizeEvent(wxEVT_GRID_COL_AUTO_SIZE, col, event))
		AutoSizeColLabelSize(col);

	SendGridSizeEvent(wxEVT_GRID_COL_SIZE, col, event);
}

void wxGridExt::CancelMouseCapture()
{
	// cancel operation currently in progress, whatever it is
	if (m_winCapture)
	{
		if (m_cursorMode == WXGRID_CURSOR_MOVE_COL ||
			m_cursorMode == WXGRID_CURSOR_MOVE_ROW)
			m_winCapture->Refresh();
		DoAfterDraggingEnd();
	}
}

void wxGridExt::DoAfterDraggingEnd()
{
	if (m_isDragging &&
		(m_cursorMode == WXGRID_CURSOR_SELECT_CELL ||
			m_cursorMode == WXGRID_CURSOR_SELECT_ROW ||
			m_cursorMode == WXGRID_CURSOR_SELECT_COL))
	{
		m_selection->EndSelecting();
	}

	m_isDragging = false;
	m_startDragPos = wxDefaultPosition;
	m_lastMousePos = wxDefaultPosition;
	// from drag moving row/col
	m_dragMoveRowOrCol = -1;
	m_dragLastPos = -1;
	m_dragLastColour = NULL;

	m_cursorMode = WXGRID_CURSOR_SELECT_CELL;
	m_winCapture->SetCursor(*wxSTANDARD_CURSOR);
	m_winCapture = NULL;
}

void wxGridExt::EndDraggingIfNecessary()
{
	if (m_winCapture)
	{
		m_winCapture->ReleaseMouse();

		DoAfterDraggingEnd();
	}
}

void wxGridExt::ChangeCursorMode(CursorMode mode,
	wxWindow* win,
	bool captureMouse)
{
#if wxUSE_LOG_TRACE
	static const wxChar* const cursorModes[] =
	{
		wxT("SELECT_CELL"),
		wxT("RESIZE_ROW"),
		wxT("RESIZE_COL"),
		wxT("SELECT_ROW"),
		wxT("SELECT_COL"),
		wxT("MOVE_ROW"),
		wxT("MOVE_COL"),
	};

	wxLogTrace(wxT("grid"),
		wxT("wxGridExt cursor mode (mouse capture for %s): %s -> %s"),
		win == m_colLabelWin ? wxT("colLabelWin")
		: win ? wxT("rowLabelWin")
		: wxT("gridWin"),
		cursorModes[m_cursorMode], cursorModes[mode]);
#endif // wxUSE_LOG_TRACE

	if (mode == m_cursorMode &&
		win == m_winCapture &&
		captureMouse == (m_winCapture != NULL))
		return;

	if (!win)
	{
		// by default use the grid itself
		win = m_gridWin;
	}

	EndDraggingIfNecessary();

	m_cursorMode = mode;

	switch (m_cursorMode)
	{
	case WXGRID_CURSOR_RESIZE_ROW:
		win->SetCursor(m_rowResizeCursor);
		break;

	case WXGRID_CURSOR_RESIZE_COL:
		win->SetCursor(m_colResizeCursor);
		break;

	case WXGRID_CURSOR_MOVE_ROW:
	case WXGRID_CURSOR_MOVE_COL:
		win->SetCursor(wxCursor(wxCURSOR_HAND));
		break;

	case WXGRID_CURSOR_SELECT_CELL:
		// Mouse is captured in ProcessGridCellMouseEvent() in this mode.
		captureMouse = false;
		wxFALLTHROUGH;

	case WXGRID_CURSOR_SELECT_ROW:
	case WXGRID_CURSOR_SELECT_COL:
		win->SetCursor(*wxSTANDARD_CURSOR);
		break;
	}

	if (captureMouse)
	{
		win->CaptureMouse();
		m_winCapture = win;
	}
}

// ----------------------------------------------------------------------------
// grid mouse event processing
// ----------------------------------------------------------------------------

bool
wxGridExt::DoGridCellDrag(wxMouseEvent& event,
	const wxGridExtCellCoords& coords,
	bool isFirstDrag)
{
	if (coords == wxGridExtNoCellCoords)
		return false; // we're outside any valid cell

	if (isFirstDrag)
	{
		// Hide the edit control, so it won't interfere with drag-shrinking.
		AcceptCellEditControlIfShown();

		switch (event.GetModifiers())
		{
		case wxMOD_CONTROL:
			// Ctrl-dragging is special, because we could have started it
			// by Ctrl-clicking a previously selected cell, which has the
			// effect of deselecting it and in this case we can't start
			// drag-selection from it because the selection anchor should
			// always be selected itself.
			if (!m_selection->IsInSelection(m_currentCellCoords))
				return false;
			break;

		case wxMOD_NONE:
			if (CanDragCell())
			{
				// if event is handled by user code, no further processing
				return SendEvent(wxEVT_GRID_CELL_BEGIN_DRAG, coords, event) == Event_Unhandled;
			}
			break;

			//default: In all the other cases, we don't have anything special
			//         to do and we'll just extend the selection below.
		}
	}

	// Note that we don't need to check the modifiers here, it doesn't matter
	// which keys are pressed for the current event, as pressing or releasing
	// Ctrl later can't change the dragging behaviour. Only the initial state
	// of the modifier keys matters.
	if (m_selection)
	{
		wxGridExtCellCoords blockStart = m_currentCellCoords;
		wxGridExtCellCoords blockEnd = coords;

		int row = coords.GetRow(), col = coords.GetCol();
		int cell_rows, cell_cols;

		if (GetCellSize(row, col, &cell_rows, &cell_cols) == CellSpan_Main)
		{
			int end_row = row + cell_rows - 1;
			int end_col = col + cell_cols - 1;

			int cell_start_rows, cell_start_cols;
			GetCellSize(end_row, end_col, &cell_start_rows, &cell_start_cols);

			int start_row = end_row + cell_start_rows;
			int start_col = end_col + cell_start_cols;

			if (m_currentCellCoords.GetRow() <= row)
			{
				if (start_row < blockStart.GetRow())
					blockStart.SetRow(start_row);

				blockEnd.SetRow(end_row);
			}
			else if (m_currentCellCoords.GetRow() > row)
			{
				if (end_row > blockStart.GetRow())
					blockStart.SetRow(end_row);

				blockEnd.SetRow(start_row);
			}

			if (m_currentCellCoords.GetCol() <= col)
			{
				if (start_col < blockStart.GetCol())
					blockStart.SetCol(start_col);

				blockEnd.SetCol(end_col);
			}
			else if (m_currentCellCoords.GetCol() > col)
			{
				if (end_col > blockStart.GetCol())
					blockStart.SetCol(end_col);

				blockEnd.SetCol(start_col);
			}
		}

		m_selection->ExtendCurrentBlock(blockStart,
			blockEnd,
			event,
			wxEVT_GRID_RANGE_SELECTING);
	}

	return true;
}

bool wxGridExt::DoGridDragEvent(wxMouseEvent& event,
	const wxGridExtCellCoords& coords,
	bool isFirstDrag,
	wxGridExtWindow* gridWindow)
{
	switch (m_cursorMode)
	{
	case WXGRID_CURSOR_SELECT_CELL:
		return DoGridCellDrag(event, coords, isFirstDrag);

	case WXGRID_CURSOR_RESIZE_ROW:
		if (m_dragRowOrCol != -1)
			DoGridDragResize(event.GetPosition(), wxGridExtRowOperations(), gridWindow);
		break;

	case WXGRID_CURSOR_RESIZE_COL:
		if (m_dragRowOrCol != -1)
			DoGridDragResize(event.GetPosition(), wxGridExtColumnOperations(), gridWindow);
		break;

	default:
		event.Skip();
	}

	return true;
}

void
wxGridExt::DoGridCellLeftDown(wxMouseEvent& event,
	const wxGridExtCellCoords& coords,
	const wxPoint& pos)
{
	if (SendEvent(wxEVT_GRID_CELL_LEFT_CLICK, coords, event) != Event_Unhandled)
	{
		// event handled by user code, no need to do anything here
		return;
	}

	// Process the mouse down event depending on the current cursor mode. Note
	// that this assumes m_cursorMode was set in the mouse move event hendler.
	switch (m_cursorMode)
	{
	case WXGRID_CURSOR_RESIZE_ROW:
	case WXGRID_CURSOR_RESIZE_COL:
	{
		int dragRowOrCol = wxNOT_FOUND;
		if (m_cursorMode == WXGRID_CURSOR_RESIZE_COL)
		{
			dragRowOrCol = XToEdgeOfCol(pos.x);
			DoStartResizeRowOrCol(dragRowOrCol, GetColSize(dragRowOrCol));
		}
		else
		{
			dragRowOrCol = YToEdgeOfRow(pos.y);
			DoStartResizeRowOrCol(dragRowOrCol, GetRowSize(dragRowOrCol));
		}
		wxCHECK_RET(dragRowOrCol != -1, "Can't determine row or column in resizing mode");
	}
	break;

	case WXGRID_CURSOR_SELECT_CELL:
	case WXGRID_CURSOR_SELECT_ROW:
	case WXGRID_CURSOR_SELECT_COL:
		DisableCellEditControl();
		MakeCellVisible(coords);

		if (event.ShiftDown() && !event.CmdDown())
		{
			if (m_selection)
			{
				m_selection->ExtendCurrentBlock(m_currentCellCoords, coords, event);
			}
		}
		else
		{
			if (event.CmdDown() && !event.ShiftDown())
			{
				if (m_selection)
				{
					if (!m_selection->IsInSelection(coords))
					{
						// If the cell is not selected, select it.
						m_selection->SelectBlock(coords.GetRow(), coords.GetCol(),
							coords.GetRow(), coords.GetCol(),
							event);
					}
					else
					{
						// Otherwise deselect it.
						m_selection->DeselectBlock(
							wxGridExtBlockCoords(coords.GetRow(), coords.GetCol(),
								coords.GetRow(), coords.GetCol()),
							event);
					}
				}
			}
			else
			{
				ClearSelection();

				if (m_selection)
				{
					// In row or column selection mode just clicking on the cell
					// should select the row or column containing it: this is more
					// convenient for the kinds of controls that use such selection
					// mode and is compatible with 2.8 behaviour (see #12062).
					switch (m_selection->GetSelectionMode())
					{
					case wxGridExtSelectNone:
					case wxGridExtSelectCells:
					case wxGridExtSelectRowsOrColumns:
						// nothing to do in these cases
						break;

					case wxGridExtSelectRows:
						m_selection->SelectRow(coords.GetRow());
						break;

					case wxGridExtSelectColumns:
						m_selection->SelectCol(coords.GetCol());
						break;
					}
				}

				m_waitForSlowClick = m_currentCellCoords == coords &&
					coords != wxGridExtNoCellCoords;
			}

			SetCurrentCell(coords);
		}
		break;

	case WXGRID_CURSOR_MOVE_ROW:
	case WXGRID_CURSOR_MOVE_COL:
		// Nothing to do here for this case.
		break;
	}
}

void
wxGridExt::DoGridCellLeftDClick(wxMouseEvent& event,
	const wxGridExtCellCoords& coords,
	const wxPoint& pos)
{
	if (XToEdgeOfCol(pos.x) < 0 && YToEdgeOfRow(pos.y) < 0)
	{
		if (SendEvent(wxEVT_GRID_CELL_LEFT_DCLICK, coords, event) == Event_Unhandled)
		{
			// we want double click to select a cell and start editing
			// (i.e. to behave in same way as sequence of two slow clicks):
			m_waitForSlowClick = true;
		}
	}
}

void
wxGridExt::DoGridCellLeftUp(wxMouseEvent& event,
	const wxGridExtCellCoords& coords,
	wxGridExtWindow* gridWindow)
{
	if (m_cursorMode == WXGRID_CURSOR_SELECT_CELL)
	{
		if (coords == m_currentCellCoords && m_waitForSlowClick && CanEnableCellControl())
		{
			ClearSelection();

			if (DoEnableCellEditControl(wxGridExtActivationSource::From(event)))
				GetCurrentCellEditorPtr()->StartingClick();

			m_waitForSlowClick = false;
		}
	}
	else if (m_cursorMode == WXGRID_CURSOR_RESIZE_ROW)
	{
		ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL);
		if (m_dragRowOrCol != -1)
			DoEndDragResizeRow(event, gridWindow);
	}
	else if (m_cursorMode == WXGRID_CURSOR_RESIZE_COL)
	{
		ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL);
		if (m_dragRowOrCol != -1)
			DoEndDragResizeCol(event, gridWindow);
	}

	m_dragLastPos = -1;
}

void
wxGridExt::DoGridMouseMoveEvent(wxMouseEvent& WXUNUSED(event),
	const wxGridExtCellCoords& coords,
	const wxPoint& pos,
	wxGridExtWindow* gridWindow)
{
	if (coords.GetRow() < 0 || coords.GetCol() < 0)
	{
		// out of grid cell area
		ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL);
		return;
	}

	int dragRow = YToEdgeOfRow(pos.y);
	int dragCol = XToEdgeOfCol(pos.x);

	// Dragging on the corner of a cell to resize in both directions is not
	// implemented, so choose to resize the column when the cursor is over the
	// cell corner, as this is a more common operation.
	if (dragCol >= 0 && CanDragGridColEdges() && CanDragColSize(dragCol))
	{
		if (m_cursorMode != WXGRID_CURSOR_RESIZE_COL)
		{
			ChangeCursorMode(WXGRID_CURSOR_RESIZE_COL, gridWindow, false);
		}
	}
	else if (dragRow >= 0 && CanDragGridRowEdges() && CanDragRowSize(dragRow))
	{
		if (m_cursorMode != WXGRID_CURSOR_RESIZE_ROW)
		{
			ChangeCursorMode(WXGRID_CURSOR_RESIZE_ROW, gridWindow, false);
		}
	}
	else // Neither on a row or col edge
	{
		if (m_cursorMode != WXGRID_CURSOR_SELECT_CELL)
		{
			ChangeCursorMode(WXGRID_CURSOR_SELECT_CELL, gridWindow, false);
		}
	}
}

void wxGridExt::ProcessGridCellMouseEvent(wxMouseEvent& event, wxGridExtWindow* eventGridWindow)
{
	// the window receiving the event might not be the same as the one under
	// the mouse (e.g. in the case of a dragging event started in one window,
	// but continuing over another one)

	if (CheckIfDragCancelled(&event))
		return;

	wxGridExtWindow* gridWindow =
		DevicePosToGridWindow(event.GetPosition() + eventGridWindow->GetPosition());

	if (!gridWindow)
		gridWindow = eventGridWindow;

	// store position, before it's modified in the next step
	const wxPoint posEvent = event.GetPosition();

	event.SetPosition(posEvent + eventGridWindow->GetPosition() -
		wxPoint((GridRowAreaEnabled() ? m_rowAreaWidth : 0) + m_rowLabelWidth, (GridColAreaEnabled() ? m_colAreaHeight : 0) + m_colLabelHeight));

	wxPoint pos = CalcGridWindowUnscrolledPosition(event.GetPosition(), gridWindow);

	// coordinates of the cell under mouse
	wxGridExtCellCoords coords = XYToCell(pos, gridWindow);

	int cell_rows, cell_cols;
	if (GetCellSize(coords.GetRow(), coords.GetCol(), &cell_rows, &cell_cols)
		== CellSpan_Inside)
	{
		coords.SetRow(coords.GetRow() + cell_rows);
		coords.SetCol(coords.GetCol() + cell_cols);
	}

	// Releasing the left mouse button must be processed in any case, so deal
	// with it first.
	if (event.LeftUp())
	{
		// Note that we must call this one first, before resetting the
		// drag-related data, as it relies on m_cursorMode being still set and
		// EndDraggingIfNecessary() resets it.
		DoGridCellLeftUp(event, coords, gridWindow);

		EndDraggingIfNecessary();
		return;
	}

	const bool isDraggingWithLeft = event.Dragging() && event.LeftIsDown();

	if (isDraggingWithLeft && (m_winCapture == eventGridWindow))
	{
		// scroll when at the edges or outside the window
		CheckDoDragScroll(eventGridWindow, m_gridWin, posEvent,
			wxHORIZONTAL | wxVERTICAL);
	}

	// While dragging the mouse, only releasing the left mouse button, which
	// cancels the drag operation, is processed (above) and any other events
	// are just ignored while it's in progress.
	if (m_isDragging)
	{
		if (isDraggingWithLeft)
			DoGridDragEvent(event, coords, false /* not first drag */, gridWindow);

		if (m_winCapture != gridWindow)
		{
			if (m_winCapture)
				m_winCapture->ReleaseMouse();

			m_winCapture = gridWindow;
			m_winCapture->CaptureMouse();
		}
		return;
	}

	// Now check if we're starting a drag operation (if it had been already
	// started, m_isDragging would be true above).
	if (isDraggingWithLeft)
	{
		// To avoid accidental drags, don't start doing anything until the
		// mouse has been dragged far enough.
		const wxPoint& pt = event.GetPosition();
		if (m_startDragPos == wxDefaultPosition)
		{
			m_startDragPos = pt;
			return;
		}

		if (abs(m_startDragPos.x - pt.x) <= DRAG_SENSITIVITY &&
			abs(m_startDragPos.y - pt.y) <= DRAG_SENSITIVITY)
			return;

		if (DoGridDragEvent(event, coords, true /* first drag */, gridWindow))
		{
			wxASSERT_MSG(!m_winCapture, "shouldn't capture the mouse twice");

			m_winCapture = gridWindow;
			m_winCapture->CaptureMouse();

			m_isDragging = true;
		}

		return;
	}

	// If we're not dragging, cancel any dragging operation which could have
	// been in progress.
	EndDraggingIfNecessary();

	bool handled = false;

	// deal with various button presses
	if (event.IsButton())
	{
		if (coords != wxGridExtNoCellCoords)
		{
			DisableCellEditControl();

			if (event.LeftDown())
				handled = (DoGridCellLeftDown(event, coords, pos), true);
			else if (event.LeftDClick())
				handled = (DoGridCellLeftDClick(event, coords, pos), true);
			else if (event.RightDown())
				handled = SendEvent(wxEVT_GRID_CELL_RIGHT_CLICK, coords, event) != Event_Unhandled;
			else if (event.RightDClick())
				handled = SendEvent(wxEVT_GRID_CELL_RIGHT_DCLICK, coords, event) != Event_Unhandled;
		}
	}
	else if (event.Moving())
	{
		DoGridMouseMoveEvent(event, coords, pos, gridWindow);
		handled = true;
	}

	if (!handled)
	{
		event.Skip();
	}
}

void wxGridExt::DoGridDragResize(const wxPoint& position,
	const wxGridExtOperations& oper,
	wxGridExtWindow* gridWindow)
{
	wxCHECK_RET(m_dragRowOrCol != -1,
		"shouldn't be called when not drag resizing");

	// Get the logical position from the physical one we're passed.
	const wxPoint
		logicalPos = CalcGridWindowUnscrolledPosition(position, gridWindow);

	// Size of the row/column is determined by the mouse coordinates in the
	// orthogonal direction.
	const int linePos = oper.Dual().Select(logicalPos);

	const int lineStart = oper.GetLineStartPos(this, m_dragRowOrCol);
	oper.SetLineSize(this, m_dragRowOrCol,
		wxMax(linePos - lineStart,
			oper.GetMinimalLineSize(this, m_dragRowOrCol)));

	// TODO: generate RESIZING event, see #10754, if the size has changed.
}

wxPoint wxGridExt::GetPositionForResizeEvent(int width) const
{
	wxCHECK_MSG(m_dragRowOrCol != -1, wxPoint(),
		"shouldn't be called when not drag resizing");

	// Note that we currently always use m_gridWin here as using
	// wxGridExtHeaderCtrl is incompatible with using frozen rows/columns.
	// This would need to be changed if they're allowed to be used together.
	int x;
	CalcGridWindowScrolledPosition(GetColLeft(m_dragRowOrCol) + width, 0,
		&x, NULL,
		m_gridWin);

	return wxPoint(x, 0);
}

void wxGridExt::DoEndDragResizeRow(const wxMouseEvent& event, wxGridExtWindow* gridWindow)
{
	DoGridDragResize(event.GetPosition(), wxGridExtRowOperations(), gridWindow);

	SendGridSizeEvent(wxEVT_GRID_ROW_SIZE, m_dragRowOrCol, event);

	m_dragRowOrCol = -1;
}

void wxGridExt::DoEndDragResizeCol(const wxMouseEvent& event, wxGridExtWindow* gridWindow)
{
	DoGridDragResize(event.GetPosition(), wxGridExtColumnOperations(), gridWindow);

	SendGridSizeEvent(wxEVT_GRID_COL_SIZE, m_dragRowOrCol, event);

	m_dragRowOrCol = -1;
}

void wxGridExt::DoHeaderStartDragResizeCol(int col)
{
	DoStartResizeRowOrCol(col, GetColSize(col));
}

void wxGridExt::DoHeaderDragResizeCol(int width)
{
	DoGridDragResize(GetPositionForResizeEvent(width),
		wxGridExtColumnOperations(),
		m_gridWin);
}

void wxGridExt::DoHeaderEndDragResizeCol(int width)
{
	// We can sometimes be called even when we're not resizing any more,
	// although it's rather difficult to reproduce: one way to do it is to
	// double click the column separator line while pressing Esc at the same
	// time. There seems to be some kind of check for Esc inside the native
	// header control and so an extra "end resizing" message gets generated.
	if (m_dragRowOrCol == -1)
		return;

	// Unfortunately we need to create a dummy mouse event here to avoid
	// modifying too much existing code. Note that only position and keyboard
	// state parts of this event object are actually used, so the rest
	// (even including some crucial parts, such as event type) can be left
	// uninitialized.
	wxMouseEvent e;
	e.SetState(wxGetMouseState());
	e.SetPosition(GetPositionForResizeEvent(width));

	DoEndDragResizeCol(e, m_gridWin);
}

void wxGridExt::DoStartMoveRowOrCol(int col)
{
	m_dragMoveRowOrCol = col;
}

void wxGridExt::DoEndMoveRow(int pos)
{
	wxASSERT_MSG(m_dragMoveRowOrCol != -1, "no matching DoStartMoveRow?");

	if (SendEvent(wxEVT_GRID_ROW_MOVE, m_dragMoveRowOrCol, -1) != Event_Vetoed)
		SetRowPos(m_dragMoveRowOrCol, pos);

	m_dragMoveRowOrCol = -1;
}

void wxGridExt::RefreshAfterRowPosChange()
{
	// recalculate the row bottoms as the row positions have changed,
	// unless we calculate them dynamically because all rows heights are the
	// same and it's easy to do
	if (!m_rowHeights.empty())
	{
		int rowBottom = 0;
		for (int rowPos = 0; rowPos < m_numRows; rowPos++)
		{
			int rowID = GetRowAt(rowPos);

			// Ignore the currently hidden rows.
			const int height = m_rowHeights[rowID];
			if (height > 0)
				rowBottom += height;

			m_rowBottoms[rowID] = rowBottom;
		}
	}

	// and make the changes visible
	m_rowAreaWin->Refresh();
	m_rowLabelWin->Refresh();
	m_gridWin->Refresh();
}

void wxGridExt::SetRowsOrder(const wxArrayInt& order)
{
	m_rowAt = order;

	RefreshAfterRowPosChange();
}

void wxGridExt::SetRowPos(int idx, int pos)
{
	// we're going to need m_rowAt now, initialize it if needed
	if (m_rowAt.empty())
	{
		m_rowAt.reserve(m_numRows);
		for (int i = 0; i < m_numRows; i++)
			m_rowAt.push_back(i);
	}

	// from wxHeaderCtrl::MoveRowInOrderArray:
	int posOld = m_rowAt.Index(idx);
	wxASSERT_MSG(posOld != wxNOT_FOUND, "invalid index");

	if (pos != posOld)
	{
		m_rowAt.RemoveAt(posOld);
		m_rowAt.Insert(idx, pos);
	}

	RefreshAfterRowPosChange();
}

int wxGridExt::GetRowPos(int idx) const
{
	wxASSERT_MSG(idx >= 0 && idx < m_numRows, "invalid row index");

	if (m_rowAt.IsEmpty())
		return idx;

	int pos = m_rowAt.Index(idx);
	wxASSERT_MSG(pos != wxNOT_FOUND, "invalid row index");

	return pos;
}

void wxGridExt::ResetRowPos()
{
	m_rowAt.clear();

	RefreshAfterRowPosChange();
}

bool wxGridExt::EnableDragRowMove(bool enable)
{
	if (m_canDragRowMove == enable ||
		(enable && m_rowFrozenLabelWin))
		return false;

	m_canDragRowMove = enable;

	// we use to call ResetRowPos() from here if !enable but this doesn't seem
	// right as it would mean there would be no way to "freeze" the current
	// rows order by disabling moving them after putting them in the desired
	// order, whereas now you can always call ResetRowPos() manually if needed
	return true;
}

void wxGridExt::DoEndMoveCol(int pos)
{
	wxASSERT_MSG(m_dragMoveRowOrCol != -1, "no matching DoStartMoveCol?");

	if (SendEvent(wxEVT_GRID_COL_MOVE, -1, m_dragMoveRowOrCol) != Event_Vetoed)
		SetColPos(m_dragMoveRowOrCol, pos);

	m_dragMoveRowOrCol = -1;
}

void wxGridExt::RefreshAfterColPosChange()
{
	// recalculate the column rights as the column positions have changed,
	// unless we calculate them dynamically because all columns widths are the
	// same and it's easy to do
	if (!m_colWidths.empty())
	{
		int colRight = 0;
		for (int colPos = 0; colPos < m_numCols; colPos++)
		{
			int colID = GetColAt(colPos);

			// Ignore the currently hidden columns.
			const int width = m_colWidths[colID];
			if (width > 0)
				colRight += width;

			m_colRights[colID] = colRight;
		}
	}

	// and make the changes visible
	if (m_useNativeHeader)
	{
		m_colAreaWin->Refresh();
		SetNativeHeaderColOrder();
	}
	else
	{
		m_colAreaWin->Refresh();
		m_colLabelWin->Refresh();
	}
	m_gridWin->Refresh();
}

void wxGridExt::SetColumnsOrder(const wxArrayInt& order)
{
	m_colAt = order;

	RefreshAfterColPosChange();
}

void wxGridExt::SetColPos(int idx, int pos)
{
	// we're going to need m_colAt now, initialize it if needed
	if (m_colAt.empty())
	{
		m_colAt.reserve(m_numCols);
		for (int i = 0; i < m_numCols; i++)
			m_colAt.push_back(i);
	}

	wxHeaderCtrl::MoveColumnInOrderArray(m_colAt, idx, pos);

	RefreshAfterColPosChange();
}

int wxGridExt::GetColPos(int idx) const
{
	wxASSERT_MSG(idx >= 0 && idx < m_numCols, "invalid column index");

	if (m_colAt.IsEmpty())
		return idx;

	int pos = m_colAt.Index(idx);
	wxASSERT_MSG(pos != wxNOT_FOUND, "invalid column index");

	return pos;
}

void wxGridExt::ResetColPos()
{
	m_colAt.clear();

	RefreshAfterColPosChange();
}

bool wxGridExt::EnableDragColMove(bool enable)
{
	if (m_canDragColMove == enable ||
		(enable && m_colFrozenLabelWin))
		return false;

	if (m_useNativeHeader)
	{
		wxHeaderCtrl* header = GetGridColHeader();
		long setFlags = header->GetWindowStyleFlag();

		if (enable)
			header->SetWindowStyleFlag(setFlags | wxHD_ALLOW_REORDER);
		else
			header->SetWindowStyleFlag(setFlags & ~wxHD_ALLOW_REORDER);
	}

	m_canDragColMove = enable;

	// we use to call ResetColPos() from here if !enable but this doesn't seem
	// right as it would mean there would be no way to "freeze" the current
	// columns order by disabling moving them after putting them in the desired
	// order, whereas now you can always call ResetColPos() manually if needed
	return true;
}

bool wxGridExt::EnableHidingColumns(bool enable)
{
	if (m_canHideColumns == enable || !m_useNativeHeader)
		return false;

	GetGridColHeader()->ToggleWindowStyle(wxHD_ALLOW_HIDE);

	m_canHideColumns = enable;

	return true;
}

void wxGridExt::InitializeFrozenWindows()
{
	// frozen row windows
	if (m_numFrozenRows > 0 && !m_frozenRowGridWin)
	{
		m_frozenRowGridWin = new wxGridExtWindow(this, wxGridExtWindow::wxGridExtWindowFrozenRow);
		m_frozenRowGridWin->SetOwnForegroundColour(m_gridWin->GetForegroundColour());
		m_frozenRowGridWin->SetOwnBackgroundColour(m_gridWin->GetBackgroundColour());

		m_rowFrozenAreaWin = new wxGridExtRowFrozenAreaWindow(this);
		m_rowFrozenAreaWin->SetOwnForegroundColour(m_labelTextColour);
		m_rowFrozenAreaWin->SetOwnBackgroundColour(m_labelBackgroundColour);

		m_rowFrozenLabelWin = new wxGridExtRowFrozenLabelWindow(this);
		m_rowFrozenLabelWin->SetOwnForegroundColour(m_labelTextColour);
		m_rowFrozenLabelWin->SetOwnBackgroundColour(m_labelBackgroundColour);
	}
	else if (m_numFrozenRows == 0 && m_frozenRowGridWin)
	{
		delete m_frozenRowGridWin;
		delete m_rowFrozenAreaWin;
		delete m_rowFrozenLabelWin;
		m_frozenRowGridWin = NULL;
		m_rowFrozenAreaWin = NULL;
		m_rowFrozenLabelWin = NULL;
	}

	// frozen column windows
	if (m_numFrozenCols > 0 && !m_frozenColGridWin)
	{
		m_frozenColGridWin = new wxGridExtWindow(this, wxGridExtWindow::wxGridExtWindowFrozenCol);
		m_frozenColGridWin->SetOwnForegroundColour(m_gridWin->GetForegroundColour());
		m_frozenColGridWin->SetOwnBackgroundColour(m_gridWin->GetBackgroundColour());

		m_colFrozenAreaWin = new wxGridExtColFrozenAreaWindow(this);
		m_colFrozenAreaWin->SetOwnForegroundColour(m_labelTextColour);
		m_colFrozenAreaWin->SetOwnBackgroundColour(m_labelBackgroundColour);

		m_colFrozenLabelWin = new wxGridExtColFrozenLabelWindow(this);
		m_colFrozenLabelWin->SetOwnForegroundColour(m_labelTextColour);
		m_colFrozenLabelWin->SetOwnBackgroundColour(m_labelBackgroundColour);
	}
	else if (m_numFrozenCols == 0 && m_frozenColGridWin)
	{
		delete m_frozenColGridWin;
		delete m_colFrozenAreaWin;
		delete m_colFrozenLabelWin;

		m_frozenColGridWin = NULL;
		m_colFrozenAreaWin = NULL;
		m_colFrozenLabelWin = NULL;
	}

	// frozen corner window
	if (m_numFrozenRows > 0 && m_numFrozenCols > 0 && !m_frozenCornerGridWin)
	{
		m_frozenCornerGridWin = new wxGridExtWindow(this, wxGridExtWindow::wxGridExtWindowFrozenCorner);

		m_frozenCornerGridWin->SetOwnForegroundColour(m_gridWin->GetForegroundColour());
		m_frozenCornerGridWin->SetOwnBackgroundColour(m_gridWin->GetBackgroundColour());
	}
	else if ((m_numFrozenRows == 0 || m_numFrozenCols == 0) && m_frozenCornerGridWin)
	{
		delete m_frozenCornerGridWin;
		m_frozenCornerGridWin = NULL;
	}
}

bool wxGridExt::FreezeTo(int row, int col)
{
	wxCHECK_MSG(row >= 0 && col >= 0, false,
		"Number of rows or cols can't be negative!");

	if (row >= m_numRows || col >= m_numCols ||
		!m_rowAt.empty() || m_canDragRowMove ||
		!m_colAt.empty() || m_canDragColMove || m_useNativeHeader)
		return false;

	// freeze
	if (row > m_numFrozenRows || col > m_numFrozenCols)
	{
		// check that it fits in client size
		int cw, ch;
		GetClientSize(&cw, &ch);

		cw -= (GridRowAreaEnabled() ? m_rowAreaWidth : 0) + m_rowLabelWidth;
		ch -= (GridColAreaEnabled() ? m_colAreaHeight : 0) + m_colLabelHeight;

		if ((row > 0 && GetRowBottom(row - 1) >= ch) ||
			(col > 0 && GetColRight(col - 1) >= cw))
			return false;

		// check all involved cells for merged ones
		int cell_rows, cell_cols;

		for (int i = m_numFrozenRows; i < row; i++)
		{
			for (int j = 0; j < m_numCols; j++)
			{
				GetCellSize(GetRowAt(i), GetColAt(j), &cell_rows, &cell_cols);

				if ((cell_rows > 1) || (cell_cols > 1))
					return false;
			}
		}

		for (int i = m_numFrozenCols; i < col; i++)
		{
			for (int j = 0; j < m_numRows; j++)
			{
				GetCellSize(GetRowAt(j), GetColAt(i), &cell_rows, &cell_cols);

				if ((cell_rows > 1) || (cell_cols > 1))
					return false;
			}
		}
	}

	m_numFrozenRows = row;
	m_numFrozenCols = col;

	HideCellEditControl();

	InitializeFrozenWindows();

	// recompute dimensions
	InvalidateBestSize();

	CalcDimensions();

	if (ShouldRefresh())
		Refresh();

	return true;
}

bool wxGridExt::IsFrozen() const
{
	return m_numFrozenRows || m_numFrozenCols;
}

void wxGridExt::UpdateGridWindows() const
{
	m_gridWin->Update();

	if (m_frozenCornerGridWin)
		m_frozenCornerGridWin->Update();

	if (m_frozenRowGridWin)
		m_frozenRowGridWin->Update();

	if (m_frozenColGridWin)
		m_frozenColGridWin->Update();
}

//
// ------ interaction with data model
//
bool wxGridExt::ProcessTableMessage(wxGridExtTableMessage& msg)
{
	switch (msg.GetId())
	{
	case wxGRIDTABLE_NOTIFY_ROWS_INSERTED:
	case wxGRIDTABLE_NOTIFY_ROWS_APPENDED:
	case wxGRIDTABLE_NOTIFY_ROWS_DELETED:
	case wxGRIDTABLE_NOTIFY_COLS_INSERTED:
	case wxGRIDTABLE_NOTIFY_COLS_APPENDED:
	case wxGRIDTABLE_NOTIFY_COLS_DELETED:
		return Redimension(msg);

	default:
		return false;
	}
}

// The behaviour of this function depends on the grid table class
// Clear() function. For the default wxGridExtStringTable class the
// behaviour is to replace all cell contents with wxEmptyString but
// not to change the number of rows or cols.
//
void wxGridExt::ClearGrid()
{
	if (m_table)
	{
		DisableCellEditControl();

		m_table->Clear();
		if (ShouldRefresh())
			m_gridWin->Refresh();
	}
}

bool
wxGridExt::DoModifyLines(bool (wxGridExtTableBase::* funcModify)(size_t, size_t),
	int pos, int num, bool WXUNUSED(updateLabels))
{
	wxCHECK_MSG(m_created, false, "must finish creating the grid first");

	if (!m_table)
		return false;

	DisableCellEditControl();

	return (m_table->*funcModify)(pos, num);

	// the table will have sent the results of the insert row
	// operation to this view object as a grid table message
}

bool
wxGridExt::DoAppendLines(bool (wxGridExtTableBase::* funcAppend)(size_t),
	int num, bool WXUNUSED(updateLabels))
{
	wxCHECK_MSG(m_created, false, "must finish creating the grid first");

	if (!m_table)
		return false;

	return (m_table->*funcAppend)(num);
}

// ----------------------------------------------------------------------------
// event generation helpers
// ----------------------------------------------------------------------------

bool
wxGridExt::SendGridSizeEvent(wxEventType type,
	int rowOrCol,
	const wxMouseEvent& mouseEv)
{
	wxGridExtSizeEvent gridEvt(GetId(),
		type,
		this,
		rowOrCol,
		mouseEv.GetX() + GetRowLabelSize(),
		mouseEv.GetY() + GetColLabelSize(),
		mouseEv);

	return ProcessWindowEvent(gridEvt);
}

wxGridExt::EventResult wxGridExt::DoSendEvent(wxGridExtEvent& gridEvt)
{
	const bool claimed = ProcessWindowEvent(gridEvt);

	// A Veto'd event may not be `claimed' so test this first
	if (!gridEvt.IsAllowed())
		return Event_Vetoed;

	// Detect the special case in which the event cell was deleted, as this
	// allows to have checks in several functions that generate an event and
	// then proceed doing something by default with the selected cell: this
	// shouldn't be done if the user-defined handler deleted this cell.
	if (gridEvt.GetRow() >= GetNumberRows() ||
		gridEvt.GetCol() >= GetNumberCols())
		return Event_CellDeleted;

	return claimed ? Event_Handled : Event_Unhandled;
}

// Generate a grid event based on a mouse event and call DoSendEvent() with it.
wxGridExt::EventResult
wxGridExt::SendEvent(wxEventType type,
	int row, int col,
	const wxMouseEvent& mouseEv)
{

	if (type == wxEVT_GRID_LABEL_LEFT_CLICK ||
		type == wxEVT_GRID_LABEL_LEFT_DCLICK ||
		type == wxEVT_GRID_LABEL_RIGHT_CLICK ||
		type == wxEVT_GRID_LABEL_RIGHT_DCLICK)
	{
		wxPoint pos = mouseEv.GetPosition();

		if (mouseEv.GetEventObject() == GetGridRowLabelWindow())
			pos.y += GetColLabelSize();
		if (mouseEv.GetEventObject() == GetGridColLabelWindow())
			pos.x += GetRowLabelSize();

		wxGridExtEvent gridEvt(GetId(),
			type,
			this,
			row, col,
			pos.x,
			pos.y,
			false,
			mouseEv);

		return DoSendEvent(gridEvt);
	}
	else
	{
		wxGridExtEvent gridEvt(GetId(),
			type,
			this,
			row, col,
			mouseEv.GetX() + GetRowLabelSize(),
			mouseEv.GetY() + GetColLabelSize(),
			false,
			mouseEv);

		if (type == wxEVT_GRID_CELL_BEGIN_DRAG)
		{
			// by default the dragging is not supported, the user code must
			// explicitly allow the event for it to take place
			gridEvt.Veto();
		}

		return DoSendEvent(gridEvt);
	}
}

// Generate a grid event of specified type, return value same as above
//
wxGridExt::EventResult
wxGridExt::SendEvent(wxEventType type, int row, int col, const wxString& s)
{
	wxGridExtEvent gridEvt(GetId(), type, this, row, col);
	gridEvt.SetString(s);

	return DoSendEvent(gridEvt);
}

void wxGridExt::Refresh(bool eraseb, const wxRect* rect)
{
	// Don't do anything if between Begin/EndBatch...
	// EndBatch() will do all this on the last nested one anyway.
	if (m_created && ShouldRefresh())
	{
		// Refresh to get correct scrolled position:
		wxScrolledCanvas::Refresh(eraseb, rect);

		if (rect)
		{
			int rect_x, rect_y, rectWidth, rectHeight;
			int width_label, width_cell, height_label, height_cell;
			int x, y;

			// Copy rectangle can get scroll offsets..
			rect_x = rect->GetX();
			rect_y = rect->GetY();
			rectWidth = rect->GetWidth();
			rectHeight = rect->GetHeight();

			width_label = (GridRowAreaEnabled() ? m_rowAreaWidth : 0) + m_rowLabelWidth - rect_x;
			if (width_label > rectWidth)
				width_label = rectWidth;

			height_label = (GridColAreaEnabled() ? m_colAreaHeight : 0) + m_colLabelHeight - rect_y;
			if (height_label > rectHeight)
				height_label = rectHeight;

			if (rect_x > (GridRowAreaEnabled() ? m_rowAreaWidth : 0) + m_rowLabelWidth)
			{
				x = rect_x - (GridRowAreaEnabled() ? m_rowAreaWidth : 0) + m_rowLabelWidth;
				width_cell = rectWidth;
			}
			else
			{
				x = 0;
				width_cell = rectWidth - ((GridRowAreaEnabled() ? m_rowAreaWidth : 0) + m_rowLabelWidth - rect_x);
			}

			if (rect_y > (GridColAreaEnabled() ? m_colAreaHeight : 0) + m_colLabelHeight)
			{
				y = rect_y - m_colAreaHeight + m_colLabelHeight;
				height_cell = rectHeight;
			}
			else
			{
				y = 0;
				height_cell = rectHeight - ((GridColAreaEnabled() ? m_colAreaHeight : 0) + m_colLabelHeight - rect_y);
			}

			// Paint corner label part intersecting rect.
			if (width_label > 0 && height_label > 0)
			{
				wxRect anotherrect(rect_x, rect_y, width_label, height_label);
				m_cornerLabelWin->Refresh(eraseb, &anotherrect);
			}

			// Paint col area part intersecting rect.
			if (width_cell > 0 && height_label > 0)
			{
				wxRect anotherrect(x, rect_y, width_cell, height_label);
				m_colAreaWin->Refresh(eraseb, &anotherrect);
			}

			// Paint col labels part intersecting rect.
			if (width_cell > 0 && height_label > 0)
			{
				wxRect anotherrect(x, rect_y, width_cell, height_label);
				m_colLabelWin->Refresh(eraseb, &anotherrect);
			}

			// Paint row area part intersecting rect.
			if (width_label > 0 && height_cell > 0)
			{
				wxRect anotherrect(rect_x, y, width_label, height_cell);
				m_rowAreaWin->Refresh(eraseb, &anotherrect);
			}

			// Paint row labels part intersecting rect.
			if (width_label > 0 && height_cell > 0)
			{
				wxRect anotherrect(rect_x, y, width_label, height_cell);
				m_rowLabelWin->Refresh(eraseb, &anotherrect);
			}

			// Paint cell area part intersecting rect.
			if (width_cell > 0 && height_cell > 0)
			{
				wxRect anotherrect(x, y, width_cell, height_cell);
				m_gridWin->Refresh(eraseb, &anotherrect);
			}
		}
		else
		{
			m_cornerLabelWin->Refresh(eraseb, NULL);
			m_colAreaWin->Refresh(eraseb, NULL);
			m_colLabelWin->Refresh(eraseb, NULL);
			m_rowAreaWin->Refresh(eraseb, NULL);
			m_rowLabelWin->Refresh(eraseb, NULL);
			m_gridWin->Refresh(eraseb, NULL);

			if (m_frozenColGridWin)
			{
				m_frozenColGridWin->Refresh(eraseb, NULL);
				m_colFrozenAreaWin->Refresh(eraseb, NULL);
				m_colFrozenLabelWin->Refresh(eraseb, NULL);
			}
			if (m_frozenRowGridWin)
			{
				m_frozenRowGridWin->Refresh(eraseb, NULL);
				m_rowFrozenAreaWin->Refresh(eraseb, NULL);
				m_rowFrozenLabelWin->Refresh(eraseb, NULL);
			}
			if (m_frozenCornerGridWin)
				m_frozenCornerGridWin->Refresh(eraseb, NULL);
		}
	}
}

void wxGridExt::RefreshBlock(const wxGridExtCellCoords& topLeft,
	const wxGridExtCellCoords& bottomRight)
{
	RefreshBlock(topLeft.GetRow(), topLeft.GetCol(),
		bottomRight.GetRow(), bottomRight.GetCol());
}

void wxGridExt::RefreshBlock(int topRow, int leftCol,
	int bottomRow, int rightCol)
{
	// Note that it is valid to call this function with wxGridExtNoCellCoords as
	// either or even both arguments, but we can't have a mix of valid and
	// invalid columns/rows for each corner coordinates.
	const bool noTopLeft = topRow == -1 || leftCol == -1;
	const bool noBottomRight = bottomRow == -1 || rightCol == -1;

	if (noTopLeft)
	{
		// So check that either both or none of the components are valid.
		wxASSERT(topRow == -1 && leftCol == -1);

		// And specifying bottom right corner when the top left one is not
		// specified doesn't make sense either.
		wxASSERT(noBottomRight);

		return;
	}

	if (noBottomRight)
	{
		wxASSERT(bottomRow == -1 && rightCol == -1);

		bottomRow = topRow;
		rightCol = leftCol;
	}


	int row = topRow;
	int col = leftCol;

	// corner grid
	if (GetRowPos(topRow) < m_numFrozenRows && GetColPos(leftCol) < m_numFrozenCols && m_frozenCornerGridWin)
	{
		row = wxMin(bottomRow, m_numFrozenRows - 1);
		col = wxMin(rightCol, m_numFrozenCols - 1);

		wxRect rect = BlockToDeviceRect(wxGridExtCellCoords(topRow, leftCol),
			wxGridExtCellCoords(row, col),
			m_frozenCornerGridWin);
		m_frozenCornerGridWin->Refresh(false, &rect);
		row++; col++;
	}

	// frozen cols grid
	if (GetColPos(leftCol) < m_numFrozenCols && GetRowPos(bottomRow) >= m_numFrozenRows && m_frozenColGridWin)
	{
		col = wxMin(rightCol, m_numFrozenCols - 1);

		wxRect rect = BlockToDeviceRect(wxGridExtCellCoords(row, leftCol),
			wxGridExtCellCoords(bottomRow, col),
			m_frozenColGridWin);
		m_frozenColGridWin->Refresh(false, &rect);
		col++;
	}

	// frozen rows grid
	if (GetRowPos(topRow) < m_numFrozenRows && GetColPos(rightCol) >= m_numFrozenCols && m_frozenRowGridWin)
	{
		row = wxMin(bottomRow, m_numFrozenRows - 1);

		wxRect rect = BlockToDeviceRect(wxGridExtCellCoords(topRow, col),
			wxGridExtCellCoords(row, rightCol),
			m_frozenRowGridWin);
		m_frozenRowGridWin->Refresh(false, &rect);
		row++;
	}

	// main grid
	if (GetRowPos(bottomRow) >= m_numFrozenRows && GetColPos(rightCol) >= m_numFrozenCols)
	{
		const wxRect rect = BlockToDeviceRect(wxGridExtCellCoords(row, col),
			wxGridExtCellCoords(bottomRow, rightCol),
			m_gridWin);
		if (!rect.IsEmpty())
			m_gridWin->Refresh(false, &rect);
	}
}

void wxGridExt::OnSize(wxSizeEvent& event)
{
	if (m_targetWindow != this) // check whether initialisation has been done
	{
		// reposition our children windows
		CalcWindowSizes();
		event.Skip();
	}
}

void wxGridExt::OnDPIChanged(wxDPIChangedEvent& event)
{
	InitPixelFields();

	// If we have any non-default row sizes, we need to scale them (default
	// ones will be scaled due to the reinitialization of m_defaultRowHeight
	// inside InitPixelFields() above).
	if (!m_rowHeights.empty())
	{
		int total = 0;
		for (unsigned i = 0; i < m_rowHeights.size(); ++i)
		{
			int height = m_rowHeights[i];

			// Skip hidden rows.
			if (height <= 0)
				continue;

			height = event.ScaleY(height);
			total += height;

			m_rowHeights[i] = height;
			m_rowBottoms[i] = total;
		}
	}

	// Similarly for columns, except that here we need to update the native
	// control even if none of the widths had been changed, as it's not going
	// to do it on its own when redisplayed.
	wxHeaderCtrl* const
		colHeader = m_useNativeHeader ? GetGridColHeader() : NULL;
	if (!m_colWidths.empty())
	{
		int total = 0;
		for (unsigned i = 0; i < m_colWidths.size(); ++i)
		{
			int width = m_colWidths[i];

			if (width <= 0)
				continue;

			width = event.ScaleX(width);
			total += width;

			m_colWidths[i] = width;
			m_colRights[i] = total;

			if (colHeader)
				colHeader->UpdateColumn(i);
		}
	}
	else if (colHeader)
	{
		for (int i = 0; i < m_numCols; ++i)
		{
			colHeader->UpdateColumn(i);
		}
	}

	InvalidateBestSize();

	CalcDimensions();

	event.Skip();
}

void wxGridExt::OnKeyDown(wxKeyEvent& event)
{
	// propagate the event up and see if it gets processed
	//wxWindow* parent = GetParent();
	//wxKeyEvent keyEvt(event);
	//keyEvt.SetEventObject(parent);

	//if (!parent->ProcessWindowEvent(keyEvt))
	{
		if (GetLayoutDirection() == wxLayout_RightToLeft)
		{
			if (event.GetKeyCode() == WXK_RIGHT)
				event.m_keyCode = WXK_LEFT;
			else if (event.GetKeyCode() == WXK_LEFT)
				event.m_keyCode = WXK_RIGHT;
		}

		// try local handlers
		switch (event.GetKeyCode())
		{
		case WXK_UP:
			DoMoveCursorFromKeyboard
			(
				event,
				wxGridExtBackwardOperations(this, wxGridExtRowOperations())
			);
			break;

		case WXK_DOWN:
			DoMoveCursorFromKeyboard
			(
				event,
				wxGridExtForwardOperations(this, wxGridExtRowOperations())
			);
			break;

		case WXK_LEFT:
			DoMoveCursorFromKeyboard
			(
				event,
				wxGridExtBackwardOperations(this, wxGridExtColumnOperations())
			);
			break;

		case WXK_RIGHT:
			DoMoveCursorFromKeyboard
			(
				event,
				wxGridExtForwardOperations(this, wxGridExtColumnOperations())
			);
			break;

		case WXK_RETURN:
		case WXK_NUMPAD_ENTER:
			if (event.ControlDown())
			{
				event.Skip();  // to let the edit control have the return
			}
			else
			{
				// We want to accept the changes in the editor when Enter
				// is pressed in any case, so do it (note that in many
				// cases this would be done by MoveCursorDown() itself, but
				// not always, e.g. it wouldn't do it when editing the
				// cells in the last row or when using Shift-Enter).
				DisableCellEditControl();

				MoveCursorDown(event.ShiftDown());
			}
			break;

		case WXK_ESCAPE:
			if (m_isDragging && m_winCapture)
			{
				switch (m_cursorMode)
				{
				case WXGRID_CURSOR_MOVE_COL:
				case WXGRID_CURSOR_MOVE_ROW:
					// end row/column moving
					m_winCapture->Refresh();
					m_dragLastPos = -1;
					m_dragLastColour = NULL;
					break;

				case WXGRID_CURSOR_RESIZE_ROW:
				case WXGRID_CURSOR_RESIZE_COL:
					// reset to size from before dragging
					(m_cursorMode == WXGRID_CURSOR_RESIZE_ROW
						? static_cast<const wxGridExtOperations&>(wxGridExtRowOperations())
						: static_cast<const wxGridExtOperations&>(wxGridExtColumnOperations())
						).SetLineSize(this, m_dragRowOrCol, m_dragRowOrColOldSize);

					m_dragRowOrCol = -1;
					break;

				case WXGRID_CURSOR_SELECT_CELL:
				case WXGRID_CURSOR_SELECT_ROW:
				case WXGRID_CURSOR_SELECT_COL:
					if (m_selection)
						m_selection->CancelSelecting();
					break;
				}
				EndDraggingIfNecessary();

				// ensure that a new drag operation is only started after a LeftUp
				m_cancelledDragging = true;
			}
			else
			{
				ClearSelection();
			}
			break;

		case WXK_TAB:
		{
			// send an event to the grid's parents for custom handling
			wxGridExtEvent gridEvt(GetId(), wxEVT_GRID_TABBING, this,
				GetGridCursorRow(), GetGridCursorCol(),
				-1, -1, false, event);
			if (ProcessWindowEvent(gridEvt))
			{
				// the event has been handled so no need for more processing
				break;
			}
		}
		DoGridProcessTab(event);
		break;

		case WXK_HOME:
		case WXK_END:
			if (m_currentCellCoords != wxGridExtNoCellCoords)
			{
				const bool goToBeginning = event.GetKeyCode() == WXK_HOME;

				// Find the first or last visible row if we need to go to it
				// (without Control, we keep the current row).
				int row;
				if (event.ControlDown())
				{
					if (goToBeginning)
					{
						for (row = 0; row < m_numRows; ++row)
						{
							if (IsRowShown(row))
								break;
						}
					}
					else
					{
						for (row = m_numRows - 1; row >= 0; --row)
						{
							if (IsRowShown(row))
								break;
						}
					}
				}
				else
				{
					// If we're extending the selection, try to continue in
					// the same row, which may well be different from the
					// one in which we started selecting.
					if (m_selection && event.ShiftDown())
					{
						row = m_selection->GetExtensionAnchor().GetRow();
					}
					else // Just use the current row.
					{
						row = m_currentCellCoords.GetRow();
					}
				}

				// Also find the last or first visible column in any case.
				int col;
				if (goToBeginning)
				{
					for (col = 0; col < m_numCols; ++col)
					{
						if (IsColShown(GetColAt(col)))
							break;
					}
				}
				else
				{
					for (col = m_numCols - 1; col >= 0; --col)
					{
						if (IsColShown(GetColAt(col)))
							break;
					}
				}

				if (event.ShiftDown())
				{
					if (m_selection)
						m_selection->ExtendCurrentBlock(
							m_currentCellCoords,
							wxGridExtCellCoords(row, col),
							event);
					MakeCellVisible(row, col);
				}
				else
				{
					ClearSelection();
					GoToCell(row, GetColAt(col));
				}
			}
			break;

		case WXK_PAGEUP:
			DoMoveCursorByPage
			(
				event,
				wxGridExtBackwardOperations(this, wxGridExtRowOperations())
			);
			break;

		case WXK_PAGEDOWN:
			DoMoveCursorByPage
			(
				event,
				wxGridExtForwardOperations(this, wxGridExtRowOperations())
			);
			break;

		case WXK_SPACE:
			// Ctrl-Space selects the current column or, if there is a
			// selection, all columns containing the selected cells,
			// Shift-Space -- the current row (or all selection rows) and
			// Ctrl-Shift-Space -- everything.
		{
			wxGridExtCellCoords selStart, selEnd;
			switch (m_selection ? event.GetModifiers() : wxMOD_NONE)
			{
			case wxMOD_CONTROL:
				selStart.Set(0, m_currentCellCoords.GetCol());
				selEnd.Set(m_numRows - 1,
					m_selection->GetExtensionAnchor().GetCol());
				break;

			case wxMOD_SHIFT:
				selStart.Set(m_currentCellCoords.GetRow(), 0);
				selEnd.Set(m_selection->GetExtensionAnchor().GetRow(),
					m_numCols - 1);
				break;

			case wxMOD_CONTROL | wxMOD_SHIFT:
				selStart.Set(0, 0);
				selEnd.Set(m_numRows - 1, m_numCols - 1);
				break;

			case wxMOD_NONE:
				if (!IsEditable())
				{
					MoveCursorRight(false);
					break;
				}
				wxFALLTHROUGH;

			default:
				event.Skip();
			}

			if (selStart != wxGridExtNoCellCoords)
				m_selection->ExtendCurrentBlock(selStart, selEnd, event);
		}
		break;

#if wxUSE_CLIPBOARD
		case WXK_INSERT:
		case 'C':
			if (event.GetModifiers() == wxMOD_CONTROL)
			{
				// Coordinates of the selected block to copy to clipboard.
				wxGridExtBlockCoords sel;

				// Check if we have any selected blocks and if we don't
				// have too many of them.
				const wxGridExtBlocks blocks = GetSelectedBlocks();
				wxGridExtBlocks::iterator iter = blocks.begin();
				if (iter == blocks.end())
				{
					// No selection, copy just the current cell.
					if (m_currentCellCoords == wxGridExtNoCellCoords)
					{
						// But we don't even have it -- nothing to do then.
						event.Skip();
						break;
					}

					sel = wxGridExtBlockCoords(GetGridCursorRow(),
						GetGridCursorCol(),
						GetGridCursorRow(),
						GetGridCursorCol());
				}
				else // We do have at least one selected block.
				{
					sel = *blocks.begin();

					if (++iter != blocks.end())
					{
						// As we use simple text format, we can't copy more
						// than one block to clipboard.
						wxLogWarning
						(
							_("Copying more than one selected block "
								"to clipboard is not supported.")
						);
						break;
					}
				}

				wxClipboardLocker lockClipboard;
				if (!lockClipboard)
				{
					// Error message should have been already given and we
					// don't have much to add.
					break;
				}

				wxString buf;
				for (int row = sel.GetTopRow(); row <= sel.GetBottomRow(); row++)
				{
					bool first = true;

					for (int col = sel.GetLeftCol(); col <= sel.GetRightCol(); col++)
					{
						if (first)
							first = false;
						else
							buf += '\t';

						buf += GetCellValue(row, col);
					}

					buf += wxTextFile::GetEOL();
				}

				wxTheClipboard->SetData(new wxTextDataObject(buf));
				break;
			}
			wxFALLTHROUGH;
#endif // wxUSE_CLIPBOARD

		default:
			event.Skip();
			break;
		}
	}
}

void wxGridExt::OnKeyUp(wxKeyEvent& WXUNUSED(event))
{
	// This function is unused and not connected to the corresponding event in
	// the event table, it is only kept to prevent changing ABI in this branch
	// and doesn't exist at all in the later wxWidgets versions.
}

void wxGridExt::OnChar(wxKeyEvent& event)
{
	// is it possible to edit the current cell at all?
	if (!IsCellEditControlEnabled() && CanEnableCellControl())
	{
		// yes, now check whether the cells editor accepts the key
		wxGridExtCellEditorPtr editor = GetCurrentCellEditorPtr();

		// <F2> is special and will always start editing, for
		// other keys - ask the editor itself
		const bool specialEditKey = event.GetKeyCode() == WXK_F2 &&
			!event.HasModifiers();
		if (specialEditKey || editor->IsAcceptedKey(event))
		{
			// ensure cell is visble
			MakeCellVisible(m_currentCellCoords);

			if (DoEnableCellEditControl(wxGridExtActivationSource::From(event))
				&& !specialEditKey)
				editor->StartingKey(event);
		}
		else
		{
			event.Skip();
		}
	}
	else
	{
		event.Skip();
	}
}

void wxGridExt::DoGridProcessTab(wxKeyboardState& kbdState)
{
	const bool isForwardTab = !kbdState.ShiftDown();

	// TAB processing only changes when we are at the borders of the grid, so
	// let's first handle the common behaviour when we are not at the border.
	if (isForwardTab)
	{
		if (GetGridCursorCol() < GetNumberCols() - 1)
		{
			MoveCursorRight(false);
			return;
		}
	}
	else // going back
	{
		if (GetGridCursorCol())
		{
			MoveCursorLeft(false);
			return;
		}
	}


	// We only get here if the cursor is at the border of the grid, apply the
	// configured behaviour.
	switch (m_tabBehaviour)
	{
	case Tab_Stop:
		// Nothing special to do, we remain at the current cell.
		break;

	case Tab_Wrap:
		// Go to the beginning of the next or the end of the previous row.
		if (isForwardTab)
		{
			if (GetGridCursorRow() < GetNumberRows() - 1)
			{
				GoToCell(GetGridCursorRow() + 1, 0);
				return;
			}
		}
		else
		{
			if (GetGridCursorRow() > 0)
			{
				GoToCell(GetGridCursorRow() - 1, GetNumberCols() - 1);
				return;
			}
		}
		break;

	case Tab_Leave:
		if (Navigate(isForwardTab ? wxNavigationKeyEvent::IsForward
			: wxNavigationKeyEvent::IsBackward))
			return;
		break;
	}

	// If we remain in this cell, stop editing it if we were doing so.
	DisableCellEditControl();
}

bool wxGridExt::SetCurrentCell(const wxGridExtCellCoords& coords)
{
	switch (SendEvent(wxEVT_GRID_SELECT_CELL, coords))
	{
	case Event_Vetoed:
	case Event_CellDeleted:
		// We shouldn't do anything if the event was vetoed and can't do
		// anything if the cell doesn't exist any longer.
		return false;

	case Event_Unhandled:
	case Event_Handled:
		// But it doesn't matter here if the event was skipped or not.
		break;
	}

	if (m_currentCellCoords != wxGridExtNoCellCoords)
	{
		DisableCellEditControl();

		if (IsVisible(m_currentCellCoords, false))
		{
#if defined(__WXOSX__) || defined(__WXGTK3__)
			RefreshBlock(m_currentCellCoords, m_currentCellCoords);
#else
			wxGridExtWindow* prevGridWindow = CellToGridWindow(m_currentCellCoords);
			wxRect r;
			r = BlockToDeviceRect(m_currentCellCoords, m_currentCellCoords, prevGridWindow);
			if (!m_gridLinesEnabled)
			{
				r.x--;
				r.y--;
				r.width++;
				r.height++;
			}

			wxGridExtCellCoordsArray cells = CalcCellsExposed(r, prevGridWindow);

			// Otherwise refresh redraws the highlight!
			m_currentCellCoords = coords;

			wxClientDC prevDc(prevGridWindow);
			PrepareDCFor(prevDc, prevGridWindow);

			DrawGridCellArea(prevDc, cells);
			DrawAllGridWindowLines(prevDc, r, prevGridWindow);

			if (prevGridWindow->GetType() != wxGridExtWindow::wxGridExtWindowNormal)
				DrawFrozenBorder(prevDc, prevGridWindow);
#endif
		}
	}

	m_currentCellCoords = coords;

#if defined(__WXOSX__) || defined(__WXGTK3__)
	RefreshBlock(coords, coords);
#else
	if (ShouldRefresh())
	{
		wxGridExtCellAttrPtr attr = GetCellAttrPtr(coords);
		wxGridExtWindow* currentGridWindow = CellToGridWindow(coords);
		wxClientDC dc(currentGridWindow);
		PrepareDCFor(dc, currentGridWindow);
		DrawCellHighlight(dc, coords.GetRow(), coords.GetCol(), attr.get());
	}
#endif

	return true;
}

// Note - this function only draws cells that are in the list of
// exposed cells (usually set from the update region by
// CalcExposedCells)
//
void wxGridExt::DrawGridCellArea(wxDC& dc, const wxGridExtCellCoordsArray& cells)
{
	if (!m_numRows || !m_numCols)
		return;

	int i, numCells = cells.GetCount();
	wxGridExtCellCoordsArray redrawCells;

	for (i = numCells - 1; i >= 0; i--)
	{
		int row, col, cell_rows, cell_cols;
		row = cells[i].GetRow();
		col = cells[i].GetCol();

		// If this cell is part of a multicell block, find owner for repaint
		if (GetCellSize(row, col, &cell_rows, &cell_cols) == CellSpan_Inside)
		{
			wxGridExtCellCoords cell(row + cell_rows, col + cell_cols);
			bool marked = false;
			for (int j = 0; j < numCells; j++)
			{
				if (cell == cells[j])
				{
					marked = true;
					break;
				}
			}

			if (!marked)
			{
				int count = redrawCells.GetCount();
				for (int j = 0; j < count; j++)
				{
					if (cell == redrawCells[j])
					{
						marked = true;
						break;
					}
				}

				if (!marked)
					redrawCells.Add(cell);
			}

			// don't bother drawing this cell
			continue;
		}

		// If this cell is empty, find cell to left that might want to overflow
		if (m_table && m_table->IsEmptyCell(row, col))
		{
			for (int l = 0; l < cell_rows; l++)
			{
				// find a cell in this row to leave already marked for repaint
				int left = col;
				for (int k = 0; k < int(redrawCells.GetCount()); k++)
					if ((redrawCells[k].GetCol() < left) &&
						(redrawCells[k].GetRow() == row))
					{
						left = redrawCells[k].GetCol();
					}

				if (left == col)
					left = 0; // oh well

				for (int j = col - 1; j >= left; j--)
				{
					if (!m_table->IsEmptyCell(row + l, j))
					{
						wxGridExtCellAttrPtr attr = GetCellAttrPtr(row + l, j);
						int numRows, numCols;
						attr->GetSize(&numRows, &numCols);
						if (GetCellSpan(numRows, numCols)
							== wxGridExt::CellSpan_Inside)
							// As above: don't bother drawing inside cells.
							continue;

						if (attr->CanOverflow())
						{
							wxGridExtCellCoords cell(row + l, j);
							bool marked = false;

							for (int k = 0; k < numCells; k++)
							{
								if (cell == cells[k])
								{
									marked = true;
									break;
								}
							}

							if (!marked)
							{
								int count = redrawCells.GetCount();
								for (int k = 0; k < count; k++)
								{
									if (cell == redrawCells[k])
									{
										marked = true;
										break;
									}
								}
								if (!marked)
									redrawCells.Add(cell);
							}
						}
						break;
					}
				}
			}
		}

		DrawCell(dc, cells[i]);
	}

	numCells = redrawCells.GetCount();

	for (i = numCells - 1; i >= 0; i--)
	{
		DrawCell(dc, redrawCells[i]);
	}
}

void wxGridExt::DrawGridSpace(wxDC& dc, wxGridExtWindow* gridWindow)
{
	int cw, ch;
	gridWindow->GetClientSize(&cw, &ch);

	int right, bottom;
	wxPoint offset = GetGridWindowOffset(gridWindow);
	CalcGridWindowUnscrolledPosition(cw + offset.x, ch + offset.y, &right, &bottom, gridWindow);

	int rightCol = m_numCols > 0 ? GetColRight(GetColAt(m_numCols - 1)) : 0;
	int bottomRow = m_numRows > 0 ? GetRowBottom(GetRowAt(m_numRows - 1)) : 0;

	if (right > rightCol || bottom > bottomRow)
	{
		int left, top;
		CalcGridWindowUnscrolledPosition(offset.x, offset.y, &left, &top, gridWindow);

		dc.SetBrush(GetDefaultCellBackgroundColour());
		dc.SetPen(*wxTRANSPARENT_PEN);

		if (right > rightCol)
		{
			dc.DrawRectangle(rightCol, top, right - rightCol, ch);
		}

		if (bottom > bottomRow)
		{
			dc.DrawRectangle(left, bottomRow, cw, bottom - bottomRow);
		}
	}
}

void wxGridExt::DrawCell(wxDC& dc, const wxGridExtCellCoords& coords)
{
	int row = coords.GetRow();
	int col = coords.GetCol();

	if (GetColWidth(col) <= 0 || GetRowHeight(row) <= 0)
		return;

	// we draw the cell border ourselves
	wxGridExtCellAttrPtr attr = GetCellAttrPtr(row, col);

	bool isCurrent = coords == m_currentCellCoords;

	wxRect rect = CellToRect(row, col);

	// if the editor is shown, we should use it and not the renderer
	// Note: However, only if it is really _shown_, i.e. not hidden!
	if (isCurrent && IsCellEditControlShown())
	{
		attr->GetEditorPtr(this, row, col)->PaintBackground(dc, rect, *attr);
	}
	else
	{
		// but all the rest is drawn by the cell renderer and hence may be customized
		attr->GetRendererPtr(this, row, col)
			->Draw(*this, *attr, dc, rect, row, col, IsInSelection(coords));
	}
}

void wxGridExt::DrawCellBorder(wxDC& dc, int row, int col, const wxGridExtCellAttr* attr)
{
	if (GetColWidth(col) <= 0 || GetRowHeight(row) <= 0)
		return;

	wxRect rect = CellToRect(row, col);

	//draw border  
	wxGridExtCellBorder borderLeft = attr->GetBorderLeft();
	if (borderLeft.m_style != wxPenStyle::wxPENSTYLE_TRANSPARENT)
	{
		dc.SetPen(wxPen(borderLeft.m_colour, borderLeft.m_width, borderLeft.m_style));
		dc.DrawLine(rect.GetLeft() - borderLeft.m_width, rect.GetTop() - borderLeft.m_width,
			rect.GetLeft() - borderLeft.m_width, rect.GetBottom() + borderLeft.m_width);
	}

	wxGridExtCellBorder borderRight = attr->GetBorderRight();
	if (borderRight.m_style != wxPenStyle::wxPENSTYLE_TRANSPARENT)
	{
		dc.SetPen(wxPen(borderLeft.m_colour, borderRight.m_width, borderRight.m_style));
		dc.DrawLine(rect.GetRight() + borderRight.m_width, rect.GetTop() - borderRight.m_width,
			rect.GetRight() + borderRight.m_width, rect.GetBottom() + borderRight.m_width);
	}

	wxGridExtCellBorder borderTop = attr->GetBorderTop();
	if (borderTop.m_style != wxPenStyle::wxPENSTYLE_TRANSPARENT)
	{
		dc.SetPen(wxPen(borderTop.m_colour, borderTop.m_width, borderTop.m_style));
		dc.DrawLine(rect.GetLeft() - borderTop.m_width, rect.GetTop() - borderTop.m_width,
			rect.GetRight() + borderTop.m_width, rect.GetTop() - borderTop.m_width);
	}

	wxGridExtCellBorder borderBottom = attr->GetBorderBottom();
	if (borderBottom.m_style != wxPenStyle::wxPENSTYLE_TRANSPARENT)
	{
		dc.SetPen(wxPen(borderBottom.m_colour, borderBottom.m_width, borderBottom.m_style));
		dc.DrawLine(rect.GetLeft() - borderBottom.m_width, rect.GetBottom() + borderBottom.m_width,
			rect.GetRight() + borderBottom.m_width, rect.GetBottom() + borderBottom.m_width);
	}
}

void wxGridExt::DrawCellHighlight(wxDC& dc, int row, int col, const wxGridExtCellAttr* attr)
{
	if (GetColWidth(col) <= 0 || GetRowHeight(row) <= 0)
		return;

	// hmmm... what could we do here to show that the cell is disabled?
	// for now, I just draw a thinner border than for the other ones, but
	// it doesn't look really good

	wxRect rect = CellToRect(row, col);

	////////////////////////////////////////////////////////////////////////
	// Draw selected lines 

	if (!IsInSelection(row, col - 1))
	{
		dc.SetPen({ *wxBLACK, 2, wxPENSTYLE_SOLID });
		dc.DrawLine(rect.GetLeft(), rect.GetTop() - 1,
			rect.GetLeft(), rect.GetBottom());
	}

	if (!IsInSelection(row, col + 1)) // !!!
	{
		dc.SetPen({ *wxBLACK, 2, wxPENSTYLE_SOLID });
		dc.DrawLine(rect.GetRight() + 1, rect.GetTop() - 1,
			rect.GetRight() + 1, rect.GetBottom());
	}

	if (!IsInSelection(row - 1, col))
	{
		dc.SetPen({ *wxBLACK, 2, wxPENSTYLE_SOLID });
		dc.DrawLine(rect.GetLeft() - 1, rect.GetTop(),
			rect.GetRight() + 1, rect.GetTop());
	}

	if (!IsInSelection(row + 1, col)) // !!!
	{
		dc.SetPen({ *wxBLACK, 2, wxPENSTYLE_SOLID });
		dc.DrawLine(rect.GetLeft() - 1, rect.GetBottom(),
			rect.GetRight() + 1, rect.GetBottom());
	}

	RefreshBlock(row, col, row, col);
}

wxPen wxGridExt::GetDefaultGridLinePen()
{
	return wxPen(GetGridLineColour());
}

wxPen wxGridExt::GetRowGridLinePen(int WXUNUSED(row))
{
	return GetDefaultGridLinePen();
}

wxPen wxGridExt::GetColGridLinePen(int WXUNUSED(col))
{
	return GetDefaultGridLinePen();
}

void wxGridExt::DrawBorder(wxDC& dc, const wxGridExtCellCoordsArray& cells)
{
	// if the active cell was repainted, repaint its highlight too because it
	// might have been damaged by the grid lines
	size_t count = cells.GetCount();
	for (size_t n = 0; n < count; n++)
	{
		const wxGridExtCellCoords& cell = cells[n];
		wxGridExtCellAttrPtr attr = GetCellAttrPtr(cells[n]);

		if (attr->HasAnyBorder())
		{
			DrawCellBorder(dc, cell.GetRow(), cell.GetCol(), attr.get());
		}
	}
}

void wxGridExt::DrawHighlight(wxDC& dc, const wxGridExtCellCoordsArray& cells)
{
	// This if block was previously in wxGrid::OnPaint but that doesn't
	// seem to get called under wxGTK - MB
	//
	if (m_currentCellCoords == wxGridExtNoCellCoords &&
		m_numRows && m_numCols)
	{
		m_currentCellCoords.Set(0, 0);
	}

	if (IsCellEditControlShown())
	{
		// don't show highlight when the edit control is shown
		return;
	}

	// if the active cell was repainted, repaint its highlight too because it
	// might have been damaged by the grid lines
	size_t count = cells.GetCount();
	for (size_t n = 0; n < count; n++)
	{
		const wxGridExtCellCoords& cell = cells[n];
		if (m_currentCellCoords == cell || (m_selection && m_selection->IsInSelection(cell)))
		{
			wxGridExtCellAttrPtr attr = GetCellAttrPtr(cell);
			DrawCellHighlight(dc, cell.GetRow(), cell.GetCol(), attr.get());
		}
	}
}

void wxGridExt::DrawFrozenBorder(wxDC& dc, wxGridExtWindow* gridWindow)
{
	if (gridWindow && m_numCols && m_numRows)
	{
		int top, bottom, left, right;
		int cw, ch;
		wxPoint gridOffset = GetGridWindowOffset(gridWindow);
		gridWindow->GetClientSize(&cw, &ch);
		CalcGridWindowUnscrolledPosition(gridOffset.x, gridOffset.y, &left, &top, gridWindow);
		CalcGridWindowUnscrolledPosition(cw + gridOffset.x, ch + gridOffset.y, &right, &bottom, gridWindow);

		if (gridWindow->GetType() & wxGridExtWindow::wxGridExtWindowFrozenRow)
		{
			right = wxMin(right, GetColRight(m_numCols - 1));

			dc.SetPen(wxPen(m_gridFrozenBorderColour,
				m_gridFrozenBorderPenWidth));
			dc.DrawLine(left, bottom, right, bottom);
		}

		if (gridWindow->GetType() & wxGridExtWindow::wxGridExtWindowFrozenCol)
		{
			bottom = wxMin(bottom, GetRowBottom(m_numRows - 1));

			dc.SetPen(wxPen(m_gridFrozenBorderColour,
				m_gridFrozenBorderPenWidth));
			dc.DrawLine(right, top, right, bottom);
		}
	}
}

void wxGridExt::DrawLabelFrozenBorder(wxDC& dc, wxWindow* window, bool isRow)
{
	if (window)
	{
		int cw, ch;
		window->GetClientSize(&cw, &ch);

		dc.SetPen(wxPen(m_gridFrozenBorderColour,
			m_gridFrozenBorderPenWidth));

		if (isRow)
			dc.DrawLine(0, ch, cw, ch);
		else
			dc.DrawLine(cw, 0, cw, ch);
	}
}

// Used by wxGridExt::Render() to draw the grid lines only for the cells in the
// specified range.
void
wxGridExt::DrawRangeGridLines(wxDC& dc,
	const wxRegion& reg,
	const wxGridExtCellCoords& topLeft,
	const wxGridExtCellCoords& bottomRight)
{
	if (!m_gridLinesEnabled)
		return;

	int top, left, width, height;
	reg.GetBox(left, top, width, height);

	// create a clipping region
	wxRegion clippedcells(dc.LogicalToDeviceX(left),
		dc.LogicalToDeviceY(top),
		dc.LogicalToDeviceXRel(width),
		dc.LogicalToDeviceYRel(height));

	// subtract multi cell span area from clipping region for lines
	wxRect rect;
	for (int row = topLeft.GetRow(); row <= bottomRight.GetRow(); row++)
	{
		for (int col = topLeft.GetCol(); col <= bottomRight.GetCol(); col++)
		{
			int cell_rows, cell_cols;
			switch (GetCellSize(row, col, &cell_rows, &cell_cols))
			{
			case CellSpan_Main: // multi cell
				rect = CellToRect(row, col);
				// cater for scaling
				// device origin already set in ::Render() for x, y
				rect.x = dc.LogicalToDeviceX(rect.x);
				rect.y = dc.LogicalToDeviceY(rect.y);
				rect.width = dc.LogicalToDeviceXRel(rect.width);
				rect.height = dc.LogicalToDeviceYRel(rect.height) - 1;
				clippedcells.Subtract(rect);
				break;

			case CellSpan_Inside: // part of multicell
				rect = CellToRect(row + cell_rows, col + cell_cols);
				rect.x = dc.LogicalToDeviceX(rect.x);
				rect.y = dc.LogicalToDeviceY(rect.y);
				rect.width = dc.LogicalToDeviceXRel(rect.width);
				rect.height = dc.LogicalToDeviceYRel(rect.height) - 1;
				clippedcells.Subtract(rect);
				break;

			case CellSpan_None:
				// Nothing special to do.
				break;
			}
		}
	}

	dc.SetDeviceClippingRegion(clippedcells);

	DoDrawGridLines(dc,
		top, left, top + height, left + width,
		topLeft.GetRow(), topLeft.GetCol(),
		bottomRight.GetRow(), bottomRight.GetCol());

	dc.DestroyClippingRegion();

	DoDrawGridNonClippingRegionLines(dc,
		top, left, top + height, left + width,
		topLeft.GetRow(), topLeft.GetCol(),
		bottomRight.GetRow(), bottomRight.GetCol());
}

// This is used to redraw all grid lines e.g. when the grid line colour
// has been changed
//
void wxGridExt::DrawAllGridWindowLines(wxDC& dc, const wxRegion& WXUNUSED(reg), wxGridExtWindow* gridWindow)
{
	if (!m_gridLinesEnabled || !gridWindow)
		return;

	int top, bottom, left, right;

	wxPoint gridOffset = GetGridWindowOffset(gridWindow);

	int cw, ch;
	gridWindow->GetClientSize(&cw, &ch);
	CalcGridWindowUnscrolledPosition(gridOffset.x, gridOffset.y, &left, &top, gridWindow);
	CalcGridWindowUnscrolledPosition(cw + gridOffset.x, ch + gridOffset.y, &right, &bottom, gridWindow);

	// avoid drawing grid lines past the last row and col
	if (m_gridLinesClipHorz)
	{
		if (!m_numCols)
			return;

		const int lastColRight = GetColRight(GetColAt(m_numCols - 1));
		if (right > lastColRight)
			right = lastColRight;
	}

	if (m_gridLinesClipVert)
	{
		if (!m_numRows)
			return;

		const int lastRowBottom = GetRowBottom(GetRowAt(m_numRows - 1));
		if (bottom > lastRowBottom)
			bottom = lastRowBottom;
	}

	// no gridlines inside multicells, clip them out
	int leftCol = GetColPos(internalXToCol(left, gridWindow));
	int topRow = GetRowPos(internalYToRow(top, gridWindow));
	int rightCol = GetColPos(internalXToCol(right, gridWindow));
	int bottomRow = GetRowPos(internalYToRow(bottom, gridWindow));

	if (gridWindow == m_gridWin)
	{
		wxRegion clippedcells(0, 0, cw, ch);

		int cell_rows, cell_cols;
		wxRect rect;

		for (int j = topRow; j <= bottomRow; j++)
		{
			for (int colPos = leftCol; colPos <= rightCol; colPos++)
			{
				int i = GetColAt(colPos);

				switch (GetCellSize(j, i, &cell_rows, &cell_cols))
				{
				case CellSpan_Main:
					rect = CellToRect(j, i);
					rect.Offset(-gridOffset);
					CalcScrolledPosition(rect.x, rect.y, &rect.x, &rect.y);
					clippedcells.Subtract(rect);
					break;

				case CellSpan_Inside:
					rect = CellToRect(j + cell_rows, i + cell_cols);
					rect.Offset(-gridOffset);
					CalcScrolledPosition(rect.x, rect.y, &rect.x, &rect.y);
					clippedcells.Subtract(rect);
					break;

				case CellSpan_None:
					// Nothing special to do.
					break;
				}
			}
		}

		dc.SetDeviceClippingRegion(clippedcells);
	}

	DoDrawGridLines(dc,
		top, left, bottom, right,
		topRow, leftCol, m_numRows, m_numCols);

	dc.DestroyClippingRegion();

	DoDrawGridNonClippingRegionLines(dc,
		top, left, bottom, right,
		topRow, leftCol, m_numRows, m_numCols);
}

void wxGridExt::DrawAllGridLines()
{
	if (m_gridWin)
	{
		wxClientDC dc(m_gridWin);
		PrepareDCFor(dc, m_gridWin);

		DrawAllGridWindowLines(dc, wxRegion(), m_gridWin);
	}

	if (m_frozenRowGridWin)
	{
		wxClientDC dc(m_frozenRowGridWin);
		PrepareDCFor(dc, m_frozenRowGridWin);

		DrawAllGridWindowLines(dc, wxRegion(), m_frozenRowGridWin);
	}

	if (m_frozenColGridWin)
	{
		wxClientDC dc(m_frozenColGridWin);
		PrepareDCFor(dc, m_frozenColGridWin);

		DrawAllGridWindowLines(dc, wxRegion(), m_frozenColGridWin);
	}

	if (m_frozenCornerGridWin)
	{
		wxClientDC dc(m_frozenCornerGridWin);
		PrepareDCFor(dc, m_frozenCornerGridWin);

		DrawAllGridWindowLines(dc, wxRegion(), m_frozenCornerGridWin);
	}
}

void
wxGridExt::DoDrawGridLines(wxDC& dc,
	int top, int left,
	int bottom, int right,
	int topRow, int leftCol,
	int bottomRow, int rightCol)
{
	// horizontal grid lines
	for (int rowPos = topRow; rowPos < bottomRow; rowPos++)
	{
		int i = GetRowAt(rowPos);
		int bot = GetRowBottom(i) - 1;

		if (bot > bottom)
			break;

		if (bot >= top)
		{
			dc.SetPen(GetRowGridLinePen(i));
			dc.DrawLine(left, bot, right, bot);
		}
	}

	// vertical grid lines
	for (int colPos = leftCol; colPos < rightCol; colPos++)
	{
		int i = GetColAt(colPos);

		int colRight = GetColRight(i);

#if defined(__WXGTK__) || defined(__WXQT__)
		if (GetLayoutDirection() != wxLayout_RightToLeft)
#endif
			colRight--;

		if (colRight > right)
			break;

		if (colRight >= left)
		{
			dc.SetPen(GetColGridLinePen(i));
			dc.DrawLine(colRight, top, colRight, bottom);
		}
	}
}

void
wxGridExt::DoDrawGridNonClippingRegionLines(wxDC& dc,
	int top, int left,
	int bottom, int right,
	int topRow, int leftCol,
	int bottomRow, int rightCol)
{
	// horizontal grid lines
	for (int rowPos = topRow; rowPos < bottomRow; rowPos++)
	{
		int i = GetRowAt(rowPos);
		int bot = GetRowBottom(i) - 1;

		//draw col print brake
		if (IsRowBrake(i))
		{
			dc.SetPen(wxPen(*wxBLACK, 1, wxPENSTYLE_SHORT_DASH));
			dc.DrawLine(left, bot, right, bot);
		}

		if (GridRowAreaEnabled())
		{
			//draw area 
			const int flagRowSection = GetRowAreaValue(i);

			// top line
			if (flagRowSection & 2) {
				int rowTop = GetRowTop(i) - 1;
				dc.SetPen(wxPen(*wxRED, 1, wxPENSTYLE_SOLID));
				dc.DrawLine(left, rowTop, right, rowTop);
			}

			// bottom line
			if (flagRowSection & 4) {
				dc.SetPen(wxPen(*wxRED, 1, wxPENSTYLE_SOLID));
				dc.DrawLine(left, bot, right, bot);
			}
		}
	}

	// vertical grid lines
	for (int colPos = leftCol; colPos < rightCol; colPos++)
	{
		int i = GetColAt(colPos);

		int colRight = GetColRight(i);

#if defined(__WXGTK__) || defined(__WXQT__)
		if (GetLayoutDirection() != wxLayout_RightToLeft)
#endif
			colRight--;

		//draw col print brake
		if (IsColBrake(i))
		{
			dc.SetPen(wxPen(*wxBLACK, 1, wxPENSTYLE_SHORT_DASH));
			dc.DrawLine(colRight, top, colRight, bottom);
		}

		if (GridColAreaEnabled())
		{
			const int flagColSection = GetColAreaValue(i);

			// left hand line
			if (flagColSection & 2) {
				int colLeft = GetColLeft(i) - 1;
				dc.SetPen(wxPen(*wxRED, 1, wxPENSTYLE_SOLID));
				dc.DrawLine(colLeft, top, colLeft, bottom);
			}

			// right hand line
			if (flagColSection & 4) {
				dc.SetPen(wxPen(*wxRED, 1, wxPENSTYLE_SOLID));
				dc.DrawLine(colRight, top, colRight, bottom);
			}
		}
	}
}

void wxGridExt::DrawRowAreas(wxDC& dc, const wxArrayInt& rows)
{
	if (!m_numRows)
		return;

	const size_t numLabels = rows.GetCount();
	for (size_t i = 0; i < numLabels; i++)
	{
		DrawRowArea(dc, rows[i]);
	}

	wxGridExtCellAttrProvider* const
		attrProvider = m_table ? m_table->GetAttrProvider() : NULL;

	int hAlign, vAlign;
	wxGridExt::GetRowLabelAlignment(&hAlign, &vAlign);

	for (size_t pos = 0; pos < m_rowAreaAt.size(); pos++) {

		size_t rowHeight = 0;

		const wxGridExtCellArea& entry = m_rowAreaAt[pos];
		for (int row = entry.m_start; row <= entry.m_end; row++) {
			rowHeight += wxGridExt::GetRowHeight(row);
		}

		// notice that an explicit static_cast is needed to avoid a compilation
		// error with VC7.1 which, for some reason, tries to instantiate (abstract)
		// wxGridExtRowHeaderRenderer class without it
		const wxGridExtRowHeaderRenderer&
			rend = attrProvider ? attrProvider->GetRowHeaderRenderer(entry.m_start)
			: static_cast<const wxGridExtRowHeaderRenderer&>
			(gs_defaultHeaderRenderers.rowRenderer);

		wxRect rect(0, GetRowTop(entry.m_start), (GridRowAreaEnabled() ? m_rowAreaWidth : 0), rowHeight);
		rend.DrawLabel(*this, dc, entry.m_areaLabel,
			rect, hAlign, vAlign, wxHORIZONTAL);
	}
}

void wxGridExt::DrawRowArea(wxDC& dc, int row)
{
	if (m_rowAreaWin != NULL) {

		static wxPen sectionPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW));

		wxRect rect(0, GetRowTop(row), (GridRowAreaEnabled() ? m_rowAreaWidth : 0), GetRowHeight(row));
		const int flagSection = GetRowAreaValue(row);

		if (flagSection & 2) {
			dc.SetPen(sectionPen);
			dc.DrawLine(rect.GetLeft(), rect.GetTop() == 0 ? 0 : rect.GetTop() - 1, rect.GetRight() + 1, rect.GetTop() == 0 ? 0 : rect.GetTop() - 1);
		}

		if (flagSection & 3) {
			dc.SetPen(sectionPen);
			dc.DrawLine(rect.GetLeft(), rect.GetTop() == 0 ? 0 : rect.GetTop() - 1, rect.GetLeft(), rect.GetBottom() + 1);
		}

		if (flagSection & 4) {
			dc.SetPen(sectionPen);
			dc.DrawLine(rect.GetLeft(), rect.GetBottom(), rect.GetRight() + 1, rect.GetBottom());
		}
	}
}

void wxGridExt::DrawRowLabels(wxDC& dc, const wxArrayInt& rows)
{
	if (!m_numRows)
		return;

	const size_t numLabels = rows.GetCount();
	for (size_t i = 0; i < numLabels; i++)
	{
		DrawRowLabel(dc, rows[i]);
	}
}

void wxGridExt::DrawRowLabel(wxDC& dc, int row)
{
	if (GetRowHeight(row) <= 0 || m_rowLabelWidth <= 0)
		return;

	wxGridExtCellAttrProvider* const
		attrProvider = m_table ? m_table->GetAttrProvider() : NULL;

	// notice that an explicit static_cast is needed to avoid a compilation
	// error with VC7.1 which, for some reason, tries to instantiate (abstract)
	// wxGridExtRowHeaderRenderer class without it
	const wxGridExtRowHeaderRenderer&
		rend = attrProvider ? attrProvider->GetRowHeaderRenderer(row)
		: static_cast<const wxGridExtRowHeaderRenderer&>
		(gs_defaultHeaderRenderers.rowRenderer);

	wxRect rect(0, GetRowTop(row), m_rowLabelWidth, GetRowHeight(row));

	if (m_cursorMode == WXGRID_CURSOR_MOVE_ROW)
	{
		// clear the background:
		// when called from ProcessRowColLabelMouseEvent the background is not
		// cleared at this point
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.SetBrush(m_colLabelWin->GetBackgroundColour());
		dc.DrawRectangle(rect);
	}

	// draw a border if the row is not being drag-moved
	// (in that case it's omitted to have a 'pressed' appearance)
	if (m_cursorMode != WXGRID_CURSOR_MOVE_ROW || row != m_dragMoveRowOrCol)
		rend.DrawBorder(*this, dc, rect);
	else
	{
		// just highlight the current row
		dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT)));
		dc.DrawRectangle(rect);
		rect.Deflate(GetBorder() == wxBORDER_NONE ? 2 : 1);
	}

	int hAlign, vAlign;
	GetRowLabelAlignment(&hAlign, &vAlign);

	rend.DrawLabel(*this, dc, GetRowLabelValue(row),
		rect, hAlign, vAlign, wxHORIZONTAL);
}

bool wxGridExt::UseNativeColHeader(bool native)
{
	if (native == m_useNativeHeader)
		return true;

	// Using native control doesn't work if any columns are frozen currently.
	if (native && m_numFrozenCols)
		return false;

	delete m_colLabelWin;
	m_useNativeHeader = native;

	CreateColumnWindow();

	if (m_useNativeHeader)
	{
		SetNativeHeaderColCount();

		wxHeaderCtrl* const colHeader = GetGridColHeader();
		colHeader->SetBackgroundColour(GetLabelBackgroundColour());
		colHeader->SetForegroundColour(GetLabelTextColour());
		colHeader->SetFont(GetLabelFont());
	}

	CalcWindowSizes();

	return true;
}

void wxGridExt::SetUseNativeColLabels(bool native)
{
	wxASSERT_MSG(!m_useNativeHeader,
		"doesn't make sense when using native header");

	m_nativeColumnLabels = native;
	if (native)
	{
		int height = wxRendererNative::Get().GetHeaderButtonHeight(this);
		SetColLabelSize(height);
	}

	GetColLabelWindow()->Refresh();
	m_cornerLabelWin->Refresh();
}

void wxGridExt::DrawColAreas(wxDC& dc, const wxArrayInt& cols)
{
	if (!m_numCols)
		return;

	const size_t numLabels = cols.GetCount();
	for (size_t i = 0; i < numLabels; i++)
	{
		DrawColArea(dc, cols[i]);
	}

	if (m_colAreaWin != NULL) {

		wxGridExtCellAttrProvider* const
			attrProvider = m_table ? m_table->GetAttrProvider() : NULL;

		int hAlign, vAlign;
		wxGridExt::GetColLabelAlignment(&hAlign, &vAlign);

		for (size_t pos = 0; pos < m_colAreaAt.size(); pos++) {

			size_t colWidth = 0;

			const wxGridExtCellArea& entry = m_colAreaAt[pos];
			for (int col = entry.m_start; col <= entry.m_end; col++) {
				colWidth += wxGridExt::GetColWidth(col);
			}

			const wxGridExtColumnHeaderRenderer&
				rend = attrProvider ? attrProvider->GetColumnHeaderRenderer(entry.m_start)
				: static_cast<wxGridExtColumnHeaderRenderer&>
				(gs_defaultHeaderRenderers.colRenderer);

			wxRect rect(GetColLeft(entry.m_start), 0, colWidth, (GridColAreaEnabled() ? m_colAreaHeight : 0));
			rend.DrawLabel(*this, dc, entry.m_areaLabel,
				rect, hAlign, vAlign, wxHORIZONTAL);
		}
	}
}

void wxGridExt::DrawColArea(wxDC& dc, int col)
{
	if (m_colAreaWin != NULL) {

		static wxPen sectionPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW));

		wxRect rect(GetColLeft(col), 0, GetColWidth(col), (GridColAreaEnabled() ? m_colAreaHeight : 0));
		const int flagSection = GetColAreaValue(col);

		if (flagSection & 2) {
			dc.SetPen(sectionPen);
			dc.DrawLine(rect.GetLeft() == 0 ? 0 : rect.GetLeft() - 1, rect.GetBottom() + 1, rect.GetLeft() == 0 ? 0 : rect.GetLeft() - 1, rect.GetTop());
		}

		if (flagSection & 3) {
			dc.SetPen(sectionPen);
			dc.DrawLine(rect.GetLeft() == 0 ? 0 : rect.GetLeft() - 1, rect.GetTop(), rect.GetRight(), rect.GetTop());
		}

		if (flagSection & 4) {
			dc.SetPen(sectionPen);
			dc.DrawLine(rect.GetRight(), rect.GetTop(), rect.GetRight(), rect.GetBottom() + 1);
		}
	}
}

void wxGridExt::DrawColLabels(wxDC& dc, const wxArrayInt& cols)
{
	if (!m_numCols)
		return;

	const size_t numLabels = cols.GetCount();
	for (size_t i = 0; i < numLabels; i++)
	{
		DrawColLabel(dc, cols[i]);
	}
}

void wxGridExt::DrawCornerLabel(wxDC& dc)
{
	wxRect rect(wxSize(m_rowLabelWidth, m_colLabelHeight));

	wxGridExtCellAttrProvider* const
		attrProvider = m_table ? m_table->GetAttrProvider() : NULL;
	const wxGridExtCornerHeaderRenderer&
		rend = attrProvider ? attrProvider->GetCornerRenderer()
		: static_cast<wxGridExtCornerHeaderRenderer&>
		(gs_defaultHeaderRenderers.cornerRenderer);

	if (m_nativeColumnLabels)
	{
		rect.Deflate(1);

		wxRendererNative::Get().DrawHeaderButton(m_cornerLabelWin, dc, rect, 0);
	}
	else
	{
		rect.width++;
		rect.height++;

		rend.DrawBorder(*this, dc, rect);
	}

	wxString label = GetCornerLabelValue();
	if (!label.IsEmpty())
	{
		int hAlign, vAlign;
		GetCornerLabelAlignment(&hAlign, &vAlign);
		const int orient = GetCornerLabelTextOrientation();

		rend.DrawLabel(*this, dc, label, rect, hAlign, vAlign, orient);
	}
}

void wxGridExt::DrawColLabel(wxDC& dc, int col)
{
	if (GetColWidth(col) <= 0 || m_colLabelHeight <= 0)
		return;

	int colLeft = GetColLeft(col);

	wxRect rect(colLeft, 0, GetColWidth(col), m_colLabelHeight);
	wxGridExtCellAttrProvider* const
		attrProvider = m_table ? m_table->GetAttrProvider() : NULL;
	const wxGridExtColumnHeaderRenderer&
		rend = attrProvider ? attrProvider->GetColumnHeaderRenderer(col)
		: static_cast<wxGridExtColumnHeaderRenderer&>
		(gs_defaultHeaderRenderers.colRenderer);

	if (m_nativeColumnLabels)
	{
		wxRendererNative::Get().DrawHeaderButton
		(
			GetColLabelWindow(),
			dc,
			rect,
			0,
			IsSortingBy(col)
			? IsSortOrderAscending()
			? wxHDR_SORT_ICON_UP
			: wxHDR_SORT_ICON_DOWN
			: wxHDR_SORT_ICON_NONE
		);
		rect.Deflate(2);
	}
	else
	{

		if (m_cursorMode == WXGRID_CURSOR_MOVE_COL)
		{
			// clear the background:
			// when called from ProcessRowColLabelMouseEvent the background
			// is not cleared at this point
			wxDCBrushChanger setBrush(dc, m_colLabelWin->GetBackgroundColour());
			wxDCPenChanger setPen(dc, *wxTRANSPARENT_PEN);
			dc.DrawRectangle(rect);
		}

		// draw a border if the column is not being drag-moved
		// (in that case it's omitted to have a 'pressed' appearance)
		if (m_cursorMode != WXGRID_CURSOR_MOVE_COL || col != m_dragMoveRowOrCol)
			rend.DrawBorder(*this, dc, rect);
		else
		{
			// just highlight the current column
			dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT)));
			dc.DrawRectangle(rect);
			rect.Deflate(GetBorder() == wxBORDER_NONE ? 2 : 1);
		}
	}

	int hAlign, vAlign;
	GetColLabelAlignment(&hAlign, &vAlign);
	const int orient = GetColLabelTextOrientation();

	rend.DrawLabel(*this, dc, GetColLabelValue(col), rect, hAlign, vAlign, orient);
}

// TODO: these 2 functions should be replaced with wxDC::DrawLabel() to which
//       we just have to add textOrientation support
void wxGridExt::DrawTextRectangle(wxDC& dc,
	const wxString& value,
	const wxRect& rect,
	int horizAlign,
	int vertAlign,
	int textOrientation)
{
	static wxArrayString lines;
	ParseLines(value, lines);
	DrawTextRectangle(dc, lines, rect, horizAlign, vertAlign, textOrientation);
}

void wxGridExt::DrawTextRectangle(wxDC& dc,
	const wxArrayString& lines,
	const wxRect& rect,
	int horizAlign,
	int vertAlign,
	int textOrientation)
{
	if (lines.empty())
		return;

	wxDCClipper clip(dc, rect);

	long textWidth,
		textHeight;

	if (textOrientation == wxHORIZONTAL)
		GetTextBoxSize(dc, lines, &textWidth, &textHeight);
	else
		GetTextBoxSize(dc, lines, &textHeight, &textWidth);

	int x = 0,
		y = 0;
	switch (vertAlign)
	{
	case wxALIGN_BOTTOM:
		if (textOrientation == wxHORIZONTAL)
			y = rect.y + (rect.height - textHeight - GRID_TEXT_MARGIN);
		else
			x = rect.x + (rect.width - textWidth - GRID_TEXT_MARGIN);
		break;

	case wxALIGN_CENTRE:
		if (textOrientation == wxHORIZONTAL)
			y = rect.y + ((rect.height - textHeight) / 2);
		else
			x = rect.x + ((rect.width - textWidth) / 2);
		break;

	case wxALIGN_TOP:
	default:
		if (textOrientation == wxHORIZONTAL)
			y = rect.y + GRID_TEXT_MARGIN;
		else
			x = rect.x + GRID_TEXT_MARGIN;
		break;
	}

	// Align each line of a multi-line label
	size_t nLines = lines.GetCount();
	for (size_t l = 0; l < nLines; l++)
	{
		const wxString& line = lines[l];

		if (line.empty())
		{
			*(textOrientation == wxHORIZONTAL ? &y : &x) += dc.GetCharHeight();
			continue;
		}

		wxCoord lineWidth = 0,
			lineHeight = 0;
		dc.GetTextExtent(line, &lineWidth, &lineHeight);

		switch (horizAlign)
		{
		case wxALIGN_RIGHT:
			if (textOrientation == wxHORIZONTAL)
				x = rect.x + (rect.width - lineWidth - GRID_TEXT_MARGIN);
			else
				y = rect.y + lineWidth + GRID_TEXT_MARGIN;
			break;

		case wxALIGN_CENTRE:
			if (textOrientation == wxHORIZONTAL)
				x = rect.x + ((rect.width - lineWidth) / 2);
			else
				y = rect.y + rect.height - ((rect.height - lineWidth) / 2);
			break;

		case wxALIGN_LEFT:
		default:
			if (textOrientation == wxHORIZONTAL)
				x = rect.x + GRID_TEXT_MARGIN;
			else
				y = rect.y + rect.height - GRID_TEXT_MARGIN;
			break;
		}

		if (textOrientation == wxHORIZONTAL)
		{
			dc.DrawText(line, x, y);
			y += lineHeight;
		}
		else
		{
			dc.DrawRotatedText(line, x, y, 90.0);
			x += lineHeight;
		}
	}
}

void wxGridExt::DrawTextRectangle(wxDC& dc,
	const wxString& text,
	const wxRect& rect,
	const wxGridExtCellAttr& attr,
	int hAlign,
	int vAlign)
{
	attr.GetNonDefaultAlignment(&hAlign, &vAlign);

	const wxEllipsizeMode mode = attr.GetFitMode().GetEllipsizeMode();
	if (mode != wxEllipsizeMode::wxELLIPSIZE_NONE) {

		// This does nothing if there is no need to ellipsize.
		const wxString& ellipsizedText = wxControl::Ellipsize
		(
			text,
			dc,
			attr.GetFitMode().GetEllipsizeMode(),
			rect.GetWidth() - 2 * GRID_TEXT_MARGIN,
			wxELLIPSIZE_FLAGS_NONE
		);

		DrawTextRectangle(dc, ellipsizedText, rect, hAlign, vAlign, attr.GetTextOrient());
	}
	else {

		DrawTextRectangle(dc, text, rect, hAlign, vAlign, attr.GetTextOrient());
	}
}

// Split multi-line text up into an array of strings.
// Any existing contents of the string array are preserved.
//
// TODO: refactor wxTextFile::Read() and reuse the same code from here
void wxGridExt::ParseLines(const wxString& value, wxArrayString& lines)
{
	lines.Empty();

	if (!value.IsEmpty()) {

		static wxString strRaw; 
		static bool bInQuote = false;

		for (const auto& c : value.ToStdWstring()) {
			strRaw += c;
			if (c == wxT('\'')) {
				bInQuote = !bInQuote;
			}
			else if (c == wxT('\n') && !bInQuote) {
				
				if (!strRaw.IsEmpty())
					lines.Add(strRaw);
				
				strRaw.Clear();
			}
		}

		if (!strRaw.IsEmpty())
			lines.Add(strRaw);
		
		strRaw.Clear();
	}
}

void wxGridExt::GetTextBoxSize(const wxDC& dc,
	const wxArrayString& lines,
	long* width, long* height)
{
	wxCoord w = 0;
	wxCoord h = 0;
	wxCoord lineW = 0, lineH = 0;

	size_t i;
	for (i = 0; i < lines.GetCount(); i++)
	{
		if (lines[i].empty())
		{
			// GetTextExtent() would return 0 for empty lines, but we still
			// need to account for their height.
			h += dc.GetCharHeight();
		}
		else
		{
			dc.GetTextExtent(lines[i], &lineW, &lineH);
			w = wxMax(w, lineW);
			h += lineH;
		}
	}

	*width = w;
	*height = h;
}

//
// ------ Batch processing.
//
void wxGridExt::EndBatch()
{
	if (m_batchCount > 0)
	{
		m_batchCount--;
		if (!m_batchCount)
		{
			CalcDimensions();
			Refresh();
		}
	}
}

// Use this, rather than wxWindow::Refresh(), to force an immediate
// repainting of the grid. Has no effect if you are already inside a
// BeginBatch / EndBatch block.
//
void wxGridExt::ForceRefresh()
{
	BeginBatch();
	EndBatch();
}

void wxGridExt::DoEnable(bool enable)
{
	wxScrolledCanvas::DoEnable(enable);

	Refresh(false /* don't erase background */);
}

//
// ------ Edit control functions
//

void wxGridExt::EnableEditing(bool edit)
{
	if (edit != m_editable)
	{
		if (!edit)
			EnableCellEditControl(edit);
		m_editable = edit;
	}
}

void wxGridExt::EnableCellEditControl(bool enable)
{
	if (!m_editable)
		return;

	if (enable != m_cellEditCtrlEnabled)
	{
		if (enable)
		{
			// this should be checked by the caller!
			wxCHECK_RET(CanEnableCellControl(), wxT("can't enable editing for this cell!"));

			DoEnableCellEditControl(wxGridExtActivationSource::FromProgram());
		}
		else
		{
			DoDisableCellEditControl();
		}
	}
}

bool wxGridExt::DoEnableCellEditControl(const wxGridExtActivationSource& actSource)
{
	switch (SendEvent(wxEVT_GRID_EDITOR_SHOWN))
	{
	case Event_Vetoed:
	case Event_CellDeleted:
		// We shouldn't do anything if the event was vetoed and can't do
		// anything if the cell doesn't exist any longer.
		return false;

	case Event_Unhandled:
	case Event_Handled:
		// But it doesn't matter here if the event was skipped or not.
		break;
	}

	if (!DoShowCellEditControl(actSource))
	{
		// We have to send the HIDDEN event matching the SHOWN one above as the
		// user code may reasonably expect always getting them in pairs, so do
		// it even if the editor hadn't really been shown at all.
		SendEvent(wxEVT_GRID_EDITOR_HIDDEN);

		return false;
	}

	return true;
}

void wxGridExt::DoDisableCellEditControl()
{
	SendEvent(wxEVT_GRID_EDITOR_HIDDEN);

	DoAcceptCellEditControl();
}

bool wxGridExt::IsCurrentCellReadOnly() const
{
	return const_cast<wxGridExt*>(this)->
		GetCellAttrPtr(m_currentCellCoords)->IsReadOnly();
}

bool wxGridExt::CanEnableCellControl() const
{
	return m_editable && (m_currentCellCoords != wxGridExtNoCellCoords) &&
		!IsCurrentCellReadOnly();
}

bool wxGridExt::IsCellEditControlShown() const
{
	bool isShown = false;

	if (m_cellEditCtrlEnabled)
	{
		if (wxGridExtCellEditorPtr editor = GetCurrentCellEditorPtr())
		{
			if (editor->IsCreated())
			{
				isShown = editor->GetWindow()->IsShown();
			}
		}
	}

	return isShown;
}

void wxGridExt::ShowCellEditControl()
{
	if (IsCellEditControlEnabled())
	{
		if (!IsVisible(m_currentCellCoords, false))
		{
			m_cellEditCtrlEnabled = false;
			return;
		}

		DoShowCellEditControl(wxGridExtActivationSource::FromProgram());
	}
}

bool wxGridExt::DoShowCellEditControl(const wxGridExtActivationSource& actSource)
{
	wxRect rect = CellToRect(m_currentCellCoords);
	int row = m_currentCellCoords.GetRow();
	int col = m_currentCellCoords.GetCol();

	wxGridExtCellAttrPtr attr = GetCellAttrPtr(row, col);
	wxGridExtCellEditorPtr editor = attr->GetEditorPtr(this, row, col);

	const wxGridExtActivationResult&
		res = editor->TryActivate(row, col, this, actSource);
	switch (res.GetAction())
	{
	case wxGridExtActivationResult::Change:
		// This is somewhat similar to what DoSaveEditControlValue() does.
		// but we don't allow vetoing CHANGED event here as this code is
		// new and shouldn't have to support this obsolete usage.
		switch (SendEvent(wxEVT_GRID_CELL_CHANGING, res.GetNewValue()))
		{
		case Event_Vetoed:
		case Event_CellDeleted:
			break;

		case Event_Unhandled:
		case Event_Handled:
			const wxString& oldval = GetCellValue(m_currentCellCoords);

			editor->DoActivate(row, col, this);

			// Show the new cell value.
			RefreshBlock(m_currentCellCoords, m_currentCellCoords);

			if (SendEvent(wxEVT_GRID_CELL_CHANGED, oldval) == Event_Vetoed)
			{
				wxFAIL_MSG("Vetoing wxEVT_GRID_CELL_CHANGED is ignored");
			}
			break;
		}
		wxFALLTHROUGH;

	case wxGridExtActivationResult::Ignore:
		// In any case, don't start editing normally.
		return false;

	case wxGridExtActivationResult::ShowEditor:
		// Continue normally.
		break;
	}

	// It's not enabled just yet, but will be soon, and we need to set it
	// before generating any events in case their user-defined handlers decide
	// to call EnableCellEditControl() to avoid reentrancy problems.
	m_cellEditCtrlEnabled = true;

	wxGridExtWindow* gridWindow = CellToGridWindow(row, col);

	// if this is part of a multicell, find owner (topleft)
	int cell_rows, cell_cols;
	if (GetCellSize(row, col, &cell_rows, &cell_cols) == CellSpan_Inside)
	{
		row += cell_rows;
		col += cell_cols;
		m_currentCellCoords.SetRow(row);
		m_currentCellCoords.SetCol(col);
	}

	rect.Offset(-GetGridWindowOffset(gridWindow));

	// convert to scrolled coords
	CalcGridWindowScrolledPosition(rect.x, rect.y, &rect.x, &rect.y, gridWindow);

#ifdef __WXQT__
	// Substract 1 pixel in every dimension to fit in the cell area.
	// If not, Qt will draw the control outside the cell.
	// TODO: Check offsets under Qt.
	rect.Deflate(1, 1);
#endif

	if (!editor->IsCreated())
	{
		editor->Create(gridWindow, wxID_ANY,
			new wxGridExtCellEditorEvtHandler(this, editor.get()));

		// Ensure the editor window has wxWANTS_CHARS flag, so that it
		// gets Tab, Enter and Esc keys, which need to be processed
		// specially by wxGridExtCellEditorEvtHandler.
		wxWindow* const editorWindow = editor->GetWindow();
		if (editorWindow)
		{
			editorWindow->SetWindowStyle(editorWindow->GetWindowStyle()
				| wxWANTS_CHARS);
		}

		wxGridExtEditorCreatedEvent evt(GetId(),
			wxEVT_GRID_EDITOR_CREATED,
			this,
			row,
			col,
			editorWindow);
		ProcessWindowEvent(evt);
	}
	else if (editor->GetWindow() &&
		editor->GetWindow()->GetParent() != gridWindow)
	{
		editor->GetWindow()->Reparent(gridWindow);
	}

	// resize editor to overflow into righthand cells if allowed
	int maxWidth = rect.width;
	wxString value = GetCellValue(row, col);
	if (!value.empty() && attr->GetOverflow())
	{
		int y;
		GetTextExtent(value, &maxWidth, &y, NULL, NULL, &attr->GetFont());
		if (maxWidth < rect.width)
			maxWidth = rect.width;
	}

	if ((maxWidth > rect.width) && (col < m_numCols) && m_table)
	{
		GetCellSize(row, col, &cell_rows, &cell_cols);
		// may have changed earlier
		for (int i = col + cell_cols; i < m_numCols; i++)
		{
			int c_rows, c_cols;
			GetCellSize(row, i, &c_rows, &c_cols);

			// looks weird going over a multicell
			if (m_table->IsEmptyCell(row, i) &&
				(rect.width < maxWidth) && (c_rows == 1))
			{
				rect.width += GetColWidth(i);
			}
			else
				break;
		}
	}

	editor->SetCellAttr(attr.get());
	editor->SetSize(rect);

	// Note that the actual rectangle used by the editor could be
	// different from the one we proposed.
	rect = editor->GetWindow()->GetRect();

	// Ensure that the edit control fits into the visible part of the
	// window by shifting it if necessary: we don't want to truncate
	// any part of it by trying to position it too far to the left or
	// top and we definitely don't want to start showing scrollbars by
	// positioning it too far to the right or bottom.
	const wxSize sizeMax = gridWindow->GetClientSize();
	if (!wxRect(sizeMax).Contains(rect))
	{
		if (rect.x < 0)
			rect.x = 0;
		if (rect.y < 0)
			rect.y = 0;
		if (rect.x > sizeMax.x - rect.width)
			rect.x = sizeMax.x - rect.width;
		if (rect.y > sizeMax.y - rect.height)
			rect.y = sizeMax.y - rect.height;

		editor->GetWindow()->Move(rect.x, rect.y);
	}

	editor->Show(true, attr.get());

	// recalc dimensions in case we need to
	// expand the scrolled window to account for editor
	CalcDimensions();

	editor->BeginEdit(row, col, this);
	editor->SetCellAttr(NULL);

	return true;
}

void wxGridExt::HideCellEditControl()
{
	if (IsCellEditControlEnabled())
	{
		DoHideCellEditControl();
	}
}

void wxGridExt::DoHideCellEditControl()
{
	wxGridExtCellEditorPtr editor = GetCurrentCellEditorPtr();
	const bool editorHadFocus = editor->GetWindow()->IsDescendant(FindFocus());

	if (editor->GetWindow()->GetParent() != m_gridWin)
		editor->GetWindow()->Reparent(m_gridWin);

	editor->Show(false);

	wxGridExtWindow* gridWindow = CellToGridWindow(m_currentCellCoords);
	// return the focus to the grid itself if the editor had it
	//
	// note that we must not do this unconditionally to avoid stealing
	// focus from the window which just received it if we are hiding the
	// editor precisely because we lost focus
	if (editorHadFocus)
		gridWindow->SetFocus();

	// refresh whole row to the right
	wxRect rect(CellToRect(m_currentCellCoords));
	rect.Offset(-GetGridWindowOffset(gridWindow));
	CalcGridWindowScrolledPosition(rect.x, rect.y, &rect.x, &rect.y, gridWindow);
	rect.width = gridWindow->GetClientSize().GetWidth() - rect.x;

#ifdef __WXMAC__
	// ensure that the pixels under the focus ring get refreshed as well
	rect.Inflate(10, 10);
#endif

	gridWindow->Refresh(false, &rect);

	// refresh also the grid to the right
	wxGridExtWindow* rightGridWindow = NULL;
	if (gridWindow->GetType() == wxGridExtWindow::wxGridExtWindowFrozenCorner)
		rightGridWindow = m_frozenRowGridWin;
	else if (gridWindow->GetType() == wxGridExtWindow::wxGridExtWindowFrozenCol)
		rightGridWindow = m_gridWin;

	if (rightGridWindow)
	{
		rect.x = 0;
		rect.width = rightGridWindow->GetClientSize().GetWidth();
		rightGridWindow->Refresh(false, &rect);
	}
}

void wxGridExt::AcceptCellEditControlIfShown()
{
	if (IsCellEditControlShown())
	{
		DoAcceptCellEditControl();
	}
}

void wxGridExt::DoAcceptCellEditControl()
{
	// Reset it first to avoid any problems with recursion via
	// DisableCellEditControl() if it's called from the user-defined event
	// handlers.
	m_cellEditCtrlEnabled = false;

	DoHideCellEditControl();

	DoSaveEditControlValue();
}

void wxGridExt::SaveEditControlValue()
{
	if (IsCellEditControlEnabled())
	{
		DoSaveEditControlValue();
	}
}

void wxGridExt::DoSaveEditControlValue()
{
	int row = m_currentCellCoords.GetRow();
	int col = m_currentCellCoords.GetCol();

	wxString oldval = GetCellValue(m_currentCellCoords);

	wxGridExtCellEditorPtr editor = GetCurrentCellEditorPtr();

	wxString newval;
	if (!editor->EndEdit(row, col, this, oldval, &newval))
		return;

	// set new brake pos
	SetRowBrake(row);
	SetColBrake(col);

	switch (SendEvent(wxEVT_GRID_CELL_CHANGING, newval))
	{
	case Event_Vetoed:
	case Event_CellDeleted:
		break;

	case Event_Unhandled:
	case Event_Handled:
		
		editor->ApplyEdit(row, col, this);

		// for compatibility reasons dating back to wx 2.8 when this event
		// was called wxEVT_GRID_CELL_CHANGE and wxEVT_GRID_CELL_CHANGING
		// didn't exist we allow vetoing this one too
		if (SendEvent(wxEVT_GRID_CELL_CHANGED, oldval) == Event_Vetoed)
		{
			// Event has been vetoed, set the data back.
			//
			// Note that we must use row and col here, which are sure to
			// not have been changed, while m_currentCellCoords could have
			// been changed by the event handler.
			SetCellValue(row, col, oldval);
		}

		SendEvent(wxEVT_GRID_CHANGED);
	}
}

//
// ------ Grid location functions
//  Note that all of these functions work with the logical coordinates of
//  grid cells and labels so you will need to convert from device
//  coordinates for mouse events etc.
//

wxGridExtCellCoords wxGridExt::XYToCell(int x, int y, wxGridExtWindow* gridWindow) const
{
	int row = YToRow(y, false, gridWindow);
	int col = XToCol(x, false, gridWindow);

	return row == -1 || col == -1 ? wxGridExtNoCellCoords
		: wxGridExtCellCoords(row, col);
}

// compute row or column from some (unscrolled) coordinate value, using either
// m_defaultRowHeight/m_defaultColWidth or binary search on array of
// m_rowBottoms/m_colRights to do it quickly in O(log n) time.
// NOTE: This may not work correctly for reordered columns.
int wxGridExt::PosToLinePos(int coord,
	bool clipToMinMax,
	const wxGridExtOperations& oper,
	wxGridExtWindow* gridWindow) const
{
	const int numLines = oper.GetNumberOfLines(this, gridWindow);

	if (coord < 0)
		return clipToMinMax && numLines > 0 ? 0 : wxNOT_FOUND;

	const int defaultLineSize = oper.GetDefaultLineSize(this);
	wxCHECK_MSG(defaultLineSize, -1, "can't have 0 default line size");

	int maxPos = coord / defaultLineSize;
	int minPos = oper.GetFirstLine(this, gridWindow);

	// check for the simplest case: if we have no explicit line sizes
	// configured, then we already know the line this position falls in
	const wxArrayInt& lineEnds = oper.GetLineEnds(this);
	if (lineEnds.empty())
	{
		if (maxPos < (numLines + minPos))
			return maxPos;

		return clipToMinMax ? numLines + minPos - 1 : -1;
	}

	// binary search is quite efficient and we can't really make any assumptions
	// on where to start here since row and columns could be of size 0 if they are
	// hidden. While this could be made more efficient, some profiling will be
	// necessary to determine if it really is a performance bottleneck
	maxPos = numLines + minPos - 1;

	// check if the position is beyond the last column
	const int lineAtMaxPos = oper.GetLineAt(this, maxPos);
	if (coord >= lineEnds[lineAtMaxPos])
		return clipToMinMax ? maxPos : wxNOT_FOUND;

	// or before the first one
	const int lineAt0 = oper.GetLineAt(this, minPos);
	if (coord < oper.GetLineStartPos(this, lineAt0))
		return clipToMinMax ? minPos : wxNOT_FOUND;
	else if (coord < lineEnds[lineAt0])
		return minPos;

	// finally do perform the binary search
	while (minPos < maxPos)
	{
		wxCHECK_MSG(lineEnds[oper.GetLineAt(this, minPos)] <= coord &&
			coord < lineEnds[oper.GetLineAt(this, maxPos)],
			-1,
			"wxGridExt: internal error in PosToLinePos()");

		if (coord >= lineEnds[oper.GetLineAt(this, maxPos - 1)])
			return maxPos;
		else
			maxPos--;

		const int median = minPos + (maxPos - minPos + 1) / 2;
		if (coord < lineEnds[oper.GetLineAt(this, median)])
			maxPos = median;
		else
			minPos = median;
	}

	return maxPos;
}

int
wxGridExt::PosToLine(int coord,
	bool clipToMinMax,
	const wxGridExtOperations& oper,
	wxGridExtWindow* gridWindow) const
{
	int pos = PosToLinePos(coord, clipToMinMax, oper, gridWindow);

	return pos == wxNOT_FOUND ? wxNOT_FOUND : oper.GetLineAt(this, pos);
}

int wxGridExt::YToRow(int y, bool clipToMinMax, wxGridExtWindow* gridWindow) const
{
	return PosToLine(y, clipToMinMax, wxGridExtRowOperations(), gridWindow);
}

int wxGridExt::XToCol(int x, bool clipToMinMax, wxGridExtWindow* gridWindow) const
{
	return PosToLine(x, clipToMinMax, wxGridExtColumnOperations(), gridWindow);
}

int wxGridExt::YToPos(int y, wxGridExtWindow* gridWindow) const
{
	return PosToLinePos(y, true /* clip */, wxGridExtRowOperations(), gridWindow);
}

int wxGridExt::XToPos(int x, wxGridExtWindow* gridWindow) const
{
	return PosToLinePos(x, true /* clip */, wxGridExtColumnOperations(), gridWindow);
}

// return the row/col number such that the pos is near the edge of, or -1 if
// not near an edge.
//
// notice that position can only possibly be near an edge if the row/column is
// large enough to still allow for an "inner" area that is _not_ near the edge
// (i.e., if the height/width is smaller than WXGRID_LABEL_EDGE_ZONE, pos will
// _never_ be considered to be near the edge).
int wxGridExt::PosToEdgeOfLine(int pos, const wxGridExtOperations& oper) const
{
	// Get the bottom or rightmost line that could match.
	int line = oper.PosToLine(this, pos, NULL, true);

	if (line == wxNOT_FOUND)
		return -1;

	const int edge = FromDIP(WXGRID_LABEL_EDGE_ZONE);

	if (oper.GetLineSize(this, line) > edge)
	{
		// We know that we are in this line, test whether we are close enough
		// to start or end border, respectively.
		if (abs(oper.GetLineEndPos(this, line) - pos) < edge)
			return line;
		else if (line > 0 && pos - oper.GetLineStartPos(this, line) < edge)
		{
			// We need to find the previous visible line, so skip all the
			// hidden (of size 0) ones.
			do
			{
				line = oper.GetLineBefore(this, line);
			} while (line >= 0 && oper.GetLineSize(this, line) == 0);

			// It can possibly be -1 here.
			return line;
		}
	}

	return -1;
}

int wxGridExt::YToEdgeOfRow(int y) const
{
	return PosToEdgeOfLine(y, wxGridExtRowOperations());
}

int wxGridExt::XToEdgeOfCol(int x) const
{
	return PosToEdgeOfLine(x, wxGridExtColumnOperations());
}

wxRect wxGridExt::CellToRect(int row, int col) const
{
	wxRect rect(-1, -1, -1, -1);

	if (row >= 0 && row < m_numRows &&
		col >= 0 && col < m_numCols)
	{
		int i, cell_rows, cell_cols;
		rect.width = rect.height = 0;
		if (GetCellSize(row, col, &cell_rows, &cell_cols) == CellSpan_Inside)
		{
			row += cell_rows;
			col += cell_cols;
			GetCellSize(row, col, &cell_rows, &cell_cols);
		}

		rect.x = GetColLeft(col);
		rect.y = GetRowTop(row);
		for (i = col; i < col + cell_cols; i++)
			rect.width += GetColWidth(i);
		for (i = row; i < row + cell_rows; i++)
			rect.height += GetRowHeight(i);

#ifndef __WXQT__
		// if grid lines are enabled, then the area of the cell is a bit smaller
		if (m_gridLinesEnabled)
		{
			rect.width -= 1;
			rect.height -= 1;
		}
#endif
	}

	return rect;
}

wxGridExtWindow* wxGridExt::CellToGridWindow(int row, int col) const
{
	// It may happen that we're called during grid creation, when the current
	// cell still has invalid coordinates -- don't return (possibly null)
	// frozen corner window in this case.
	if (row == -1 && col == -1)
		return m_gridWin;
	else if (GetRowPos(row) < m_numFrozenRows && GetColPos(col) < m_numFrozenCols)
		return m_frozenCornerGridWin;
	else if (GetRowPos(row) < m_numFrozenRows)
		return m_frozenRowGridWin;
	else if (GetColPos(col) < m_numFrozenCols)
		return m_frozenColGridWin;

	return m_gridWin;
}

void wxGridExt::GetGridWindowOffset(const wxGridExtWindow* gridWindow, int& x, int& y) const
{
	wxPoint pt = GetGridWindowOffset(gridWindow);

	x = pt.x;
	y = pt.y;
}

wxPoint wxGridExt::GetGridWindowOffset(const wxGridExtWindow* gridWindow) const
{
	wxPoint pt(0, 0);

	if (gridWindow)
	{
		if (m_frozenRowGridWin &&
			(gridWindow->GetType() & wxGridExtWindow::wxGridExtWindowFrozenRow) == 0)
		{
			pt.y = m_frozenRowGridWin->GetClientSize().y;
		}

		if (m_frozenColGridWin &&
			(gridWindow->GetType() & wxGridExtWindow::wxGridExtWindowFrozenCol) == 0)
		{
			pt.x = m_frozenColGridWin->GetClientSize().x;
		}
	}

	return pt;
}

wxGridExtWindow* wxGridExt::DevicePosToGridWindow(wxPoint pos) const
{
	return DevicePosToGridWindow(pos.x, pos.y);
}

wxGridExtWindow* wxGridExt::DevicePosToGridWindow(int x, int y) const
{
	if (m_gridWin->GetRect().Contains(x, y))
		return m_gridWin;
	else if (m_frozenCornerGridWin && m_frozenCornerGridWin->GetRect().Contains(x, y))
		return m_frozenCornerGridWin;
	else if (m_frozenRowGridWin && m_frozenRowGridWin->GetRect().Contains(x, y))
		return m_frozenRowGridWin;
	else if (m_frozenColGridWin && m_frozenColGridWin->GetRect().Contains(x, y))
		return m_frozenColGridWin;

	return NULL;
}

void wxGridExt::CalcGridWindowUnscrolledPosition(int x, int y, int* xx, int* yy,
	const wxGridExtWindow* gridWindow) const
{
	CalcUnscrolledPosition(x, y, xx, yy);

	if (gridWindow)
	{
		if (yy && (gridWindow->GetType() & wxGridExtWindow::wxGridExtWindowFrozenRow))
			*yy = y;
		if (xx && (gridWindow->GetType() & wxGridExtWindow::wxGridExtWindowFrozenCol))
			*xx = x;
	}
}

wxPoint wxGridExt::CalcGridWindowUnscrolledPosition(const wxPoint& pt,
	const wxGridExtWindow* gridWindow) const
{
	wxPoint pt2;
	CalcGridWindowUnscrolledPosition(pt.x, pt.y, &pt2.x, &pt2.y, gridWindow);
	return pt2;
}

void wxGridExt::CalcGridWindowScrolledPosition(int x, int y, int* xx, int* yy,
	const wxGridExtWindow* gridWindow) const
{
	CalcScrolledPosition(x, y, xx, yy);

	if (gridWindow)
	{
		if (yy && (gridWindow->GetType() & wxGridExtWindow::wxGridExtWindowFrozenRow))
			*yy = y;
		if (xx && (gridWindow->GetType() & wxGridExtWindow::wxGridExtWindowFrozenCol))
			*xx = x;
	}
}

wxPoint wxGridExt::CalcGridWindowScrolledPosition(const wxPoint& pt,
	const wxGridExtWindow* gridWindow) const
{
	wxPoint pt2;
	CalcGridWindowScrolledPosition(pt.x, pt.y, &pt2.x, &pt2.y, gridWindow);
	return pt2;
}

bool wxGridExt::IsVisible(int row, int col, bool wholeCellVisible) const
{
	// get the cell rectangle in logical coords
	//
	wxRect r(CellToRect(row, col));

	wxGridExtWindow* gridWindow = CellToGridWindow(row, col);
	r.Offset(-GetGridWindowOffset(gridWindow));

	// convert to device coords
	//
	int left, top, right, bottom;
	CalcGridWindowScrolledPosition(r.GetLeft(), r.GetTop(), &left, &top, gridWindow);
	CalcGridWindowScrolledPosition(r.GetRight(), r.GetBottom(), &right, &bottom, gridWindow);

	// check against the client area of the grid window
	int cw, ch;
	gridWindow->GetClientSize(&cw, &ch);

	if (wholeCellVisible)
	{
		// is the cell wholly visible ?
		return (left >= 0 && right <= cw &&
			top >= 0 && bottom <= ch);
	}
	else
	{
		// is the cell partly visible ?
		//
		return (((left >= 0 && left < cw) || (right > 0 && right <= cw)) &&
			((top >= 0 && top < ch) || (bottom > 0 && bottom <= ch)));
	}
}

// make the specified cell location visible by doing a minimal amount
// of scrolling
//
void wxGridExt::MakeCellVisible(int row, int col)
{
	int xpos = -1, ypos = -1;

	if (row < -1 || row >= m_numRows ||
		col < -1 || col >= m_numCols)
		return;

	// We don't scroll in the corresponding direction if "pixels per line" is 0.
	const bool processRow = row != -1 && m_yScrollPixelsPerLine != 0;
	const bool processCol = col != -1 && m_xScrollPixelsPerLine != 0;

	// Get the cell rectangle in logical coords.
	wxRect r;
	wxGridExtWindow* gridWindow;

	if (processRow && processCol)
	{
		r = CellToRect(row, col);
		gridWindow = CellToGridWindow(row, col);
	}
	else if (processRow)
	{
		r.SetTop(GetRowTop(row));
		r.SetHeight(GetRowHeight(row));
		gridWindow = row < m_numFrozenRows
			? m_frozenRowGridWin
			: m_gridWin;
	}
	else if (processCol)
	{
		r.SetLeft(GetColLeft(col));
		r.SetWidth(GetColWidth(col));
		gridWindow = col < m_numFrozenCols
			? m_frozenColGridWin
			: m_gridWin;
	}
	else
	{
		return;
	}

	wxPoint gridOffset = GetGridWindowOffset(gridWindow);

	if (processRow)
	{
		// Convert to device coords.
		int top, bottom;
		CalcGridWindowScrolledPosition(0, r.GetTop(), NULL, &top, gridWindow);
		CalcGridWindowScrolledPosition(0, r.GetBottom(), NULL, &bottom, gridWindow);

		int ch;
		gridWindow->GetClientSize(NULL, &ch);

		if (top < gridOffset.y)
		{
			ypos = r.GetTop() - gridOffset.y;
		}
		else if (bottom > (ch + gridOffset.y))
		{
			int h = r.GetHeight();

			ypos = r.GetTop() - gridOffset.y;

			int i;
			for (i = row - 1; i >= 0; i--)
			{
				int rowHeight = GetRowHeight(i);
				if (h + rowHeight > ch)
					break;

				h += rowHeight;
				ypos -= rowHeight;
			}

			// we divide it later by GRID_SCROLL_LINE, make sure that we don't
			// have rounding errors (this is important, because if we do,
			// we might not scroll at all and some cells won't be redrawn)
			//
			// Sometimes GRID_SCROLL_LINE / 2 is not enough,
			// so just add a full scroll unit...
			ypos += m_yScrollPixelsPerLine;
		}
	}

	if (processCol)
	{
		// Convert to device coords.
		int left, right;
		CalcGridWindowScrolledPosition(r.GetLeft(), 0, &left, NULL, gridWindow);
		CalcGridWindowScrolledPosition(r.GetRight(), 0, &right, NULL, gridWindow);

		int cw;
		gridWindow->GetClientSize(&cw, NULL);

		// special handling for wide cells - show always left part of the cell!
		// Otherwise, e.g. when stepping from row to row, it would jump between
		// left and right part of the cell on every step!
//      if ( left < 0 )
		if (left < gridOffset.x || (right - left) >= cw)
		{
			xpos = r.GetLeft() - gridOffset.x;
		}
		else if (right > (cw + gridOffset.x))
		{
			// position the view so that the cell is on the right
			int x0, y0;
			CalcGridWindowUnscrolledPosition(0, 0, &x0, &y0, gridWindow);
			xpos = x0 + (right - cw);

			// see comment for ypos above
			xpos += m_xScrollPixelsPerLine;
		}
	}

	if (xpos == -1 && ypos == -1)
		return;

	if (xpos != -1)
		xpos /= m_xScrollPixelsPerLine;
	if (ypos != -1)
		ypos /= m_yScrollPixelsPerLine;
	Scroll(xpos, ypos);
	AdjustScrollbars();
}

int wxGridExt::GetFirstFullyVisibleRow() const
{
	if (m_numRows == 0)
		return -1;

	int row;
	if (GetNumberFrozenRows() > 0)
	{
		row = 0;
	}
	else
	{
		int y;
		CalcGridWindowUnscrolledPosition(0, 0,
			NULL, &y,
			m_gridWin);

		row = internalYToRow(y, m_gridWin);

		// If the row is not fully visible (if only 2 pixels is hidden
		// the row still looks fully visible).
		if (GetRowTop(row) + 2 < y)
		{
			// Use the next visible row.
			for (;; )
			{
				// But we can't go beyond the last row anyhow.
				if (row == m_numRows - 1)
					break;

				if (IsRowShown(++row))
					break;
			}
		}
	}

	return row;
}

int wxGridExt::GetFirstFullyVisibleColumn() const
{
	if (m_numCols == 0)
		return -1;

	int col;
	if (GetNumberFrozenCols() > 0)
	{
		col = 0;
	}
	else
	{
		int x;
		CalcGridWindowUnscrolledPosition(0, 0,
			&x, NULL,
			m_gridWin);

		col = internalXToCol(x, m_gridWin);

		// If the column is not fully visible.
		if (GetColLeft(col) < x)
		{
			// Use the next visible column.
			for (;; )
			{
				if (col == m_numCols - 1)
					break;

				if (IsColShown(GetColAt(++col)))
					break;
			}
		}
	}

	return col;
}

//
// ------ Grid cursor movement functions
//

namespace
{

	// Helper function creating dummy wxKeyboardState object corresponding to the
	// value of "expandSelection" flag specified to our public MoveCursorXXX().
	// This function only exists to avoid breaking compatibility in the public API,
	// all internal code should use the real, not dummy, wxKeyboardState.
	inline
		wxKeyboardState DummyKeyboardState(bool expandSelection)
	{
		// Normally "expandSelection" is set depending on whether Shift is pressed
		// or not, but here we reconstruct the keyboard state from this flag.
		return wxKeyboardState(false /* control */, expandSelection /* shift */);
	}

} // anonymous namespace

void
wxGridExt::DoMoveCursorFromKeyboard(const wxKeyboardState& kbdState,
	const wxGridExtDirectionOperations& diroper)
{
	if (kbdState.ControlDown())
		DoMoveCursorByBlock(kbdState, diroper);
	else
		DoMoveCursor(kbdState, diroper);
}

bool
wxGridExt::DoMoveCursor(const wxKeyboardState& kbdState,
	const wxGridExtDirectionOperations& diroper)
{
	if (m_currentCellCoords == wxGridExtNoCellCoords)
		return false;

	// Expand selection if Shift is pressed.
	if (kbdState.ShiftDown())
	{
		if (!m_selection)
			return false;

		wxGridExtCellCoords coords(m_selection->GetExtensionAnchor());
		if (!diroper.TryToAdvance(coords))
			return false;

		if (m_selection->ExtendCurrentBlock(m_currentCellCoords,
			coords,
			kbdState))
		{
			// We want to show a line (a row or a column), not the end of
			// the selection block. And do it only if the selection block
			// was actually changed.
			MakeCellVisible(diroper.MakeWholeLineCoords(coords));
		}
	}
	else // don't expand selection
	{
		ClearSelection();

		wxGridExtCellCoords coords = m_currentCellCoords;
		if (!diroper.TryToAdvance(coords))
			return false;

		GoToCell(coords);
	}

	return true;
}

bool wxGridExt::MoveCursorUp(bool expandSelection)
{
	return DoMoveCursor(DummyKeyboardState(expandSelection),
		wxGridExtBackwardOperations(this, wxGridExtRowOperations()));
}

bool wxGridExt::MoveCursorDown(bool expandSelection)
{
	return DoMoveCursor(DummyKeyboardState(expandSelection),
		wxGridExtForwardOperations(this, wxGridExtRowOperations()));
}

bool wxGridExt::MoveCursorLeft(bool expandSelection)
{
	return DoMoveCursor(DummyKeyboardState(expandSelection),
		wxGridExtBackwardOperations(this, wxGridExtColumnOperations()));
}

bool wxGridExt::MoveCursorRight(bool expandSelection)
{
	return DoMoveCursor(DummyKeyboardState(expandSelection),
		wxGridExtForwardOperations(this, wxGridExtColumnOperations()));
}

bool
wxGridExt::AdvanceByPage(wxGridExtCellCoords& coords,
	const wxGridExtDirectionOperations& diroper)
{
	if (diroper.IsAtBoundary(coords))
		return false;

	const int oldRow = coords.GetRow();
	coords.SetRow(diroper.MoveByPixelDistance(oldRow, m_gridWin->GetClientSize().y));
	if (coords.GetRow() == oldRow)
		diroper.Advance(coords);

	return true;
}

bool
wxGridExt::DoMoveCursorByPage(const wxKeyboardState& kbdState,
	const wxGridExtDirectionOperations& diroper)
{
	if (m_currentCellCoords == wxGridExtNoCellCoords)
		return false;

	// We don't handle Ctrl-PageUp/Down, it's not really clear what are they
	// supposed to do, so don't do anything for now.
	if (kbdState.ControlDown())
		return false;

	if (kbdState.ShiftDown())
	{
		if (!m_selection)
			return false;

		wxGridExtCellCoords coords = m_selection->GetExtensionAnchor();
		if (!AdvanceByPage(coords, diroper))
			return false;

		if (m_selection->ExtendCurrentBlock(m_currentCellCoords, coords, kbdState))
			MakeCellVisible(diroper.MakeWholeLineCoords(coords));
	}
	else
	{
		wxGridExtCellCoords coords(m_currentCellCoords);
		if (!AdvanceByPage(coords, diroper))
			return false;

		ClearSelection();
		GoToCell(coords);
	}

	return true;
}

bool wxGridExt::MovePageUp()
{
	return DoMoveCursorByPage(DummyKeyboardState(false),
		wxGridExtBackwardOperations(this, wxGridExtRowOperations()));
}

bool wxGridExt::MovePageDown()
{
	return DoMoveCursorByPage(DummyKeyboardState(false),
		wxGridExtForwardOperations(this, wxGridExtRowOperations()));
}

// helper of DoMoveCursorByBlock(): advance the cell coordinates using diroper
// until we find a non-empty cell or reach the grid end
void
wxGridExt::AdvanceToNextNonEmpty(wxGridExtCellCoords& coords,
	const wxGridExtDirectionOperations& diroper)
{
	while (!diroper.IsAtBoundary(coords))
	{
		diroper.Advance(coords);
		if (!m_table->IsEmpty(coords))
			break;
	}
}

bool
wxGridExt::AdvanceByBlock(wxGridExtCellCoords& coords,
	const wxGridExtDirectionOperations& diroper)
{
	if (m_table->IsEmpty(coords))
	{
		// we are in an empty cell: find the next block of non-empty cells
		AdvanceToNextNonEmpty(coords, diroper);
	}
	else // current cell is not empty
	{
		if (!diroper.TryToAdvance(coords))
			return false;

		if (m_table->IsEmpty(coords))
		{
			// we started at the end of a block, find the next one
			AdvanceToNextNonEmpty(coords, diroper);
		}
		else // we're in a middle of a block
		{
			// go to the end of it, i.e. find the last cell before the next
			// empty one
			while (!diroper.IsAtBoundary(coords))
			{
				wxGridExtCellCoords coordsNext(coords);
				diroper.Advance(coordsNext);
				if (m_table->IsEmpty(coordsNext))
					break;

				coords = coordsNext;
			}
		}
	}

	return true;
}

bool
wxGridExt::DoMoveCursorByBlock(const wxKeyboardState& kbdState,
	const wxGridExtDirectionOperations& diroper)
{
	if (!m_table)
		return false;

	wxGridExtCellCoords coords(m_currentCellCoords);
	if (kbdState.ShiftDown())
	{
		if (!m_selection)
			return false;

		// Extending selection by block is tricky for several reasons. First of
		// all, we need to combine the coordinates of the current cell and the
		// anchor point of selection to find our starting point.
		//
		// To explain why we need to do this, consider the example of moving by
		// rows (i.e. vertically). In this case, it's the contents of column
		// containing the current cell which should determine the destination
		// of Shift-Ctrl-Up/Down movement, just as it does for Ctrl-Up/Down. In
		// fact, the column containing the selection anchor might not even be
		// visible at all, e.g. if a whole row is selected and that column is
		// the last one, so we definitely don't want to use the contents of
		// this column to determine the destination row.
		//
		// So instead of using the anchor itself here, use only its component
		// component in "our" direction with the current cell component.
		const wxGridExtCellCoords anchor = m_selection->GetExtensionAnchor();

		// This is a really ugly hack that we use to check if we're moving by
		// rows or columns here, but it's not worth adding a specific method
		// just for this, so just check if MakeWholeLineCoords() fixes the
		// column or the row as -1 to determine the direction we're moving in.
		const bool byRow = diroper.MakeWholeLineCoords(coords).GetCol() == -1;

		if (byRow)
			coords.SetRow(anchor.GetRow());
		else
			coords.SetCol(anchor.GetCol());

		if (!AdvanceByBlock(coords, diroper))
			return false;

		// Second, now that we've found the destination, we need to copy the
		// other component, i.e. the one we didn't modify, from the anchor to
		// do as if we started from the anchor originally.
		//
		// Again, to understand why this is necessary, consider the same
		// example as above: if we didn't do this, we would lose the column of
		// the original anchor completely and end up with a single column
		// selection block even if we had started with the full row selection.
		if (byRow)
			coords.SetCol(anchor.GetCol());
		else
			coords.SetRow(anchor.GetRow());

		if (m_selection->ExtendCurrentBlock(m_currentCellCoords,
			coords,
			kbdState))
		{
			// We want to show a line (a row or a column), not the end of
			// the selection block. And do it only if the selection block
			// was actually changed.
			MakeCellVisible(diroper.MakeWholeLineCoords(coords));
		}
	}
	else
	{
		if (!AdvanceByBlock(coords, diroper))
			return false;

		ClearSelection();
		GoToCell(coords);
	}

	return true;
}

bool wxGridExt::MoveCursorUpBlock(bool expandSelection)
{
	return DoMoveCursorByBlock(
		DummyKeyboardState(expandSelection),
		wxGridExtBackwardOperations(this, wxGridExtRowOperations())
	);
}

bool wxGridExt::MoveCursorDownBlock(bool expandSelection)
{
	return DoMoveCursorByBlock(
		DummyKeyboardState(expandSelection),
		wxGridExtForwardOperations(this, wxGridExtRowOperations())
	);
}

bool wxGridExt::MoveCursorLeftBlock(bool expandSelection)
{
	return DoMoveCursorByBlock(
		DummyKeyboardState(expandSelection),
		wxGridExtBackwardOperations(this, wxGridExtColumnOperations())
	);
}

bool wxGridExt::MoveCursorRightBlock(bool expandSelection)
{
	return DoMoveCursorByBlock(
		DummyKeyboardState(expandSelection),
		wxGridExtForwardOperations(this, wxGridExtColumnOperations())
	);
}

//
// ------ Label values and formatting
//

void wxGridExt::GetRowLabelAlignment(int* horiz, int* vert) const
{
	if (horiz)
		*horiz = m_rowLabelHorizAlign;
	if (vert)
		*vert = m_rowLabelVertAlign;
}

void wxGridExt::GetColLabelAlignment(int* horiz, int* vert) const
{
	if (horiz)
		*horiz = m_colLabelHorizAlign;
	if (vert)
		*vert = m_colLabelVertAlign;
}

int wxGridExt::GetColLabelTextOrientation() const
{
	return m_colLabelTextOrientation;
}

void wxGridExt::GetCornerLabelAlignment(int* horiz, int* vert) const
{
	if (horiz)
		*horiz = m_cornerLabelHorizAlign;
	if (vert)
		*vert = m_cornerLabelVertAlign;
}

int wxGridExt::GetCornerLabelTextOrientation() const
{
	return m_cornerLabelTextOrientation;
}

int wxGridExt::GetRowAreaValue(int row, wxString* areaLabel) const
{
	int result = 0;

	for (unsigned int n = 0; n < m_rowAreaAt.size(); n++) {

		const wxGridExtCellArea& entry = m_rowAreaAt[n];

		if (row >= entry.m_start && row <= entry.m_end)
			result += 1;
		if (row == entry.m_start)
			result += 2;
		if (row == entry.m_end)
			result += 4;

		if (result > 0) {
			if (areaLabel)
				*areaLabel = entry.m_areaLabel;
			return result;
		}
	}

	return result;
}

wxString wxGridExt::GetRowLabelValue(int row) const
{
	if (m_table)
	{
		return m_table->GetRowLabelValue(row);
	}
	else
	{
		wxString s;
		s << row;
		return s;
	}
}

int wxGridExt::GetColAreaValue(int col, wxString* areaLabel) const
{
	int result = 0;

	for (unsigned int n = 0; n < m_colAreaAt.size(); n++) {

		const wxGridExtCellArea& entry = m_colAreaAt[n];

		if (col >= entry.m_start && col <= entry.m_end)
			result += 1;
		if (col == entry.m_start)
			result += 2;
		if (col == entry.m_end)
			result += 4;

		if (result > 0) {
			if (areaLabel)
				*areaLabel = entry.m_areaLabel;
			return result;
		}
	}

	return result;
}

wxString wxGridExt::GetColLabelValue(int col) const
{
	if (m_table)
	{
		return m_table->GetColLabelValue(col);
	}
	else
	{
		wxString s;
		s << col;
		return s;
	}
}

wxString wxGridExt::GetCornerLabelValue() const
{
	if (m_table)
	{
		return m_table->GetCornerLabelValue();
	}
	else
	{
		return wxString();
	}
}

void wxGridExt::SetRowLabelSize(int width)
{
	wxASSERT(width >= 0 || width == wxGRID_AUTOSIZE);

	if (width == wxGRID_AUTOSIZE)
	{
		width = CalcColOrRowLabelAreaMinSize(wxGRID_ROW);
	}

	if (width != m_rowLabelWidth)
	{
		if (width == 0)
		{
			m_rowAreaWin->Show(false);
			m_rowLabelWin->Show(false);
			m_cornerLabelWin->Show(false);
		}
		else if (m_rowLabelWidth == 0)
		{
			if (GridRowAreaEnabled())
				m_rowAreaWin->Show(true);
			m_rowLabelWin->Show(true);
			if (m_colLabelHeight > 0)
				m_cornerLabelWin->Show(true);
		}

		m_rowLabelWidth = width;
		InvalidateBestSize();
		CalcWindowSizes();
		wxScrolledCanvas::Refresh(true);
	}

	SendEvent(wxEVT_GRID_CHANGED);
}

void wxGridExt::SetColLabelSize(int height)
{
	wxASSERT(height >= 0 || height == wxGRID_AUTOSIZE);

	if (height == wxGRID_AUTOSIZE)
	{
		height = CalcColOrRowLabelAreaMinSize(wxGRID_COLUMN);
	}

	if (height != m_colLabelHeight)
	{
		if (height == 0)
		{
			m_colAreaWin->Show(false);
			m_colLabelWin->Show(false);
			m_cornerLabelWin->Show(false);
		}
		else if (m_colLabelHeight == 0)
		{
			if (GridColAreaEnabled())
				m_colAreaWin->Show(true);

			m_colLabelWin->Show(true);

			if (m_rowLabelWidth > 0)
				m_cornerLabelWin->Show(true);
		}

		m_colLabelHeight = height;
		InvalidateBestSize();
		CalcWindowSizes();
		wxScrolledCanvas::Refresh(true);
	}

	SendEvent(wxEVT_GRID_CHANGED);
}

void wxGridExt::SetLabelBackgroundColour(const wxColour& colour)
{
	if (m_labelBackgroundColour != colour)
	{
		m_labelBackgroundColour = colour;
		m_rowAreaWin->SetBackgroundColour(colour);
		m_rowLabelWin->SetBackgroundColour(colour);
		m_colAreaWin->SetBackgroundColour(colour);
		m_colLabelWin->SetBackgroundColour(colour);
		m_cornerLabelWin->SetBackgroundColour(colour);

		if (m_rowFrozenAreaWin)
			m_rowFrozenAreaWin->SetBackgroundColour(colour);
		if (m_rowFrozenLabelWin)
			m_rowFrozenLabelWin->SetBackgroundColour(colour);
		if (m_colFrozenAreaWin)
			m_colFrozenAreaWin->SetBackgroundColour(colour);
		if (m_colFrozenLabelWin)
			m_colFrozenLabelWin->SetBackgroundColour(colour);

		if (ShouldRefresh())
		{
			m_rowAreaWin->Refresh();
			m_rowLabelWin->Refresh();
			m_colAreaWin->Refresh();
			m_colLabelWin->Refresh();
			m_cornerLabelWin->Refresh();

			if (m_rowFrozenAreaWin)
				m_rowFrozenAreaWin->Refresh();
			if (m_rowFrozenLabelWin)
				m_rowFrozenLabelWin->Refresh();
			if (m_colFrozenAreaWin)
				m_colFrozenAreaWin->Refresh();
			if (m_colFrozenLabelWin)
				m_colFrozenLabelWin->Refresh();
		}
	}
}

void wxGridExt::SetLabelTextColour(const wxColour& colour)
{
	if (m_labelTextColour != colour)
	{
		m_labelTextColour = colour;

		if (IsUsingNativeHeader())
			GetGridColHeader()->SetForegroundColour(colour);

		if (ShouldRefresh())
		{
			m_rowAreaWin->Refresh();
			m_rowLabelWin->Refresh();
			m_colAreaWin->Refresh();
			m_colLabelWin->Refresh();
		}
	}

	SendEvent(wxEVT_GRID_CHANGED);
}

void wxGridExt::SetLabelFont(const wxFont& font)
{
	m_labelFont = font;

	if (IsUsingNativeHeader())
		GetGridColHeader()->SetFont(font);

	if (ShouldRefresh())
	{
		m_rowAreaWin->Refresh();
		m_rowLabelWin->Refresh();
		m_colAreaWin->Refresh();
		m_colLabelWin->Refresh();
	}

	SendEvent(wxEVT_GRID_CHANGED);
}

void wxGridExt::SetRowLabelAlignment(int horiz, int vert)
{
	// allow old (incorrect) defs to be used
	switch (horiz)
	{
	case wxLEFT:   horiz = wxALIGN_LEFT; break;
	case wxRIGHT:  horiz = wxALIGN_RIGHT; break;
	case wxCENTRE: horiz = wxALIGN_CENTRE; break;
	}

	switch (vert)
	{
	case wxTOP:    vert = wxALIGN_TOP;    break;
	case wxBOTTOM: vert = wxALIGN_BOTTOM; break;
	case wxCENTRE: vert = wxALIGN_CENTRE; break;
	}

	if (horiz == wxALIGN_LEFT || horiz == wxALIGN_CENTRE || horiz == wxALIGN_RIGHT)
	{
		m_rowLabelHorizAlign = horiz;
	}

	if (vert == wxALIGN_TOP || vert == wxALIGN_CENTRE || vert == wxALIGN_BOTTOM)
	{
		m_rowLabelVertAlign = vert;
	}

	if (ShouldRefresh())
	{
		m_rowAreaWin->Refresh();
		m_rowLabelWin->Refresh();
	}

	SendEvent(wxEVT_GRID_CHANGED);
}

void wxGridExt::SetColLabelAlignment(int horiz, int vert)
{
	// allow old (incorrect) defs to be used
	switch (horiz)
	{
	case wxLEFT:   horiz = wxALIGN_LEFT; break;
	case wxRIGHT:  horiz = wxALIGN_RIGHT; break;
	case wxCENTRE: horiz = wxALIGN_CENTRE; break;
	}

	switch (vert)
	{
	case wxTOP:    vert = wxALIGN_TOP;    break;
	case wxBOTTOM: vert = wxALIGN_BOTTOM; break;
	case wxCENTRE: vert = wxALIGN_CENTRE; break;
	}

	if (horiz == wxALIGN_LEFT || horiz == wxALIGN_CENTRE || horiz == wxALIGN_RIGHT)
	{
		m_colLabelHorizAlign = horiz;
	}

	if (vert == wxALIGN_TOP || vert == wxALIGN_CENTRE || vert == wxALIGN_BOTTOM)
	{
		m_colLabelVertAlign = vert;
	}

	if (ShouldRefresh())
	{
		m_colAreaWin->Refresh();
		m_colLabelWin->Refresh();
	}

	SendEvent(wxEVT_GRID_CHANGED);
}

void wxGridExt::SetCornerLabelAlignment(int horiz, int vert)
{
	// allow old (incorrect) defs to be used
	switch (horiz)
	{
	case wxLEFT:   horiz = wxALIGN_LEFT; break;
	case wxRIGHT:  horiz = wxALIGN_RIGHT; break;
	case wxCENTRE: horiz = wxALIGN_CENTRE; break;
	}

	switch (vert)
	{
	case wxTOP:    vert = wxALIGN_TOP;    break;
	case wxBOTTOM: vert = wxALIGN_BOTTOM; break;
	case wxCENTRE: vert = wxALIGN_CENTRE; break;
	}

	if (horiz == wxALIGN_LEFT || horiz == wxALIGN_CENTRE || horiz == wxALIGN_RIGHT)
	{
		m_cornerLabelHorizAlign = horiz;
	}

	if (vert == wxALIGN_TOP || vert == wxALIGN_CENTRE || vert == wxALIGN_BOTTOM)
	{
		m_cornerLabelVertAlign = vert;
	}

	if (ShouldRefresh())
	{
		m_cornerLabelWin->Refresh();
	}

	SendEvent(wxEVT_GRID_CHANGED);
}

// Note: under MSW, the default column label font must be changed because it
//       does not support vertical printing
//
// Example:
//      pGrid->SetLabelFont(wxFontInfo(9).Family(wxFONTFAMILY_SWISS));
//      pGrid->SetColLabelTextOrientation(wxVERTICAL);
//
void wxGridExt::SetColLabelTextOrientation(int textOrientation)
{
	if (textOrientation == wxHORIZONTAL || textOrientation == wxVERTICAL)
		m_colLabelTextOrientation = textOrientation;

	if (ShouldRefresh())
	{
		m_colAreaWin->Refresh();
		m_colLabelWin->Refresh();
	}

	SendEvent(wxEVT_GRID_CHANGED);
}

void wxGridExt::SetCornerLabelTextOrientation(int textOrientation)
{
	if (textOrientation == wxHORIZONTAL || textOrientation == wxVERTICAL)
		m_cornerLabelTextOrientation = textOrientation;

	if (ShouldRefresh())
		m_cornerLabelWin->Refresh();

	SendEvent(wxEVT_GRID_CHANGED);
}

void wxGridExt::SetRowLabelValue(int row, const wxString& s)
{
	if (m_table)
	{
		m_table->SetRowLabelValue(row, s);
		if (ShouldRefresh())
		{
			wxRect rect = CellToRect(row, 0);
			if (rect.height > 0)
			{
				CalcScrolledPosition(0, rect.y, &rect.x, &rect.y);

				rect.x = 0;
				rect.width = m_rowLabelWidth;

				m_rowAreaWin->Refresh(true, &rect);
				m_rowLabelWin->Refresh(true, &rect);
			}
		}

		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetColLabelValue(int col, const wxString& s)
{
	if (m_table)
	{
		m_table->SetColLabelValue(col, s);
		if (ShouldRefresh())
		{
			if (m_useNativeHeader)
			{
				GetGridColHeader()->UpdateColumn(col);
			}
			else
			{
				wxRect rect = CellToRect(0, col);
				if (rect.width > 0)
				{
					CalcScrolledPosition(rect.x, 0, &rect.x, &rect.y);
					rect.y = 0;
					rect.height = (GridColAreaEnabled() ? m_colAreaHeight : 0) + m_colLabelHeight;
					GetColLabelWindow()->Refresh(true, &rect);
				}
			}
		}

		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetCornerLabelValue(const wxString& s)
{
	if (m_table)
	{
		m_table->SetCornerLabelValue(s);
		if (ShouldRefresh())
		{
			wxRect rect = m_cornerLabelWin->GetRect();
			m_cornerLabelWin->Refresh(true, &rect);
		}

		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetGridLineColour(const wxColour& colour)
{
	if (m_gridLineColour != colour)
	{
		m_gridLineColour = colour;

		if (GridLinesEnabled())
			RedrawGridLines();

		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetCellHighlightColour(const wxColour& colour)
{
	if (m_cellHighlightColour != colour)
	{
		m_cellHighlightColour = colour;

		RefreshBlock(m_currentCellCoords, m_currentCellCoords);
		
		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetCellHighlightPenWidth(int width)
{
	if (m_cellHighlightPenWidth != width)
	{
		m_cellHighlightPenWidth = width;

		// Just redrawing the cell highlight is not enough since that won't
		// make any visible change if the thickness is getting smaller.
		int row = m_currentCellCoords.GetRow();
		int col = m_currentCellCoords.GetCol();
		if (row == -1 || col == -1 || GetColWidth(col) <= 0 || GetRowHeight(row) <= 0)
			return;

		wxRect rect = CellToRect(row, col);
		wxGridExtWindow* gridWindow = CellToGridWindow(row, col);
		gridWindow->Refresh(true, &rect);

		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetCellHighlightROPenWidth(int width)
{
	if (m_cellHighlightROPenWidth != width)
	{
		m_cellHighlightROPenWidth = width;

		// Just redrawing the cell highlight is not enough since that won't
		// make any visible change if the thickness is getting smaller.
		int row = m_currentCellCoords.GetRow();
		int col = m_currentCellCoords.GetCol();
		if (row == -1 || col == -1 ||
			GetColWidth(col) <= 0 || GetRowHeight(row) <= 0)
			return;

		wxRect rect = CellToRect(row, col);
		wxGridExtWindow* gridWindow = CellToGridWindow(row, col);
		gridWindow->Refresh(true, &rect);

		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetGridFrozenBorderColour(const wxColour& colour)
{
	if (m_gridFrozenBorderColour != colour)
	{
		m_gridFrozenBorderColour = colour;

		if (ShouldRefresh())
		{
			if (m_frozenRowGridWin)
				m_frozenRowGridWin->Refresh();
			if (m_frozenColGridWin)
				m_frozenColGridWin->Refresh();
		}

		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetGridFrozenBorderPenWidth(int width)
{
	if (m_gridFrozenBorderPenWidth != width)
	{
		m_gridFrozenBorderPenWidth = width;

		if (ShouldRefresh())
		{
			if (m_frozenRowGridWin)
				m_frozenRowGridWin->Refresh();
			if (m_frozenColGridWin)
				m_frozenColGridWin->Refresh();
		}

		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::RedrawGridLines()
{
	// the lines will be redrawn when the window is thawed or shown
	if (!ShouldRefresh())
		return;

	m_gridWin->Refresh();

	if (m_frozenColGridWin)
		m_frozenColGridWin->Refresh();
	if (m_frozenRowGridWin)
		m_frozenRowGridWin->Refresh();
	if (m_frozenCornerGridWin)
		m_frozenCornerGridWin->Refresh();
}

void wxGridExt::EnableGridLines(bool enable)
{
	if (enable != m_gridLinesEnabled)
	{
		m_gridLinesEnabled = enable;

		RedrawGridLines();
	}
}

void wxGridExt::DoClipGridLines(bool& var, bool clip)
{
	if (clip != var)
	{
		var = clip;

		if (GridLinesEnabled())
			RedrawGridLines();
	}
}

int wxGridExt::GetDefaultRowSize() const
{
	return m_defaultRowHeight;
}

int wxGridExt::GetRowSize(int row) const
{
	wxCHECK_MSG(row >= 0 && row < m_numRows, 0, wxT("invalid row index"));

	return GetRowHeight(row);
}

int wxGridExt::GetDefaultColSize() const
{
	return m_defaultColWidth;
}

int wxGridExt::GetColSize(int col) const
{
	wxCHECK_MSG(col >= 0 && col < m_numCols, 0, wxT("invalid column index"));

	return GetColWidth(col);
}

// ============================================================================
// access to the grid attributes: each of them has a default value in the grid
// itself and may be overidden on a per-cell basis
// ============================================================================

// ----------------------------------------------------------------------------
// setting default attributes
// ----------------------------------------------------------------------------

void wxGridExt::SetDefaultCellBackgroundColour(const wxColour& col)
{
	m_defaultCellAttr->SetBackgroundColour(col);
#if defined(__WXGTK__) || defined(__WXQT__)
	m_gridWin->SetBackgroundColour(col);
#endif
}

void wxGridExt::SetDefaultCellTextColour(const wxColour& col)
{
	m_defaultCellAttr->SetTextColour(col);
}

void wxGridExt::SetDefaultCellTextOrient(const int& orient)
{
	m_defaultCellAttr->SetTextOrient(orient);
}

void wxGridExt::SetDefaultCellAlignment(int horiz, int vert)
{
	m_defaultCellAttr->SetAlignment(horiz, vert);
}

void wxGridExt::SetDefaultCellFitMode(wxGridExtFitMode fitMode)
{
	m_defaultCellAttr->SetFitMode(fitMode);
}

void wxGridExt::SetDefaultCellFont(const wxFont& font)
{
	m_defaultCellAttr->SetFont(font);
}

void wxGridExt::SetDefaultCellBorderLeft(wxPenStyle style, const wxColour& colour, int width)
{
	m_defaultCellAttr->SetBorderLeft(style, colour, width);
}

void wxGridExt::SetDefaultCellBorderRight(wxPenStyle style, const wxColour& colour, int width)
{
	m_defaultCellAttr->SetBorderRight(style, colour, width);
}

void wxGridExt::SetDefaultCellBorderTop(wxPenStyle style, const wxColour& colour, int width)
{
	m_defaultCellAttr->SetBorderTop(style, colour, width);
}

void wxGridExt::SetDefaultCellBorderBottom(wxPenStyle style, const wxColour& colour, int width)
{
	m_defaultCellAttr->SetBorderBottom(style, colour, width);
}

// For editors and renderers the type registry takes precedence over the
// default attr, so we need to register the new editor/renderer for the string
// data type in order to make setting a default editor/renderer appear to
// work correctly.

void wxGridExt::SetDefaultRenderer(wxGridExtCellRenderer* renderer)
{
	RegisterDataType(wxGRID_VALUE_STRING,
		renderer,
		GetDefaultEditorForType(wxGRID_VALUE_STRING));
}

void wxGridExt::SetDefaultEditor(wxGridExtCellEditor* editor)
{
	RegisterDataType(wxGRID_VALUE_STRING,
		GetDefaultRendererForType(wxGRID_VALUE_STRING),
		editor);
}

// ----------------------------------------------------------------------------
// access to the default attributes
// ----------------------------------------------------------------------------

wxColour wxGridExt::GetDefaultCellBackgroundColour() const
{
	return m_defaultCellAttr->GetBackgroundColour();
}

wxColour wxGridExt::GetDefaultCellTextColour() const
{
	return m_defaultCellAttr->GetTextColour();
}

wxFont wxGridExt::GetDefaultCellFont() const
{
	return m_defaultCellAttr->GetFont();
}

void wxGridExt::GetDefaultCellAlignment(int* horiz, int* vert) const
{
	m_defaultCellAttr->GetAlignment(horiz, vert);
}

wxGridExtCellBorder wxGridExt::GetDefaultCellBorderLeft() const
{
	return m_defaultCellAttr->GetBorderLeft();
}

wxGridExtCellBorder wxGridExt::GetDefaultCellBorderRight() const
{
	return m_defaultCellAttr->GetBorderRight();
}

wxGridExtCellBorder wxGridExt::GetDefaultCellBorderTop() const
{
	return m_defaultCellAttr->GetBorderTop();
}

wxGridExtCellBorder wxGridExt::GetDefaultCellBorderBottom() const
{
	return m_defaultCellAttr->GetBorderBottom();
}

wxGridExtFitMode wxGridExt::GetDefaultCellFitMode() const
{
	return m_defaultCellAttr->GetFitMode();
}

wxGridExtCellRenderer* wxGridExt::GetDefaultRenderer() const
{
	return m_defaultCellAttr->GetRenderer(NULL, 0, 0);
}

wxGridExtCellEditor* wxGridExt::GetDefaultEditor() const
{
	return m_defaultCellAttr->GetEditor(NULL, 0, 0);
}

// ----------------------------------------------------------------------------
// access to cell attributes
// ----------------------------------------------------------------------------

wxColour wxGridExt::GetCellBackgroundColour(int row, int col) const
{
	return GetCellAttrPtr(row, col)->GetBackgroundColour();
}

wxColour wxGridExt::GetCellTextColour(int row, int col) const
{
	return GetCellAttrPtr(row, col)->GetTextColour();
}

int wxGridExt::GetCellTextOrient(int row, int col) const
{
	return GetCellAttrPtr(row, col)->GetTextOrient();
}

wxFont wxGridExt::GetCellFont(int row, int col) const
{
	return  GetCellAttrPtr(row, col)->GetFont();
}

void wxGridExt::GetCellAlignment(int row, int col, int* horiz, int* vert) const
{
	return  GetCellAttrPtr(row, col)->GetAlignment(horiz, vert);
}

wxGridExtCellBorder wxGridExt::GetCellBorderLeft(int row, int col) const
{
	return GetCellAttrPtr(row, col)->GetBorderLeft();
}

wxGridExtCellBorder wxGridExt::GetCellBorderRight(int row, int col) const
{
	return GetCellAttrPtr(row, col)->GetBorderRight();
}

wxGridExtCellBorder wxGridExt::GetCellBorderTop(int row, int col) const
{
	return GetCellAttrPtr(row, col)->GetBorderTop();
}

wxGridExtCellBorder wxGridExt::GetCellBorderBottom(int row, int col) const
{
	return GetCellAttrPtr(row, col)->GetBorderBottom();
}

wxGridExtFitMode wxGridExt::GetCellFitMode(int row, int col) const
{
	return GetCellAttrPtr(row, col)->GetFitMode();
}

wxGridExtCellArea* wxGridExt::GetRowArea(int row) const
{
	for (size_t pos = 0; pos < m_rowAreaAt.size(); pos++) {
		wxGridExtCellArea& entry = m_rowAreaAt[pos];
		if (entry.m_start <= row && entry.m_end >= row)
			return &entry;
	}

	return NULL;
}

wxGridExtCellArea* wxGridExt::GetColArea(int col) const
{
	for (size_t pos = 0; pos < m_colAreaAt.size(); pos++) {
		wxGridExtCellArea& entry = m_colAreaAt[pos];
		if (entry.m_start <= col && entry.m_end >= col)
			return &entry;
	}

	return NULL;
}

wxGridExt::CellSpan
wxGridExt::GetCellSize(int row, int col, int* num_rows, int* num_cols) const
{
	GetCellAttrPtr(row, col)->GetSize(num_rows, num_cols);

	return GetCellSpan(*num_rows, *num_cols);
}

wxGridExtCellRenderer* wxGridExt::GetCellRenderer(int row, int col) const
{
	return  GetCellAttrPtr(row, col)->GetRenderer(this, row, col);
}

wxGridExtCellEditor* wxGridExt::GetCellEditor(int row, int col) const
{
	return GetCellAttrPtr(row, col)->GetEditor(this, row, col);
}

bool wxGridExt::IsReadOnly(int row, int col) const
{
	return GetCellAttrPtr(row, col)->IsReadOnly();
}

// ----------------------------------------------------------------------------
// area support: cache, automatic provider creation, ...
// ----------------------------------------------------------------------------

void wxGridExt::AddAreaRow(int start, int end)
{
	for (unsigned int idx = 0; idx < m_rowAreaAt.size(); idx++) {

		wxGridExtCellArea& item = m_rowAreaAt[idx];

		if (start >= item.m_start && start <= item.m_end) {
			if (end > item.m_end) { //expand the bottom border
				item.m_end = end;
				SetRowBrake(end);
				return;
			}
		}
		else if (end >= item.m_start && end <= item.m_end) {

			if (start < item.m_start) { //extending the upper bound
				item.m_start = start;
				return;
			}
		}

		if (start >= item.m_start && start <= item.m_end)
			return;

		if (end >= item.m_start && end <= item.m_end)
			return;
	}

	unsigned int countRec = 1;

	//adding a new section
	wxGridExtCellArea entry;

	entry.m_start = start;
	entry.m_end = end;
	entry.m_areaLabel = wxString::Format(wxT("%s%d"), _("Area"), countRec);

	while (countRec > 0)
	{

		bool foundedName = false;

		for (unsigned int idx = 0; idx < m_rowAreaAt.Count(); idx++)
		{
			if (entry.m_areaLabel == m_rowAreaAt[idx].m_areaLabel)
			{
				foundedName = true;
				break;
			}
		}

		if (!foundedName)
			break;

		entry.m_areaLabel = wxString::Format(wxT("%s%d"), _("Area"), ++countRec);
	}

	if (MakeRowAreaLabel(&entry))
	{
		m_rowAreaAt.Add(entry);
		SetRowBrake(end);
		CalcDimensions();
	}

	SendEvent(wxEVT_GRID_CHANGED);
}

void wxGridExt::AddAreaCol(int start, int end)
{
	for (unsigned int idx = 0; idx < m_colAreaAt.size(); idx++) {

		wxGridExtCellArea& item = m_colAreaAt[idx];

		if (start >= item.m_start && start <= item.m_end) {
			if (end > item.m_end) {//expand the bottom border
				item.m_end = end;
				SetColBrake(end);
				return;
			}
		}
		else if (end >= item.m_start && end <= item.m_end) {

			if (start < item.m_start) { //extending the upper bound
				item.m_start = start;
				return;
			}
		}

		if (start >= item.m_start && start <= item.m_end)
			return;

		if (end >= item.m_start && end <= item.m_end)
			return;
	}

	unsigned int countRec = 1;

	//adding a new section
	wxGridExtCellArea entry;

	entry.m_start = start;
	entry.m_end = end;
	entry.m_areaLabel = wxString::Format(wxT("%s%d"), _("Area"), countRec);

	while (countRec > 0) {

		bool foundedName = false;

		for (unsigned int idx = 0; idx < m_colAreaAt.Count(); idx++)
		{
			if (entry.m_areaLabel == m_colAreaAt[idx].m_areaLabel)
			{
				foundedName = true;
				break;
			}
		}

		if (!foundedName)
			break;

		entry.m_areaLabel = wxString::Format(wxT("%s%d"), _("Area"), ++countRec);
	}

	if (MakeColAreaName(&entry))
	{
		m_colAreaAt.Add(entry);
		CalcDimensions();
		SetColBrake(end);
	}

	SendEvent(wxEVT_GRID_CHANGED);
}

void wxGridExt::AddArea()
{
	if (m_selection != NULL)
	{
		int row1 = m_numRows, col1 = m_numCols,
			row2 = 0, col2 = 0; bool hasBlocks = false;

		for (const auto coords : GetSelectedBlocks()) {
			if (row1 > coords.GetTopRow()) row1 = coords.GetTopRow();
			if (col1 > coords.GetLeftCol()) col1 = coords.GetLeftCol();
			if (row2 < coords.GetBottomRow()) row2 = coords.GetBottomRow();
			if (col2 < coords.GetRightCol()) col2 = coords.GetRightCol();
			hasBlocks = true;
		}

		if (!hasBlocks) {
			if (row1 > GetGridCursorRow()) row1 = GetGridCursorRow();
			if (col1 > GetGridCursorCol()) col1 = GetGridCursorCol();
			if (row2 < GetGridCursorRow()) row2 = GetGridCursorRow();
			if (col2 < GetGridCursorCol()) col2 = GetGridCursorCol();
		}

		if (col2 == m_numCols - 1 && row2 != m_numRows - 1) {
			AddAreaRow(row1, row2); //horzontal
		}
		else if (col2 != m_numCols - 1 && row2 == m_numRows - 1) {
			AddAreaCol(col1, col2); //vertical
		}
	}
}

void wxGridExt::DeleteAreaRow(int start, int end)
{
	for (unsigned int idx = 0; idx < m_rowAreaAt.size(); idx++)
	{
		wxGridExtCellArea& item = m_rowAreaAt[idx];

		if (start == item.m_start && end == item.m_end) { //deleting a section
			m_rowAreaAt.Detach(idx);
		}
		else if (start >= item.m_start && start <= item.m_end) {
			if (end >= item.m_end) { //removing the bottom border		
				item.m_end = start - 1;
			}
		}
		else if (end >= item.m_start && end <= item.m_end) {
			if (start <= item.m_start) { //removing the top border
				item.m_start = end + 1;
			}
		}
	}

	SendEvent(wxEVT_GRID_CHANGED);
	CalcDimensions();
}

void wxGridExt::DeleteAreaCol(int start, int end)
{
	for (unsigned int idx = 0; idx < m_colAreaAt.size(); idx++)
	{
		wxGridExtCellArea& item = m_colAreaAt[idx];

		if (start == item.m_start && end == item.m_end) { //deleting a section
			m_colAreaAt.Detach(idx);
		}
		else if (start >= item.m_start && start <= item.m_end) {
			if (end >= item.m_end) { //removing the bottom border		
				item.m_end = start - 1;
			}
		}
		else if (end >= item.m_start && end <= item.m_end) {
			if (start <= item.m_start) { //removing the top border
				item.m_start = end + 1;
			}
		}
	}

	SendEvent(wxEVT_GRID_CHANGED);
	CalcDimensions();
}

void wxGridExt::DeleteArea()
{
	if (m_selection != NULL)
	{
		int row1 = m_numRows, col1 = m_numCols,
			row2 = 0, col2 = 0; bool hasBlocks = false;

		for (const auto coords : GetSelectedBlocks()) {
			if (row1 > coords.GetTopRow()) row1 = coords.GetTopRow();
			if (col1 > coords.GetLeftCol()) col1 = coords.GetLeftCol();
			if (row2 < coords.GetBottomRow()) row2 = coords.GetBottomRow();
			if (col2 < coords.GetRightCol()) col2 = coords.GetRightCol();
			hasBlocks = true;
		}

		if (!hasBlocks) {
			if (row1 > GetGridCursorRow()) row1 = GetGridCursorRow();
			if (col1 > GetGridCursorCol()) col1 = GetGridCursorCol();
			if (row2 < GetGridCursorRow()) row2 = GetGridCursorRow();
			if (col2 < GetGridCursorCol()) col2 = GetGridCursorCol();
		}

		if (col2 == m_numCols - 1 && row2 != m_numRows - 1) {
			DeleteAreaRow(row1, row2); //horzontal
		}
		else if (col2 != m_numCols - 1 && row2 == m_numRows - 1) {
			DeleteAreaCol(col1, col2); //vertical
		}
	}
}

//make new name
bool wxGridExt::MakeRowAreaLabel(wxGridExtCellArea* rowArea)
{
	if (rowArea != NULL)
	{
		wxGridExtDialogInputArea dlg(this);
		dlg.SetAreaLabel(rowArea->m_areaLabel);

		if (dlg.ShowModal() == wxID_OK)
		{
			const wxString& areaLabel = dlg.GetAreaLabel();

			for (unsigned int idx = 0; idx < m_rowAreaAt.size(); idx++)
			{
				if (rowArea != &m_rowAreaAt[idx] && areaLabel == m_rowAreaAt[idx].m_areaLabel)
				{
					wxMessageBox(_("Wrong area name!"), _("Grid editor"));
					return false;
				}
			}

			bool valid_area = true;

			for (const auto c : areaLabel)
			{
				if (c == wxT('_') || iswalpha(c) || isdigit(c))
					continue;

				valid_area = false;
				break;
			}

			if (valid_area)
			{
				rowArea->m_areaLabel = areaLabel;
				const int founded = m_rowAreaAt.Index(*rowArea);
				if (founded != wxNOT_FOUND)
					SendEvent(wxEVT_GRID_CHANGED);
				return true;
			}

			wxMessageBox(_("Wrong area name!"), _("Grid editor"));
		}
	}

	return false;
}

bool wxGridExt::MakeColAreaName(wxGridExtCellArea* colArea)
{
	if (colArea != NULL)
	{
		wxGridExtDialogInputArea dlg(this);
		dlg.SetAreaLabel(colArea->m_areaLabel);

		if (dlg.ShowModal() == wxID_OK)
		{
			const wxString& areaLabel = dlg.GetAreaLabel();

			for (unsigned int idx = 0; idx < m_colAreaAt.size(); idx++)
			{
				if (colArea != &m_colAreaAt[idx] && areaLabel == m_colAreaAt[idx].m_areaLabel)
				{
					wxMessageBox(_("Wrong area name!"), _("Grid editor"), wxOK | wxCENTRE, this);
					return false;
				}
			}

			bool valid_area = true;

			for (const auto c : areaLabel)
			{
				if (c == wxT('_') || iswalpha(c) || isdigit(c))
					continue;

				valid_area = false;
				break;
			}

			if (valid_area)
			{
				colArea->m_areaLabel = areaLabel;

				const int founded = m_colAreaAt.Index(*colArea);
				if (founded != wxNOT_FOUND)
					SendEvent(wxEVT_GRID_CHANGED);
				return true;
			}

			wxMessageBox(_("Wrong area name!"), _("Grid editor"), wxOK | wxCENTRE, this);
		}
	}

	return false;
}

// ----------------------------------------------------------------------------
// attribute support: cache, automatic provider creation, ...
// ----------------------------------------------------------------------------

bool wxGridExt::CanHaveAttributes() const
{
	if (!m_table)
	{
		return false;
	}

	return m_table->CanHaveAttributes();
}

void wxGridExt::ClearAttrCache()
{
	if (m_attrCache.row != -1)
	{
		wxGridExtCellAttr* oldAttr = m_attrCache.attr;
		m_attrCache.attr = NULL;
		m_attrCache.row = -1;
		// wxSafeDecRec(...) might cause event processing that accesses
		// the cached attribute, if one exists (e.g. by deleting the
		// editor stored within the attribute). Therefore it is important
		// to invalidate the cache  before calling wxSafeDecRef!
		wxSafeDecRef(oldAttr);
	}
}

void wxGridExt::RefreshAttr(int row, int col)
{
	if (m_attrCache.row == row && m_attrCache.col == col)
		ClearAttrCache();
}


void wxGridExt::CacheAttr(int row, int col, wxGridExtCellAttr* attr) const
{
	if (attr != NULL)
	{
		wxGridExt* const self = const_cast<wxGridExt*>(this);

		self->ClearAttrCache();
		self->m_attrCache.row = row;
		self->m_attrCache.col = col;
		self->m_attrCache.attr = attr;
		wxSafeIncRef(attr);
	}
}

bool wxGridExt::LookupAttr(int row, int col, wxGridExtCellAttr** attr) const
{
	if (row == m_attrCache.row && col == m_attrCache.col)
	{
		*attr = m_attrCache.attr;
		wxSafeIncRef(m_attrCache.attr);

#ifdef DEBUG_ATTR_CACHE
		gs_nAttrCacheHits++;
#endif

		return true;
	}
	else
	{
#ifdef DEBUG_ATTR_CACHE
		gs_nAttrCacheMisses++;
#endif

		return false;
	}
}

wxGridExtCellAttr* wxGridExt::GetCellAttr(int row, int col) const
{
	wxGridExtCellAttr* attr = NULL;
	// Additional test to avoid looking at the cache e.g. for
	// wxNoCellCoords, as this will confuse memory management.
	if (row >= 0)
	{
		if (!LookupAttr(row, col, &attr))
		{
			attr = m_table ? m_table->GetAttr(row, col, wxGridExtCellAttr::Any)
				: NULL;
			CacheAttr(row, col, attr);
		}
	}

	if (attr)
	{
		attr->SetDefAttr(m_defaultCellAttr);
	}
	else
	{
		attr = m_defaultCellAttr;
		attr->IncRef();
	}

	return attr;
}

wxGridExtCellAttr* wxGridExt::GetOrCreateCellAttr(int row, int col) const
{
	wxGridExtCellAttr* attr = NULL;
	const bool canHave = CanHaveAttributes();

	wxCHECK_MSG(canHave, attr, wxT("Cell attributes not allowed"));
	wxCHECK_MSG(m_table, attr, wxT("must have a table"));

	attr = m_table->GetAttr(row, col, wxGridExtCellAttr::Cell);
	if (!attr)
	{
		attr = new wxGridExtCellAttr(m_defaultCellAttr);

		// artificially inc the ref count to match DecRef() in caller
		attr->IncRef();
		m_table->SetAttr(attr, row, col);
	}

	return attr;
}

// ----------------------------------------------------------------------------
// setting column attributes (wrappers around SetColAttr)
// ----------------------------------------------------------------------------

void wxGridExt::SetColFormatBool(int col)
{
	SetColFormatCustom(col, wxGRID_VALUE_BOOL);
}

void wxGridExt::SetColFormatNumber(int col)
{
	SetColFormatCustom(col, wxGRID_VALUE_NUMBER);
}

void wxGridExt::SetColFormatFloat(int col, int width, int precision)
{
	wxString typeName = wxGRID_VALUE_FLOAT;
	if ((width != -1) || (precision != -1))
	{
		typeName << wxT(':') << width << wxT(',') << precision;
	}

	SetColFormatCustom(col, typeName);
}

void wxGridExt::SetColFormatDate(int col, const wxString& format)
{
	wxString typeName = wxGRID_VALUE_DATE;
	if (!format.empty())
	{
		typeName << ':' << format;
	}
	SetColFormatCustom(col, typeName);
}

void wxGridExt::SetColFormatCustom(int col, const wxString& typeName)
{
	wxGridExtCellAttr* attr = m_table->GetAttr(-1, col, wxGridExtCellAttr::Col);
	if (!attr)
		attr = new wxGridExtCellAttr;
	wxGridExtCellRenderer* renderer = GetDefaultRendererForType(typeName);
	attr->SetRenderer(renderer);
	wxGridExtCellEditor* editor = GetDefaultEditorForType(typeName);
	attr->SetEditor(editor);

	SetColAttr(col, attr);

}

// ----------------------------------------------------------------------------
// setting cell attributes: this is forwarded to the table
// ----------------------------------------------------------------------------

void wxGridExt::SetAttr(int row, int col, wxGridExtCellAttr* attr)
{
	if (CanHaveAttributes())
	{
		m_table->SetAttr(attr, row, col);
		ClearAttrCache();
	}
	else
	{
		wxSafeDecRef(attr);
	}

	SendEvent(wxEVT_GRID_CHANGED);
}

void wxGridExt::SetRowAttr(int row, wxGridExtCellAttr* attr)
{
	if (CanHaveAttributes())
	{
		m_table->SetRowAttr(attr, row);
		ClearAttrCache();
	}
	else
	{
		wxSafeDecRef(attr);
	}

	SendEvent(wxEVT_GRID_CHANGED);
}

void wxGridExt::SetColAttr(int col, wxGridExtCellAttr* attr)
{
	if (CanHaveAttributes())
	{
		m_table->SetColAttr(attr, col);
		ClearAttrCache();
	}
	else
	{
		wxSafeDecRef(attr);
	}

	SendEvent(wxEVT_GRID_CHANGED);
}

void wxGridExt::SetCellBackgroundColour(int row, int col, const wxColour& colour)
{
	if (CanHaveAttributes())
	{
		GetOrCreateCellAttrPtr(row, col)->SetBackgroundColour(colour);
		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetCellTextColour(int row, int col, const wxColour& colour)
{
	if (CanHaveAttributes())
	{
		GetOrCreateCellAttrPtr(row, col)->SetTextColour(colour);
		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetCellTextOrient(int row, int col, const int& orient)
{
	if (CanHaveAttributes())
	{
		GetOrCreateCellAttrPtr(row, col)->SetTextOrient(orient);
		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetCellFont(int row, int col, const wxFont& font)
{
	if (CanHaveAttributes())
	{
		GetOrCreateCellAttrPtr(row, col)->SetFont(font);
		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetCellAlignment(int row, int col, int horiz, int vert)
{
	if (CanHaveAttributes())
	{
		GetOrCreateCellAttrPtr(row, col)->SetAlignment(horiz, vert);
		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetCellBorderLeft(int row, int col, wxPenStyle style, const wxColour& colour, int width)
{
	if (CanHaveAttributes())
	{
		GetOrCreateCellAttrPtr(row, col)->SetBorderLeft(style, colour, width);
		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetCellBorderRight(int row, int col, wxPenStyle style, const wxColour& colour, int width)
{
	if (CanHaveAttributes())
	{
		GetOrCreateCellAttrPtr(row, col)->SetBorderRight(style, colour, width);
		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetCellBorderTop(int row, int col, wxPenStyle style, const wxColour& colour, int width)
{
	if (CanHaveAttributes())
	{
		GetOrCreateCellAttrPtr(row, col)->SetBorderTop(style, colour, width);
		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetCellBorderBottom(int row, int col, wxPenStyle style, const wxColour& colour, int width)
{
	if (CanHaveAttributes())
	{
		GetOrCreateCellAttrPtr(row, col)->SetBorderBottom(style, colour, width);
		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetCellFitMode(int row, int col, wxGridExtFitMode fitMode)
{
	if (CanHaveAttributes())
	{
		GetOrCreateCellAttrPtr(row, col)->SetFitMode(fitMode);
		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetCellSize(int row, int col, int num_rows, int num_cols)
{
	if (CanHaveAttributes())
	{
		int cell_rows, cell_cols;

		wxGridExtCellAttrPtr attr = GetOrCreateCellAttrPtr(row, col);
		attr->GetSize(&cell_rows, &cell_cols);
		attr->SetSize(num_rows, num_cols);

		SetRowBrake(row + num_rows - 1);
		SetColBrake(col + num_cols - 1);

		// Cannot set the size of a cell to 0 or negative values
		// While it is perfectly legal to do that, this function cannot
		// handle all the possibilies, do it by hand by getting the CellAttr.
		// You can only set the size of a cell to 1,1 or greater with this fn
		wxASSERT_MSG(!((cell_rows < 1) || (cell_cols < 1)),
			wxT("wxGridExt::SetCellSize setting cell size that is already part of another cell"));
		wxASSERT_MSG(!((num_rows < 1) || (num_cols < 1)),
			wxT("wxGridExt::SetCellSize setting cell size to < 1"));

		// if this was already a multicell then "turn off" the other cells first
		if ((cell_rows > 1) || (cell_cols > 1))
		{
			int i, j;
			for (j = row; j < row + cell_rows; j++)
			{
				for (i = col; i < col + cell_cols; i++)
				{
					if ((i != col) || (j != row))
					{
						GetOrCreateCellAttrPtr(j, i)->SetSize(1, 1);
					}
				}
			}
		}

		// mark the cells that will be covered by this cell to
		// negative or zero values to point back at this cell
		if (((num_rows > 1) || (num_cols > 1)) && (num_rows >= 1) && (num_cols >= 1))
		{
			int i, j;
			for (j = row; j < row + num_rows; j++)
			{
				for (i = col; i < col + num_cols; i++)
				{
					if ((i != col) || (j != row))
					{
						GetOrCreateCellAttrPtr(j, i)->SetSize(row - j, col - i);
					}
				}
			}
		}

		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetCellRenderer(int row, int col, wxGridExtCellRenderer* renderer)
{
	if (CanHaveAttributes())
	{
		GetOrCreateCellAttrPtr(row, col)->SetRenderer(renderer);
		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetCellEditor(int row, int col, wxGridExtCellEditor* editor)
{
	if (CanHaveAttributes())
	{
		GetOrCreateCellAttrPtr(row, col)->SetEditor(editor);
		SendEvent(wxEVT_GRID_CHANGED);
	}
}

void wxGridExt::SetReadOnly(int row, int col, bool isReadOnly)
{
	if (CanHaveAttributes())
	{
		GetOrCreateCellAttrPtr(row, col)->SetReadOnly(isReadOnly);
		SendEvent(wxEVT_GRID_CHANGED);
	}
}

// ----------------------------------------------------------------------------
// Data type registration
// ----------------------------------------------------------------------------

void wxGridExt::RegisterDataType(const wxString& typeName,
	wxGridExtCellRenderer* renderer,
	wxGridExtCellEditor* editor)
{
	m_typeRegistry->RegisterDataType(typeName, renderer, editor);
}

wxGridExtCellEditor* wxGridExt::GetDefaultEditorForCell(int row, int col) const
{
	if (!m_table)
		return NULL;

	static wxString typeName;
	if (!m_table->GetTypeName(row, col, typeName))
		return NULL;
	return GetDefaultEditorForType(typeName);
}

wxGridExtCellRenderer* wxGridExt::GetDefaultRendererForCell(int row, int col) const
{
	if (!m_table)
		return NULL;

	static wxString typeName;
	if (!m_table->GetTypeName(row, col, typeName))
		return NULL;
	return GetDefaultRendererForType(typeName);
}

wxGridExtCellEditor* wxGridExt::GetDefaultEditorForType(const wxString& typeName) const
{
	int index = m_typeRegistry->FindOrCloneDataType(typeName);
	if (index == wxNOT_FOUND)
	{
		wxFAIL_MSG(wxString::Format(wxT("Unknown data type name [%s]"), typeName.c_str()));

		return NULL;
	}

	return m_typeRegistry->GetEditor(index);
}

wxGridExtCellRenderer* wxGridExt::GetDefaultRendererForType(const wxString& typeName) const
{
	int index = m_typeRegistry->FindOrCloneDataType(typeName);
	if (index == wxNOT_FOUND)
	{
		wxFAIL_MSG(wxString::Format(wxT("Unknown data type name [%s]"), typeName.c_str()));

		return NULL;
	}

	return m_typeRegistry->GetRenderer(index);
}

// ----------------------------------------------------------------------------
// row/col size
// ----------------------------------------------------------------------------

void wxGridExt::DoDisableLineResize(int line, wxGridExtFixedIndicesSet*& setFixed)
{
	if (!setFixed)
	{
		setFixed = new wxGridExtFixedIndicesSet;
	}

	setFixed->insert(line);
}

bool
wxGridExt::DoCanResizeLine(int line, const wxGridExtFixedIndicesSet* setFixed) const
{
	return !setFixed || !setFixed->count(line);
}

void wxGridExt::EnableDragRowSize(bool enable)
{
	m_canDragRowSize = enable;
}

void wxGridExt::EnableDragColSize(bool enable)
{
	m_canDragColSize = enable;
}

void wxGridExt::EnableDragGridSize(bool enable)
{
	m_canDragGridSize = enable;
}

void wxGridExt::EnableDragCell(bool enable)
{
	m_canDragCell = enable;
}

void wxGridExt::SetDefaultRowSize(int height, bool resizeExistingRows)
{
	m_defaultRowHeight = wxMax(height, m_minAcceptableRowHeight);

	if (resizeExistingRows)
	{
		// since we are resizing all rows to the default row size,
		// we can simply clear the row heights and row bottoms
		// arrays (which also allows us to take advantage of
		// some speed optimisations)
		m_rowHeights.Empty();
		m_rowBottoms.Empty();
		CalcDimensions();
	}
}

namespace
{

	// This is a common part of SetRowSize() and SetColSize() which takes care of
	// updating the height/width of a row/column depending on its current value and
	// the new one.
	//
	// Returns the difference between the new and the old size.
	int UpdateRowOrColSize(int& sizeCurrent, int sizeNew)
	{
		// On input here sizeCurrent can be negative if it's currently hidden (the
		// real size is its absolute value then). And sizeNew can be 0 to indicate
		// that the row/column should be hidden or -1 to indicate that it should be
		// shown again.

		if (sizeNew < 0)
		{
			// We're showing back a previously hidden row/column.
			wxASSERT_MSG(sizeNew == -1, wxS("New size must be positive or -1."));

			// If it's already visible, simply do nothing.
			if (sizeCurrent >= 0)
				return 0;

			// Otherwise show it by restoring its old size.
			sizeCurrent = -sizeCurrent;

			// This is positive which is correct.
			return sizeCurrent;
		}
		else if (sizeNew == 0)
		{
			// We're hiding a row/column.

			// If it's already hidden, simply do nothing.
			if (sizeCurrent <= 0)
				return 0;

			// Otherwise hide it and also remember the shown size to be able to
			// restore it later.
			sizeCurrent = -sizeCurrent;

			// This is negative which is correct.
			return sizeCurrent;
		}
		else // We're just changing the row/column size.
		{
			// Here it could have been hidden or not previously.
			const int sizeOld = sizeCurrent < 0 ? 0 : sizeCurrent;

			sizeCurrent = sizeNew;

			return sizeCurrent - sizeOld;
		}
	}

} // anonymous namespace

void wxGridExt::SetRowSize(int row, int height)
{
	// See comment in SetColSize
	if (height > 0 && height < GetRowMinimalAcceptableHeight())
		return;

	// The value of -1 is special and means to fit the height to the row label.
	// As with the columns, ignore attempts to auto-size the hidden rows.
	if (height == -1 && GetRowHeight(row) != 0)
	{
		long w, h;
		wxArrayString lines;
		wxClientDC dc(m_rowLabelWin);
		dc.SetFont(GetLabelFont());
		ParseLines(GetRowLabelValue(row), lines);
		GetTextBoxSize(dc, lines, &w, &h);

		// As with the columns, don't make the row smaller than minimal height.
		height = wxMax(h, GetRowMinimalHeight(row));
	}

	DoSetRowSize(row, height);
}

void wxGridExt::DoSetRowSize(int row, int height)
{
	wxCHECK_RET(row >= 0 && row < m_numRows, wxT("invalid row index"));

	if (m_rowHeights.IsEmpty())
	{
		// need to really create the array
		InitRowHeights();
	}

	const int diff = UpdateRowOrColSize(m_rowHeights[row], height);
	if (!diff)
		return;


	for (int rowPos = GetRowPos(row); rowPos < m_numRows; rowPos++)
	{
		m_rowBottoms[GetRowAt(rowPos)] += diff;
	}

	InvalidateBestSize();

	CalcDimensions();

	if (ShouldRefresh())
	{
		// We need to check the size of all the currently visible cells and
		// decrease the row to cover the start of the multirow cells, if any,
		// because we need to refresh such cells entirely when resizing.
		int topRow = row;

		// Note that we don't care about the cells in frozen windows here as
		// they can't have multiple rows currently.
		const wxRect rect = m_gridWin->GetRect();
		int left, right;
		CalcUnscrolledPosition(rect.GetLeft(), 0, &left, NULL);
		CalcUnscrolledPosition(rect.GetRight(), 0, &right, NULL);

		const int posLeft = XToPos(left, m_gridWin);
		const int posRight = XToPos(right, m_gridWin);
		for (int pos = posLeft; pos <= posRight; ++pos)
		{
			int col = GetColAt(pos);

			int numRows, numCols;
			if (GetCellSize(row, col, &numRows, &numCols) == CellSpan_Inside)
			{
				// Notice that numRows here is negative.
				if (row + numRows < topRow)
					topRow = row + numRows;
			}
		}

		// Helper object to refresh part of the window below the given position
		// (in physical coordinates).
		class LowerWindowPartRefresher
		{
		public:
			explicit LowerWindowPartRefresher(int top)
				: m_top(top)
			{
			}

			void operator()(wxWindow* w) const
			{
				wxSize size = w->GetClientSize();
				if (size.y > m_top)
				{
					size.y -= m_top;
					w->RefreshRect(wxRect(wxPoint(0, m_top), size));
				}
				//else: the area to refresh is not in view anyhow
			}

		private:
			const int m_top;
		};

		int y;
		CalcScrolledPosition(0, GetRowTop(topRow), NULL, &y);

		if (topRow < m_numFrozenRows)
		{
			// This row is frozen, refresh the frozen windows.
			LowerWindowPartRefresher refreshLowerPart(y);

			refreshLowerPart(m_rowFrozenAreaWin);
			refreshLowerPart(m_rowFrozenLabelWin);
			refreshLowerPart(m_frozenRowGridWin);

			// If there are any frozen columns as well, there is one more
			// window to refresh.
			if (m_frozenCornerGridWin)
				refreshLowerPart(m_frozenCornerGridWin);
		}
		else // This row is not frozen.
		{
			// If we have any frozen rows, all the windows we're refreshing
			// here are offset by their height.
			if (m_rowFrozenLabelWin)
				y -= m_rowFrozenLabelWin->GetSize().y;

			LowerWindowPartRefresher refreshLowerPart(y);

			refreshLowerPart(m_rowAreaWin);
			refreshLowerPart(m_rowLabelWin);
			refreshLowerPart(m_gridWin);

			if (m_frozenColGridWin)
				refreshLowerPart(m_frozenColGridWin);
		}
	}

	//support printing
	SetRowBrake(row);
	SendEvent(wxEVT_GRID_CHANGED);
}

void wxGridExt::SetDefaultColSize(int width, bool resizeExistingCols)
{
	// we dont allow zero default column width
	m_defaultColWidth = wxMax(wxMax(width, m_minAcceptableColWidth), 1);

	if (resizeExistingCols)
	{
		// since we are resizing all columns to the default column size,
		// we can simply clear the col widths and col rights
		// arrays (which also allows us to take advantage of
		// some speed optimisations)
		m_colWidths.Empty();
		m_colRights.Empty();

		CalcDimensions();
	}
}

void wxGridExt::SetColSize(int col, int width)
{
	// we intentionally don't test whether the width is less than
	// GetColMinimalWidth() here but we do compare it with
	// GetColMinimalAcceptableWidth() as otherwise things currently break (see
	// #651) -- and we also always allow the width of 0 as it has the special
	// sense of hiding the column
	if (width > 0 && width < GetColMinimalAcceptableWidth())
		return;

	// The value of -1 is special and means to fit the width to the column label.
	//
	// Notice that we currently don't support auto-sizing hidden columns (we
	// could, but it's not clear whether this is really needed and it would
	// make the code more complex), and for them passing -1 simply means to
	// show the column back using its old size.
	if (width == -1 && GetColWidth(col) != 0)
	{
		if (m_useNativeHeader)
		{
			width = GetGridColHeader()->GetColumnTitleWidth(col);
		}
		else
		{
			long w, h;
			wxArrayString lines;
			wxClientDC dc(m_colLabelWin);
			dc.SetFont(GetLabelFont());
			ParseLines(GetColLabelValue(col), lines);
			if (GetColLabelTextOrientation() == wxHORIZONTAL)
				GetTextBoxSize(dc, lines, &w, &h);
			else
				GetTextBoxSize(dc, lines, &h, &w);
			width = w + 6;
		}

		// Check that it is not less than the minimal width and do use the
		// possibly greater than minimal-acceptable-width minimal-width itself
		// here as we shouldn't become too small when auto-sizing, otherwise
		// the column could be resized to be too small by double clicking its
		// divider line (which ends up in a call to this function) even though
		// it couldn't be resized to this size by dragging it.
		width = wxMax(width, GetColMinimalWidth(col));
	}

	DoSetColSize(col, width);
}

void wxGridExt::DoSetColSize(int col, int width)
{
	wxCHECK_RET(col >= 0 && col < m_numCols, wxT("invalid column index"));

	if (m_colWidths.IsEmpty())
	{
		// need to really create the array
		InitColWidths();
	}

	const int diff = UpdateRowOrColSize(m_colWidths[col], width);
	if (!diff)
		return;

	if (m_useNativeHeader)
	{
		// We have to update the native control if we're called from the
		// program (directly or indirectly, e.g. via AutoSizeColumn()), but we
		// want to avoid doing it when the column is being resized
		// interactively, as this is unnecessary and results in very visible
		// flicker, so take care to call the special method of our header
		// control checking for whether it's being resized interactively
		// instead of the usual UpdateColumn().
		static_cast<wxGridExtHeaderCtrl*>(m_colLabelWin)->UpdateIfNotResizing(col);
	}
	//else: will be refreshed when the header is redrawn

	for (int colPos = GetColPos(col); colPos < m_numCols; colPos++)
	{
		m_colRights[GetColAt(colPos)] += diff;
	}

	InvalidateBestSize();

	CalcDimensions();

	if (ShouldRefresh())
	{
		// This code is symmetric with DoSetRowSize(), see there for more
		// comments.

		int leftCol = col;

		const wxRect rect = m_gridWin->GetRect();
		int top, bottom;
		CalcUnscrolledPosition(0, rect.GetTop(), NULL, &top);
		CalcUnscrolledPosition(0, rect.GetBottom(), NULL, &bottom);

		const int posTop = YToPos(top, m_gridWin);
		const int posBottom = YToPos(bottom, m_gridWin);
		for (int pos = posTop; pos <= posBottom; ++pos)
		{
			int row = GetRowAt(pos);

			int numRows, numCols;
			if (GetCellSize(row, col, &numRows, &numCols) == CellSpan_Inside)
			{
				if (col + numCols < leftCol)
					leftCol = col + numCols;
			}
		}

		// This is supposed to be the equivalent of LowerWindowPartRefresher
		// for the rows, but there is no real counterpart to "lower" in
		// horizontal direction, so use the clumsy "further" as the least bad
		// alternative.
		class FurtherWindowPartRefresher
		{
		public:
			explicit FurtherWindowPartRefresher(int left)
				: m_left(left)
			{
			}

			void operator()(wxWindow* w) const
			{
				wxSize size = w->GetClientSize();
				if (size.x > m_left)
				{
					size.x -= m_left;
					w->RefreshRect(wxRect(wxPoint(m_left, 0), size));
				}
			}

		private:
			const int m_left;
		};

		int x;
		CalcScrolledPosition(GetColLeft(leftCol), 0, &x, NULL);

		if (leftCol < m_numFrozenCols)
		{
			FurtherWindowPartRefresher refreshFurtherPart(x);

			refreshFurtherPart(m_colFrozenLabelWin);
			refreshFurtherPart(m_frozenColGridWin);

			if (m_frozenCornerGridWin)
				refreshFurtherPart(m_frozenCornerGridWin);
		}
		else
		{
			if (m_colFrozenLabelWin)
				x -= m_colFrozenLabelWin->GetSize().x;

			FurtherWindowPartRefresher refreshFurtherPart(x);

			// Refreshing the native header is unnecessary, as it updates
			// itself correctly anyhow, and just results in extra flicker.
			refreshFurtherPart(m_colAreaWin);
			if (!IsUsingNativeHeader())
				refreshFurtherPart(m_colLabelWin);

			refreshFurtherPart(m_gridWin);

			if (m_frozenRowGridWin)
				refreshFurtherPart(m_frozenRowGridWin);
		}
	}

	//support printing
	SetColBrake(col);
	SendEvent(wxEVT_GRID_CHANGED);
}

void wxGridExt::SetColMinimalWidth(int col, int width)
{
	if (width > GetColMinimalAcceptableWidth())
	{
		wxLongToLongHashMap::key_type key = (wxLongToLongHashMap::key_type)col;
		m_colMinWidths[key] = width;
	}
}

void wxGridExt::SetRowMinimalHeight(int row, int width)
{
	if (width > GetRowMinimalAcceptableHeight())
	{
		wxLongToLongHashMap::key_type key = (wxLongToLongHashMap::key_type)row;
		m_rowMinHeights[key] = width;
	}
}

int wxGridExt::GetColMinimalWidth(int col) const
{
	wxLongToLongHashMap::key_type key = (wxLongToLongHashMap::key_type)col;
	wxLongToLongHashMap::const_iterator it = m_colMinWidths.find(key);

	return it != m_colMinWidths.end() ? (int)it->second : m_minAcceptableColWidth;
}

int wxGridExt::GetRowMinimalHeight(int row) const
{
	wxLongToLongHashMap::key_type key = (wxLongToLongHashMap::key_type)row;
	wxLongToLongHashMap::const_iterator it = m_rowMinHeights.find(key);

	return it != m_rowMinHeights.end() ? (int)it->second : m_minAcceptableRowHeight;
}

void wxGridExt::SetColMinimalAcceptableWidth(int width)
{
	// We do allow a width of 0 since this gives us
	// an easy way to temporarily hiding columns.
	if (width >= 0)
		m_minAcceptableColWidth = width;
}

void wxGridExt::SetRowMinimalAcceptableHeight(int height)
{
	// We do allow a height of 0 since this gives us
	// an easy way to temporarily hiding rows.
	if (height >= 0)
		m_minAcceptableRowHeight = height;
}

int  wxGridExt::GetColMinimalAcceptableWidth() const
{
	return m_minAcceptableColWidth;
}

int  wxGridExt::GetRowMinimalAcceptableHeight() const
{
	return m_minAcceptableRowHeight;
}

void wxGridExt::SetNativeHeaderColCount()
{
	wxASSERT_MSG(m_useNativeHeader, "no column header window");

	GetGridColHeader()->SetColumnCount(m_numCols);

	SetNativeHeaderColOrder();
}

void wxGridExt::SetNativeHeaderColOrder()
{
	wxASSERT_MSG(m_useNativeHeader, "no column header window");

	if (!m_colAt.empty())
		GetGridColHeader()->SetColumnsOrder(m_colAt);
	else
		GetGridColHeader()->ResetColumnsOrder();
}

// ----------------------------------------------------------------------------
// auto sizing
// ----------------------------------------------------------------------------

void
wxGridExt::AutoSizeColOrRow(int colOrRow, bool setAsMin, wxGridExtDirection direction)
{
	const bool column = direction == wxGRID_COLUMN;

	// We don't support auto-sizing hidden rows or columns, this doesn't seem
	// to make much sense.
	if (column)
	{
		if (GetColWidth(colOrRow) == 0)
			return;
	}
	else
	{
		if (GetRowHeight(colOrRow) == 0)
			return;
	}

	wxClientDC dc(m_gridWin);

	AcceptCellEditControlIfShown();

	// initialize both of them just to avoid compiler warnings even if only
	// really needs to be initialized here
	int row,
		col;
	if (column)
	{
		row = -1;
		col = colOrRow;
	}
	else
	{
		row = colOrRow;
		col = -1;
	}

	// If possible, reuse the same attribute and renderer for all cells: this
	// is an important optimization (resulting in up to 80% speed up of
	// AutoSizeColumns()) as finding the attribute and renderer for the cell
	// are very slow operations, due to the number of steps involved in them.
	const bool canReuseAttr = column && m_table->CanMeasureColUsingSameAttr(col);
	wxGridExtCellAttrPtr attr;
	wxGridExtCellRendererPtr renderer;

	wxCoord extent, extentMax = 0;
	int max = column ? m_numRows : m_numCols;
	for (int rowOrCol = 0; rowOrCol < max; rowOrCol++)
	{
		if (column)
		{
			if (!IsRowShown(rowOrCol))
				continue;

			row = rowOrCol;
			col = colOrRow;
		}
		else
		{
			if (!IsColShown(rowOrCol))
				continue;

			row = colOrRow;
			col = rowOrCol;
		}

		// we need to account for the cells spanning multiple columns/rows:
		// while they may need a lot of space, they don't need all of it in
		// this column/row
		int numRows, numCols;
		const CellSpan span = GetCellSize(row, col, &numRows, &numCols);
		if (span == CellSpan_Inside)
		{
			// we need to get the size of the main cell, not of a cell hidden
			// by it
			row += numRows;
			col += numCols;

			// get the size of the main cell too
			GetCellSize(row, col, &numRows, &numCols);
		}

		// get cell ( main cell if CellSpan_Inside ) renderer best size
		if (!canReuseAttr || !attr)
		{
			attr = GetCellAttrPtr(row, col);
			renderer = attr->GetRendererPtr(this, row, col);

			if (canReuseAttr)
			{
				// Try to get the best width for the entire column at once, if
				// it's supported by the renderer.
				extent = renderer->GetMaxBestSize(*this, *attr, dc).x;

				if (extent != wxDefaultCoord)
				{
					extentMax = extent;

					// No need to check all the values.
					break;
				}
			}
		}

		if (renderer)
		{
			extent = column
				? renderer->GetBestWidth(*this, *attr, dc, row, col,
					GetRowHeight(row))
				: renderer->GetBestHeight(*this, *attr, dc, row, col,
					GetColWidth(col));

			if (span != CellSpan_None)
			{
				// we spread the size of a spanning cell over all the cells it
				// covers evenly -- this is probably not ideal but we can't
				// really do much better here
				//
				// notice that numCols and numRows are never 0 as they
				// correspond to the size of the main cell of the span and not
				// of the cell inside it
				extent /= column ? numCols : numRows;
			}

			if (extent > extentMax)
				extentMax = extent;
		}
	}

	// now also compare with the column label extent
	wxCoord extentLabel;
	dc.SetFont(GetLabelFont());

	// We add some margin around text for better readability.
	const int margin = FromDIP(column ? 10 : 6);

	if (column)
	{
		if (m_useNativeHeader)
		{
			extentLabel = GetGridColHeader()->GetColumnTitleWidth(colOrRow);

			// Note that GetColumnTitleWidth already adds margins internally,
			// so we don't need to add them here.
		}
		else
		{
			const wxSize
				size = dc.GetMultiLineTextExtent(GetColLabelValue(colOrRow));
			extentLabel = GetColLabelTextOrientation() == wxVERTICAL
				? size.y
				: size.x;

			// Add some margins around text for better readability.
			extentLabel += margin;
		}
	}
	else
	{
		extentLabel = dc.GetMultiLineTextExtent(GetRowLabelValue(colOrRow)).y;

		// As above, add some margins for readability, although a smaller one
		// in vertical direction.
		extentLabel += margin;
	}


	// Finally determine the suitable extent fitting both the cells contents
	// and the label.
	if (!extentMax)
	{
		// Special case: all the cells are empty, use the label extent.
		extentMax = extentLabel;
		if (!extentMax)
		{
			// But if the label is empty too, use the default width/height.
			extentMax = column ? m_defaultColWidth : m_defaultRowHeight;
		}
	}
	else // We have some data in the column cells.
	{
		// Ensure we have the same margin around the cells text as we use
		// around the label.
		extentMax += margin;

		// And increase it to fit the label if necessary.
		if (extentLabel > extentMax)
			extentMax = extentLabel;
	}


	if (column)
	{
		// Ensure automatic width is not less than minimal width. See the
		// comment in SetColSize() for explanation of why this isn't done
		// in SetColSize().
		if (!setAsMin)
			extentMax = wxMax(extentMax, GetColMinimalWidth(colOrRow));

		SetColSize(colOrRow, extentMax);
		if (ShouldRefresh())
		{
			if (m_useNativeHeader)
			{
				GetGridColHeader()->UpdateColumn(colOrRow);
			}
			else
			{
				int cw, ch, dummy;
				m_gridWin->GetClientSize(&cw, &ch);
				wxRect rect(CellToRect(0, colOrRow));
				rect.y = 0;
				CalcScrolledPosition(rect.x, 0, &rect.x, &dummy);
				rect.width = cw - rect.x;
				rect.height = (GridColAreaEnabled() ? m_colAreaHeight : 0) + m_colLabelHeight;
				GetColLabelWindow()->Refresh(true, &rect);
			}
		}
	}
	else
	{
		// Ensure automatic width is not less than minimal height. See the
		// comment in SetColSize() for explanation of why this isn't done
		// in SetRowSize().
		if (!setAsMin)
			extentMax = wxMax(extentMax, GetRowMinimalHeight(colOrRow));

		SetRowSize(colOrRow, extentMax);
		if (ShouldRefresh())
		{
			int cw, ch, dummy;
			m_gridWin->GetClientSize(&cw, &ch);
			wxRect rect(CellToRect(colOrRow, 0));
			rect.x = 0;
			CalcScrolledPosition(0, rect.y, &dummy, &rect.y);
			rect.width = m_rowLabelWidth;
			rect.height = ch - rect.y;

			m_rowAreaWin->Refresh(true, &rect);
			m_rowLabelWin->Refresh(true, &rect);
		}
	}

	if (setAsMin)
	{
		if (column)
			SetColMinimalWidth(colOrRow, extentMax);
		else
			SetRowMinimalHeight(colOrRow, extentMax);
	}
}

wxCoord wxGridExt::CalcColOrRowLabelAreaMinSize(wxGridExtDirection direction)
{
	// calculate size for the rows or columns?
	const bool calcRows = direction == wxGRID_ROW;

	wxClientDC dc(calcRows ? GetGridRowLabelWindow()
		: GetGridColLabelWindow());
	dc.SetFont(GetLabelFont());

	// which dimension should we take into account for calculations?
	//
	// for columns, the text can be only horizontal so it's easy but for rows
	// we also have to take into account the text orientation
	const bool
		useWidth = calcRows || (GetColLabelTextOrientation() == wxVERTICAL);

	wxArrayString lines;
	wxCoord extentMax = 0;

	const int numRowsOrCols = calcRows ? m_numRows : m_numCols;
	for (int rowOrCol = 0; rowOrCol < numRowsOrCols; rowOrCol++)
	{
		lines.Clear();

		wxString label = calcRows ? GetRowLabelValue(rowOrCol)
			: GetColLabelValue(rowOrCol);
		ParseLines(label, lines);

		long w, h;
		GetTextBoxSize(dc, lines, &w, &h);

		const wxCoord extent = useWidth ? w : h;
		if (extent > extentMax)
			extentMax = extent;
	}

	if (!extentMax)
	{
		// empty column - give default extent (notice that if extentMax is less
		// than default extent but != 0, it's OK)
		extentMax = calcRows ? GetDefaultRowLabelSize()
			: GetDefaultColLabelSize();
	}

	// leave some space around text (taken from AutoSizeColOrRow)
	if (calcRows)
		extentMax += 10;
	else
		extentMax += 6;

	return extentMax;
}

void wxGridExt::AutoSizeColumns(bool setAsMin)
{
	wxGridExtUpdateLocker locker(this);

	for (int col = 0; col < m_numCols; col++)
		AutoSizeColumn(col, setAsMin);
}

void wxGridExt::AutoSizeRows(bool setAsMin)
{
	wxGridExtUpdateLocker locker(this);

	for (int row = 0; row < m_numRows; row++)
		AutoSizeRow(row, setAsMin);
}

void wxGridExt::AutoSize()
{
	wxGridExtUpdateLocker locker(this);

	AutoSizeColumns();
	AutoSizeRows();

	// we know that we're not going to have scrollbars so disable them now to
	// avoid trouble in SetClientSize() which can otherwise set the correct
	// client size but also leave space for (not needed any more) scrollbars
	SetScrollbars(m_xScrollPixelsPerLine, m_yScrollPixelsPerLine,
		0, 0, 0, 0, true);

	SetSize(DoGetBestSize());
}

void wxGridExt::AutoSizeRowLabelSize(int row)
{
	// Hide the edit control, so it
	// won't interfere with drag-shrinking.
	AcceptCellEditControlIfShown();

	// autosize row height depending on label text
	SetRowSize(row, -1);

	ForceRefresh();
}

void wxGridExt::AutoSizeColLabelSize(int col)
{
	// Hide the edit control, so it
	// won't interfere with drag-shrinking.
	AcceptCellEditControlIfShown();

	// autosize column width depending on label text
	SetColSize(col, -1);

	ForceRefresh();
}

wxSize wxGridExt::DoGetBestSize() const
{
	wxSize size((GridRowAreaEnabled() ? m_rowAreaWidth : 0) + m_rowLabelWidth + m_extraWidth,
		(GridColAreaEnabled() ? m_colAreaHeight : 0) + m_colLabelHeight + m_extraHeight);

	if (m_colWidths.empty())
	{
		size.x += m_defaultColWidth * m_numCols;
	}
	else
	{
		for (int col = 0; col < m_numCols; col++)
			size.x += GetColWidth(col);
	}

	if (m_rowHeights.empty())
	{
		size.y += m_defaultRowHeight * m_numRows;
	}
	else
	{
		for (int row = 0; row < m_numRows; row++)
			size.y += GetRowHeight(row);
	}

	return size + GetWindowBorderSize();
}

void wxGridExt::Fit()
{
	AutoSize();
}

void wxGridExt::SetFocus()
{
	m_gridWin->SetFocus();
}

#if WXWIN_COMPATIBILITY_2_8
wxPen& wxGridExt::GetDividerPen() const
{
	return wxNullPen;
}
#endif // WXWIN_COMPATIBILITY_2_8

// ----------------------------------------------------------------------------
// cell value accessor functions
// ----------------------------------------------------------------------------

void wxGridExt::SetCellValue(int row, int col, const wxString& s)
{
	if (s == GetCellValue(row, col))
	{
		// Avoid flicker by not doing anything in this case.
		return;
	}

	if (m_table)
	{
		// set new brake pos
		SetRowBrake(row);
		SetColBrake(col);

		m_table->SetValue(row, col, s);
		if (ShouldRefresh())
		{
			int dummy;
			wxRect rect(CellToRect(row, col));
			rect.x = 0;
			rect.width = m_gridWin->GetClientSize().GetWidth();
			CalcScrolledPosition(0, rect.y, &dummy, &rect.y);
			m_gridWin->Refresh(false, &rect);
		}

		if (m_currentCellCoords.GetRow() == row &&
			m_currentCellCoords.GetCol() == col &&
			IsCellEditControlShown())
			// Note: If we are using IsCellEditControlEnabled,
			// this interacts badly with calling SetCellValue from
			// an EVT_GRID_CELL_CHANGE handler.
		{
			HideCellEditControl();
			ShowCellEditControl(); // will reread data from table
		}
	}

	SendEvent(wxEVT_GRID_CHANGED);
}

// ----------------------------------------------------------------------------
// block, row and column selection
// ----------------------------------------------------------------------------

void wxGridExt::SelectRow(int row, bool addToSelected)
{
	if (!m_selection)
		return;

	if (!addToSelected)
		ClearSelection();

	m_selection->SelectRow(row);
}

void wxGridExt::SelectCol(int col, bool addToSelected)
{
	if (!m_selection)
		return;

	if (!addToSelected)
		ClearSelection();

	m_selection->SelectCol(col);
}

void wxGridExt::SelectBlock(int topRow, int leftCol, int bottomRow, int rightCol,
	bool addToSelected)
{
	if (!m_selection)
		return;

	if (!addToSelected)
		ClearSelection();

	m_selection->SelectBlock(topRow, leftCol, bottomRow, rightCol);
}

void wxGridExt::SelectAll()
{
	if (m_selection)
		m_selection->SelectAll();
}

// ----------------------------------------------------------------------------
// cell, row and col deselection
// ----------------------------------------------------------------------------

void wxGridExt::DeselectRow(int row)
{
	wxCHECK_RET(row >= 0 && row < m_numRows, wxT("invalid row index"));

	if (m_selection)
		m_selection->DeselectBlock(wxGridExtBlockCoords(row, 0, row, m_numCols - 1));
}

void wxGridExt::DeselectCol(int col)
{
	wxCHECK_RET(col >= 0 && col < m_numCols, wxT("invalid column index"));

	if (m_selection)
		m_selection->DeselectBlock(wxGridExtBlockCoords(0, col, m_numRows - 1, col));
}

void wxGridExt::DeselectCell(int row, int col)
{
	wxCHECK_RET(row >= 0 && row < m_numRows &&
		col >= 0 && col < m_numCols,
		wxT("invalid cell coords"));

	if (m_selection)
		m_selection->DeselectBlock(wxGridExtBlockCoords(row, col, row, col));
}

bool wxGridExt::IsSelection() const
{
	return m_selection && m_selection->IsSelection();
}

bool wxGridExt::IsInSelection(int row, int col) const
{
	return m_selection && m_selection->IsInSelection(row, col);
}

wxGridExtBlocks wxGridExt::GetSelectedBlocks() const
{
	if (!m_selection)
		return wxGridExtBlocks();

	const wxVectorGridBlockCoords& blocks = m_selection->GetBlocks();
	return wxGridExtBlocks(blocks.begin(), blocks.end());
}

static
wxGridExtBlockCoordsVector
DoGetRowOrColBlocks(wxGridExtBlocks blocks, const wxGridExtOperations& oper)
{
	wxGridExtBlockCoordsVector res;

	for (wxGridExtBlocks::iterator it = blocks.begin(); it != blocks.end(); ++it)
	{
		const int firstNew = oper.SelectFirst(*it);
		const int lastNew = oper.SelectLast(*it);

		// Check if this block intersects any of the existing ones.
		//
		// We use simple linear search because we assume there are only a few
		// blocks in all, and it's not worth complicating the code to use
		// anything more advanced, but this definitely could be improved to use
		// the fact that the vector is always sorted.
		for (size_t n = 0;; )
		{
			if (n == res.size())
			{
				// We didn't find any overlapping blocks, so add this one to
				// the end.
				res.push_back(*it);
				break;
			}

			wxGridExtBlockCoords& block = res[n];
			const int firstThis = oper.SelectFirst(block);
			const int lastThis = oper.SelectLast(block);

			if (lastNew < firstThis)
			{
				// Not only it doesn't overlap this block, but it won't overlap
				// any subsequent ones either, so insert it here and stop.
				res.insert(res.begin() + n, *it);
				break;
			}

			if (lastThis < firstNew)
			{
				// It doesn't overlap this one, but continue checking.
				n++;
				continue;
			}

			// The blocks overlap, combine them by adjusting the bounds of the
			// current block.

			// The first bound is simple as we know that firstNew must be
			// strictly greater than the last coordinate of all the previous
			// elements, otherwise we would have combined it with them earlier.
			if (firstNew < firstThis)
				oper.SetFirst(block, firstNew);

			// But for the last one, we need to find the last element it
			// overlaps (which may be this block itself). We call its index n2
			// to avoid confusion with "last" used for the block component.
			size_t n2 = n;
			for (;; )
			{
				const wxGridExtBlockCoords& block2 = res[n2];
				if (lastNew < oper.SelectFirst(block2))
				{
					oper.SetLast(block, lastNew);
					break;
				}

				// Do it here as we'll need to remove the current block if it's
				// the last overlapping one and we break just below.
				n2++;

				if (lastNew < oper.SelectLast(block2))
				{
					oper.SetLast(block, oper.SelectLast(block2));
					break;
				}

				if (n2 == res.size())
				{
					oper.SetLast(block, lastNew);
					break;
				}
			}

			if (n2 > n + 1)
				res.erase(res.begin() + n + 1, res.begin() + n2);

			break;
		}
	}

	// This is another inefficiency: it would be also possible to do everything
	// in one pass, combining the adjacent ranges in the loop above. But this
	// is more complicated and doesn't seem to be worth it, for the arrays of
	// small sizes that we work with here, so do an extra path combining the
	// adjacent ranges.
	for (size_t n = 0;; )
	{
		if (n + 1 >= res.size())
			break;

		if (oper.SelectFirst(res[n + 1]) == oper.SelectLast(res[n]) + 1)
		{
			// The ranges touch, combine them.
			oper.SetLast(res[n], oper.SelectLast(res[n + 1]));

			// And erase the subsumed range.
			res.erase(res.begin() + n + 1, res.begin() + n + 2);
		}
		else // Just go to the next one.
		{
			n++;
		}
	}

	return res;
}

wxGridExtBlockCoordsVector wxGridExt::GetSelectedRowBlocks() const
{
	if (!m_selection || m_selection->GetSelectionMode() != wxGridExtSelectRows)
		return wxGridExtBlockCoordsVector();

	return DoGetRowOrColBlocks(GetSelectedBlocks(), wxGridExtRowOperations());
}

wxGridExtBlockCoordsVector wxGridExt::GetSelectedColBlocks() const
{
	if (!m_selection || m_selection->GetSelectionMode() != wxGridExtSelectColumns)
		return wxGridExtBlockCoordsVector();

	return DoGetRowOrColBlocks(GetSelectedBlocks(), wxGridExtColumnOperations());
}

wxGridExtBlockCoords wxGridExt::GetSelectedCellRange() const
{
	int row1 = m_numRows, col1 = m_numCols,
		row2 = 0, col2 = 0; bool hasBlocks = false;

	for (auto& coords : wxGridExt::GetSelectedBlocks()) {
		if (row1 > coords.GetTopRow()) row1 = coords.GetTopRow();
		if (col1 > coords.GetLeftCol()) col1 = coords.GetLeftCol();
		if (row2 < coords.GetBottomRow()) row2 = coords.GetBottomRow();
		if (col2 < coords.GetRightCol()) col2 = coords.GetRightCol();
		hasBlocks = true;
	}

	if (!hasBlocks) {
		if (row1 > wxGridExt::GetGridCursorRow()) row1 = wxGridExt::GetGridCursorRow();
		if (col1 > wxGridExt::GetGridCursorCol()) col1 = wxGridExt::GetGridCursorCol();
		if (row2 < wxGridExt::GetGridCursorRow()) row2 = wxGridExt::GetGridCursorRow();
		if (col2 < wxGridExt::GetGridCursorCol()) col2 = wxGridExt::GetGridCursorCol();
	}

	return wxGridExtBlockCoords(row1, col1, row2, col2);
}

wxGridExtCellCoordsArray wxGridExt::GetSelectedCells() const
{
	if (!m_selection)
	{
		wxGridExtCellCoordsArray a;
		return a;
	}

	return m_selection->GetCellSelection();
}

wxGridExtCellCoordsArray wxGridExt::GetSelectionBlockTopLeft() const
{
	if (!m_selection)
	{
		wxGridExtCellCoordsArray a;
		return a;
	}

	return m_selection->GetBlockSelectionTopLeft();
}

wxGridExtCellCoordsArray wxGridExt::GetSelectionBlockBottomRight() const
{
	if (!m_selection)
	{
		wxGridExtCellCoordsArray a;
		return a;
	}

	return m_selection->GetBlockSelectionBottomRight();
}

wxArrayInt wxGridExt::GetSelectedRows() const
{
	if (!m_selection)
	{
		wxArrayInt a;
		return a;
	}

	return m_selection->GetRowSelection();
}

wxArrayInt wxGridExt::GetSelectedCols() const
{
	if (!m_selection)
	{
		wxArrayInt a;
		return a;
	}

	return m_selection->GetColSelection();
}

void wxGridExt::ClearSelection()
{
	if (m_selection)
		m_selection->ClearSelection();
}

// This function returns the rectangle that encloses the given block
// in device coords clipped to the client size of the grid window.
//
wxRect wxGridExt::BlockToDeviceRect(const wxGridExtCellCoords& topLeft,
	const wxGridExtCellCoords& bottomRight,
	const wxGridExtWindow* gridWindow) const
{
	wxRect resultRect;
	wxRect tempCellRect = CellToRect(topLeft);
	if (tempCellRect != wxGridExtNoCellRect)
	{
		resultRect = tempCellRect;
	}
	else
	{
		resultRect = wxRect(0, 0, 0, 0);
	}

	tempCellRect = CellToRect(bottomRight);
	if (tempCellRect != wxGridExtNoCellRect)
	{
		resultRect += tempCellRect;
	}
	else
	{
		// If both inputs were "wxGridExtNoCellRect," then there's nothing to do.
		return wxGridExtNoCellRect;
	}

	// Ensure that left/right and top/bottom pairs are in order.
	int left = resultRect.GetLeft();
	int top = resultRect.GetTop();
	int right = resultRect.GetRight();
	int bottom = resultRect.GetBottom();

	int leftCol = topLeft.GetCol();
	int topRow = topLeft.GetRow();
	int rightCol = bottomRight.GetCol();
	int bottomRow = bottomRight.GetRow();

	if (left > right)
	{
		int tmp = left;
		left = right;
		right = tmp;

		tmp = leftCol;
		leftCol = rightCol;
		rightCol = tmp;
	}

	if (top > bottom)
	{
		int tmp = top;
		top = bottom;
		bottom = tmp;

		tmp = topRow;
		topRow = bottomRow;
		bottomRow = tmp;
	}

	if (!gridWindow)
		gridWindow = m_gridWin;

	int cw, ch;
	gridWindow->GetClientSize(&cw, &ch);
	wxPoint offset = GetGridWindowOffset(gridWindow);

	// The following loop is ONLY necessary to detect and handle merged cells.
	if (gridWindow == m_gridWin)
	{
		// Get the origin coordinates: notice that they will be negative if the
		// grid is scrolled downwards/to the right.
		int gridOriginX = offset.x;
		int gridOriginY = offset.y;
		CalcScrolledPosition(gridOriginX, gridOriginY, &gridOriginX, &gridOriginY);

		int onScreenLeftmostCol = internalXToCol(-gridOriginX, m_gridWin);
		int onScreenUppermostRow = internalYToRow(-gridOriginY, m_gridWin);

		int onScreenRightmostCol = internalXToCol(-gridOriginX + cw, m_gridWin);
		int onScreenBottommostRow = internalYToRow(-gridOriginY + ch, m_gridWin);

		// Bound our loop so that we only examine the portion of the selected block
		// that is shown on screen. Therefore, we compare the Top-Left block values
		// to the Top-Left screen values, and the Bottom-Right block values to the
		// Bottom-Right screen values, choosing appropriately.
		const int visibleTopRow = wxMax(topRow, onScreenUppermostRow);
		const int visibleBottomRow = wxMin(bottomRow, onScreenBottommostRow);
		const int visibleLeftCol = wxMax(leftCol, onScreenLeftmostCol);
		const int visibleRightCol = wxMin(rightCol, onScreenRightmostCol);

		for (int j = visibleTopRow; j <= visibleBottomRow; j++)
		{
			for (int i = visibleLeftCol; i <= visibleRightCol; i++)
			{
				if ((j == visibleTopRow) || (j == visibleBottomRow) ||
					(i == visibleLeftCol) || (i == visibleRightCol))
				{
					tempCellRect = CellToRect(j, i);

					if (tempCellRect.x < left)
						left = tempCellRect.x;
					if (tempCellRect.y < top)
						top = tempCellRect.y;
					if (tempCellRect.x + tempCellRect.width > right)
						right = tempCellRect.x + tempCellRect.width;
					if (tempCellRect.y + tempCellRect.height > bottom)
						bottom = tempCellRect.y + tempCellRect.height;
				}
				else
				{
					i = visibleRightCol; // jump over inner cells.
				}
			}
		}
	}

	// Convert to scrolled coords
	CalcGridWindowScrolledPosition(left - offset.x, top - offset.y, &left, &top, gridWindow);
	CalcGridWindowScrolledPosition(right - offset.x, bottom - offset.y, &right, &bottom, gridWindow);

	if (right < 0 || bottom < 0 || left > cw || top > ch)
		return wxRect(0, 0, 0, 0);

	resultRect.SetLeft(wxMax(0, left));
	resultRect.SetTop(wxMax(0, top));
	resultRect.SetRight(wxMin(cw, right));
	resultRect.SetBottom(wxMin(ch, bottom));

	return resultRect;
}

void wxGridExt::DoSetSizes(const wxGridExtSizesInfo& sizeInfo,
	const wxGridExtOperations& oper)
{
	BeginBatch();
	oper.SetDefaultLineSize(this, sizeInfo.m_sizeDefault, true);
	const int numLines = oper.GetNumberOfLines(this, NULL);
	for (int i = 0; i < numLines; i++)
	{
		int size = sizeInfo.GetSize(i);
		if (size != sizeInfo.m_sizeDefault)
			oper.SetLineSize(this, i, size);
	}
	EndBatch();
}

void wxGridExt::SetColSizes(const wxGridExtSizesInfo& sizeInfo)
{
	DoSetSizes(sizeInfo, wxGridExtColumnOperations());
}

void wxGridExt::SetRowSizes(const wxGridExtSizesInfo& sizeInfo)
{
	DoSetSizes(sizeInfo, wxGridExtRowOperations());
}

wxGridExtSizesInfo::wxGridExtSizesInfo(int defSize, const wxArrayInt& allSizes)
{
	m_sizeDefault = defSize;
	for (size_t i = 0; i < allSizes.size(); i++)
	{
		if (allSizes[i] != defSize)
			m_customSizes[i] = allSizes[i];
	}
}

int wxGridExtSizesInfo::GetSize(unsigned pos) const
{
	wxUnsignedToIntHashMap::const_iterator it = m_customSizes.find(pos);

	// if it's not found return the default
	if (it == m_customSizes.end())
		return m_sizeDefault;

	// otherwise return 0 if it's hidden, currently there is no way to get
	// its size before it had been hidden
	if (it->second < 0)
		return 0;

	return it->second;
}

// ----------------------------------------------------------------------------
// drop target
// ----------------------------------------------------------------------------

#if wxUSE_DRAG_AND_DROP

// this allow setting drop target directly on wxGridExt
void wxGridExt::SetDropTarget(wxDropTarget* dropTarget)
{
	GetGridWindow()->SetDropTarget(dropTarget);
}

#endif // wxUSE_DRAG_AND_DROP

// ----------------------------------------------------------------------------
// grid event classes
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(wxGridExtEvent, wxNotifyEvent);

wxGridExtEvent::wxGridExtEvent(int id, wxEventType type, wxObject* obj,
	int row, int col, int x, int y, bool sel,
	bool control, bool shift, bool alt, bool meta)
	: wxNotifyEvent(type, id),
	wxKeyboardState(control, shift, alt, meta)
{
	Init(row, col, x, y, sel);

	SetEventObject(obj);
}

wxIMPLEMENT_DYNAMIC_CLASS(wxGridExtSizeEvent, wxNotifyEvent);

wxGridExtSizeEvent::wxGridExtSizeEvent(int id, wxEventType type, wxObject* obj,
	int rowOrCol, int x, int y,
	bool control, bool shift, bool alt, bool meta)
	: wxNotifyEvent(type, id),
	wxKeyboardState(control, shift, alt, meta)
{
	Init(rowOrCol, x, y);

	SetEventObject(obj);
}


wxIMPLEMENT_DYNAMIC_CLASS(wxGridExtRangeSelectEvent, wxNotifyEvent);

wxGridExtRangeSelectEvent::wxGridExtRangeSelectEvent(int id, wxEventType type, wxObject* obj,
	const wxGridExtCellCoords& topLeft,
	const wxGridExtCellCoords& bottomRight,
	bool sel, bool control,
	bool shift, bool alt, bool meta)
	: wxNotifyEvent(type, id),
	wxKeyboardState(control, shift, alt, meta)
{
	Init(topLeft, bottomRight, sel);

	SetEventObject(obj);
}


wxIMPLEMENT_DYNAMIC_CLASS(wxGridExtEditorCreatedEvent, wxCommandEvent);

wxGridExtEditorCreatedEvent::wxGridExtEditorCreatedEvent(int id, wxEventType type,
	wxObject* obj, int row,
	int col, wxWindow* window)
	: wxCommandEvent(type, id)
{
	SetEventObject(obj);
	m_row = row;
	m_col = col;
	m_window = window;
}


// ----------------------------------------------------------------------------
// wxGridExtTypeRegistry
// ----------------------------------------------------------------------------

wxGridExtTypeRegistry::~wxGridExtTypeRegistry()
{
	size_t count = m_typeinfo.GetCount();
	for (size_t i = 0; i < count; i++)
		delete m_typeinfo[i];
}

void wxGridExtTypeRegistry::RegisterDataType(const wxString& typeName,
	wxGridExtCellRenderer* renderer,
	wxGridExtCellEditor* editor)
{
	wxGridExtDataTypeInfo* info = new wxGridExtDataTypeInfo(typeName, renderer, editor);

	// is it already registered?
	int loc = FindRegisteredDataType(typeName);
	if (loc != wxNOT_FOUND)
	{
		delete m_typeinfo[loc];
		m_typeinfo[loc] = info;
	}
	else
	{
		m_typeinfo.Add(info);
	}
}

int wxGridExtTypeRegistry::FindRegisteredDataType(const wxString& typeName)
{
	size_t count = m_typeinfo.GetCount();
	for (size_t i = 0; i < count; i++)
	{
		if (typeName == m_typeinfo[i]->m_typeName)
		{
			return i;
		}
	}

	return wxNOT_FOUND;
}

int wxGridExtTypeRegistry::FindDataType(const wxString& typeName)
{
	int index = FindRegisteredDataType(typeName);
	if (index == wxNOT_FOUND)
	{
		// check whether this is one of the standard ones, in which case
		// register it "on the fly"
#if wxUSE_TEXTCTRL
		if (typeName == wxGRID_VALUE_STRING)
		{
			RegisterDataType(wxGRID_VALUE_STRING,
				new wxGridExtCellStringRenderer,
				new wxGridExtCellTextEditor);
		}
		else
#endif // wxUSE_TEXTCTRL
#if wxUSE_CHECKBOX
			if (typeName == wxGRID_VALUE_BOOL)
			{
				RegisterDataType(wxGRID_VALUE_BOOL,
					new wxGridExtCellBoolRenderer,
					new wxGridExtCellBoolEditor);
			}
			else
#endif // wxUSE_CHECKBOX
#if wxUSE_TEXTCTRL
				if (typeName == wxGRID_VALUE_NUMBER)
				{
					RegisterDataType(wxGRID_VALUE_NUMBER,
						new wxGridExtCellNumberRenderer,
						new wxGridExtCellNumberEditor);
				}
				else if (typeName == wxGRID_VALUE_FLOAT)
				{
					RegisterDataType(wxGRID_VALUE_FLOAT,
						new wxGridExtCellFloatRenderer,
						new wxGridExtCellFloatEditor);
				}
				else
#endif // wxUSE_TEXTCTRL
#if wxUSE_COMBOBOX
					if (typeName == wxGRID_VALUE_CHOICE)
					{
						RegisterDataType(wxGRID_VALUE_CHOICE,
							new wxGridExtCellChoiceRenderer,
							new wxGridExtCellChoiceEditor);
					}
					else
#endif // wxUSE_COMBOBOX
#if wxUSE_DATEPICKCTRL
						if (typeName == wxGRID_VALUE_DATE)
						{
							RegisterDataType(wxGRID_VALUE_DATE,
								new wxGridExtCellDateRenderer,
								new wxGridExtCellDateEditor);
						}
						else
#endif // wxUSE_DATEPICKCTRL
						{
							return wxNOT_FOUND;
						}

		// we get here only if just added the entry for this type, so return
		// the last index
		index = m_typeinfo.GetCount() - 1;
	}

	return index;
}

int wxGridExtTypeRegistry::FindOrCloneDataType(const wxString& typeName)
{
	int index = FindDataType(typeName);
	if (index == wxNOT_FOUND)
	{
		// the first part of the typename is the "real" type, anything after ':'
		// are the parameters for the renderer
		index = FindDataType(typeName.BeforeFirst(wxT(':')));
		if (index == wxNOT_FOUND)
		{
			return wxNOT_FOUND;
		}

		wxGridExtCellRenderer* const
			renderer = wxGridExtCellRendererPtr(GetRenderer(index))->Clone();

		wxGridExtCellEditor* const
			editor = wxGridExtCellEditorPtr(GetEditor(index))->Clone();

		// do it even if there are no parameters to reset them to defaults
		wxString params = typeName.AfterFirst(wxT(':'));
		renderer->SetParameters(params);
		editor->SetParameters(params);

		// register the new typename
		RegisterDataType(typeName, renderer, editor);

		// we just registered it, it's the last one
		index = m_typeinfo.GetCount() - 1;
	}

	return index;
}

wxGridExtCellRenderer* wxGridExtTypeRegistry::GetRenderer(int index)
{
	wxGridExtCellRenderer* renderer = m_typeinfo[index]->m_renderer;
	if (renderer)
		renderer->IncRef();

	return renderer;
}

wxGridExtCellEditor* wxGridExtTypeRegistry::GetEditor(int index)
{
	wxGridExtCellEditor* editor = m_typeinfo[index]->m_editor;
	if (editor)
		editor->IncRef();

	return editor;
}

wxRect
wxGetContentRect(wxSize contentSize,
	const wxRect& cellRect,
	int hAlign,
	int vAlign)
{
	// Keep square aspect ratio for the checkbox, but ensure that it fits into
	// the available space, even if it's smaller than the standard size.
	const wxCoord minSize = wxMin(cellRect.width, cellRect.height);
	if (contentSize.x >= minSize || contentSize.y >= minSize)
	{
		// It must still have positive size, however.
		const int fittingSize = wxMax(1, minSize - 2 * GRID_CELL_CHECKBOX_MARGIN);

		contentSize.x =
			contentSize.y = fittingSize;
	}

	wxRect contentRect(contentSize);

	if (hAlign & wxALIGN_CENTER_HORIZONTAL)
	{
		contentRect = contentRect.CentreIn(cellRect, wxHORIZONTAL);
	}
	else if (hAlign & wxALIGN_RIGHT)
	{
		contentRect.SetX(cellRect.x + cellRect.width
			- contentSize.x - GRID_CELL_CHECKBOX_MARGIN);
	}
	else // ( hAlign == wxALIGN_LEFT ) and invalid alignment value
	{
		contentRect.SetX(cellRect.x + GRID_CELL_CHECKBOX_MARGIN);
	}

	if (vAlign & wxALIGN_CENTER_VERTICAL)
	{
		contentRect = contentRect.CentreIn(cellRect, wxVERTICAL);
	}
	else if (vAlign & wxALIGN_BOTTOM)
	{
		contentRect.SetY(cellRect.y + cellRect.height
			- contentSize.y - GRID_CELL_CHECKBOX_MARGIN);
	}
	else // wxALIGN_TOP
	{
		contentRect.SetY(cellRect.y + GRID_CELL_CHECKBOX_MARGIN);
	}

	return contentRect;
}

#endif // wxUSE_GRID
