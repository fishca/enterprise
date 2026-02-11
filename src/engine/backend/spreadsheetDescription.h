#ifndef __CELL_DESCRIPTION_H__
#define __CELL_DESCRIPTION_H__

#include "backend/compiler/value.h"

struct CSpreadsheetBorderDescription {

	bool operator == (const CSpreadsheetBorderDescription& rhs) const {
		return m_style == rhs.m_style &&
			m_width == rhs.m_width && m_colour == rhs.m_colour;
	}

	wxPenStyle m_style = wxPENSTYLE_TRANSPARENT;
	wxColour m_colour = *wxBLACK;
	int m_width = 1;
};

struct CSpreadsheetAttrDescription {

	enum EFitMode
	{
		// This is a hack to save space: the first 4 elements of this enum are
		// the same as those of wxEllipsizeMode.
		Mode_Unset = wxELLIPSIZE_NONE,
		Mode_EllipsizeStart = wxELLIPSIZE_START,
		Mode_EllipsizeMiddle = wxELLIPSIZE_MIDDLE,
		Mode_EllipsizeEnd = wxELLIPSIZE_END,
		Mode_Overflow,
		Mode_Clip
	};

	CSpreadsheetAttrDescription(int row, int col) : m_row(row), m_col(col) {}

	bool IsEmptyValue() const { return m_value.IsEmpty(); }

	wxString GetValue() const { return m_value; }
	void GetValue(wxString& s) const { s = m_value; }

	int GetCellSize(int* num_rows, int* num_cols) const {

		if (num_rows != nullptr)
			*num_rows = m_row_size;
		if (num_cols != nullptr)
			*num_cols = m_col_size;

		if (m_row_size == 1 && m_col_size == 1)
			return 0; // just a normal cell

		if (m_row_size < 0 || m_col_size < 0)
			return -1; // covered by a multi-span cell

		// this cell spans multiple cells to its right/bottom
		return 1;
	}

	void SetCellSize(int num_rows, int num_cols) {
		m_row_size = num_rows;
		m_col_size = num_cols;
	}

	bool operator == (const CSpreadsheetAttrDescription& rhs) const {
		return m_row == rhs.m_row && m_col == rhs.m_col
			&& m_alignHorz == rhs.m_alignHorz
			&& m_alignVert == rhs.m_alignVert
			&& m_textOrient == rhs.m_textOrient
			&& m_font == rhs.m_font
			&& m_backgroundColour == rhs.m_backgroundColour
			&& m_textColour == rhs.m_textColour
			&& m_borderAt == rhs.m_borderAt
			&& m_row_size == rhs.m_row_size && m_col_size == rhs.m_col_size;
	}

	unsigned int m_row, m_col;
	wxString m_value;
	int m_alignHorz = wxALIGN_LEFT;
	int m_alignVert = wxALIGN_TOP;
	int m_textOrient = wxHORIZONTAL;
	wxFont m_font = wxFont(8, wxFontFamily::wxFONTFAMILY_DEFAULT, wxFontStyle::wxFONTSTYLE_NORMAL, wxFontWeight::wxFONTWEIGHT_NORMAL);
	wxColour m_backgroundColour = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
	wxColour m_textColour = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	CSpreadsheetBorderDescription m_borderAt[4]; //left, right, top, bottom
	int m_row_size = 1, m_col_size = 1;
	EFitMode m_fitMode = EFitMode::Mode_Overflow;
	bool m_isReadOnly = false;
};

struct CSpreadsheetAreaDescription {

	CSpreadsheetAreaDescription(const wxString& label, unsigned int start, unsigned int end) : m_label(label), m_start(start), m_end(end) {}

	bool operator == (const CSpreadsheetAreaDescription& rhs) const {
		return m_label == rhs.m_label &&
			m_start == rhs.m_start && m_end == rhs.m_end;
	}

	wxString m_label;
	unsigned int m_start, m_end;
};

