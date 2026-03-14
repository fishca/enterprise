/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/dataview.h
// Purpose:     wxDataViewExtCtrl generic implementation header
// Author:      Robert Roebling
// Modified By: Bo Yang
// Copyright:   (c) 1998 Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef __GENERICDATAVIEWCTRLH__
#define __GENERICDATAVIEWCTRLH__

#include "wx/defs.h"
#include "wx/object.h"
#include "wx/compositewin.h"
#include "wx/control.h"
#include "wx/scrolwin.h"
#include "wx/icon.h"
#include "wx/vector.h"
#if wxUSE_ACCESSIBILITY
#include "wx/access.h"
#endif // wxUSE_ACCESSIBILITY

class wxDataViewExtMainWindow;
class wxDataViewExtHeaderWindow;
class wxDataViewExtFooterWindow;
#if wxUSE_ACCESSIBILITY
class wxDataViewExtCtrlAccessible;
#endif // wxUSE_ACCESSIBILITY

// ---------------------------------------------------------
// wxDataViewExtColumn
// ---------------------------------------------------------

class wxDataViewExtColumn : public wxDataViewExtColumnBase
{
	class wxDataViewExtFooterColumn : public wxSettableHeaderColumn
	{
	public:
		// ctors and dtor
		wxDataViewExtFooterColumn(wxDataViewExtColumn* owner)
			: m_owner(owner)
		{
			Init();
		}

		// implement base class pure virtuals
		virtual void SetTitle(const wxString& title) wxOVERRIDE
		{
			m_title = title;
			m_owner->UpdateWidth();
		}

		virtual wxString GetTitle() const wxOVERRIDE { return m_title; }

		virtual void SetBitmap(const wxBitmapBundle& bitmap) wxOVERRIDE { m_bitmap = bitmap; }
		wxBitmap GetBitmap() const wxOVERRIDE { wxFAIL_MSG("unreachable"); return wxNullBitmap; }
		wxBitmapBundle GetBitmapBundle() const wxOVERRIDE { return m_bitmap; }

		virtual void SetWidth(int width) wxOVERRIDE { m_owner->SetWidth(width); }
		virtual int GetWidth() const wxOVERRIDE { return m_owner->GetWidth(); }

		virtual void SetMinWidth(int minWidth) wxOVERRIDE { m_owner->SetMinWidth(minWidth); }
		virtual int GetMinWidth() const wxOVERRIDE { return m_owner->GetMinWidth(); }

		virtual void SetAlignment(wxAlignment align) wxOVERRIDE
		{
			m_align = align;
			m_owner->UpdateDisplay();
		}

		virtual wxAlignment GetAlignment() const wxOVERRIDE { return m_align; }

		virtual void SetFlags(int flags) wxOVERRIDE {}
		virtual int GetFlags() const wxOVERRIDE { return m_owner->GetFlags(); }

		virtual bool IsSortKey() const wxOVERRIDE { return false; }
		virtual void UnsetAsSortKey() wxOVERRIDE {}

		virtual void SetSortOrder(bool ascending) wxOVERRIDE {}
		virtual bool IsSortOrderAscending() const wxOVERRIDE { return false; }

	private:

		// common part of all ctors
		void Init()
		{
			m_align = wxAlignment::wxALIGN_LEFT;
		}

		wxDataViewExtColumn* m_owner;

		wxString m_title;
		wxBitmapBundle m_bitmap;
		wxAlignment m_align;
	};

public:

	wxDataViewExtColumn(const wxString& title,
		wxDataViewExtRenderer* renderer,
		unsigned int model_column,
		int width = wxDVC_DEFAULT_WIDTH,
		wxAlignment align = wxALIGN_CENTER,
		int flags = wxDATAVIEW_COL_RESIZABLE)
		: wxDataViewExtColumnBase(renderer, model_column),
		m_title(title), m_footer_column(this)
	{
		Init(width, align, flags);
	}

	wxDataViewExtColumn(const wxBitmapBundle& bitmap,
		wxDataViewExtRenderer* renderer,
		unsigned int model_column,
		int width = wxDVC_DEFAULT_WIDTH,
		wxAlignment align = wxALIGN_CENTER,
		int flags = wxDATAVIEW_COL_RESIZABLE)
		: wxDataViewExtColumnBase(bitmap, renderer, model_column), m_footer_column(this)
	{
		Init(width, align, flags);
	}

	// footer column data
	const wxSettableHeaderColumn& GetFooterColumn() const { return m_footer_column; }

