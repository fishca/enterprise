///////////////////////////////////////////////////////////////////////////////
// Name:        wx/headerctrl.h
// Purpose:     wxHeaderGenericCtrlBase class: interface of wxHeaderGenericCtrl
// Author:      Vadim Zeitlin
// Created:     2008-12-01
// Copyright:   (c) 2008 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef __WX_HEADERCTRL_H__
#define __WX_HEADERCTRL_H__

#include <wx/control.h>

#if wxUSE_HEADERCTRL

#include <wx/dynarray.h>
#include <wx/vector.h>
#include <wx/overlay.h>

#include <wx/headercol.h>

// notice that the classes in this header are defined in the core library even
// although currently they're only used by wxGrid which is in wxAdv because we
// plan to use it in wxListCtrl which is in core too in the future
class wxHeaderGenericCtrlEvent;

#include "frontend/frontend.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

enum
{
	// allow column drag and drop
	wxHD_ALLOW_REORDER = 0x0001,

	// allow hiding (and showing back) the columns using the menu shown by
	// right clicking the header
	wxHD_ALLOW_HIDE = 0x0002,

	// force putting column images on right
	wxHD_BITMAP_ON_RIGHT = 0x0004,

	// style used by default when creating the control
	wxHD_DEFAULT_STYLE = wxHD_ALLOW_REORDER
};

extern const char wxHeaderGenericCtrlNameStr[];

// ----------------------------------------------------------------------------
// wxHeaderGenericCtrlBase defines the interface of a header control
// ----------------------------------------------------------------------------

class wxHeaderGenericCtrlBase : public wxControl
{
public:
	/*
		Derived classes must provide default ctor as well as a ctor and
		Create() function with the following signatures:

	wxHeaderGenericCtrl(wxWindow *parent,
				 wxWindowID winid = wxID_ANY,
				 const wxPoint& pos = wxDefaultPosition,
				 const wxSize& size = wxDefaultSize,
				 long style = wxHD_DEFAULT_STYLE,
				 const wxString& name = wxASCII_STR(wxHeaderGenericCtrlNameStr));

	bool Create(wxWindow *parent,
				wxWindowID winid = wxID_ANY,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxHD_DEFAULT_STYLE,
				const wxString& name = wxASCII_STR(wxHeaderGenericCtrlNameStr));
	 */

	 // column-related methods
	 // ----------------------

	 // set the number of columns in the control
	 //
	 // this also calls UpdateColumn() for all columns
	void SetColumnCount(unsigned int count);

	// return the number of columns in the control as set by SetColumnCount()
	unsigned int GetColumnCount() const { return DoGetCount(); }

	// return whether the control has any columns
	bool IsEmpty() const { return DoGetCount() == 0; }

	// update the column with the given index
	void UpdateColumn(unsigned int idx)
	{
		wxCHECK_RET(idx < GetColumnCount(), "invalid column index");

		DoUpdate(idx);
	}


	// columns order
	// -------------

	// set the columns order: the array defines the column index which appears
	// the given position, it must have GetColumnCount() elements and contain
	// all indices exactly once
	void SetColumnsOrder(const wxArrayInt& order);
	wxArrayInt GetColumnsOrder() const;

	// get the index of the column at the given display position
	unsigned int GetColumnAt(unsigned int pos) const;

	// get the position at which this column is currently displayed
	unsigned int GetColumnPos(unsigned int idx) const;

	// reset the columns order to the natural one
	void ResetColumnsOrder();

	// helper function used by the generic version of this control and also
	// wxGrid: reshuffles the array of column indices indexed by positions
	// (i.e. using the same convention as for SetColumnsOrder()) so that the
	// column with the given index is found at the specified position
	static void MoveColumnInOrderArray(wxArrayInt& order,
		unsigned int idx,
		unsigned int pos);


	// UI helpers
	// ----------

#if wxUSE_MENUS
	// show the popup menu containing all columns with check marks for the ones
	// which are currently shown and return true if something was done using it
	// (in this case UpdateColumnVisibility() will have been called) or false
	// if the menu was cancelled
	//
	// this is called from the default right click handler for the controls
	// with wxHD_ALLOW_HIDE style
	bool ShowColumnsMenu(const wxPoint& pt, const wxString& title = wxString());

