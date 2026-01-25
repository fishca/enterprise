#ifndef __BACKEND_CELL_H__
#define __BACKEND_CELL_H__

#include "spreadsheetDescription.h"

class BACKEND_API CBackendSpreadSheetDocument {
public:

	CBackendSpreadSheetDocument() {}
	CBackendSpreadSheetDocument(const CSpreadsheetDescription& spreadsheetDesc) : m_spreadsheetDesc(spreadsheetDesc) {}

	const CSpreadsheetDescription& GetSpreadsheetDesc() const { return m_spreadsheetDesc; }

	//cell 
	void ResetSpreadsheet() { m_spreadsheetDesc.ResetSpreadsheet(); }
	bool IsEmptySpreadsheet() const { return m_spreadsheetDesc.IsEmptySpreadsheet(); }

	int GetNumberRows() const { return m_spreadsheetDesc.GetNumberRows(); }
	int GetNumberCols() const { return m_spreadsheetDesc.GetNumberCols(); }

	void AppendCell(int row, int col,
		const wxString& strCellValue = wxEmptyString, int cellAlignHorz = wxAlignment::wxALIGN_LEFT, int cellAlignVert = wxAlignment::wxALIGN_CENTER, int textOrient = wxOrientation::wxHORIZONTAL,
		const wxFont& cellFont = wxNullFont, wxColour cellBackgroundColour = *wxWHITE, wxColour cellTextColour = *wxBLACK,
		const CSpreadsheetBorderChunk borderAt[4] = {}, int row_size = 1, int col_size = 1)
	{
		CSpreadsheetAttrChunk entry;

		entry.m_row = row;
		entry.m_col = col;

		entry.m_value = strCellValue;
		entry.m_alignHorz = cellAlignHorz;
		entry.m_alignVert = cellAlignVert;
		entry.m_textOrient = textOrient;

		entry.m_font = cellFont;
		entry.m_backgroundColour = cellBackgroundColour;
		entry.m_textColour = cellTextColour;

		entry.m_borderAt[0] = borderAt[0];
		entry.m_borderAt[1] = borderAt[1];
		entry.m_borderAt[2] = borderAt[2];
		entry.m_borderAt[3] = borderAt[3];

		entry.m_row_size = row_size;
		entry.m_col_size = col_size;

		m_spreadsheetDesc.m_cellAt.push_back(entry);
	}

	const CSpreadsheetAttrChunk* GetCell(int row, int col) const
	{
		auto iterator = std::find_if(m_spreadsheetDesc.m_cellAt.begin(), m_spreadsheetDesc.m_cellAt.end(),
			[row, col](const auto& v) { return v.m_row == row && v.m_col == col; });

		if (iterator != m_spreadsheetDesc.m_cellAt.end())
			return &*iterator;

		return nullptr;
	}

	//print brake 
	void AppendBrakeRow(int row) { m_spreadsheetDesc.m_rowBrakeAt.push_back(row); }
	void AppendBrakeCol(int col) { m_spreadsheetDesc.m_colBrakeAt.push_back(col); }

	int GetBrakeRow(size_t idx) const {
		if (idx > m_spreadsheetDesc.m_rowBrakeAt.size())
			return 0;
		return m_spreadsheetDesc.m_rowBrakeAt[idx];
	}

	int GetBrakeCol(size_t idx) const {
		if (idx > m_spreadsheetDesc.m_colBrakeAt.size())
			return 0;
		return m_spreadsheetDesc.m_colBrakeAt[idx];
	}

	int GetBrakeNumberRows() const { return m_spreadsheetDesc.m_rowBrakeAt.size(); }
	int GetBrakeNumberCols() const { return m_spreadsheetDesc.m_colBrakeAt.size(); }

	//size 
	void SetSizeRow(int row, int height = 0) {
		CSpreadsheetRowSizeChunk entry;
		entry.m_row = row;
		entry.m_height = height;
		m_spreadsheetDesc.m_rowHeightAt.push_back(entry);
	}