	// implement wxHeaderColumnBase methods
	virtual void SetTitle(const wxString& title) wxOVERRIDE
	{
		m_title = title;
		UpdateWidth();
	}

	virtual wxString GetTitle() const wxOVERRIDE
	{
		return m_title;
	}

	virtual void SetWidth(int width) wxOVERRIDE
	{
		// Call the actual update method, used for both automatic and "manual"
		// width changes.
		WXUpdateWidth(width);

		// Do remember the last explicitly set width: this is used to prevent
		// UpdateColumnSizes() from resizing the last column to be smaller than
		// this size.
		m_manuallySetWidth = width;
	}
	virtual int GetWidth() const wxOVERRIDE;

	virtual void SetMinWidth(int minWidth) wxOVERRIDE
	{
		m_minWidth = minWidth;
		UpdateWidth();
	}
	virtual int GetMinWidth() const wxOVERRIDE
	{
		return m_minWidth;
	}

	virtual void SetAlignment(wxAlignment align) wxOVERRIDE
	{
		m_align = align;
		UpdateDisplay();
	}
	virtual wxAlignment GetAlignment() const wxOVERRIDE
	{
		return m_align;
	}

	virtual void SetFlags(int flags) wxOVERRIDE
	{
		m_flags = flags;
		UpdateDisplay();
	}
	virtual int GetFlags() const wxOVERRIDE
	{
		return m_flags;
	}

	virtual bool IsSortKey() const wxOVERRIDE
	{
		return m_sort;
	}

	virtual void UnsetAsSortKey() wxOVERRIDE;

	virtual void SetSortOrder(bool ascending) wxOVERRIDE;

	virtual bool IsSortOrderAscending() const wxOVERRIDE
	{
		return m_sortAscending;
	}

	virtual void SetBitmap(const wxBitmapBundle& bitmap) wxOVERRIDE
	{
		wxDataViewExtColumnBase::SetBitmap(bitmap);
		UpdateWidth();
	}

	// implement footer methods
	void SetFooterTitle(const wxString& title)
	{
		m_footer_column.SetTitle(title);
	}

	wxString GetFooterTitle() const
	{
		return m_footer_column.GetTitle();
	}

	void SetFooterBitmap(const wxBitmapBundle& bitmap)
	{
		m_footer_column.SetBitmap(bitmap);
	}

	wxBitmap GetFooterBitmap() const
	{
		return m_footer_column.GetBitmap();
	}

	wxBitmapBundle GetFooterBitmapBundle() const
	{
		return m_footer_column.GetBitmapBundle();
	}

	virtual void SetFooterAlignment(wxAlignment align)
	{
		m_footer_column.SetAlignment(align);
	}

	virtual wxAlignment GetFooterAlignment() const
	{
		return m_footer_column.GetAlignment();
	}

	// This method is specific to the generic implementation and is used only
	// by wxWidgets itself.
	void WXUpdateWidth(int width)
	{
		if (width == m_width)
			return;

		m_width = width;
		UpdateWidth();
	}

	// This method is also internal and called when the column is resized by
	// user interactively.
	void WXOnResize(int width);

	virtual int WXGetSpecifiedWidth() const wxOVERRIDE;

private:
	// common part of all ctors
	void Init(int width, wxAlignment align, int flags);

	// These methods forward to wxDataViewExtCtrl::OnColumnChange() and
	// OnColumnWidthChange() respectively, i.e. the latter is stronger than the
	// former.
	void UpdateDisplay();
	void UpdateWidth();

	// Return the effective value corresponding to the given width, handling
	// its negative values such as wxCOL_WIDTH_DEFAULT.
	int DoGetEffectiveWidth(int width) const;

	wxString m_title;
	int m_width,
		m_manuallySetWidth,
		m_minWidth;
	wxAlignment m_align;
	int m_flags;
	bool m_sort,
		m_sortAscending;

	wxDataViewExtFooterColumn m_footer_column;

	friend class wxDataViewExtHeaderWindowBase;
	friend class wxDataViewExtHeaderWindow;
	friend class wxDataViewExtHeaderWindowMSW;
};

// ---------------------------------------------------------
// wxDataViewExtCtrl
// ---------------------------------------------------------

