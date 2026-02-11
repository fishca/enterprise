#ifndef __GRID_EXT_COMMON_H__
#define __GRID_EXT_COMMON_H__

#include "frontend/frontend.h"

#include "backend/backend_spreadsheet.h"
#include "backend/propertyManager/propertyManager.h"

#include "frontend/visualView/controlEnum.h"

static const wxArrayString wxEmptyArrayString;

#include "frontend/win/ctrls/grid/gridextctrl.h"
#include "frontend/win/ctrls/grid/gridexteditors.h"

class FRONTEND_API CGridEditor : public wxGridExt {

	class CGenericSpreadsheetNotifier : public IBackendSpreadsheetNotifier {
	public:

		CGenericSpreadsheetNotifier(CGridEditor* view) : m_view(view) {}
		virtual void ResetSpreadsheet() { m_view->ClearGrid(); }

		//size 
		virtual void SetRowSize(int row, int height = 0) { m_view->SetRowSize(row, height); }
		virtual void SetColSize(int col, int width = 0) { m_view->SetColSize(col, width); }

		//freeze 
		virtual void SetFreezeRow(int row) { m_view->FreezeTo(row, 0); }
		virtual void SetFreezeCol(int col) { m_view->FreezeTo(0, col); }

		//area 
		virtual void AddRowArea(const wxString& strAreaName, unsigned int start, unsigned int end) {}
		virtual void DeleteRowArea(const wxString& strAreaName) {}
		virtual void AddColArea(const wxString& strAreaName, unsigned int start, unsigned int end) {}
		virtual void DeleteColArea(const wxString& strAreaName) {}
		virtual void SetRowSizeArea(const wxString& strAreaName, int start, int end) {}
		virtual void SetRowNameArea(size_t idx, const wxString& strAreaName) {}
		virtual void SetColSizeArea(const wxString& strAreaName, int start, int end) {}
		virtual void SetColNameArea(size_t idx, const wxString& strAreaName) {}

		// ------ row and col formatting
		//

		virtual void SetCellBackgroundColour(int row, int col, const wxColour& colour) { m_view->SetCellBackgroundColour(row, col, colour, false); }
		virtual void SetCellTextColour(int row, int col, const wxColour& colour) { m_view->SetCellTextColour(row, col, colour, false); }
		virtual void SetCellTextOrient(int row, int col, const int orient) { m_view->SetCellTextOrient(row, col, orient, false); }
		virtual void SetCellFont(int row, int col, const wxFont& font) { m_view->SetCellFont(row, col, font, false); }
		virtual void SetCellAlignment(int row, int col, const int horiz, const int vert) { m_view->SetCellAlignment(row, col, horiz, vert, false); }
		virtual void SetCellBorderLeft(int row, int col, const CSpreadsheetBorderDescription& desc) {}
		virtual void SetCellBorderRight(int row, int col, const CSpreadsheetBorderDescription& desc) {}
		virtual void SetCellBorderTop(int row, int col, const CSpreadsheetBorderDescription& desc) {}
		virtual void SetCellBorderBottom(int row, int col, const CSpreadsheetBorderDescription& desc) {}
		virtual void SetCellSize(int row, int col, int num_rows, int num_cols) { m_view->SetCellSize(row, col, num_rows, num_cols, false); }
		virtual void SetCellFitMode(int row, int col, CSpreadsheetAttrDescription::EFitMode fitMode) { m_view->SetCellFitMode(row, col, fitMode == CSpreadsheetAttrDescription::EFitMode::Mode_Overflow ? wxGridExtFitMode::Overflow() : wxGridExtFitMode::Clip(), false); }
		virtual void SetCellReadOnly(int row, int col, bool isReadOnly = true) { m_view->SetCellReadOnly(row, col, isReadOnly, false); }

		// ------ cell brake accessors
		//
		//support printing 
		virtual void AddRowBrake(int row) { m_view->AddRowBrake(row); }
		virtual void AddColBrake(int col) { m_view->AddColBrake(col); }

		virtual void SetRowBrake(int row) { m_view->SetRowBrake(row); }
		virtual void SetColBrake(int col) { m_view->SetColBrake(col); }

		// ------ cell value accessors
		//
		virtual void SetCellValue(int row, int col, const wxString& s) { m_view->SetCellValue(row, col, s, false); }

	private:
		CGridEditor* m_view;
	};

	// the editor for string/text data
	class CGridEditorCellTextEditor : public wxGridExtCellEditor
	{
	public:
		explicit CGridEditorCellTextEditor(size_t maxChars = 0)
			: wxGridExtCellEditor(),
			m_maxChars(maxChars)
		{
		}

