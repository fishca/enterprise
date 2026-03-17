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

#include <wx/defs.h>
#include <wx/object.h>
#include <wx/compositewin.h>
#include <wx/control.h>
#include <wx/scrolwin.h>
#include <wx/selstore.h>
#include <wx/icon.h>
#include <wx/vector.h>
#if wxUSE_ACCESSIBILITY
#include <wx/access.h>
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
	void SetFooterTitle(const wxString& title) { m_footer_column.SetTitle(title); }
	wxString GetFooterTitle() const { return m_footer_column.GetTitle(); }

	void SetFooterBitmap(const wxBitmapBundle& bitmap) { m_footer_column.SetBitmap(bitmap); }
	wxBitmap GetFooterBitmap() const { return m_footer_column.GetBitmap(); }
	wxBitmapBundle GetFooterBitmapBundle() const { return m_footer_column.GetBitmapBundle(); }

	void SetFooterAlignment(wxAlignment align) { m_footer_column.SetAlignment(align); }
	wxAlignment GetFooterAlignment() const { return m_footer_column.GetAlignment(); }

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

namespace
{
	// Flags for Walker() function defined below.
	enum WalkFlags
	{
		Walk_All,               // Visit all items.
		Walk_ExpandedOnly       // Visit only expanded items.
	};

	// The column is either the index of the column to be used for sorting or one
	// of the special values in this enum:
	enum
	{
		// Don't sort at all.
		SortColumn_None = -2,

		// Sort using the model default sort order.
		SortColumn_Default = -1
	};

	// A class storing the definition of sort order used, as a column index and
	// sort direction by this column.
	//
	// Notice that the sort order may be invalid, meaning that items shouldn't be
	// sorted.
	class SortOrder
	{
	public:
		explicit SortOrder(int column = SortColumn_None, bool ascending = true)
			: m_column(column),
			m_ascending(ascending)
		{
		}

		// Default copy ctor, assignment operator and dtor are all OK.

		bool IsNone() const { return m_column == SortColumn_None; }

		bool UsesColumn() const { return m_column >= 0; }

		int GetColumn() const { return m_column; }
		bool IsAscending() const { return m_ascending; }

		bool operator==(const SortOrder& other) const
		{
			return m_column == other.m_column && m_ascending == other.m_ascending;
		}

		bool operator!=(const SortOrder& other) const
		{
			return !(*this == other);
		}

	private:
		int m_column;
		bool m_ascending;
	};

} // anonymous namespace

// ----------------------------------------------------------------------------
// wxDataViewExtTreeNodeViewMode
// ----------------------------------------------------------------------------

enum wxDataViewExtTreeNodeViewMode
{
	wxDataViewExtCell,
	wxDataViewExtHierarchy,
};

//-----------------------------------------------------------------------------
// wxDataViewExtTreeNode
//-----------------------------------------------------------------------------

class wxDataViewExtCtrl;
class wxDataViewExtTreeNode;

typedef wxVector<wxDataViewExtTreeNode*> wxDataViewExtTreeNodes;

// Note: this class is not used at all for virtual list models, so all code
// using it, i.e. any functions taking or returning objects of this type,
// including wxDataViewExtCtrl::m_root, can only be called after checking
// that we're using a non-"virtual list" model.

class wxDataViewExtTreeNode
{
public:

	wxDataViewExtTreeNode(wxDataViewExtTreeNode* parent, const wxDataViewExtItem& item)
		: m_parent(parent),
		m_item(item),
		m_branchData(NULL), m_viewMode(wxDataViewExtCell)
	{
	}

	wxDataViewExtTreeNode(wxDataViewExtTreeNode* parent, const wxDataViewExtItem& item, wxDataViewExtTreeNodeViewMode nodeType)
		: m_parent(parent),
		m_item(item),
		m_branchData(NULL), m_viewMode(nodeType)
	{
	}

	~wxDataViewExtTreeNode()
	{
		if (m_branchData)
		{
			wxDataViewExtTreeNodes& nodes = m_branchData->children;
			for (wxDataViewExtTreeNodes::iterator i = nodes.begin();
				i != nodes.end();
				++i)
			{
				delete* i;
			}

			delete m_branchData;
		}
	}

	wxDataViewExtTreeNodeViewMode GetViewMode() const
	{
		return m_viewMode;
	}