class wxDataViewExtCtrl
	: public wxCompositeWindow<wxDataViewExtCtrlBase>,
	public wxScrollHelper
{
	friend class wxDataViewExtMainWindow;
	friend class wxDataViewExtHeaderWindowBase;
	friend class wxDataViewExtHeaderWindow;
	friend class wxDataViewExtHeaderWindowMSW;
	friend class wxDataViewExtColumn;
#if wxUSE_ACCESSIBILITY
	friend class wxDataViewExtCtrlAccessible;
#endif // wxUSE_ACCESSIBILITY

	typedef wxCompositeWindow<wxDataViewExtCtrlBase> BaseType;

public:
	wxDataViewExtCtrl() : wxScrollHelper(this)
	{
		Init();
	}

	wxDataViewExtCtrl(wxWindow* parent, wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize, long style = 0,
		const wxValidator& validator = wxDefaultValidator,
		const wxString& name = wxASCII_STR(wxDataViewExtCtrlNameStr))
		: wxScrollHelper(this)
	{
		Create(parent, id, pos, size, style, validator, name);
	}

	virtual ~wxDataViewExtCtrl();

	void Init();

	bool Create(wxWindow* parent, wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize, long style = 0,
		const wxValidator& validator = wxDefaultValidator,
		const wxString& name = wxASCII_STR(wxDataViewExtCtrlNameStr));

	virtual bool AssociateModel(wxDataViewExtModel* model) wxOVERRIDE;

	virtual bool AppendColumn(wxDataViewExtColumn* col) wxOVERRIDE;
	virtual bool PrependColumn(wxDataViewExtColumn* col) wxOVERRIDE;
	virtual bool InsertColumn(unsigned int pos, wxDataViewExtColumn* col) wxOVERRIDE;

	virtual void DoSetExpanderColumn() wxOVERRIDE;
	virtual void DoSetIndent() wxOVERRIDE;

	virtual unsigned int GetColumnCount() const wxOVERRIDE;
	virtual wxDataViewExtColumn* GetColumn(unsigned int pos) const wxOVERRIDE;
	virtual bool DeleteColumn(wxDataViewExtColumn* column) wxOVERRIDE;
	virtual bool ClearColumns() wxOVERRIDE;
	virtual int GetColumnPosition(const wxDataViewExtColumn* column) const wxOVERRIDE;

	virtual wxDataViewExtColumn* GetSortingColumn() const wxOVERRIDE;
	virtual wxVector<wxDataViewExtColumn*> GetSortingColumns() const wxOVERRIDE;

	virtual wxDataViewExtItem GetTopItem() const wxOVERRIDE;
	virtual int GetCountPerPage() const wxOVERRIDE;

	virtual int GetSelectedItemsCount() const wxOVERRIDE;
	virtual int GetSelections(wxDataViewExtItemArray& sel) const wxOVERRIDE;
	virtual void SetSelections(const wxDataViewExtItemArray& sel) wxOVERRIDE;
	virtual void Select(const wxDataViewExtItem& item) wxOVERRIDE;
	virtual void Unselect(const wxDataViewExtItem& item) wxOVERRIDE;
	virtual bool IsSelected(const wxDataViewExtItem& item) const wxOVERRIDE;

	virtual void SelectAll() wxOVERRIDE;
	virtual void UnselectAll() wxOVERRIDE;

	virtual void SetSelectionMode(wxDataViewExtSelectionMode selmode) wxOVERRIDE;
	virtual wxDataViewExtSelectionMode GetSelectionMode() const wxOVERRIDE;

	virtual void SetViewMode(wxDataViewExtViewMode viewMode) wxOVERRIDE;
	virtual wxDataViewExtViewMode GetViewMode() const wxOVERRIDE;

	virtual void SetTopParent(const wxDataViewExtItem& item) const wxOVERRIDE;

	virtual void EnsureVisible(const wxDataViewExtItem& item,
		const wxDataViewExtColumn* column = NULL) wxOVERRIDE;
	virtual void HitTest(const wxPoint& point, wxDataViewExtItem& item,
		wxDataViewExtColumn*& column) const wxOVERRIDE;
	virtual wxRect GetItemRect(const wxDataViewExtItem& item,
		const wxDataViewExtColumn* column = NULL) const wxOVERRIDE;

	virtual bool SetRowHeight(int rowHeight) wxOVERRIDE;

	virtual void Collapse(const wxDataViewExtItem& item) wxOVERRIDE;
	virtual bool IsExpanded(const wxDataViewExtItem& item) const wxOVERRIDE;

	virtual void SetFocus() wxOVERRIDE;

	virtual bool SetFont(const wxFont& font) wxOVERRIDE;
	virtual bool SetForegroundColour(const wxColour& colour) wxOVERRIDE;
	virtual bool SetBackgroundColour(const wxColour& colour) wxOVERRIDE;

#if wxUSE_ACCESSIBILITY
	virtual bool Show(bool show = true) wxOVERRIDE;
	virtual void SetName(const wxString& name) wxOVERRIDE;
	virtual bool Reparent(wxWindowBase* newParent) wxOVERRIDE;
#endif // wxUSE_ACCESSIBILITY
	virtual bool Enable(bool enable = true) wxOVERRIDE;

	virtual void ScrollWindow(int dx, int dy, const wxRect* rect) wxOVERRIDE;

	//show data filter
	virtual bool ShowFilter(struct CFilterRow& filter) { return false; }
	virtual bool ShowViewMode() { return false; }

	virtual bool AllowMultiColumnSort(bool allow) wxOVERRIDE;
	virtual bool IsMultiColumnSortAllowed() const wxOVERRIDE { return m_allowMultiColumnSort; }
	virtual void ToggleSortByColumn(int column) wxOVERRIDE;

#if wxUSE_DRAG_AND_DROP
	virtual bool EnableDragSource(const wxDataFormat& format) wxOVERRIDE;
	virtual bool DoEnableDropTarget(const wxVector<wxDataFormat>& formats) wxOVERRIDE;
#endif // wxUSE_DRAG_AND_DROP

	virtual wxBorder GetDefaultBorder() const wxOVERRIDE;

	virtual void EditItem(const wxDataViewExtItem& item, const wxDataViewExtColumn* column) wxOVERRIDE;

	virtual bool SetHeaderAttr(const wxItemAttr& attr) wxOVERRIDE;

	virtual bool SetAlternateRowColour(const wxColour& colour) wxOVERRIDE;

	// This method is specific to generic wxDataViewExtCtrl implementation and
	// should not be used in portable code.
	wxColour GetAlternateRowColour() const { return m_alternateRowColour; }

	// The returned pointer is null if the control has wxDV_NO_HEADER style.
	//
	// This method is only available in the generic versions.
	wxHeaderGenericCtrl* GenericGetHeader() const;

	void ShowHeaderWindow(bool e);
	void ShowFooterWindow(bool e);

protected:
	void EnsureVisibleRowCol(int row, int column);

	// Notice that row here may be invalid (i.e. >= GetRowCount()), this is not
	// an error and this function simply returns an invalid item in this case.
	wxDataViewExtItem GetItemByRow(unsigned int row) const;
	int GetRowByItem(const wxDataViewExtItem& item) const;

	// Mark the column as being used or not for sorting.
	void UseColumnForSorting(int idx);
	void DontUseColumnForSorting(int idx);

	// Return true if the given column is sorted
	bool IsColumnSorted(int idx) const;

	// Reset all columns currently used for sorting.
	void ResetAllSortColumns();

	virtual void DoEnableSystemTheme(bool enable, wxWindow* window) wxOVERRIDE;

	void OnDPIChanged(wxDPIChangedEvent& event);

public:     // utility functions not part of the API

	// returns the "best" width for the idx-th column
	unsigned int GetBestColumnWidth(int idx) const;

	// called by header window after reorder
	void ColumnMoved(wxDataViewExtColumn* col, unsigned int new_pos);

	// update the display after a change to an individual column
	void OnColumnChange(unsigned int idx);

	// update after the column width changes due to interactive resizing
	void OnColumnResized();

	// update after the column width changes because of e.g. title or bitmap
	// change, invalidates the column best width and calls OnColumnChange()
	void OnColumnWidthChange(unsigned int idx);

	// update after a change to the number of columns
	void OnColumnsCountChanged();

	wxWindow* GetMainWindow() { return (wxWindow*)m_tableAreaWin; }

	// return the index of the given column in m_cols
	int GetColumnIndex(const wxDataViewExtColumn* column) const;

	// Return the index of the column having the given model index.
	int GetModelColumnIndex(unsigned int model_column) const;

	// return the column displayed at the given position in the control
	wxDataViewExtColumn* GetColumnAt(unsigned int pos) const;

	virtual wxDataViewExtColumn* GetCurrentColumn() const wxOVERRIDE;

	virtual void OnInternalIdle() wxOVERRIDE;

#if wxUSE_ACCESSIBILITY
	virtual wxAccessible* CreateAccessible() wxOVERRIDE;
#endif // wxUSE_ACCESSIBILITY

private:
	// Implement pure virtual method inherited from wxCompositeWindow.
	virtual wxWindowList GetCompositeWindowParts() const wxOVERRIDE;

	virtual wxDataViewExtItem DoGetCurrentItem() const wxOVERRIDE;
	virtual void DoSetCurrentItem(const wxDataViewExtItem& item) wxOVERRIDE;

	virtual void DoExpand(const wxDataViewExtItem& item, bool expandChildren) wxOVERRIDE;

	void InvalidateColBestWidths();
	void InvalidateColBestWidth(int idx);
	void UpdateColWidths();

	void DoClearColumns();

	wxVector<wxDataViewExtColumn*> m_cols;
	// cached column best widths information, values are for
	// respective columns from m_cols and the arrays have same size
	struct CachedColWidthInfo
	{
		CachedColWidthInfo() : width(0), dirty(true) {}
		int width;  // cached width or 0 if not computed
		bool dirty; // column was invalidated, header needs updating
	};
	wxVector<CachedColWidthInfo> m_colsBestWidths;
	// This indicates that at least one entry in m_colsBestWidths has 'dirty'
	// flag set. It's cheaper to check one flag in OnInternalIdle() than to
	// iterate over m_colsBestWidths to check if anything needs to be done.
	bool                      m_colsDirty;

	wxDataViewExtModelNotifier* m_notifier;

	wxDataViewExtHeaderWindow* m_headerAreaWin;
	wxDataViewExtMainWindow* m_tableAreaWin;
	wxDataViewExtFooterWindow* m_footerAreaWin;

	// user defined color to draw row lines, may be invalid
	wxColour m_alternateRowColour;

	// columns indices used for sorting, empty if nothing is sorted
	wxVector<int> m_sortingColumnIdxs;

	// if true, allow sorting by more than one column
	bool m_allowMultiColumnSort;

private:
	void OnSize(wxSizeEvent& event);
	virtual wxSize GetSizeAvailableForScrollTarget(const wxSize& size) wxOVERRIDE;

	// we need to return a special WM_GETDLGCODE value to process just the
	// arrows but let the other navigation characters through
#ifdef __WXMSW__
	virtual WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam) wxOVERRIDE;
