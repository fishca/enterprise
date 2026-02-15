#ifndef __GRID_EXT_COMMON_H__
#define __GRID_EXT_COMMON_H__

#include "frontend/frontend.h"

#include "backend/backend_spreadsheet.h"
#include "backend/propertyManager/propertyManager.h"
#include "backend/system/value/valueSpreadsheet.h"

static const wxArrayString wxEmptyArrayString;

static const wxString s_strTypeTextOrString = wxT("stringText");
static const wxString s_strTypeTemplate = wxT("stringTemplate");
static const wxString s_strTypeParameter = wxT("stringParameter");

#include "frontend/win/ctrls/grid/gridextctrl.h"
#include "frontend/win/ctrls/grid/gridexteditors.h"

class FRONTEND_API CGridEditor : public wxGridExt {

	class CGenericSpreadsheetNotifier : public IBackendSpreadsheetNotifier {
	public:

		CGenericSpreadsheetNotifier(CGridEditor* view) : m_view(view) {}
		virtual void ClearSpreadsheet() { m_view->ClearGrid(); }

		//size 
		virtual void SetRowSize(int row, int height = 0) { m_view->SetRowSize(row, height); }
		virtual void SetColSize(int col, int width = 0) { m_view->SetColSize(col, width); }

		//freeze 
		virtual void SetRowFreeze(int row) { m_view->FreezeTo(row, 0); }
		virtual void SetColFreeze(int col) { m_view->FreezeTo(0, col); }

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
		virtual void SetCellFitMode(int row, int col, CSpreadsheetCellDescription::EFitMode fitMode) { m_view->SetCellFitMode(row, col, fitMode == CSpreadsheetCellDescription::EFitMode::Mode_Overflow ? wxGridExtFitMode::Overflow() : wxGridExtFitMode::Clip(), false); }
		virtual void SetCellReadOnly(int row, int col, bool isReadOnly = true) { m_view->SetCellReadOnly(row, col, isReadOnly, false); }

		// ------ cell brake accessors
		//
		//support printing 
		virtual void AddRowBrake(int row) { m_view->AddRowBrake(row); }
		virtual void AddColBrake(int col) { m_view->AddColBrake(col); }

		virtual void DeleteRowBrake(int row) { m_view->DeleteRowBrake(row); }
		virtual void DeleteColBrake(int col) { m_view->DeleteColBrake(col); }

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

		virtual bool IsEmptyCell(int row, int col) {
			wxCHECK_MSG((row >= 0 && row < GetNumberRows()) &&
				(col >= 0 && col < GetNumberCols()),
				true,
				wxT("invalid row or column index in wxGridExtStringTable"));

			const enSpreadsheetFillType type = GetTypeString(row, col);
			return type == enSpreadsheetFillType_StrText || type == enSpreadsheetFillType_StrTemplate ?
				CBackendLocalization::IsEmptyLocalizationString(m_data[row][col]) : m_data[row][col].IsEmpty();
		}

		virtual bool CanGetValueAs(int row, int col, const wxString& typeName)
		{
			const enSpreadsheetFillType type = GetTypeString(row, col);

			if (type == enSpreadsheetFillType_StrText)
				return typeName == s_strTypeTextOrString;
			else if (type == enSpreadsheetFillType_StrTemplate)
				return typeName == s_strTypeTemplate;
			else if (type == enSpreadsheetFillType_StrParameter)
				return typeName == s_strTypeParameter;

			return typeName == wxT("string");
		}

		virtual bool CanSetValueAs(int row, int col, const wxString& typeName)
		{
			const enSpreadsheetFillType type = GetTypeString(row, col);

			if (type == enSpreadsheetFillType_StrText)
				return typeName == s_strTypeTextOrString;
			else if (type == enSpreadsheetFillType_StrTemplate)
				return typeName == s_strTypeTemplate;
			else if (type == enSpreadsheetFillType_StrParameter)
				return typeName == s_strTypeParameter;

			return typeName == wxT("string");
		}

