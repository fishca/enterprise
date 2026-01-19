////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxwidgets community
//	Description : grid printer 
////////////////////////////////////////////////////////////////////////////

#include "gridPrintout.h"

CGridEditorPrintout::CGridEditorPrintout(const wxString& title) : wxPrintout(title)
{
	m_view = nullptr;
	SetStyle(wxGP_SHOW_NONE);

	m_minPage = 1;
	m_maxPage = 1;
	m_selPageFrom = 1;
	m_selPageTo = 1;
	m_screenScale = 1.0;
	m_userScale = 1.0;
	m_topMargin = 50;
	m_bottomMargin = 50;
	m_leftMargin = 50;
	m_rightMargin = 50;
}

CGridEditorPrintout::CGridEditorPrintout(CGridEditor* gridCtrl, int style, const wxString& title) : wxPrintout(title)
{
	m_view = gridCtrl;
	SetStyle(style);

	m_minPage = 1;
	m_maxPage = 1;
	m_selPageFrom = 1;
	m_selPageTo = 1;
	m_screenScale = 1.0;
	m_userScale = 1.0;
	m_topMargin = 50;
	m_bottomMargin = 50;
	m_leftMargin = 50;
	m_rightMargin = 50;
}

void CGridEditorPrintout::SetGrid(CGridEditor* gridCtrl)
{
	m_view = gridCtrl;
}

CGridEditor* CGridEditorPrintout::GetGrid() const
{
	return m_view;
}

void CGridEditorPrintout::SetStyle(int style)
{
	m_style = style;

	m_showCl = false;
	m_showRl = false;
	m_showClAlways = false;
	m_showRlAlways = false;

	if (m_style & wxGP_DEFAULT) {
		m_showCl = true;
		m_showRl = true;
		m_showClAlways = true;
		m_showRlAlways = true;

		return;
	}
	if (m_style & wxGP_SHOW_CL)
		m_showCl = true;
	if (m_style & wxGP_SHOW_RL)
		m_showRl = true;
	if (m_style & wxGP_SHOW_CL_ALWAYS)
		m_showClAlways = true;
	if (m_style & wxGP_SHOW_RL_ALWAYS)
		m_showRlAlways = true;
}

int CGridEditorPrintout::GetStyle() const
{
	return m_style;
}

bool CGridEditorPrintout::OnPrintPage(int page)
{
	wxDC* dc = GetDC();

	if (dc == nullptr)
		return false;

	CalculateScale(dc);

	m_overallScale = m_screenScale * m_userScale;

	dc->SetUserScale(m_overallScale, m_overallScale);
	dc->SetDeviceOrigin(50 * m_overallScale, 50 * m_overallScale);

	return DrawPage(dc, page);
}

bool CGridEditorPrintout::HasPage(int page)
{
	return true;
}

void CGridEditorPrintout::GetPageInfo(int* minPage, int* maxPage, int* selPageFrom, int* selPageTo)
{
	*minPage = m_minPage;
	*maxPage = m_maxPage;
	*selPageFrom = m_selPageFrom;
	*selPageTo = m_selPageTo;
}