	static wxDataViewExtTreeNode* CreateRootNode()
	{
		wxDataViewExtTreeNode* n = new wxDataViewExtTreeNode(NULL, wxDataViewExtItem());
		n->m_branchData = new BranchNodeData;
		n->m_branchData->open = true;
		return n;
	}

	wxDataViewExtTreeNode* GetParent() const { return m_parent; }

	const wxDataViewExtTreeNodes& GetChildNodes() const
	{
		return m_branchData->children;
	}

	void InsertChild(wxDataViewExtCtrl* window,
		wxDataViewExtTreeNode* node, unsigned index);

	void RemoveChild(unsigned index)
	{
		wxCHECK_RET(m_branchData != NULL, "leaf node doesn't have children");
		m_branchData->RemoveChild(index);
	}

	// returns position of child node for given item in children list or wxNOT_FOUND
	int FindChildByItem(const wxDataViewExtItem& item) const
	{
		if (!m_branchData)
			return wxNOT_FOUND;

		const wxDataViewExtTreeNodes& nodes = m_branchData->children;
		const int len = nodes.size();
		for (int i = 0; i < len; i++)
		{
			if (nodes[i]->m_item == item)
				return i;
		}
		return wxNOT_FOUND;
	}

	const wxDataViewExtItem& GetItem() const { return m_item; }
	void SetItem(const wxDataViewExtItem& item) { m_item = item; }

	int GetIndentLevel() const
	{
		int ret = 0;
		const wxDataViewExtTreeNode* node = this;
		while (node->GetParent()->GetParent() != NULL)
		{
			node = node->GetParent();
			ret++;
		}
		return ret;
	}

	bool IsOpen() const
	{
		return m_branchData && m_branchData->open;
	}

	void ToggleOpen(wxDataViewExtCtrl* window)
	{
		// We do not allow the (invisible) root node to be collapsed because
		// there is no way to expand it again.
		if (!m_parent)
			return;

		wxCHECK_RET(m_branchData != NULL, "can't open leaf node");

		int sum = 0;

		const wxDataViewExtTreeNodes& nodes = m_branchData->children;
		const int len = nodes.size();
		for (int i = 0; i < len; i++)
			sum += 1 + nodes[i]->GetSubTreeCount();

		if (m_branchData->open)
		{
			ChangeSubTreeCount(-sum);
			m_branchData->open = !m_branchData->open;
		}
		else
		{
			m_branchData->open = !m_branchData->open;
			ChangeSubTreeCount(+sum);
			// Sort the children if needed
			Resort(window);
		}
	}

	// "HasChildren" property corresponds to model's IsContainer(). Note that it may be true
	// even if GetChildNodes() is empty; see below.
	bool HasChildren() const
	{
		return m_branchData != NULL;
	}

	void SetHasChildren(bool has)
	{
		// The invisible root item always has children, so ignore any attempts
		// to change this.
		if (!m_parent)
			return;

		if (!has)
		{
			wxDELETE(m_branchData);
		}
		else if (m_branchData == NULL)
		{
			m_branchData = new BranchNodeData;
		}
	}

	int GetSubTreeCount() const
	{
		return m_branchData ? m_branchData->subTreeCount : 0;
	}

	void ChangeSubTreeCount(int num)
	{
		wxASSERT(m_branchData != NULL);

		if (!m_branchData->open)
			return;

		m_branchData->subTreeCount += num;
		wxASSERT(m_branchData->subTreeCount >= 0);

		if (m_parent)
			m_parent->ChangeSubTreeCount(num);
	}

	void Resort(wxDataViewExtCtrl* window);

	// Should be called after changing the item value to update its position in
	// the control if necessary.
	void PutInSortOrder(wxDataViewExtCtrl* window)
	{
		if (m_parent)
			m_parent->PutChildInSortOrder(window, this);
	}

private:

	// Called by the child after it has been updated to put it in the right
	// place among its siblings, depending on the sort order.
	//
	// The argument must be non-null, but is passed as a pointer as it's
	// inserted into m_branchData->children.
	void PutChildInSortOrder(wxDataViewExtCtrl* window,
		wxDataViewExtTreeNode* childNode);


	wxDataViewExtTreeNode* m_parent;

	// Set node type 
	wxDataViewExtTreeNodeViewMode m_viewMode;

	// Corresponding model item.
	wxDataViewExtItem       m_item;

