/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/datavgen.cpp
// Purpose:     wxDataViewExtCtrl generic implementation
// Author:      Robert Roebling
// Modified by: Francesco Montorsi, Guru Kathiresan, Bo Yang
// Copyright:   (c) 1998 Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#if wxUSE_DATAVIEWCTRL

#include "dataview.h"

#ifndef WX_PRECOMP
#ifdef __WXMSW__
#include <wx/app.h>          // GetRegisteredClassName()
#include <wx/msw/private.h>
#include <wx/msw/wrapwin.h>
#include <wx/msw/wrapcctl.h> // include <commctrl.h> "properly"
#endif
#include <wx/sizer.h>
#include <wx/log.h>
#include <wx/dcclient.h>
#include <wx/timer.h>
#include <wx/settings.h>
#include <wx/msgdlg.h>
#include <wx/dcscreen.h>
#include <wx/frame.h>
#include <wx/vector.h>
#endif

#include <wx/stockitem.h>
#include <wx/popupwin.h>
#include <wx/renderer.h>
#include <wx/dcbuffer.h>
#include <wx/icon.h>
#include <wx/itemattr.h>
#include <wx/list.h>
#include <wx/listimpl.cpp>
#include <wx/imaglist.h>
#include <wx/dnd.h>
#include <wx/selstore.h>
#include <wx/stopwatch.h>
#include <wx/weakref.h>

#include <wx/generic/private/markuptext.h>
#include <wx/generic/private/rowheightcache.h>
#include <wx/generic/private/widthcalc.h>

#if wxUSE_ACCESSIBILITY
#include <wx/private/markupparser.h>
#endif // wxUSE_ACCESSIBILITY

//-----------------------------------------------------------------------------
// classes
//-----------------------------------------------------------------------------

class wxDataViewExtColumn;
class wxDataViewExtHeaderWindow;
class wxDataViewExtCtrl;

//-----------------------------------------------------------------------------
// constants
//-----------------------------------------------------------------------------

// the cell padding on the left/right
static const int PADDING_RIGHTLEFT = 3;

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

	// ----------------------------------------------------------------------------
	// helper functions
	// ----------------------------------------------------------------------------

	// Return the expander column or, if it is not set, the first column and also
	// set it as the expander one for the future.
	wxDataViewExtColumn* GetExpanderColumnOrFirstOne(wxDataViewExtCtrl* dataview)
	{
		wxDataViewExtColumn* expander = dataview->GetExpanderColumn();
		if (!expander)
		{
			// TODO-RTL: last column for RTL support
			expander = dataview->GetColumnAt(0);
			dataview->SetExpanderColumn(expander);
		}

		return expander;
	}

	wxTextCtrl* CreateEditorTextCtrl(wxWindow* parent, const wxRect& labelRect, const wxString& value)
	{
		wxTextCtrl* ctrl = new wxTextCtrl(parent, wxID_ANY, value,
			labelRect.GetPosition(),
			labelRect.GetSize(),
			wxTE_PROCESS_ENTER);

		// Adjust size of wxTextCtrl editor to fit text, even if it means being
		// wider than the corresponding column (this is how Explorer behaves).
		const int fitting = ctrl->GetSizeFromTextSize(ctrl->GetTextExtent(ctrl->GetValue())).x;
		const int current = ctrl->GetSize().x;
		const int maxwidth = ctrl->GetParent()->GetSize().x - ctrl->GetPosition().x;

		// Adjust size so that it fits all content. Don't change anything if the
		// allocated space is already larger than needed and don't extend wxDVC's
		// boundaries.
		int width = wxMin(wxMax(current, fitting), maxwidth);

		if (width != current)
			ctrl->SetSize(wxSize(width, -1));

		// select the text in the control an place the cursor at the end
		ctrl->SetInsertionPointEnd();
		ctrl->SelectAll();

		return ctrl;
	}

} // anonymous namespace

//-----------------------------------------------------------------------------
// wxDataViewExtColumn
//-----------------------------------------------------------------------------

void wxDataViewExtColumn::Init(int width, wxAlignment align, int flags)
{
	m_width =
		m_manuallySetWidth = width;
	m_minWidth = 0;
	m_align = align;
	m_flags = flags;
	m_sort = false;
	m_sortAscending = true;
}

int wxDataViewExtColumn::DoGetEffectiveWidth(int width) const
{
	switch (width)
	{
	case wxCOL_WIDTH_DEFAULT:
		return wxWindow::FromDIP(wxDVC_DEFAULT_WIDTH, m_owner);

	case wxCOL_WIDTH_AUTOSIZE:
		wxCHECK_MSG(m_owner, wxDVC_DEFAULT_WIDTH, "no owner control");
		return m_owner->GetBestColumnWidth(m_owner->GetColumnIndex(this));

	default:
		return width;
	}
}

int wxDataViewExtColumn::GetWidth() const
{
	return DoGetEffectiveWidth(m_width);
}

void wxDataViewExtColumn::WXOnResize(int width)
{
	m_width =
		m_manuallySetWidth = width;

	int idx = m_owner->GetColumnIndex(this);

	m_owner->InvalidateColBestWidth(idx);
	m_owner->OnColumnResized();
}

int wxDataViewExtColumn::WXGetSpecifiedWidth() const
{
	// Note that we need to return valid value even if no width was initially
	// specified, as otherwise the last column created without any explicit
	// width could be reduced to nothing by UpdateColumnSizes() when the
	// control is shrunk.
	return DoGetEffectiveWidth(m_manuallySetWidth);
}

void wxDataViewExtColumn::UpdateDisplay()
{
	if (m_owner)
	{
		int idx = m_owner->GetColumnIndex(this);
		m_owner->OnColumnChange(idx);
	}
}

void wxDataViewExtColumn::UpdateWidth()
{
	if (m_owner)
	{
		int idx = m_owner->GetColumnIndex(this);
		m_owner->OnColumnWidthChange(idx);
	}
}

void wxDataViewExtColumn::UnsetAsSortKey()
{
	m_sort = false;

	if (m_owner)
		m_owner->DontUseColumnForSorting(m_owner->GetColumnIndex(this));

	UpdateDisplay();
}

void wxDataViewExtColumn::SetSortOrder(bool ascending)
{
	if (!m_owner)
		return;

	const int idx = m_owner->GetColumnIndex(this);

	// If this column isn't sorted already, mark it as sorted
	if (!m_sort)
	{
		wxASSERT(!m_owner->IsColumnSorted(idx));

		// Now set this one as the new sort column.
		m_owner->UseColumnForSorting(idx);
		m_sort = true;
	}

	m_sortAscending = ascending;

	// Call this directly instead of using UpdateDisplay() as we already have
	// the column index, no need to look it up again.
	m_owner->OnColumnChange(idx);
}

//-----------------------------------------------------------------------------
// wxDataViewExtHeaderWindow
//-----------------------------------------------------------------------------

class wxDataViewExtHeaderWindow : public wxHeaderGenericCtrl
{
public:
	wxDataViewExtHeaderWindow(wxDataViewExtCtrl* parent)
		: wxHeaderGenericCtrl(parent, wxID_ANY,
			wxDefaultPosition, wxDefaultSize,
			wxHD_DEFAULT_STYLE | wxHD_BITMAP_ON_RIGHT)
	{
	}

	wxDataViewExtCtrl* GetOwner() const
	{
		return static_cast<wxDataViewExtCtrl*>(GetParent());
	}

	virtual wxWindow* GetMainWindowOfCompositeControl() wxOVERRIDE
	{
		return GetOwner();
	}

	// Add/Remove additional column to sorting columns
	void ToggleSortByColumn(int column)
	{
		wxDataViewExtCtrl* const owner = GetOwner();

		if (!owner->IsMultiColumnSortAllowed())
			return;

		wxDataViewExtColumn* const col = owner->GetColumn(column);
		if (!col->IsSortable())
			return;

		if (owner->IsColumnSorted(column))
		{
			col->UnsetAsSortKey();
			SendEvent(wxEVT_DATAVIEW_COLUMN_SORTED, column);
		}
		else // Do start sortign by it.
		{
			col->SetSortOrder(true);
			SendEvent(wxEVT_DATAVIEW_COLUMN_SORTED, column);
		}
	}

#if wxUSE_ACCESSIBILITY
	virtual wxAccessible* CreateAccessible() wxOVERRIDE
	{
		// Under MSW wxHeadrCtrl is a native control
		// so we just need to pass all requests
		// to the accessibility framework.
		return new wxAccessible(this);
	}
#endif // wxUSE_ACCESSIBILITY

protected:
	// implement/override wxHeaderGenericCtrl functions by forwarding them to the main
	// control
	virtual const wxHeaderColumn& GetColumn(unsigned int idx) const wxOVERRIDE
	{
		return *(GetOwner()->GetColumn(idx));
	}

	virtual bool UpdateColumnWidthToFit(unsigned int idx, int widthTitle) wxOVERRIDE
	{
		wxDataViewExtCtrl* const owner = GetOwner();

		int widthContents = owner->GetBestColumnWidth(idx);
		owner->GetColumn(idx)->SetWidth(wxMax(widthTitle, widthContents));
		owner->OnColumnChange(idx);

		return true;
	}

private:

	void FinishEditing();

	bool SendEvent(wxEventType type, unsigned int n)
	{
		wxDataViewExtCtrl* const owner = GetOwner();
		wxDataViewExtEvent event(type, owner, owner->GetColumn(n));

		// for events created by wxDataViewExtHeaderWindow the
		// row / value fields are not valid
		return owner->ProcessWindowEvent(event);
	}

	void OnClick(wxHeaderGenericCtrlEvent& event)
	{
		FinishEditing();

		const unsigned idx = event.GetColumn();

		if (SendEvent(wxEVT_DATAVIEW_COLUMN_HEADER_CLICK, idx))
			return;

		// default handling for the column click is to sort by this column or
		// toggle its sort order
		wxDataViewExtCtrl* const owner = GetOwner();
		wxDataViewExtColumn* const col = owner->GetColumn(idx);
		if (!col->IsSortable())
		{
			// no default handling for non-sortable columns
			event.Skip();
			return;
		}

		if (col->IsSortKey())
		{
			// already using this column for sorting, just change the order
			col->ToggleSortOrder();
		}
		else // not using this column for sorting yet
		{
			// We will sort by this column only now, so reset all the
			// previously used ones.
			owner->ResetAllSortColumns();

			// Sort the column
			col->SetSortOrder(true);
		}

		wxDataViewExtModel* const model = owner->GetModel();
		if (model)
			model->Resort();

		owner->OnColumnChange(idx);

		SendEvent(wxEVT_DATAVIEW_COLUMN_SORTED, idx);
	}

	void OnRClick(wxHeaderGenericCtrlEvent& event)
	{
		// Event wasn't processed somewhere, use default behaviour
		if (!SendEvent(wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK,
			event.GetColumn()))
		{
			event.Skip();
			ToggleSortByColumn(event.GetColumn());
		}
	}

	void OnResize(wxHeaderGenericCtrlEvent& event)
	{
		FinishEditing();

		wxDataViewExtCtrl* const owner = GetOwner();

		const unsigned col = event.GetColumn();
		owner->GetColumn(col)->WXOnResize(event.GetWidth());
	}

	void OnEndReorder(wxHeaderGenericCtrlEvent& event)
	{
		FinishEditing();

		wxDataViewExtCtrl* const owner = GetOwner();
		owner->ColumnMoved(owner->GetColumn(event.GetColumn()),
			event.GetNewOrder());
	}

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_NO_COPY_CLASS(wxDataViewExtHeaderWindow);
};

wxBEGIN_EVENT_TABLE(wxDataViewExtHeaderWindow, wxHeaderGenericCtrl)
EVT_HEADER_CLICK(wxID_ANY, wxDataViewExtHeaderWindow::OnClick)
EVT_HEADER_RIGHT_CLICK(wxID_ANY, wxDataViewExtHeaderWindow::OnRClick)

EVT_HEADER_RESIZING(wxID_ANY, wxDataViewExtHeaderWindow::OnResize)
EVT_HEADER_END_RESIZE(wxID_ANY, wxDataViewExtHeaderWindow::OnResize)

EVT_HEADER_END_REORDER(wxID_ANY, wxDataViewExtHeaderWindow::OnEndReorder)
wxEND_EVENT_TABLE()

//-----------------------------------------------------------------------------
// wxDataViewExtFooterWindow
//-----------------------------------------------------------------------------

class wxDataViewExtFooterWindow : public wxHeaderGenericCtrl
{
public:

	wxDataViewExtFooterWindow(wxDataViewExtCtrl* parent)
		: wxHeaderGenericCtrl(parent, wxID_ANY,
			wxDefaultPosition, wxDefaultSize,
			wxHD_BITMAP_ON_RIGHT)
	{
	}

	wxDataViewExtCtrl* GetOwner() const
	{
		return static_cast<wxDataViewExtCtrl*>(GetParent());
	}

	virtual wxWindow* GetMainWindowOfCompositeControl() wxOVERRIDE
	{
		return GetOwner();
	}

#if wxUSE_ACCESSIBILITY
	virtual wxAccessible* CreateAccessible() wxOVERRIDE
	{
		// Under MSW wxHeadrCtrl is a native control
		// so we just need to pass all requests
		// to the accessibility framework.
		return new wxAccessible(this);
	}
#endif // wxUSE_ACCESSIBILITY

protected:
	// implement/override wxHeaderGenericCtrl functions by forwarding them to the main
	// control
	virtual const wxHeaderColumn& GetColumn(unsigned int idx) const wxOVERRIDE
	{
		return GetOwner()->GetColumn(idx)->GetFooterColumn();
	}

	virtual bool UpdateColumnWidthToFit(unsigned int idx, int widthTitle) wxOVERRIDE
	{
		wxDataViewExtCtrl* const owner = GetOwner();

		int widthContents = owner->GetBestColumnWidth(idx);
		owner->GetColumn(idx)->SetWidth(wxMax(widthTitle, widthContents));
		owner->OnColumnChange(idx);

		return true;
	}

	void OnResize(wxHeaderGenericCtrlEvent& event)
	{
		wxDataViewExtCtrl* const owner = GetOwner();

		const unsigned col = event.GetColumn();
		owner->GetColumn(col)->WXOnResize(event.GetWidth());
	}

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_NO_COPY_CLASS(wxDataViewExtFooterWindow);
};

wxBEGIN_EVENT_TABLE(wxDataViewExtFooterWindow, wxHeaderGenericCtrl)
EVT_HEADER_RESIZING(wxID_ANY, wxDataViewExtFooterWindow::OnResize)
EVT_HEADER_END_RESIZE(wxID_ANY, wxDataViewExtFooterWindow::OnResize)
wxEND_EVENT_TABLE()

//-----------------------------------------------------------------------------
// wxDataViewExtRenameTimer
//-----------------------------------------------------------------------------

class wxDataViewExtRenameTimer : public wxTimer
{
private:
	wxDataViewExtMainWindow* m_owner;

public:
	wxDataViewExtRenameTimer(wxDataViewExtMainWindow* owner);
	void Notify() wxOVERRIDE;
};

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

class wxDataViewExtMainWindow;
class wxDataViewExtTreeNode;

typedef wxVector<wxDataViewExtTreeNode*> wxDataViewExtTreeNodes;