	const CSpreadsheetRowSizeChunk* GetSizeRow(size_t idx) const {
		if (idx > m_spreadsheetDesc.m_rowHeightAt.size())
			return nullptr;
		return &m_spreadsheetDesc.m_rowHeightAt[idx];
	}
	
	void SetSizeCol(int col, int width = 0) {
		CSpreadsheetColSizeChunk entry;
		entry.m_col = col;
		entry.m_width = width;
		m_spreadsheetDesc.m_colWidthAt.push_back(entry);
	}

	const CSpreadsheetColSizeChunk* GetSizeCol(size_t idx) const {
		if (idx > m_spreadsheetDesc.m_colWidthAt.size())
			return nullptr;
		return &m_spreadsheetDesc.m_colWidthAt[idx];
	}

	int GetSizeNumberRows() const { return m_spreadsheetDesc.m_rowHeightAt.size(); }
	int GetSizeNumberCols() const { return m_spreadsheetDesc.m_colWidthAt.size(); }

	//freeze 
	void SetFreezeRow(int row) { m_spreadsheetDesc.m_freezeRow = row; }
	void SetFreezeCol(int col) { m_spreadsheetDesc.m_freezeCol = col; }

	int GetFreezeRow() const { return m_spreadsheetDesc.m_freezeRow; }
	int GetFreezeCol() const { return m_spreadsheetDesc.m_freezeCol; }

	//area 
	void AppendAreaRow(const wxString& strAreaName,
		unsigned int start, unsigned int end)
	{
		CSpreadsheetAreaChunk entry;
		entry.m_label = strAreaName;
		entry.m_start = start;
		entry.m_end = end;

		m_spreadsheetDesc.m_rowAreaAt.push_back(entry);
	}

	void AppendAreaCol(const wxString& strAreaName,
		unsigned int start, unsigned int end)
	{
		CSpreadsheetAreaChunk entry;
		entry.m_label = strAreaName;
		entry.m_start = start;
		entry.m_end = end;

		m_spreadsheetDesc.m_colAreaAt.push_back(entry);
	}

	const CSpreadsheetAreaChunk* GetAreaRow(size_t idx) const {
		if (idx > m_spreadsheetDesc.m_rowAreaAt.size())
			return nullptr;
		return &m_spreadsheetDesc.m_rowAreaAt[idx];
	}

	const CSpreadsheetAreaChunk* GetAreaRow(const wxString& strAreaName) const {
		auto iterator = std::find_if(m_spreadsheetDesc.m_rowAreaAt.begin(), m_spreadsheetDesc.m_rowAreaAt.end(),
			[strAreaName](const auto& v) { return v.m_label == strAreaName; });
		if (iterator != m_spreadsheetDesc.m_rowAreaAt.end())
			return &*iterator;
		return nullptr;
	}

	const CSpreadsheetAreaChunk* GetAreaCol(size_t idx) const {
		if (idx > m_spreadsheetDesc.m_colAreaAt.size())
			return nullptr;
		return &m_spreadsheetDesc.m_colAreaAt[idx];
	}

	const CSpreadsheetAreaChunk* GetAreaCol(const wxString& strAreaName) const {
		auto iterator = std::find_if(m_spreadsheetDesc.m_colAreaAt.begin(), m_spreadsheetDesc.m_colAreaAt.end(),
			[strAreaName](const auto& v) { return v.m_label == strAreaName; });
		if (iterator != m_spreadsheetDesc.m_colAreaAt.end())
			return &*iterator;
		return nullptr;
	}

	int GetAreaNumberRows() const { return m_spreadsheetDesc.m_rowAreaAt.size(); }
	int GetAreaNumberCols() const { return m_spreadsheetDesc.m_colAreaAt.size(); }

	//load/save form file
	bool LoadFromFile(const wxString& strFileName);
	bool SaveToFile(const wxString& strFileName);

private:

	//cell
	CSpreadsheetDescription m_spreadsheetDesc;
};

#endif 