	// Data specific to non-leaf (branch, inner) nodes. They are kept in a
	// separate struct in order to conserve memory.
	struct BranchNodeData
	{
		BranchNodeData()
			: open(false),
			subTreeCount(0)
		{
		}

		void InsertChild(wxDataViewExtTreeNode* node, unsigned index)
		{
			children.insert(children.begin() + index, node);
		}

		void RemoveChild(unsigned index)
		{
			children.erase(children.begin() + index);
		}

		// Child nodes. Note that this may be empty even if m_hasChildren in
		// case this branch of the tree wasn't expanded and realized yet.
		wxDataViewExtTreeNodes  children;

		// Order in which children are sorted (possibly none).
		SortOrder            sortOrder;

		// Is the branch node currently open (expanded)?
		bool                 open;

		// Total count of expanded (i.e. visible with the help of some
		// scrolling) items in the subtree, but excluding this node. I.e. it is
		// 0 for leaves and is the number of rows the subtree occupies for
		// branch nodes.
		int                  subTreeCount;
	};

	BranchNodeData* m_branchData;
};

//-----------------------------------------------------------------------------
// wxDataViewExtRenameTimer
//-----------------------------------------------------------------------------

class wxDataViewExtRenameTimer : public wxTimer
{
public:
	wxDataViewExtRenameTimer(wxDataViewExtCtrl* owner);
	void Notify() wxOVERRIDE;
private:
	wxDataViewExtCtrl* m_owner;
};

// ---------------------------------------------------------
// wxDataViewExtCtrl
// ---------------------------------------------------------

