/////////////////////////////////////////////////////////////////////////////
// Name:        wx/dataview.h
// Purpose:     wxDataViewExtCtrl base classes
// Author:      Robert Roebling
// Modified by: Bo Yang
// Created:     08.01.06
// Copyright:   (c) Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DATAVIEW_H_BASE_
#define _WX_DATAVIEW_H_BASE_

#include <wx/defs.h>

#if wxUSE_DATAVIEWCTRL

#include <wx/textctrl.h>
#include <wx/headercol.h>
#include <wx/variant.h>
#include <wx/dnd.h>            // For wxDragResult declaration only.
#include <wx/dynarray.h>
#include <wx/icon.h>
#include <wx/itemid.h>
#include <wx/weakref.h>
#include <wx/vector.h>
#include <wx/dataobj.h>
#include <wx/withimages.h>
#include <wx/systhemectrl.h>
#include <wx/vector.h>
#include <wx/headerctrl.h>

class WXDLLIMPEXP_FWD_CORE wxImageList;
class wxItemAttr;
class WXDLLIMPEXP_FWD_CORE wxHeaderCtrl;

#include "backend/tableView.h"

// ----------------------------------------------------------------------------
// wxDataViewExtCtrl globals
// ----------------------------------------------------------------------------

class BACKEND_API wxDataViewExtModel;
class wxDataViewExtCtrl;
class wxDataViewExtColumn;
class wxDataViewExtRenderer;
class wxDataViewExtModelNotifier;
#if wxUSE_ACCESSIBILITY
class wxDataViewExtCtrlAccessible;
#endif // wxUSE_ACCESSIBILITY

extern const char wxDataViewExtCtrlNameStr[];

// ----------------------------------------------------------------------------
// wxDataViewExtListModel: a model of a list, i.e. flat data structure without any
//      branches/containers, used as base class by wxDataViewExtIndexListModel and
//      wxDataViewExtVirtualListModel
// ----------------------------------------------------------------------------

class wxDataViewExtListModel : public wxDataViewExtModel
{
public:
	// derived classes should override these methods instead of
	// {Get,Set}Value() and GetAttr() inherited from the base class

	virtual void GetValueByRow(wxVariant& variant,
		unsigned row, unsigned col) const = 0;

	virtual bool SetValueByRow(const wxVariant& variant,
		unsigned row, unsigned col) = 0;

	virtual bool
		GetAttrByRow(unsigned WXUNUSED(row), unsigned WXUNUSED(col),
			wxDataViewExtItemAttr& WXUNUSED(attr)) const
	{
		return false;
	}

	virtual bool IsEnabledByRow(unsigned int WXUNUSED(row),
		unsigned int WXUNUSED(col)) const
	{
		return true;
	}


	// helper methods provided by list models only
	virtual unsigned GetRow(const wxDataViewExtItem& item) const = 0;

	// returns the number of rows
	virtual unsigned int GetCount() const = 0;

	// implement some base class pure virtual directly
	virtual wxDataViewExtItem
		GetParent(const wxDataViewExtItem& WXUNUSED(item)) const wxOVERRIDE
	{
		// items never have valid parent in this model
		return wxDataViewExtItem();
	}

	virtual bool IsContainer(const wxDataViewExtItem& item) const wxOVERRIDE
	{
		// only the invisible (and invalid) root item has children
		return !item.IsOk();
	}

	// and implement some others by forwarding them to our own ones
	virtual void GetValue(wxVariant& variant,
		const wxDataViewExtItem& item, unsigned int col) const wxOVERRIDE
	{
		GetValueByRow(variant, GetRow(item), col);
	}

	virtual bool SetValue(const wxVariant& variant,
		const wxDataViewExtItem& item, unsigned int col) wxOVERRIDE
	{
		return SetValueByRow(variant, GetRow(item), col);
	}

	virtual bool GetAttr(const wxDataViewExtItem& item, unsigned int col,
		wxDataViewExtItemAttr& attr) const wxOVERRIDE
	{
		return GetAttrByRow(GetRow(item), col, attr);
	}

	virtual bool IsEnabled(const wxDataViewExtItem& item, unsigned int col) const wxOVERRIDE
	{
		return IsEnabledByRow(GetRow(item), col);
	}


	virtual bool IsListModel() const wxOVERRIDE { return true; }
};

// ---------------------------------------------------------
// wxDataViewExtIndexListModel
// ---------------------------------------------------------

class wxDataViewExtIndexListModel : public wxDataViewExtListModel
{
public:
	wxDataViewExtIndexListModel(unsigned int initial_size = 0);

	void RowPrepended();
	void RowInserted(unsigned int before);
	void RowAppended();
	void RowDeleted(unsigned int row);
	void RowsDeleted(const wxArrayInt& rows);
	void RowChanged(unsigned int row);
	void RowValueChanged(unsigned int row, unsigned int col);
	void Reset(unsigned int new_size);

	// convert to/from row/wxDataViewExtItem

	virtual unsigned GetRow(const wxDataViewExtItem& item) const wxOVERRIDE;
	wxDataViewExtItem GetItem(unsigned int row) const;

	// implement base methods
	virtual unsigned int GetChildren(const wxDataViewExtItem& item, wxDataViewExtItemArray& children) const wxOVERRIDE;

	unsigned int GetCount() const wxOVERRIDE { return (unsigned int)m_hash.GetCount(); }

private:
	wxDataViewExtItemArray m_hash;
	unsigned int m_nextFreeID;
	bool m_ordered;
};

// ---------------------------------------------------------
// wxDataViewExtVirtualListModel
// ---------------------------------------------------------

#ifdef __WXMAC__
// better than nothing
typedef wxDataViewExtIndexListModel wxDataViewExtVirtualListModel;
#else

class wxDataViewExtVirtualListModel : public wxDataViewExtListModel
{
public:
	wxDataViewExtVirtualListModel(unsigned int initial_size = 0);

	void RowPrepended();
	void RowInserted(unsigned int before);
	void RowAppended();
	void RowDeleted(unsigned int row);
	void RowsDeleted(const wxArrayInt& rows);
	void RowChanged(unsigned int row);
	void RowValueChanged(unsigned int row, unsigned int col);
	void Reset(unsigned int new_size);

	// convert to/from row/wxDataViewExtItem

	virtual unsigned GetRow(const wxDataViewExtItem& item) const wxOVERRIDE;
	wxDataViewExtItem GetItem(unsigned int row) const;

	// compare based on index

	virtual int Compare(const wxDataViewExtItem& item1, const wxDataViewExtItem& item2,
		unsigned int column, bool ascending) const wxOVERRIDE;
	virtual bool HasDefaultCompare() const wxOVERRIDE;

	// implement base methods
	virtual unsigned int GetChildren(const wxDataViewExtItem& item, wxDataViewExtItemArray& children) const wxOVERRIDE;

	unsigned int GetCount() const wxOVERRIDE { return m_size; }