bool CGridEditorPrintout::DrawPage(wxDC* dc, int page)
{
	int columnPages = m_colsPerPage.Count();
	int rowPages = m_rowsPerPage.Count();

	int colIndex, rowIndex;

	page--;

	colIndex = page % columnPages;
	rowIndex = page / columnPages;

	int toCol;
	if (colIndex == m_colsPerPage.Count() - 1)
		toCol = m_view->GetNumberCols();
	else
		toCol = m_colsPerPage.Item(colIndex + 1);
	int toRow;
	if (rowIndex == m_rowsPerPage.Count() - 1)
		toRow = m_view->GetNumberRows();
	else
		toRow = m_rowsPerPage.Item(rowIndex + 1);

	int countWidth = 0;
	int countHeight = 0;

	int cellInitialH = 0;
	int cellInitialW = 0;

	//draw column headers if requested //
	if ((m_showCl && rowIndex == 0) || m_showClAlways) {
		if ((m_showRl && colIndex == 0) || m_showRlAlways) {
			dc->SetBrush(*wxLIGHT_GREY_BRUSH);
			dc->DrawRectangle(countWidth, 0, m_view->GetRowLabelSize(), m_view->GetColLabelSize());
			countWidth += m_view->GetRowLabelSize();
		}
		for (int i = m_colsPerPage.Item(colIndex); i < toCol; i++) {
			dc->SetBrush(*wxLIGHT_GREY_BRUSH);
			wxString str = m_view->GetColLabelValue(i);
			wxRect rect = wxRect(countWidth, 0, m_view->GetColSize(i), m_view->GetColLabelSize());
			wxFont fnt = m_view->GetLabelFont();
			DrawTextInRectangle(*dc, str, rect, fnt, *wxBLACK, wxALIGN_CENTER, wxALIGN_CENTER);
			countWidth += m_view->GetColSize(i);
		}

		cellInitialH = m_view->GetColLabelSize();
	}
	//////////////////////////////////////

	//draw row headers if requested //
	countHeight = cellInitialH;
	if ((m_showRl && colIndex == 0) || m_showRlAlways) {
		for (int i = m_rowsPerPage.Item(rowIndex); i < toRow; i++) {
			dc->SetBrush(*wxLIGHT_GREY_BRUSH);
			wxString str = m_view->GetRowLabelValue(i);
			wxRect rct = wxRect(0, countHeight, m_view->GetRowLabelSize(), m_view->GetRowSize(i));
			wxFont fnt = m_view->GetLabelFont();
			DrawTextInRectangle(*dc, str, rct, fnt, *wxBLACK, wxALIGN_CENTER, wxALIGN_CENTER);
			countHeight += m_view->GetRowSize(i);
		}
		cellInitialW = m_view->GetRowLabelSize();
	}
	////////////////////////////////

	// Draw cell content //
	countHeight = cellInitialH;
	for (int row = m_rowsPerPage.Item(rowIndex); row < toRow; row++) {
		countWidth = cellInitialW;
		for (int col = m_colsPerPage.Item(colIndex); col < toCol; col++) {
			int cell_rows, cell_cols;
			if (m_view->GetCellSize(row, col, &cell_rows, &cell_cols) == wxGridExt::CellSpan_Main) {
				int colSize = 0, rowSize = 0;
				for (int i = col; i < col + cell_cols; i++)
					colSize += m_view->GetColSize(i);
				for (int i = row; i < row + cell_rows; i++)
					rowSize += m_view->GetRowSize(i);
				wxRect rect(countWidth, countHeight, colSize, rowSize);
				dc->SetBrush(m_view->GetCellBackgroundColour(row, col));
				dc->SetPen(*wxTRANSPARENT_PEN);
				dc->DrawRectangle(rect);
				int horz, vert;
				m_view->GetCellAlignment(row, col, &horz, &vert);
				
				DrawTextInRectangle(*dc, m_view->GetCellValue(row, col),
					rect,
					m_view->GetCellFont(row, col),
					m_view->GetCellTextColour(row, col),
					horz, vert,
					m_view->GetCellTextOrient(row, col)
				);

				wxGridExtCellBorder borderLeft = m_view->GetCellBorderLeft(row, col);
				if (borderLeft.m_style != wxPenStyle::wxPENSTYLE_TRANSPARENT) {
					dc->SetPen(wxPen(borderLeft.m_colour, borderLeft.m_width, borderLeft.m_style));
					dc->DrawLine(rect.GetLeft() - borderLeft.m_width, rect.GetTop() - borderLeft.m_width,
						rect.GetLeft() - borderLeft.m_width, rect.GetBottom() + borderLeft.m_width);
				}

				wxGridExtCellBorder borderRight = m_view->GetCellBorderRight(row, col);
				if (borderRight.m_style != wxPenStyle::wxPENSTYLE_TRANSPARENT) {
					dc->SetPen(wxPen(borderLeft.m_colour, borderRight.m_width, borderRight.m_style));
					dc->DrawLine(rect.GetRight() + borderRight.m_width, rect.GetTop() - borderRight.m_width,
						rect.GetRight() + borderRight.m_width, rect.GetBottom() + borderRight.m_width);
				}

				wxGridExtCellBorder borderTop = m_view->GetCellBorderTop(row, col);
				if (borderTop.m_style != wxPenStyle::wxPENSTYLE_TRANSPARENT) {
					dc->SetPen(wxPen(borderTop.m_colour, borderTop.m_width, borderTop.m_style));
					dc->DrawLine(rect.GetLeft() - borderTop.m_width, rect.GetTop() - borderTop.m_width,
						rect.GetRight() + borderTop.m_width, rect.GetTop() - borderTop.m_width);
				}

				wxGridExtCellBorder borderBottom = m_view->GetCellBorderBottom(row, col);
				if (borderBottom.m_style != wxPenStyle::wxPENSTYLE_TRANSPARENT) {
					dc->SetPen(wxPen(borderBottom.m_colour, borderBottom.m_width, borderBottom.m_style));
					dc->DrawLine(rect.GetLeft() - borderBottom.m_width, rect.GetBottom() + borderBottom.m_width,
						rect.GetRight() + borderBottom.m_width, rect.GetBottom() + borderBottom.m_width);
				}
			}
			else if (m_view->GetCellSize(row, col, &cell_rows, &cell_cols) == wxGridExt::CellSpan_None) {
				wxRect rect(countWidth, countHeight, m_view->GetColSize(col), m_view->GetRowSize(row));
				dc->SetBrush(m_view->GetCellBackgroundColour(row, col));
				dc->SetPen(*wxTRANSPARENT_PEN);
				dc->DrawRectangle(rect);
				int horz, vert;
				m_view->GetCellAlignment(row, col, &horz, &vert);
				DrawTextInRectangle(*dc, m_view->GetCellValue(row, col),
					rect,
					m_view->GetCellFont(row, col),
					m_view->GetCellTextColour(row, col),
					horz, vert,
					m_view->GetCellTextOrient(row, col)
				);
				
				wxGridExtCellBorder borderLeft = m_view->GetCellBorderLeft(row, col);
				if (borderLeft.m_style != wxPenStyle::wxPENSTYLE_TRANSPARENT) {
					dc->SetPen(wxPen(borderLeft.m_colour, borderLeft.m_width, borderLeft.m_style));
					dc->DrawLine(rect.GetLeft() - borderLeft.m_width, rect.GetTop() - borderLeft.m_width,
						rect.GetLeft() - borderLeft.m_width, rect.GetBottom() + borderLeft.m_width);
				}

				wxGridExtCellBorder borderRight = m_view->GetCellBorderRight(row, col);
				if (borderRight.m_style != wxPenStyle::wxPENSTYLE_TRANSPARENT) {
					dc->SetPen(wxPen(borderLeft.m_colour, borderRight.m_width, borderRight.m_style));
					dc->DrawLine(rect.GetRight() + borderRight.m_width, rect.GetTop() - borderRight.m_width,
						rect.GetRight() + borderRight.m_width, rect.GetBottom() + borderRight.m_width);
				}

				wxGridExtCellBorder borderTop = m_view->GetCellBorderTop(row, col);
				if (borderTop.m_style != wxPenStyle::wxPENSTYLE_TRANSPARENT) {
					dc->SetPen(wxPen(borderTop.m_colour, borderTop.m_width, borderTop.m_style));
					dc->DrawLine(rect.GetLeft() - borderTop.m_width, rect.GetTop() - borderTop.m_width,
						rect.GetRight() + borderTop.m_width, rect.GetTop() - borderTop.m_width);
				}

				wxGridExtCellBorder borderBottom = m_view->GetCellBorderBottom(row, col);
				if (borderBottom.m_style != wxPenStyle::wxPENSTYLE_TRANSPARENT) {
					dc->SetPen(wxPen(borderBottom.m_colour, borderBottom.m_width, borderBottom.m_style));
					dc->DrawLine(rect.GetLeft() - borderBottom.m_width, rect.GetBottom() + borderBottom.m_width,
						rect.GetRight() + borderBottom.m_width, rect.GetBottom() + borderBottom.m_width);
				}
			}
			
			countWidth += m_view->GetColSize(col);
		}
		
		countHeight += m_view->GetRowSize(row);
	}

	////////////////////////////////

	dc->SetBrush(*wxTRANSPARENT_BRUSH);
	dc->SetPen(*wxBLACK_PEN);
	dc->DrawRectangle(1, 1, m_maxWidth, m_maxHeight);

	return true;
}