		CGridEditorCellTextEditor(const CGridEditorCellTextEditor& other);

		virtual void Create(wxWindow* parent,
			wxWindowID id,
			wxEvtHandler* evtHandler) override;

		virtual void SetSize(const wxRect& rect) override;

		virtual bool IsAcceptedKey(wxKeyEvent& event) override;
		virtual void BeginEdit(int row, int col, wxGridExt* grid) override;
		virtual bool EndEdit(int row, int col, const wxGridExt* grid,
			const wxString& oldval, wxString* newval) override;
		virtual void ApplyEdit(int row, int col, wxGridExt* grid) override;

		virtual void Reset() override;
		virtual void StartingKey(wxKeyEvent& event) override;
		virtual void HandleReturn(wxKeyEvent& event) override;

		// parameters string format is "max_width"
		virtual void SetParameters(const wxString& params) override;
#if wxUSE_VALIDATORS
		virtual void SetValidator(const wxValidator& validator);
#endif

		virtual wxGridExtCellEditor* Clone() const override {
			return new CGridEditorCellTextEditor(*this);
		}

		// added GetValue so we can get the value which is in the control
		virtual wxString GetValue() const override;

	protected:
		wxTextCtrl* Text() const { return (wxTextCtrl*)m_control; }

		// parts of our virtual functions reused by the derived classes
		void DoCreate(wxWindow* parent, wxWindowID id, wxEvtHandler* evtHandler,
			long style = 0);
		void DoBeginEdit(const wxString& startValue);
		void DoReset(const wxString& startValue);

	private:
		size_t                   m_maxChars;        // max number of chars allowed
#if wxUSE_VALIDATORS
		wxScopedPtr<wxValidator> m_validator;
#endif
		wxString                 m_value;
	};

	class CGridEditorStringTable : public wxGridExtStringTable {
	public:

		CGridEditorStringTable() : wxGridExtStringTable() {}
		CGridEditorStringTable(int numRows, int numCols) : wxGridExtStringTable(numRows, numCols) {}

		virtual bool IsEmptyCell(int row, int col)
		{
			wxCHECK_MSG((row >= 0 && row < GetNumberRows()) &&
				(col >= 0 && col < GetNumberCols()),
				true,
				wxT("invalid row or column index in wxGridExtStringTable"));

			return CBackendLocalization::IsEmptyLocalizationString(m_data[row][col]);
		}

		virtual void GetValue(int row, int col, wxString& s) override
		{
			CBackendLocalization::GetTranslateGetRawLocText(m_data[row][col], s);
		}

		virtual void SetValue(int row, int col, const wxString& s) override
		{
			const wxString& value = m_data[row][col];

			if (s != value)
			{
				if (CBackendLocalization::IsLocalizationString(s))
				{
					wxGridExtStringTable::SetValue(row, col, s);
				}
				else if (CBackendLocalization::IsLocalizationString(value))
				{
					static CBackendLocalizationEntryArray array;
					if (CBackendLocalization::CreateLocalizationArray(value, array))
					{
						CBackendLocalization::SetArrayTranslate(array, s);
						wxGridExtStringTable::SetValue(row, col,
							CBackendLocalization::GetRawLocText(array));
					}
				}
				else
				{
					wxGridExtStringTable::SetValue(row, col, CBackendLocalization::CreateLocalizationRawLocText(s));
				}
			}
		}

		// For user defined types
		virtual void* GetValueAsCustom(int row, int col, const wxString& typeName)
		{
			CBackendLocalizationEntryArray* array =
				new CBackendLocalizationEntryArray();
			CBackendLocalization::CreateLocalizationArray(m_data[row][col], *array);
			return array;
		}

		// overridden functions from wxGridExtTableBase
		//
		wxString GetRowLabelValue(int row) {
			// RD: Starting the rows at zero confuses users,
			// no matter how much it makes sense to us geeks.
			return stringUtils::IntToStr(row + 1);
		}

		virtual wxString GetColLabelValue(int col) override {
			return stringUtils::IntToStr(col + 1);
		}
	};

	// the property of grid 
	class CGridEditorCellProperty : public IPropertyObject {
	public:

		void SetView(CGridEditor* view) { m_view = view; }
		CGridEditor* GetView(CGridEditor* view) const { return m_view; }

		CGridEditorCellProperty() : m_view(nullptr) {}

		virtual bool IsEditable() const { return m_view->IsEditable(); }