struct CSpreadsheetRowSizeDescription
{
	CSpreadsheetRowSizeDescription(unsigned int row, unsigned int height = 0) : m_row(row), m_height(height) {}

	unsigned int m_row;
	unsigned int m_height = 0;
};

struct CSpreadsheetColSizeDescription
{
	CSpreadsheetColSizeDescription(unsigned int col, unsigned int width = 0) : m_col(col), m_width(width) {}

	unsigned int m_col;
	unsigned int m_width = 0;
};

struct CSpreadsheetDescription {

	void ResetSpreadsheet(int count = 0) {

		m_cellAt.reserve(count);
		m_cellAt.clear();

		m_rowBrakeAt.clear();
		m_colBrakeAt.clear();

		m_freezeRow = 0, m_freezeCol = 0;

		m_rowAreaAt.clear();
		m_colAreaAt.clear();
	}

	bool IsEmptySpreadsheet() const { return GetNumberRows() == 0 && GetNumberCols() == 0; }

	const CSpreadsheetAttrDescription* GetCell(int row, int col) const {
		auto iterator = std::find_if(m_cellAt.begin(), m_cellAt.end(),
			[row, col](const auto& v) { return v.m_row == row && v.m_col == col; });

		if (iterator != m_cellAt.end())
			return &*iterator;

		return nullptr;
	}

	CSpreadsheetAttrDescription* GetOrCreateCell(int row, int col) {
		auto iterator = std::find_if(m_cellAt.begin(), m_cellAt.end(),
			[row, col](const auto& v) { return v.m_row == row && v.m_col == col; });

		if (iterator != m_cellAt.end())
			return &*iterator;

		CSpreadsheetAttrDescription& entry =
			m_cellAt.emplace_back(row, col);

		return &entry;
	}

	const CSpreadsheetAttrDescription* GetCellByIdx(size_t idx) const {
		if (idx > m_cellAt.size())
			return nullptr;
		return &m_cellAt[idx];
	}

	int GetCellCount() const { return m_cellAt.size(); }

	int GetRowBrakeByIdx(size_t idx) const {
		if (idx > m_rowBrakeAt.size())
			return 0;
		return m_rowBrakeAt[idx];
	}
	
	int GetColBrakeByIdx(size_t idx) const {
		if (idx > m_colBrakeAt.size())
			return 0;
		return m_colBrakeAt[idx];
	}

	int GetBrakeNumberRows() const { return m_rowBrakeAt.size(); }
	int GetBrakeNumberCols() const { return m_colBrakeAt.size(); }

	//size 
	void SetRowSize(int row, int height = 0) {

		auto iterator = std::find_if(m_rowHeightAt.begin(),
			m_rowHeightAt.end(), [row](const auto& value) { return row == value.m_row; });

		if (iterator != m_rowHeightAt.end()) {
			iterator->m_height = height;
			return;
		}

		m_rowHeightAt.emplace_back(row, height);
	}

	const CSpreadsheetRowSizeDescription* GetRowSizeByIdx(size_t idx) const {
		if (idx > m_rowHeightAt.size())
			return nullptr;
		return &m_rowHeightAt[idx];
	}

	void SetColSize(int col, int width = 0) {

		auto iterator = std::find_if(m_colWidthAt.begin(),
			m_colWidthAt.end(), [col](const auto& value) { return col == value.m_col; });

		if (iterator != m_colWidthAt.end()) {
			iterator->m_width = width;
			return;
		}

		m_colWidthAt.emplace_back(col, width);
	}

	const CSpreadsheetColSizeDescription* GetColSizeByIdx(size_t idx) const {
		if (idx > m_colWidthAt.size())
			return nullptr;
		return &m_colWidthAt[idx];
	}

	int GetSizeNumberRows() const { return m_rowHeightAt.size(); }
	int GetSizeNumberCols() const { return m_colWidthAt.size(); }