		virtual void GetValue(int row, int col, wxString& s) override {
			const enSpreadsheetFillType type = GetTypeString(row, col);
			if (type == enSpreadsheetFillType_StrText || type == enSpreadsheetFillType_StrTemplate)
				CBackendLocalization::GetTranslateGetRawLocText(m_data[row][col], s);
			else if (type == enSpreadsheetFillType_StrParameter)
				s = m_data[row][col];
		}

		virtual void SetValue(int row, int col, const wxString& s) override {
			const wxString& value = m_data[row][col];
			if (s != value) {
				const enSpreadsheetFillType type = GetTypeString(row, col);
				if (type == enSpreadsheetFillType_StrText || type == enSpreadsheetFillType_StrTemplate) {
					if (CBackendLocalization::IsLocalizationString(s)) {
						wxGridExtStringTable::SetValue(row, col, s);
					}
					else if (CBackendLocalization::IsLocalizationString(value)) {
						static CBackendLocalizationEntryArray array;
						if (CBackendLocalization::CreateLocalizationArray(value, array)) {
							CBackendLocalization::SetArrayTranslate(array, s);
							wxGridExtStringTable::SetValue(row, col,
								CBackendLocalization::GetRawLocText(array));
						}
					}
					else {
						wxGridExtStringTable::SetValue(row, col, CBackendLocalization::CreateLocalizationRawLocText(s));
					}
				}
				else if (type == enSpreadsheetFillType_StrParameter) {
					wxGridExtStringTable::SetValue(row, col, s);
				}
			}
		}

		// For user defined types
		virtual void SetValueAsCustom(int row, int col, const wxString& typeName, void* value) {

			if (value && stringUtils::CompareString(typeName, s_strTypeTextOrString)) {
				const wxString* s = static_cast<wxString*>(value);
				m_setValue.insert_or_assign(std::make_pair(row, col), enSpreadsheetFillType_StrText);
				CGridEditorStringTable::SetValue(row, col, *s);
			}
			else if (value && stringUtils::CompareString(typeName, s_strTypeTemplate)) {
				const wxString* s = static_cast<wxString*>(value);
				m_setValue.insert_or_assign(std::make_pair(row, col), enSpreadsheetFillType_StrTemplate);
				CGridEditorStringTable::SetValue(row, col, *s);
			}
			else if (value && stringUtils::CompareString(typeName, s_strTypeParameter)) {
				const wxString* s = static_cast<wxString*>(value);
				m_setValue.insert_or_assign(std::make_pair(row, col), enSpreadsheetFillType_StrParameter);
				CGridEditorStringTable::SetValue(row, col, *s);
			}
		}

		virtual void* GetValueAsCustom(int row, int col, const wxString& typeName) {

			const enSpreadsheetFillType typeFill = GetTypeString(row, col);
			if (stringUtils::CompareString(typeName, s_strTypeTextOrString)) {
				if (typeFill == enSpreadsheetFillType::enSpreadsheetFillType_StrText || typeFill == enSpreadsheetFillType::enSpreadsheetFillType_StrTemplate) {
					CBackendLocalizationEntryArray array;
					CBackendLocalization::CreateLocalizationArray(m_data[row][col], array);
					wxString* s = new wxString;
					CBackendLocalization::GetRawLocText(array, *s);
					return s;
				}
				return new wxString(CBackendLocalization::CreateLocalizationRawLocText(m_data[row][col]));
			}
			else if (stringUtils::CompareString(typeName, s_strTypeTemplate)) {
				if (typeFill == enSpreadsheetFillType::enSpreadsheetFillType_StrText || typeFill == enSpreadsheetFillType::enSpreadsheetFillType_StrTemplate) {
					CBackendLocalizationEntryArray array;
					CBackendLocalization::CreateLocalizationArray(m_data[row][col], array);
					wxString* s = new wxString;
					CBackendLocalization::GetRawLocText(array, *s);
					return s;
				}
				return new wxString(CBackendLocalization::CreateLocalizationRawLocText(m_data[row][col]));
			}
			else if (stringUtils::CompareString(typeName, s_strTypeParameter)) {
				if (typeFill == enSpreadsheetFillType::enSpreadsheetFillType_StrText || typeFill == enSpreadsheetFillType::enSpreadsheetFillType_StrTemplate) {
					wxString* s = new wxString;
					CBackendLocalization::GetTranslateGetRawLocText(m_data[row][col], *s);
					return s;
				}
				return new wxString(m_data[row][col]);
			}

			return nullptr;
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

	private:

		enSpreadsheetFillType GetTypeString(int row, int col) const {
			auto iterator =
				m_setValue.find(std::make_pair(row, col));
			if (iterator != m_setValue.end())
				return iterator->second;
			return enSpreadsheetFillType_StrText;
		}

		std::map<std::pair<int, int>, enSpreadsheetFillType> m_setValue;
	};