		//system override 
		virtual wxString GetObjectTypeName() const override { return wxT("extGridProperty"); }
		virtual wxString GetClassName() const { return wxT("extGridProperty"); }

		/// Gets the metadata object
		virtual IMetaData* GetMetaData() const;

		/**
		* Property events
		*/
		virtual void OnPropertyCreated(IProperty* property);
		virtual void OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue);

	private:

		void ClearSelectedCell() { m_currentSelection.clear(); }

		void OnPropertyCreated(IProperty* property, const wxGridExtBlockCoords& coords);
		void OnPropertyChanged(IProperty* property, const wxGridExtBlockCoords& coords);

		void AddSelectedCell(const wxGridExtBlockCoords& coords, bool afterErase = false);
		void ShowProperty();

		CGridEditor* m_view;

		std::vector<wxGridExtBlockCoords> m_currentSelection;

		CPropertyCategory* m_categoryGeneral = IPropertyObject::CreatePropertyCategory(wxT("general"), _("General"));
		CPropertyUString* m_propertyName = IPropertyObject::CreateProperty<CPropertyUString>(m_categoryGeneral, wxT("name"), _("Name"), wxEmptyString);
		CPropertyTString* m_propertyText = IPropertyObject::CreateProperty<CPropertyTString>(m_categoryGeneral, wxT("text"), _("Text"), wxEmptyString);
		CPropertyBoolean* m_propertyReadOnly = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryGeneral, wxT("readOnly"), _("Read only"), false);

		CPropertyCategory* m_categoryAlignment = IPropertyObject::CreatePropertyCategory(wxT("alignment"), _("Alignment"));

		CPropertyEnum<CValueEnumFitMode>* m_propertyFitMode = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumFitMode>>(m_categoryAlignment, wxT("fit_mode"), _("Fit mode"), enFitMode::enFitMode_Overflow);
		CPropertyEnum<CValueEnumHorizontalAlignment>* m_propertyAlignHorz = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumHorizontalAlignment>>(m_categoryAlignment, wxT("align_horz"), _("Horizontal"), wxAlignment::wxALIGN_LEFT);
		CPropertyEnum<CValueEnumVerticalAlignment>* m_propertyAlignVert = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumVerticalAlignment>>(m_categoryAlignment, wxT("align_vert"), _("Vertical"), wxAlignment::wxALIGN_CENTER);
		CPropertyEnum<CValueEnumOrient>* m_propertyOrient = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumOrient>>(m_categoryAlignment, wxT("orient_text"), _("Orientation text"), wxOrientation::wxVERTICAL);

		CPropertyCategory* m_categoryAppearance = IPropertyObject::CreatePropertyCategory(wxT("appearance"), _("Appearance"));
		CPropertyFont* m_propertyFont = IPropertyObject::CreateProperty<CPropertyFont>(m_categoryAppearance, wxT("font"), _("Font"));
		CPropertyColour* m_propertyBackgroundColour = IPropertyObject::CreateProperty<CPropertyColour>(m_categoryAppearance, wxT("background_colour"), _("Background colour"), wxNullColour);
		CPropertyColour* m_propertyTextColour = IPropertyObject::CreateProperty<CPropertyColour>(m_categoryAppearance, wxT("text_colour"), _("Text colour"), wxNullColour);

		CPropertyCategory* m_categoryBorder = IPropertyObject::CreatePropertyCategory(wxT("border"), _("Border"));
		CPropertyEnum<CValueEnumBorder>* m_propertyLeftBorder = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumBorder>>(m_categoryBorder, wxT("left_border"), _("Left"), wxPENSTYLE_TRANSPARENT);
		CPropertyEnum<CValueEnumBorder>* m_propertyRightBorder = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumBorder>>(m_categoryBorder, wxT("right_border"), _("Right"), wxPENSTYLE_TRANSPARENT);
		CPropertyEnum<CValueEnumBorder>* m_propertyTopBorder = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumBorder>>(m_categoryBorder, wxT("top_border"), _("Top"), wxPENSTYLE_TRANSPARENT);
		CPropertyEnum<CValueEnumBorder>* m_propertyBottomBorder = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumBorder>>(m_categoryBorder, wxT("bottom_border"), _("Bottom"), wxPENSTYLE_TRANSPARENT);
		CPropertyColour* m_propertyColourBorder = IPropertyObject::CreateProperty<CPropertyColour>(m_categoryBorder, wxT("border_colour"), _("Colour"), wxNullColour);

		friend class CGridEditor;
	};