	// append the entries for all our columns to the given menu, with the
	// currently visible columns being checked
	//
	// this is used by ShowColumnsMenu() but can also be used if you use your
	// own custom columns menu but nevertheless want to show all the columns in
	// it
	//
	// the ids of the items corresponding to the columns are consecutive and
	// start from idColumnsBase
	void AddColumnsItems(wxMenu& menu, int idColumnsBase = 0);
#endif // wxUSE_MENUS

	// show the columns customization dialog and return true if something was
	// changed using it (in which case UpdateColumnVisibility() and/or
	// UpdateColumnsOrder() will have been called)
	//
	// this is called by the control itself from ShowColumnsMenu() (which in
	// turn is only called by the control if wxHD_ALLOW_HIDE style was
	// specified) and if the control has wxHD_ALLOW_REORDER style as well
	bool ShowCustomizeDialog();

	// compute column title width
	int GetColumnTitleWidth(const wxHeaderColumn& col);

	// compute column title width for the column with the given index
	int GetColumnTitleWidth(unsigned int idx)
	{
		return GetColumnTitleWidth(GetColumn(idx));
	}

	// implementation only from now on
	// -------------------------------

	// the user doesn't need to TAB to this control
	virtual bool AcceptsFocusFromKeyboard() const wxOVERRIDE { return false; }

	// this method is only overridden in order to synchronize the control with
	// the main window when it is scrolled, the derived class must implement
	// DoScrollHorz()
	virtual void ScrollWindow(int dx, int dy, const wxRect* rect = NULL) wxOVERRIDE;

protected:
	// this method must be implemented by the derived classes to return the
	// information for the given column
	virtual const wxHeaderColumn& GetColumn(unsigned int idx) const = 0;

	// this method is called from the default EVT_HEADER_SEPARATOR_DCLICK
	// handler to update the fitting column width of the given column, it
	// should return true if the width was really updated
	virtual bool UpdateColumnWidthToFit(unsigned int WXUNUSED(idx),
		int WXUNUSED(widthTitle))
	{
		return false;
	}

	// this method is called from ShowColumnsMenu() and must be overridden to
	// update the internal column visibility (there is no need to call
	// UpdateColumn() from here, this will be done internally)
	virtual void UpdateColumnVisibility(unsigned int WXUNUSED(idx),
		bool WXUNUSED(show))
	{
		wxFAIL_MSG("must be overridden if called");
	}

	// this method is called from ShowCustomizeDialog() to reorder all columns
	// at once and should be implemented for controls using wxHD_ALLOW_REORDER
	// style (there is no need to call SetColumnsOrder() from here, this is
	// done by the control itself)
	virtual void UpdateColumnsOrder(const wxArrayInt& WXUNUSED(order))
	{
		wxFAIL_MSG("must be overridden if called");
	}

	// this method can be overridden in the derived classes to do something
	// (e.g. update/resize some internal data structures) before the number of
	// columns in the control changes
	virtual void OnColumnCountChanging(unsigned int WXUNUSED(count)) {}


	// helper function for the derived classes: update the array of column
	// indices after the number of columns changed
	void DoResizeColumnIndices(wxArrayInt& colIndices, unsigned int count);

protected:
	// this window doesn't look nice with the border so don't use it by default
	virtual wxBorder GetDefaultBorder() const wxOVERRIDE { return wxBORDER_NONE; }

private:
	// methods implementing our public API and defined in platform-specific
	// implementations
	virtual void DoSetCount(unsigned int count) = 0;
	virtual unsigned int DoGetCount() const = 0;
	virtual void DoUpdate(unsigned int idx) = 0;

	virtual void DoScrollHorz(int dx) = 0;

	virtual void DoSetColumnsOrder(const wxArrayInt& order) = 0;
	virtual wxArrayInt DoGetColumnsOrder() const = 0;