void CGridEditorPrintout::OnPreparePrinting()
{
	wxDC* dc = GetDC();
	CalculateScale(dc);
	m_overallScale = m_screenScale * m_userScale;

	dc->SetUserScale(m_overallScale, m_overallScale);
	dc->GetSize(&m_maxWidth, &m_maxHeight);

	m_maxWidth /= m_overallScale;
	m_maxHeight /= m_overallScale;

	m_maxWidth -= 100;
	m_maxHeight -= 100;

	long widthCount = 0, heightCount = 0;

	//Calculate pages per columns
	m_colsPerPage.Clear();
	m_colsPerPage.Add(0);

	if (m_showRl || m_showRlAlways)
		widthCount = m_view->GetRowLabelSize();

	for (int i = 0; i < m_view->GetMaxColBrake(); i++) {
		widthCount += m_view->GetColSize(i);
		if ((widthCount >= m_maxWidth) ||
			(m_colsPerPage.Last() != i && m_view->IsColBrake(i))) {
			m_colsPerPage.Add(i);
			if (m_showRlAlways)
				widthCount = m_view->GetRowLabelSize();
			else
				widthCount = 0;
			i--;
		}
	}

	//Calculate pager per rows
	m_rowsPerPage.Clear();
	m_rowsPerPage.Add(0);

	if (m_showCl || m_showClAlways)
		heightCount = m_view->GetColLabelSize();

	for (int i = 0; i < m_view->GetMaxRowBrake(); i++) {
		heightCount += m_view->GetRowSize(i);
		if ((heightCount >= m_maxHeight) ||
			(m_rowsPerPage.Last() != i && m_view->IsRowBrake(i))) {
			m_rowsPerPage.Add(i);
			if (m_showClAlways)
				heightCount = m_view->GetColLabelSize();
			else
				heightCount = 0;
			i--;
		}
	}

	m_maxPage = m_rowsPerPage.GetCount() * m_colsPerPage.GetCount();
}