public:

	void ActivateEditor();

	void SendPropertyModify() {
		wxGridExt::SendEvent(wxEVT_GRID_EDITOR_HIDDEN, m_currentCellCoords);
	}

	void SendPropertyModify(const wxGridExtCellCoords& coords) {
		wxGridExt::SendEvent(wxEVT_GRID_EDITOR_HIDDEN, coords);
	}

	void EnableProperty(bool enable = true) { m_enableProperty = enable; }

	////////////////////////////////////////////////////////////

	// ctor and Create() create the grid window, as with the other controls
	CGridEditor();
	CGridEditor(class CMetaDocument* document, wxWindow* parent,
		wxWindowID id, const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);

	virtual ~CGridEditor();

#pragma region area

	void AddArea();
	void DeleteArea();

#pragma endregion

	void ShowCells() { wxGridExt::EnableGridLines(!m_gridLinesEnabled); }

	void ShowHeader() {
		if (m_rowLabelWidth > 0) {
			wxGridExt::EnableGridArea(false);
			wxGridExt::SetRowLabelSize(0);
			wxGridExt::SetColLabelSize(0);
		}
		else {
			wxGridExt::EnableGridArea(true);
			wxGridExt::SetRowLabelSize(WXGRID_DEFAULT_ROW_LABEL_WIDTH - 42);
			wxGridExt::SetColLabelSize(WXGRID_MIN_COL_WIDTH);
		}
	}

	void ShowArea() { wxGridExt::EnableGridArea(!m_areaEnabled); }

	void MergeCells();
	void DockTable();

	void Copy();
	void Paste();

	bool AssociateDocument(const wxObjectDataPtr<CBackendSpreadsheetObject>& doc);
	bool GetActiveDocument(wxObjectDataPtr<CBackendSpreadsheetObject>& doc) const;

#pragma region file

	bool LoadDocument(const CSpreadsheetDescription& spreadsheetDesc);
	bool LoadDocument(const wxObjectDataPtr<CBackendSpreadsheetObject>& doc);
	bool SaveDocument(CSpreadsheetDescription& spreadsheetDesc) const;
	bool SaveDocument(wxObjectDataPtr<CBackendSpreadsheetObject>& doc) const;

#pragma endregion 

	class CGridEditorPrintout* CreatePrintout() const;

protected:

	bool LoadSpreadsheet(const CSpreadsheetDescription& spreadsheetDesc);
	bool SaveSpreadsheet(CSpreadsheetDescription& spreadsheetDesc) const;

	//events:
	void OnMouseRightDown(wxGridExtEvent& event);

	// This seems to be required for wxMotif/wxGTK otherwise the mouse
	// cursor must be in the cell edit control to get key events
	//
	void OnKeyDown(wxKeyEvent& event);

	void OnSelectCell(wxGridExtEvent& event);
	void OnSelectCells(wxGridExtRangeSelectEvent& event);

	void OnGridRowSize(wxGridExtSizeEvent& event);
	void OnGridColSize(wxGridExtSizeEvent& event);
	void OnGridRowBrake(wxGridExtSizeEvent& event);
	void OnGridColBrake(wxGridExtSizeEvent& event);
	void OnGridRowArea(wxGridExtAreaEvent& event);
	void OnGridColArea(wxGridExtAreaEvent& event);

	void OnGridTableModified(wxGridExtEvent& event);
	void OnGridTableAttrModified(wxGridExtEvent& event);

	void OnCopy(wxCommandEvent& event);
	void OnPaste(wxCommandEvent& event);
	void OnDelete(wxCommandEvent& event);

	void OnRowHeight(wxCommandEvent& event);
	void OnColWidth(wxCommandEvent& event);
	void OnHideCell(wxCommandEvent& event);
	void OnShowCell(wxCommandEvent& event);

	void OnProperties(wxCommandEvent& event);

	void OnScroll(wxScrollWinEvent& event);

	void OnIdle(wxIdleEvent& event);
	void OnSize(wxSizeEvent& event);

	void OnGridZoom(wxGridExtEvent& event);

private:

	const wxString m_GRID_VALUE_STRING = wxGRID_VALUE_STRING;

	//document 
	CMetaDocument* m_document;

	// grid property
	CGridEditorCellProperty* m_cellProperty;

	//grid doc
	wxObjectDataPtr<CBackendSpreadsheetObject> m_spreadsheetObject;
	wxSharedPtr<IBackendSpreadsheetNotifier> m_notifier;

	//grid enabled property? 
	bool m_enableProperty;

	wxDECLARE_EVENT_TABLE();
};


#endif 