	// event handlers
	void OnSeparatorDClick(wxHeaderGenericCtrlEvent& event);
#if wxUSE_MENUS
	void OnRClick(wxHeaderGenericCtrlEvent& event);
#endif // wxUSE_MENUS

	wxDECLARE_EVENT_TABLE();
};

// ----------------------------------------------------------------------------
// wxHeaderGenericCtrl
// ----------------------------------------------------------------------------

class wxHeaderGenericCtrl : public wxHeaderGenericCtrlBase
{
public:
	wxHeaderGenericCtrl()
	{
		Init();
	}

	wxHeaderGenericCtrl(wxWindow* parent,
		wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxHD_DEFAULT_STYLE,
		const wxString& name = wxASCII_STR(wxHeaderGenericCtrlNameStr))
	{
		Init();

		Create(parent, id, pos, size, style, name);
	}

	bool Create(wxWindow* parent,
		wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxHD_DEFAULT_STYLE,
		const wxString& name = wxASCII_STR(wxHeaderGenericCtrlNameStr));

	virtual ~wxHeaderGenericCtrl();

	void SetColumnHeight(int point) 
	{ 
		InvalidateBestSize();
		m_numHeight = point; 
	}

	int GetColumnHeight() const { return m_numHeight; }

protected:

	virtual wxSize DoGetBestSize() const wxOVERRIDE;

private:

	// implement base class pure virtuals
	virtual void DoSetCount(unsigned int count) wxOVERRIDE;
	virtual unsigned int DoGetCount() const wxOVERRIDE;
	virtual void DoUpdate(unsigned int idx) wxOVERRIDE;

	virtual void DoScrollHorz(int dx) wxOVERRIDE;

	virtual void DoSetColumnsOrder(const wxArrayInt& order) wxOVERRIDE;
	virtual wxArrayInt DoGetColumnsOrder() const wxOVERRIDE;

	// common part of all ctors
	void Init();

	// event handlers
	void OnPaint(wxPaintEvent& event);
	void OnMouse(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnCaptureLost(wxMouseCaptureLostEvent& event);

	// move the column with given idx at given position (this doesn't generate
	// any events but does refresh the display)
	void DoMoveCol(unsigned int idx, unsigned int pos);

	// return the horizontal start position of the given column in physical
	// coordinates
	int GetColStart(unsigned int idx) const;

	// and the end position
	int GetColEnd(unsigned int idx) const;

	// refresh the given column [only]; idx must be valid
	void RefreshCol(unsigned int idx);

	// refresh the given column if idx is valid
	void RefreshColIfNotNone(unsigned int idx);

	// refresh all the controls starting from (and including) the given one
	void RefreshColsAfter(unsigned int idx);

	// return the column at the given position or -1 if it is beyond the
	// rightmost column and put true into onSeparator output parameter if the
	// position is near the divider at the right end of this column (notice
	// that this means that we return column 0 even if the position is over
	// column 1 but close enough to the divider separating it from column 0)
	unsigned int FindColumnAtPoint(int x, bool* onSeparator = NULL) const;

	// return the result of FindColumnAtPoint() if it is a valid column,
	// otherwise the index of the last (rightmost) displayed column
	unsigned int FindColumnClosestToPoint(int xPhysical) const;

	// return true if a drag resizing operation is currently in progress
	bool IsResizing() const;

	// return true if a drag reordering operation is currently in progress
	bool IsReordering() const;

	// return true if any drag operation is currently in progress
	bool IsDragging() const { return IsResizing() || IsReordering(); }

	// end any drag operation currently in progress (resizing or reordering)
	void EndDragging();

	// cancel the drag operation currently in progress and generate an event
	// about it
	void CancelDragging();

	// start (if m_colBeingResized is -1) or continue resizing the column
	//
	// this generates wxEVT_HEADER_BEGIN_RESIZE/RESIZING events and can
	// cancel the operation if the user handler decides so
	void StartOrContinueResizing(unsigned int col, int xPhysical);

	// end the resizing operation currently in progress and generate an event
	// about it with its cancelled flag set if xPhysical is -1
	void EndResizing(int xPhysical);

	// same functions as above but for column moving/reordering instead of
	// resizing
	void StartReordering(unsigned int col, int xPhysical);

	// returns true if we did drag the column somewhere else or false if we
	// didn't really move it -- in this case we consider that no reordering
	// took place and that a normal column click event should be generated
	bool EndReordering(int xPhysical);

	// constrain the given position to be larger than the start position of the
	// given column plus its minimal width and return the effective width
	int ConstrainByMinWidth(unsigned int col, int& xPhysical);

	// update the information displayed while a column is being moved around
	void UpdateReorderingMarker(int xPhysical);

	// clear any overlaid markers
	void ClearMarkers();

	// pos physical
	int m_xPhysical;

	//number of column height point
	unsigned int m_numHeight;

	// number of columns in the control currently
	unsigned int m_numColumns;

	// index of the column under mouse or -1 if none
	unsigned int m_hover;

	// the column being resized or -1 if there is no resizing operation in
	// progress
	unsigned int m_colBeingResized;

	// the column being moved or -1 if there is no reordering operation in
	// progress
	unsigned int m_colBeingReordered;

	// the distance from the start of m_colBeingReordered and the mouse
	// position when the user started to drag it
	int m_dragOffset;

	// the horizontal scroll offset
	int m_scrollOffset;

	// the overlay display used during the dragging operations
	wxOverlay m_overlay;

	// the indices of the column appearing at the given position on the display
	// (its size is always m_numColumns)
	wxArrayInt m_colIndices;

	bool m_wasSeparatorDClick;

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_NO_COPY_CLASS(wxHeaderGenericCtrl);
};

// ----------------------------------------------------------------------------
// wxHeaderGenericCtrlSimple: concrete header control which can be used standalone
// ----------------------------------------------------------------------------

class wxHeaderGenericCtrlSimple : public wxHeaderGenericCtrl
{
public:
	// control creation
	// ----------------