	// internal
	virtual bool IsVirtualListModel() const wxOVERRIDE { return true; }

private:
	unsigned int m_size;
};
#endif

// ----------------------------------------------------------------------------
// wxDataViewExtRenderer and related classes
// ----------------------------------------------------------------------------

#include "dvrenderers.h"

// ---------------------------------------------------------
// wxDataViewExtColumnBase
// ---------------------------------------------------------

// for compatibility only, do not use
enum wxDataViewExtColumnFlags
{
	wxDATAVIEW_COL_RESIZABLE = wxCOL_RESIZABLE,
	wxDATAVIEW_COL_SORTABLE = wxCOL_SORTABLE,
	wxDATAVIEW_COL_REORDERABLE = wxCOL_REORDERABLE,
	wxDATAVIEW_COL_HIDDEN = wxCOL_HIDDEN
};

class wxDataViewExtColumnBase : public wxSettableHeaderColumn
{
public:
	// ctor for the text columns: takes ownership of renderer
	wxDataViewExtColumnBase(wxDataViewExtRenderer* renderer,
		unsigned int model_column)
	{
		Init(renderer, model_column);
	}

	// ctor for the bitmap columns
	wxDataViewExtColumnBase(const wxBitmapBundle& bitmap,
		wxDataViewExtRenderer* renderer,
		unsigned int model_column)
		: m_bitmap(bitmap)
	{
		Init(renderer, model_column);
	}

	virtual ~wxDataViewExtColumnBase();

	// setters:
	virtual void SetOwner(wxDataViewExtCtrl* owner)
	{
		m_owner = owner;
	}

	// getters:
	unsigned int GetModelColumn() const { return static_cast<unsigned int>(m_model_column); }
	wxDataViewExtCtrl* GetOwner() const { return m_owner; }
	wxDataViewExtRenderer* GetRenderer() const { return m_renderer; }

	// implement some of base class pure virtuals (the rest is port-dependent
	// and done differently in generic and native versions)
	virtual void SetBitmap(const wxBitmapBundle& bitmap) wxOVERRIDE { m_bitmap = bitmap; }
	virtual wxBitmap GetBitmap() const wxOVERRIDE { return m_bitmap.GetBitmap(wxDefaultSize); }
	virtual wxBitmapBundle GetBitmapBundle() const wxOVERRIDE { return m_bitmap; }

	// Special accessor for use by wxWidgets only returning the width that was
	// explicitly set, either by the application, using SetWidth(), or by the
	// user, resizing the column interactively. It is usually the same as
	// GetWidth(), but can be different for the last column.
	virtual int WXGetSpecifiedWidth() const { return GetWidth(); }

protected:
	wxDataViewExtRenderer* m_renderer;
	int                      m_model_column;
	wxBitmapBundle           m_bitmap;
	wxDataViewExtCtrl* m_owner;

private:
	// common part of all ctors
	void Init(wxDataViewExtRenderer* renderer, unsigned int model_column);
};

// ---------------------------------------------------------
// wxDataViewExtCtrlBase
// ---------------------------------------------------------

#define wxDV_SINGLE                  0x0000     // for convenience
#define wxDV_MULTIPLE                0x0001     // can select multiple items

#define wxDV_NO_HEADER               0x0002     // column titles not visible
#define wxDV_HORIZ_RULES             0x0004     // light horizontal rules between rows
#define wxDV_VERT_RULES              0x0008     // light vertical rules between columns

#define wxDV_ROW_LINES               0x0010     // alternating colour in rows
#define wxDV_VARIABLE_LINE_HEIGHT    0x0020     // variable line height

class wxDataViewExtCtrlBase : public wxSystemThemedControl<wxControl>
{
public:
	wxDataViewExtCtrlBase();
	virtual ~wxDataViewExtCtrlBase();

	// model
	// -----

	virtual bool AssociateModel(wxDataViewExtModel* model);
	wxDataViewExtModel* GetModel();
	const wxDataViewExtModel* GetModel() const;


	// column management
	// -----------------

