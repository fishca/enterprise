#ifndef __CELL_DESCRIPTION_H__
#define __CELL_DESCRIPTION_H__

#include "backend/compiler/value.h"

struct CSpreadsheetBorderChunk {

	bool operator == (const CSpreadsheetBorderChunk& rhs) const {
		return m_style == rhs.m_style &&
			m_width == rhs.m_width && m_colour == rhs.m_colour;
	}

	wxPenStyle m_style;
	unsigned int m_width;
	wxColour m_colour;
};

struct CSpreadsheetAttrChunk {

	bool operator == (const CSpreadsheetAttrChunk& rhs) const {
		return m_row == rhs.m_row && m_col == rhs.m_col 
			&& m_cellAlignHorz == rhs.m_cellAlignHorz 
			&& m_cellAlignVert == rhs.m_cellAlignVert 
			&& m_textOrient == rhs.m_textOrient 
			&& m_cellFont == rhs.m_cellFont 
			&& m_cellBackgroundColour == rhs.m_cellBackgroundColour 
			&& m_cellTextColour == rhs.m_cellTextColour
			&& m_borderAt == rhs.m_borderAt
			&& m_row_size == rhs.m_row_size && m_col_size == rhs.m_col_size;
	}

	unsigned int m_row, m_col;
	wxString m_strCellValue;
	int m_cellAlignHorz;
	int m_cellAlignVert;
	int m_textOrient;
	wxFont m_cellFont;
	wxColour m_cellBackgroundColour;
	wxColour m_cellTextColour;
	CSpreadsheetBorderChunk m_borderAt[4]; //left, right, top, bottom
	int m_row_size = 1, m_col_size = 1;
};

struct CSpreadsheetAreaChunk {
	
	bool operator == (const CSpreadsheetAreaChunk& rhs) const {
		return m_strLabel == rhs.m_strLabel &&
			m_start == rhs.m_start && m_end == rhs.m_end;
	}

	wxString m_strLabel;
	unsigned int m_start, m_end;
};

struct CSpreadsheetRowSizeChunk
{
	unsigned int m_row; 
	unsigned int m_height;
};

struct CSpreadsheetColSizeChunk
{
	unsigned int m_col;
	unsigned int m_width;
};

struct CSpreadsheetDescription {

	void ResetSpreadsheet() {
		
		m_cellAt.clear(); 
		
		m_rowBrakeAt.clear();
		m_colBrakeAt.clear();

		m_freezeRow = 0, m_freezeCol = 0;

		m_rowAreaAt.clear();
		m_colAreaAt.clear();
	}

	bool IsEmptySpreadsheet() const { return GetNumberRows() == 0 && GetNumberCols() == 0; }

	int GetNumberRows() const {
		unsigned int max_row = 0;
		for (const auto& c : m_cellAt)
			max_row = std::max(max_row, c.m_row + 1);
		return max_row;
	}

	int GetNumberCols() const {
		unsigned int max_col = 0;
		for (const auto& c : m_cellAt)
			max_col = std::max(max_col, c.m_col + 1);
		return max_col;
	}

	bool operator == (const CSpreadsheetDescription& rhs) const {

		if (m_cellAt != rhs.m_cellAt)
			return false;

		if (m_rowAreaAt != rhs.m_rowAreaAt || m_colAreaAt != rhs.m_colAreaAt)
			return false;

		if (m_rowBrakeAt != rhs.m_rowBrakeAt || m_colBrakeAt != rhs.m_colBrakeAt)
			return false;

		return m_freezeRow == rhs.m_freezeRow && 
			m_freezeCol == rhs.m_freezeCol;
	}

	//size row 
	std::vector <CSpreadsheetRowSizeChunk> m_rowHeightAt;

	//size col 
	std::vector <CSpreadsheetColSizeChunk> m_colWidthAt;

	//cell
	std::vector<CSpreadsheetAttrChunk> m_cellAt;

	//print brake 
	std::vector <unsigned int> m_rowBrakeAt;
	std::vector <unsigned int> m_colBrakeAt;

	//freeze 
	int m_freezeRow = 0, m_freezeCol = 0;

	//area 
	std::vector<CSpreadsheetAreaChunk> m_rowAreaAt;
	std::vector<CSpreadsheetAreaChunk> m_colAreaAt;
};

class CSpreadsheetDescriptionMemory {
public:
	//load & save object in control 
	static bool LoadData(class CMemoryReader& reader, CSpreadsheetDescription& spreadsheetDesc);
	static bool SaveData(class CMemoryWriter& writer, CSpreadsheetDescription& spreadsheetDesc);
};

#endif  