	wxHeaderGenericCtrlSimple() { Init(); }
	wxHeaderGenericCtrlSimple(wxWindow* parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxHD_DEFAULT_STYLE,
		const wxString& name = wxASCII_STR(wxHeaderGenericCtrlNameStr))
	{
		Init();

		Create(parent, winid, pos, size, style, name);
	}

	// managing the columns
	// --------------------

	// insert the column at the given position, using GetColumnCount() as
	// position appends it at the end
	void InsertColumn(const wxHeaderColumnSimple& col, unsigned int idx)
	{
		wxCHECK_RET(idx <= GetColumnCount(), "invalid column index");

		DoInsert(col, idx);
	}

	// append the column to the end of the control
	void AppendColumn(const wxHeaderColumnSimple& col)
	{
		DoInsert(col, GetColumnCount());
	}

	// delete the column at the given index
	void DeleteColumn(unsigned int idx)
	{
		wxCHECK_RET(idx < GetColumnCount(), "invalid column index");

		DoDelete(idx);
	}

	// delete all the existing columns
	void DeleteAllColumns();


	// modifying columns
	// -----------------

	// show or hide the column, notice that even when a column is hidden we
	// still account for it when using indices
	void ShowColumn(unsigned int idx, bool show = true)
	{
		wxCHECK_RET(idx < GetColumnCount(), "invalid column index");

		DoShowColumn(idx, show);
	}

	void HideColumn(unsigned int idx)
	{
		ShowColumn(idx, false);
	}

	// indicate that the column is used for sorting
	void ShowSortIndicator(unsigned int idx, bool ascending = true)
	{
		wxCHECK_RET(idx < GetColumnCount(), "invalid column index");

		DoShowSortIndicator(idx, ascending);
	}

	// remove the sort indicator completely
	void RemoveSortIndicator();

protected:
	// implement/override base class methods
	virtual const wxHeaderColumn& GetColumn(unsigned int idx) const wxOVERRIDE;
	virtual bool UpdateColumnWidthToFit(unsigned int idx, int widthTitle) wxOVERRIDE;