void CGridEditorPrintout::CalculateScale(wxDC* dc)
{
	// You might use THIS code to set the printer DC to roughly
	// reflect the screen text size. This page also draws lines of
	// actual length 5cm on the page.

	// Get the logical pixels per inch of screen and printer
	int ppiScreenX, ppiScreenY;
	GetPPIScreen(&ppiScreenX, &ppiScreenY);
	int ppiPrinterX, ppiPrinterY;
	GetPPIPrinter(&ppiPrinterX, &ppiPrinterY);

	// This scales the DC so that the printout roughly represents the
	// the screen scaling.
	float scale = (float)((float)ppiPrinterX / (float)ppiScreenX);

	// Now we have to check in case our real page size is reduced
	// (e.g. because we're drawing to a print preview memory DC)
	int pageWidth, pageHeight;
	int w, h;
	dc->GetSize(&w, &h);
	GetPageSizePixels(&pageWidth, &pageHeight);

	// If printer pageWidth == current DC width, then this doesn't
	// change. But w might be the preview bitmap width,
	// so scale down.
	float screenScale = scale * (float)(w / (float)pageWidth);

	m_screenScale = screenScale;
}

void CGridEditorPrintout::SetUserScale(float scale)
{
	m_userScale = scale;
	m_overallScale = m_screenScale * m_userScale;
}

void CGridEditorPrintout::DrawTextInRectangle(wxDC& dc, const wxString& strValue, wxRect& rect, const wxFont& font, const wxColour& fontClr,
	int horizAlign, int vertAlign, int textOrientation)
{
	wxArrayString lines, naturalLines;
	CGridEditor::ParseLines(strValue, naturalLines);

	rect.x += 2;
	rect.width -= 2;

	for (unsigned int i = 0; i < naturalLines.Count(); i++) {
		wxArrayString wrappedLines = CGridEditor::CGridEditorDrawHelper::GetTextLines(dc, naturalLines.Item(i), font, rect);
		for (unsigned int j = 0; j < wrappedLines.Count(); j++) {
			lines.Add(wrappedLines.Item(j));
		}
	}

	dc.SetTextBackground(fontClr);
	dc.SetTextForeground(fontClr);

	dc.SetFont(font);

	CGridEditor::CGridEditorDrawHelper::DrawTextRectangle(dc,
		lines, rect, horizAlign, vertAlign, textOrientation);
}