	wxDataViewExtColumn* PrependTextColumn(const wxString& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxCOL_WIDTH_DEFAULT,
		wxAlignment align = wxALIGN_NOT,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* PrependIconTextColumn(const wxString& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxCOL_WIDTH_DEFAULT,
		wxAlignment align = wxALIGN_NOT,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* PrependToggleColumn(const wxString& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxDVC_TOGGLE_DEFAULT_WIDTH,
		wxAlignment align = wxALIGN_CENTER,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* PrependProgressColumn(const wxString& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxDVC_DEFAULT_WIDTH,
		wxAlignment align = wxALIGN_CENTER,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* PrependDateColumn(const wxString& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_ACTIVATABLE, int width = wxCOL_WIDTH_DEFAULT,
		wxAlignment align = wxALIGN_NOT,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* PrependBitmapColumn(const wxString& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxCOL_WIDTH_DEFAULT,
		wxAlignment align = wxALIGN_CENTER,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* PrependTextColumn(const wxBitmap& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxCOL_WIDTH_DEFAULT,
		wxAlignment align = wxALIGN_NOT,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* PrependIconTextColumn(const wxBitmap& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxCOL_WIDTH_DEFAULT,
		wxAlignment align = wxALIGN_NOT,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* PrependToggleColumn(const wxBitmap& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxDVC_TOGGLE_DEFAULT_WIDTH,
		wxAlignment align = wxALIGN_CENTER,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* PrependProgressColumn(const wxBitmap& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxDVC_DEFAULT_WIDTH,
		wxAlignment align = wxALIGN_CENTER,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* PrependDateColumn(const wxBitmap& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_ACTIVATABLE, int width = wxCOL_WIDTH_DEFAULT,
		wxAlignment align = wxALIGN_NOT,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* PrependBitmapColumn(const wxBitmap& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxCOL_WIDTH_DEFAULT,
		wxAlignment align = wxALIGN_CENTER,
		int flags = wxDATAVIEW_COL_RESIZABLE);

	wxDataViewExtColumn* AppendTextColumn(const wxString& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxCOL_WIDTH_DEFAULT,
		wxAlignment align = wxALIGN_NOT,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* AppendIconTextColumn(const wxString& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxCOL_WIDTH_DEFAULT,
		wxAlignment align = wxALIGN_NOT,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* AppendToggleColumn(const wxString& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxDVC_TOGGLE_DEFAULT_WIDTH,
		wxAlignment align = wxALIGN_CENTER,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* AppendProgressColumn(const wxString& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxDVC_DEFAULT_WIDTH,
		wxAlignment align = wxALIGN_CENTER,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* AppendDateColumn(const wxString& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_ACTIVATABLE, int width = wxCOL_WIDTH_DEFAULT,
		wxAlignment align = wxALIGN_NOT,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* AppendBitmapColumn(const wxString& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxCOL_WIDTH_DEFAULT,
		wxAlignment align = wxALIGN_CENTER,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* AppendTextColumn(const wxBitmap& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxCOL_WIDTH_DEFAULT,
		wxAlignment align = wxALIGN_NOT,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* AppendIconTextColumn(const wxBitmap& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxCOL_WIDTH_DEFAULT,
		wxAlignment align = wxALIGN_NOT,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* AppendToggleColumn(const wxBitmap& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxDVC_TOGGLE_DEFAULT_WIDTH,
		wxAlignment align = wxALIGN_CENTER,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* AppendProgressColumn(const wxBitmap& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxDVC_DEFAULT_WIDTH,
		wxAlignment align = wxALIGN_CENTER,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* AppendDateColumn(const wxBitmap& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_ACTIVATABLE, int width = wxCOL_WIDTH_DEFAULT,
		wxAlignment align = wxALIGN_NOT,
		int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* AppendBitmapColumn(const wxBitmap& label, unsigned int model_column,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxCOL_WIDTH_DEFAULT,
		wxAlignment align = wxALIGN_CENTER,
		int flags = wxDATAVIEW_COL_RESIZABLE);

	virtual bool PrependColumn(wxDataViewExtColumn* col);
	virtual bool InsertColumn(unsigned int pos, wxDataViewExtColumn* col);
	virtual bool AppendColumn(wxDataViewExtColumn* col);

	virtual unsigned int GetColumnCount() const = 0;
	virtual wxDataViewExtColumn* GetColumn(unsigned int pos) const = 0;
	virtual int GetColumnPosition(const wxDataViewExtColumn* column) const = 0;

	virtual bool DeleteColumn(wxDataViewExtColumn* column) = 0;
	virtual bool ClearColumns() = 0;

	void SetExpanderColumn(wxDataViewExtColumn* col)
	{
		m_expander_column = col; DoSetExpanderColumn();
	}
	wxDataViewExtColumn* GetExpanderColumn() const
	{
		return m_expander_column;
	}

	virtual wxDataViewExtColumn* GetSortingColumn() const = 0;
	virtual wxVector<wxDataViewExtColumn*> GetSortingColumns() const
	{
		wxVector<wxDataViewExtColumn*> columns;
		if (wxDataViewExtColumn* col = GetSortingColumn())
			columns.push_back(col);
		return columns;
	}

	// This must be overridden to return true if the control does allow sorting
	// by more than one column, which is not the case by default.
	virtual bool AllowMultiColumnSort(bool allow)
	{
		// We can still return true when disabling multi-column sort.
		return !allow;
	}

	// Return true if multi column sort is currently allowed.
	virtual bool IsMultiColumnSortAllowed() const { return false; }

	// This should also be overridden to actually use the specified column for
	// sorting if using multiple columns is supported.
	virtual void ToggleSortByColumn(int WXUNUSED(column)) {}


	// items management
	// ----------------

	void SetIndent(int indent)
	{
		m_indent = indent; DoSetIndent();
	}
	int GetIndent() const
	{
		return m_indent;
	}

	// Current item is the one used by the keyboard navigation, it is the same
	// as the (unique) selected item in single selection mode so these
	// functions are mostly useful for controls with wxDV_MULTIPLE style.
	wxDataViewExtItem GetCurrentItem() const;
	void SetCurrentItem(const wxDataViewExtItem& item);

	virtual wxDataViewExtItem GetTopItem() const { return wxDataViewExtItem(NULL); }
	virtual int GetCountPerPage() const { return wxNOT_FOUND; }

	// Currently focused column of the current item or NULL if no column has focus
	virtual wxDataViewExtColumn* GetCurrentColumn() const = 0;

	// Selection: both GetSelection() and GetSelections() can be used for the
	// controls both with and without wxDV_MULTIPLE style. For single selection
	// controls GetSelections() is not very useful however. And for multi
	// selection controls GetSelection() returns an invalid item if more than
	// one item is selected. Use GetSelectedItemsCount() or HasSelection() to
	// check if any items are selected at all.
	virtual int GetSelectedItemsCount() const = 0;
	bool HasSelection() const { return GetSelectedItemsCount() != 0; }
	wxDataViewExtItem GetSelection() const;
	virtual int GetSelections(wxDataViewExtItemArray& sel) const = 0;
	virtual void SetSelections(const wxDataViewExtItemArray& sel) = 0;
	virtual void Select(const wxDataViewExtItem& item) = 0;
	virtual void Unselect(const wxDataViewExtItem& item) = 0;
	virtual bool IsSelected(const wxDataViewExtItem& item) const = 0;

	virtual void SelectAll() = 0;
	virtual void UnselectAll() = 0;

	void Expand(const wxDataViewExtItem& item);
	void ExpandChildren(const wxDataViewExtItem& item);
	void ExpandAncestors(const wxDataViewExtItem& item);
	virtual void Collapse(const wxDataViewExtItem& item) = 0;
	virtual bool IsExpanded(const wxDataViewExtItem& item) const = 0;

	virtual void EnsureVisible(const wxDataViewExtItem& item,
		const wxDataViewExtColumn* column = NULL) = 0;
	virtual void HitTest(const wxPoint& point, wxDataViewExtItem& item, wxDataViewExtColumn*& column) const = 0;
	virtual wxRect GetItemRect(const wxDataViewExtItem& item, const wxDataViewExtColumn* column = NULL) const = 0;

	virtual bool SetRowHeight(int WXUNUSED(rowHeight)) { return false; }

	virtual void EditItem(const wxDataViewExtItem& item, const wxDataViewExtColumn* column) = 0;

	// Use EditItem() instead
	wxDEPRECATED(void StartEditor(const wxDataViewExtItem& item, unsigned int column));

#if wxUSE_DRAG_AND_DROP
	virtual bool EnableDragSource(const wxDataFormat& WXUNUSED(format))
	{
		return false;
	}

	bool EnableDropTargets(const wxVector<wxDataFormat>& formats)
	{
		return DoEnableDropTarget(formats);
	}

	bool EnableDropTarget(const wxDataFormat& format)
	{
		wxVector<wxDataFormat> formats;
		if (format.GetType() != wxDF_INVALID)
		{
			formats.push_back(format);
		}

		return DoEnableDropTarget(formats);
	}

#endif // wxUSE_DRAG_AND_DROP

	// define control visual attributes
	// --------------------------------

	// Header attributes: only implemented in the generic version currently.
	virtual bool SetHeaderAttr(const wxItemAttr& WXUNUSED(attr))
	{
		return false;
	}

	// Set the colour used for the "alternate" rows when wxDV_ROW_LINES is on.
	// Also only supported in the generic version, which returns true to
	// indicate it.
	virtual bool SetAlternateRowColour(const wxColour& WXUNUSED(colour))
	{
		return false;
	}

	virtual wxVisualAttributes GetDefaultAttributes() const wxOVERRIDE
	{
		return GetClassDefaultAttributes(GetWindowVariant());
	}

	static wxVisualAttributes
		GetClassDefaultAttributes(wxWindowVariant variant = wxWINDOW_VARIANT_NORMAL)
	{
		return wxControl::GetCompositeControlsDefaultAttributes(variant);
	}

protected:
	virtual void DoSetExpanderColumn() = 0;
	virtual void DoSetIndent() = 0;

#if wxUSE_DRAG_AND_DROP
	// Helper function which can be used by DoEnableDropTarget() implementations
	// in the derived classes: return a composite data object supporting the
	// given formats or null if the vector is empty.
	static wxDataObjectComposite*
		CreateDataObject(const wxVector<wxDataFormat>& formats);

	virtual bool DoEnableDropTarget(const wxVector<wxDataFormat>& WXUNUSED(formats))
	{
		return false;
	}
#endif // wxUSE_DRAG_AND_DROP

	// Just expand this item assuming it is already shown, i.e. its parent has
	// been already expanded using ExpandAncestors().
	//
	// If expandChildren is true, also expand all its children recursively.
	virtual void DoExpand(const wxDataViewExtItem& item, bool expandChildren) = 0;

private:
	// Implementation of the public Set/GetCurrentItem() methods which are only
	// called in multi selection case (for single selection controls their
	// implementation is trivial and is done in the base class itself).
	virtual wxDataViewExtItem DoGetCurrentItem() const = 0;
	virtual void DoSetCurrentItem(const wxDataViewExtItem& item) = 0;

	wxDataViewExtModel* m_model;
	wxDataViewExtColumn* m_expander_column;
	int m_indent;

protected:
	wxDECLARE_DYNAMIC_CLASS_NO_COPY(wxDataViewExtCtrlBase);
};

// ----------------------------------------------------------------------------
// wxDataViewExtEvent - the event class for the wxDataViewExtCtrl notifications
// ----------------------------------------------------------------------------

#include "frontend/frontend.h"

class FRONTEND_API wxDataViewExtEvent : public wxNotifyEvent
{
public:
	// Default ctor, normally shouldn't be used and mostly exists only for
	// backwards compatibility.
	wxDataViewExtEvent()
		: wxNotifyEvent()
	{
		Init(NULL, NULL, wxDataViewExtItem());
	}

	// Constructor for the events affecting columns (and possibly also items).
	wxDataViewExtEvent(wxEventType evtType,
		wxDataViewExtCtrlBase* dvc,
		wxDataViewExtColumn* column,
		const wxDataViewExtItem& item = wxDataViewExtItem())
		: wxNotifyEvent(evtType, dvc->GetId())
	{
		Init(dvc, column, item);
	}

	// Constructor for the events affecting only the items.
	wxDataViewExtEvent(wxEventType evtType,
		wxDataViewExtCtrlBase* dvc,
		const wxDataViewExtItem& item)
		: wxNotifyEvent(evtType, dvc->GetId())
	{
		Init(dvc, NULL, item);
	}

	wxDataViewExtEvent(const wxDataViewExtEvent& event)
		: wxNotifyEvent(event),
		m_item(event.m_item),
		m_col(event.m_col),
		m_model(event.m_model),
		m_value(event.m_value),
		m_column(event.m_column),
		m_pos(event.m_pos),
		m_cacheFrom(event.m_cacheFrom),
		m_cacheTo(event.m_cacheTo),
		m_editCancelled(event.m_editCancelled)
#if wxUSE_DRAG_AND_DROP
		, m_dataObject(event.m_dataObject),
		m_dataFormat(event.m_dataFormat),
		m_dataBuffer(event.m_dataBuffer),
		m_dataSize(event.m_dataSize),
		m_dragFlags(event.m_dragFlags),
		m_dropEffect(event.m_dropEffect),
		m_proposedDropIndex(event.m_proposedDropIndex)
#endif
	{
	}

	wxDataViewExtItem GetItem() const { return m_item; }
	int GetColumn() const { return m_col; }
	wxDataViewExtModel* GetModel() const { return m_model; }

	const wxVariant& GetValue() const { return m_value; }
	void SetValue(const wxVariant& value) { m_value = value; }

	// for wxEVT_DATAVIEW_ITEM_EDITING_DONE only
	bool IsEditCancelled() const { return m_editCancelled; }

	// for wxEVT_DATAVIEW_COLUMN_HEADER_CLICKED only
	wxDataViewExtColumn* GetDataViewColumn() const { return m_column; }

	// for wxEVT_DATAVIEW_CONTEXT_MENU only
	wxPoint GetPosition() const { return m_pos; }
	void SetPosition(int x, int y) { m_pos.x = x; m_pos.y = y; }

	// For wxEVT_DATAVIEW_CACHE_HINT
	int GetCacheFrom() const { return m_cacheFrom; }
	int GetCacheTo() const { return m_cacheTo; }
	void SetCache(int from, int to) { m_cacheFrom = from; m_cacheTo = to; }


#if wxUSE_DRAG_AND_DROP
	// For drag operations
	void SetDataObject(wxDataObject* obj) { m_dataObject = obj; }
	wxDataObject* GetDataObject() const { return m_dataObject; }

	// For drop operations
	void SetDataFormat(const wxDataFormat& format) { m_dataFormat = format; }
	wxDataFormat GetDataFormat() const { return m_dataFormat; }
	void SetDataSize(size_t size) { m_dataSize = size; }
	size_t GetDataSize() const { return m_dataSize; }
	void SetDataBuffer(void* buf) { m_dataBuffer = buf; }
	void* GetDataBuffer() const { return m_dataBuffer; }
	void SetDragFlags(int flags) { m_dragFlags = flags; }
	int GetDragFlags() const { return m_dragFlags; }
	void SetDropEffect(wxDragResult effect) { m_dropEffect = effect; }
	wxDragResult GetDropEffect() const { return m_dropEffect; }
	// For platforms (currently generic and OSX) that support Drag/Drop
	// insertion of items, this is the proposed child index for the insertion.
	void SetProposedDropIndex(int index) { m_proposedDropIndex = index; }
	int GetProposedDropIndex() const { return m_proposedDropIndex; }

	// Internal, only used by wxWidgets itself.
	void InitData(wxDataObjectComposite* obj, wxDataFormat format);
#endif // wxUSE_DRAG_AND_DROP

	virtual wxEvent* Clone() const wxOVERRIDE { return new wxDataViewExtEvent(*this); }

	// These methods shouldn't be used outside of wxWidgets and wxWidgets
	// itself doesn't use them any longer either as it constructs the events
	// with the appropriate ctors directly.
#if WXWIN_COMPATIBILITY_3_0
	wxDEPRECATED_MSG("Pass the argument to the ctor instead")
		void SetModel(wxDataViewExtModel* model) { m_model = model; }
	wxDEPRECATED_MSG("Pass the argument to the ctor instead")
		void SetDataViewColumn(wxDataViewExtColumn* col) { m_column = col; }
	wxDEPRECATED_MSG("Pass the argument to the ctor instead")
		void SetItem(const wxDataViewExtItem& item) { m_item = item; }
#endif // WXWIN_COMPATIBILITY_3_0

	void SetColumn(int col) { m_col = col; }
	void SetEditCancelled() { m_editCancelled = true; }

protected:
	wxDataViewExtItem      m_item;
	int                 m_col;
	wxDataViewExtModel* m_model;
	wxVariant           m_value;
	wxDataViewExtColumn* m_column;
	wxPoint             m_pos;
	int                 m_cacheFrom;
	int                 m_cacheTo;
	bool                m_editCancelled;

#if wxUSE_DRAG_AND_DROP
	wxDataObject* m_dataObject;

	wxMemoryBuffer      m_dataBuf;
	wxDataFormat        m_dataFormat;
	void* m_dataBuffer;
	size_t              m_dataSize;

	int                 m_dragFlags;
	wxDragResult        m_dropEffect;
	int                 m_proposedDropIndex;
#endif // wxUSE_DRAG_AND_DROP

private:
	// Common part of non-copy ctors.
	void Init(wxDataViewExtCtrlBase* dvc,
		wxDataViewExtColumn* column,
		const wxDataViewExtItem& item);

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxDataViewExtEvent);
};

wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_SELECTION_CHANGED, wxDataViewExtEvent);

wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_ITEM_ACTIVATED, wxDataViewExtEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_ITEM_COLLAPSED, wxDataViewExtEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_ITEM_EXPANDED, wxDataViewExtEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_ITEM_COLLAPSING, wxDataViewExtEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_ITEM_EXPANDING, wxDataViewExtEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_ITEM_START_EDITING, wxDataViewExtEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_ITEM_EDITING_STARTED, wxDataViewExtEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_ITEM_EDITING_DONE, wxDataViewExtEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewExtEvent);

wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_ITEM_START_INSERTING, wxDataViewExtEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_ITEM_START_DELETING, wxDataViewExtEvent);

wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewExtEvent);

wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_COLUMN_HEADER_CLICK, wxDataViewExtEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK, wxDataViewExtEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_COLUMN_SORTED, wxDataViewExtEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_COLUMN_REORDERED, wxDataViewExtEvent);

wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_CACHE_HINT, wxDataViewExtEvent);

wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_ITEM_BEGIN_DRAG, wxDataViewExtEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_ITEM_DROP_POSSIBLE, wxDataViewExtEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_DATAVIEW_ITEM_DROP, wxDataViewExtEvent);

typedef void (wxEvtHandler::* wxDataViewExtEventFunction)(wxDataViewExtEvent&);

#define wxDataViewExtEventHandler(func) \
    wxEVENT_HANDLER_CAST(wxDataViewExtEventFunction, func)

#define wx__DECLARE_DATAVIEWEVT(evt, id, fn) \
    wx__DECLARE_EVT1(wxEVT_DATAVIEW_ ## evt, id, wxDataViewExtEventHandler(fn))

#define EVT_DATAVIEW_SELECTION_CHANGED(id, fn) wx__DECLARE_DATAVIEWEVT(SELECTION_CHANGED, id, fn)

#define EVT_DATAVIEW_ITEM_ACTIVATED(id, fn) wx__DECLARE_DATAVIEWEVT(ITEM_ACTIVATED, id, fn)
#define EVT_DATAVIEW_ITEM_COLLAPSING(id, fn) wx__DECLARE_DATAVIEWEVT(ITEM_COLLAPSING, id, fn)
#define EVT_DATAVIEW_ITEM_COLLAPSED(id, fn) wx__DECLARE_DATAVIEWEVT(ITEM_COLLAPSED, id, fn)
#define EVT_DATAVIEW_ITEM_EXPANDING(id, fn) wx__DECLARE_DATAVIEWEVT(ITEM_EXPANDING, id, fn)
#define EVT_DATAVIEW_ITEM_EXPANDED(id, fn) wx__DECLARE_DATAVIEWEVT(ITEM_EXPANDED, id, fn)
#define EVT_DATAVIEW_ITEM_START_EDITING(id, fn) wx__DECLARE_DATAVIEWEVT(ITEM_START_EDITING, id, fn)
#define EVT_DATAVIEW_ITEM_EDITING_STARTED(id, fn) wx__DECLARE_DATAVIEWEVT(ITEM_EDITING_STARTED, id, fn)
#define EVT_DATAVIEW_ITEM_EDITING_DONE(id, fn) wx__DECLARE_DATAVIEWEVT(ITEM_EDITING_DONE, id, fn)
#define EVT_DATAVIEW_ITEM_VALUE_CHANGED(id, fn) wx__DECLARE_DATAVIEWEVT(ITEM_VALUE_CHANGED, id, fn)

#define EVT_DATAVIEW_ITEM_CONTEXT_MENU(id, fn) wx__DECLARE_DATAVIEWEVT(ITEM_CONTEXT_MENU, id, fn)

#define EVT_DATAVIEW_COLUMN_HEADER_CLICK(id, fn) wx__DECLARE_DATAVIEWEVT(COLUMN_HEADER_CLICK, id, fn)
#define EVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK(id, fn) wx__DECLARE_DATAVIEWEVT(COLUMN_HEADER_RIGHT_CLICK, id, fn)
#define EVT_DATAVIEW_COLUMN_SORTED(id, fn) wx__DECLARE_DATAVIEWEVT(COLUMN_SORTED, id, fn)
#define EVT_DATAVIEW_COLUMN_REORDERED(id, fn) wx__DECLARE_DATAVIEWEVT(COLUMN_REORDERED, id, fn)
#define EVT_DATAVIEW_CACHE_HINT(id, fn) wx__DECLARE_DATAVIEWEVT(CACHE_HINT, id, fn)

#define EVT_DATAVIEW_ITEM_BEGIN_DRAG(id, fn) wx__DECLARE_DATAVIEWEVT(ITEM_BEGIN_DRAG, id, fn)
#define EVT_DATAVIEW_ITEM_DROP_POSSIBLE(id, fn) wx__DECLARE_DATAVIEWEVT(ITEM_DROP_POSSIBLE, id, fn)
#define EVT_DATAVIEW_ITEM_DROP(id, fn) wx__DECLARE_DATAVIEWEVT(ITEM_DROP, id, fn)

// Old and not documented synonym, don't use.
#define EVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICKED(id, fn) EVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK(id, fn)

#include "datavgen.h"

//-----------------------------------------------------------------------------
// wxDataViewExtListStore
//-----------------------------------------------------------------------------

class wxDataViewExtListStoreLine
{
public:
	wxDataViewExtListStoreLine(wxUIntPtr data = 0)
	{
		m_data = data;
	}

	void SetData(wxUIntPtr data)
	{
		m_data = data;
	}
	wxUIntPtr GetData() const
	{
		return m_data;
	}

	wxVector<wxVariant>  m_values;

private:
	wxUIntPtr m_data;
};


class wxDataViewExtListStore : public wxDataViewExtIndexListModel
{
public:
	wxDataViewExtListStore();
	~wxDataViewExtListStore();

	void PrependColumn(const wxString& varianttype);
	void InsertColumn(unsigned int pos, const wxString& varianttype);
	void AppendColumn(const wxString& varianttype);

	void AppendItem(const wxVector<wxVariant>& values, wxUIntPtr data = 0);
	void PrependItem(const wxVector<wxVariant>& values, wxUIntPtr data = 0);
	void InsertItem(unsigned int row, const wxVector<wxVariant>& values, wxUIntPtr data = 0);
	void DeleteItem(unsigned int pos);
	void DeleteAllItems();
	void ClearColumns();

	unsigned int GetItemCount() const;

	void SetItemData(const wxDataViewExtItem& item, wxUIntPtr data);
	wxUIntPtr GetItemData(const wxDataViewExtItem& item) const;

	// override base virtuals

	virtual void GetValueByRow(wxVariant& value,
		unsigned int row, unsigned int col) const wxOVERRIDE;

	virtual bool SetValueByRow(const wxVariant& value,
		unsigned int row, unsigned int col) wxOVERRIDE;


public:
	wxVector<wxDataViewExtListStoreLine*> m_data;
	wxArrayString                      m_cols;
};

//-----------------------------------------------------------------------------

class wxDataViewExtListCtrl : public wxDataViewExtCtrl
{
public:
	wxDataViewExtListCtrl();
	wxDataViewExtListCtrl(wxWindow* parent, wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize, long style = wxDV_ROW_LINES,
		const wxValidator& validator = wxDefaultValidator);
	~wxDataViewExtListCtrl();

	bool Create(wxWindow* parent, wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize, long style = wxDV_ROW_LINES,
		const wxValidator& validator = wxDefaultValidator);

	wxDataViewExtListStore* GetStore()
	{
		return (wxDataViewExtListStore*)GetModel();
	}
	const wxDataViewExtListStore* GetStore() const
	{
		return (const wxDataViewExtListStore*)GetModel();
	}

	int ItemToRow(const wxDataViewExtItem& item) const
	{
		return item.IsOk() ? (int)GetStore()->GetRow(item) : wxNOT_FOUND;
	}
	wxDataViewExtItem RowToItem(int row) const
	{
		return row == wxNOT_FOUND ? wxDataViewExtItem() : GetStore()->GetItem(row);
	}

	int GetSelectedRow() const
	{
		return ItemToRow(GetSelection());
	}
	void SelectRow(unsigned row)
	{
		Select(RowToItem(row));
	}
	void UnselectRow(unsigned row)
	{
		Unselect(RowToItem(row));
	}
	bool IsRowSelected(unsigned row) const
	{
		return IsSelected(RowToItem(row));
	}

	bool AppendColumn(wxDataViewExtColumn* column, const wxString& varianttype);
	bool PrependColumn(wxDataViewExtColumn* column, const wxString& varianttype);
	bool InsertColumn(unsigned int pos, wxDataViewExtColumn* column, const wxString& varianttype);

	// overridden from base class
	virtual bool PrependColumn(wxDataViewExtColumn* col) wxOVERRIDE;
	virtual bool InsertColumn(unsigned int pos, wxDataViewExtColumn* col) wxOVERRIDE;
	virtual bool AppendColumn(wxDataViewExtColumn* col) wxOVERRIDE;
	virtual bool ClearColumns() wxOVERRIDE;

	wxDataViewExtColumn* AppendTextColumn(const wxString& label,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxCOL_WIDTH_DEFAULT,
		wxAlignment align = wxALIGN_LEFT, int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* AppendToggleColumn(const wxString& label,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_ACTIVATABLE, int width = wxCOL_WIDTH_DEFAULT,
		wxAlignment align = wxALIGN_LEFT, int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* AppendProgressColumn(const wxString& label,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxCOL_WIDTH_DEFAULT,
		wxAlignment align = wxALIGN_LEFT, int flags = wxDATAVIEW_COL_RESIZABLE);
	wxDataViewExtColumn* AppendIconTextColumn(const wxString& label,
		wxDataViewExtCellMode mode = wxDATAVIEW_CELL_INERT, int width = wxCOL_WIDTH_DEFAULT,
		wxAlignment align = wxALIGN_LEFT, int flags = wxDATAVIEW_COL_RESIZABLE);

	void AppendItem(const wxVector<wxVariant>& values, wxUIntPtr data = 0)
	{
		GetStore()->AppendItem(values, data);
	}
	void PrependItem(const wxVector<wxVariant>& values, wxUIntPtr data = 0)
	{
		GetStore()->PrependItem(values, data);
	}
	void InsertItem(unsigned int row, const wxVector<wxVariant>& values, wxUIntPtr data = 0)
	{
		GetStore()->InsertItem(row, values, data);
	}
	void DeleteItem(unsigned row)
	{
		GetStore()->DeleteItem(row);
	}
	void DeleteAllItems()
	{
		GetStore()->DeleteAllItems();
	}

	void SetValue(const wxVariant& value, unsigned int row, unsigned int col)
	{
		GetStore()->SetValueByRow(value, row, col);
		GetStore()->RowValueChanged(row, col);
	}
	void GetValue(wxVariant& value, unsigned int row, unsigned int col)
	{
		GetStore()->GetValueByRow(value, row, col);
	}

	void SetTextValue(const wxString& value, unsigned int row, unsigned int col)
	{
		GetStore()->SetValueByRow(value, row, col);
		GetStore()->RowValueChanged(row, col);
	}
	wxString GetTextValue(unsigned int row, unsigned int col) const
	{
		wxVariant value; GetStore()->GetValueByRow(value, row, col); return value.GetString();
	}

	void SetToggleValue(bool value, unsigned int row, unsigned int col)
	{
		GetStore()->SetValueByRow(value, row, col);
		GetStore()->RowValueChanged(row, col);
	}
	bool GetToggleValue(unsigned int row, unsigned int col) const
	{
		wxVariant value; GetStore()->GetValueByRow(value, row, col); return value.GetBool();
	}

	void SetItemData(const wxDataViewExtItem& item, wxUIntPtr data)
	{
		GetStore()->SetItemData(item, data);
	}
	wxUIntPtr GetItemData(const wxDataViewExtItem& item) const
	{
		return GetStore()->GetItemData(item);
	}

	int GetItemCount() const
	{
		return GetStore()->GetItemCount();
	}

private:
	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxDataViewExtListCtrl);
};

//-----------------------------------------------------------------------------
// wxDataViewExtTreeStore
//-----------------------------------------------------------------------------

class wxDataViewExtTreeStoreNode
{
public:
	wxDataViewExtTreeStoreNode(wxDataViewExtTreeStoreNode* parent,
		const wxString& text,
		const wxBitmapBundle& icon = wxBitmapBundle(),
		wxClientData* data = NULL);
	virtual ~wxDataViewExtTreeStoreNode();

	void SetText(const wxString& text)
	{
		m_text = text;
	}
	wxString GetText() const
	{
		return m_text;
	}
	void SetIcon(const wxBitmapBundle& icon)
	{
		m_icon = icon;
	}
	const wxBitmapBundle& GetBitmapBundle() const
	{
		return m_icon;
	}
	wxIcon GetIcon() const
	{
		return m_icon.GetIcon(wxDefaultSize);
	}
	void SetData(wxClientData* data)
	{
		delete m_data; m_data = data;
	}
	wxClientData* GetData() const
	{
		return m_data;
	}

	wxDataViewExtItem GetItem() const
	{
		return wxDataViewExtItem(const_cast<void*>(static_cast<const void*>(this)));
	}

	virtual bool IsContainer()
	{
		return false;
	}

	wxDataViewExtTreeStoreNode* GetParent()
	{
		return m_parent;
	}

private:
	wxDataViewExtTreeStoreNode* m_parent;
	wxString                  m_text;
	wxBitmapBundle            m_icon;
	wxClientData* m_data;
};

typedef wxVector<wxDataViewExtTreeStoreNode*> wxDataViewExtTreeStoreNodes;

class wxDataViewExtTreeStoreContainerNode : public wxDataViewExtTreeStoreNode
{
public:
	wxDataViewExtTreeStoreContainerNode(wxDataViewExtTreeStoreNode* parent,
		const wxString& text,
		const wxBitmapBundle& icon = wxBitmapBundle(),
		const wxBitmapBundle& expanded = wxBitmapBundle(),
		wxClientData* data = NULL);
	virtual ~wxDataViewExtTreeStoreContainerNode();

	const wxDataViewExtTreeStoreNodes& GetChildren() const
	{
		return m_children;
	}
	wxDataViewExtTreeStoreNodes& GetChildren()
	{
		return m_children;
	}

	wxDataViewExtTreeStoreNodes::iterator FindChild(wxDataViewExtTreeStoreNode* node);

	void SetExpandedIcon(const wxBitmapBundle& icon)
	{
		m_iconExpanded = icon;
	}
	const wxBitmapBundle& GetExpandedBitmapBundle() const
	{
		return m_iconExpanded;
	}
	wxIcon GetExpandedIcon() const
	{
		return m_iconExpanded.GetIcon(wxDefaultSize);
	}

	void SetExpanded(bool expanded = true)
	{
		m_isExpanded = expanded;
	}
	bool IsExpanded() const
	{
		return m_isExpanded;
	}

	virtual bool IsContainer() wxOVERRIDE
	{
		return true;
	}

	void DestroyChildren();

private:
	wxDataViewExtTreeStoreNodes     m_children;
	wxBitmapBundle               m_iconExpanded;
	bool                         m_isExpanded;
};

//-----------------------------------------------------------------------------

class wxDataViewExtTreeStore : public wxDataViewExtModel
{
public:
	wxDataViewExtTreeStore();
	~wxDataViewExtTreeStore();

	wxDataViewExtItem AppendItem(const wxDataViewExtItem& parent,
		const wxString& text,
		const wxBitmapBundle& icon = wxBitmapBundle(),
		wxClientData* data = NULL);
	wxDataViewExtItem PrependItem(const wxDataViewExtItem& parent,
		const wxString& text,
		const wxBitmapBundle& icon = wxBitmapBundle(),
		wxClientData* data = NULL);
	wxDataViewExtItem InsertItem(const wxDataViewExtItem& parent, const wxDataViewExtItem& previous,
		const wxString& text,
		const wxBitmapBundle& icon = wxBitmapBundle(),
		wxClientData* data = NULL);

	wxDataViewExtItem PrependContainer(const wxDataViewExtItem& parent,
		const wxString& text,
		const wxBitmapBundle& icon = wxBitmapBundle(),
		const wxBitmapBundle& expanded = wxBitmapBundle(),
		wxClientData* data = NULL);
	wxDataViewExtItem AppendContainer(const wxDataViewExtItem& parent,
		const wxString& text,
		const wxBitmapBundle& icon = wxBitmapBundle(),
		const wxBitmapBundle& expanded = wxBitmapBundle(),
		wxClientData* data = NULL);
	wxDataViewExtItem InsertContainer(const wxDataViewExtItem& parent, const wxDataViewExtItem& previous,
		const wxString& text,
		const wxBitmapBundle& icon = wxBitmapBundle(),
		const wxBitmapBundle& expanded = wxBitmapBundle(),
		wxClientData* data = NULL);

	wxDataViewExtItem GetNthChild(const wxDataViewExtItem& parent, unsigned int pos) const;
	int GetChildCount(const wxDataViewExtItem& parent) const;

	void SetItemText(const wxDataViewExtItem& item, const wxString& text);
	wxString GetItemText(const wxDataViewExtItem& item) const;
	void SetItemIcon(const wxDataViewExtItem& item, const wxBitmapBundle& icon);
	wxBitmapBundle GetItemBitmapBundle(const wxDataViewExtItem& item) const;
	wxIcon GetItemIcon(const wxDataViewExtItem& item) const;
	void SetItemExpandedIcon(const wxDataViewExtItem& item, const wxBitmapBundle& icon);
	wxBitmapBundle GetItemExpandedBitmapBundle(const wxDataViewExtItem& item) const;
	wxIcon GetItemExpandedIcon(const wxDataViewExtItem& item) const;
	void SetItemData(const wxDataViewExtItem& item, wxClientData* data);
	wxClientData* GetItemData(const wxDataViewExtItem& item) const;

	void DeleteItem(const wxDataViewExtItem& item);
	void DeleteChildren(const wxDataViewExtItem& item);
	void DeleteAllItems();

	// implement base methods

	virtual void GetValue(wxVariant& variant,
		const wxDataViewExtItem& item, unsigned int col) const wxOVERRIDE;
	virtual bool SetValue(const wxVariant& variant,
		const wxDataViewExtItem& item, unsigned int col) wxOVERRIDE;
	virtual wxDataViewExtItem GetParent(const wxDataViewExtItem& item) const wxOVERRIDE;
	virtual bool IsContainer(const wxDataViewExtItem& item) const wxOVERRIDE;
	virtual unsigned int GetChildren(const wxDataViewExtItem& item, wxDataViewExtItemArray& children) const wxOVERRIDE;

	virtual int Compare(const wxDataViewExtItem& item1, const wxDataViewExtItem& item2,
		unsigned int column, bool ascending) const wxOVERRIDE;

	virtual bool HasDefaultCompare() const wxOVERRIDE
	{
		return true;
	}

	wxDataViewExtTreeStoreNode* FindNode(const wxDataViewExtItem& item) const;
	wxDataViewExtTreeStoreContainerNode* FindContainerNode(const wxDataViewExtItem& item) const;
	wxDataViewExtTreeStoreNode* GetRoot() const { return m_root; }

public:
	wxDataViewExtTreeStoreNode* m_root;
};

//-----------------------------------------------------------------------------

class wxDataViewExtTreeCtrl : public wxDataViewExtCtrl,
	public wxWithImages
{
public:
	wxDataViewExtTreeCtrl() {}
	wxDataViewExtTreeCtrl(wxWindow* parent,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxDV_NO_HEADER | wxDV_ROW_LINES,
		const wxValidator& validator = wxDefaultValidator)
	{
		Create(parent, id, pos, size, style, validator);
	}

	bool Create(wxWindow* parent,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxDV_NO_HEADER | wxDV_ROW_LINES,
		const wxValidator& validator = wxDefaultValidator);

	wxDataViewExtTreeStore* GetStore()
	{
		return (wxDataViewExtTreeStore*)GetModel();
	}
	const wxDataViewExtTreeStore* GetStore() const
	{
		return (const wxDataViewExtTreeStore*)GetModel();
	}

	bool IsContainer(const wxDataViewExtItem& item) const
	{
		return GetStore()->IsContainer(item);
	}

	wxDataViewExtItem AppendItem(const wxDataViewExtItem& parent,
		const wxString& text, int icon = NO_IMAGE, wxClientData* data = NULL);
	wxDataViewExtItem PrependItem(const wxDataViewExtItem& parent,
		const wxString& text, int icon = NO_IMAGE, wxClientData* data = NULL);
	wxDataViewExtItem InsertItem(const wxDataViewExtItem& parent, const wxDataViewExtItem& previous,
		const wxString& text, int icon = NO_IMAGE, wxClientData* data = NULL);

	wxDataViewExtItem PrependContainer(const wxDataViewExtItem& parent,
		const wxString& text, int icon = NO_IMAGE, int expanded = NO_IMAGE,
		wxClientData* data = NULL);
	wxDataViewExtItem AppendContainer(const wxDataViewExtItem& parent,
		const wxString& text, int icon = NO_IMAGE, int expanded = NO_IMAGE,
		wxClientData* data = NULL);
	wxDataViewExtItem InsertContainer(const wxDataViewExtItem& parent, const wxDataViewExtItem& previous,
		const wxString& text, int icon = NO_IMAGE, int expanded = NO_IMAGE,
		wxClientData* data = NULL);

	wxDataViewExtItem GetNthChild(const wxDataViewExtItem& parent, unsigned int pos) const
	{
		return GetStore()->GetNthChild(parent, pos);
	}
	int GetChildCount(const wxDataViewExtItem& parent) const
	{
		return GetStore()->GetChildCount(parent);
	}
	wxDataViewExtItem GetItemParent(wxDataViewExtItem item) const
	{
		return GetStore()->GetParent(item);
	}

	void SetItemText(const wxDataViewExtItem& item, const wxString& text);
	wxString GetItemText(const wxDataViewExtItem& item) const
	{
		return GetStore()->GetItemText(item);
	}
	void SetItemIcon(const wxDataViewExtItem& item, const wxBitmapBundle& icon);
	wxIcon GetItemIcon(const wxDataViewExtItem& item) const
	{
		return GetStore()->GetItemIcon(item);
	}
	void SetItemExpandedIcon(const wxDataViewExtItem& item, const wxBitmapBundle& icon);
	wxIcon GetItemExpandedIcon(const wxDataViewExtItem& item) const
	{
		return GetStore()->GetItemExpandedIcon(item);
	}
	void SetItemData(const wxDataViewExtItem& item, wxClientData* data)
	{
		GetStore()->SetItemData(item, data);
	}
	wxClientData* GetItemData(const wxDataViewExtItem& item) const
	{
		return GetStore()->GetItemData(item);
	}

	void DeleteItem(const wxDataViewExtItem& item);
	void DeleteChildren(const wxDataViewExtItem& item);
	void DeleteAllItems();

	void OnExpanded(wxDataViewExtEvent& event);
	void OnCollapsed(wxDataViewExtEvent& event);
	void OnSize(wxSizeEvent& event);

protected:
	virtual void OnImagesChanged() wxOVERRIDE;

private:
	wxDECLARE_EVENT_TABLE();
	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxDataViewExtTreeCtrl);
};

// old wxEVT_COMMAND_* constants
#define wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED           wxEVT_DATAVIEW_SELECTION_CHANGED
#define wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED              wxEVT_DATAVIEW_ITEM_ACTIVATED
#define wxEVT_COMMAND_DATAVIEW_ITEM_COLLAPSED              wxEVT_DATAVIEW_ITEM_COLLAPSED
#define wxEVT_COMMAND_DATAVIEW_ITEM_EXPANDED               wxEVT_DATAVIEW_ITEM_EXPANDED
#define wxEVT_COMMAND_DATAVIEW_ITEM_COLLAPSING             wxEVT_DATAVIEW_ITEM_COLLAPSING
#define wxEVT_COMMAND_DATAVIEW_ITEM_EXPANDING              wxEVT_DATAVIEW_ITEM_EXPANDING
#define wxEVT_COMMAND_DATAVIEW_ITEM_START_EDITING          wxEVT_DATAVIEW_ITEM_START_EDITING
#define wxEVT_COMMAND_DATAVIEW_ITEM_EDITING_STARTED        wxEVT_DATAVIEW_ITEM_EDITING_STARTED
#define wxEVT_COMMAND_DATAVIEW_ITEM_EDITING_DONE           wxEVT_DATAVIEW_ITEM_EDITING_DONE
#define wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED          wxEVT_DATAVIEW_ITEM_VALUE_CHANGED
#define wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU           wxEVT_DATAVIEW_ITEM_CONTEXT_MENU
#define wxEVT_COMMAND_DATAVIEW_COLUMN_HEADER_CLICK         wxEVT_DATAVIEW_COLUMN_HEADER_CLICK
#define wxEVT_COMMAND_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK   wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK
#define wxEVT_COMMAND_DATAVIEW_COLUMN_SORTED               wxEVT_DATAVIEW_COLUMN_SORTED
#define wxEVT_COMMAND_DATAVIEW_COLUMN_REORDERED            wxEVT_DATAVIEW_COLUMN_REORDERED
#define wxEVT_COMMAND_DATAVIEW_CACHE_HINT                  wxEVT_DATAVIEW_CACHE_HINT
#define wxEVT_COMMAND_DATAVIEW_ITEM_BEGIN_DRAG             wxEVT_DATAVIEW_ITEM_BEGIN_DRAG
#define wxEVT_COMMAND_DATAVIEW_ITEM_DROP_POSSIBLE          wxEVT_DATAVIEW_ITEM_DROP_POSSIBLE
#define wxEVT_COMMAND_DATAVIEW_ITEM_DROP                   wxEVT_DATAVIEW_ITEM_DROP

#endif // wxUSE_DATAVIEWCTRL

#endif
	// _WX_DATAVIEW_H_BASE_