	// and define another one to be overridden in the derived classes: it
	// should return the best width for the given column contents or -1 if not
	// implemented, we use it to implement UpdateColumnWidthToFit()
	virtual int GetBestFittingWidth(unsigned int WXUNUSED(idx)) const
	{
		return -1;
	}

	void OnHeaderResizing(wxHeaderGenericCtrlEvent& evt);

private:
	// functions implementing our public API
	void DoInsert(const wxHeaderColumnSimple& col, unsigned int idx);
	void DoDelete(unsigned int idx);
	void DoShowColumn(unsigned int idx, bool show);
	void DoShowSortIndicator(unsigned int idx, bool ascending);

	// common part of all ctors
	void Init();

	// bring the column count in sync with the number of columns we store
	void UpdateColumnCount()
	{
		SetColumnCount(static_cast<int>(m_cols.size()));
	}


	// all our current columns
	typedef wxVector<wxHeaderColumnSimple> Columns;
	Columns m_cols;

	// the column currently used for sorting or -1 if none
	unsigned int m_sortKey;


	wxDECLARE_NO_COPY_CLASS(wxHeaderGenericCtrlSimple);
	wxDECLARE_EVENT_TABLE();
};

// ----------------------------------------------------------------------------
// wxHeaderGenericCtrl events
// ----------------------------------------------------------------------------

class wxHeaderGenericCtrlEvent : public wxNotifyEvent
{
public:
	wxHeaderGenericCtrlEvent(wxEventType commandType = wxEVT_NULL, int winid = 0)
		: wxNotifyEvent(commandType, winid),
		m_col(-1),
		m_width(0),
		m_order(static_cast<unsigned int>(-1))
	{
	}

	wxHeaderGenericCtrlEvent(const wxHeaderGenericCtrlEvent& event)
		: wxNotifyEvent(event),
		m_col(event.m_col),
		m_width(event.m_width),
		m_order(event.m_order)
	{
	}

	// the column which this event pertains to: valid for all header events
	int GetColumn() const { return m_col; }
	void SetColumn(int col) { m_col = col; }

	// the width of the column: valid for column resizing/dragging events only
	int GetWidth() const { return m_width; }
	void SetWidth(int width) { m_width = width; }

	// the new position of the column: for end reorder events only
	unsigned int GetNewOrder() const { return m_order; }
	void SetNewOrder(unsigned int order) { m_order = order; }

	virtual wxEvent* Clone() const wxOVERRIDE { return new wxHeaderGenericCtrlEvent(*this); }

protected:
	// the column affected by the event
	int m_col;

	// the current width for the dragging events
	int m_width;

	// the new column position for end reorder event
	unsigned int m_order;

private:

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxHeaderGenericCtrlEvent);
};


wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_HEADER_CLICK, wxHeaderGenericCtrlEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_HEADER_RIGHT_CLICK, wxHeaderGenericCtrlEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_HEADER_MIDDLE_CLICK, wxHeaderGenericCtrlEvent);

wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_HEADER_DCLICK, wxHeaderGenericCtrlEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_HEADER_RIGHT_DCLICK, wxHeaderGenericCtrlEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_HEADER_MIDDLE_DCLICK, wxHeaderGenericCtrlEvent);

wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_HEADER_SEPARATOR_DCLICK, wxHeaderGenericCtrlEvent);

wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_HEADER_BEGIN_RESIZE, wxHeaderGenericCtrlEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_HEADER_RESIZING, wxHeaderGenericCtrlEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_HEADER_END_RESIZE, wxHeaderGenericCtrlEvent);

wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_HEADER_BEGIN_REORDER, wxHeaderGenericCtrlEvent);
wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_HEADER_END_REORDER, wxHeaderGenericCtrlEvent);

wxDECLARE_EXPORTED_EVENT(FRONTEND_API, wxEVT_HEADER_DRAGGING_CANCELLED, wxHeaderGenericCtrlEvent);

typedef void (wxEvtHandler::* wxHeaderGenericCtrlEventFunction)(wxHeaderGenericCtrlEvent&);