	//freeze 
	void SetFreezeRow(int row) { m_freezeRow = row; }
	void SetFreezeCol(int col) { m_freezeCol = col; }

	int GetFreezeRow() const { return m_freezeRow; }
	int GetFreezeCol() const { return m_freezeCol; }

	//area 
	void AddRowArea(const wxString& strAreaName,
		unsigned int start, unsigned int end) {
		m_rowAreaAt.emplace_back(strAreaName, start, end);
	}

	void DeleteRowArea(const wxString& strAreaName) {
		auto iterator = std::find_if(m_rowAreaAt.begin(), m_rowAreaAt.end(),
			[strAreaName](const auto& v) { return v.m_label == strAreaName; });
		if (iterator != m_rowAreaAt.end())
			m_rowAreaAt.erase(iterator);
	}

	void AddColArea(const wxString& strAreaName,
		unsigned int start, unsigned int end) {
		m_colAreaAt.emplace_back(strAreaName, start, end);
	}

	void DeleteColArea(const wxString& strAreaName) {
		auto iterator = std::find_if(m_colAreaAt.begin(), m_colAreaAt.end(),
			[strAreaName](const auto& v) { return v.m_label == strAreaName; });
		if (iterator != m_colAreaAt.end())
			m_colAreaAt.erase(iterator);
	}

	const CSpreadsheetAreaDescription* GetRowAreaByIdx(size_t idx) const {
		if (idx > m_rowAreaAt.size())
			return nullptr;
		return &m_rowAreaAt[idx];
	}

	const CSpreadsheetAreaDescription* GetRowAreaByName(const wxString& strAreaName) const {
		auto iterator = std::find_if(m_rowAreaAt.begin(), m_rowAreaAt.end(),
			[strAreaName](const auto& v) { return v.m_label == strAreaName; });
		if (iterator != m_rowAreaAt.end())
			return &*iterator;
		return nullptr;
	}

	void SetRowSizeArea(const wxString& strAreaName, int start, int end) {
		auto iterator = std::find_if(m_rowAreaAt.begin(), m_rowAreaAt.end(),
			[strAreaName](const auto& v) { return v.m_label == strAreaName; });
		if (iterator != m_rowAreaAt.end()) {
			iterator->m_start = start;
			iterator->m_end = end;
		}
	}

	void SetRowNameArea(size_t idx, const wxString& strAreaName) {
		if (idx > m_rowAreaAt.size())
			return;
		m_rowAreaAt[idx].m_label = strAreaName;
	}

	const CSpreadsheetAreaDescription* GetColAreaByIdx(size_t idx) const {
		if (idx > m_colAreaAt.size())
			return nullptr;
		return &m_colAreaAt[idx];
	}

	const CSpreadsheetAreaDescription* GetColAreaByName(const wxString& strAreaName) const {
		auto iterator = std::find_if(m_colAreaAt.begin(), m_colAreaAt.end(),
			[strAreaName](const auto& v) { return v.m_label == strAreaName; });
		if (iterator != m_colAreaAt.end())
			return &*iterator;
		return nullptr;
	}

	void SetColSizeArea(const wxString& strAreaName, int start, int end) {
		auto iterator = std::find_if(m_colAreaAt.begin(), m_colAreaAt.end(),
			[strAreaName](const auto& v) { return v.m_label == strAreaName; });
		if (iterator != m_colAreaAt.end()) {
			iterator->m_start = start;
			iterator->m_end = end;
		}
	}

	void SetColNameArea(size_t idx, const wxString& strAreaName) {
		if (idx > m_colAreaAt.size())
			return;
		m_colAreaAt[idx].m_label = strAreaName;
	}

	int GetAreaNumberRows() const { return m_rowAreaAt.size(); }
	int GetAreaNumberCols() const { return m_colAreaAt.size(); }

	// ------ grid dimensions
	//

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