#endif // __WXMSW__

	WX_FORWARD_TO_SCROLL_HELPER()

private:
	wxDECLARE_DYNAMIC_CLASS(wxDataViewExtCtrl);
	wxDECLARE_NO_COPY_CLASS(wxDataViewExtCtrl);
	wxDECLARE_EVENT_TABLE();
};

#if wxUSE_ACCESSIBILITY
//-----------------------------------------------------------------------------
// wxDataViewExtCtrlAccessible
//-----------------------------------------------------------------------------

class wxDataViewExtCtrlAccessible : public wxWindowAccessible
{
public:
	wxDataViewExtCtrlAccessible(wxDataViewExtCtrl* win);
	virtual ~wxDataViewExtCtrlAccessible() {}

	virtual wxAccStatus HitTest(const wxPoint& pt, int* childId,
		wxAccessible** childObject) wxOVERRIDE;

	virtual wxAccStatus GetLocation(wxRect& rect, int elementId) wxOVERRIDE;

	virtual wxAccStatus Navigate(wxNavDir navDir, int fromId,
		int* toId, wxAccessible** toObject) wxOVERRIDE;

	virtual wxAccStatus GetName(int childId, wxString* name) wxOVERRIDE;

	virtual wxAccStatus GetChildCount(int* childCount) wxOVERRIDE;