// Note: this class is not used at all for virtual list models, so all code
// using it, i.e. any functions taking or returning objects of this type,
// including wxDataViewExtMainWindow::m_root, can only be called after checking
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

	void InsertChild(wxDataViewExtMainWindow* window,
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

	void ToggleOpen(wxDataViewExtMainWindow* window)
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

	void Resort(wxDataViewExtMainWindow* window);

	// Should be called after changing the item value to update its position in
	// the control if necessary.
	void PutInSortOrder(wxDataViewExtMainWindow* window)
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
	void PutChildInSortOrder(wxDataViewExtMainWindow* window,
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
// wxDataViewExtMainWindow
//-----------------------------------------------------------------------------

class wxDataViewExtMainWindow : public wxWindow
{
public:
	wxDataViewExtMainWindow(wxDataViewExtCtrl* parent,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		const wxString& name = wxT("wxdataviewctrlmainwindow"));
	virtual ~wxDataViewExtMainWindow();

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
	void Resort()
	{
		ClearRowHeightCache();

		if (!IsVirtualList())
		{
			m_root->Resort(this);
		}
		UpdateDisplay();
	}
	void ClearRowHeightCache()
	{
		if (m_rowHeightCache)
			m_rowHeightCache->Clear();
	}

	SortOrder GetSortOrder() const
	{
		wxDataViewExtColumn* const col = GetOwner()->GetSortingColumn();
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

	void SetOwner(wxDataViewExtCtrl* owner) { m_owner = owner; }
	wxDataViewExtCtrl* GetOwner() { return m_owner; }
	const wxDataViewExtCtrl* GetOwner() const { return m_owner; }

	wxDataViewExtModel* GetModel() { return GetOwner()->GetModel(); }
	const wxDataViewExtModel* GetModel() const { return GetOwner()->GetModel(); }

	virtual wxWindow* GetMainWindowOfCompositeControl() wxOVERRIDE
	{
		return GetOwner();
	}

#if wxUSE_DRAG_AND_DROP
	wxBitmap CreateItemBitmap(unsigned int row, int& indent);
#endif // wxUSE_DRAG_AND_DROP
	void OnPaint(wxPaintEvent& event);
	void OnCharHook(wxKeyEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnLeftKey(wxKeyEvent& event);
	void OnRightKey(wxKeyEvent& event);
	void OnMouse(wxMouseEvent& event);
	void OnSetFocus(wxFocusEvent& event);
	void OnKillFocus(wxFocusEvent& event);

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

	void UpdateDisplay();
	void RecalculateDisplay();
	void OnInternalIdle() wxOVERRIDE;

	void OnRenameTimer();

	void ScrollWindow(int dx, int dy, const wxRect* rect = NULL) wxOVERRIDE;
	void ScrollTo(int rows, int column);

	unsigned GetCurrentRow() const { return m_currentRow; }
	bool HasCurrentRow() { return m_currentRow != (unsigned int)-1; }
	void ChangeCurrentRow(unsigned int row);
	bool TryAdvanceCurrentColumn(wxDataViewExtTreeNode* node, wxKeyEvent& event, bool forward);

	wxDataViewExtColumn* GetCurrentColumn() const { return m_currentCol; }
	void ClearCurrentColumn() { m_currentCol = NULL; }

	bool IsSingleSel() const { return !GetParent()->HasFlag(wxDV_MULTIPLE); }
	bool IsEmpty() { return GetRowCount() == 0; }

	int GetCountPerPage() const;
	int GetEndOfLastCol() const;

	// Returns the position where the given column starts.
	// The column must be valid.
	int GetColumnStart(int column) const;

	unsigned int GetFirstVisibleRow() const;
	wxDataViewExtItem GetTopItem() const;

	// I change this method to un const because in the tree view,
	// the displaying number of the tree are changing along with the
	// expanding/collapsing of the tree nodes
	unsigned int GetLastVisibleRow();
	unsigned int GetLastFullyVisibleRow();
	unsigned int GetRowCount() const;

	const wxSelectionStore& GetSelections() const { return m_selection; }
	void ClearSelection() { m_selection.SelectRange(0, GetRowCount() - 1, false); }
	void Select(const wxArrayInt& aSelections);

	void SelectAllRows()
	{
		m_selection.SelectRange(0, GetRowCount() - 1);
		Refresh();
	}

	// If a valid row is specified and it was previously selected, it is left
	// selected and the function returns false. Otherwise, i.e. if there is
	// really no selection left in the control, it returns true.
	bool UnselectAllRows(unsigned int except = (unsigned int)-1);

	void SelectRow(unsigned int row, bool on);
	void SelectRows(unsigned int from, unsigned int to);
	void ReverseRowSelection(unsigned int row);
	bool IsRowSelected(unsigned int row);
	void SendSelectionChangedEvent(const wxDataViewExtItem& item);

	void SetSelectionMode(wxDataViewExtSelectionMode selmode)
	{
		if (m_selectionMode != selmode)
		{
			m_selectionMode = selmode;

			UpdateDisplay();
		}
	}

	wxDataViewExtSelectionMode GetSelectionMode() const
	{
		return m_selectionMode;
	}

	void SetViewMode(wxDataViewExtViewMode viewMode)
	{
		if (m_viewMode != viewMode)
		{
			m_viewMode = viewMode;

			const wxDataViewExtItem& selection =
				GetItemByRow(m_currentRow);

			if (m_viewMode == wxDataViewExtViewMode::wxDataViewExtHierarchical) {

				wxDataViewExtModel* model = GetModel();

				if (model != NULL)
				{
					const wxDataViewExtItem& item =
						model->GetParent(selection);

					if (item == model->GetParentTopItem())
					{
						model->SetParentTopItem(
							model->GetParent(selection));
					}
					else if (model->IsContainer(item))
					{
						model->SetParentTopItem(item);
					}
				}
			}

			Cleared();

			if (m_viewMode == wxDataViewExtViewMode::wxDataViewExtTree)
				GetOwner()->ExpandAncestors(selection);

			int current = GetRowByItem(selection);

			if (current != wxNOT_FOUND && (IsSingleSel() || !IsRowSelected(current)))
			{
				ChangeCurrentRow(current);
				if (UnselectAllRows(current))
				{
					SelectRow(m_currentRow, true);
					SendSelectionChangedEvent(GetItemByRow(m_currentRow));
				}
			}
			else if (current != wxNOT_FOUND) // multi sel & current is highlighted & no mod keys
			{
				m_lineSelectSingleOnUp = current;
				ChangeCurrentRow(current); // change focus
			}

			// Send the event to wxDataViewExtCtrl itself.
			wxDataViewExtEvent cache_event(wxEVT_DATAVIEW_VIEW_SET, m_owner, selection);
			cache_event.SetViewMode(viewMode);
			m_owner->ProcessWindowEvent(cache_event);

			GetOwner()->EnsureVisibleRowCol(m_currentRow, -1);
		}
	}

	wxDataViewExtViewMode GetViewMode() const
	{
		return m_viewMode;
	}

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

	void SetRowHeight(int lineHeight) { m_lineHeight = lineHeight; }
	int GetRowHeight() const { return m_lineHeight; }
	int GetDefaultRowHeight() const;

	// Some useful functions for row and item mapping
	wxDataViewExtItem GetItemByRow(unsigned int row) const;
	int GetRowByItem(const wxDataViewExtItem& item,
		WalkFlags flags = Walk_All) const;

	wxDataViewExtTreeNode* GetTreeNodeByRow(unsigned int row) const;
	// We did not need this temporarily
	// wxDataViewExtTreeNode * GetTreeNodeByItem( const wxDataViewExtItem & item );

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

	bool EnableDragSource(const wxDataFormat& format);

	void RefreshDropHint();
	void RemoveDropHint();
	DropItemInfo GetDropItemInfo(const wxCoord x, const wxCoord y);
	wxDragResult OnDragOver(wxDataFormat format, wxCoord x, wxCoord y, wxDragResult def);
	bool OnDrop(wxDataFormat format, wxCoord x, wxCoord y);
	wxDragResult OnData(wxDataFormat format, wxCoord x, wxCoord y, wxDragResult def);
	void OnLeave();
#endif // wxUSE_DRAG_AND_DROP

	void OnColumnsCountChanged();

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

	void SetTopParent(const wxDataViewExtItem& item)
	{
		if ((!IsList()) && m_viewMode == wxDataViewExtViewMode::wxDataViewExtHierarchical)
		{
			wxDataViewExtModel* model = GetModel();

			if (model != NULL && item == model->GetParentTopItem())
			{
				model->SetParentTopItem(
					model->GetParent(item));
				Cleared();
			}
			else if (model != NULL && model->IsContainer(item))
			{
				model->SetParentTopItem(item);
				Cleared();
			}
		}
	}

private:
	void InvalidateCount() { m_count = -1; }
	void UpdateCount(int count)
	{
		m_count = count;
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
		const unsigned int cols = GetOwner()->GetColumnCount();
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
		const unsigned int cols = GetOwner()->GetColumnCount();
		const wxDataViewExtModel* const model = GetModel();
		for (unsigned int i = 0; i < cols; i++)
		{
			if (model->HasValue(item, i))
				return GetOwner()->GetColumnAt(i);
		}

		return NULL;
	}

	// Helper of public Expand(), must be called with a valid node.
	void DoExpand(wxDataViewExtTreeNode* node, unsigned int row, bool expandChildren);

private:
	wxDataViewExtCtrl* m_owner;
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
	HeightCache* m_rowHeightCache;

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
	int m_count;

	// This is the tree node under the cursor
	wxDataViewExtTreeNode* m_underMouse;

	// The control used for editing or NULL.
	wxWeakRef<wxWindow> m_editorCtrl;

	// Id m_editorCtrl is non-NULL, pointer to the associated renderer.
	wxDataViewExtRenderer* m_editorRenderer;

private:
	wxDECLARE_DYNAMIC_CLASS(wxDataViewExtMainWindow);
	wxDECLARE_EVENT_TABLE();
};

// ---------------------------------------------------------
// wxGenericDataViewModelNotifier
// ---------------------------------------------------------

class wxGenericDataViewModelNotifier : public wxDataViewExtModelNotifier
{
public:

	wxGenericDataViewModelNotifier(wxDataViewExtMainWindow* mainWindow)
	{
		m_mainWindow = mainWindow;
	}

	virtual bool ItemAdded(const wxDataViewExtItem& parent, const wxDataViewExtItem& item) wxOVERRIDE
	{
		return m_mainWindow->ItemAdded(parent, item);
	}

	virtual bool ItemDeleted(const wxDataViewExtItem& parent, const wxDataViewExtItem& item) wxOVERRIDE
	{
		return m_mainWindow->ItemDeleted(parent, item);
	}

	virtual bool ItemChanged(const wxDataViewExtItem& item) wxOVERRIDE
	{
		return m_mainWindow->ItemChanged(item);
	}

	virtual bool ValueChanged(const wxDataViewExtItem& item, unsigned int col) wxOVERRIDE
	{
		return m_mainWindow->ValueChanged(item, col);
	}

	virtual bool Cleared() wxOVERRIDE
	{
		return m_mainWindow->Cleared();
	}

#pragma region __table_notifier__h__

	virtual unsigned int GetCurrentModelColumn() const
	{
		wxDataViewExtCtrl* owner = m_mainWindow->GetOwner();
		wxASSERT(owner);
		wxDataViewExtColumn* column = owner->GetCurrentColumn();
		if (column != nullptr)
			return column->GetModelColumn();
		return 0;
	}

	virtual void StartEditing(const wxDataViewExtItem& item, unsigned int col) const
	{
		if (!item.IsOk())
			return;

		wxDataViewExtCtrl* owner = m_mainWindow->GetOwner();
		wxASSERT(owner);

		int viewColumn = owner->GetModelColumnIndex(col);
		if (viewColumn != wxNOT_FOUND) {
			owner->EditItem(item,
				owner->GetColumn(viewColumn)
			);
		}
		else if (col == 0) {

			wxDataViewExtColumn* currentColumn = owner->GetCurrentColumn();
			if (currentColumn != nullptr) {
				owner->EditItem(item,
					currentColumn
				);
			}
			else if (owner->GetColumnCount() > 0) {
				owner->EditItem(item,
					owner->GetColumnAt(0)
				);
			}
		}
	}

	virtual bool ShowFilter(struct CFilterRow& filter)
	{
		wxDataViewExtCtrl* owner = m_mainWindow->GetOwner();
		wxASSERT(owner);
		return owner->ShowFilter(filter);
	}

	virtual bool ShowViewMode()
	{
		wxDataViewExtCtrl* owner = m_mainWindow->GetOwner();
		wxASSERT(owner);
		return owner->ShowViewMode();
	}

	virtual void Select(const wxDataViewExtItem& item) const
	{
		wxDataViewExtCtrl* owner = m_mainWindow->GetOwner();
		wxASSERT(owner);
		owner->Select(item);
	}

	virtual int GetCountPerPage() const
	{
		wxDataViewExtCtrl* owner = m_mainWindow->GetOwner();
		wxASSERT(owner);
		return m_mainWindow->GetCountPerPage();
	}

	virtual wxDataViewExtItem GetSelection() const
	{
		wxDataViewExtCtrl* owner = m_mainWindow->GetOwner();
		wxASSERT(owner);
		return owner->GetSelection();
	}

	virtual int GetSelections(wxDataViewExtItemArray& sel) const
	{
		wxDataViewExtCtrl* owner = m_mainWindow->GetOwner();
		wxASSERT(owner);
		return owner->GetSelections(sel);
	}

#pragma endregion 

	virtual void Resort() wxOVERRIDE
	{
		m_mainWindow->Resort();
	}

	wxDataViewExtMainWindow* m_mainWindow;
};

// ---------------------------------------------------------
// wxDataViewExtRenderer
// ---------------------------------------------------------

wxIMPLEMENT_ABSTRACT_CLASS(wxDataViewExtRenderer, wxDataViewExtRendererBase);

wxDataViewExtRenderer::wxDataViewExtRenderer(const wxString& varianttype,
	wxDataViewExtCellMode mode,
	int align) :
	wxDataViewExtCustomRendererBase(varianttype, mode, align)
{
	m_align = align;
	m_mode = mode;
	m_ellipsizeMode = wxELLIPSIZE_MIDDLE;
	m_dc = NULL;
	m_state = 0;
}

wxDataViewExtRenderer::~wxDataViewExtRenderer()
{
	delete m_dc;
}

wxDC* wxDataViewExtRenderer::GetDC()
{
	if (m_dc == NULL)
	{
		if (GetOwner() == NULL)
			return NULL;
		if (GetOwner()->GetOwner() == NULL)
			return NULL;
		m_dc = new wxClientDC(GetOwner()->GetOwner());
	}

	return m_dc;
}

void wxDataViewExtRenderer::SetAlignment(int align)
{
	m_align = align;
}

int wxDataViewExtRenderer::GetAlignment() const
{
	return m_align;
}

// ---------------------------------------------------------
// wxDataViewExtCustomRenderer
// ---------------------------------------------------------

wxIMPLEMENT_ABSTRACT_CLASS(wxDataViewExtCustomRenderer, wxDataViewExtRenderer);

wxDataViewExtCustomRenderer::wxDataViewExtCustomRenderer(const wxString& varianttype,
	wxDataViewExtCellMode mode, int align) :
	wxDataViewExtRenderer(varianttype, mode, align)
{
}

#if wxUSE_ACCESSIBILITY
wxString wxDataViewExtCustomRenderer::GetAccessibleDescription() const
{
	wxVariant val;
	GetValue(val);

	wxString strVal;
	if (val.IsType(wxS("bool")))
	{
		/* TRANSLATORS: Name of Boolean true value */
		strVal = val.GetBool() ? _("true")
			/* TRANSLATORS: Name of Boolean false value */
			: _("false");
	}
	else
	{
		strVal = val.MakeString();
	}

	return strVal;
}
#endif // wxUSE_ACCESSIBILITY

// ---------------------------------------------------------
// wxDataViewExtTextRenderer
// ---------------------------------------------------------

wxIMPLEMENT_CLASS(wxDataViewExtTextRenderer, wxDataViewExtRenderer);

wxDataViewExtTextRenderer::wxDataViewExtTextRenderer(const wxString& varianttype,
	wxDataViewExtCellMode mode, int align) :
	wxDataViewExtRenderer(varianttype, mode, align)
{
#if wxUSE_MARKUP
	m_markupText = NULL;
#endif // wxUSE_MARKUP
}

wxDataViewExtTextRenderer::~wxDataViewExtTextRenderer()
{
#if wxUSE_MARKUP
	delete m_markupText;
#endif // wxUSE_MARKUP
}

#if wxUSE_MARKUP
void wxDataViewExtTextRenderer::EnableMarkup(bool enable)
{
	if (enable)
	{
		if (!m_markupText)
		{
			m_markupText = new wxItemMarkupText(wxString());
		}
	}
	else
	{
		if (m_markupText)
		{
			delete m_markupText;
			m_markupText = NULL;
		}
	}
}
#endif // wxUSE_MARKUP

bool wxDataViewExtTextRenderer::SetValue(const wxVariant& value)
{
	m_text = value.GetString();

#if wxUSE_MARKUP
	if (m_markupText)
		m_markupText->SetMarkup(m_text);
#endif // wxUSE_MARKUP

	return true;
}

bool wxDataViewExtTextRenderer::GetValue(wxVariant& WXUNUSED(value)) const
{
	return false;
}

#if wxUSE_ACCESSIBILITY
wxString wxDataViewExtTextRenderer::GetAccessibleDescription() const
{
#if wxUSE_MARKUP
	if (m_markupText)
		return wxMarkupParser::Strip(m_text);
#endif // wxUSE_MARKUP
	return m_text;
}
#endif // wxUSE_ACCESSIBILITY

bool wxDataViewExtTextRenderer::HasEditorCtrl() const
{
	return true;
}

wxWindow* wxDataViewExtTextRenderer::CreateEditorCtrl(wxWindow* parent,
	wxRect labelRect, const wxVariant& value)
{
	return CreateEditorTextCtrl(parent, labelRect, value);
}

bool wxDataViewExtTextRenderer::GetValueFromEditorCtrl(wxWindow* editor, wxVariant& value)
{
	wxTextCtrl* text = (wxTextCtrl*)editor;
	value = text->GetValue();
	return true;
}

bool wxDataViewExtTextRenderer::Render(wxRect rect, wxDC* dc, int state)
{
#if wxUSE_MARKUP
	if (m_markupText)
	{
		int flags = 0;
		if (state & wxDATAVIEW_CELL_SELECTED)
			flags |= wxCONTROL_SELECTED;
		m_markupText->Render(GetView(), *dc, rect, flags, GetEllipsizeMode());
	}
	else
#endif // wxUSE_MARKUP
		RenderText(m_text, 0, rect, dc, state);

	return true;
}

wxSize wxDataViewExtTextRenderer::GetSize() const
{
	if (!m_text.empty())
	{
#if wxUSE_MARKUP
		if (m_markupText)
		{
			wxDataViewExtCtrl* const view = GetView();
			wxClientDC dc(view);
			if (GetAttr().HasFont())
				dc.SetFont(GetAttr().GetEffectiveFont(view->GetFont()));

			return m_markupText->Measure(dc);
		}
#endif // wxUSE_MARKUP

		return GetTextExtent(m_text);
	}
	else
		return GetView()->FromDIP(wxSize(wxDVC_DEFAULT_RENDERER_SIZE,
			wxDVC_DEFAULT_RENDERER_SIZE));
}

// ---------------------------------------------------------
// wxDataViewExtBitmapRenderer
// ---------------------------------------------------------

wxIMPLEMENT_CLASS(wxDataViewExtBitmapRenderer, wxDataViewExtRenderer);

wxDataViewExtBitmapRenderer::wxDataViewExtBitmapRenderer(const wxString& varianttype,
	wxDataViewExtCellMode mode, int align) :
	wxDataViewExtRenderer(varianttype, mode, align)
{
}

bool wxDataViewExtBitmapRenderer::SetValue(const wxVariant& value)
{
	if (value.GetType() == wxT("wxBitmapBundle"))
	{
		m_bitmapBundle << value;
	}
	else if (value.GetType() == wxT("wxBitmap"))
	{
		wxBitmap bitmap;
		bitmap << value;
		m_bitmapBundle = wxBitmapBundle(bitmap);
	}
	else if (value.GetType() == wxT("wxIcon"))
	{
		wxIcon icon;
		icon << value;
		m_bitmapBundle = wxBitmapBundle(icon);
	}
	else
	{
		m_bitmapBundle.Clear();
	}

	return true;
}

bool wxDataViewExtBitmapRenderer::GetValue(wxVariant& WXUNUSED(value)) const
{
	return false;
}

bool
wxDataViewExtBitmapRenderer::IsCompatibleVariantType(const wxString& variantType) const
{
	// We can accept values of any types checked by SetValue().
	return variantType == wxS("wxBitmapBundle")
		|| variantType == wxS("wxBitmap")
		|| variantType == wxS("wxIcon");
}

#if wxUSE_ACCESSIBILITY
wxString wxDataViewExtBitmapRenderer::GetAccessibleDescription() const
{
	return wxEmptyString;
}
#endif // wxUSE_ACCESSIBILITY

bool wxDataViewExtBitmapRenderer::Render(wxRect cell, wxDC* dc, int WXUNUSED(state))
{
	if (m_bitmapBundle.IsOk())
	{
		dc->DrawBitmap(m_bitmapBundle.GetBitmapFor(GetView()),
			cell.x, cell.y,
			true /* use mask */);
	}

	return true;
}

wxSize wxDataViewExtBitmapRenderer::GetSize() const
{
	if (m_bitmapBundle.IsOk())
		return m_bitmapBundle.GetPreferredBitmapSizeFor(GetView());

	return GetView()->FromDIP(wxSize(wxDVC_DEFAULT_RENDERER_SIZE,
		wxDVC_DEFAULT_RENDERER_SIZE));
}

// ---------------------------------------------------------
// wxDataViewExtToggleRenderer
// ---------------------------------------------------------

wxIMPLEMENT_ABSTRACT_CLASS(wxDataViewExtToggleRenderer, wxDataViewExtRenderer);

wxDataViewExtToggleRenderer::wxDataViewExtToggleRenderer(const wxString& varianttype,
	wxDataViewExtCellMode mode, int align) :
	wxDataViewExtRenderer(varianttype, mode, align)
{
	m_toggle = false;
	m_radio = false;
}

bool wxDataViewExtToggleRenderer::SetValue(const wxVariant& value)
{
	m_toggle = value.GetBool();

	return true;
}

bool wxDataViewExtToggleRenderer::GetValue(wxVariant& WXUNUSED(value)) const
{
	return false;
}

#if wxUSE_ACCESSIBILITY
wxString wxDataViewExtToggleRenderer::GetAccessibleDescription() const
{
	/* TRANSLATORS: Checkbox state name */
	return m_toggle ? _("checked")
		/* TRANSLATORS: Checkbox state name */
		: _("unchecked");
}
#endif // wxUSE_ACCESSIBILITY

bool wxDataViewExtToggleRenderer::Render(wxRect cell, wxDC* dc, int WXUNUSED(state))
{
	int flags = 0;
	if (m_toggle)
		flags |= wxCONTROL_CHECKED;
	if (GetMode() != wxDATAVIEW_CELL_ACTIVATABLE ||
		!(GetOwner()->GetOwner()->IsEnabled() && GetEnabled()))
		flags |= wxCONTROL_DISABLED;

	// Ensure that the check boxes always have at least the minimal required
	// size, otherwise DrawCheckBox() doesn't really work well. If this size is
	// greater than the cell size, the checkbox will be truncated but this is a
	// lesser evil.
	wxSize size = cell.GetSize();
	size.IncTo(GetSize());
	cell.SetSize(size);

	wxRendererNative& renderer = wxRendererNative::Get();
	wxWindow* const win = GetOwner()->GetOwner();
	if (m_radio)
		renderer.DrawRadioBitmap(win, *dc, cell, flags);
	else
		renderer.DrawCheckBox(win, *dc, cell, flags);

	return true;
}

bool wxDataViewExtToggleRenderer::WXActivateCell(const wxRect& WXUNUSED(cellRect),
	wxDataViewExtModel* model,
	const wxDataViewExtItem& item,
	unsigned int col,
	const wxMouseEvent* mouseEvent)
{
	if (mouseEvent)
	{
		// Only react to clicks directly on the checkbox, not elsewhere in the
		// same cell.
		if (!wxRect(GetSize()).Contains(mouseEvent->GetPosition()))
			return false;
	}

	model->ChangeValue(!m_toggle, item, col);
	return true;
}

wxSize wxDataViewExtToggleRenderer::GetSize() const
{
	return wxRendererNative::Get().GetCheckBoxSize(GetView());
}

// ---------------------------------------------------------
// wxDataViewExtProgressRenderer
// ---------------------------------------------------------

wxIMPLEMENT_ABSTRACT_CLASS(wxDataViewExtProgressRenderer, wxDataViewExtRenderer);

wxDataViewExtProgressRenderer::wxDataViewExtProgressRenderer(const wxString& label,
	const wxString& varianttype, wxDataViewExtCellMode mode, int align) :
	wxDataViewExtRenderer(varianttype, mode, align)
	, m_label(label)
{
	m_value = 0;
}

bool wxDataViewExtProgressRenderer::SetValue(const wxVariant& value)
{
	m_value = (long)value;

	if (m_value < 0) m_value = 0;
	if (m_value > 100) m_value = 100;

	return true;
}

bool wxDataViewExtProgressRenderer::GetValue(wxVariant& value) const
{
	value = (long)m_value;
	return true;
}

#if wxUSE_ACCESSIBILITY
wxString wxDataViewExtProgressRenderer::GetAccessibleDescription() const
{
	return wxString::Format(wxS("%i %%"), m_value);
}
#endif // wxUSE_ACCESSIBILITY

bool
wxDataViewExtProgressRenderer::Render(wxRect rect, wxDC* dc, int WXUNUSED(state))
{
	const wxDataViewExtItemAttr& attr = GetAttr();
	if (attr.HasColour())
		dc->SetBackground(attr.GetColour());

	// This is a hack, but native renderers don't support using custom colours,
	// but typically gauge colour is important (e.g. it's commonly green/red to
	// indicate some qualitative difference), so we fall back to the generic
	// implementation which looks ugly but does support using custom colour.
	wxRendererNative& renderer = attr.HasColour()
		? wxRendererNative::GetGeneric()
		: wxRendererNative::Get();
	renderer.DrawGauge(
		GetOwner()->GetOwner(),
		*dc,
		rect,
		m_value,
		100);

	return true;
}

wxSize wxDataViewExtProgressRenderer::GetSize() const
{
	// Return -1 width because a progress bar fits any width; unlike most
	// renderers, it doesn't have a "good" width for the content. This makes it
	// grow to the whole column, which is pretty much always the desired
	// behaviour. Keep the height fixed so that the progress bar isn't too fat.
	return GetView()->FromDIP(wxSize(-1, 12));
}

// ---------------------------------------------------------
// wxDataViewExtIconTextRenderer
// ---------------------------------------------------------

wxIMPLEMENT_CLASS(wxDataViewExtIconTextRenderer, wxDataViewExtRenderer);

wxDataViewExtIconTextRenderer::wxDataViewExtIconTextRenderer(
	const wxString& varianttype, wxDataViewExtCellMode mode, int align) :
	wxDataViewExtRenderer(varianttype, mode, align)
{
	SetMode(mode);
	SetAlignment(align);
}

bool wxDataViewExtIconTextRenderer::SetValue(const wxVariant& value)
{
	m_value << value;
	return true;
}

bool wxDataViewExtIconTextRenderer::GetValue(wxVariant& WXUNUSED(value)) const
{
	return false;
}

#if wxUSE_ACCESSIBILITY
wxString wxDataViewExtIconTextRenderer::GetAccessibleDescription() const
{
	return m_value.GetText();
}
#endif // wxUSE_ACCESSIBILITY

bool wxDataViewExtIconTextRenderer::Render(wxRect rect, wxDC* dc, int state)
{
	int xoffset = 0;

	const wxBitmapBundle& bb = m_value.GetBitmapBundle();
	if (bb.IsOk())
	{
		wxWindow* const dvc = GetView();
		const wxIcon& icon = bb.GetIconFor(dvc);
		dc->DrawIcon(icon, rect.x, rect.y + (rect.height - icon.GetLogicalHeight()) / 2);
		xoffset = icon.GetLogicalWidth() + dvc->FromDIP(4);
	}

	RenderText(m_value.GetText(), xoffset, rect, dc, state);

	return true;
}

wxSize wxDataViewExtIconTextRenderer::GetSize() const
{
	wxWindow* const dvc = GetView();

	if (!m_value.GetText().empty())
	{
		wxSize size = GetTextExtent(m_value.GetText());

		const wxBitmapBundle& bb = m_value.GetBitmapBundle();
		if (bb.IsOk())
			size.x += bb.GetPreferredLogicalSizeFor(dvc).x + dvc->FromDIP(4);
		return size;
	}
	return dvc->FromDIP(wxSize(80, 20));
}

wxWindow* wxDataViewExtIconTextRenderer::CreateEditorCtrl(wxWindow* parent, wxRect labelRect, const wxVariant& value)
{
	wxDataViewExtIconText iconText;
	iconText << value;

	wxString text = iconText.GetText();

	// adjust the label rect to take the width of the icon into account
	const wxBitmapBundle& bb = iconText.GetBitmapBundle();
	if (bb.IsOk())
	{
		wxWindow* const dvc = GetView();

		int w = bb.GetPreferredLogicalSizeFor(dvc).x + dvc->FromDIP(4);
		labelRect.x += w;
		labelRect.width -= w;
	}

	return CreateEditorTextCtrl(parent, labelRect, text);
}

bool wxDataViewExtIconTextRenderer::GetValueFromEditorCtrl(wxWindow* editor, wxVariant& value)
{
	wxTextCtrl* text = (wxTextCtrl*)editor;

	// The icon can't be edited so get its old value and reuse it.
	wxVariant valueOld;
	wxDataViewExtColumn* const col = GetOwner();
	GetView()->GetModel()->GetValue(valueOld, m_item, col->GetModelColumn());

	wxDataViewExtIconText iconText;
	iconText << valueOld;

	// But replace the text with the value entered by user.
	iconText.SetText(text->GetValue());

	value << iconText;
	return true;
}

//-----------------------------------------------------------------------------
// wxDataViewExtDropTarget
//-----------------------------------------------------------------------------

#if wxUSE_DRAG_AND_DROP

class wxBitmapCanvas : public wxWindow
{
public:
	wxBitmapCanvas(wxWindow* parent, const wxBitmap& bitmap, const wxSize& size) :
		wxWindow(parent, wxID_ANY, wxPoint(0, 0), size)
		, m_bitmap(bitmap)
	{
		Bind(wxEVT_PAINT, &wxBitmapCanvas::OnPaint, this);
	}

	void OnPaint(wxPaintEvent& WXUNUSED(event))
	{
		wxPaintDC dc(this);
		dc.DrawBitmap(m_bitmap, 0, 0);
	}

	wxBitmap m_bitmap;
};

class wxDataViewExtDropSource : public wxDropSource
{
public:
	wxDataViewExtDropSource(wxDataViewExtMainWindow* win, unsigned int row) :
		wxDropSource(win)
	{
		m_win = win;
		m_row = row;
		m_hint = NULL;
	}

	~wxDataViewExtDropSource()
	{
		delete m_hint;
	}

	virtual bool GiveFeedback(wxDragResult WXUNUSED(effect)) wxOVERRIDE
	{
		wxPoint pos = wxGetMousePosition();

		if (!m_hint)
		{
			int liney = m_win->GetLineStart(m_row);
			int linex = 0;
			m_win->GetOwner()->CalcUnscrolledPosition(0, liney, NULL, &liney);
			m_win->ClientToScreen(&linex, &liney);
			m_dist_x = pos.x - linex;
			m_dist_y = pos.y - liney;

			int indent = 0;
			wxBitmap ib = m_win->CreateItemBitmap(m_row, indent);
			m_dist_x -= indent;
			m_hint = new wxFrame(m_win->GetParent(), wxID_ANY, wxEmptyString,
				wxPoint(pos.x - m_dist_x, pos.y + 5),
				wxSize(1, 1),
				wxFRAME_TOOL_WINDOW |
				wxFRAME_FLOAT_ON_PARENT |
				wxFRAME_NO_TASKBAR |
				wxNO_BORDER);
			new wxBitmapCanvas(m_hint, ib, ib.GetLogicalSize());
			m_hint->SetClientSize(ib.GetLogicalSize());
			m_hint->SetTransparent(128);
			m_hint->Show();
		}
		else
		{
			m_hint->Move(pos.x - m_dist_x, pos.y + 5);
		}

		return false;
	}

	wxDataViewExtMainWindow* m_win;
	unsigned int            m_row;
	wxFrame* m_hint;
	int m_dist_x, m_dist_y;
};


class wxDataViewExtDropTarget : public wxDropTarget
{
public:
	wxDataViewExtDropTarget(wxDataObjectComposite* obj, wxDataViewExtMainWindow* win) :
		wxDropTarget(obj),
		m_obj(obj)
	{
		m_win = win;
	}

	wxDataObjectComposite* GetCompositeDataObject() const { return m_obj; }

	virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def) wxOVERRIDE
	{
		wxDataFormat format = GetMatchingPair();
		if (format == wxDF_INVALID)
			return wxDragNone;
		return m_win->OnDragOver(format, x, y, def);
	}

	virtual bool OnDrop(wxCoord x, wxCoord y) wxOVERRIDE
	{
		wxDataFormat format = GetMatchingPair();
		if (format == wxDF_INVALID)
			return false;
		return m_win->OnDrop(format, x, y);
	}

	virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def) wxOVERRIDE
	{
		wxDataFormat format = GetMatchingPair();
		if (format == wxDF_INVALID)
			return wxDragNone;
		if (!GetData())
			return wxDragNone;
		return m_win->OnData(format, x, y, def);
	}

	virtual void OnLeave() wxOVERRIDE
	{
		m_win->OnLeave();
	}

private:
	wxDataObjectComposite* const m_obj;

	wxDataViewExtMainWindow* m_win;
};

#endif // wxUSE_DRAG_AND_DROP

//-----------------------------------------------------------------------------
// wxDataViewExtRenameTimer
//-----------------------------------------------------------------------------

wxDataViewExtRenameTimer::wxDataViewExtRenameTimer(wxDataViewExtMainWindow* owner)
{
	m_owner = owner;
}

void wxDataViewExtRenameTimer::Notify()
{
	m_owner->OnRenameTimer();
}



// ----------------------------------------------------------------------------
// wxDataViewExtTreeNode
// ----------------------------------------------------------------------------

namespace
{

	// Comparator used for sorting the tree nodes using the model-defined sort
	// order and also for performing binary search in our own code.
	class wxGenericTreeModelNodeCmp
	{
	public:
		wxGenericTreeModelNodeCmp(wxDataViewExtMainWindow* window,
			const SortOrder& sortOrder)
			: m_model(window->GetModel()),
			m_sortOrder(sortOrder)
		{
			wxASSERT_MSG(!m_sortOrder.IsNone(), "should have sort order");
		}

		// Return negative, zero or positive value depending on whether the first
		// item is less than, equal to or greater than the second one.
		int Compare(wxDataViewExtTreeNode* first, wxDataViewExtTreeNode* second) const
		{
			if (first->HasChildren() != second->HasChildren())
				return first->HasChildren() ? -1 : 1;

			return m_model->Compare(first->GetItem(), second->GetItem(),
				m_sortOrder.GetColumn(),
				m_sortOrder.IsAscending());
		}

		// Return true if the items are (strictly) in order, i.e. the first item is
		// less than the second one. This is used by std::sort().
		bool operator()(wxDataViewExtTreeNode* first, wxDataViewExtTreeNode* second) const
		{
			return Compare(first, second) < 0;
		}

	private:
		wxDataViewExtModel* const m_model;
		const SortOrder m_sortOrder;
	};

} // anonymous namespace

void wxDataViewExtTreeNode::InsertChild(wxDataViewExtMainWindow* window,
	wxDataViewExtTreeNode* node, unsigned index)
{
	if (!m_branchData)
		m_branchData = new BranchNodeData;

	const SortOrder sortOrder = window->GetSortOrder();

	// Flag indicating whether we should retain existing sorted list when
	// inserting the child node.
	bool insertSorted = false;

	if (sortOrder.IsNone())
	{
		// We should insert assuming an unsorted list. This will cause the
		// child list to lose the current sort order, if any.
		m_branchData->sortOrder = SortOrder();
	}
	else if (m_branchData->children.empty())
	{
		if (m_branchData->open)
		{
			// We don't need to search for the right place to insert the first
			// item (there is only one), but we do need to remember the sort
			// order to use for the subsequent ones.
			m_branchData->sortOrder = sortOrder;
		}
		else
		{
			// We're inserting the first child of a closed node. We can choose
			// whether to consider this empty child list sorted or unsorted.
			// By choosing unsorted, we postpone comparisons until the parent
			// node is opened in the view, which may be never.
			m_branchData->sortOrder = SortOrder();
		}
	}
	else if (m_branchData->open)
	{
		// For open branches, children should be already sorted.
		wxASSERT_MSG(m_branchData->sortOrder == sortOrder,
			wxS("Logic error in wxDVC sorting code"));

		// We can use fast insertion.
		insertSorted = true;
	}
	else if (m_branchData->sortOrder == sortOrder)
	{
		// The children are already sorted by the correct criteria (because
		// the node must have been opened in the same time in the past). Even
		// though it is closed now, we still insert in sort order to avoid a
		// later resort.
		insertSorted = true;
	}
	else
	{
		// The children of this closed node aren't sorted by the correct
		// criteria, so we just insert unsorted.
		m_branchData->sortOrder = SortOrder();
	}


	if (insertSorted)
	{
		// Use binary search to find the correct position to insert at.
		wxGenericTreeModelNodeCmp cmp(window, sortOrder);
		int lo = 0, hi = m_branchData->children.size();
		while (lo < hi)
		{
			int mid = lo + (hi - lo) / 2;
			int r = cmp.Compare(node, m_branchData->children[mid]);
			if (r < 0)
				hi = mid;
			else if (r > 0)
				lo = mid + 1;
			else
				lo = hi = mid;
		}
		m_branchData->InsertChild(node, lo);
	}
	else
	{
		m_branchData->InsertChild(node, index);
	}
}


void wxDataViewExtTreeNode::Resort(wxDataViewExtMainWindow* window)
{
	if (!m_branchData)
		return;

	// No reason to sort a closed node.
	if (!m_branchData->open)
		return;

	const SortOrder sortOrder = window->GetSortOrder();
	if (!sortOrder.IsNone())
	{
		wxDataViewExtTreeNodes& nodes = m_branchData->children;

		// When sorting by column value, we can skip resorting entirely if the
		// same sort order was used previously. However we can't do this when
		// using model-specific sort order, which can change at any time.
		if (m_branchData->sortOrder != sortOrder || !sortOrder.UsesColumn())
		{
			std::sort(m_branchData->children.begin(),
				m_branchData->children.end(),
				wxGenericTreeModelNodeCmp(window, sortOrder));

			m_branchData->sortOrder = sortOrder;
		}

		// There may be open child nodes that also need a resort.
		int len = nodes.size();
		for (int i = 0; i < len; i++)
		{
			if (nodes[i]->HasChildren())
				nodes[i]->Resort(window);
		}
	}
}


void
wxDataViewExtTreeNode::PutChildInSortOrder(wxDataViewExtMainWindow* window,
	wxDataViewExtTreeNode* childNode)
{
	// The childNode has changed, and may need to be moved to another location
	// in the sorted child list.

	if (!m_branchData)
		return;
	if (!m_branchData->open)
		return;
	if (m_branchData->sortOrder.IsNone())
		return;

	wxDataViewExtTreeNodes& nodes = m_branchData->children;

	// This is more than an optimization, the code below assumes that 1 is a
	// valid index.
	if (nodes.size() == 1)
		return;

	// We should already be sorted in the right order.
	wxASSERT(m_branchData->sortOrder == window->GetSortOrder());

	// First find the node in the current child list
	int hi = nodes.size();
	int oldLocation = wxNOT_FOUND;
	for (int index = 0; index < hi; ++index)
	{
		if (nodes[index] == childNode)
		{
			oldLocation = index;
			break;
		}
	}
	wxCHECK_RET(oldLocation >= 0, "not our child?");

	wxGenericTreeModelNodeCmp cmp(window, m_branchData->sortOrder);

	// Check if we actually need to move the node.
	bool locationChanged = false;

	// Compare with next node
	if (oldLocation != hi - 1)
	{
		if (!cmp(childNode, nodes[oldLocation + 1]))
			locationChanged = true;
	}

	// Compare with previous node
	if (!locationChanged && oldLocation > 0)
	{
		if (!cmp(nodes[oldLocation - 1], childNode))
			locationChanged = true;
	}

	if (!locationChanged)
		return;

	// Remove and reinsert the node in the child list
	m_branchData->RemoveChild(oldLocation);
	hi = nodes.size();
	int lo = 0;
	while (lo < hi)
	{
		int mid = lo + (hi - lo) / 2;
		int r = cmp.Compare(childNode, m_branchData->children[mid]);
		if (r < 0)
			hi = mid;
		else if (r > 0)
			lo = mid + 1;
		else
			lo = hi = mid;
	}
	m_branchData->InsertChild(childNode, lo);

	// Make sure the change is actually shown right away
	window->UpdateDisplay();
}


//-----------------------------------------------------------------------------
// wxDataViewExtMainWindow
//-----------------------------------------------------------------------------

// The tree building helper, declared firstly

static void BuildHierarchicalHelper(wxDataViewExtMainWindow* window,
	const wxDataViewExtModel* model,
	wxDataViewExtTreeNode* node);

static void BuildListHelper(wxDataViewExtMainWindow* window,
	const wxDataViewExtModel* model,
	const wxDataViewExtItem& item,
	wxDataViewExtTreeNode* node);

static void BuildTreeHelper(wxDataViewExtMainWindow* window,
	const wxDataViewExtModel* model,
	const wxDataViewExtItem& item,
	wxDataViewExtTreeNode* node);

wxIMPLEMENT_ABSTRACT_CLASS(wxDataViewExtMainWindow, wxWindow);

wxBEGIN_EVENT_TABLE(wxDataViewExtMainWindow, wxWindow)
EVT_PAINT(wxDataViewExtMainWindow::OnPaint)
EVT_MOUSE_EVENTS(wxDataViewExtMainWindow::OnMouse)
EVT_SET_FOCUS(wxDataViewExtMainWindow::OnSetFocus)
EVT_KILL_FOCUS(wxDataViewExtMainWindow::OnKillFocus)
EVT_CHAR_HOOK(wxDataViewExtMainWindow::OnCharHook)
EVT_CHAR(wxDataViewExtMainWindow::OnChar)
wxEND_EVENT_TABLE()

wxDataViewExtMainWindow::wxDataViewExtMainWindow(wxDataViewExtCtrl* parent, wxWindowID id,
	const wxPoint& pos, const wxSize& size, const wxString& name)
{
	// We want to use a specific class name for this window in wxMSW to make it
	// possible to configure screen readers to handle it specifically.
#ifdef __WXMSW__
	CreateUsingMSWClass
	(
		wxApp::GetRegisteredClassName
		(
			wxT("wxDataViewExt"),
			-1, // no specific background brush
			0, // no special styles either
			wxApp::RegClass_OnlyNR
		),
		parent, id, pos, size, wxWANTS_CHARS | wxBORDER_NONE, name
	);
#else
	Create(parent, id, pos, size, wxWANTS_CHARS | wxBORDER_NONE, name);
#endif

	SetOwner(parent);

	m_editorRenderer = NULL;
	m_selectionMode = wxDataViewExtSelectionMode::wxDataViewExtSelectRow;

	m_viewMode = wxDataViewExtViewMode::wxDataViewExtTree;

	m_lastOnSame = false;
	m_renameTimer = new wxDataViewExtRenameTimer(this);

	// TODO: user better initial values/nothing selected
	m_currentCol = NULL;
	m_currentColSetByKeyboard = false;
	m_useCellFocus = false;
	m_currentRow = (unsigned)-1;
	m_lineHeight = GetDefaultRowHeight();
	if (GetOwner()->HasFlag(wxDV_VARIABLE_LINE_HEIGHT))
	{
		m_rowHeightCache = new HeightCache();
	}
	else
	{
		m_rowHeightCache = NULL;
	}

#if wxUSE_DRAG_AND_DROP
	m_dragCount = 0;
	m_dragStart = wxPoint(0, 0);

	m_dragEnabled = false;
	m_dropItemInfo = DropItemInfo();
#endif // wxUSE_DRAG_AND_DROP

	m_lineLastClicked = (unsigned int)-1;
	m_lineBeforeLastClicked = (unsigned int)-1;
	m_lineSelectSingleOnUp = (unsigned int)-1;

	m_hasFocus = false;

	SetBackgroundColour(*wxWHITE);

	SetBackgroundStyle(wxBG_STYLE_PAINT);

	m_penRule = wxPen(GetRuleColour());

	m_root = wxDataViewExtTreeNode::CreateRootNode();

	// Make m_count = -1 will cause the class recaculate the real displaying number of rows.
	m_count = -1;
	m_underMouse = NULL;

	UpdateDisplay();
}

wxDataViewExtMainWindow::~wxDataViewExtMainWindow()
{
	DestroyTree();
	delete m_renameTimer;
	delete m_rowHeightCache;
}

int wxDataViewExtMainWindow::GetDefaultRowHeight() const
{
	const int SMALL_ICON_HEIGHT = FromDIP(16);

#ifdef __WXMSW__
	// We would like to use the same line height that Explorer uses. This is
	// different from standard ListView control since Vista.
	if (wxGetWinVersion() >= wxWinVersion_Vista)
		return wxMax(SMALL_ICON_HEIGHT, GetCharHeight()) + FromDIP(6);
	else
#endif // __WXMSW__
		return wxMax(SMALL_ICON_HEIGHT, GetCharHeight()) + FromDIP(1);
}



#if wxUSE_DRAG_AND_DROP
bool wxDataViewExtMainWindow::EnableDragSource(const wxDataFormat& format)
{
	m_dragFormat = format;
	m_dragEnabled = format != wxDF_INVALID;

	return true;
}

void wxDataViewExtMainWindow::RefreshDropHint()
{
	const unsigned row = m_dropItemInfo.m_row;

	switch (m_dropItemInfo.m_hint)
	{
	case DropHint_None:
		break;

	case DropHint_Inside:
		RefreshRow(row);
		break;

	case DropHint_Above:
		RefreshRows(row == 0 ? 0 : row - 1, row);
		break;

	case DropHint_Below:
		// It's not a problem here if row+1 is out of range, RefreshRows()
		// allows this.
		RefreshRows(row, row + 1);
		break;
	}
}

void wxDataViewExtMainWindow::RemoveDropHint()
{
	RefreshDropHint();

	m_dropItemInfo = DropItemInfo();
}

wxDataViewExtMainWindow::DropItemInfo wxDataViewExtMainWindow::GetDropItemInfo(const wxCoord x, const wxCoord y)
{
	DropItemInfo dropItemInfo;

	int xx = x;
	int yy = y;
	m_owner->CalcUnscrolledPosition(xx, yy, &xx, &yy);

	unsigned int row = GetLineAt(yy);
	dropItemInfo.m_row = row;

	if (row >= GetRowCount() || xx > GetEndOfLastCol())
		return dropItemInfo;

	if (IsVirtualList())
	{
		dropItemInfo.m_item = GetItemByRow(row);

		if (dropItemInfo.m_item.IsOk())
			dropItemInfo.m_hint = DropHint_Inside;
	}
	else
	{
		wxDataViewExtTreeNode* node = GetTreeNodeByRow(row);
		if (!node)
			return dropItemInfo;

		dropItemInfo.m_item = node->GetItem();

		const int itemStart = GetLineStart(row);
		const int itemHeight = GetLineHeight(row);

		// 15% is an arbitrarily chosen threshold here, which could be changed
		// or made configurable if really needed.
		static const double UPPER_ITEM_PART = 0.15;

		bool insertAbove = yy - itemStart < itemHeight * UPPER_ITEM_PART;
		if (insertAbove)
		{
			// Can be treated as 'insertBelow" with the small difference:
			node = GetTreeNodeByRow(row - 1);   // We need the node from the previous row

			dropItemInfo.m_hint = DropHint_Above;

			if (!node)
			{
				// Seems to be dropped in the root node

				dropItemInfo.m_indentLevel = 0;
				dropItemInfo.m_proposedDropIndex = 0;
				dropItemInfo.m_item = wxDataViewExtItem();

				return dropItemInfo;
			}

		}

		bool insertBelow = yy - itemStart > itemHeight * (1.0 - UPPER_ITEM_PART);
		if (insertBelow)
			dropItemInfo.m_hint = DropHint_Below;

		if (insertBelow || insertAbove)
		{
			// Insert inside the 'item' or after (below) it. Depends on:
			// 1 - the 'item' is a container
			// 2 - item's child index in it's parent (the last in the parent or not)
			// 3 - expanded (opened) or not
			// 4 - mouse x position

			int xStart = 0;     // Expander column x position start
			wxDataViewExtColumn* const expander = GetExpanderColumnOrFirstOne(GetOwner());
			for (unsigned int i = 0; i < GetOwner()->GetColumnCount(); i++)
			{
				wxDataViewExtColumn* col = GetOwner()->GetColumnAt(i);
				if (col->IsHidden())
					continue;   // skip it!

				if (col == expander)
					break;

				xStart += col->GetWidth();
			}

			const int expanderWidth = wxRendererNative::Get().GetExpanderSize(this).GetWidth();

			int level = node->GetIndentLevel();

			wxDataViewExtTreeNode* prevAscendNode = node;
			wxDataViewExtTreeNode* ascendNode = node;
			while (ascendNode != NULL)
			{
				dropItemInfo.m_indentLevel = level + 1;

				if (m_owner->GetModel()->IsContainer(ascendNode->GetItem()))
				{
					// Item can be inserted
					dropItemInfo.m_item = ascendNode->GetItem();

					int itemPosition = ascendNode->FindChildByItem(prevAscendNode->GetItem());
					if (itemPosition == wxNOT_FOUND)
						itemPosition = 0;
					else
						itemPosition++;

					dropItemInfo.m_proposedDropIndex = itemPosition;

					// We must break the loop if the applied node is expanded
					// (opened) and the proposed drop position is not the last
					// in this node.
					if (ascendNode->IsOpen())
					{
						const size_t lastPos = ascendNode->GetChildNodes().size();
						if (static_cast<size_t>(itemPosition) != lastPos)
							break;
					}

					int indent = GetOwner()->GetIndent() * level + expanderWidth;

					if (xx >= xStart + indent)
						break;
				}

				prevAscendNode = ascendNode;
				ascendNode = ascendNode->GetParent();
				--level;
			}
		}
		else
		{
			dropItemInfo.m_hint = DropHint_Inside;
		}
	}

	return dropItemInfo;
}


wxDragResult wxDataViewExtMainWindow::OnDragOver(wxDataFormat format, wxCoord x,
	wxCoord y, wxDragResult def)
{
	DropItemInfo nextDropItemInfo = GetDropItemInfo(x, y);

	wxDataViewExtEvent event(wxEVT_DATAVIEW_ITEM_DROP_POSSIBLE, m_owner, nextDropItemInfo.m_item);
	event.SetProposedDropIndex(nextDropItemInfo.m_proposedDropIndex);
	event.SetDataFormat(format);
	event.SetDropEffect(def);

	wxDragResult result = def;

	if (m_owner->HandleWindowEvent(event) && event.IsAllowed())
	{
		// Processing handled event

		result = event.GetDropEffect();
		switch (result)
		{
		case wxDragCopy:
		case wxDragMove:
		case wxDragLink:
			break;

		case wxDragNone:
		case wxDragCancel:
		case wxDragError:
		{
			RemoveDropHint();
			return result;
		}
		}
	}
	else
	{
		RemoveDropHint();
		return wxDragNone;
	}

	if (nextDropItemInfo.m_hint != DropHint_None)
	{
		if (m_dropItemInfo.m_hint != nextDropItemInfo.m_hint ||
			m_dropItemInfo.m_row != nextDropItemInfo.m_row)
		{
			RefreshDropHint();   // refresh previous rows
		}

		m_dropItemInfo.m_hint = nextDropItemInfo.m_hint;
		m_dropItemInfo.m_row = nextDropItemInfo.m_row;

		RefreshDropHint();
	}
	else
	{
		RemoveDropHint();
	}

	m_dropItemInfo = nextDropItemInfo;

	return result;
}

bool wxDataViewExtMainWindow::OnDrop(wxDataFormat format, wxCoord x, wxCoord y)
{
	RemoveDropHint();

	DropItemInfo dropItemInfo = GetDropItemInfo(x, y);

	wxDataViewExtEvent event(wxEVT_DATAVIEW_ITEM_DROP_POSSIBLE, m_owner, dropItemInfo.m_item);
	event.SetProposedDropIndex(dropItemInfo.m_proposedDropIndex);
	event.SetDataFormat(format);
	if (!m_owner->HandleWindowEvent(event) || !event.IsAllowed())
		return false;

	return true;
}

wxDragResult wxDataViewExtMainWindow::OnData(wxDataFormat format, wxCoord x, wxCoord y,
	wxDragResult def)
{
	DropItemInfo dropItemInfo = GetDropItemInfo(x, y);

	wxDataViewExtDropTarget* const
		target = static_cast<wxDataViewExtDropTarget*>(GetDropTarget());
	wxDataObjectComposite* const obj = target->GetCompositeDataObject();

	wxDataViewExtEvent event(wxEVT_DATAVIEW_ITEM_DROP, m_owner, dropItemInfo.m_item);
	event.SetProposedDropIndex(dropItemInfo.m_proposedDropIndex);
	event.InitData(obj, format);
	event.SetDropEffect(def);
	if (!m_owner->HandleWindowEvent(event) || !event.IsAllowed())
		return wxDragNone;

	return def;
}

void wxDataViewExtMainWindow::OnLeave()
{
	RemoveDropHint();
}

wxBitmap wxDataViewExtMainWindow::CreateItemBitmap(unsigned int row, int& indent)
{
	int height = GetLineHeight(row);
	int width = 0;
	unsigned int cols = GetOwner()->GetColumnCount();
	unsigned int col;
	for (col = 0; col < cols; col++)
	{
		wxDataViewExtColumn* column = GetOwner()->GetColumnAt(col);
		if (column->IsHidden())
			continue;      // skip it!
		width += column->GetWidth();
	}

	indent = 0;
	if (!IsList())
	{
		wxDataViewExtTreeNode* node = GetTreeNodeByRow(row);
		indent = GetOwner()->GetIndent() * node->GetIndentLevel();
		indent += wxRendererNative::Get().GetExpanderSize(this).GetWidth();
	}
	width -= indent;

	wxBitmap bitmap;
#ifdef wxHAS_DPI_INDEPENDENT_PIXELS
	bitmap.CreateWithDIPSize(width, height, GetDPIScaleFactor());
#else
	bitmap.Create(width, height);
	bitmap.SetScaleFactor(GetDPIScaleFactor());
#endif
	wxMemoryDC dc(bitmap);
	dc.SetFont(GetFont());
	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(*wxWHITE_BRUSH);
	dc.DrawRectangle(0, 0, width, height);

	wxDataViewExtModel* model = m_owner->GetModel();

	wxDataViewExtColumn* const
		expander = GetExpanderColumnOrFirstOne(GetOwner());

	int x = 0;
	for (col = 0; col < cols; col++)
	{
		wxDataViewExtColumn* column = GetOwner()->GetColumnAt(col);
		wxDataViewExtRenderer* cell = column->GetRenderer();

		if (column->IsHidden())
			continue;       // skip it!

		width = column->GetWidth();

		if (column == expander)
			width -= indent;

		wxDataViewExtItem item = GetItemByRow(row);
		if (cell->PrepareForItem(model, item, column->GetModelColumn()))
		{
			wxRect item_rect(x, 0, width, height);
			item_rect.Deflate(FromDIP(PADDING_RIGHTLEFT), 0);

			// dc.SetClippingRegion( item_rect );
			cell->WXCallRender(item_rect, &dc, 0);
			// dc.DestroyClippingRegion();
		}

		x += width;
	}

	return bitmap;
}

#endif // wxUSE_DRAG_AND_DROP

void wxDataViewExtMainWindow::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	wxDataViewExtModel* model = GetModel();
	wxAutoBufferedPaintDC dc(this);

	const wxSize size = GetClientSize();

	dc.SetBrush(GetOwner()->GetBackgroundColour());
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(size);

	if (IsEmpty())
	{
		// No items to draw.
		return;
	}

	// prepare the DC
	GetOwner()->PrepareDC(dc);
	dc.SetFont(GetFont());

	wxRect update = GetUpdateRegion().GetBox();
	m_owner->CalcUnscrolledPosition(update.x, update.y, &update.x, &update.y);

	// compute which items needs to be redrawn
	unsigned int item_start = GetLineAt(wxMax(0, update.y));
	unsigned int item_count =
		wxMin((int)(GetLineAt(wxMax(0, update.y + update.height)) - item_start + 1),
			(int)(GetRowCount() - item_start));
	unsigned int item_last = item_start + item_count;

	// Send the event to wxDataViewExtCtrl itself.
	wxDataViewExtEvent cache_event(wxEVT_DATAVIEW_CACHE_HINT, m_owner, NULL);
	cache_event.SetCache(item_start, item_last - 1);
	m_owner->ProcessWindowEvent(cache_event);

	// compute which columns needs to be redrawn
	unsigned int cols = GetOwner()->GetColumnCount();
	if (!cols)
	{
		// we assume that we have at least one column below and painting an
		// empty control is unnecessary anyhow
		return;
	}

	unsigned int col_start = 0;
	unsigned int x_start;
	for (x_start = 0; col_start < cols; col_start++)
	{
		wxDataViewExtColumn* col = GetOwner()->GetColumnAt(col_start);
		if (col->IsHidden())
			continue;      // skip it!

		unsigned int w = col->GetWidth();
		if (x_start + w >= (unsigned int)update.x)
			break;

		x_start += w;
	}

	unsigned int col_last = col_start;
	unsigned int x_last = x_start;
	for (; col_last < cols; col_last++)
	{
		wxDataViewExtColumn* col = GetOwner()->GetColumnAt(col_last);
		if (col->IsHidden())
			continue;      // skip it!

		if (x_last > (unsigned int)update.GetRight())
			break;

		x_last += col->GetWidth();
	}

	// Instead of calling GetLineStart() for each line from the first to the
	// last one, we will compute the starts of the lines as we iterate over
	// them starting from this one, as this is much more efficient when using
	// wxDV_VARIABLE_LINE_HEIGHT (and doesn't really change anything when not
	// using it, so there is no need to use two different approaches).
	const unsigned int first_line_start = GetLineStart(item_start);

	// Draw background of alternate rows specially if required
	if (m_owner->HasFlag(wxDV_ROW_LINES))
	{
		wxColour altRowColour = m_owner->m_alternateRowColour;
		if (!altRowColour.IsOk())
		{
			// Determine the alternate rows colour automatically from the
			// background colour.
			const wxColour bgColour = m_owner->GetBackgroundColour();

			// Depending on the background, alternate row color
			// will be 3% more dark or 50% brighter.
			int alpha = bgColour.GetRGB() > 0x808080 ? 97 : 150;
			altRowColour = bgColour.ChangeLightness(alpha);
		}

		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.SetBrush(wxBrush(altRowColour));

		// We only need to draw the visible part, so limit the rectangle to it.
		const int xRect = m_owner->CalcUnscrolledPosition(wxPoint(0, 0)).x;
		const int widthRect = size.x;
		unsigned int cur_line_start = first_line_start;
		for (unsigned int item = item_start; item < item_last; item++)
		{
			const int h = GetLineHeight(item);
			if (item % 2)
			{
				dc.DrawRectangle(xRect, cur_line_start, widthRect, h);
			}
			cur_line_start += h;
		}
	}

	// Draw horizontal rules if required
	if (m_owner->HasFlag(wxDV_HORIZ_RULES))
	{
		dc.SetPen(m_penRule);
		dc.SetBrush(*wxTRANSPARENT_BRUSH);

		unsigned int cur_line_start = first_line_start;
		for (unsigned int i = item_start; i <= item_last; i++)
		{
			const int h = GetLineHeight(i);
			dc.DrawLine(x_start, cur_line_start, x_last, cur_line_start);
			cur_line_start += h;
		}
	}

	// Draw vertical rules if required
	if (m_owner->HasFlag(wxDV_VERT_RULES))
	{
		dc.SetPen(m_penRule);
		dc.SetBrush(*wxTRANSPARENT_BRUSH);

		// NB: Vertical rules are drawn in the last pixel of a column so that
		//     they align perfectly with native MSW wxHeaderGenericCtrl as well as for
		//     consistency with MSW native list control. There's no vertical
		//     rule at the most-left side of the control.

		int x = x_start - 1;
		int line_last = GetLineStart(item_last);
		for (unsigned int i = col_start; i < col_last; i++)
		{
			wxDataViewExtColumn* col = GetOwner()->GetColumnAt(i);
			if (col->IsHidden())
				continue;       // skip it

			x += col->GetWidth();

			dc.DrawLine(x, first_line_start,
				x, line_last);
		}
	}

	// redraw the background for the items which are selected/current
	unsigned int cur_line_start = first_line_start;
	for (unsigned int item = item_start; item < item_last; item++)
	{
		bool selected = m_selection.IsSelected(item);
		const int line_height = GetLineHeight(item);

		if (selected || item == m_currentRow)
		{
			wxRect rowRect(x_start, cur_line_start,
				x_last - x_start, line_height);

			bool renderColumnFocus = false;

			int flags = wxCONTROL_SELECTED;
			if (m_hasFocus)
				flags |= wxCONTROL_FOCUSED;

			// draw keyboard focus rect if applicable
			if (item == m_currentRow && m_hasFocus)
			{
				if (m_useCellFocus && m_currentCol && m_currentColSetByKeyboard)
				{
					renderColumnFocus = true;

					// If there is just a single value, render full-row focus:
					if (!IsList())
					{
						wxDataViewExtTreeNode* node = GetTreeNodeByRow(item);
						if (IsItemSingleValued(node->GetItem()))
							renderColumnFocus = false;
					}
				}

				if (renderColumnFocus)
				{
					wxRect colRect(rowRect);

					for (unsigned int i = col_start; i < col_last; i++)
					{
						wxDataViewExtColumn* col = GetOwner()->GetColumnAt(i);
						if (col->IsHidden())
							continue;

						colRect.width = col->GetWidth();

						if (col == m_currentCol)
						{
							// Draw selection rect left of column
							{
								wxRect clipRect(rowRect);
								clipRect.width = colRect.x;

								wxDCClipper clip(dc, clipRect);
								wxRendererNative::Get().DrawItemSelectionRect
								(
									this,
									dc,
									rowRect,
									flags
								);
							}

							// Draw selection rect right of column
							{
								wxRect clipRect(rowRect);
								clipRect.x = colRect.x + colRect.width;
								clipRect.width = rowRect.width - clipRect.x;

								wxDCClipper clip(dc, clipRect);
								wxRendererNative::Get().DrawItemSelectionRect
								(
									this,
									dc,
									rowRect,
									flags
								);
							}

							// Draw column selection rect
							wxRendererNative::Get().DrawItemSelectionRect
							(
								this,
								dc,
								colRect,
								flags | wxCONTROL_CURRENT | wxCONTROL_CELL
							);

							break;
						}

						colRect.x += colRect.width;
					}
				}
				else // Not using column focus.
				{
					flags |= wxCONTROL_CURRENT | wxCONTROL_FOCUSED;

					// We still need to show the current item if it's not
					// selected.
					if (!selected)
					{
						wxRendererNative::Get().DrawFocusRect
						(
							this,
							dc,
							rowRect,
							flags
						);
					}
					//else: The current item is selected, will be drawn below.
				}
			}

			// draw selection and whole-item focus:
			if (selected && !renderColumnFocus)
			{
				if (m_selectionMode == wxDataViewExtSelectionMode::wxDataViewExtSelectRow)
				{
					wxRendererNative::Get().DrawItemSelectionRect
					(
						this,
						dc,
						rowRect,
						flags
					);
				}
				else if (m_selectionMode == wxDataViewExtSelectionMode::wxDataViewExtSelectCell)
				{
					wxRect colRect(rowRect);

					for (unsigned int i = col_start; i < col_last; i++)
					{
						wxDataViewExtColumn* col = GetOwner()->GetColumnAt(i);
						if (col->IsHidden())
							continue;

						wxDataViewExtRenderer* cell = col->GetRenderer();

						colRect.width = col->GetWidth();

						if (col == m_currentCol || m_currentCol == nullptr)
						{
							// Draw column selection rect
							wxRendererNative::Get().DrawItemSelectionRect
							(
								this,
								dc,
								colRect,
								flags
							);

							break;
						}

						colRect.x += colRect.width;
					}
				}
				else
				{
					wxRendererNative::Get().DrawItemSelectionRect
					(
						this,
						dc,
						rowRect,
						flags
					);
				}
			}
		}
		cur_line_start += line_height;
	}

#if wxUSE_DRAG_AND_DROP
	wxRect dropItemRect;

	if (m_dropItemInfo.m_hint == DropHint_Inside)
	{
		int rect_y = GetLineStart(m_dropItemInfo.m_row);
		int rect_h = GetLineHeight(m_dropItemInfo.m_row);
		wxRect rect(x_start, rect_y, x_last - x_start, rect_h);

		wxRendererNative::Get().DrawItemSelectionRect(this, dc, rect, wxCONTROL_SELECTED | wxCONTROL_FOCUSED);
	}
#endif // wxUSE_DRAG_AND_DROP

	wxDataViewExtColumn* const
		expander = GetExpanderColumnOrFirstOne(GetOwner());

	// redraw all cells for all rows which must be repainted and all columns
	wxRect cell_rect;
	cell_rect.x = x_start;

	for (unsigned int i = col_start; i < col_last; i++)
	{
		wxDataViewExtColumn* col = GetOwner()->GetColumnAt(i);
		if (col->IsHidden())
			continue;       // skip it!

		wxDataViewExtRenderer* cell = col->GetRenderer();
		cell_rect.width = col->GetWidth();
		if (cell_rect.width <= 0)
			continue;

		cell_rect.y = first_line_start;

		for (unsigned int item = item_start; item < item_last; item++)
		{
			// get the cell value and set it into the renderer
			wxDataViewExtTreeNode* node = NULL;
			wxDataViewExtItem dataitem;
			const int line_height = GetLineHeight(item);

			wxDataViewExtTreeNodeViewMode datatype =
				wxDataViewExtTreeNodeViewMode::wxDataViewExtCell;

			if (!IsVirtualList())
			{
				node = GetTreeNodeByRow(item);
				if (node == NULL)
				{
					cell_rect.y += line_height;
					continue;
				}

				dataitem = node->GetItem();
				datatype = node->GetViewMode();
			}
			else
			{
				dataitem = wxDataViewExtItem(wxUIntToPtr(item + 1));
			}

			// update cell_rect
			cell_rect.height = line_height;

			bool selected = m_selectionMode == wxDataViewExtSelectCell
				? m_selection.IsSelected(item) && col == m_currentCol : m_selection.IsSelected(item);

			int state = 0;
			if (selected)
				state |= wxDATAVIEW_CELL_SELECTED;

			cell->SetState(state);
			const bool hasValue = cell->PrepareForItem(model, dataitem, col->GetModelColumn());

			// draw the background
			if (!selected)
				DrawCellBackground(cell, dc, cell_rect, datatype);

			// deal with the expander
			int indent = 0;
			if ((!IsList()) && (col == expander))
			{
				// Calculate the indent first
				if (m_viewMode == wxDataViewExtViewMode::wxDataViewExtTree)
					indent = GetOwner()->GetIndent() * node->GetIndentLevel();

				// Get expander size
				wxSize expSize = wxRendererNative::Get().GetExpanderSize(this);

				// draw expander if needed
				if (node->HasChildren())
				{
					wxRect rect = cell_rect;
					rect.x += indent;
					rect.y += (cell_rect.GetHeight() - expSize.GetHeight()) / 2; // center vertically
					rect.width = expSize.GetWidth();
					rect.height = expSize.GetHeight();

					int flag = 0;
					if (m_underMouse == node)
						flag |= wxCONTROL_CURRENT;
					if (node->IsOpen())
						flag |= wxCONTROL_EXPANDED;

					// ensure that we don't overflow the cell (which might
					// happen if the column is very narrow)
					wxDCClipper clip(dc, cell_rect);

					wxRendererNative::Get().DrawTreeItemButton(this, dc, rect, flag);
				}

				indent += expSize.GetWidth();

				// force the expander column to left-center align
				cell->SetAlignment(wxALIGN_CENTER_VERTICAL);

#if wxUSE_DRAG_AND_DROP
				if (item == m_dropItemInfo.m_row)
				{
					dropItemRect = cell_rect;
					dropItemRect.x += expSize.GetWidth();
					dropItemRect.width -= expSize.GetWidth();
					if (m_dropItemInfo.m_indentLevel >= 0)
					{
						int hintIndent = GetOwner()->GetIndent() * m_dropItemInfo.m_indentLevel;
						dropItemRect.x += hintIndent;
						dropItemRect.width -= hintIndent;
					}
				}
#endif
			}

			wxRect item_rect = cell_rect;
			item_rect.Deflate(FromDIP(PADDING_RIGHTLEFT), 0);

			// account for the tree indent (harmless if we're not indented)
			item_rect.x += indent;
			item_rect.width -= indent;

			if (item_rect.width <= 0)
			{
				cell_rect.y += line_height;
				continue;
			}

			// TODO: it would be much more efficient to create a clipping
			//       region for the entire column being rendered (in the OnPaint
			//       of wxDataViewExtMainWindow) instead of a single clip region for
			//       each cell. However it would mean that each renderer should
			//       respect the given wxRect's top & bottom coords, eventually
			//       violating only the left & right coords - however the user can
			//       make its own renderer and thus we cannot be sure of that.
			wxDCClipper clip(dc, item_rect);

			if (hasValue)
				cell->WXCallRender(item_rect, &dc, state);

			cell_rect.y += line_height;
		}

		cell_rect.x += cell_rect.width;
	}

#if wxUSE_DRAG_AND_DROP
	if (m_dropItemInfo.m_hint == DropHint_Below || m_dropItemInfo.m_hint == DropHint_Above)
	{
		const int insertLineHeight = 2;     // TODO: setup (should be even)

		int rect_y = dropItemRect.y - insertLineHeight / 2;     // top insert
		if (m_dropItemInfo.m_hint == DropHint_Below)
			rect_y += dropItemRect.height;                    // bottom insert

		wxRect rect(dropItemRect.x, rect_y, dropItemRect.width, insertLineHeight);
		wxRendererNative::Get().DrawItemSelectionRect(this, dc, rect, wxCONTROL_SELECTED);
	}
#endif // wxUSE_DRAG_AND_DROP
}


void wxDataViewExtMainWindow::DrawCellBackground(wxDataViewExtRenderer* cell, wxDC& dc, const wxRect& rect, wxDataViewExtTreeNodeViewMode mode)
{
	wxRect rectBg(rect);

	// don't overlap the horizontal rules
	if (m_owner->HasFlag(wxDV_HORIZ_RULES))
	{
		rectBg.y++;
		rectBg.height--;
	}

	// don't overlap the vertical rules
	if (m_owner->HasFlag(wxDV_VERT_RULES))
	{
		// same note as in OnPaint handler above
		// NB: Vertical rules are drawn in the last pixel of a column so that
		//     they align perfectly with native MSW wxHeaderGenericCtrl as well as for
		//     consistency with MSW native list control. There's no vertical
		//     rule at the most-left side of the control.
		rectBg.width--;
	}

	if (mode == wxDataViewExtTreeNodeViewMode::wxDataViewExtHierarchy)
	{
		const wxColour& colour = wxColour(197, 220, 232);
		wxDCPenChanger changePen(dc, colour);
		wxDCBrushChanger changeBrush(dc, colour);

		dc.DrawRectangle(rect);
		return; 
	}

	cell->RenderBackground(&dc, rectBg);
}

void wxDataViewExtMainWindow::OnRenameTimer()
{
	// We have to call this here because changes may just have
	// been made and no screen update taken place.
	if (m_dirty)
	{
		// TODO: use wxTheApp->SafeYieldFor(NULL, wxEVT_CATEGORY_UI) instead
		//       (needs to be tested!)
		wxSafeYield();
	}

	wxDataViewExtItem item = GetItemByRow(m_currentRow);

	StartEditing(item, m_currentCol);
}

void
wxDataViewExtMainWindow::StartEditing(const wxDataViewExtItem& item,
	const wxDataViewExtColumn* col)
{
	wxDataViewExtRenderer* renderer = col->GetRenderer();
	if (!IsCellEditableInMode(item, col, wxDATAVIEW_CELL_EDITABLE))
		return;

	const wxRect itemRect = GetItemRect(item, col);
	if (renderer->StartEditing(item, itemRect))
	{
		renderer->NotifyEditingStarted(item);

		// Save the renderer to be able to finish/cancel editing it later and
		// save the control to be able to detect if we're still editing it.
		m_editorRenderer = renderer;
		m_editorCtrl = renderer->GetEditorCtrl();
	}
}

void wxDataViewExtMainWindow::FinishEditing()
{
	if (m_editorCtrl)
	{
		m_editorRenderer->FinishEditing();
	}
}

void wxDataViewExtHeaderWindow::FinishEditing()
{
	wxDataViewExtMainWindow* win = static_cast<wxDataViewExtMainWindow*>(GetOwner()->GetMainWindow());
	win->FinishEditing();
}

//-----------------------------------------------------------------------------
// Helper class for do operation on the tree node
//-----------------------------------------------------------------------------

namespace
{

	class DoJob
	{
	public:
		DoJob() {}
		virtual ~DoJob() {}

		// The return value control how the tree-walker tranverse the tree
		enum
		{
			DONE,          // Job done, stop traversing and return
			SKIP_SUBTREE,  // Ignore the current node's subtree and continue
			CONTINUE       // Job not done, continue
		};

		virtual int operator() (wxDataViewExtTreeNode* node) = 0;
	};

	bool
		Walker(wxDataViewExtTreeNode* node, DoJob& func, WalkFlags flags = Walk_All)
	{
		wxCHECK_MSG(node, false, "can't walk NULL node");

		switch (func(node))
		{
		case DoJob::DONE:
			return true;
		case DoJob::SKIP_SUBTREE:
			return false;
		case DoJob::CONTINUE:
			break;
		}

		if (node->HasChildren() && (flags != Walk_ExpandedOnly || node->IsOpen()))
		{
			const wxDataViewExtTreeNodes& nodes = node->GetChildNodes();

			for (wxDataViewExtTreeNodes::const_iterator i = nodes.begin();
				i != nodes.end();
				++i)
			{
				if (Walker(*i, func, flags))
					return true;
			}
		}

		return false;
	}

} // anonymous namespace

bool wxDataViewExtMainWindow::ItemAdded(const wxDataViewExtItem& parent, const wxDataViewExtItem& item)
{
	// Send event
	wxDataViewExtEvent le(wxEVT_DATAVIEW_ITEM_START_INSERTING, m_owner, GetCurrentColumn(), item);
	if (m_owner->ProcessWindowEvent(le) || !le.IsAllowed())
		return false;

	if (IsVirtualList())
	{
		wxDataViewExtVirtualListModel* list_model =
			(wxDataViewExtVirtualListModel*)GetModel();
		m_count = list_model->GetCount();
	}
	else
	{
		// specific position (row) is unclear, so clear whole height cache
		ClearRowHeightCache();

		const FindNodeResult findResult = FindNode(parent);
		wxDataViewExtTreeNode* parentNode = findResult.m_node;

		// If one of parents is not realized yet (has children but was never
		// expanded). Return as nodes will be initialized in Expand().
		if (!findResult.m_subtreeRealized)
			return true;

		// The parent node was not found.
		if (!parentNode)
			return false;

		// If the parent has not children then just mask it as container and return.
		// Nodes will be initialized in Expand().
		if (!parentNode->HasChildren())
		{
			parentNode->SetHasChildren(true);
			return true;
		}

		// If the parent has children but child nodes was not initialized and
		// the node is collapsed then just return as nodes will be initialized in
		// Expand().
		if (!parentNode->IsOpen() && parentNode->GetChildNodes().empty())
			return true;

		parentNode->SetHasChildren(true);

		wxDataViewExtTreeNode* itemNode = new wxDataViewExtTreeNode(parentNode, item);
		itemNode->SetHasChildren(GetModel()->IsContainer(item));

		if (GetSortOrder().IsNone())
		{
			// There's no sorting, so we need to select an insertion position

			wxDataViewExtItemArray modelSiblings;
			GetModel()->GetChildren(parent, modelSiblings);
			const int modelSiblingsSize = modelSiblings.size();

			int posInModel = modelSiblings.Index(item, /*fromEnd=*/true);
			wxCHECK_MSG(posInModel != wxNOT_FOUND, false, "adding non-existent item?");


			const wxDataViewExtTreeNodes& nodeSiblings = parentNode->GetChildNodes();
			const int nodeSiblingsSize = nodeSiblings.size();

			int nodePos = 0;

			if (posInModel == modelSiblingsSize - 1)
			{
				nodePos = nodeSiblingsSize;
			}
			else if (modelSiblingsSize == nodeSiblingsSize + 1)
			{
				// This is the simple case when our node tree already matches the
				// model and only this one item is missing.
				nodePos = posInModel;
			}
			else
			{
				// It's possible that a larger discrepancy between the model and
				// our realization exists. This can happen e.g. when adding a bunch
				// of items to the model and then calling ItemsAdded() just once
				// afterwards. In this case, we must find the right position by
				// looking at sibling items.

				// append to the end if we won't find a better position:
				nodePos = nodeSiblingsSize;

				for (int nextItemPos = posInModel + 1;
					nextItemPos < modelSiblingsSize;
					nextItemPos++)
				{
					int nextNodePos = parentNode->FindChildByItem(modelSiblings[nextItemPos]);
					if (nextNodePos != wxNOT_FOUND)
					{
						nodePos = nextNodePos;
						break;
					}
				}
			}
			parentNode->ChangeSubTreeCount(+1);
			parentNode->InsertChild(this, itemNode, nodePos);
		}
		else
		{
			// Node list is or will be sorted, so InsertChild do not need insertion position
			parentNode->ChangeSubTreeCount(+1);
			parentNode->InsertChild(this, itemNode, 0);
		}

		InvalidateCount();
	}

	m_selection.OnItemsInserted(GetRowByItem(item), 1);

	GetOwner()->InvalidateColBestWidths();
	UpdateDisplay();

	return true;
}

bool wxDataViewExtMainWindow::ItemDeleted(const wxDataViewExtItem& parent,
	const wxDataViewExtItem& item)
{
	// Send event
	wxDataViewExtEvent le(wxEVT_DATAVIEW_ITEM_START_DELETING, m_owner, GetCurrentColumn(), item);
	if (m_owner->ProcessWindowEvent(le) || !le.IsAllowed())
		return false;

	if (IsVirtualList())
	{
		wxDataViewExtVirtualListModel* list_model =
			(wxDataViewExtVirtualListModel*)GetModel();
		m_count = list_model->GetCount();

		m_selection.OnItemDelete(GetRowByItem(item));
	}
	else // general case
	{
		const FindNodeResult findResult = FindNode(parent);
		wxDataViewExtTreeNode* parentNode = findResult.m_node;

		// One of parents of the parent node has children but was never
		// expanded, so the tree was not built and we have nothing to delete.
		if (!findResult.m_subtreeRealized)
			return true;

		// Notice that it is possible that the item being deleted is not in the
		// tree at all, for example we could be deleting a never shown (because
		// collapsed) item in a tree model. So it's not an error if we don't know
		// about this item, just return without doing anything then.
		if (!parentNode)
			return true;

		wxCHECK_MSG(parentNode->HasChildren(), false, "parent node doesn't have children?");
		const wxDataViewExtTreeNodes& parentsChildren = parentNode->GetChildNodes();

		// We can't use FindNode() to find 'item', because it was already
		// removed from the model by the time ItemDeleted() is called, so we
		// have to do it manually. We keep track of its position as well for
		// later use.
		int itemPosInNode = 0;
		wxDataViewExtTreeNode* itemNode = NULL;
		for (wxDataViewExtTreeNodes::const_iterator i = parentsChildren.begin();
			i != parentsChildren.end();
			++i, ++itemPosInNode)
		{
			if ((*i)->GetItem() == item)
			{
				itemNode = *i;
				break;
			}
		}

		// If the parent wasn't expanded, it's possible that we didn't have a
		// node corresponding to 'item' and so there's nothing left to do.
		if (!itemNode)
		{
			// If this was the last child to be removed, it's possible the parent
			// node became a leaf. Let's ask the model about it.
			if (parentNode->GetChildNodes().empty())
				parentNode->SetHasChildren(GetModel()->IsContainer(parent));

			return true;
		}

		if (m_rowHeightCache)
			m_rowHeightCache->Remove(GetRowByItem(parent) + itemPosInNode);

		// Delete the item from wxDataViewExtTreeNode representation:
		const int itemsDeleted = 1 + itemNode->GetSubTreeCount();

		parentNode->RemoveChild(itemPosInNode);
		delete itemNode;
		parentNode->ChangeSubTreeCount(-itemsDeleted);

		// Make the row number invalid and get a new valid one when user call GetRowCount
		InvalidateCount();

		// If this was the last child to be removed, it's possible the parent
		// node became a leaf. Let's ask the model about it.
		if (parentNode->GetChildNodes().empty())
		{
			bool isContainer = GetModel()->IsContainer(parent);
			parentNode->SetHasChildren(isContainer);
			if (isContainer)
			{
				// If it's still a container, make sure we show "+" icon for it
				// and not "-" one as there is nothing to collapse any more.
				if (parentNode->IsOpen())
					parentNode->ToggleOpen(this);
			}
		}

		// Update selection by removing 'item' and its entire children tree from the selection.
		if (!m_selection.IsEmpty())
		{
			// we can't call GetRowByItem() on 'item', as it's already deleted, so compute it from
			// the parent ('parentNode') and position in its list of children
			int itemRow;
			if (itemPosInNode == 0)
			{
				// 1st child, row number is that of the parent parentNode + 1
				itemRow = GetRowByItem(parentNode->GetItem()) + 1;
			}
			else
			{
				// row number is that of the sibling above 'item' + its subtree if any + 1
				const wxDataViewExtTreeNode* siblingNode = parentNode->GetChildNodes()[itemPosInNode - 1];

				itemRow = GetRowByItem(siblingNode->GetItem()) +
					siblingNode->GetSubTreeCount() +
					1;
			}

			m_selection.OnItemsDeleted(itemRow, itemsDeleted);
		}
	}

	// Change the current row to the last row if the current exceed the max row number
	if (HasCurrentRow() && m_currentRow >= GetRowCount())
		ChangeCurrentRow(m_count - 1);

	GetOwner()->InvalidateColBestWidths();
	UpdateDisplay();

	return true;
}

bool wxDataViewExtMainWindow::DoItemChanged(const wxDataViewExtItem& item, int view_column)
{
	if (!IsVirtualList())
	{
		if (m_rowHeightCache)
			m_rowHeightCache->Remove(GetRowByItem(item));

		// Move this node to its new correct place after it was updated.
		//
		// In principle, we could skip the call to PutInSortOrder() if the modified
		// column is not the sort column, but in real-world applications it's fully
		// possible and likely that custom compare uses not only the selected model
		// column but also falls back to other values for comparison. To ensure
		// consistency it is better to treat a value change as if it was an item
		// change.
		const FindNodeResult findResult = FindNode(item);
		wxDataViewExtTreeNode* const node = findResult.m_node;
		if (!findResult.m_subtreeRealized)
			return true;
		wxCHECK_MSG(node, false, "invalid item");
		node->PutInSortOrder(this);
	}

	wxDataViewExtColumn* column;
	if (view_column == wxNOT_FOUND)
	{
		column = NULL;
		GetOwner()->InvalidateColBestWidths();
	}
	else
	{
		column = m_owner->GetColumn(view_column);
		GetOwner()->InvalidateColBestWidth(view_column);
	}

	// Update the displayed value(s).
	RefreshRow(GetRowByItem(item));

	// Send event
	wxDataViewExtEvent le(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, m_owner, column, item);
	m_owner->ProcessWindowEvent(le);

	return true;
}

bool wxDataViewExtMainWindow::ValueChanged(const wxDataViewExtItem& item, unsigned int model_column)
{
	int view_column = m_owner->GetModelColumnIndex(model_column);
	if (view_column == wxNOT_FOUND)
		return false;

	return DoItemChanged(item, view_column);
}

bool wxDataViewExtMainWindow::Cleared()
{
	DestroyTree();
	m_selection.Clear();
	m_currentRow = (unsigned)-1;

	ClearRowHeightCache();

	if (GetModel())
	{
		BuildTree(GetModel());
	}
	else
	{
		m_count = 0;
	}

	GetOwner()->InvalidateColBestWidths();
	UpdateDisplay();

	return true;
}

void wxDataViewExtMainWindow::UpdateDisplay()
{
	m_dirty = true;
	m_underMouse = NULL;
}

void wxDataViewExtMainWindow::OnInternalIdle()
{
	wxWindow::OnInternalIdle();

	if (m_dirty)
	{
		RecalculateDisplay();
		m_dirty = false;
	}
}

void wxDataViewExtMainWindow::RecalculateDisplay()
{
	wxDataViewExtModel* model = GetModel();
	if (!model)
	{
		Refresh();
		return;
	}

	int width = GetEndOfLastCol();
	int height = GetLineStart(GetRowCount());

	SetVirtualSize(width, height);
	GetOwner()->SetScrollRate(FromDIP(10), m_lineHeight);
	UpdateColumnSizes();

	Refresh();
}

void wxDataViewExtMainWindow::ScrollWindow(int dx, int dy, const wxRect* rect)
{
	m_underMouse = NULL;
	m_owner->ScrollWindow(dx, dy, rect);
}

void wxDataViewExtMainWindow::ScrollTo(int rows, int column)
{
	m_underMouse = NULL;

	int x, y;
	m_owner->GetScrollPixelsPerUnit(&x, &y);

	// Take care to not divide by 0 if we're somehow called before scrolling
	// parameters are initialized.
	int sy = y ? GetLineStart(rows) / y : -1;
	int sx = -1;
	if (column != -1 && x)
		sx = GetColumnStart(column) / x;
	m_owner->Scroll(sx, sy);
}

int wxDataViewExtMainWindow::GetCountPerPage() const
{
	wxSize size = GetClientSize();
	return size.y / m_lineHeight;
}

wxDataViewExtItem wxDataViewExtMainWindow::GetTopItem() const
{
	unsigned int item = GetFirstVisibleRow();
	wxDataViewExtTreeNode* node = NULL;
	wxDataViewExtItem dataitem;

	if (!IsVirtualList())
	{
		node = GetTreeNodeByRow(item);
		if (node == NULL) return wxDataViewExtItem(0);

		dataitem = node->GetItem();
	}
	else
	{
		dataitem = wxDataViewExtItem(wxUIntToPtr(item + 1));
	}

	return dataitem;
}

int wxDataViewExtMainWindow::GetEndOfLastCol() const
{
	int width = 0;
	unsigned int i;
	for (i = 0; i < GetOwner()->GetColumnCount(); i++)
	{
		const wxDataViewExtColumn* c =
			const_cast<wxDataViewExtCtrl*>(GetOwner())->GetColumnAt(i);

		if (!c->IsHidden())
			width += c->GetWidth();
	}
	return width;
}

int wxDataViewExtMainWindow::GetColumnStart(int column) const
{
	wxASSERT(column >= 0);
	int sx = -1;

	wxRect rect = GetClientRect();
	int colnum = 0;
	int x_start, w = 0;
	int xx, yy, xe;
	m_owner->CalcUnscrolledPosition(rect.x, rect.y, &xx, &yy);
	for (x_start = 0; colnum < column; colnum++)
	{
		wxDataViewExtColumn* col = GetOwner()->GetColumnAt(colnum);
		if (col->IsHidden())
			continue;      // skip it!

		w = col->GetWidth();
		x_start += w;
	}

	int x_end = x_start + w;
	xe = xx + rect.width;
	if (x_end > xe)
	{
		sx = (xx + x_end - xe);
	}
	if (x_start < xx)
	{
		sx = x_start;
	}
	return sx;
}

unsigned int wxDataViewExtMainWindow::GetFirstVisibleRow() const
{
	int x = 0;
	int y = 0;
	m_owner->CalcUnscrolledPosition(x, y, &x, &y);

	return GetLineAt(y);
}

unsigned int wxDataViewExtMainWindow::GetLastVisibleRow()
{
	wxSize client_size = GetClientSize();
	// Find row occupying the bottom line of the client area (dimY-1).
	m_owner->CalcUnscrolledPosition(client_size.x, client_size.y - 1,
		&client_size.x, &client_size.y);
	unsigned int row = GetLineAt(client_size.y);

	return wxMin(GetRowCount() - 1, row);
}

unsigned int wxDataViewExtMainWindow::GetLastFullyVisibleRow()
{
	unsigned int row = GetLastVisibleRow();

	int bottom = GetLineStart(row) + GetLineHeight(row);
	m_owner->CalcScrolledPosition(-1, bottom, NULL, &bottom);

	if (bottom > GetClientSize().y)
		return wxMax(0, row - 1);
	else
		return row;
}

unsigned int wxDataViewExtMainWindow::GetRowCount() const
{
	if (m_count == -1)
	{
		wxDataViewExtMainWindow* const
			self = const_cast<wxDataViewExtMainWindow*>(this);
		self->UpdateCount(RecalculateCount());
		self->UpdateDisplay();
	}
	return m_count;
}

void wxDataViewExtMainWindow::ChangeCurrentRow(unsigned int row)
{
	m_currentRow = row;

	// send event
#if wxUSE_ACCESSIBILITY
	wxAccessible::NotifyEvent(wxACC_EVENT_OBJECT_FOCUS, m_owner, wxOBJID_CLIENT, m_currentRow + 1);
#endif // wxUSE_ACCESSIBILITY
}

bool wxDataViewExtMainWindow::UnselectAllRows(unsigned int except)
{
	if (!m_selection.IsEmpty())
	{
		for (unsigned i = GetFirstVisibleRow(); i <= GetLastVisibleRow(); i++)
		{
			if (m_selection.IsSelected(i) && i != except)
				RefreshRow(i);
		}

		if (except != (unsigned int)-1)
		{
			const bool wasSelected = m_selection.IsSelected(except);
			ClearSelection();
			if (wasSelected)
			{
				m_selection.SelectItem(except);

				// The special item is still selected.
				return false;
			}
		}
		else
		{
			ClearSelection();
		}
	}

	// There are no selected items left.
	return true;
}

void wxDataViewExtMainWindow::SelectRow(unsigned int row, bool on)
{
	if (m_selection.SelectItem(row, on))
		RefreshRow(row);
}

void wxDataViewExtMainWindow::SelectRows(unsigned int from, unsigned int to)
{
	wxArrayInt changed;
	if (m_selection.SelectRange(from, to, true, &changed))
	{
		for (unsigned i = 0; i < changed.size(); i++)
			RefreshRow(changed[i]);
	}
	else // Selection of too many rows has changed.
	{
		RefreshRows(from, to);
	}
}

void wxDataViewExtMainWindow::Select(const wxArrayInt& aSelections)
{
	for (size_t i = 0; i < aSelections.GetCount(); i++)
	{
		int n = aSelections[i];

		if (m_selection.SelectItem(n))
			RefreshRow(n);
	}
}

void wxDataViewExtMainWindow::ReverseRowSelection(unsigned int row)
{
	m_selection.SelectItem(row, !m_selection.IsSelected(row));
	RefreshRow(row);
}

bool wxDataViewExtMainWindow::IsRowSelected(unsigned int row)
{
	return m_selection.IsSelected(row);
}

void wxDataViewExtMainWindow::SendSelectionChangedEvent(const wxDataViewExtItem& item)
{
#if wxUSE_ACCESSIBILITY
	wxAccessible::NotifyEvent(wxACC_EVENT_OBJECT_SELECTIONWITHIN, m_owner, wxOBJID_CLIENT, wxACC_SELF);
#endif // wxUSE_ACCESSIBILITY

	wxDataViewExtEvent le(wxEVT_DATAVIEW_SELECTION_CHANGED, m_owner, item);
	m_owner->ProcessWindowEvent(le);
}

void wxDataViewExtMainWindow::RefreshRows(unsigned int from, unsigned int to)
{
	wxRect rect = GetLinesRect(from, to);

	m_owner->CalcScrolledPosition(rect.x, rect.y, &rect.x, &rect.y);

	wxSize client_size = GetClientSize();
	wxRect client_rect(0, 0, client_size.x, client_size.y);
	wxRect intersect_rect = client_rect.Intersect(rect);
	if (!intersect_rect.IsEmpty())
		Refresh(true, &intersect_rect);
}

void wxDataViewExtMainWindow::RefreshRowsAfter(unsigned int firstRow)
{
	wxSize client_size = GetClientSize();
	int start = GetLineStart(firstRow);
	m_owner->CalcScrolledPosition(start, 0, &start, NULL);
	if (start > client_size.y) return;

	wxRect rect(0, start, client_size.x, client_size.y - start);

	Refresh(true, &rect);
}

wxRect wxDataViewExtMainWindow::GetLinesRect(unsigned int rowFrom, unsigned int rowTo) const
{
	if (rowFrom > rowTo)
		wxSwap(rowFrom, rowTo);

	wxRect rect;
	rect.x = 0;
	rect.y = GetLineStart(rowFrom);
	// Don't calculate exact width of the row, because GetEndOfLastCol() is
	// expensive to call, and controls with rows not spanning entire width rare.
	// It is more efficient to e.g. repaint empty parts of the window needlessly.
	rect.width = INT_MAX;
	if (rowFrom == rowTo)
		rect.height = GetLineHeight(rowFrom);
	else
		rect.height = GetLineStart(rowTo) - rect.y + GetLineHeight(rowTo);
	return rect;
}

int wxDataViewExtMainWindow::GetLineStart(unsigned int row) const
{
	// check for the easy case first
	if (!m_rowHeightCache || !GetOwner()->HasFlag(wxDV_VARIABLE_LINE_HEIGHT))
		return row * m_lineHeight;

	int start = 0;
	if (m_rowHeightCache->GetLineStart(row, start))
		return start;

	unsigned int r;
	for (r = 0; r < row; r++)
	{
		int height = 0;
		if (!m_rowHeightCache->GetLineHeight(r, height))
		{
			// row height not in cache -> get it from the renderer...
			wxDataViewExtItem item = GetItemByRow(r);
			if (!item)
				break;

			height = QueryAndCacheLineHeight(r, item);
		}

		start += height;
	}

	return start;
}

int wxDataViewExtMainWindow::GetLineAt(unsigned int y) const
{
	// check for the easy case first
	if (!m_rowHeightCache || !GetOwner()->HasFlag(wxDV_VARIABLE_LINE_HEIGHT))
		return y / m_lineHeight;

	unsigned int row = 0;
	if (m_rowHeightCache->GetLineAt(y, row))
		return row;

	// OnPaint asks GetLineAt for the very last y position and this is always
	// below the last item (--> an invalid item). To prevent iterating over all
	// items, check if y is below the last row.
	// Because this is done very often (for each repaint) its worth to handle
	// this special case separately.
	int height = 0;
	int start = 0;
	unsigned int rowCount = GetRowCount();
	if (rowCount == 0 ||
		(m_rowHeightCache->GetLineInfo(rowCount - 1, start, height) &&
			y >= static_cast<unsigned int>(start + height)))
	{
		return rowCount;
	}

	// sum all item heights until y is reached
	unsigned int yy = 0;
	for (;;)
	{
		height = 0;
		if (!m_rowHeightCache->GetLineHeight(row, height))
		{
			// row height not in cache -> get it from the renderer...
			wxDataViewExtItem item = GetItemByRow(row);
			if (!item)
			{
				wxASSERT(row >= GetRowCount());
				break;
			}

			height = QueryAndCacheLineHeight(row, item);
		}

		yy += height;
		if (y < yy)
			break;

		row++;
	}
	return row;
}

int wxDataViewExtMainWindow::GetLineHeight(unsigned int row) const
{
	// check for the easy case first
	if (!m_rowHeightCache || !GetOwner()->HasFlag(wxDV_VARIABLE_LINE_HEIGHT))
		return m_lineHeight;

	int height = 0;
	if (m_rowHeightCache->GetLineHeight(row, height))
		return height;

	wxDataViewExtItem item = GetItemByRow(row);
	if (!item)
		return m_lineHeight;

	height = QueryAndCacheLineHeight(row, item);
	return height;
}

int wxDataViewExtMainWindow::QueryAndCacheLineHeight(unsigned int row, wxDataViewExtItem item) const
{
	const wxDataViewExtModel* model = GetModel();
	int height = m_lineHeight;
	unsigned int cols = GetOwner()->GetColumnCount();
	unsigned int col;
	for (col = 0; col < cols; col++)
	{
		const wxDataViewExtColumn* column = GetOwner()->GetColumn(col);
		if (column->IsHidden())
			continue;      // skip it!

		if (!model->HasValue(item, col))
			continue;      // skip it!

		wxDataViewExtRenderer* renderer =
			const_cast<wxDataViewExtRenderer*>(column->GetRenderer());
		if (renderer->PrepareForItem(model, item, column->GetModelColumn()))
			height = wxMax(height, renderer->GetSize().y);
	}

	// ... and store the height in the cache
	m_rowHeightCache->Put(row, height);

	return height;
}

namespace
{
	class RowToTreeNodeJob : public DoJob
	{
	public:
		// Note that we initialize m_current to -1 because the first node passed to
		// our operator() will be the root node, which doesn't appear in the window
		// and so doesn't count as a real row.
		explicit RowToTreeNodeJob(int row)
			: m_row(row), m_current(-1), m_ret(NULL)
		{
		}

		virtual int operator() (wxDataViewExtTreeNode* node) wxOVERRIDE
		{
			if (m_current == m_row)
			{
				m_ret = node;
				return DoJob::DONE;
			}

			if (node->GetSubTreeCount() + m_current < m_row)
			{
				m_current += node->GetSubTreeCount() + 1;
				return  DoJob::SKIP_SUBTREE;
			}
			else
			{
				// If the current node has only leaf children, we can find the
				// desired node directly. This can speed up finding the node
				// in some cases, and will have a very good effect for list views.
				if (node->HasChildren() &&
					(int)node->GetChildNodes().size() == node->GetSubTreeCount())
				{
					const int index = m_row - m_current - 1;
					m_ret = node->GetChildNodes()[index];
					return DoJob::DONE;
				}

				m_current++;

				return DoJob::CONTINUE;
			}
		}

		wxDataViewExtTreeNode* GetResult() const
		{
			return m_ret;
		}

	private:
		const int m_row;
		int m_current;
		wxDataViewExtTreeNode* m_ret;
	};

}

// anonymous namespace
//
wxDataViewExtTreeNode* wxDataViewExtMainWindow::GetTreeNodeByRow(unsigned int row) const
{
	wxASSERT(!IsVirtualList());

	if (row == (unsigned)-1)
		return NULL;

	RowToTreeNodeJob job(static_cast<int>(row));
	Walker(m_root, job);
	return job.GetResult();
}

wxDataViewExtItem wxDataViewExtMainWindow::GetItemByRow(unsigned int row) const
{
	wxDataViewExtItem item;
	if (IsVirtualList())
	{
		if (row < GetRowCount())
			item = wxDataViewExtItem(wxUIntToPtr(row + 1));
	}
	else
	{
		wxDataViewExtTreeNode* node = GetTreeNodeByRow(row);
		if (node)
			item = node->GetItem();
	}

	return item;
}

bool
wxDataViewExtMainWindow::SendExpanderEvent(wxEventType type,
	const wxDataViewExtItem& item)
{
#if wxUSE_ACCESSIBILITY
	if (type == wxEVT_DATAVIEW_ITEM_EXPANDED || type == wxEVT_DATAVIEW_ITEM_COLLAPSED)
		wxAccessible::NotifyEvent(wxACC_EVENT_OBJECT_REORDER, m_owner, wxOBJID_CLIENT, wxACC_SELF);
#endif // wxUSE_ACCESSIBILITY

	wxDataViewExtEvent le(type, m_owner, item);
	return !m_owner->ProcessWindowEvent(le) || le.IsAllowed();
}

bool wxDataViewExtMainWindow::IsExpanded(unsigned int row) const
{
	if (IsList())
		return false;

	wxDataViewExtTreeNode* node = GetTreeNodeByRow(row);
	if (!node)
		return false;

	if (!node->HasChildren())
		return false;

	return node->IsOpen();
}

bool wxDataViewExtMainWindow::HasChildren(unsigned int row) const
{
	if (IsList())
		return false;

	wxDataViewExtTreeNode* node = GetTreeNodeByRow(row);
	if (!node)
		return false;

	if (!node->HasChildren())
		return false;

	return true;
}

void wxDataViewExtMainWindow::Expand(unsigned int row, bool expandChildren)
{
	if (IsList())
		return;

	wxDataViewExtTreeNode* node = GetTreeNodeByRow(row);
	if (!node)
		return;

	return DoExpand(node, row, expandChildren);
}

void
wxDataViewExtMainWindow::DoExpand(wxDataViewExtTreeNode* node,
	unsigned int row,
	bool expandChildren)
{
	if (!node->HasChildren())
		return;

	if (!node->IsOpen())
	{
		if (!SendExpanderEvent(wxEVT_DATAVIEW_ITEM_EXPANDING, node->GetItem()))
		{
			// Vetoed by the event handler.
			return;
		}

		if (m_rowHeightCache)
		{
			// Expand makes new rows visible thus we invalidates all following
			// rows in the height cache
			m_rowHeightCache->Remove(row);
		}

		node->ToggleOpen(this);

		// build the children of current node
		if (node->GetChildNodes().empty())
		{
			::BuildTreeHelper(this, GetModel(), node->GetItem(), node);
		}

		const unsigned countNewRows = node->GetSubTreeCount();

		// Shift all stored indices after this row by the number of newly added
		// rows.
		m_selection.OnItemsInserted(row + 1, countNewRows);
		if (HasCurrentRow() && m_currentRow > row)
			ChangeCurrentRow(m_currentRow + countNewRows);

		if (m_count != -1)
			m_count += countNewRows;

		// Expanding this item means the previously cached column widths could
		// have become invalid as new items are now visible.
		GetOwner()->InvalidateColBestWidths();

		UpdateDisplay();
		// Send the expanded event
		SendExpanderEvent(wxEVT_DATAVIEW_ITEM_EXPANDED, node->GetItem());
	}

	// Note that we have to expand the children when expanding recursively even
	// when this node itself was already open.
	if (expandChildren)
	{
		const wxDataViewExtTreeNodes& children = node->GetChildNodes();

		for (wxDataViewExtTreeNodes::const_iterator i = children.begin();
			i != children.end();
			++i)
		{
			wxDataViewExtTreeNode* const child = *i;

			// Row currently corresponds to the previous item, so increment it
			// first to correspond to this child.
			DoExpand(child, ++row, true);

			// We don't need +1 here because we'll increment the row during the
			// next loop iteration.
			row += child->GetSubTreeCount();
		}
	}
}

void wxDataViewExtMainWindow::Collapse(unsigned int row)
{
	if (IsList())
		return;

	wxDataViewExtTreeNode* node = GetTreeNodeByRow(row);
	if (!node)
		return;

	if (!node->HasChildren())
		return;

	if (m_rowHeightCache)
	{
		// Collapse hides rows thus we invalidates all following
		// rows in the height cache
		m_rowHeightCache->Remove(row);
	}

	if (node->IsOpen())
	{
		if (!SendExpanderEvent(wxEVT_DATAVIEW_ITEM_COLLAPSING, node->GetItem()))
		{
			// Vetoed by the event handler.
			return;
		}

		const unsigned countDeletedRows = node->GetSubTreeCount();

		if (m_selection.OnItemsDeleted(row + 1, countDeletedRows))
		{
			SendSelectionChangedEvent(GetItemByRow(row));

			// The event handler for wxEVT_DATAVIEW_SELECTION_CHANGED could
			// have called Collapse() itself, in which case the node would be
			// already closed and we shouldn't try to close it again.
			if (!node->IsOpen())
				return;
		}

		node->ToggleOpen(this);

		// Adjust the current row if necessary.
		if (HasCurrentRow() && m_currentRow > row)
		{
			// If the current row was among the collapsed items, make the
			// parent itself current.
			if (m_currentRow <= row + countDeletedRows)
				ChangeCurrentRow(row);
			else // Otherwise just update the index.
				ChangeCurrentRow(m_currentRow - countDeletedRows);
		}

		if (m_count != -1)
			m_count -= countDeletedRows;

		GetOwner()->InvalidateColBestWidths();

		UpdateDisplay();
		SendExpanderEvent(wxEVT_DATAVIEW_ITEM_COLLAPSED, node->GetItem());
	}
}

wxDataViewExtMainWindow::FindNodeResult
wxDataViewExtMainWindow::FindNode(const wxDataViewExtItem& item)
{
	FindNodeResult result;
	result.m_node = NULL;
	result.m_subtreeRealized = true;

	const wxDataViewExtModel* model = GetModel();
	if (model == NULL)
		return result;

	if (!item.IsOk())
	{
		result.m_node = m_root;
		return result;
	}

	// Compose the parent-chain for the item we are looking for
	wxVector<wxDataViewExtItem> parentChain;
	wxDataViewExtItem it(item);
	while (it.IsOk())
	{
		parentChain.push_back(it);
		it = model->GetParent(it);
	}

	// Find the item along the parent-chain.
	// This algorithm is designed to speed up the node-finding method
	wxDataViewExtTreeNode* node = m_root;
	for (unsigned iter = parentChain.size() - 1; ; --iter)
	{
		if (node->HasChildren())
		{
			if (node->GetChildNodes().empty())
			{
				// Even though the item is a container, it doesn't have any
				// child nodes in the control's representation yet.
				result.m_subtreeRealized = false;
				return result;
			}

			const wxDataViewExtTreeNodes& nodes = node->GetChildNodes();
			bool found = false;

			for (unsigned i = 0; i < nodes.size(); ++i)
			{
				wxDataViewExtTreeNode* currentNode = nodes[i];
				if (currentNode->GetItem() == parentChain[iter])
				{
					if (currentNode->GetItem() == item)
					{
						result.m_node = currentNode;
						return result;
					}

					node = currentNode;
					found = true;
					break;
				}
			}
			if (!found)
				return result;
		}
		else
			return result;

		if (!iter)
			break;
	}
	return result;
}

void wxDataViewExtMainWindow::HitTest(const wxPoint& point, wxDataViewExtItem& item,
	wxDataViewExtColumn*& column)
{
	wxDataViewExtColumn* col = NULL;
	unsigned int cols = GetOwner()->GetColumnCount();
	unsigned int colnum = 0;
	int x, y;
	m_owner->CalcUnscrolledPosition(point.x, point.y, &x, &y);
	for (unsigned x_start = 0; colnum < cols; colnum++)
	{
		col = GetOwner()->GetColumnAt(colnum);
		if (col->IsHidden())
			continue;      // skip it!

		unsigned int w = col->GetWidth();
		if (x_start + w >= (unsigned int)x)
			break;

		x_start += w;
	}

	column = col;
	item = GetItemByRow(GetLineAt(y));
}

wxRect wxDataViewExtMainWindow::GetItemRect(const wxDataViewExtItem& item,
	const wxDataViewExtColumn* column)
{
	int xpos = 0;
	int width = 0;

	unsigned int cols = GetOwner()->GetColumnCount();
	// If column is null the loop will compute the combined width of all columns.
	// Otherwise, it will compute the x position of the column we are looking for.
	for (unsigned int i = 0; i < cols; i++)
	{
		wxDataViewExtColumn* col = GetOwner()->GetColumnAt(i);

		if (col == column)
			break;

		if (col->IsHidden())
			continue;      // skip it!

		xpos += col->GetWidth();
		width += col->GetWidth();
	}

	if (column != 0)
	{
		// If we have a column, we need can get its width directly.
		if (column->IsHidden())
			width = 0;
		else
			width = column->GetWidth();

	}
	else
	{
		// If we have no column, we reset the x position back to zero.
		xpos = 0;
	}

	const int row = GetRowByItem(item, Walk_ExpandedOnly);
	if (row == -1)
	{
		// This means the row is currently not visible at all.
		return wxRect();
	}

	// we have to take an expander column into account and compute its indentation
	// to get the correct x position where the actual text is
	int indent = 0;
	if (!IsList() && m_viewMode == wxDataViewExtViewMode::wxDataViewExtTree &&
		(column == 0 || GetExpanderColumnOrFirstOne(GetOwner()) == column))
	{
		wxDataViewExtTreeNode* node = GetTreeNodeByRow(row);
		indent = GetOwner()->GetIndent() * node->GetIndentLevel();
		indent += wxRendererNative::Get().GetExpanderSize(this).GetWidth();
	}

	wxRect itemRect(xpos + indent,
		GetLineStart(row),
		width - indent,
		GetLineHeight(row));

	GetOwner()->CalcScrolledPosition(itemRect.x, itemRect.y,
		&itemRect.x, &itemRect.y);

	// Check if the rectangle is completely outside of the currently visible
	// area and, if so, return an empty rectangle to indicate that the item is
	// not visible.
	if (itemRect.GetBottom() < 0 || itemRect.GetTop() > GetClientSize().y)
	{
		return wxRect();
	}

	return itemRect;
}

int wxDataViewExtMainWindow::RecalculateCount() const
{
	if (IsVirtualList())
	{
		const wxDataViewExtVirtualListModel* list_model =
			static_cast<const wxDataViewExtVirtualListModel*>(GetModel());

		return list_model->GetCount();
	}
	else
	{
		return m_root->GetSubTreeCount();
	}
}

namespace
{

	class ItemToRowJob : public DoJob
	{
	public:
		// As with RowToTreeNodeJob above, we initialize m_current to -1 because
		// the first node passed to our operator() is the root node which is not
		// visible on screen and so we should return 0 for its first child node and
		// not for the root itself.
		ItemToRowJob(const wxDataViewExtItem& item, wxVector<wxDataViewExtItem>::reverse_iterator iter)
			: m_item(item), m_iter(iter), m_current(-1)
		{
		}

		// Maybe binary search will help to speed up this process
		virtual int operator() (wxDataViewExtTreeNode* node) wxOVERRIDE
		{
			if (node->GetItem() == m_item)
			{
				return DoJob::DONE;
			}

			// Is this node the next (grand)parent of the item we're looking for?
			if (node->GetItem() == *m_iter)
			{
				// Search for the next (grand)parent now and skip this item itself.
				++m_iter;
				++m_current;
				return DoJob::CONTINUE;
			}
			else
			{
				// Skip this node and all its currently visible children.
				m_current += node->GetSubTreeCount() + 1;
				return DoJob::SKIP_SUBTREE;
			}

		}

		int GetResult() const
		{
			return m_current;
		}

	private:
		const wxDataViewExtItem m_item;
		wxVector<wxDataViewExtItem>::reverse_iterator m_iter;

		// The row corresponding to the last node seen in our operator().
		int m_current;

	};

} // anonymous namespace

int
wxDataViewExtMainWindow::GetRowByItem(const wxDataViewExtItem& item,
	WalkFlags flags) const
{
	const wxDataViewExtModel* model = GetModel();
	if (model == NULL)
		return -1;

	if (IsVirtualList())
	{
		return wxPtrToUInt(item.GetID()) - 1;
	}
	else
	{
		if (!item.IsOk())
			return -1;

		// Compose the parent-chain of the item we are looking for
		wxVector<wxDataViewExtItem> parentChain;
		wxDataViewExtItem it(item);
		while (it.IsOk())
		{
			parentChain.push_back(it);
			it = model->GetParent(it);
		}

		// add an 'invalid' item to represent our 'invisible' root node
		parentChain.push_back(wxDataViewExtItem());

		// the parent chain was created by adding the deepest parent first.
		// so if we want to start at the root node, we have to iterate backwards through the vector
		ItemToRowJob job(item, parentChain.rbegin());
		if (!Walker(m_root, job, flags))
			return -1;

		return job.GetResult();
	}
}

static void BuildHierarchicalHelper(wxDataViewExtMainWindow* window, const wxDataViewExtModel* model,
	wxDataViewExtTreeNode* node)
{
	wxDataViewExtItemArray children;

	wxDataViewExtItem item =
		model->GetParentTopItem();
	wxDataViewExtItem child = item;

	while (child.IsOk())
	{
		children.Add(child);
		child = model->GetParent(child);
	}

	wxDataViewExtTreeNode* parent = node;

	for (unsigned int num = children.Count(); num > 0; num--)
	{
		wxDataViewExtTreeNode* node = new wxDataViewExtTreeNode(parent, children[num - 1], wxDataViewExtTreeNodeViewMode::wxDataViewExtHierarchy);

		{
			node->SetHasChildren(true);
			node->ToggleOpen(window);
		}

		parent->InsertChild(window, node, 0);

		if (parent->IsOpen())
			parent->ChangeSubTreeCount(1);

		parent = node;
	}

	BuildTreeHelper(window, model,
		item, parent);
}

static void BuildTreeHelper(wxDataViewExtMainWindow* window, const wxDataViewExtModel* model,
	const wxDataViewExtItem& item, wxDataViewExtTreeNode* node)
{
	if (!model->IsContainer(item))
		return;

	wxDataViewExtItemArray children;
	unsigned int num = model->GetChildren(item, children);

	for (unsigned int index = 0; index < num; index++)
	{
		wxDataViewExtTreeNode* n = new wxDataViewExtTreeNode(node, children[index]);

		if (model->IsContainer(children[index]))
			n->SetHasChildren(true);

		node->InsertChild(window, n, index);
	}

	if (node->IsOpen())
		node->ChangeSubTreeCount(+num);
}

void BuildListHelper(wxDataViewExtMainWindow* window, const wxDataViewExtModel* model,
	const wxDataViewExtItem& item, wxDataViewExtTreeNode* node)
{
	wxDataViewExtItemArray children;
	unsigned int num = model->GetChildren(item, children);

	for (unsigned int index = 0; index < num; index++)
	{
		wxDataViewExtTreeNode* n = new wxDataViewExtTreeNode(node, children[index]);

		node->InsertChild(window, n, index);

		BuildListHelper(window, model, children[index], node);
	}

	if (node->IsOpen())
		node->ChangeSubTreeCount(+num);
}

void wxDataViewExtMainWindow::BuildTree(wxDataViewExtModel* model)
{
	DestroyTree();

	if (GetModel()->IsVirtualListModel())
	{
		InvalidateCount();
		return;
	}

	m_root = wxDataViewExtTreeNode::CreateRootNode();

	if (m_viewMode == wxDataViewExtHierarchical)
	{
		BuildHierarchicalHelper(this, model, m_root);
	}
	else if (m_viewMode == wxDataViewExtList)
	{
		// First we define a invalid item to fetch the top-level elements
		wxDataViewExtItem item;
		BuildListHelper(this, model, item, m_root);
	}
	else if (m_viewMode == wxDataViewExtTree)
	{
		// First we define a invalid item to fetch the top-level elements
		wxDataViewExtItem item;
		BuildTreeHelper(this, model, item, m_root);
	}

	InvalidateCount();
}

void wxDataViewExtMainWindow::DestroyTree()
{
	if (!IsVirtualList())
	{
		wxDELETE(m_root);
		m_count = 0;
	}
}

wxDataViewExtColumn*
wxDataViewExtMainWindow::FindColumnForEditing(const wxDataViewExtItem& item, wxDataViewExtCellMode mode) const
{
	// Edit the current column editable in 'mode'. If no column is focused
	// (typically because the user has full row selected), try to find the
	// first editable column (this would typically be a checkbox for
	// wxDATAVIEW_CELL_ACTIVATABLE and we don't want to force the user to set
	// focus on the checkbox column; or on the only editable text column).

	wxDataViewExtColumn* candidate = m_currentCol;

	if (candidate && !IsCellEditableInMode(item, candidate, mode))
	{
		if (m_currentColSetByKeyboard)
		{
			// If current column was set by keyboard to something not editable (in
			// 'mode') and the user pressed Space/F2 then do not edit anything
			// because focus is visually on that column and editing
			// something else would be surprising.
			return NULL;
		}
		else
		{
			// But if the current column was set by mouse to something not editable (in
			// 'mode') and the user pressed Space/F2 to edit it, treat the
			// situation as if there was whole-row focus, because that's what is
			// visually indicated and the mouse click could very well be targeted
			// on the row rather than on an individual cell.
			candidate = NULL;
		}
	}

	if (!candidate)
	{
		const unsigned cols = GetOwner()->GetColumnCount();
		for (unsigned i = 0; i < cols; i++)
		{
			wxDataViewExtColumn* c = GetOwner()->GetColumnAt(i);
			if (c->IsHidden())
				continue;

			if (IsCellEditableInMode(item, c, mode))
			{
				candidate = c;
				break;
			}
		}
	}

	// Switch to the first column with value if the current column has no value
	if (candidate && !GetModel()->HasValue(item, candidate->GetModelColumn()))
		candidate = FindFirstColumnWithValue(item);

	if (!candidate)
		return NULL;

	if (!IsCellEditableInMode(item, candidate, mode))
		return NULL;

	return candidate;
}

bool wxDataViewExtMainWindow::IsCellEditableInMode(const wxDataViewExtItem& item,
	const wxDataViewExtColumn* col,
	wxDataViewExtCellMode mode) const
{
	if (col->GetRenderer()->GetMode() != mode)
		return false;

	if (!GetModel()->IsEnabled(item, col->GetModelColumn()))
		return false;

	if (!GetModel()->HasValue(item, col->GetModelColumn()))
		return false;

	return true;
}

void wxDataViewExtMainWindow::OnCharHook(wxKeyEvent& event)
{
	if (m_editorCtrl)
	{
		// Handle any keys special for the in-place editor and return without
		// calling Skip() below.
		switch (event.GetKeyCode())
		{
		case WXK_ESCAPE:
			m_editorRenderer->CancelEditing();
			return;

		case WXK_RETURN:
			// Shift-Enter is not special either.
			if (event.ShiftDown())
				break;
			wxFALLTHROUGH;

		case WXK_TAB:
			// Ctrl/Alt-Tab or Enter could be used for something else, so
			// don't handle them here.
			if (event.HasModifiers())
				break;

			m_editorRenderer->FinishEditing();
			return;
		}
	}
	else if (m_useCellFocus)
	{
		if (event.GetKeyCode() == WXK_TAB && !event.HasModifiers())
		{
			if (event.ShiftDown())
				OnLeftKey(event);
			else
				OnRightKey(event);
			return;
		}
	}

	event.Skip();
}

void wxDataViewExtMainWindow::OnChar(wxKeyEvent& event)
{
	wxWindow* const parent = GetParent();

	// propagate the char event upwards
	wxKeyEvent eventForParent(event);
	eventForParent.SetEventObject(parent);
	if (parent->ProcessWindowEvent(eventForParent))
		return;

	if (parent->HandleAsNavigationKey(event))
		return;

	// no item -> nothing to do
	if (!HasCurrentRow())
	{
		event.Skip();
		return;
	}

	switch (event.GetKeyCode())
	{
	case WXK_RETURN:
		if (event.HasModifiers())
		{
			event.Skip();
			break;
		}
		else
		{
			// Enter activates the item, i.e. sends wxEVT_DATAVIEW_ITEM_ACTIVATED to
			// it. Only if that event is not handled do we activate column renderer (which
			// is normally done by Space) or even inline editing.

			const wxDataViewExtItem item = GetItemByRow(m_currentRow);

			wxDataViewExtEvent le(wxEVT_DATAVIEW_ITEM_ACTIVATED, m_owner, item);
			if (m_owner->ProcessWindowEvent(le))
				break;
			// else: fall through to WXK_SPACE handling
		}
		wxFALLTHROUGH;

	case WXK_SPACE:
		if (event.HasModifiers())
		{
			event.Skip();
			break;
		}
		else
		{
			// Space toggles activatable items or -- if not activatable --
			// starts inline editing (this is normally done using F2 on
			// Windows, but Space is common everywhere else, so use it too
			// for greater cross-platform compatibility).

			const wxDataViewExtItem item = GetItemByRow(m_currentRow);

			// Activate the current activatable column. If not column is focused (typically
			// because the user has full row selected), try to find the first activatable
			// column (this would typically be a checkbox and we don't want to force the user
			// to set focus on the checkbox column).
			wxDataViewExtColumn* activatableCol = FindColumnForEditing(item, wxDATAVIEW_CELL_ACTIVATABLE);

			if (activatableCol)
			{
				const unsigned colIdx = activatableCol->GetModelColumn();
				const wxRect cell_rect = GetOwner()->GetItemRect(item, activatableCol);

				wxDataViewExtRenderer* cell = activatableCol->GetRenderer();
				cell->PrepareForItem(GetModel(), item, colIdx);
				cell->WXActivateCell(cell_rect, GetModel(), item, colIdx, NULL);

				break;
			}
			// else: fall through to WXK_F2 handling
			wxFALLTHROUGH;
		}

	case WXK_F2:
		if (event.HasModifiers())
		{
			event.Skip();
			break;
		}
		else
		{
			if (!m_selection.IsEmpty())
			{
				// Mimic Windows 7 behavior: edit the item that has focus
				// if it is selected and the first selected item if focus
				// is out of selection.
				unsigned sel;
				if (m_selection.IsSelected(m_currentRow))
				{
					sel = m_currentRow;
				}
				else // Focused item is not selected.
				{
					wxSelectionStore::IterationState cookie;
					sel = m_selection.GetFirstSelectedItem(cookie);
				}


				const wxDataViewExtItem item = GetItemByRow(sel);

				// Edit the current column. If no column is focused
				// (typically because the user has full row selected), try
				// to find the first editable column.
				wxDataViewExtColumn* editableCol = FindColumnForEditing(item, wxDATAVIEW_CELL_EDITABLE);

				if (editableCol)
					GetOwner()->EditItem(item, editableCol);
			}
		}
		break;

	case WXK_UP:
		GoToRelativeRow(event, -1);
		break;

	case WXK_DOWN:
		GoToRelativeRow(event, +1);
		break;

	case '+':
	case WXK_ADD:
		Expand(m_currentRow);
		break;

	case '*':
	case WXK_MULTIPLY:
		if (!IsExpanded(m_currentRow))
		{
			Expand(m_currentRow, true /* recursively */);
			break;
		}
		//else: fall through to Collapse()
		wxFALLTHROUGH;

	case '-':
	case WXK_SUBTRACT:
		Collapse(m_currentRow);
		break;

	case WXK_LEFT:
		OnLeftKey(event);
		break;

	case WXK_RIGHT:
		OnRightKey(event);
		break;

	case WXK_END:
		GoToRelativeRow(event, +(int)GetRowCount());
		break;

	case WXK_HOME:
		GoToRelativeRow(event, -(int)GetRowCount());
		break;

	case WXK_PAGEUP:
		GoToRelativeRow(event, -(GetCountPerPage() - 1));
		break;

	case WXK_PAGEDOWN:
		GoToRelativeRow(event, +(GetCountPerPage() - 1));
		break;

	default:
		event.Skip();
	}
}

void wxDataViewExtMainWindow::GoToRelativeRow(const wxKeyboardState& kbdState, int delta)
{
	// if there is no selection, we cannot move it anywhere
	if (!HasCurrentRow() || IsEmpty())
		return;

	int newRow = (int)m_currentRow + delta;

	// let's keep the new row inside the allowed range
	if (newRow < 0)
		newRow = 0;

	const int rowCount = (int)GetRowCount();
	if (newRow >= rowCount)
		newRow = rowCount - 1;

	GoToRow(kbdState, newRow);
}

void
wxDataViewExtMainWindow::GoToRow(const wxKeyboardState& kbdState,
	unsigned int newCurrent)
{
	unsigned int oldCurrent = m_currentRow;

	if (newCurrent == oldCurrent)
		return;

	// in single selection we just ignore Shift as we can't select several
	// items anyhow
	if (kbdState.ShiftDown() && !IsSingleSel())
	{
		RefreshRow(oldCurrent);

		ChangeCurrentRow(newCurrent);

		// select all the items between the old and the new one
		if (oldCurrent > newCurrent)
		{
			newCurrent = oldCurrent;
			oldCurrent = m_currentRow;
		}

		SelectRows(oldCurrent, newCurrent);

		wxSelectionStore::IterationState cookie;
		const unsigned firstSel = m_selection.GetFirstSelectedItem(cookie);
		if (firstSel != wxSelectionStore::NO_SELECTION)
			SendSelectionChangedEvent(GetItemByRow(firstSel));
	}
	else // !shift
	{
		RefreshRow(oldCurrent);

		// all previously selected items are unselected unless ctrl is held
		if (!kbdState.ControlDown())
			UnselectAllRows();

		ChangeCurrentRow(newCurrent);

		if (!kbdState.ControlDown())
		{
			SelectRow(m_currentRow, true);
			SendSelectionChangedEvent(GetItemByRow(m_currentRow));
		}
		else
			RefreshRow(m_currentRow);
	}

	GetOwner()->EnsureVisibleRowCol(m_currentRow, -1);
}

void wxDataViewExtMainWindow::OnLeftKey(wxKeyEvent& event)
{
	if (IsList())
	{
		TryAdvanceCurrentColumn(NULL, event, /*forward=*/false);
	}
	else
	{
		wxDataViewExtTreeNode* node = GetTreeNodeByRow(m_currentRow);
		if (!node)
			return;

		if (TryAdvanceCurrentColumn(node, event, /*forward=*/false))
			return;

		const bool dontCollapseNodes = event.GetKeyCode() == WXK_TAB;
		if (dontCollapseNodes)
		{
			m_currentCol = NULL;
			// allow focus change
			event.Skip();
			return;
		}

		// Because TryAdvanceCurrentColumn() return false, we are at the first
		// column or using whole-row selection. In this situation, we can use
		// the standard TreeView handling of the left key.
		if (node->HasChildren() && node->IsOpen())
		{
			Collapse(m_currentRow);
		}
		else
		{
			// if the node is already closed, we move the selection to its parent
			wxDataViewExtTreeNode* parent_node = node->GetParent();

			if (parent_node)
			{
				int parent = GetRowByItem(parent_node->GetItem());
				if (parent >= 0)
				{
					GoToRow(event, parent);
				}
			}
		}
	}
}

void wxDataViewExtMainWindow::OnRightKey(wxKeyEvent& event)
{
	if (IsList())
	{
		TryAdvanceCurrentColumn(NULL, event, /*forward=*/true);
	}
	else
	{
		wxDataViewExtTreeNode* node = GetTreeNodeByRow(m_currentRow);
		if (!node)
			return;

		if (node->HasChildren())
		{
			if (!node->IsOpen())
			{
				Expand(m_currentRow);
			}
			else
			{
				// if the node is already open, we move the selection to the first child
				GoToRelativeRow(event, +1);
			}
		}
		else
		{
			TryAdvanceCurrentColumn(node, event, /*forward=*/true);
		}
	}
}

bool wxDataViewExtMainWindow::TryAdvanceCurrentColumn(wxDataViewExtTreeNode* node, wxKeyEvent& event, bool forward)
{
	if (GetOwner()->GetColumnCount() == 0)
		return false;

	if (!m_useCellFocus)
		return false;

	const bool wrapAround = event.GetKeyCode() == WXK_TAB;

	// navigation shouldn't work in nodes with fewer than two columns
	if (node && IsItemSingleValued(node->GetItem()))
		return false;

	if (m_currentCol == NULL || !m_currentColSetByKeyboard)
	{
		if (forward)
		{
			if (node)
			{
				// find first column with value
				m_currentCol = FindFirstColumnWithValue(node->GetItem());
			}
			else
			{
				// in the special "list" case, all columns have values, so just
				// take the first one
				m_currentCol = GetOwner()->GetColumnAt(0);
			}

			m_currentColSetByKeyboard = true;
			RefreshRow(m_currentRow);
			return true;
		}
		else
		{
			if (!wrapAround)
				return false;
		}
	}

	int idx = GetOwner()->GetColumnIndex(m_currentCol);
	const unsigned int cols = GetOwner()->GetColumnCount();
	for (unsigned int i = 0; i < cols; i++)
	{
		idx += (forward ? +1 : -1);
		if (idx >= (int)GetOwner()->GetColumnCount())
		{
			if (!wrapAround)
				return false;

			if (GetCurrentRow() < GetRowCount() - 1)
			{
				// go to the first column of the next row:
				idx = 0;
				GoToRelativeRow(wxKeyboardState()/*dummy*/, +1);
			}
			else
			{
				// allow focus change
				event.Skip();
				return false;
			}
		}
		else if (idx < 0)
		{
			if (!wrapAround)
				return false;

			if (GetCurrentRow() > 0)
			{
				// go to the last column of the previous row:
				idx = (int)GetOwner()->GetColumnCount() - 1;
				GoToRelativeRow(wxKeyboardState()/*dummy*/, -1);
			}
			else
			{
				// allow focus change
				event.Skip();
				return false;
			}
		}
		if (!node || GetModel()->HasValue(node->GetItem(), i))
			break;
	}

	GetOwner()->EnsureVisibleRowCol(m_currentRow, idx);

	if (idx < 1)
	{
		// We are going to the left of the second column. Reset to whole-row
		// focus (which means first column would be edited).
		m_currentCol = NULL;
		RefreshRow(m_currentRow);
		return true;
	}

	m_currentCol = GetOwner()->GetColumnAt(idx);
	m_currentColSetByKeyboard = true;
	RefreshRow(m_currentRow);
	return true;
}

void wxDataViewExtMainWindow::OnMouse(wxMouseEvent& event)
{
	if (event.GetEventType() == wxEVT_MOUSEWHEEL)
	{
		// set one always one scroll
		event.m_linesPerAction = event.m_columnsPerAction = 1;

		// let the base handle mouse wheel events.
		if (!m_owner->ProcessWindowEvent(event))
			event.Skip();

		return;
	}

	int x = event.GetX();
	int y = event.GetY();
	m_owner->CalcUnscrolledPosition(x, y, &x, &y);
	wxDataViewExtColumn* col = NULL;

	int xpos = 0;
	unsigned int cols = GetOwner()->GetColumnCount();
	unsigned int i;
	for (i = 0; i < cols; i++)
	{
		wxDataViewExtColumn* c = GetOwner()->GetColumnAt(i);
		if (c->IsHidden())
			continue;      // skip it!

		if (x < xpos + c->GetWidth())
		{
			col = c;
			break;
		}
		xpos += c->GetWidth();
	}

	wxDataViewExtModel* const model = GetModel();

	const unsigned int current = GetLineAt(y);
	const wxDataViewExtItem item = GetItemByRow(current);

	if (event.ButtonDown())
	{
		// Not skipping button down events would prevent the system from
		// setting focus to this window as most (all?) of them do by default,
		// so skip it to enable default handling.
		event.Skip();

		// Also stop editing if any mouse button is pressed: this is not really
		// necessary for the left button, as it would result in a focus loss
		// that would make the editor close anyhow, but we do need to do it for
		// the other ones and it does no harm to do it for the left one too.
		FinishEditing();
	}

	// Handle right clicking here, before everything else as context menu
	// events should be sent even when we click outside of any item, unlike all
	// the other ones.
	if (event.RightUp())
	{
		wxDataViewExtEvent le(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, m_owner, col, item);
		int xx = event.GetX();
		int yy = event.GetY();
		ClientToScreen(&xx, &yy);
		m_owner->ScreenToClient(&xx, &yy);
		le.SetPosition(xx, yy);
		m_owner->ProcessWindowEvent(le);
	}

#if wxUSE_DRAG_AND_DROP
	if (event.Dragging() || ((m_dragCount > 0) && event.Leaving()))
	{
		if (m_dragCount == 0)
		{
			// we have to report the raw, physical coords as we want to be
			// able to call HitTest(event.m_pointDrag) from the user code to
			// get the item being dragged
			m_dragStart = event.GetPosition();
		}

		m_dragCount++;
		if ((m_dragCount < 3) && (event.Leaving()))
			m_dragCount = 3;
		else if (m_dragCount != 3)
			return;

		if (event.LeftIsDown())
		{
			m_owner->CalcUnscrolledPosition(m_dragStart.x, m_dragStart.y,
				&m_dragStart.x, &m_dragStart.y);
			unsigned int drag_item_row = GetLineAt(m_dragStart.y);
			if (drag_item_row >= GetRowCount() || m_dragStart.x > GetEndOfLastCol())
				return;

			wxDataViewExtItem itemDragged = GetItemByRow(drag_item_row);

			// Notify cell about drag
			wxDataViewExtEvent evt(wxEVT_DATAVIEW_ITEM_BEGIN_DRAG, m_owner, itemDragged);
			if (!m_owner->HandleWindowEvent(evt))
				return;

			if (!evt.IsAllowed())
				return;

			wxDataObject* obj = evt.GetDataObject();
			if (!obj)
				return;

			wxDataViewExtDropSource drag(this, drag_item_row);
			drag.SetData(*obj);
			/* wxDragResult res = */ drag.DoDragDrop(evt.GetDragFlags());
			delete obj;
		}
		return;
	}
	else
	{
		m_dragCount = 0;
	}
#endif // wxUSE_DRAG_AND_DROP

	// Check if we clicked outside the item area.
	if ((current >= GetRowCount()) || !col)
	{
		// Follow Windows convention here: clicking either left or right (but
		// not middle) button clears the existing selection.
		if (m_owner && (event.LeftDown() || event.RightDown()))
		{
			if (!m_selection.IsEmpty())
			{
				m_owner->UnselectAll();
				SendSelectionChangedEvent(wxDataViewExtItem());
			}
		}
		event.Skip();
		return;
	}

	wxDataViewExtRenderer* cell = col->GetRenderer();
	wxDataViewExtColumn* const
		expander = GetExpanderColumnOrFirstOne(GetOwner());

	// Test whether the mouse is hovering over the expander (a.k.a tree "+"
	// button) and also determine the offset of the real cell start, skipping
	// the indentation and the expander itself.
	bool hoverOverExpander = false;
	int itemOffset = 0;
	if ((!IsList()) && (expander == col) && m_viewMode == wxDataViewExtViewMode::wxDataViewExtTree)
	{
		wxDataViewExtTreeNode* node = GetTreeNodeByRow(current);

		int indent = node->GetIndentLevel();
		itemOffset = GetOwner()->GetIndent() * indent;
		const int expWidth = wxRendererNative::Get().GetExpanderSize(this).GetWidth();

		if (node->HasChildren())
		{
			// we make the rectangle we are looking in a bit bigger than the actual
			// visual expander so the user can hit that little thing reliably
			wxRect rect(xpos + itemOffset,
				GetLineStart(current) + (GetLineHeight(current) - m_lineHeight) / 2,
				expWidth, m_lineHeight);

			if (rect.Contains(x, y))
			{
				// So the mouse is over the expander
				hoverOverExpander = true;
				if (m_underMouse && m_underMouse != node)
				{
					// wxLogMessage("Undo the row: %d", GetRowByItem(m_underMouse->GetItem()));
					RefreshRow(GetRowByItem(m_underMouse->GetItem()));
				}
				if (m_underMouse != node)
				{
					// wxLogMessage("Do the row: %d", current);
					RefreshRow(current);
				}
				m_underMouse = node;
			}
		}

		// Account for the expander as well, even if this item doesn't have it,
		// its parent does so it still counts for the offset.
		itemOffset += expWidth;
	}

	if (!hoverOverExpander)
	{
		if (m_underMouse != NULL)
		{
			// wxLogMessage("Undo the row: %d", GetRowByItem(m_underMouse->GetItem()));
			RefreshRow(GetRowByItem(m_underMouse->GetItem()));
			m_underMouse = NULL;
		}
	}

	bool simulateClick = false;

	if (event.ButtonDClick())
	{
		m_renameTimer->Stop();
		m_lastOnSame = false;

		if ((!IsList()) && m_viewMode == wxDataViewExtViewMode::wxDataViewExtHierarchical)
		{
			wxDataViewExtTreeNode* node = GetTreeNodeByRow(current);
			if (node->HasChildren())
			{
				SetTopParent(node->GetItem());

				return;
			}
		}
	}

	bool ignore_other_columns =
		(expander != col) &&
		(!model->HasValue(item, col->GetModelColumn()));

	if (event.LeftDClick())
	{
		if (!hoverOverExpander && (current == m_lineLastClicked))
		{
			wxDataViewExtEvent le(wxEVT_DATAVIEW_ITEM_ACTIVATED, m_owner, col, item);
			if (m_owner->ProcessWindowEvent(le))
			{
				// Item activation was handled from the user code.
				return;
			}
		}

		// Either it was a double click over the expander, or the second click
		// happened on another item than the first one or it was a bona fide
		// double click which was unhandled. In all these cases we continue
		// processing this event as a simple click, e.g. to select the item or
		// activate the renderer.
		simulateClick = true;
	}

	if (event.LeftUp() && !hoverOverExpander)
	{
		if (m_lineSelectSingleOnUp != (unsigned int)-1)
		{
			// select single line
			if (UnselectAllRows(m_lineSelectSingleOnUp))
			{
				SelectRow(m_lineSelectSingleOnUp, true);
			}

			SendSelectionChangedEvent(GetItemByRow(m_lineSelectSingleOnUp));
		}

		// If the user click the expander, we do not do editing even if the column
		// with expander are editable
		if (m_lastOnSame && !ignore_other_columns)
		{
			if ((col == m_currentCol) && (current == m_currentRow) &&
				IsCellEditableInMode(item, col, wxDATAVIEW_CELL_EDITABLE))
			{
				m_renameTimer->Start(100, true);
			}
		}

		m_lastOnSame = false;
		m_lineSelectSingleOnUp = (unsigned int)-1;
	}
	else if (!event.LeftUp())
	{
		// This is necessary, because after a DnD operation in
		// from and to ourself, the up event is swallowed by the
		// DnD code. So on next non-up event (which means here and
		// now) m_lineSelectSingleOnUp should be reset.
		m_lineSelectSingleOnUp = (unsigned int)-1;
	}

	if (event.RightDown())
	{
		m_lineBeforeLastClicked = m_lineLastClicked;
		m_lineLastClicked = current;

		// If the item is already selected, do not update the selection.
		// Multi-selections should not be cleared if a selected item is clicked.
		if (!IsRowSelected(current))
		{
			UnselectAllRows();

			const unsigned oldCurrent = m_currentRow;
			ChangeCurrentRow(current);
			SelectRow(m_currentRow, true);
			RefreshRow(oldCurrent);
			SendSelectionChangedEvent(GetItemByRow(m_currentRow));
		}
	}
	else if (event.MiddleDown())
	{
	}

	if ((event.LeftDown() || simulateClick) && hoverOverExpander)
	{
		wxDataViewExtTreeNode* node = GetTreeNodeByRow(current);

		// hoverOverExpander being true tells us that our node must be
		// valid and have children.
		// So we don't need any extra checks.
		if (node->IsOpen())
			Collapse(current);
		else
			Expand(current);

	}
	else if (((event.LeftDown() || event.RightDown()) || simulateClick) && !hoverOverExpander)
	{
		m_lineBeforeLastClicked = m_lineLastClicked;
		m_lineLastClicked = current;

		unsigned int oldCurrentRow = m_currentRow;
		bool oldWasSelected = IsRowSelected(m_currentRow);

		bool cmdModifierDown = event.CmdDown();
		if (IsSingleSel() || !(cmdModifierDown || event.ShiftDown()))
		{
			if (IsSingleSel() || !IsRowSelected(current))
			{
				ChangeCurrentRow(current);
				if (UnselectAllRows(current))
				{
					SelectRow(m_currentRow, true);
					SendSelectionChangedEvent(GetItemByRow(m_currentRow));
				}
			}
			else // multi sel & current is highlighted & no mod keys
			{
				m_lineSelectSingleOnUp = current;
				ChangeCurrentRow(current); // change focus
			}
		}
		else // multi sel & either ctrl or shift is down
		{
			if (cmdModifierDown)
			{
				ChangeCurrentRow(current);
				ReverseRowSelection(m_currentRow);
				SendSelectionChangedEvent(GetItemByRow(m_currentRow));
			}
			else if (event.ShiftDown())
			{
				ChangeCurrentRow(current);

				unsigned int lineFrom = oldCurrentRow,
					lineTo = current;

				if (lineFrom == static_cast<unsigned>(-1))
				{
					// If we hadn't had any current row before, treat this as a
					// simple click and select the new row only.
					lineFrom = current;
				}

				if (lineTo < lineFrom)
				{
					lineTo = lineFrom;
					lineFrom = m_currentRow;
				}

				SelectRows(lineFrom, lineTo);

				wxSelectionStore::IterationState cookie;
				const unsigned firstSel = m_selection.GetFirstSelectedItem(cookie);
				if (firstSel != wxSelectionStore::NO_SELECTION)
					SendSelectionChangedEvent(GetItemByRow(firstSel));
			}
			else // !ctrl, !shift
			{
				// test in the enclosing if should make it impossible
				wxFAIL_MSG(wxT("how did we get here?"));
			}
		}

		if (m_currentRow != oldCurrentRow)
			RefreshRow(oldCurrentRow);

		wxDataViewExtColumn* oldCurrentCol = m_currentCol;

		// Update selection here...
		m_currentCol = col;
		m_currentColSetByKeyboard = false;

		if (col != oldCurrentCol)
			UpdateDisplay();

		// This flag is used to decide whether we should start editing the item
		// label. We do it if the user clicks twice (but not double clicks,
		// i.e. simulateClick is false) on the same item but not if the click
		// was used for something else already, e.g. selecting the item (so it
		// must have been already selected) or giving the focus to the control
		// (so it must have had focus already).
		m_lastOnSame = !simulateClick && ((col == oldCurrentCol) &&
			(current == oldCurrentRow)) && oldWasSelected &&
			HasFocus();

		// Call ActivateCell() after everything else as under GTK+
		if (IsCellEditableInMode(item, col, wxDATAVIEW_CELL_ACTIVATABLE))
		{
			// notify cell about click

			wxRect cell_rect(xpos + itemOffset,
				GetLineStart(current),
				col->GetWidth() - itemOffset,
				GetLineHeight(current));

			// Note that PrepareForItem() should be called after GetLineStart()
			// call in cell_rect initialization above as GetLineStart() calls
			// PrepareForItem() for other items from inside it.
			cell->PrepareForItem(model, item, col->GetModelColumn());

			// Report position relative to the cell's custom area, i.e.
			// not the entire space as given by the control but the one
			// used by the renderer after calculation of alignment etc.
			//
			// Notice that this results in negative coordinates when clicking
			// in the upper left corner of a centre-aligned cell which doesn't
			// fill its column entirely so this is somewhat surprising, but we
			// do it like this for compatibility with the native GTK+ version,
			// see #12270.

			// adjust the rectangle ourselves to account for the alignment
			const int align = cell->GetEffectiveAlignment();

			wxRect rectItem = cell_rect;
			const wxSize size = cell->GetSize();
			if (size.x >= 0 && size.x < cell_rect.width)
			{
				if (align & wxALIGN_CENTER_HORIZONTAL)
					rectItem.x += (cell_rect.width - size.x) / 2;
				else if (align & wxALIGN_RIGHT)
					rectItem.x += cell_rect.width - size.x;
				// else: wxALIGN_LEFT is the default
			}

			if (size.y >= 0 && size.y < cell_rect.height)
			{
				if (align & wxALIGN_CENTER_VERTICAL)
					rectItem.y += (cell_rect.height - size.y) / 2;
				else if (align & wxALIGN_BOTTOM)
					rectItem.y += cell_rect.height - size.y;
				// else: wxALIGN_TOP is the default
			}

			wxMouseEvent event2(event);
			event2.m_x -= rectItem.x;
			event2.m_y -= rectItem.y;
			m_owner->CalcUnscrolledPosition(event2.m_x, event2.m_y, &event2.m_x, &event2.m_y);

			/* ignore ret */ cell->WXActivateCell
			(
				cell_rect,
				model,
				item,
				col->GetModelColumn(),
				&event2
			);
		}
	}
}

void wxDataViewExtMainWindow::OnSetFocus(wxFocusEvent& event)
{
	m_hasFocus = true;

	// Make the control usable from keyboard once it gets focus by ensuring
	// that it has a current row, if at all possible.
	if (!HasCurrentRow() && !IsEmpty())
	{
		ChangeCurrentRow(0);
	}

	if (HasCurrentRow())
	{
		Refresh();
	}
#if wxUSE_ACCESSIBILITY
	else
	{
		wxAccessible::NotifyEvent(wxACC_EVENT_OBJECT_FOCUS, m_owner, wxOBJID_CLIENT, wxACC_SELF);
	}
#endif // wxUSE_ACCESSIBILITY

	event.Skip();
}

void wxDataViewExtMainWindow::OnKillFocus(wxFocusEvent& event)
{
	m_hasFocus = false;

	if (HasCurrentRow())
		Refresh();

	event.Skip();
}

void wxDataViewExtMainWindow::OnColumnsCountChanged()
{
	int editableCount = 0;

	const unsigned cols = GetOwner()->GetColumnCount();
	for (unsigned i = 0; i < cols; i++)
	{
		wxDataViewExtColumn* c = GetOwner()->GetColumnAt(i);
		if (c->IsHidden())
			continue;
		if (c->GetRenderer()->GetMode() != wxDATAVIEW_CELL_INERT)
			editableCount++;
	}

	m_useCellFocus = (editableCount > 0);

	UpdateDisplay();
}

void wxDataViewExtMainWindow::UpdateColumnSizes()
{
	int colsCount = GetOwner()->GetColumnCount();
	if (!colsCount)
		return;

	wxDataViewExtCtrl* owner = GetOwner();

	int fullWinWidth = GetClientSize().x;

	// Find the last shown column: we shouldn't bother to resize the columns
	// that are hidden anyhow.
	int lastColIndex = -1;
	wxDataViewExtColumn* lastCol wxDUMMY_INITIALIZE(NULL);
	for (int colIndex = colsCount - 1; colIndex >= 0; --colIndex)
	{
		lastCol = owner->GetColumnAt(colIndex);
		if (!lastCol->IsHidden())
		{
			lastColIndex = colIndex;
			break;
		}
	}

	if (lastColIndex == -1)
	{
		// All columns are hidden.
		return;
	}

	int lastColX = 0;
	for (int colIndex = 0; colIndex < lastColIndex; ++colIndex)
	{
		const wxDataViewExtColumn* c = owner->GetColumnAt(colIndex);

		if (!c->IsHidden())
			lastColX += c->GetWidth();
	}

	int colswidth = lastColX + lastCol->GetWidth();
	if (lastColX < fullWinWidth)
	{
		const int availableWidth = fullWinWidth - lastColX;

		// Never make the column automatically smaller than the last width it
		// was explicitly given nor its minimum width (however we do need to
		// reduce it until this size if it's currently wider, so this
		// comparison needs to be strict).
		if (availableWidth < wxMax(lastCol->GetMinWidth(),
			lastCol->WXGetSpecifiedWidth()))
		{
			return;
		}

		lastCol->WXUpdateWidth(availableWidth);

		// All columns fit on screen, so we don't need horizontal scrolling.
		// To prevent flickering scrollbar when resizing the window to be
		// narrower, force-set the virtual width to 0 here. It will eventually
		// be corrected at idle time.
		SetVirtualSize(0, m_virtualSize.y);

		RefreshRect(wxRect(lastColX, 0, availableWidth, GetSize().y));
	}
	else
	{
		// else: don't bother, the columns won't fit anyway
		SetVirtualSize(colswidth, m_virtualSize.y);
	}
}

//-----------------------------------------------------------------------------
// wxDataViewExtCtrl
//-----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(wxDataViewExtCtrl, wxDataViewExtCtrlBase);
wxBEGIN_EVENT_TABLE(wxDataViewExtCtrl, wxDataViewExtCtrlBase)
EVT_SIZE(wxDataViewExtCtrl::OnSize)
EVT_DPI_CHANGED(wxDataViewExtCtrl::OnDPIChanged)
wxEND_EVENT_TABLE()

wxDataViewExtCtrl::~wxDataViewExtCtrl()
{
	if (m_notifier)
		GetModel()->RemoveNotifier(m_notifier);

	DoClearColumns();

#if wxUSE_ACCESSIBILITY
	SetAccessible(NULL);
	wxAccessible::NotifyEvent(wxACC_EVENT_OBJECT_DESTROY, this, wxOBJID_CLIENT, wxACC_SELF);
#endif // wxUSE_ACCESSIBILITY
}

void wxDataViewExtCtrl::Init()
{
	m_notifier = NULL;

	m_headerAreaWin = NULL;
	m_tableAreaWin = NULL;
	m_footerAreaWin = NULL;

	m_colsDirty = false;

	m_allowMultiColumnSort = false;
}

bool wxDataViewExtCtrl::Create(wxWindow* parent,
	wxWindowID id,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxValidator& validator,
	const wxString& name)
{
	//    if ( (style & wxBORDER_MASK) == 0)
	//        style |= wxBORDER_SUNKEN;

	Init();

	if (!wxControl::Create(parent, id, pos, size,
		style | wxScrolledWindowStyle, validator, name))
		return false;

	SetInitialSize(size);

#ifdef __WXMAC__
	MacSetClipChildren(true);
#endif

	m_tableAreaWin = new wxDataViewExtMainWindow(this, wxID_ANY);

	// We use the cursor keys for moving the selection, not scrolling, so call
	// this method to ensure wxScrollHelperEvtHandler doesn't catch all
	// keyboard events forwarded to us from wxListMainWindow.
	DisableKeyboardScrolling();

	if (HasFlag(wxDV_NO_HEADER))
		m_headerAreaWin = NULL;
	else
		m_headerAreaWin = new wxDataViewExtHeaderWindow(this);

	if (!HasFlag(wxDV_FOOTER))
		m_footerAreaWin = NULL;
	else
		m_footerAreaWin = new wxDataViewExtFooterWindow(this);

	SetTargetWindow(m_tableAreaWin);

	CreateTableSizer();

	EnableSystemThemeByDefault();

#if wxUSE_ACCESSIBILITY
	wxAccessible::NotifyEvent(wxACC_EVENT_OBJECT_CREATE, this, wxOBJID_CLIENT, wxACC_SELF);
#endif // wxUSE_ACCESSIBILITY

	return true;
}

wxWindowList wxDataViewExtCtrl::GetCompositeWindowParts() const
{
	wxWindowList parts;
	parts.push_back(m_headerAreaWin); // It's ok to add it even if it's null.
	parts.push_back(m_tableAreaWin);
	parts.push_back(m_footerAreaWin); // It's ok to add it even if it's null.
	return parts;
}

wxBorder wxDataViewExtCtrl::GetDefaultBorder() const
{
	return wxBORDER_THEME;
}

wxHeaderGenericCtrl* wxDataViewExtCtrl::GenericGetHeader() const
{
	return m_headerAreaWin;
}

void wxDataViewExtCtrl::CreateTableSizer()
{
	SetSizer(NULL);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	if (m_headerAreaWin)
		sizer->Add(m_headerAreaWin, 0, wxGROW);
	
	sizer->Add(m_tableAreaWin, 1, wxGROW);
	
	if (m_footerAreaWin)
		sizer->Add(m_footerAreaWin, 0, wxGROW);
	
	SetSizer(sizer);
}

void wxDataViewExtCtrl::ShowHeaderWindow(bool e)
{
	int flags = GetWindowStyleFlag();

	if (e)
		flags &= ~wxDV_NO_HEADER;
	else
		flags |= wxDV_NO_HEADER;

	SetWindowStyleFlag(flags);

	if (m_headerAreaWin != NULL && HasFlag(wxDV_NO_HEADER))
	{
		if (m_headerAreaWin)
			m_headerAreaWin->Destroy();
		
		m_headerAreaWin = NULL;
		CreateTableSizer();
	}
	else if (m_headerAreaWin == NULL && !HasFlag(wxDV_NO_HEADER))
	{
		m_headerAreaWin = new wxDataViewExtHeaderWindow(this);
		m_headerAreaWin->SetColumnCount(GetColumnCount());

		CreateTableSizer();
	}
}

void wxDataViewExtCtrl::ShowFooterWindow(bool e)
{
	int flags = GetWindowStyleFlag();

	if (!e)
		flags &= ~wxDV_FOOTER;
	else
		flags |= wxDV_FOOTER;

	SetWindowStyleFlag(flags);

	if (m_footerAreaWin != NULL && !HasFlag(wxDV_FOOTER))
	{
		if (m_footerAreaWin)
			m_footerAreaWin->Destroy();

		m_footerAreaWin = NULL;
		CreateTableSizer();
	}
	else if (m_footerAreaWin == NULL && HasFlag(wxDV_FOOTER))
	{
		m_footerAreaWin = new wxDataViewExtFooterWindow(this);
		m_footerAreaWin->SetColumnCount(GetColumnCount());

		CreateTableSizer();
	}
}

#ifdef __WXMSW__
WXLRESULT wxDataViewExtCtrl::MSWWindowProc(WXUINT nMsg,
	WXWPARAM wParam,
	WXLPARAM lParam)
{
	WXLRESULT rc = wxDataViewExtCtrlBase::MSWWindowProc(nMsg, wParam, lParam);

	// we need to process arrows ourselves for scrolling
	if (nMsg == WM_GETDLGCODE)
	{
		rc |= DLGC_WANTARROWS;
	}

	return rc;
}
#endif

wxSize wxDataViewExtCtrl::GetSizeAvailableForScrollTarget(const wxSize& size)
{
	wxSize newsize = size;
	if (!HasFlag(wxDV_NO_HEADER) && (m_headerAreaWin))
		newsize.y -= m_headerAreaWin->GetSize().y;

	if (HasFlag(wxDV_FOOTER) && (m_footerAreaWin))
		newsize.y -= m_footerAreaWin->GetSize().y;

	return newsize;
}

void wxDataViewExtCtrl::OnSize(wxSizeEvent& event)
{
	int cw, ch;
	GetClientSize(&cw, &ch);

	// We need to override OnSize so that our scrolled
	// window a) does call Layout() to use sizers for
	// positioning the controls but b) does not query
	// the sizer for their size and use that for setting
	// the scrollable area as set that ourselves by
	// calling SetScrollbar() further down.

	Layout();

	// Update the last column size to take all the available space. Note that
	// this must be done after calling Layout() to update m_tableAreaWin size.
	if (m_tableAreaWin)
		m_tableAreaWin->UpdateColumnSizes();

	AdjustScrollbars();

	// We must redraw the headers if their height changed. Normally this
	// shouldn't happen as the control shouldn't let itself be resized beneath
	// its minimal height but avoid the display artefacts that appear if it
	// does happen, e.g. because there is really not enough vertical space.
	if (!HasFlag(wxDV_NO_HEADER) && m_headerAreaWin &&
		m_headerAreaWin->GetSize().y <= m_headerAreaWin->GetBestSize().y)
	{
		m_headerAreaWin->Refresh();
	}

	if (HasFlag(wxDV_FOOTER) && m_footerAreaWin &&
		m_footerAreaWin->GetSize().y <= m_footerAreaWin->GetBestSize().y)
	{
		m_footerAreaWin->Refresh();
	}

	event.Skip();
}

void wxDataViewExtCtrl::OnDPIChanged(wxDPIChangedEvent& event)
{
	if (m_tableAreaWin)
	{
		m_tableAreaWin->ClearRowHeightCache();
		m_tableAreaWin->SetRowHeight(m_tableAreaWin->GetDefaultRowHeight());
	}

	for (unsigned i = 0; i < m_cols.size(); ++i)
	{
		int minWidth = m_cols[i]->GetMinWidth();
		if (minWidth > 0)
			minWidth = event.ScaleX(minWidth);
		m_cols[i]->SetMinWidth(minWidth);

		int width = m_cols[i]->WXGetSpecifiedWidth();
		if (width > 0)
			width = event.ScaleX(width);
		m_cols[i]->SetWidth(width);
	}

	event.Skip();
}

void wxDataViewExtCtrl::SetFocus()
{
	if (m_tableAreaWin)
		m_tableAreaWin->SetFocus();
}

bool wxDataViewExtCtrl::SetFont(const wxFont& font)
{
	if (!BaseType::SetFont(font))
		return false;

	if (m_tableAreaWin)
	{
		m_tableAreaWin->SetRowHeight(m_tableAreaWin->GetDefaultRowHeight());
	}

	if (m_headerAreaWin || m_tableAreaWin)
	{
		InvalidateColBestWidths();
		Layout();
	}

	return true;
}

bool wxDataViewExtCtrl::SetForegroundColour(const wxColour& colour)
{
	// Previous versions of this class, not using wxCompositeWindow, as well as
	// the native versions of this control, don't change the header foreground
	// when this method is called and this could be more desirable in practice,
	// as well we being more compatible, so skip calling the base class version
	// that would change it as well and change only the main items area colour
	// here too.
	if (!wxDataViewExtCtrlBase::SetForegroundColour(colour))
		return false;

	if (m_tableAreaWin)
		m_tableAreaWin->SetForegroundColour(colour);

	return true;
}

bool wxDataViewExtCtrl::SetBackgroundColour(const wxColour& colour)
{
	// See SetForegroundColour() above.
	if (!wxDataViewExtCtrlBase::SetBackgroundColour(colour))
		return false;

	if (m_tableAreaWin)
		m_tableAreaWin->SetBackgroundColour(colour);

	return true;
}

#if wxUSE_ACCESSIBILITY
bool wxDataViewExtCtrl::Show(bool show)
{
	bool changed = wxControl::Show(show);
	if (changed)
	{
		wxAccessible::NotifyEvent(show ? wxACC_EVENT_OBJECT_SHOW : wxACC_EVENT_OBJECT_HIDE,
			this, wxOBJID_CLIENT, wxACC_SELF);
	}

	return changed;
}

void wxDataViewExtCtrl::SetName(const wxString& name)
{
	wxControl::SetName(name);
	wxAccessible::NotifyEvent(wxACC_EVENT_OBJECT_NAMECHANGE, this, wxOBJID_CLIENT, wxACC_SELF);
}

bool wxDataViewExtCtrl::Reparent(wxWindowBase* newParent)
{
	bool changed = wxControl::Reparent(newParent);
	if (changed)
	{
		wxAccessible::NotifyEvent(wxACC_EVENT_OBJECT_PARENTCHANGE, this, wxOBJID_CLIENT, wxACC_SELF);
	}

	return changed;
}
#endif // wxUSE_ACCESIBILITY

bool wxDataViewExtCtrl::Enable(bool enable)
{
	bool changed = wxControl::Enable(enable);
	if (changed)
	{
#if wxUSE_ACCESSIBILITY
		wxAccessible::NotifyEvent(wxACC_EVENT_OBJECT_STATECHANGE, this, wxOBJID_CLIENT, wxACC_SELF);
#endif // wxUSE_ACCESIBILITY
		Refresh();
	}

	return changed;
}

void wxDataViewExtCtrl::ScrollWindow(int dx, int dy, const wxRect* rect)
{
	// We must explicitly call wxWindow version to avoid infinite recursion as
	// wxGridExtWindow::ScrollWindow() calls this method back.

	if (m_headerAreaWin)
		m_headerAreaWin->ScrollWindow(dx, 0);

	if (m_footerAreaWin)
		m_footerAreaWin->ScrollWindow(dx, 0);

	if (m_tableAreaWin)
		m_tableAreaWin->wxWindow::ScrollWindow(dx, dy, rect);
}

bool wxDataViewExtCtrl::AssociateModel(wxDataViewExtModel* model)
{
	if (!wxDataViewExtCtrlBase::AssociateModel(model))
		return false;

	if (model)
	{
		m_notifier = new wxGenericDataViewModelNotifier(m_tableAreaWin);
		model->AddNotifier(m_notifier);
	}
	else
	{
		// Our previous notifier has either been already deleted when the
		// previous model was DecRef()'d in the base class AssociateModel() or
		// is not associated with us any more because if the model is still
		// alive, it's not used by this control.
		m_notifier = NULL;
	}

	m_tableAreaWin->DestroyTree();

	if (model)
	{
		m_tableAreaWin->BuildTree(model);
	}

	m_tableAreaWin->UpdateDisplay();

	return true;
}

#if wxUSE_DRAG_AND_DROP

bool wxDataViewExtCtrl::EnableDragSource(const wxDataFormat& format)
{
	return m_tableAreaWin->EnableDragSource(format);
}

bool wxDataViewExtCtrl::DoEnableDropTarget(const wxVector<wxDataFormat>& formats)
{
	wxDataViewExtDropTarget* dt = NULL;
	if (wxDataObjectComposite* dataObject = CreateDataObject(formats))
	{
		dt = new wxDataViewExtDropTarget(dataObject, m_tableAreaWin);
	}

	m_tableAreaWin->SetDropTarget(dt);

	return true;
}

#endif // wxUSE_DRAG_AND_DROP

bool wxDataViewExtCtrl::AppendColumn(wxDataViewExtColumn* col)
{
	if (!wxDataViewExtCtrlBase::AppendColumn(col))
		return false;

	m_cols.push_back(col);
	m_colsBestWidths.push_back(CachedColWidthInfo());
	OnColumnsCountChanged();
	return true;
}

bool wxDataViewExtCtrl::PrependColumn(wxDataViewExtColumn* col)
{
	if (!wxDataViewExtCtrlBase::PrependColumn(col))
		return false;

	m_cols.insert(m_cols.begin(), col);
	m_colsBestWidths.insert(m_colsBestWidths.begin(), CachedColWidthInfo());
	OnColumnsCountChanged();
	return true;
}

bool wxDataViewExtCtrl::InsertColumn(unsigned int pos, wxDataViewExtColumn* col)
{
	if (!wxDataViewExtCtrlBase::InsertColumn(pos, col))
		return false;

	m_cols.insert(m_cols.begin() + pos, col);
	m_colsBestWidths.insert(m_colsBestWidths.begin() + pos, CachedColWidthInfo());
	OnColumnsCountChanged();
	return true;
}

void wxDataViewExtCtrl::OnColumnResized()
{
	m_tableAreaWin->UpdateDisplay();
}

void wxDataViewExtCtrl::OnColumnWidthChange(unsigned int idx)
{
	InvalidateColBestWidth(idx);

	OnColumnChange(idx);
}

void wxDataViewExtCtrl::OnColumnChange(unsigned int idx)
{
	if (m_headerAreaWin)
		m_headerAreaWin->UpdateColumn(idx);

	if (m_footerAreaWin)
		m_footerAreaWin->UpdateColumn(idx);

	m_tableAreaWin->UpdateDisplay();
}

void wxDataViewExtCtrl::OnColumnsCountChanged()
{
	if (m_headerAreaWin)
		m_headerAreaWin->SetColumnCount(GetColumnCount());

	if (m_footerAreaWin)
		m_footerAreaWin->SetColumnCount(GetColumnCount());

	m_tableAreaWin->OnColumnsCountChanged();
}

void wxDataViewExtCtrl::DoSetExpanderColumn()
{
	wxDataViewExtColumn* column = GetExpanderColumn();
	if (column)
	{
		int index = GetColumnIndex(column);
		if (index != wxNOT_FOUND)
			InvalidateColBestWidth(index);
	}

	m_tableAreaWin->UpdateDisplay();
}

void wxDataViewExtCtrl::DoSetIndent()
{
	m_tableAreaWin->UpdateDisplay();
}

unsigned int wxDataViewExtCtrl::GetColumnCount() const
{
	return m_cols.size();
}

bool wxDataViewExtCtrl::SetRowHeight(int lineHeight)
{
	if (!m_tableAreaWin)
		return false;

	m_tableAreaWin->SetRowHeight(lineHeight);

	return true;
}

wxDataViewExtColumn* wxDataViewExtCtrl::GetColumn(unsigned int idx) const
{
	return m_cols[idx];
}

wxDataViewExtColumn* wxDataViewExtCtrl::GetColumnAt(unsigned int pos) const
{
	// columns can't be reordered if there is no header window which allows
	// to do this
	const unsigned idx = m_headerAreaWin ? m_headerAreaWin->GetColumnsOrder()[pos]
		: pos;

	return GetColumn(idx);
}

int wxDataViewExtCtrl::GetColumnIndex(const wxDataViewExtColumn* column) const
{
	const unsigned count = m_cols.size();
	for (unsigned n = 0; n < count; n++)
	{
		if (m_cols[n] == column)
			return n;
	}

	return wxNOT_FOUND;
}

int wxDataViewExtCtrl::GetModelColumnIndex(unsigned int model_column) const
{
	const int count = GetColumnCount();
	for (int index = 0; index < count; index++)
	{
		wxDataViewExtColumn* column = GetColumn(index);
		if (column->GetModelColumn() == model_column)
			return index;
	}
	return wxNOT_FOUND;
}

class wxDataViewExtMaxWidthCalculator : public wxMaxWidthCalculatorBase
{
public:
	wxDataViewExtMaxWidthCalculator(const wxDataViewExtCtrl* dvc,
		wxDataViewExtMainWindow* clientArea,
		wxDataViewExtRenderer* renderer,
		const wxDataViewExtModel* model,
		size_t model_column,
		int expanderSize)
		: wxMaxWidthCalculatorBase(model_column),
		m_dvc(dvc),
		m_tableAreaWin(clientArea),
		m_renderer(renderer),
		m_model(model),
		m_expanderSize(expanderSize)
	{
		int index = dvc->GetModelColumnIndex(model_column);
		wxDataViewExtColumn* column = index == wxNOT_FOUND ? NULL : dvc->GetColumn(index);
		m_isExpanderCol =
			!clientArea->IsList() &&
			(column == 0 ||
				GetExpanderColumnOrFirstOne(const_cast<wxDataViewExtCtrl*>(dvc)) == column);
	}

	virtual void UpdateWithRow(int row) wxOVERRIDE
	{
		int width = 0;
		wxDataViewExtItem item;

		if (m_isExpanderCol)
		{
			wxDataViewExtTreeNode* node = m_tableAreaWin->GetTreeNodeByRow(row);
			item = node->GetItem();
			width = m_dvc->GetIndent() * node->GetIndentLevel() + m_expanderSize;
		}
		else
		{
			item = m_tableAreaWin->GetItemByRow(row);
		}

		if (m_model->HasValue(item, GetColumn()))
		{
			if (m_renderer->PrepareForItem(m_model, item, GetColumn()))
				width += m_renderer->GetSize().x;
		}

		UpdateWithWidth(width);
	}

private:
	const wxDataViewExtCtrl* m_dvc;
	wxDataViewExtMainWindow* m_tableAreaWin;
	wxDataViewExtRenderer* m_renderer;
	const wxDataViewExtModel* m_model;
	bool m_isExpanderCol;
	int m_expanderSize;
};


unsigned int wxDataViewExtCtrl::GetBestColumnWidth(int idx) const
{
	if (m_colsBestWidths[idx].width != 0)
		return m_colsBestWidths[idx].width;

	const int count = m_tableAreaWin->GetRowCount();
	wxDataViewExtColumn* column = GetColumn(idx);
	wxDataViewExtRenderer* renderer =
		const_cast<wxDataViewExtRenderer*>(column->GetRenderer());

	wxDataViewExtMaxWidthCalculator calculator(this, m_tableAreaWin, renderer,
		GetModel(), column->GetModelColumn(),
		m_tableAreaWin->GetRowHeight());

	calculator.UpdateWithWidth(column->GetMinWidth());

	if (m_headerAreaWin)
		calculator.UpdateWithWidth(m_headerAreaWin->GetColumnTitleWidth(*column));

	if (m_footerAreaWin)
		calculator.UpdateWithWidth(m_footerAreaWin->GetColumnTitleWidth(*column));

	const wxPoint origin = CalcUnscrolledPosition(wxPoint(0, 0));
	calculator.ComputeBestColumnWidth(count,
		m_tableAreaWin->GetLineAt(origin.y),
		m_tableAreaWin->GetLineAt(origin.y + GetClientSize().y));

	int max_width = calculator.GetMaxWidth();
	if (max_width > 0)
		max_width += 2 * FromDIP(PADDING_RIGHTLEFT);

	const_cast<wxDataViewExtCtrl*>(this)->m_colsBestWidths[idx].width = max_width;
	return max_width;
}

void wxDataViewExtCtrl::ColumnMoved(wxDataViewExtColumn* col, unsigned int new_pos)
{
	// do _not_ reorder m_cols elements here, they should always be in the
	// order in which columns were added, we only display the columns in
	// different order
	m_tableAreaWin->UpdateDisplay();

	wxDataViewExtEvent event(wxEVT_DATAVIEW_COLUMN_REORDERED, this, col);
	event.SetColumn(new_pos);
	ProcessWindowEvent(event);
}

bool wxDataViewExtCtrl::DeleteColumn(wxDataViewExtColumn* column)
{
	const int idx = GetColumnIndex(column);
	if (idx == wxNOT_FOUND)
		return false;

	m_colsBestWidths.erase(m_colsBestWidths.begin() + idx);
	m_cols.erase(m_cols.begin() + idx);

	if (m_tableAreaWin->GetCurrentColumn() == column)
		m_tableAreaWin->ClearCurrentColumn();

	OnColumnsCountChanged();

	return true;
}

void wxDataViewExtCtrl::DoClearColumns()
{
	typedef wxVector<wxDataViewExtColumn*>::const_iterator citer;
	for (citer it = m_cols.begin(); it != m_cols.end(); ++it)
		delete* it;
}

bool wxDataViewExtCtrl::ClearColumns()
{
	SetExpanderColumn(NULL);

	DoClearColumns();

	m_cols.clear();
	m_sortingColumnIdxs.clear();
	m_colsBestWidths.clear();

	m_tableAreaWin->ClearCurrentColumn();

	OnColumnsCountChanged();

	return true;
}

void wxDataViewExtCtrl::InvalidateColBestWidth(int idx)
{
	m_colsBestWidths[idx].width = 0;
	m_colsBestWidths[idx].dirty = true;
	m_colsDirty = true;
}

void wxDataViewExtCtrl::InvalidateColBestWidths()
{
	// mark all columns as dirty:
	m_colsBestWidths.clear();
	m_colsBestWidths.resize(m_cols.size());
	m_colsDirty = true;
}

void wxDataViewExtCtrl::UpdateColWidths()
{
	m_colsDirty = false;

	if (!m_headerAreaWin && !m_footerAreaWin)
		return;

	const unsigned len = m_colsBestWidths.size();
	for (unsigned i = 0; i < len; i++)
	{
		// Note that we have to have an explicit 'dirty' flag here instead of
		// checking if the width==0, as is done in GetBestColumnWidth().
		//
		// Testing width==0 wouldn't work correctly if some code called
		// GetWidth() after col. width invalidation but before
		// wxDataViewExtCtrl::UpdateColWidths() was called at idle time. This
		// would result in the header's column width getting out of sync with
		// the control itself.
		if (m_colsBestWidths[i].dirty)
		{
			if (m_headerAreaWin)
				m_headerAreaWin->UpdateColumn(i);
			if (m_footerAreaWin)
				m_footerAreaWin->UpdateColumn(i);

			m_colsBestWidths[i].dirty = false;
		}
	}
}

void wxDataViewExtCtrl::OnInternalIdle()
{
	wxDataViewExtCtrlBase::OnInternalIdle();

	if (m_colsDirty)
		UpdateColWidths();
}

int wxDataViewExtCtrl::GetColumnPosition(const wxDataViewExtColumn* column) const
{
	unsigned int len = GetColumnCount();
	for (unsigned int i = 0; i < len; i++)
	{
		wxDataViewExtColumn* col = GetColumnAt(i);
		if (column == col)
			return i;
	}

	return wxNOT_FOUND;
}

wxDataViewExtColumn* wxDataViewExtCtrl::GetSortingColumn() const
{
	if (m_sortingColumnIdxs.empty())
		return NULL;

	return GetColumn(m_sortingColumnIdxs.front());
}

wxVector<wxDataViewExtColumn*> wxDataViewExtCtrl::GetSortingColumns() const
{
	wxVector<wxDataViewExtColumn*> out;

	for (wxVector<int>::const_iterator it = m_sortingColumnIdxs.begin(),
		end = m_sortingColumnIdxs.end();
		it != end;
		++it)
	{
		out.push_back(GetColumn(*it));
	}

	return out;
}

wxDataViewExtItem wxDataViewExtCtrl::DoGetCurrentItem() const
{
	return GetItemByRow(m_tableAreaWin->GetCurrentRow());
}

void wxDataViewExtCtrl::DoSetCurrentItem(const wxDataViewExtItem& item)
{
	const int row = m_tableAreaWin->GetRowByItem(item);

	const unsigned oldCurrent = m_tableAreaWin->GetCurrentRow();
	if (static_cast<unsigned>(row) != oldCurrent)
	{
		m_tableAreaWin->ChangeCurrentRow(row);
		m_tableAreaWin->RefreshRow(oldCurrent);
		m_tableAreaWin->RefreshRow(row);
	}
}

wxDataViewExtColumn* wxDataViewExtCtrl::GetCurrentColumn() const
{
	return m_tableAreaWin->GetCurrentColumn();
}

int wxDataViewExtCtrl::GetSelectedItemsCount() const
{
	return m_tableAreaWin->GetSelections().GetSelectedCount();
}

wxDataViewExtItem wxDataViewExtCtrl::GetTopItem() const
{
	return m_tableAreaWin->GetTopItem();
}

int wxDataViewExtCtrl::GetCountPerPage() const
{
	return m_tableAreaWin->GetCountPerPage();
}

int wxDataViewExtCtrl::GetSelections(wxDataViewExtItemArray& sel) const
{
	sel.Empty();
	const wxSelectionStore& selections = m_tableAreaWin->GetSelections();

	wxSelectionStore::IterationState cookie;
	for (unsigned row = selections.GetFirstSelectedItem(cookie);
		row != wxSelectionStore::NO_SELECTION;
		row = selections.GetNextSelectedItem(cookie))
	{
		wxDataViewExtItem item = m_tableAreaWin->GetItemByRow(row);
		if (item.IsOk())
		{
			sel.Add(item);
		}
		else
		{
			wxFAIL_MSG("invalid item in selection - bad internal state");
		}
	}

	return sel.size();
}

void wxDataViewExtCtrl::SetSelections(const wxDataViewExtItemArray& sel)
{
	m_tableAreaWin->ClearSelection();

	if (sel.empty())
		return;

	wxDataViewExtItem last_parent;

	for (size_t i = 0; i < sel.size(); i++)
	{
		wxDataViewExtItem item = sel[i];
		wxDataViewExtItem parent = GetModel()->GetParent(item);
		if (parent)
		{
			if (parent != last_parent)
				ExpandAncestors(item);
		}

		last_parent = parent;
		int row = m_tableAreaWin->GetRowByItem(item);
		if (row >= 0)
			m_tableAreaWin->SelectRow(static_cast<unsigned int>(row), true);
	}

	// Also make the last item as current item
	DoSetCurrentItem(sel.Last());
}

void wxDataViewExtCtrl::Select(const wxDataViewExtItem& item)
{
	ExpandAncestors(item);

	int row = m_tableAreaWin->GetRowByItem(item);
	if (row >= 0)
	{
		// Unselect all rows before select another in the single select mode
		if (m_tableAreaWin->IsSingleSel())
			m_tableAreaWin->UnselectAllRows();

		// Set top parent 
		//m_tableAreaWin->SetTopParent(item);
		m_tableAreaWin->SelectRow(row, true);

		// Also set focus to the selected item
		m_tableAreaWin->ChangeCurrentRow(row);
	}
}

void wxDataViewExtCtrl::Unselect(const wxDataViewExtItem& item)
{
	int row = m_tableAreaWin->GetRowByItem(item);
	if (row >= 0)
		m_tableAreaWin->SelectRow(row, false);
}

bool wxDataViewExtCtrl::IsSelected(const wxDataViewExtItem& item) const
{
	int row = m_tableAreaWin->GetRowByItem(item);
	if (row >= 0)
	{
		return m_tableAreaWin->IsRowSelected(row);
	}
	return false;
}

bool wxDataViewExtCtrl::SetHeaderAttr(const wxItemAttr& attr)
{
	if (!m_headerAreaWin && !m_footerAreaWin)
		return false;

	// Call all functions unconditionally to reset the previously set
	// attributes, if any.
	if (m_headerAreaWin)
	{
		m_headerAreaWin->SetForegroundColour(attr.GetTextColour());
		m_headerAreaWin->SetBackgroundColour(attr.GetBackgroundColour());
		m_headerAreaWin->SetFont(attr.GetFont());
	}

	if (m_footerAreaWin)
	{
		m_footerAreaWin->SetForegroundColour(attr.GetTextColour());
		m_footerAreaWin->SetBackgroundColour(attr.GetBackgroundColour());
		m_footerAreaWin->SetFont(attr.GetFont());
	}

	// If the font has changed, the size of the header might need to be
	// updated.
	Layout();

	return true;
}

bool wxDataViewExtCtrl::SetAlternateRowColour(const wxColour& colour)
{
	m_alternateRowColour = colour;
	return true;
}

void wxDataViewExtCtrl::SelectAll()
{
	m_tableAreaWin->SelectAllRows();
}

void wxDataViewExtCtrl::UnselectAll()
{
	m_tableAreaWin->UnselectAllRows();
}

void wxDataViewExtCtrl::SetSelectionMode(wxDataViewExtSelectionMode selmode)
{
	m_tableAreaWin->SetSelectionMode(selmode);
}

wxDataViewExtSelectionMode wxDataViewExtCtrl::GetSelectionMode() const
{
	return m_tableAreaWin->GetSelectionMode();
}

void wxDataViewExtCtrl::SetViewMode(wxDataViewExtViewMode selmode)
{
	m_tableAreaWin->SetViewMode(selmode);
}

wxDataViewExtViewMode wxDataViewExtCtrl::GetViewMode() const
{
	return m_tableAreaWin->GetViewMode();
}

void wxDataViewExtCtrl::SetTopParent(const wxDataViewExtItem& item) const
{
	m_tableAreaWin->SetTopParent(item);
}

void wxDataViewExtCtrl::EnsureVisibleRowCol(int row, int column)
{
	if (row < 0)
		row = 0;
	if (row > (int) m_tableAreaWin->GetRowCount())
		row = m_tableAreaWin->GetRowCount();

	int first = m_tableAreaWin->GetFirstVisibleRow();
	int last = m_tableAreaWin->GetLastFullyVisibleRow();
	if (row <= first)
	{
		m_tableAreaWin->ScrollTo(row, column);
	}
	else if (row > last)
	{
		if (!HasFlag(wxDV_VARIABLE_LINE_HEIGHT))
		{
			// Simple case as we can directly find the item to scroll to.
			m_tableAreaWin->ScrollTo(row - last + first, column);
		}
		else
		{
			// calculate scroll position based on last visible item
			const int itemStart = m_tableAreaWin->GetLineStart(row);
			const int itemHeight = m_tableAreaWin->GetLineHeight(row);
			const int clientHeight = m_tableAreaWin->GetSize().y;
			int scrollX, scrollY;
			GetScrollPixelsPerUnit(&scrollX, &scrollY);
			int scrollPosY =
				(itemStart + itemHeight - clientHeight + scrollY - 1) / scrollY;
			int scrollPosX = -1;
			if (column != -1 && scrollX)
				scrollPosX = m_tableAreaWin->GetColumnStart(column) / scrollX;
			Scroll(scrollPosX, scrollPosY);
		}
	}
}

void wxDataViewExtCtrl::EnsureVisible(const wxDataViewExtItem& item, const wxDataViewExtColumn* column)
{
	ExpandAncestors(item);

	m_tableAreaWin->RecalculateDisplay();

	int row = m_tableAreaWin->GetRowByItem(item);
	if (row >= 0)
	{
		if (column == NULL)
			EnsureVisibleRowCol(row, -1);
		else
			EnsureVisibleRowCol(row, GetColumnIndex(column));
	}

}

void wxDataViewExtCtrl::HitTest(const wxPoint& point, wxDataViewExtItem& item,
	wxDataViewExtColumn*& column) const
{
	// Convert from wxDataViewExtCtrl coordinates to wxDataViewExtMainWindow coordinates.
	// (They can be different due to the presence of the header.).
	const wxPoint clientPt = m_tableAreaWin->ScreenToClient(ClientToScreen(point));
	m_tableAreaWin->HitTest(clientPt, item, column);
}

wxRect wxDataViewExtCtrl::GetItemRect(const wxDataViewExtItem& item,
	const wxDataViewExtColumn* column) const
{
	// Convert position from the main window coordinates to the control coordinates.
	// (They can be different due to the presence of the header.).
	wxRect r = m_tableAreaWin->GetItemRect(item, column);
	if (r.width || r.height)
	{
		const wxPoint ctrlPos = ScreenToClient(m_tableAreaWin->ClientToScreen(r.GetPosition()));
		r.SetPosition(ctrlPos);
	}
	return r;
}

wxDataViewExtItem wxDataViewExtCtrl::GetItemByRow(unsigned int row) const
{
	return m_tableAreaWin->GetItemByRow(row);
}

int wxDataViewExtCtrl::GetRowByItem(const wxDataViewExtItem& item) const
{
	return m_tableAreaWin->GetRowByItem(item);
}

void wxDataViewExtCtrl::DoExpand(const wxDataViewExtItem& item, bool expandChildren)
{
	int row = m_tableAreaWin->GetRowByItem(item);
	if (row != -1)
		m_tableAreaWin->Expand(row, expandChildren);
}

void wxDataViewExtCtrl::Collapse(const wxDataViewExtItem& item)
{
	int row = m_tableAreaWin->GetRowByItem(item);
	if (row != -1)
		m_tableAreaWin->Collapse(row);
}

bool wxDataViewExtCtrl::IsExpanded(const wxDataViewExtItem& item) const
{
	int row = m_tableAreaWin->GetRowByItem(item);
	if (row != -1)
		return m_tableAreaWin->IsExpanded(row);
	return false;
}

void wxDataViewExtCtrl::EditItem(const wxDataViewExtItem& item, const wxDataViewExtColumn* column)
{
	wxCHECK_RET(item.IsOk(), "invalid item");
	wxCHECK_RET(column, "no column provided");

	m_tableAreaWin->StartEditing(item, column);
}

void wxDataViewExtCtrl::ResetAllSortColumns()
{
	// Must make copy, because unsorting will remove it from original vector
	wxVector<int> const copy(m_sortingColumnIdxs);
	for (wxVector<int>::const_iterator it = copy.begin(),
		end = copy.end();
		it != end;
		++it)
	{
		GetColumn(*it)->UnsetAsSortKey();
	}

	wxASSERT(m_sortingColumnIdxs.empty());
}

bool wxDataViewExtCtrl::AllowMultiColumnSort(bool allow)
{
	if (m_allowMultiColumnSort == allow)
		return true;

	m_allowMultiColumnSort = allow;

	// If disabling, must disable any multiple sort that are active
	if (!allow)
	{
		ResetAllSortColumns();

		if (wxDataViewExtModel* model = GetModel())
			model->Resort();
	}

	return true;
}


bool wxDataViewExtCtrl::IsColumnSorted(int idx) const
{
	for (wxVector<int>::const_iterator it = m_sortingColumnIdxs.begin(),
		end = m_sortingColumnIdxs.end();
		it != end;
		++it)
	{
		if (*it == idx)
			return true;
	}

	return false;
}

void wxDataViewExtCtrl::UseColumnForSorting(int idx)
{
	m_sortingColumnIdxs.push_back(idx);
}

void wxDataViewExtCtrl::DontUseColumnForSorting(int idx)
{
	for (wxVector<int>::iterator it = m_sortingColumnIdxs.begin(),
		end = m_sortingColumnIdxs.end();
		it != end;
		++it)
	{
		if (*it == idx)
		{
			m_sortingColumnIdxs.erase(it);
			return;
		}
	}

	wxFAIL_MSG("Column is not used for sorting");
}

void wxDataViewExtCtrl::ToggleSortByColumn(int column)
{
	m_headerAreaWin->ToggleSortByColumn(column);
}

void wxDataViewExtCtrl::DoEnableSystemTheme(bool enable, wxWindow* window)
{
	typedef wxSystemThemedControl<wxControl> Base;
	Base::DoEnableSystemTheme(enable, window);
	Base::DoEnableSystemTheme(enable, m_tableAreaWin);
	if (m_headerAreaWin)
		Base::DoEnableSystemTheme(enable, m_headerAreaWin);
	if (m_footerAreaWin)
		Base::DoEnableSystemTheme(enable, m_footerAreaWin);
}

#if wxUSE_ACCESSIBILITY
wxAccessible* wxDataViewExtCtrl::CreateAccessible()
{
	return new wxDataViewExtCtrlAccessible(this);
}
#endif // wxUSE_ACCESSIBILITY

#if wxUSE_ACCESSIBILITY
//-----------------------------------------------------------------------------
// wxDataViewExtCtrlAccessible
//-----------------------------------------------------------------------------

wxDataViewExtCtrlAccessible::wxDataViewExtCtrlAccessible(wxDataViewExtCtrl* win)
	: wxWindowAccessible(win)
{
}

// Can return either a child object, or an integer
// representing the child element, starting from 1.
wxAccStatus wxDataViewExtCtrlAccessible::HitTest(const wxPoint& pt,
	int* childId, wxAccessible** childObject)
{
	wxDataViewExtCtrl* dvCtrl = wxDynamicCast(GetWindow(), wxDataViewExtCtrl);
	wxCHECK(dvCtrl, wxACC_FAIL);

	wxDataViewExtItem item;
	wxDataViewExtColumn* col;
	const wxPoint posCtrl = dvCtrl->ScreenToClient(pt);
	dvCtrl->HitTest(posCtrl, item, col);
	if (item.IsOk())
	{
		*childId = dvCtrl->GetRowByItem(item) + 1;
		*childObject = NULL;
	}
	else
	{
		if (((wxWindow*)dvCtrl)->HitTest(posCtrl) == wxHT_WINDOW_INSIDE)
		{
			// First check if provided point belongs to the header
			// because header control handles accesibility requestes on its own.
			wxHeaderGenericCtrl* dvHdr = dvCtrl->GenericGetHeader();
			if (dvHdr)
			{
				const wxPoint posHdr = dvHdr->ScreenToClient(pt);
				if (dvHdr->HitTest(posHdr) == wxHT_WINDOW_INSIDE)
				{
					*childId = wxACC_SELF;
					*childObject = dvHdr->GetOrCreateAccessible();
					return wxACC_OK;
				}
			}

			*childId = wxACC_SELF;
			*childObject = this;
		}
		else
		{
			*childId = wxACC_SELF;
			*childObject = NULL;
		}
	}

	return wxACC_OK;
}

// Returns the rectangle for this object (id = 0) or a child element (id > 0).
wxAccStatus wxDataViewExtCtrlAccessible::GetLocation(wxRect& rect, int elementId)
{
	wxDataViewExtCtrl* dvCtrl = wxDynamicCast(GetWindow(), wxDataViewExtCtrl);
	wxCHECK(dvCtrl, wxACC_FAIL);
	wxDataViewExtMainWindow* dvWnd = wxDynamicCast(dvCtrl->GetMainWindow(), wxDataViewExtMainWindow);

	if (elementId == wxACC_SELF)
	{
		// Header accesibility requestes are handled separately
		// so header is excluded from effective client area
		// and hence only main window area is reported.
		rect = dvWnd->GetScreenRect();
	}
	else
	{
		wxDataViewExtItem item = dvWnd->GetItemByRow(elementId - 1);
		if (!item.IsOk())
		{
			return wxACC_NOT_IMPLEMENTED;
		}

		rect = dvWnd->GetItemRect(item, NULL);
		// Indentation and expander column should be included here and therefore
		// reported row width should by the same as the width of the client area.
		rect.width += rect.x;
		rect.x = 0;
		wxPoint posScreen = dvWnd->ClientToScreen(rect.GetPosition());
		rect.SetPosition(posScreen);
	}

	return wxACC_OK;
}

// Navigates from fromId to toId/toObject.
wxAccStatus wxDataViewExtCtrlAccessible::Navigate(wxNavDir navDir, int fromId,
	int* toId, wxAccessible** toObject)
{
	wxDataViewExtCtrl* dvCtrl = wxDynamicCast(GetWindow(), wxDataViewExtCtrl);
	wxCHECK(dvCtrl, wxACC_FAIL);
	wxDataViewExtMainWindow* dvWnd = wxDynamicCast(dvCtrl->GetMainWindow(), wxDataViewExtMainWindow);

	const int numRows = (int)dvWnd->GetRowCount();

	if (fromId == wxACC_SELF)
	{
		switch (navDir)
		{
		case wxNAVDIR_FIRSTCHILD:
			if (numRows > 0)
			{
				*toId = 1;
				*toObject = NULL;
				return wxACC_OK;
			}
			return wxACC_FALSE;
		case wxNAVDIR_LASTCHILD:
			if (numRows > 0)
			{
				*toId = numRows;
				*toObject = NULL;
				return wxACC_OK;
			}
			return wxACC_FALSE;
		case wxNAVDIR_DOWN:
			wxFALLTHROUGH;
		case wxNAVDIR_NEXT:
			wxFALLTHROUGH;
		case wxNAVDIR_UP:
			wxFALLTHROUGH;
		case wxNAVDIR_PREVIOUS:
			wxFALLTHROUGH;
		case wxNAVDIR_LEFT:
			wxFALLTHROUGH;
		case wxNAVDIR_RIGHT:
			// Standard wxWindow navigation is applicable here.
			return wxWindowAccessible::Navigate(navDir, fromId, toId, toObject);
		}
	}
	else
	{
		switch (navDir)
		{
		case wxNAVDIR_FIRSTCHILD:
			return wxACC_FALSE;
		case wxNAVDIR_LASTCHILD:
			return wxACC_FALSE;
		case wxNAVDIR_LEFT:
			return wxACC_FALSE;
		case wxNAVDIR_RIGHT:
			return wxACC_FALSE;
		case wxNAVDIR_DOWN:
			wxFALLTHROUGH;
		case wxNAVDIR_NEXT:
			if (fromId < numRows)
			{
				*toId = fromId + 1;
				*toObject = NULL;
				return wxACC_OK;
			}
			return wxACC_FALSE;
		case wxNAVDIR_PREVIOUS:
			wxFALLTHROUGH;
		case wxNAVDIR_UP:
			if (fromId > 1)
			{
				*toId = fromId - 1;
				*toObject = NULL;
				return wxACC_OK;
			}
			return wxACC_FALSE;
		}
	}

	// Let the framework handle the other cases.
	return wxACC_NOT_IMPLEMENTED;
}

// Gets the name of the specified object.
wxAccStatus wxDataViewExtCtrlAccessible::GetName(int childId, wxString* name)
{
	wxDataViewExtCtrl* dvCtrl = wxDynamicCast(GetWindow(), wxDataViewExtCtrl);
	wxCHECK(dvCtrl, wxACC_FAIL);

	if (childId == wxACC_SELF)
	{
		*name = dvCtrl->GetName();
	}
	else
	{
		wxDataViewExtItem item = dvCtrl->GetItemByRow(childId - 1);
		if (!item.IsOk())
		{
			return wxACC_NOT_IMPLEMENTED;
		}

		// Name is the value in the first textual column
		// plus the name of this column:
		// Column1: Value1
		wxString itemName;

		wxDataViewExtModel* model = dvCtrl->GetModel();
		const unsigned int numCols = dvCtrl->GetColumnCount();
		for (unsigned int col = 0; col < numCols; col++)
		{
			wxDataViewExtColumn* dvCol = dvCtrl->GetColumnAt(col);
			if (dvCol->IsHidden())
				continue; // skip it

			wxVariant value;
			model->GetValue(value, item, dvCol->GetModelColumn());
			if (value.IsNull() || value.IsType(wxS("bool")))
				continue; // Skip non-textual items

			wxDataViewExtRenderer* r = dvCol->GetRenderer();
			if (!r->PrepareForItem(model, item, dvCol->GetModelColumn()))
				continue;

			wxString vs = r->GetAccessibleDescription();
			if (!vs.empty())
			{
				itemName = vs;
				break;
			}
		}

		if (itemName.empty())
		{
			// Return row number if no textual column found.
			// Rows are numbered from 1.
			*name = wxString::Format(_("Row %i"), childId);
		}
		else
		{
			*name = itemName;
		}
	}

	return wxACC_OK;
}

// Gets the number of children.
wxAccStatus wxDataViewExtCtrlAccessible::GetChildCount(int* childCount)
{
	wxDataViewExtCtrl* dvCtrl = wxDynamicCast(GetWindow(), wxDataViewExtCtrl);
	wxCHECK(dvCtrl, wxACC_FAIL);
	wxDataViewExtMainWindow* dvWnd = wxDynamicCast(dvCtrl->GetMainWindow(), wxDataViewExtMainWindow);

	*childCount = (int)dvWnd->GetRowCount();
	return wxACC_OK;
}

// Gets the specified child (starting from 1).
// If *child is NULL and return value is wxACC_OK,
// this means that the child is a simple element and
// not an accessible object.
wxAccStatus wxDataViewExtCtrlAccessible::GetChild(int childId, wxAccessible** child)
{
	*child = (childId == wxACC_SELF) ? this : NULL;
	return wxACC_OK;
}

// Performs the default action. childId is 0 (the action for this object)
// or > 0 (the action for a child).
// Return wxACC_NOT_SUPPORTED if there is no default action for this
// window (e.g. an edit control).
wxAccStatus wxDataViewExtCtrlAccessible::DoDefaultAction(int childId)
{
	wxDataViewExtCtrl* dvCtrl = wxDynamicCast(GetWindow(), wxDataViewExtCtrl);
	wxCHECK(dvCtrl, wxACC_FAIL);

	if (childId != wxACC_SELF)
	{
		wxDataViewExtMainWindow* dvWnd = wxDynamicCast(dvCtrl->GetMainWindow(), wxDataViewExtMainWindow);
		if (!dvWnd->IsList())
		{
			const unsigned int row = childId - 1;
			wxDataViewExtTreeNode* node = dvWnd->GetTreeNodeByRow(row);
			if (node)
			{
				if (node->HasChildren())
				{
					// Expand or collapse the node.
					if (node->IsOpen())
						dvWnd->Collapse(row);
					else
						dvWnd->Expand(row);
					return wxACC_OK;
				}
			}
		}
	}

	return wxACC_NOT_SUPPORTED;
}

// Gets the default action for this object (0) or > 0 (the action for a child).
// Return wxACC_OK even if there is no action. actionName is the action, or the empty
// string if there is no action.
// The retrieved string describes the action that is performed on an object,
// not what the object does as a result. For example, a toolbar button that prints
// a document has a default action of "Press" rather than "Prints the current document."
wxAccStatus wxDataViewExtCtrlAccessible::GetDefaultAction(int childId, wxString* actionName)
{
	wxDataViewExtCtrl* dvCtrl = wxDynamicCast(GetWindow(), wxDataViewExtCtrl);
	wxCHECK(dvCtrl, wxACC_FAIL);

	wxString action;
	if (childId != wxACC_SELF)
	{
		wxDataViewExtMainWindow* dvWnd = wxDynamicCast(dvCtrl->GetMainWindow(), wxDataViewExtMainWindow);
		if (!dvWnd->IsList())
		{
			wxDataViewExtTreeNode* node = dvWnd->GetTreeNodeByRow(childId - 1);
			if (node)
			{
				if (node->HasChildren())
				{
					if (node->IsOpen())
						/* TRANSLATORS: Action for manipulating a tree control */
						action = _("Collapse");
					else
						/* TRANSLATORS: Action for manipulating a tree control */
						action = _("Expand");
				}
			}
		}
	}

	*actionName = action;
	return wxACC_OK;
}

// Returns the description for this object or a child.
wxAccStatus wxDataViewExtCtrlAccessible::GetDescription(int childId, wxString* description)
{
	wxDataViewExtCtrl* dvCtrl = wxDynamicCast(GetWindow(), wxDataViewExtCtrl);
	wxCHECK(dvCtrl, wxACC_FAIL);

	if (childId == wxACC_SELF)
	{
		wxDataViewExtMainWindow* dvWnd = wxDynamicCast(dvCtrl->GetMainWindow(), wxDataViewExtMainWindow);
		*description = wxString::Format(_("%s (%d items)"),
			dvCtrl->GetName().c_str(), dvWnd->GetRowCount());
	}
	else
	{
		wxDataViewExtItem item = dvCtrl->GetItemByRow(childId - 1);
		if (!item.IsOk())
		{
			return wxACC_NOT_IMPLEMENTED;
		}

		// Description is concatenation of the contents of items in all columns:
		// Column1: Value1, Column2: Value2, ...
		// First textual item should be skipped because it is returned
		// as a Name property.
		wxString itemDesc;

		bool firstTextSkipped = false;
		wxDataViewExtModel* model = dvCtrl->GetModel();
		const unsigned int numCols = dvCtrl->GetColumnCount();
		for (unsigned int col = 0; col < numCols; col++)
		{
			if (!model->HasValue(item, col))
				continue; // skip it

			wxDataViewExtColumn* dvCol = dvCtrl->GetColumnAt(col);
			if (dvCol->IsHidden())
				continue; // skip it

			wxVariant value;
			model->GetValue(value, item, dvCol->GetModelColumn());

			wxDataViewExtRenderer* r = dvCol->GetRenderer();
			if (!r->PrepareForItem(model, item, dvCol->GetModelColumn()))
				continue;

			wxString valStr = r->GetAccessibleDescription();
			// Skip first textual item
			if (!firstTextSkipped && !value.IsNull() && !value.IsType(wxS("bool")) && !valStr.empty())
			{
				firstTextSkipped = true;
				continue;
			}

			if (!valStr.empty())
			{
				wxString colName = dvCol->GetTitle();
				// If column has no label then present its index.
				if (colName.empty())
				{
					// Columns are numbered from 1.
					colName = wxString::Format(_("Column %u"), col + 1);
				}

				if (!itemDesc.empty())
					itemDesc.Append(wxS(", "));
				itemDesc.Append(colName);
				itemDesc.Append(wxS(": "));
				itemDesc.Append(valStr);
			}
		}

		*description = itemDesc;
	}

	return wxACC_OK;
}

// Returns help text for this object or a child, similar to tooltip text.
wxAccStatus wxDataViewExtCtrlAccessible::GetHelpText(int childId, wxString* helpText)
{
	wxDataViewExtCtrl* dvCtrl = wxDynamicCast(GetWindow(), wxDataViewExtCtrl);
	wxCHECK(dvCtrl, wxACC_FAIL);

#if wxUSE_HELP
	if (childId == wxACC_SELF)
	{
		*helpText = dvCtrl->GetHelpText();
	}
	else
	{
		wxDataViewExtItem item = dvCtrl->GetItemByRow(childId - 1);
		if (item.IsOk())
		{
			wxDataViewExtMainWindow* dvWnd = wxDynamicCast(dvCtrl->GetMainWindow(), wxDataViewExtMainWindow);
			wxRect rect = dvWnd->GetItemRect(item, NULL);
			*helpText = dvWnd->GetHelpTextAtPoint(rect.GetPosition(), wxHelpEvent::Origin_Keyboard);
		}
		else
		{
			helpText->clear();
		}
	}
	return wxACC_OK;
#else
	(void)childId;
	(void)helpText;
	return wxACC_NOT_IMPLEMENTED;
#endif
}

// Returns the keyboard shortcut for this object or child.
// Return e.g. ALT+K
wxAccStatus wxDataViewExtCtrlAccessible::GetKeyboardShortcut(int childId, wxString* shortcut)
{
	wxDataViewExtCtrl* dvCtrl = wxDynamicCast(GetWindow(), wxDataViewExtCtrl);
	wxCHECK(dvCtrl, wxACC_FAIL);

	if (childId != wxACC_SELF)
	{
		wxDataViewExtMainWindow* dvWnd = wxDynamicCast(dvCtrl->GetMainWindow(), wxDataViewExtMainWindow);
		if (!dvWnd->IsList())
		{
			wxDataViewExtTreeNode* node = dvWnd->GetTreeNodeByRow(childId - 1);
			if (node)
			{
				if (node->HasChildren())
				{
					if (node->IsOpen())
						/* TRANSLATORS: Keystroke for manipulating a tree control */
						*shortcut = _("Left");
					else
						/* TRANSLATORS: Keystroke for manipulating a tree control */
						*shortcut = _("Right");

					return wxACC_OK;
				}
			}
		}
	}

	return wxACC_FALSE;
}

// Returns a role constant.
wxAccStatus wxDataViewExtCtrlAccessible::GetRole(int childId, wxAccRole* role)
{
	wxDataViewExtCtrl* dvCtrl = wxDynamicCast(GetWindow(), wxDataViewExtCtrl);
	wxCHECK(dvCtrl, wxACC_FAIL);
	wxDataViewExtMainWindow* dvWnd = wxDynamicCast(dvCtrl->GetMainWindow(), wxDataViewExtMainWindow);

	if (childId == wxACC_SELF)
		*role = dvWnd->IsList() ? wxROLE_SYSTEM_LIST : wxROLE_SYSTEM_OUTLINE;
	else
		*role = dvWnd->IsList() ? wxROLE_SYSTEM_LISTITEM : wxROLE_SYSTEM_OUTLINEITEM;

	return wxACC_OK;
}

// Returns a state constant.
wxAccStatus wxDataViewExtCtrlAccessible::GetState(int childId, long* state)
{
	wxDataViewExtCtrl* dvCtrl = wxDynamicCast(GetWindow(), wxDataViewExtCtrl);
	wxCHECK(dvCtrl, wxACC_FAIL);
	wxDataViewExtMainWindow* dvWnd = wxDynamicCast(dvCtrl->GetMainWindow(), wxDataViewExtMainWindow);

	long st = 0;
	// State flags common to the object and its children.
	if (!dvWnd->IsEnabled())
		st |= wxACC_STATE_SYSTEM_UNAVAILABLE;
	if (!dvWnd->IsShown())
		st |= wxACC_STATE_SYSTEM_INVISIBLE;

	if (childId == wxACC_SELF)
	{
		if (dvWnd->IsFocusable())
			st |= wxACC_STATE_SYSTEM_FOCUSABLE;
		if (dvWnd->HasFocus())
			st |= wxACC_STATE_SYSTEM_FOCUSED;
	}
	else
	{
		const unsigned int rowNum = childId - 1;

		if (dvWnd->IsFocusable())
			st |= wxACC_STATE_SYSTEM_FOCUSABLE | wxACC_STATE_SYSTEM_SELECTABLE;
		if (!dvWnd->IsSingleSel())
			st |= wxACC_STATE_SYSTEM_MULTISELECTABLE | wxACC_STATE_SYSTEM_EXTSELECTABLE;

		if (rowNum < dvWnd->GetFirstVisibleRow() || rowNum > dvWnd->GetLastFullyVisibleRow())
			st |= wxACC_STATE_SYSTEM_OFFSCREEN;
		if (dvWnd->GetCurrentRow() == rowNum)
			st |= wxACC_STATE_SYSTEM_FOCUSED;
		if (dvWnd->IsRowSelected(rowNum))
			st |= wxACC_STATE_SYSTEM_SELECTED;

		if (!dvWnd->IsList())
		{
			wxDataViewExtTreeNode* node = dvWnd->GetTreeNodeByRow(rowNum);
			if (node)
			{
				if (node->HasChildren())
				{
					if (node->IsOpen())
						st |= wxACC_STATE_SYSTEM_EXPANDED;
					else
						st |= wxACC_STATE_SYSTEM_COLLAPSED;
				}
			}
		}
	}
	*state = st;
	return wxACC_OK;
}

// Returns a localized string representing the value for the object
// or child.
wxAccStatus wxDataViewExtCtrlAccessible::GetValue(int childId, wxString* strValue)
{
	wxDataViewExtCtrl* dvCtrl = wxDynamicCast(GetWindow(), wxDataViewExtCtrl);
	wxCHECK(dvCtrl, wxACC_FAIL);

	wxString val;

	if (childId != wxACC_SELF)
	{
		wxDataViewExtMainWindow* dvWnd = wxDynamicCast(dvCtrl->GetMainWindow(), wxDataViewExtMainWindow);
		if (!dvWnd->IsList())
		{
			// In the tree view each item within the control has a zero-based value
			// that represents its level within the hierarchy and this value
			// is returned as a Value property.
			wxDataViewExtTreeNode* node = dvWnd->GetTreeNodeByRow(childId - 1);
			if (node)
			{
				val = wxString::Format(wxS("%i"), node->GetIndentLevel());
			}
		}
	}

	*strValue = val;
	return wxACC_OK;
}

// Selects the object or child.
wxAccStatus wxDataViewExtCtrlAccessible::Select(int childId, wxAccSelectionFlags selectFlags)
{
	wxDataViewExtCtrl* dvCtrl = wxDynamicCast(GetWindow(), wxDataViewExtCtrl);
	wxCHECK(dvCtrl, wxACC_FAIL);
	wxDataViewExtMainWindow* dvWnd = wxDynamicCast(dvCtrl->GetMainWindow(), wxDataViewExtMainWindow);

	if (childId == wxACC_SELF)
	{
		if (selectFlags == wxACC_SEL_TAKEFOCUS)
		{
			dvWnd->SetFocus();
		}
		else if (selectFlags != wxACC_SEL_NONE)
		{
			wxFAIL_MSG(wxS("Invalid selection flag"));
			return wxACC_INVALID_ARG;
		}
	}
	else
	{
		// These flags are not allowed in the single-selection mode:
		if (dvWnd->IsSingleSel() &&
			selectFlags & (wxACC_SEL_EXTENDSELECTION | wxACC_SEL_ADDSELECTION | wxACC_SEL_REMOVESELECTION))
		{
			wxFAIL_MSG(wxS("Invalid selection flag"));
			return wxACC_INVALID_ARG;
		}

		const int row = childId - 1;

		if (selectFlags == wxACC_SEL_TAKEFOCUS)
		{
			dvWnd->ChangeCurrentRow(row);
		}
		else if (selectFlags & wxACC_SEL_TAKESELECTION)
		{
			// This flag must not be combined with the following flags:
			if (selectFlags & (wxACC_SEL_EXTENDSELECTION | wxACC_SEL_ADDSELECTION | wxACC_SEL_REMOVESELECTION))
			{
				wxFAIL_MSG(wxS("Invalid selection flag"));
				return wxACC_INVALID_ARG;
			}

			dvWnd->UnselectAllRows();
			dvWnd->SelectRow(row, true);
			if (selectFlags & wxACC_SEL_TAKEFOCUS || dvWnd->IsSingleSel())
			{
				dvWnd->ChangeCurrentRow(row);
			}
		}
		else if (selectFlags & wxACC_SEL_EXTENDSELECTION)
		{
			// This flag must not be combined with the following flag:
			if (selectFlags & wxACC_SEL_TAKESELECTION)
			{
				wxFAIL_MSG(wxS("Invalid selection flag"));
				return wxACC_INVALID_ARG;
			}
			// These flags cannot be set together:
			if ((selectFlags & (wxACC_SEL_ADDSELECTION | wxACC_SEL_REMOVESELECTION))
				== (wxACC_SEL_ADDSELECTION | wxACC_SEL_REMOVESELECTION))
			{
				wxFAIL_MSG(wxS("Invalid selection flag"));
				return wxACC_INVALID_ARG;
			}

			// We have to have a focused object as a selection anchor.
			unsigned int focusedRow = dvWnd->GetCurrentRow();
			if (focusedRow == (unsigned int)-1)
			{
				wxFAIL_MSG(wxS("No selection anchor"));
				return wxACC_INVALID_ARG;
			}

			bool doSelect;
			if (selectFlags & wxACC_SEL_ADDSELECTION)
				doSelect = true;
			else if (selectFlags & wxACC_SEL_REMOVESELECTION)
				doSelect = false;
			else
				// If the anchor object is selected, the selection is extended.
				// If the anchor object is not selected, all objects are unselected.
				doSelect = dvWnd->IsRowSelected(focusedRow);

			if (doSelect)
			{
				dvWnd->SelectRows(focusedRow, row);
			}
			else
			{
				for (int r = focusedRow; r <= row; r++)
					dvWnd->SelectRow(r, false);
			}

			if (selectFlags & wxACC_SEL_TAKEFOCUS)
			{
				dvWnd->ChangeCurrentRow(row);
			}
		}
		else if (selectFlags & wxACC_SEL_ADDSELECTION)
		{
			// This flag must not be combined with the following flags:
			if (selectFlags & (wxACC_SEL_TAKESELECTION | wxACC_SEL_REMOVESELECTION))
			{
				wxFAIL_MSG(wxS("Invalid selection flag"));
				return wxACC_INVALID_ARG;
			}

			// Combination with wxACC_SEL_EXTENDSELECTION is already handled
			// (see wxACC_SEL_EXTENDSELECTION block).
			dvWnd->SelectRow(row, true);
			if (selectFlags & wxACC_SEL_TAKEFOCUS)
			{
				dvWnd->ChangeCurrentRow(row);
			}
		}
		else if (selectFlags & wxACC_SEL_REMOVESELECTION)
		{
			// This flag must not be combined with the following flags:
			if (selectFlags & (wxACC_SEL_TAKESELECTION | wxACC_SEL_ADDSELECTION))
			{
				wxFAIL_MSG(wxS("Invalid selection flag"));
				return wxACC_INVALID_ARG;
			}

			// Combination with wxACC_SEL_EXTENDSELECTION is already handled
			// (see wxACC_SEL_EXTENDSELECTION block).
			dvWnd->SelectRow(row, false);
			if (selectFlags & wxACC_SEL_TAKEFOCUS)
			{
				dvWnd->ChangeCurrentRow(row);
			}
		}
	}

	return wxACC_OK;
}

// Gets the window with the keyboard focus.
// If childId is 0 and child is NULL, no object in
// this subhierarchy has the focus.
// If this object has the focus, child should be 'this'.
wxAccStatus wxDataViewExtCtrlAccessible::GetFocus(int* childId, wxAccessible** child)
{
	wxDataViewExtCtrl* dvCtrl = wxDynamicCast(GetWindow(), wxDataViewExtCtrl);
	wxCHECK(dvCtrl, wxACC_FAIL);
	wxDataViewExtMainWindow* dvWnd = wxDynamicCast(dvCtrl->GetMainWindow(), wxDataViewExtMainWindow);

	const unsigned int row = dvWnd->GetCurrentRow();
	if (row != (unsigned int)*childId - 1)
	{
		*childId = row + 1;
		*child = NULL;
	}
	else
	{
		// First check if header is focused because header control
		// handles accesibility requestes on its own.
		wxHeaderGenericCtrl* dvHdr = dvCtrl->GenericGetHeader();
		if (dvHdr)
		{
			if (dvHdr->HasFocus())
			{
				*childId = wxACC_SELF;
				*child = dvHdr->GetOrCreateAccessible();
				return wxACC_OK;
			}
		}

		if (dvWnd->HasFocus())
		{
			*childId = wxACC_SELF;
			*child = this;
		}
		else
		{
			*childId = 0;
			*child = NULL;
		}
	}

	return wxACC_OK;
}

// Gets a variant representing the selected children
// of this object.
// Acceptable values:
// - a null variant (IsNull() returns true)
// - a "void*" pointer to a wxAccessible child object
// - an integer representing the selected child element,
//   or 0 if this object is selected (GetType() == wxT("long"))
// - a list variant (GetType() == wxT("list"))
wxAccStatus wxDataViewExtCtrlAccessible::GetSelections(wxVariant* selections)
{
	wxDataViewExtCtrl* dvCtrl = wxDynamicCast(GetWindow(), wxDataViewExtCtrl);
	wxCHECK(dvCtrl, wxACC_FAIL);

	wxDataViewExtItemArray sel;
	dvCtrl->GetSelections(sel);
	if (sel.IsEmpty())
	{
		selections->MakeNull();
	}
	else
	{
		wxVariantList tempList;
		wxVariant v(tempList);

		for (size_t i = 0; i < sel.GetCount(); i++)
		{
			int row = dvCtrl->GetRowByItem(sel[i]);
			v.Append(wxVariant((long)row + 1));
		}

		// Don't return the list if one child is selected.
		if (v.GetCount() == 1)
			*selections = wxVariant(v[0].GetLong());
		else
			*selections = v;
	}

	return wxACC_OK;
}
#endif // wxUSE_ACCESSIBILITY

#endif // !wxUSE_DATAVIEWCTRL