	// ------ row and col formatting
	//
	int GetRowSize(int row) const {
		auto iterator = std::find_if(m_rowHeightAt.begin(),
			m_rowHeightAt.end(), [row](const auto& value) { return row == value.m_row; });

		if (iterator != m_rowHeightAt.end())
			return iterator->m_height;

		return s_defaultRowHeight;
	}
	bool IsRowShown(int row) const { return GetRowSize(row) != 0; }
	int GetColSize(int col) const {
		auto iterator = std::find_if(m_colWidthAt.begin(),
			m_colWidthAt.end(), [col](const auto& value) { return col == value.m_col; });

		if (iterator != m_colWidthAt.end())
			return iterator->m_width;

		return s_defaultColWidth;
	}

	bool IsColShown(int col) const { return GetColSize(col) != 0; }

	wxColour GetCellBackgroundColour(int row, int col) const {
		const CSpreadsheetAttrDescription* cell = GetCell(row, col);
		if (cell != nullptr)
			return cell->m_backgroundColour;
		return wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
	}

	void SetCellBackgroundColour(int row, int col, const wxColour& colour) {
		CSpreadsheetAttrDescription* cell = GetOrCreateCell(row, col);
		if (cell != nullptr)
			cell->m_backgroundColour = colour;
	}

	wxColour GetCellTextColour(int row, int col) const {
		const CSpreadsheetAttrDescription* cell = GetCell(row, col);
		if (cell != nullptr)
			return cell->m_textColour;
		return wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	}

	void SetCellTextColour(int row, int col, const wxColour& colour) {
		CSpreadsheetAttrDescription* cell = GetOrCreateCell(row, col);
		if (cell != nullptr)
			cell->m_textColour = colour;
	}

	int GetCellTextOrient(int row, int col) const {
		const CSpreadsheetAttrDescription* cell = GetCell(row, col);
		if (cell != nullptr)
			return cell->m_textOrient;
		return wxHORIZONTAL;
	}

	void SetCellTextOrient(int row, int col, const int orient) {
		CSpreadsheetAttrDescription* cell = GetOrCreateCell(row, col);
		if (cell != nullptr)
			cell->m_textOrient = orient;
	}

	wxFont GetCellFont(int row, int col) const {
		const CSpreadsheetAttrDescription* cell = GetCell(row, col);
		if (cell != nullptr)
			return cell->m_font;
		return wxFont(8, wxFontFamily::wxFONTFAMILY_DEFAULT, wxFontStyle::wxFONTSTYLE_NORMAL, wxFontWeight::wxFONTWEIGHT_NORMAL);
	}

	void SetCellFont(int row, int col, const wxFont& font) {
		CSpreadsheetAttrDescription* cell = GetOrCreateCell(row, col);
		if (cell != nullptr)
			cell->m_font = font;
	}

	void GetCellAlignment(int row, int col, int* horiz, int* vert) const {
		const CSpreadsheetAttrDescription* cell = GetCell(row, col);
		if (cell != nullptr) {
			if (horiz) *horiz = cell->m_alignHorz;
			if (vert) *vert = cell->m_alignVert;
		}
	}

	void SetCellAlignment(int row, int col, const int horiz, const int vert) {
		CSpreadsheetAttrDescription* cell = GetOrCreateCell(row, col);
		if (cell != nullptr) {
			cell->m_alignHorz = horiz;
			cell->m_alignVert = vert;
		}
	}

	CSpreadsheetBorderDescription GetCellBorderLeft(int row, int col) const {
		const CSpreadsheetAttrDescription* cell = GetCell(row, col);
		if (cell != nullptr) return cell->m_borderAt[0];
		return CSpreadsheetBorderDescription();
	}

	void SetCellBorderLeft(int row, int col, const CSpreadsheetBorderDescription& desc) {
		CSpreadsheetAttrDescription* cell = GetOrCreateCell(row, col);
		if (cell != nullptr)
			cell->m_borderAt[0] = desc;
	}