	virtual wxAccStatus GetChild(int childId, wxAccessible** child) wxOVERRIDE;

	// wxWindowAccessible::GetParent() implementation is enough.
	// virtual wxAccStatus GetParent(wxAccessible** parent) wxOVERRIDE;

	virtual wxAccStatus DoDefaultAction(int childId) wxOVERRIDE;

	virtual wxAccStatus GetDefaultAction(int childId, wxString* actionName) wxOVERRIDE;

	virtual wxAccStatus GetDescription(int childId, wxString* description) wxOVERRIDE;

	virtual wxAccStatus GetHelpText(int childId, wxString* helpText) wxOVERRIDE;

	virtual wxAccStatus GetKeyboardShortcut(int childId, wxString* shortcut) wxOVERRIDE;

	virtual wxAccStatus GetRole(int childId, wxAccRole* role) wxOVERRIDE;

	virtual wxAccStatus GetState(int childId, long* state) wxOVERRIDE;

	virtual wxAccStatus GetValue(int childId, wxString* strValue) wxOVERRIDE;

	virtual wxAccStatus Select(int childId, wxAccSelectionFlags selectFlags) wxOVERRIDE;

	virtual wxAccStatus GetFocus(int* childId, wxAccessible** child) wxOVERRIDE;

	virtual wxAccStatus GetSelections(wxVariant* selections) wxOVERRIDE;
};
#endif // wxUSE_ACCESSIBILITY

#endif // __GENERICDATAVIEWCTRLH__
