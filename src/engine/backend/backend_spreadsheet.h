#ifndef __BACKEND_CELL_H__
#define __BACKEND_CELL_H__

#include "spreadsheetDescription.h"

class BACKEND_API IBackendSpreadsheetObjectNotifier {
public:

	virtual ~IBackendSpreadsheetObjectNotifier() {}
	virtual void ResetSpreadsheet(int count = 0) = 0;

	//size 
	virtual void SetRowSize(int row, int height = 0) = 0;
	virtual void SetColSize(int col, int width = 0) = 0;

	//freeze 
	virtual void SetFreezeRow(int row) = 0;
	virtual void SetFreezeCol(int col) = 0;

	//area 
	virtual void AddRowArea(const wxString& strAreaName,
		unsigned int start, unsigned int end) = 0;
	virtual void DeleteRowArea(const wxString& strAreaName) = 0;
	virtual void AddColArea(const wxString& strAreaName,
		unsigned int start, unsigned int end) = 0;
	virtual void DeleteColArea(const wxString& strAreaName) = 0;
	virtual void SetRowSizeArea(const wxString& strAreaName, int start, int end) = 0;
	virtual void SetRowNameArea(size_t idx, const wxString& strAreaName) = 0;;
	virtual void SetColSizeArea(const wxString& strAreaName, int start, int end) = 0;;
	virtual void SetColNameArea(size_t idx, const wxString& strAreaName) = 0;;

	// ------ row and col formatting
	//

	virtual void SetCellBackgroundColour(int row, int col, const wxColour& colour) = 0;
	virtual void SetCellTextColour(int row, int col, const wxColour& colour) = 0;
	virtual void SetCellTextOrient(int row, int col, const int orient) = 0;
	virtual void SetCellFont(int row, int col, const wxFont& font) = 0;
	virtual void SetCellAlignment(int row, int col, const int horiz, const int vert) = 0;
	virtual void SetCellBorderLeft(int row, int col, const CSpreadsheetBorderDescription& desc) = 0;
	virtual void SetCellBorderRight(int row, int col, const CSpreadsheetBorderDescription& desc) = 0;
	virtual void SetCellBorderTop(int row, int col, const CSpreadsheetBorderDescription& desc) = 0;
	virtual void SetCellBorderBottom(int row, int col, const CSpreadsheetBorderDescription& desc) = 0;
	virtual void SetCellSize(int row, int col, int num_rows, int num_cols) = 0;
	virtual void SetCellFitMode(int row, int col, CSpreadsheetAttrDescription::EFitMode fitMode) = 0;
	virtual void SetCellReadOnly(int row, int col, bool isReadOnly = true) = 0;

	// ------ cell brake accessors
	//
	//support printing 
	virtual void AddRowBrake(int row) = 0;
	virtual void AddColBrake(int col) = 0;

	virtual void SetRowBrake(int row) = 0;
	virtual void SetColBrake(int col) = 0;

	// ------ cell value accessors
	//
	virtual void SetCellValue(int row, int col, const wxString& s) = 0;
};

class BACKEND_API CBackendSpreadsheetObject : public wxRefCounter {
public:

	CBackendSpreadsheetObject() : m_docGuid(wxNewUniqueGuid) {}
	CBackendSpreadsheetObject(const CSpreadsheetDescription& spreadsheetDesc) : m_docGuid(wxNewUniqueGuid), m_spreadsheetDesc(spreadsheetDesc) {}

	CSpreadsheetDescription& GetSpreadsheetDesc() { return m_spreadsheetDesc; }
	const CSpreadsheetDescription& GetSpreadsheetDesc() const { return m_spreadsheetDesc; }

	bool IsEmptyDocument() const { return m_spreadsheetDesc.IsEmptySpreadsheet(); }

#pragma region __notifier_h__

	void ResetSpreadsheet(int count = 0);

	//size 
	void SetRowSize(int row, int height = 0);
	void SetColSize(int col, int width = 0);

	//freeze 
	void SetFreezeRow(int row);
	void SetFreezeCol(int col);

	//area 
	void AddRowArea(const wxString& strAreaName,
		unsigned int start, unsigned int end);
	void DeleteRowArea(const wxString& strAreaName);
	void AddColArea(const wxString& strAreaName,
		unsigned int start, unsigned int end);
	void DeleteColArea(const wxString& strAreaName);
	void SetRowSizeArea(const wxString& strAreaName, int start, int end);
	void SetRowNameArea(size_t idx, const wxString& strAreaName);;
	void SetColSizeArea(const wxString& strAreaName, int start, int end);;
	void SetColNameArea(size_t idx, const wxString& strAreaName);;

	// ------ row and col formatting
	//

	void SetCellBackgroundColour(int row, int col, const wxColour& colour);
	void SetCellTextColour(int row, int col, const wxColour& colour);
	void SetCellTextOrient(int row, int col, const int orient);
	void SetCellFont(int row, int col, const wxFont& font);
	void SetCellAlignment(int row, int col, const int horiz, const int vert);
	void SetCellBorderLeft(int row, int col, const CSpreadsheetBorderDescription& desc);
	void SetCellBorderRight(int row, int col, const CSpreadsheetBorderDescription& desc);
	void SetCellBorderTop(int row, int col, const CSpreadsheetBorderDescription& desc);
	void SetCellBorderBottom(int row, int col, const CSpreadsheetBorderDescription& desc);
	void SetCellSize(int row, int col, int num_rows, int num_cols);
	void SetCellFitMode(int row, int col, CSpreadsheetAttrDescription::EFitMode fitMode);
	void SetCellReadOnly(int row, int col, bool isReadOnly = true);

	// ------ cell brake accessors
	//
	//support printing 
	void AddRowBrake(int row);
	void AddColBrake(int col);

	void SetRowBrake(int row);
	void SetColBrake(int col);

	// ------ cell value accessors
	//
	void SetCellValue(int row, int col, const wxString& s);

#pragma endregion 

#pragma region __fs_h__

	//load/save form file
	bool LoadFromFile(const wxString& strFileName);
	bool SaveToFile(const wxString& strFileName);

#pragma endregion 

	//lang 
	wxString GetLangCode() const { return m_locLangCode; }

	//guid 
	CGuid GetDocGuid() const { return m_docGuid; }

#pragma region __notifier_h__

	template <typename T, typename... Args>
	wxSharedPtr<IBackendSpreadsheetObjectNotifier> AddNotifier(Args&&... args) {
		wxSharedPtr<IBackendSpreadsheetObjectNotifier> notifier(new T(std::forward<Args>(args)...));
		m_spreadsheetNotifiers.push_back(notifier);
		return notifier;
	}

	void RemoveNotifier(const wxSharedPtr<IBackendSpreadsheetObjectNotifier>& notify) {
		m_spreadsheetNotifiers.erase(
			std::remove(m_spreadsheetNotifiers.begin(), m_spreadsheetNotifiers.end(), notify));
	}

#pragma endregion

private:

	//localization
	wxString m_locLangCode;

	// assoc with unique id
	CGuid m_docGuid;

	//cell desc
	CSpreadsheetDescription m_spreadsheetDesc;

	//grid notifier 
	wxVector<wxSharedPtr<IBackendSpreadsheetObjectNotifier>> m_spreadsheetNotifiers;
};

#endif 