	CSpreadsheetBorderDescription GetCellBorderRight(int row, int col) const {
		const CSpreadsheetAttrDescription* cell = GetCell(row, col);
		if (cell != nullptr) return cell->m_borderAt[1];
		return CSpreadsheetBorderDescription();
	}

	void SetCellBorderRight(int row, int col, const CSpreadsheetBorderDescription& desc) {
		CSpreadsheetAttrDescription* cell = GetOrCreateCell(row, col);
		if (cell != nullptr)
			cell->m_borderAt[1] = desc;
	}

	CSpreadsheetBorderDescription GetCellBorderTop(int row, int col) const {
		const CSpreadsheetAttrDescription* cell = GetCell(row, col);
		if (cell != nullptr) return cell->m_borderAt[2];
		return CSpreadsheetBorderDescription();
	}

	void SetCellBorderTop(int row, int col, const CSpreadsheetBorderDescription& desc) {
		CSpreadsheetAttrDescription* cell = GetOrCreateCell(row, col);
		if (cell != nullptr)
			cell->m_borderAt[2] = desc;
	}

	CSpreadsheetBorderDescription GetCellBorderBottom(int row, int col) const {
		const CSpreadsheetAttrDescription* cell = GetCell(row, col);
		if (cell != nullptr) return cell->m_borderAt[3];
		return CSpreadsheetBorderDescription();
	}

	void SetCellBorderBottom(int row, int col, const CSpreadsheetBorderDescription& desc) {
		CSpreadsheetAttrDescription* cell = GetOrCreateCell(row, col);
		if (cell != nullptr)
			cell->m_borderAt[3] = desc;
	}

	int GetCellSize(int row, int col, int* num_rows, int* num_cols) const {
		const CSpreadsheetAttrDescription* cell = GetCell(row, col);
		if (cell != nullptr) return cell->GetCellSize(num_rows, num_cols);
		if (num_rows != nullptr) *num_rows = 1;
		if (num_cols != nullptr) *num_cols = 1;
		return 0;
	}

	void SetCellSize(int row, int col, int num_rows, int num_cols) {
		CSpreadsheetAttrDescription* cell = GetOrCreateCell(row, col);
		if (cell != nullptr) cell->SetCellSize(num_rows, num_cols);
	}

	CSpreadsheetAttrDescription::EFitMode GetCellFitMode(int row, int col) {
		CSpreadsheetAttrDescription* cell = GetOrCreateCell(row, col);
		if (cell != nullptr) return cell->m_fitMode;
		return CSpreadsheetAttrDescription::EFitMode::Mode_Overflow;
	}

	void SetCellFitMode(int row, int col, CSpreadsheetAttrDescription::EFitMode fitMode) {
		CSpreadsheetAttrDescription* cell = GetOrCreateCell(row, col);
		if (cell != nullptr) cell->m_fitMode = fitMode;
	}

	bool GetCellReadOnly(int row, int col, bool isReadOnly = true)
	{
		CSpreadsheetAttrDescription* cell = GetOrCreateCell(row, col);
		if (cell != nullptr) return cell->m_isReadOnly;
		return false;
	}

	void SetCellReadOnly(int row, int col, bool isReadOnly = true) {
		CSpreadsheetAttrDescription* cell = GetOrCreateCell(row, col);
		if (cell != nullptr) cell->m_isReadOnly = isReadOnly;
	}

	// ------ cell brake accessors
	//
	//support printing 
	void AddRowBrake(int row) { m_rowBrakeAt.emplace_back(row); }
	void AddColBrake(int col) { m_colBrakeAt.emplace_back(col); }

	void SetRowBrake(int row) {
		if (m_rowBrakeAt.size() != 0)
			m_rowBrakeAt[m_rowBrakeAt.size() - 1] =
			wxMax(wxMin(m_rowBrakeAt[m_rowBrakeAt.size() - 1], GetNumberRows() - 1), row);
		else
			m_rowBrakeAt.emplace_back(row);
	}