	// the property of grid 
	class CPropertyGridEditorSpreadsheet :
		public IPropertyObject {
	public:

		CPropertyGridEditorSpreadsheet(CGridEditor* view) : m_view(view) {
			if (m_view != nullptr) {
				m_view->Bind(wxEVT_GRID_SELECT_CELL, &CPropertyGridEditorSpreadsheet::OnSelectCell, this);
				m_view->Bind(wxEVT_GRID_RANGE_SELECTED, &CPropertyGridEditorSpreadsheet::OnSelectCells, this);
			}
		}

		virtual ~CPropertyGridEditorSpreadsheet() {
			if (m_view != nullptr) m_view->DeletePendingEvents();
		}

		virtual bool IsEditable() const { return m_view->IsEditable(); }

		//system override 
		virtual wxString GetObjectTypeName() const override { return s_strPropertyClass; }
		virtual wxString GetClassName() const { return s_strPropertyClass; }

		/// Gets the metadata object
		virtual IMetaData* GetMetaData() const;

		/**
		* Property events
		*/
		virtual void OnPropertyCreated(IProperty* property);
		virtual void OnPropertyRefresh(class wxPropertyGridManager* pg, class wxPGProperty* pgProperty, IProperty* property);
		virtual void OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue);

		friend class CGridEditor;

	protected:

		void OnSelectCell(wxGridExtEvent& event);
		void OnSelectCells(wxGridExtRangeSelectEvent& event);

		CGridEditor* m_view;

		wxVector<wxGridExtBlockCoords> m_selection;

	private:

		void ShowInspector();

		void OnPropertyCreated(IProperty* property, const wxGridExtBlockCoords& coords);
		void OnPropertyChanged(IProperty* property, const wxGridExtBlockCoords& coords);

		const wxString s_strPropertyClass = wxT("propertySpreadsheet");

	private:

		CPropertyCategory* m_categoryGeneral = IPropertyObject::CreatePropertyCategory(wxT("General"), _("General"));
		CPropertyUString* m_propertyName = IPropertyObject::CreateProperty<CPropertyUString>(m_categoryGeneral, wxT("Name"), _("Name"), wxEmptyString);
		CPropertyTString* m_propertyText = IPropertyObject::CreateProperty<CPropertyTString>(m_categoryGeneral, wxT("Text"), _("Text"), wxEmptyString);
		CPropertyBoolean* m_propertyReadOnly = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryGeneral, wxT("ReadOnly"), _("Read only"), false);

		CPropertyCategory* m_categoryTemplate = IPropertyObject::CreatePropertyCategory(wxT("Template"), _("Template"));
		CPropertyEnum<CValueEnumSpreadsheetFillType>* m_propertyFillType = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumSpreadsheetFillType>>(m_categoryTemplate, wxT("FillType"), _("Fill type"), enSpreadsheetFillType::enSpreadsheetFillType_StrText);
		CPropertyUString* m_propertyParameter = IPropertyObject::CreateProperty<CPropertyUString>(m_categoryTemplate, wxT("Parameter"), _("Parameter"), wxEmptyString);