#define wxHeaderGenericCtrlEventHandler(func) \
    wxEVENT_HANDLER_CAST(wxHeaderGenericCtrlEventFunction, func)

#define wx__DECLARE_HEADER_EVT(evt, id, fn) \
    wx__DECLARE_EVT1(wxEVT_HEADER_ ## evt, id, wxHeaderGenericCtrlEventHandler(fn))

#define EVT_HEADER_CLICK(id, fn) wx__DECLARE_HEADER_EVT(CLICK, id, fn)
#define EVT_HEADER_RIGHT_CLICK(id, fn) wx__DECLARE_HEADER_EVT(RIGHT_CLICK, id, fn)
#define EVT_HEADER_MIDDLE_CLICK(id, fn) wx__DECLARE_HEADER_EVT(MIDDLE_CLICK, id, fn)

#define EVT_HEADER_DCLICK(id, fn) wx__DECLARE_HEADER_EVT(DCLICK, id, fn)
#define EVT_HEADER_RIGHT_DCLICK(id, fn) wx__DECLARE_HEADER_EVT(RIGHT_DCLICK, id, fn)
#define EVT_HEADER_MIDDLE_DCLICK(id, fn) wx__DECLARE_HEADER_EVT(MIDDLE_DCLICK, id, fn)

#define EVT_HEADER_SEPARATOR_DCLICK(id, fn) wx__DECLARE_HEADER_EVT(SEPARATOR_DCLICK, id, fn)

#define EVT_HEADER_BEGIN_RESIZE(id, fn) wx__DECLARE_HEADER_EVT(BEGIN_RESIZE, id, fn)
#define EVT_HEADER_RESIZING(id, fn) wx__DECLARE_HEADER_EVT(RESIZING, id, fn)
#define EVT_HEADER_END_RESIZE(id, fn) wx__DECLARE_HEADER_EVT(END_RESIZE, id, fn)

#define EVT_HEADER_BEGIN_REORDER(id, fn) wx__DECLARE_HEADER_EVT(BEGIN_REORDER, id, fn)
#define EVT_HEADER_END_REORDER(id, fn) wx__DECLARE_HEADER_EVT(END_REORDER, id, fn)

#define EVT_HEADER_DRAGGING_CANCELLED(id, fn) wx__DECLARE_HEADER_EVT(DRAGGING_CANCELLED, id, fn)

// old wxEVT_COMMAND_* constants
#define wxEVT_COMMAND_HEADER_CLICK                wxEVT_HEADER_CLICK
#define wxEVT_COMMAND_HEADER_RIGHT_CLICK          wxEVT_HEADER_RIGHT_CLICK
#define wxEVT_COMMAND_HEADER_MIDDLE_CLICK         wxEVT_HEADER_MIDDLE_CLICK
#define wxEVT_COMMAND_HEADER_DCLICK               wxEVT_HEADER_DCLICK
#define wxEVT_COMMAND_HEADER_RIGHT_DCLICK         wxEVT_HEADER_RIGHT_DCLICK
#define wxEVT_COMMAND_HEADER_MIDDLE_DCLICK        wxEVT_HEADER_MIDDLE_DCLICK
#define wxEVT_COMMAND_HEADER_SEPARATOR_DCLICK     wxEVT_HEADER_SEPARATOR_DCLICK
#define wxEVT_COMMAND_HEADER_BEGIN_RESIZE         wxEVT_HEADER_BEGIN_RESIZE
#define wxEVT_COMMAND_HEADER_RESIZING             wxEVT_HEADER_RESIZING
#define wxEVT_COMMAND_HEADER_END_RESIZE           wxEVT_HEADER_END_RESIZE
#define wxEVT_COMMAND_HEADER_BEGIN_REORDER        wxEVT_HEADER_BEGIN_REORDER
#define wxEVT_COMMAND_HEADER_END_REORDER          wxEVT_HEADER_END_REORDER
#define wxEVT_COMMAND_HEADER_DRAGGING_CANCELLED   wxEVT_HEADER_DRAGGING_CANCELLED

#endif // wxUSE_HEADERCTRL

#endif // _WX_HEADERCTRL_H_