	void SetColBrake(int col) {
		if (m_colBrakeAt.size() != 0)
			m_colBrakeAt[m_colBrakeAt.size() - 1] =
			wxMax(wxMin(m_colBrakeAt[m_colBrakeAt.size() - 1], GetNumberCols() - 1), col);
		else
			m_colBrakeAt.emplace_back(col);
	}

	bool IsColBrake(int col) const {
		auto iterator =
			std::find(m_colBrakeAt.begin(), m_colBrakeAt.end(), col);
		return iterator != m_colBrakeAt.end();
	}

	bool IsRowBrake(int row) const {
		auto iterator =
			std::find(m_rowBrakeAt.begin(), m_rowBrakeAt.end(), row);
		return iterator != m_rowBrakeAt.end();
	}

	int GetMaxRowBrake() const {
		if (m_rowBrakeAt.size() != 0)
			return m_rowBrakeAt[m_rowBrakeAt.size() - 1];
		return 0;
	}

	int GetMaxColBrake() const {
		if (m_colBrakeAt.size() != 0)
			return m_colBrakeAt[m_colBrakeAt.size() - 1];
		return 0;
	}

	// ------ label and gridline formatting
	//
	int      GetRowLabelSize() const { return s_rowLabelWidth; }
	int      GetColLabelSize() const { return s_colLabelHeight; }

	wxFont   GetLabelFont() const { return m_labelFont; }

	wxString GetRowLabelValue(int row) const { return stringUtils::IntToStr(row + 1); }
	wxString GetColLabelValue(int col) const { return stringUtils::IntToStr(col + 1); }

	// ------ cell value accessors
	//
	bool IsEmptyCell(int row, int col) const {
		const CSpreadsheetAttrDescription* cell = GetCell(row, col);
		if (cell != nullptr)
			return cell->IsEmptyValue();
		return true;
	}

	wxString GetCellValue(int row, int col) const {
		const CSpreadsheetAttrDescription* cell = GetCell(row, col);
		if (cell != nullptr)
			return cell->m_value;
		return wxT("");
	}

	void GetCellValue(int row, int col, wxString& s) const {
		const CSpreadsheetAttrDescription* cell = GetCell(row, col);
		if (cell != nullptr)
			s = cell->m_value;
		s = wxT("");
	}

	void SetCellValue(int row, int col, const wxString& s) {
		CSpreadsheetAttrDescription* cell = GetOrCreateCell(row, col);
		if (cell != nullptr)
			cell->m_value = s;
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

private:

	const static int s_defaultRowHeight = 15;
	const static int s_rowLabelWidth = 40;

	const static int s_defaultColWidth = 70;
	const static int s_colLabelHeight = 15;

	// default font
	wxFont m_labelFont;

	//size row 
	std::vector <CSpreadsheetRowSizeDescription> m_rowHeightAt;

	//size col 
	std::vector <CSpreadsheetColSizeDescription> m_colWidthAt;

	//cell
	std::vector<CSpreadsheetAttrDescription> m_cellAt;

	//print brake 
	std::vector <unsigned int> m_rowBrakeAt;
	std::vector <unsigned int> m_colBrakeAt;

	//freeze 
	int m_freezeRow = 0, m_freezeCol = 0;

	//area 
	std::vector<CSpreadsheetAreaDescription> m_rowAreaAt;
	std::vector<CSpreadsheetAreaDescription> m_colAreaAt;
};

class BACKEND_API CSpreadsheetDescriptionMemory {
public:
	//load & save object in control 
	static bool LoadData(class CMemoryReader& reader, CSpreadsheetDescription& spreadsheetDesc);
	static bool SaveData(class CMemoryWriter& writer, CSpreadsheetDescription& spreadsheetDesc);
};

#endif  