		CPropertyCategory* m_categoryAlignment = IPropertyObject::CreatePropertyCategory(wxT("Alignment"), _("Alignment"));
		CPropertyEnum<CValueEnumSpreadsheetFitMode>* m_propertyFitMode = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumSpreadsheetFitMode>>(m_categoryAlignment, wxT("Git_mode"), _("Fit mode"), enSpreadsheetFitMode::enFitMode_Overflow);
		CPropertyEnum<CValueEnumSpreadsheetHorizontalAlignment>* m_propertyAlignHorz = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumSpreadsheetHorizontalAlignment>>(m_categoryAlignment, wxT("Align_horz"), _("Horizontal"), enSpreadsheetAlignmentHorz::enAlignmentHorz_Left);
		CPropertyEnum<CValueEnumSpreadsheetVerticalAlignment>* m_propertyAlignVert = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumSpreadsheetVerticalAlignment>>(m_categoryAlignment, wxT("Align_vert"), _("Vertical"), enSpreadsheetAlignmentVert::enAlignmentVert_Center);
		CPropertyEnum<CValueEnumSpreadsheetOrient>* m_propertyOrient = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumSpreadsheetOrient>>(m_categoryAlignment, wxT("Orient_text"), _("Orientation text"), enSpreadsheetOrientation::enOrient_Vertical);

		CPropertyCategory* m_categoryAppearance = IPropertyObject::CreatePropertyCategory(wxT("Appearance"), _("Appearance"));
		CPropertyFont* m_propertyFont = IPropertyObject::CreateProperty<CPropertyFont>(m_categoryAppearance, wxT("Font"), _("Font"));
		CPropertyColour* m_propertyBackgroundColour = IPropertyObject::CreateProperty<CPropertyColour>(m_categoryAppearance, wxT("Background_colour"), _("Background colour"), wxNullColour);
		CPropertyColour* m_propertyTextColour = IPropertyObject::CreateProperty<CPropertyColour>(m_categoryAppearance, wxT("Text_colour"), _("Text colour"), wxNullColour);

		CPropertyCategory* m_categoryBorder = IPropertyObject::CreatePropertyCategory(wxT("Border"), _("Border"));
		CPropertyEnum<CValueEnumSpreadsheetBorder>* m_propertyLeftBorder = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumSpreadsheetBorder>>(m_categoryBorder, wxT("Left_border"), _("Left"), enSpreadsheetPenStyle::enPenStyle_Transparent);
		CPropertyEnum<CValueEnumSpreadsheetBorder>* m_propertyRightBorder = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumSpreadsheetBorder>>(m_categoryBorder, wxT("Right_border"), _("Right"), enSpreadsheetPenStyle::enPenStyle_Transparent);
		CPropertyEnum<CValueEnumSpreadsheetBorder>* m_propertyTopBorder = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumSpreadsheetBorder>>(m_categoryBorder, wxT("Top_border"), _("Top"), enSpreadsheetPenStyle::enPenStyle_Transparent);
		CPropertyEnum<CValueEnumSpreadsheetBorder>* m_propertyBottomBorder = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumSpreadsheetBorder>>(m_categoryBorder, wxT("Bottom_border"), _("Bottom"), enSpreadsheetPenStyle::enPenStyle_Transparent);
		CPropertyColour* m_propertyColourBorder = IPropertyObject::CreateProperty<CPropertyColour>(m_categoryBorder, wxT("Border_colour"), _("Colour"), wxNullColour);
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

	void OnGridRowSize(wxGridExtSizeEvent& event);
	void OnGridColSize(wxGridExtSizeEvent& event);
	void OnGridRowBrake(wxGridExtSizeEvent& event);
	void OnGridColBrake(wxGridExtSizeEvent& event);
	void OnGridRowArea(wxGridExtAreaEvent& event);
	void OnGridColArea(wxGridExtAreaEvent& event);
	void OnGridRowFreeze(wxGridExtSizeEvent& event);
	void OnGridColFreeze(wxGridExtSizeEvent& event);

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
	CPropertyGridEditorSpreadsheet* m_propertySpreadsheet;

	//grid doc
	wxObjectDataPtr<CBackendSpreadsheetObject> m_spreadsheetObject;
	wxSharedPtr<IBackendSpreadsheetNotifier> m_notifier;

	//grid enabled property? 
	bool m_enableProperty;

	wxDECLARE_EVENT_TABLE();
};


#endif 