class wxDataViewExtCtrl
	: public wxCompositeWindow<wxScrolled<wxDataViewExtCtrlBase>>
{
	friend class wxDataViewExtMainWindow;
	friend class wxDataViewExtHeaderWindowBase;
	friend class wxDataViewExtHeaderWindow;
	friend class wxDataViewExtHeaderWindowMSW;
	friend class wxDataViewExtColumn;

	friend class wxDataViewExtMaxWidthCalculator;

#if wxUSE_ACCESSIBILITY
	friend class wxDataViewExtCtrlAccessible;
#endif // wxUSE_ACCESSIBILITY

	typedef wxCompositeWindow<wxScrolled<wxDataViewExtCtrlBase>> BaseType;

public:
	wxDataViewExtCtrl()
	{
		Init();
	}

	wxDataViewExtCtrl(wxWindow* parent, wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize, long style = 0,
		const wxValidator& validator = wxDefaultValidator,
		const wxString& name = wxASCII_STR(wxDataViewExtCtrlNameStr))
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

	virtual void SetTopParent(const wxDataViewExtItem& item) wxOVERRIDE;

	virtual void EnsureVisible(const wxDataViewExtItem& item,
		const wxDataViewExtColumn* column = NULL) wxOVERRIDE;
	virtual void HitTest(const wxPoint& point, wxDataViewExtItem& item,
		wxDataViewExtColumn*& column) const wxOVERRIDE;
	virtual wxRect GetItemRect(const wxDataViewExtItem& item,
		const wxDataViewExtColumn* column = NULL) const wxOVERRIDE;

	virtual bool SetRowHeight(int rowHeight) wxOVERRIDE;
	virtual int GetRowHeight() const wxOVERRIDE;
	virtual int GetDefaultRowHeight() const wxOVERRIDE;

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
	virtual void Refresh(bool eraseb = true, const wxRect* rect = NULL) wxOVERRIDE;

	// this function is called when the current cell highlight must be redrawn
	// and may be overridden by the user
	virtual void DrawTableContent(wxDC& dc, wxDataViewExtMainWindow* tableWindow);

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

	bool FreezeTo(int row, int col);
	bool IsFrozen() const;

	void SetHeaderHeight(int point);
	int GetHeaderHeight() const;
	void SetFooterHeight(int point);
	int GetFooterHeight() const;

	void ShowHeaderWindow(bool e);
	void ShowFooterWindow(bool e);

	wxDataViewExtMainWindow* CellToDataViewWindow(const wxDataViewExtItem& item, const wxDataViewExtColumn* column) const;

	void    GetDataViewWindowOffset(const wxDataViewExtMainWindow* tableWindow, int& x, int& y) const;
	wxPoint GetDataViewWindowOffset(const wxDataViewExtMainWindow* tableWindow) const;

	wxDataViewExtMainWindow* DevicePosToDataViewWindow(wxPoint pos) const;
	wxDataViewExtMainWindow* DevicePosToDataViewWindow(int x, int y) const;

	void    CalcDataViewWindowUnscrolledPosition(int x, int y, int* xx, int* yy,
		const wxDataViewExtMainWindow* tableWindow) const;
	wxPoint CalcDataViewWindowUnscrolledPosition(const wxPoint& pt,
		const wxDataViewExtMainWindow* tableWindow) const;

	void    CalcDataViewWindowScrolledPosition(int x, int y, int* xx, int* yy,
		const wxDataViewExtMainWindow* tableWindow) const;
	wxPoint CalcDataViewWindowScrolledPosition(const wxPoint& pt,
		const wxDataViewExtMainWindow* tableWindow) const;

protected:

	void CalcWindowSizes();

	void ProcessTableCharEvent(
		wxKeyEvent& event,
		wxDataViewExtMainWindow* tableWin);

	void ProcessTableCharHookEvent(
		wxKeyEvent& event,
		wxDataViewExtMainWindow* tableWin);

	void ProcessTableCharLeftKeyEvent(wxKeyEvent& event);
	void ProcessTableCharRightKeyEvent(wxKeyEvent& event);

	void ProcessTableMouseEvent(
		wxMouseEvent& event,
		wxDataViewExtMainWindow* tableWin);

	void EnsureVisibleRowCol(int row, int column);

	// Notice that row here may be invalid (i.e. >= GetRowCount()), this is not
	// an error and this function simply returns an invalid item in this case.
	wxDataViewExtItem GetItemByRow(unsigned int row) const;
	int GetRowByItem(const wxDataViewExtItem& item,
		WalkFlags flags = Walk_All) const;

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

public:

	bool IsList() const { return GetModel()->IsListModel(); }
	bool IsVirtualList() const { return m_root == NULL; }

	// notifications from wxDataViewExtModel
	bool ItemAdded(const wxDataViewExtItem& parent, const wxDataViewExtItem& item);
	bool ItemDeleted(const wxDataViewExtItem& parent, const wxDataViewExtItem& item);
	bool ItemChanged(const wxDataViewExtItem& item)
	{
		return DoItemChanged(item, wxNOT_FOUND);
	}

	bool ValueChanged(const wxDataViewExtItem& item, unsigned int model_column);
	bool Cleared();
	void Resort();

	void ClearRowHeightCache();

	SortOrder GetSortOrder() const
	{
		wxDataViewExtColumn* const col = GetSortingColumn();
		if (col)
		{
			return SortOrder(col->GetModelColumn(),
				col->IsSortOrderAscending());
		}
		else
		{
			if (GetModel()->HasDefaultCompare())
				return SortOrder(SortColumn_Default);
			else
				return SortOrder();
		}
	}

#if wxUSE_DRAG_AND_DROP
	wxBitmap CreateItemBitmap(unsigned int row, int& indent);

#endif // wxUSE_DRAG_AND_DROP

	// Go to the specified row, i.e. make it current and change or extend the
	// selection extended depending on the modifier keys flags in the keyboard
	// state.
	//
	// The row must be valid.
	void GoToRow(const wxKeyboardState& state, unsigned int row);

	// Go to the item at position delta rows away (delta may be positive or
	// negative) from the current row.
	//
	// If adding delta would result in an invalid item, it's clamped to the
	// valid items range.
	void GoToRelativeRow(const wxKeyboardState& kbdState, int delta);

	void OnRenameTimer();

	void UpdateDisplay();
	void RecalculateDisplay();

	void ScrollTo(int rows, int column);

	unsigned GetCurrentRow() const { return m_currentRow; }
	bool HasCurrentRow() { return m_currentRow != (unsigned int)-1; }
	void ChangeCurrentRow(unsigned int row);
	bool TryAdvanceCurrentColumn(wxDataViewExtTreeNode* node, wxKeyEvent& event, bool forward);

	void ClearCurrentColumn() { m_currentCol = NULL; }

	bool IsSingleSel() const { return !HasFlag(wxDV_MULTIPLE); }
	bool IsEmpty() { return GetRowCount() == 0; }

	int GetEndOfLastCol() const;

	// Returns the position where the given column starts.
	// The column must be valid.
	int GetColumnStart(int column) const;

	unsigned int GetFirstVisibleRow() const;

	// I change this method to un const because in the tree view,
	// the displaying number of the tree are changing along with the
	// expanding/collapsing of the tree nodes
	unsigned int GetLastVisibleRow();
	unsigned int GetLastFullyVisibleRow();
	unsigned int GetRowCount() const;

	const wxSelectionStore& GetSelections() const { return m_selection; }
	void ClearSelection() { m_selection.SelectRange(0, GetRowCount() - 1, false); }
	void Select(const wxArrayInt& aSelections);

	// If a valid row is specified and it was previously selected, it is left
	// selected and the function returns false. Otherwise, i.e. if there is
	// really no selection left in the control, it returns true.
	bool UnselectAllRows(unsigned int except = (unsigned int)-1);

	void SelectRow(unsigned int row, bool on);
	void SelectRows(unsigned int from, unsigned int to);
	void ReverseRowSelection(unsigned int row);
	bool IsRowSelected(unsigned int row) const;
	void SendSelectionChangedEvent(const wxDataViewExtItem& item);

	void RefreshRow(unsigned int row) { RefreshRows(row, row); }
	void RefreshRows(unsigned int from, unsigned int to);
	void RefreshRowsAfter(unsigned int firstRow);

	// returns the colour to be used for drawing the rules
	wxColour GetRuleColour() const
	{
		return wxSystemSettings::GetColour(wxSYS_COLOUR_3DLIGHT);
	}

	wxRect GetLinesRect(unsigned int rowFrom, unsigned int rowTo) const;

	int GetLineStart(unsigned int row) const;  // row * m_lineHeight in fixed mode
	int GetLineHeight(unsigned int row) const; // m_lineHeight in fixed mode
	int GetLineAt(unsigned int y) const;       // y / m_lineHeight in fixed mode
	int QueryAndCacheLineHeight(unsigned int row, wxDataViewExtItem item) const;

	wxDataViewExtTreeNode* GetTreeNodeByRow(unsigned int row) const;

	// Methods for building the mapping tree
	void BuildTree(wxDataViewExtModel* model);
	void DestroyTree();
	void HitTest(const wxPoint& point, wxDataViewExtItem& item, wxDataViewExtColumn*& column);
	wxRect GetItemRect(const wxDataViewExtItem& item, const wxDataViewExtColumn* column);

	void Expand(unsigned int row, bool expandChildren = false);
	void Collapse(unsigned int row);
	bool IsExpanded(unsigned int row) const;
	bool HasChildren(unsigned int row) const;

#if wxUSE_DRAG_AND_DROP
	enum DropHint
	{
		DropHint_None = 0,
		DropHint_Inside,
		DropHint_Below,
		DropHint_Above
	};

	struct DropItemInfo
	{
		unsigned int        m_row;
		DropHint            m_hint;

		wxDataViewExtItem      m_item;
		int                 m_proposedDropIndex;
		int                 m_indentLevel;

		DropItemInfo()
			: m_row(static_cast<unsigned int>(-1))
			, m_hint(DropHint_None)
			, m_item(NULL)
			, m_proposedDropIndex(-1)
			, m_indentLevel(-1)
		{
		}
	};

	void RefreshDropHint();
	void RemoveDropHint();
	DropItemInfo GetDropItemInfo(const wxCoord x, const wxCoord y);
	wxDragResult OnDragOver(wxDataFormat format, wxCoord x, wxCoord y, wxDragResult def);
	bool OnDrop(wxDataFormat format, wxCoord x, wxCoord y);
	wxDragResult OnData(wxDataFormat format, wxCoord x, wxCoord y, wxDragResult def);
	void OnLeave();
#endif // wxUSE_DRAG_AND_DROP

	// Adjust last column to window size
	void UpdateColumnSizes();

	// Called by wxDataViewExtCtrl and our own OnRenameTimer() to start edit the
	// specified item in the given column.
	void StartEditing(const wxDataViewExtItem& item, const wxDataViewExtColumn* col);
	void FinishEditing();
	bool HasEditableColumn(const wxDataViewExtItem& item) const
	{
		return FindColumnForEditing(item, wxDATAVIEW_CELL_EDITABLE) != NULL;
	}

private:

	// depending on the values of m_numFrozenRows and m_numFrozenCols, it will
	// create and initialize or delete the frozen windows
	void InitializeFrozenWindows();

	void InvalidateCount() { m_countRows = -1; }
	void UpdateCount(int count)
	{
		m_countRows = count;
		m_selection.SetItemCount(count);
	}

	int RecalculateCount() const;

	// Return false only if the event was vetoed by its handler.
	bool SendExpanderEvent(wxEventType type, const wxDataViewExtItem& item);

	struct FindNodeResult
	{
		wxDataViewExtTreeNode* m_node;
		bool                 m_subtreeRealized;
	};

	FindNodeResult FindNode(const wxDataViewExtItem& item);

	wxDataViewExtColumn* FindColumnForEditing(const wxDataViewExtItem& item, wxDataViewExtCellMode mode) const;

	bool IsCellEditableInMode(const wxDataViewExtItem& item, const wxDataViewExtColumn* col, wxDataViewExtCellMode mode) const;

	void DrawCellBackground(wxDataViewExtRenderer* cell, wxDC& dc, const wxRect& rect, wxDataViewExtTreeNodeViewMode mode);

	// Common part of {Item,Value}Changed(): if view_column is wxNOT_FOUND,
	// assumes that all columns were modified, otherwise just this one.
	bool DoItemChanged(const wxDataViewExtItem& item, int view_column);

	// Return whether the item has at most one column with a value.
	bool IsItemSingleValued(const wxDataViewExtItem& item) const
	{
		bool hadColumnWithValue = false;
		const unsigned int cols = GetColumnCount();
		const wxDataViewExtModel* const model = GetModel();
		for (unsigned int i = 0; i < cols; i++)
		{
			if (model->HasValue(item, i))
			{
				if (hadColumnWithValue)
					return false;
				hadColumnWithValue = true;
			}
		}

		return true;
	}

	// Find the first column with a value in it.
	wxDataViewExtColumn* FindFirstColumnWithValue(const wxDataViewExtItem& item) const
	{
		const unsigned int cols = GetColumnCount();
		const wxDataViewExtModel* const model = GetModel();
		for (unsigned int i = 0; i < cols; i++)
		{
			if (model->HasValue(item, i))
				return GetColumnAt(i);
		}

		return NULL;
	}

	// Helper of public Expand(), must be called with a valid node.
	void DoExpand(wxDataViewExtTreeNode* node, unsigned int row, bool expandChildren);

private:

	int                         m_lineHeight;
	bool                        m_dirty;

	wxDataViewExtColumn* m_currentCol;
	unsigned int                m_currentRow;
	wxSelectionStore            m_selection;
	wxDataViewExtSelectionMode	m_selectionMode;

	wxDataViewExtViewMode		m_viewMode;

	wxDataViewExtRenameTimer* m_renameTimer;
	bool                        m_lastOnSame;

	bool                        m_hasFocus;
	bool                        m_useCellFocus;
	bool                        m_currentColSetByKeyboard;

	class HeightCache* m_rowHeightCache;

#if wxUSE_DRAG_AND_DROP
	int                         m_dragCount;
	wxPoint                     m_dragStart;

	bool                        m_dragEnabled;
	wxDataFormat                m_dragFormat;

	DropItemInfo                m_dropItemInfo;
#endif // wxUSE_DRAG_AND_DROP

	// for double click logic
	unsigned int m_lineLastClicked,
		m_lineBeforeLastClicked,
		m_lineSelectSingleOnUp;

	// the pen used to draw horiz/vertical rules
	wxPen m_penRule;

	// This is the tree structure of the model
	wxDataViewExtTreeNode* m_root;

	int m_countRows;

	// Number of frozen rows/columns in the beginning of the grid, 0 if none.
	int m_countFrozenRows;
	int m_countFrozenCols;

	int m_countFrozenHierarchicalRows;

	// This is the tree node under the cursor
	wxDataViewExtTreeNode* m_underMouse;

	// The control used for editing or NULL.
	wxWeakRef<wxWindow> m_editorCtrl;

	// Id m_editorCtrl is non-NULL, pointer to the associated renderer.
	wxDataViewExtRenderer* m_editorRenderer;

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
	wxDataViewExtMainWindow* m_tableFrozenRowAreaWin;
	wxDataViewExtMainWindow* m_tableFrozenColAreaWin;
	wxDataViewExtMainWindow* m_tableFrozenCornerAreaWin;
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
