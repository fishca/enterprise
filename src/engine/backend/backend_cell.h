#ifndef __BACKEND_CELL_H__
#define __BACKEND_CELL_H__

enum EAreaMode
{
	ESectionMode_Left,
	ESectionMode_Upper,
};

#include "backend/backend_core.h"

struct CGridBorderChunk {

	CGridBorderChunk(const wxPenStyle& borderStyle = wxPenStyle::wxPENSTYLE_TRANSPARENT) : m_borderStyle(borderStyle), m_borderWidth(1) {}
	CGridBorderChunk(const wxPenStyle& borderStyle, int borderWidth) : m_borderStyle(borderStyle), m_borderWidth(borderWidth) {}

	operator wxPenStyle() const { return m_borderStyle; }

	wxPenStyle m_borderStyle;
	int m_borderWidth = 1;
};

struct CGridCellChunk
{
	int m_row;
	int m_col;
};

struct CGridCellSizeChunk
{
	CGridCellChunk m_cell;

	int m_row_size = 1;
	int m_col_size = 1;
};

struct CGridAttrChunk
{
	CGridCellChunk m_cell;

	wxString m_strCellName;
	wxString m_strCellText = wxEmptyString;

	wxAlignment m_cellAlignHorz = wxAlignment::wxALIGN_LEFT;
	wxAlignment m_cellAlignVert = wxAlignment::wxALIGN_CENTER;
	wxOrientation m_textOrient = wxOrientation::wxHORIZONTAL;

	wxFont m_cellFont = wxNullFont;
	wxColour m_cellBackgroundColour = *wxWHITE;
	wxColour m_cellTextColour = *wxBLACK;

	CGridBorderChunk m_leftBorder = wxPenStyle::wxPENSTYLE_TRANSPARENT;
	CGridBorderChunk m_rightBorder = wxPenStyle::wxPENSTYLE_TRANSPARENT;
	CGridBorderChunk m_topBorder = wxPenStyle::wxPENSTYLE_TRANSPARENT;
	CGridBorderChunk m_bottomBorder = wxPenStyle::wxPENSTYLE_TRANSPARENT;

	wxColour m_borderColour = *wxBLACK;
};

struct CGridAreaChunk {
	wxString m_strAreaName;
	unsigned int m_rangeFrom;
	unsigned int m_rangeTo;
};

struct CGridDataDocument {

	std::vector<CGridAttrChunk> m_arrayCell;
	std::vector<CGridCellSizeChunk> m_arrayCellSize;

	std::vector<CGridAreaChunk> m_arrayAreaLeft;
	std::vector<CGridAreaChunk> m_arrayAreaUpper;
};

#endif 