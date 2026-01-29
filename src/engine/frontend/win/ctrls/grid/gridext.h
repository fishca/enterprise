/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/grid.h
// Purpose:     wxGridExt and related classes
// Author:      Michael Bedward (based on code by Julian Smart, Robin Dunn)
// Modified by: Santiago Palacios
// Created:     1/08/1999
// Copyright:   (c) Michael Bedward
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_GRID_H_
#define _WX_GENERIC_GRID_H_

#include "wx/defs.h"

#if wxUSE_GRID

#include "wx/hashmap.h"

#include "wx/scrolwin.h"

#if wxUSE_STD_CONTAINERS_COMPATIBLY
#include <iterator>
#endif

	// ----------------------------------------------------------------------------
	// constants
	// ----------------------------------------------------------------------------

extern const char wxGridExtNameStr[];

// Obsolete constants not used by wxWidgets itself any longer, preserved only
// for compatibility.
#define WXGRID_DEFAULT_NUMBER_ROWS            10
#define WXGRID_DEFAULT_NUMBER_COLS            10
#if defined(__WXMSW__) || defined(__WXGTK20__)
#define WXGRID_DEFAULT_ROW_HEIGHT             25
#else
#define WXGRID_DEFAULT_ROW_HEIGHT             30
#endif  // __WXMSW__
#define WXGRID_DEFAULT_SCROLLBAR_WIDTH        16

// Various constants used in wxGridExt code.
//
// Note that all the values are in DIPs, not pixels, i.e. you must use
// FromDIP() when using them in your code.
#define WXGRID_DEFAULT_COL_WIDTH              80
#define WXGRID_DEFAULT_COL_LABEL_HEIGHT       32
#define WXGRID_DEFAULT_ROW_LABEL_WIDTH        82
#define WXGRID_LABEL_EDGE_ZONE                 2
#define WXGRID_MIN_ROW_HEIGHT                 15
#define WXGRID_MIN_COL_WIDTH                  15

// type names for grid table values
#define wxGRID_VALUE_STRING     wxT("string")
#define wxGRID_VALUE_BOOL       wxT("bool")
#define wxGRID_VALUE_NUMBER     wxT("long")
#define wxGRID_VALUE_FLOAT      wxT("double")
#define wxGRID_VALUE_CHOICE     wxT("choice")
#define wxGRID_VALUE_DATE       wxT("date")

#define wxGRID_VALUE_TEXT wxGRID_VALUE_STRING
#define wxGRID_VALUE_LONG wxGRID_VALUE_NUMBER

// magic constant which tells (to some functions) to automatically calculate
// the appropriate size
#define wxGRID_AUTOSIZE (-1)

// many wxGridExt methods work either with columns or rows, this enum is used for
// the parameter indicating which one should it be
enum wxGridExtDirection
{
	wxGRID_COLUMN,
	wxGRID_ROW
};

// Flags used with wxGridExt::Render() to select parts of the grid to draw.
enum wxGridExtRenderStyle
{
	wxGRID_DRAW_ROWS_HEADER = 0x001,
	wxGRID_DRAW_COLS_HEADER = 0x002,
	wxGRID_DRAW_CELL_LINES = 0x004,
	wxGRID_DRAW_BOX_RECT = 0x008,
	wxGRID_DRAW_SELECTION = 0x010,
	wxGRID_DRAW_DEFAULT = wxGRID_DRAW_ROWS_HEADER |
	wxGRID_DRAW_COLS_HEADER |
	wxGRID_DRAW_CELL_LINES |
	wxGRID_DRAW_BOX_RECT
};

// ----------------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------------

class wxGridExt;
class wxGridExtCellAttr;
class wxGridExtCellAttrProviderData;
class wxGridExtColLabelWindow;
class wxGridExtCornerLabelWindow;
class wxGridExtEvent;
class wxGridExtRowAreaWindow;
class wxGridExtRowLabelWindow;
class wxGridExtWindow;
class wxGridExtSubwindow;
class wxGridExtTypeRegistry;
class wxGridExtSelection;

class WXDLLIMPEXP_FWD_CORE wxHeaderCtrl;
class WXDLLIMPEXP_FWD_CORE wxCheckBox;
class WXDLLIMPEXP_FWD_CORE wxComboBox;
class WXDLLIMPEXP_FWD_CORE wxTextCtrl;
#if wxUSE_SPINCTRL
class WXDLLIMPEXP_FWD_CORE wxSpinCtrl;
#endif
#if wxUSE_DATEPICKCTRL
class WXDLLIMPEXP_FWD_CORE wxDatePickerCtrl;
#endif

class wxGridExtFixedIndicesSet;

class wxGridExtOperations;
class wxGridExtRowOperations;
class wxGridExtColumnOperations;
class wxGridExtDirectionOperations;


// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

#define wxSafeIncRef(p) if ( p ) (p)->IncRef()
#define wxSafeDecRef(p) if ( p ) (p)->DecRef()

// ----------------------------------------------------------------------------
// wxGridExtCellWorker: common base class for wxGridExtCellRenderer and
// wxGridExtCellEditor
//
// NB: this is more an implementation convenience than a design issue, so this
//     class is not documented and is not public at all
// ----------------------------------------------------------------------------

class wxGridExtCellWorker : public wxSharedClientDataContainer,
	public wxRefCounter
{
public:
	wxGridExtCellWorker() {}

	wxGridExtCellWorker(const wxGridExtCellWorker& other);

	// interpret renderer parameters: arbitrary string whose interpretation is
	// left to the derived classes
	virtual void SetParameters(const wxString& params);

protected:
	// virtual dtor for any base class - private because only DecRef() can
	// delete us
	virtual ~wxGridExtCellWorker();

private:
	// suppress the stupid gcc warning about the class having private dtor and
	// no friends
	friend class wxGridExtCellWorkerDummyFriend;
};

// ----------------------------------------------------------------------------
// wxGridExtCellRenderer: this class is responsible for actually drawing the cell
// in the grid. You may pass it to the wxGridExtCellAttr (below) to change the
// format of one given cell or to wxGridExt::SetDefaultRenderer() to change the
// view of all cells. This is an ABC, you will normally use one of the
// predefined derived classes or derive your own class from it.
// ----------------------------------------------------------------------------

class wxGridExtCellRenderer : public wxGridExtCellWorker
{
public:
	wxGridExtCellRenderer()
		: wxGridExtCellWorker()
	{
	}

	wxGridExtCellRenderer(const wxGridExtCellRenderer& other)
		: wxGridExtCellWorker(other)
	{
	}

	// draw the given cell on the provided DC inside the given rectangle
	// using the style specified by the attribute and the default or selected
	// state corresponding to the isSelected value.
	//
	// this pure virtual function has a default implementation which will
	// prepare the DC using the given attribute: it will draw the rectangle
	// with the bg colour from attr and set the text colour and font
	virtual void Draw(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc,
		const wxRect& rect,
		int row, int col,
		bool isSelected) = 0;

	// get the preferred size of the cell for its contents
	virtual wxSize GetBestSize(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc,
		int row, int col) = 0;

	// Get the preferred height for a given width. Override this method if the
	// renderer computes height as function of its width, as is the case of the
	// standard wxGridExtCellAutoWrapStringRenderer, for example.
	// and vice versa
	virtual int GetBestHeight(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc,
		int row, int col,
		int WXUNUSED(width))
	{
		return GetBestSize(grid, attr, dc, row, col).GetHeight();
	}

	// Get the preferred width for a given height, this is the symmetric
	// version of GetBestHeight().
	virtual int GetBestWidth(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc,
		int row, int col,
		int WXUNUSED(height))
	{
		return GetBestSize(grid, attr, dc, row, col).GetWidth();
	}


	// Unlike GetBestSize(), this functions is optional: it is used when
	// auto-sizing columns to determine the best width without iterating over
	// all cells in this column, if possible.
	//
	// If it isn't, return wxDefaultSize as the base class version does by
	// default.
	virtual wxSize GetMaxBestSize(wxGridExt& WXUNUSED(grid),
		wxGridExtCellAttr& WXUNUSED(attr),
		wxDC& WXUNUSED(dc))
	{
		return wxDefaultSize;
	}

	// create a new object which is the copy of this one
	virtual wxGridExtCellRenderer* Clone() const = 0;

protected:
	// set the text colours before drawing
	void SetTextColoursAndFont(const wxGridExt& grid,
		const wxGridExtCellAttr& attr,
		wxDC& dc,
		bool isSelected);
};

// Smart pointer to wxGridExtCellRenderer, calling DecRef() on it automatically.
typedef wxObjectDataPtr<wxGridExtCellRenderer> wxGridExtCellRendererPtr;


// ----------------------------------------------------------------------------
// Helper classes used by wxGridExtCellEditor::TryActivate() and DoActivate().
// ----------------------------------------------------------------------------

// This class represents a source of cell activation, which may be either a
// user event (mouse or keyboard) or the program itself.
//
// Note that objects of this class are supposed to be ephemeral and so store
// pointers to the events specified when creating them, which are supposed to
// have life-time greater than that of the objects of this class.
class wxGridExtActivationSource
{
public:
	enum Origin
	{
		Program,
		Key,
		Mouse
	};

	// Factory functions, only used by the library itself.
	static wxGridExtActivationSource FromProgram()
	{
		return wxGridExtActivationSource(Program, NULL);
	}

	static wxGridExtActivationSource From(const wxKeyEvent& event)
	{
		return wxGridExtActivationSource(Key, &event);
	}

	static wxGridExtActivationSource From(const wxMouseEvent& event)
	{
		return wxGridExtActivationSource(Mouse, &event);
	}

	// Accessors allowing to retrieve information about the source.

	// Can be called for any object.
	Origin GetOrigin() const { return m_origin; }

	// Can be called for objects with Key origin only.
	const wxKeyEvent& GetKeyEvent() const
	{
		wxASSERT(m_origin == Key);

		return *static_cast<const wxKeyEvent*>(m_event);
	}

	// Can be called for objects with Mouse origin only.
	const wxMouseEvent& GetMouseEvent() const
	{
		wxASSERT(m_origin == Mouse);

		return *static_cast<const wxMouseEvent*>(m_event);
	}

private:
	wxGridExtActivationSource(Origin origin, const wxEvent* event)
		: m_origin(origin),
		m_event(event)
	{
	}

	const Origin m_origin;
	const wxEvent* const m_event;
};

// This class represents the result of TryActivate(), which may be either
// absence of any action (if activating wouldn't change the value anyhow),
// attempt to change the value to the specified one or just start normal
// editing, which is the default for the editors not supporting activation.
class wxGridExtActivationResult
{
public:
	enum Action
	{
		Ignore,
		Change,
		ShowEditor
	};

	// Factory functions, only used by the library itself.
	static wxGridExtActivationResult DoNothing()
	{
		return wxGridExtActivationResult(Ignore);
	}

	static wxGridExtActivationResult DoChange(const wxString& newval)
	{
		return wxGridExtActivationResult(Change, newval);
	}

	static wxGridExtActivationResult DoEdit()
	{
		return wxGridExtActivationResult(ShowEditor);
	}

	// Accessors allowing to retrieve information about the result.

	// Can be called for any object.
	Action GetAction() const { return m_action; }

	// Can be called for objects with Change action type only.
	const wxString& GetNewValue() const
	{
		wxASSERT(m_action == Change);

		return m_newval;
	}

private:
	explicit
		wxGridExtActivationResult(Action action, const wxString& newval = wxString())
		: m_action(action),
		m_newval(newval)
	{
	}

	const Action m_action;
	const wxString m_newval;
};

// ----------------------------------------------------------------------------
// wxGridExtCellEditor:  This class is responsible for providing and manipulating
// the in-place edit controls for the grid.  Instances of wxGridExtCellEditor
// (actually, instances of derived classes since it is an ABC) can be
// associated with the cell attributes for individual cells, rows, columns, or
// even for the entire grid.
// ----------------------------------------------------------------------------

class wxGridExtCellEditor : public wxGridExtCellWorker
{
public:
	wxGridExtCellEditor()
		: wxGridExtCellWorker(),
		m_control(NULL),
		m_attr(NULL)
	{
	}

	wxGridExtCellEditor(const wxGridExtCellEditor& other);

	bool IsCreated() const { return m_control != NULL; }

	wxWindow* GetWindow() const { return m_control; }
	void SetWindow(wxWindow* window) { m_control = window; }

	wxGridExtCellAttr* GetCellAttr() const { return m_attr; }
	void SetCellAttr(wxGridExtCellAttr* attr) { m_attr = attr; }

	// Creates the actual edit control
	virtual void Create(wxWindow* parent,
		wxWindowID id,
		wxEvtHandler* evtHandler) = 0;

	// Size and position the edit control
	virtual void SetSize(const wxRect& rect);

	// Show or hide the edit control, use the specified attributes to set
	// colours/fonts for it
	virtual void Show(bool show, wxGridExtCellAttr* attr = NULL);

	// Draws the part of the cell not occupied by the control: the base class
	// version just fills it with background colour from the attribute
	virtual void PaintBackground(wxDC& dc,
		const wxRect& rectCell,
		const wxGridExtCellAttr& attr);


	// The methods called by wxGridExt when a cell is edited: first BeginEdit() is
	// called, then EndEdit() is and if it returns true and if the change is
	// not vetoed by a user-defined event handler, finally ApplyEdit() is called

	// Fetch the value from the table and prepare the edit control
	// to begin editing.  Set the focus to the edit control.
	virtual void BeginEdit(int row, int col, wxGridExt* grid) = 0;

	// Returns false if nothing changed, otherwise returns true and return the
	// new value in its string form in the newval output parameter.
	//
	// This should also store the new value in its real type internally so that
	// it could be used by ApplyEdit() but it must not modify the grid as the
	// change could still be vetoed.
	virtual bool EndEdit(int row, int col, const wxGridExt* grid,
		const wxString& oldval, wxString* newval) = 0;

	// Complete the editing of the current cell by storing the value saved by
	// the previous call to EndEdit() in the grid
	virtual void ApplyEdit(int row, int col, wxGridExt* grid) = 0;


	// Reset the value in the control back to its starting value
	virtual void Reset() = 0;

	// return true to allow the given key to start editing: the base class
	// version only checks that the event has no modifiers. The derived
	// classes are supposed to do "if ( base::IsAcceptedKey() && ... )" in
	// their IsAcceptedKey() implementation, although, of course, it is not a
	// mandatory requirement.
	//
	// NB: if the key is F2 (special), editing will always start and this
	//     method will not be called at all (but StartingKey() will)
	virtual bool IsAcceptedKey(wxKeyEvent& event);

	// If the editor is enabled by pressing keys on the grid, this will be
	// called to let the editor do something about that first key if desired
	virtual void StartingKey(wxKeyEvent& event);

	// if the editor is enabled by clicking on the cell, this method will be
	// called
	virtual void StartingClick();

	// Some types of controls on some platforms may need some help
	// with the Return key.
	virtual void HandleReturn(wxKeyEvent& event);

	// Final cleanup
	virtual void Destroy();

	// create a new object which is the copy of this one
	virtual wxGridExtCellEditor* Clone() const = 0;

	// added GetValue so we can get the value which is in the control
	virtual wxString GetValue() const = 0;


	// These functions exist only for backward compatibility, use Get and
	// SetWindow() instead in the new code.
	wxControl* GetControl() { return wxDynamicCast(m_control, wxControl); }
	void SetControl(wxControl* control) { m_control = control; }


	// Support for "activatable" editors: those change the value of the cell
	// immediately, instead of creating an editor control and waiting for user
	// input.
	//
	// See wxGridExtCellBoolEditor for an example of such editor.

	// Override this function to return "Change" activation result from it to
	// show that the editor supports activation. DoActivate() will be called if
	// the cell changing event is not vetoed.
	virtual
		wxGridExtActivationResult
		TryActivate(int WXUNUSED(row), int WXUNUSED(col),
			wxGridExt* WXUNUSED(grid),
			const wxGridExtActivationSource& WXUNUSED(actSource))
	{
		return wxGridExtActivationResult::DoEdit();
	}

	virtual
		void
		DoActivate(int WXUNUSED(row), int WXUNUSED(col), wxGridExt* WXUNUSED(grid))
	{
		wxFAIL_MSG("Must be overridden if TryActivate() is overridden");
	}

protected:
	// the dtor is private because only DecRef() can delete us
	virtual ~wxGridExtCellEditor();

	// Helper for the derived classes positioning the control according to the
	// attribute alignment if the desired control size is smaller than the cell
	// size, or centering it vertically if its size is bigger: this looks like
	// the best compromise when the editor control doesn't fit into the cell.
	void DoPositionEditor(const wxSize& size,
		const wxRect& rectCell,
		int hAlign = wxALIGN_LEFT,
		int vAlign = wxALIGN_CENTRE_VERTICAL);


	// the actual window we show on screen (this variable should actually be
	// named m_window, but m_control is kept for backward compatibility)
	wxWindow* m_control;

	// a temporary pointer to the attribute being edited
	wxGridExtCellAttr* m_attr;

	// if we change the colours/font of the control from the default ones, we
	// must restore the default later and we save them here between calls to
	// Show(true) and Show(false)
	wxColour m_colFgOld,
		m_colBgOld;
	wxFont m_fontOld;

	// suppress the stupid gcc warning about the class having private dtor and
	// no friends
	friend class wxGridExtCellEditorDummyFriend;
};

// Smart pointer to wxGridExtCellEditor, calling DecRef() on it automatically.
typedef wxObjectDataPtr<wxGridExtCellEditor> wxGridExtCellEditorPtr;

// Base class for editors that can be only activated and not edited normally.
class wxGridExtCellActivatableEditor : public wxGridExtCellEditor
{
public:
	wxGridExtCellActivatableEditor()
		: wxGridExtCellEditor()
	{
	}

	wxGridExtCellActivatableEditor(const wxGridExtCellActivatableEditor& other)
		: wxGridExtCellEditor(other)
	{
	}

	// In this class these methods must be overridden.
	virtual wxGridExtActivationResult
		TryActivate(int row, int col, wxGridExt* grid,
			const wxGridExtActivationSource& actSource) wxOVERRIDE = 0;
	virtual void DoActivate(int row, int col, wxGridExt* grid) wxOVERRIDE = 0;

	// All the other methods that normally must be implemented in an editor are
	// defined as just stubs below, as they should be never called.

	virtual void Create(wxWindow*, wxWindowID, wxEvtHandler*) wxOVERRIDE
	{
		wxFAIL;
	}
	virtual void BeginEdit(int, int, wxGridExt*) wxOVERRIDE
	{
		wxFAIL;
	}
	virtual bool EndEdit(int, int, const wxGridExt*,
		const wxString&, wxString*) wxOVERRIDE
	{
		wxFAIL; return false;
	}
	virtual void ApplyEdit(int, int, wxGridExt*) wxOVERRIDE
	{
		wxFAIL;
	}
	virtual void Reset() wxOVERRIDE
	{
		wxFAIL;
	}
	virtual wxString GetValue() const wxOVERRIDE
	{
		wxFAIL; return wxString();
	}
};

// ----------------------------------------------------------------------------
// wxGridExtHeaderRenderer and company: like wxGridExtCellRenderer but for headers
// ----------------------------------------------------------------------------

// Base class for header cells renderers.
class wxGridExtHeaderLabelsRenderer
{
public:
	virtual ~wxGridExtHeaderLabelsRenderer() {}

	// Draw the border around cell window.
	virtual void DrawBorder(const wxGridExt& grid,
		wxDC& dc,
		wxRect& rect) const = 0;

	// Draw header cell label
	virtual void DrawLabel(const wxGridExt& grid,
		wxDC& dc,
		const wxString& value,
		const wxRect& rect,
		int horizAlign,
		int vertAlign,
		int textOrientation) const;
};

// Currently the row/column/corner renders don't need any methods other than
// those already in wxGridExtHeaderLabelsRenderer but still define separate classes
// for them for future extensions and also for better type safety (i.e. to
// avoid inadvertently using a column header renderer for the row headers)
class wxGridExtRowHeaderRenderer
	: public wxGridExtHeaderLabelsRenderer
{
};

class wxGridExtColumnHeaderRenderer
	: public wxGridExtHeaderLabelsRenderer
{
};

class wxGridExtCornerHeaderRenderer
	: public wxGridExtHeaderLabelsRenderer
{
};

// Also define the default renderers which are used by wxGridExtCellAttrProvider
// by default
class wxGridExtRowHeaderRendererDefault
	: public wxGridExtRowHeaderRenderer
{
public:
	virtual void DrawBorder(const wxGridExt& grid,
		wxDC& dc,
		wxRect& rect) const wxOVERRIDE;
};

// Column header cells renderers
class wxGridExtColumnHeaderRendererDefault
	: public wxGridExtColumnHeaderRenderer
{
public:
	virtual void DrawBorder(const wxGridExt& grid,
		wxDC& dc,
		wxRect& rect) const wxOVERRIDE;
};

// Header corner renderer
class wxGridExtCornerHeaderRendererDefault
	: public wxGridExtCornerHeaderRenderer
{
public:
	virtual void DrawBorder(const wxGridExt& grid,
		wxDC& dc,
		wxRect& rect) const wxOVERRIDE;
};

// ----------------------------------------------------------------------------
// wxGridExtCellBorder: unit border of a cell in the grid
// ----------------------------------------------------------------------------

struct wxGridExtCellBorder {
	wxPenStyle m_style = wxPenStyle::wxPENSTYLE_TRANSPARENT;
	int m_width = 1;
	wxColour m_colour = *wxBLACK;
};

// ----------------------------------------------------------------------------
// Helper class used to define What should happen if the cell contents doesn't
// fit into its allotted space.
// ----------------------------------------------------------------------------

class wxGridExtFitMode
{
public:
	// Default ctor creates an object not specifying any particular behaviour.
	wxGridExtFitMode() : m_mode(Mode_Unset) {}

	// Static methods allowing to create objects actually specifying behaviour.
	static wxGridExtFitMode Clip() { return wxGridExtFitMode(Mode_Clip); }
	static wxGridExtFitMode Overflow() { return wxGridExtFitMode(Mode_Overflow); }
	static wxGridExtFitMode Ellipsize(wxEllipsizeMode ellipsize = wxELLIPSIZE_END)
	{
		// This cast works because the enum elements are the same, see below.
		return wxGridExtFitMode(static_cast<Mode>(ellipsize));
	}

	// Accessors.
	bool IsSpecified() const { return m_mode != Mode_Unset; }
	bool IsClip() const { return m_mode == Mode_Clip; }
	bool IsOverflow() const { return m_mode == Mode_Overflow; }

	wxEllipsizeMode GetEllipsizeMode() const
	{
		switch (m_mode)
		{
		case Mode_Unset:
		case Mode_EllipsizeStart:
		case Mode_EllipsizeMiddle:
		case Mode_EllipsizeEnd:
			return static_cast<wxEllipsizeMode>(m_mode);

		case Mode_Overflow:
		case Mode_Clip:
			break;
		}

		return wxELLIPSIZE_NONE;
	}

	// This one is used in the implementation only.
	static wxGridExtFitMode FromOverflowFlag(bool allow)
	{
		return allow ? Overflow() : Clip();
	}

private:
	enum Mode
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

	explicit wxGridExtFitMode(Mode mode) : m_mode(mode) {}

	Mode m_mode;
};

// ----------------------------------------------------------------------------
// wxGridExtCellAttr: this class can be used to alter the cells appearance in
// the grid by changing their colour/font/... from default. An object of this
// class may be returned by wxGridExtTable::GetAttr().
// ----------------------------------------------------------------------------

class wxGridExtCellAttr : public wxSharedClientDataContainer,
	public wxRefCounter
{
public:
	enum wxAttrKind
	{
		Any,
		Default,
		Cell,
		Row,
		Col,
		Merged
	};

	// default ctor
	explicit wxGridExtCellAttr(wxGridExtCellAttr* attrDefault = NULL)
	{
		Init(attrDefault);

		SetAlignment(wxALIGN_INVALID, wxALIGN_INVALID);
	}

	// ctor setting the most common attributes
	wxGridExtCellAttr(const wxColour& colText,
		const wxColour& colBack,
		const wxFont& font,
		int hAlign,
		int vAlign)
		: m_colText(colText), m_colBack(colBack), m_font(font)
	{
		Init();
		SetAlignment(hAlign, vAlign);
	}

	// creates a new copy of this object
	wxGridExtCellAttr* Clone() const;
	void MergeWith(wxGridExtCellAttr* mergefrom);

	// setters
	void SetTextColour(const wxColour& colText) { m_colText = colText; }
	void SetTextOrient(int orient) { m_orientText = orient; }
	void SetBackgroundColour(const wxColour& colBack) { m_colBack = colBack; }
	void SetFont(const wxFont& font) { m_font = font; }
	void SetAlignment(int hAlign, int vAlign)
	{
		m_hAlign = hAlign;
		m_vAlign = vAlign;
	}

	void SetSize(int num_rows, int num_cols);

	void SetBorderLeft(wxPenStyle style, const wxColour& colour, int width = 1);
	void SetBorderRight(wxPenStyle style, const wxColour& colour, int width = 1);
	void SetBorderTop(wxPenStyle style, const wxColour& colour, int width = 1);
	void SetBorderBottom(wxPenStyle style, const wxColour& colour, int width = 1);

	void SetFitMode(wxGridExtFitMode fitMode) { m_fitMode = fitMode; }
	void SetOverflow(bool allow = true)
	{
		SetFitMode(wxGridExtFitMode::FromOverflowFlag(allow));
	}
	void SetReadOnly(bool isReadOnly = true)
	{
		m_isReadOnly = isReadOnly ? ReadOnly : ReadWrite;
	}

	// takes ownership of the pointer
	void SetRenderer(wxGridExtCellRenderer* renderer)
	{
		wxSafeDecRef(m_renderer); m_renderer = renderer;
	}
	void SetEditor(wxGridExtCellEditor* editor)
	{
		wxSafeDecRef(m_editor); m_editor = editor;
	}

	void SetKind(wxAttrKind kind) { m_attrkind = kind; }

	// accessors
	bool HasTextColour() const { return m_colText.IsOk(); }
	bool HasTextOrient() const { return m_orientText != -1; }
	bool HasBackgroundColour() const { return m_colBack.IsOk(); }
	bool HasFont() const { return m_font.IsOk(); }
	bool HasAlignment() const
	{
		return m_hAlign != wxALIGN_INVALID || m_vAlign != wxALIGN_INVALID;
	}
	bool HasRenderer() const { return m_renderer != NULL; }
	bool HasEditor() const { return m_editor != NULL; }
	bool HasReadWriteMode() const { return m_isReadOnly != Unset; }
	bool HasOverflowMode() const { return m_fitMode.IsSpecified(); }
	bool HasSize() const { return m_sizeRows != 1 || m_sizeCols != 1; }

	bool HasAnyBorder() const { return HasBorderLeft() || HasBorderRight() || HasBorderRight() || HasBorderBottom(); }

	bool HasBorderLeft() const { return m_borderLeft.m_style != wxPenStyle::wxPENSTYLE_INVALID; }
	bool HasBorderRight() const { return m_borderRight.m_style != wxPenStyle::wxPENSTYLE_INVALID; }
	bool HasBorderTop() const { return m_borderTop.m_style != wxPenStyle::wxPENSTYLE_INVALID; }
	bool HasBorderBottom() const { return m_borderBottom.m_style != wxPenStyle::wxPENSTYLE_INVALID; }

	const wxColour& GetTextColour() const;
	const int GetTextOrient() const;
	const wxColour& GetBackgroundColour() const;
	const wxFont& GetFont() const;
	void GetAlignment(int* hAlign, int* vAlign) const;

	// unlike GetAlignment() which always overwrites its output arguments with
	// the alignment values to use, falling back on default alignment if this
	// attribute doesn't have any, this function will preserve the values of
	// parameters on entry if the corresponding alignment is not set in this
	// attribute meaning that they can be initialized to default alignment (and
	// also that they must be initialized, unlike with GetAlignment())
	void GetNonDefaultAlignment(int* hAlign, int* vAlign) const;

	wxGridExtCellBorder GetBorderLeft() const;
	wxGridExtCellBorder GetBorderRight() const;
	wxGridExtCellBorder GetBorderTop() const;
	wxGridExtCellBorder GetBorderBottom() const;

	void GetSize(int* num_rows, int* num_cols) const;
	wxGridExtFitMode GetFitMode() const;
	bool GetOverflow() const { return GetFitMode().IsOverflow(); }
	// whether the cell will draw the overflowed text to neighbour cells
	// currently only left aligned cells can overflow
	bool CanOverflow() const;

	wxGridExtCellRenderer* GetRenderer(const wxGridExt* grid, int row, int col) const;
	wxGridExtCellRendererPtr GetRendererPtr(const wxGridExt* grid, int row, int col) const
	{
		return wxGridExtCellRendererPtr(GetRenderer(grid, row, col));
	}

	wxGridExtCellEditor* GetEditor(const wxGridExt* grid, int row, int col) const;
	wxGridExtCellEditorPtr GetEditorPtr(const wxGridExt* grid, int row, int col) const
	{
		return wxGridExtCellEditorPtr(GetEditor(grid, row, col));
	}

	bool IsReadOnly() const { return m_isReadOnly == wxGridExtCellAttr::ReadOnly; }

	wxAttrKind GetKind() { return m_attrkind; }

	void SetDefAttr(wxGridExtCellAttr* defAttr) { m_defGridAttr = defAttr; }

protected:
	// the dtor is private because only DecRef() can delete us
	virtual ~wxGridExtCellAttr()
	{
		wxSafeDecRef(m_renderer);
		wxSafeDecRef(m_editor);
	}

private:
	enum wxAttrReadMode
	{
		Unset = -1,
		ReadWrite,
		ReadOnly
	};

	// the common part of all ctors
	void Init(wxGridExtCellAttr* attrDefault = NULL);


	wxColour m_colText,
		m_colBack;
	int m_orientText;
	wxFont   m_font;
	int      m_hAlign,
		m_vAlign;
	int      m_sizeRows,
		m_sizeCols;

	wxGridExtCellBorder m_borderLeft;
	wxGridExtCellBorder	m_borderRight;
	wxGridExtCellBorder	m_borderTop;
	wxGridExtCellBorder	m_borderBottom;

	wxGridExtFitMode m_fitMode;

	wxGridExtCellRenderer* m_renderer;
	wxGridExtCellEditor* m_editor;
	wxGridExtCellAttr* m_defGridAttr;

	wxAttrReadMode m_isReadOnly;

	wxAttrKind m_attrkind;

	// use Clone() instead
	wxDECLARE_NO_COPY_CLASS(wxGridExtCellAttr);

	// suppress the stupid gcc warning about the class having private dtor and
	// no friends
	friend class wxGridExtCellAttrDummyFriend;
};

// Smart pointer to wxGridExtCellAttr, calling DecRef() on it automatically.
typedef wxObjectDataPtr<wxGridExtCellAttr> wxGridExtCellAttrPtr;

// ----------------------------------------------------------------------------
// wxGridExtCellAttrProvider: class used by wxGridExtTableBase to retrieve/store the
// cell attributes.
// ----------------------------------------------------------------------------

// implementation note: we separate it from wxGridExtTableBase because we wish to
// avoid deriving a new table class if possible, and sometimes it will be
// enough to just derive another wxGridExtCellAttrProvider instead
//
// the default implementation is reasonably efficient for the generic case,
// but you might still wish to implement your own for some specific situations
// if you have performance problems with the stock one
class wxGridExtCellAttrProvider : public wxClientDataContainer
{
public:
	wxGridExtCellAttrProvider();
	virtual ~wxGridExtCellAttrProvider();

	// DecRef() must be called on the returned pointer
	virtual wxGridExtCellAttr* GetAttr(int row, int col,
		wxGridExtCellAttr::wxAttrKind  kind) const;

	// Helper returning smart pointer calling DecRef() automatically.
	wxGridExtCellAttrPtr GetAttrPtr(int row, int col,
		wxGridExtCellAttr::wxAttrKind  kind) const
	{
		return wxGridExtCellAttrPtr(GetAttr(row, col, kind));
	}

	// all these functions take ownership of the pointer, don't call DecRef()
	// on it
	virtual void SetAttr(wxGridExtCellAttr* attr, int row, int col);
	virtual void SetRowAttr(wxGridExtCellAttr* attr, int row);
	virtual void SetColAttr(wxGridExtCellAttr* attr, int col);

	// these functions must be called whenever some rows/cols are deleted
	// because the internal data must be updated then
	void UpdateAttrRows(size_t pos, int numRows);
	void UpdateAttrCols(size_t pos, int numCols);


	// get renderers for the given row/column header label and the corner
	// window: unlike cell renderers, these objects are not reference counted
	// and are never NULL so they are returned by reference
	virtual const wxGridExtColumnHeaderRenderer& GetColumnHeaderRenderer(int col);
	virtual const wxGridExtRowHeaderRenderer& GetRowHeaderRenderer(int row);
	virtual const wxGridExtCornerHeaderRenderer& GetCornerRenderer();

private:
	void InitData();

	wxGridExtCellAttrProviderData* m_data;

	wxDECLARE_NO_COPY_CLASS(wxGridExtCellAttrProvider);
};

// ----------------------------------------------------------------------------
// wxGridExtCellCoords: location of a cell in the grid
// ----------------------------------------------------------------------------

class wxGridExtCellCoords
{
public:
	wxGridExtCellCoords() { m_row = m_col = -1; }
	wxGridExtCellCoords(int r, int c) { m_row = r; m_col = c; }

	// default copy ctor is ok

	int GetRow() const { return m_row; }
	void SetRow(int n) { m_row = n; }
	int GetCol() const { return m_col; }
	void SetCol(int n) { m_col = n; }
	void Set(int row, int col) { m_row = row; m_col = col; }

	bool operator==(const wxGridExtCellCoords& other) const
	{
		return (m_row == other.m_row && m_col == other.m_col);
	}

	bool operator!=(const wxGridExtCellCoords& other) const
	{
		return (m_row != other.m_row || m_col != other.m_col);
	}

	bool operator!() const
	{
		return (m_row == -1 && m_col == -1);
	}

private:
	int m_row;
	int m_col;
};

// ----------------------------------------------------------------------------
// wxGridExtCellCache: location of a cell in the grid
// ----------------------------------------------------------------------------

struct wxGridExtCellCache
{
	wxGridExtCellCoords m_coords;
	wxRect m_rect;
	wxGridExtCellAttrPtr m_attr;
};

// ----------------------------------------------------------------------------
// wxGridExtBlockCoords: location of a block of cells in the grid
// ----------------------------------------------------------------------------

struct wxGridExtBlockDiffResult;

class wxGridExtBlockCoords
{
public:
	wxGridExtBlockCoords() :
		m_topRow(-1),
		m_leftCol(-1),
		m_bottomRow(-1),
		m_rightCol(-1)
	{
	}

	wxGridExtBlockCoords(int topRow, int leftCol, int bottomRow, int rightCol) :
		m_topRow(topRow),
		m_leftCol(leftCol),
		m_bottomRow(bottomRow),
		m_rightCol(rightCol)
	{
	}

	// default copy ctor is ok

	int GetTopRow() const { return m_topRow; }
	void SetTopRow(int row) { m_topRow = row; }
	int GetLeftCol() const { return m_leftCol; }
	void SetLeftCol(int col) { m_leftCol = col; }
	int GetBottomRow() const { return m_bottomRow; }
	void SetBottomRow(int row) { m_bottomRow = row; }
	int GetRightCol() const { return m_rightCol; }
	void SetRightCol(int col) { m_rightCol = col; }

	wxGridExtCellCoords GetTopLeft() const
	{
		return wxGridExtCellCoords(m_topRow, m_leftCol);
	}

	wxGridExtCellCoords GetBottomRight() const
	{
		return wxGridExtCellCoords(m_bottomRow, m_rightCol);
	}

	wxGridExtBlockCoords Canonicalize() const
	{
		wxGridExtBlockCoords result = *this;

		if (result.m_topRow > result.m_bottomRow)
			wxSwap(result.m_topRow, result.m_bottomRow);

		if (result.m_leftCol > result.m_rightCol)
			wxSwap(result.m_leftCol, result.m_rightCol);

		return result;
	}

	bool Intersects(const wxGridExtBlockCoords& other) const
	{
		return m_topRow <= other.m_bottomRow && m_bottomRow >= other.m_topRow &&
			m_leftCol <= other.m_rightCol && m_rightCol >= other.m_leftCol;
	}

	// Return whether this block contains the given cell.
	bool Contains(const wxGridExtCellCoords& cell) const
	{
		return m_topRow <= cell.GetRow() && cell.GetRow() <= m_bottomRow &&
			m_leftCol <= cell.GetCol() && cell.GetCol() <= m_rightCol;
	}

	// Return whether this blocks fully contains another one.
	bool Contains(const wxGridExtBlockCoords& other) const
	{
		return m_topRow <= other.m_topRow && other.m_bottomRow <= m_bottomRow &&
			m_leftCol <= other.m_leftCol && other.m_rightCol <= m_rightCol;
	}

	// Calculates the result blocks by subtracting the other block from this
	// block. splitOrientation can be wxVERTICAL or wxHORIZONTAL.
	wxGridExtBlockDiffResult
		Difference(const wxGridExtBlockCoords& other, int splitOrientation) const;

	// Calculates the symmetric difference of the blocks.
	wxGridExtBlockDiffResult
		SymDifference(const wxGridExtBlockCoords& other) const;

	bool operator==(const wxGridExtBlockCoords& other) const
	{
		return m_topRow == other.m_topRow && m_leftCol == other.m_leftCol &&
			m_bottomRow == other.m_bottomRow && m_rightCol == other.m_rightCol;
	}

	bool operator!=(const wxGridExtBlockCoords& other) const
	{
		return !(*this == other);
	}

	bool operator!() const
	{
		return m_topRow == -1 && m_leftCol == -1 &&
			m_bottomRow == -1 && m_rightCol == -1;
	}

private:
	int m_topRow;
	int m_leftCol;
	int m_bottomRow;
	int m_rightCol;
};

typedef wxVector<wxGridExtBlockCoords> wxGridExtBlockCoordsVector;

// ----------------------------------------------------------------------------
// wxGridExtBlockDiffResult: The helper struct uses as a result type for difference
// functions of wxGridExtBlockCoords class.
// Parts can be uninitialized (equals to wxGridExtNoBlockCoords), that means
// that the corresponding part doesn't exists in the result.
// ----------------------------------------------------------------------------

struct wxGridExtBlockDiffResult
{
	wxGridExtBlockCoords m_parts[4];
};

// ----------------------------------------------------------------------------
// wxGridExtCellArea: area of a cell in the grid
// ----------------------------------------------------------------------------

struct wxGridExtCellArea {

	wxString m_areaLabel;
	int m_start = -1, m_end = -1;
};

// ----------------------------------------------------------------------------
// wxGridExtDialogInputArea: dialog area of a cell in the grid
// ----------------------------------------------------------------------------

class wxGridExtDialogInputArea : public wxDialog {
public:

	wxGridExtDialogInputArea(wxWindow* parent, wxWindowID id = wxID_ANY)
		: wxDialog(parent, id, _("Identifier section"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
	{
		m_areaInput = new wxTextCtrl(this, wxID_ANY);

		m_sdbSizer = new wxStdDialogButtonSizer();
		m_sdbSizerOK = new wxButton(this, wxID_OK);
		m_sdbSizer->AddButton(m_sdbSizerOK);
		m_sdbSizerCancel = new wxButton(this, wxID_CANCEL);
		m_sdbSizer->AddButton(m_sdbSizerCancel);
		m_sdbSizer->Realize();

		wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
		mainSizer->Add(m_areaInput, 1, wxALL, 5);
		mainSizer->Add(m_sdbSizer, 1, wxEXPAND, 5);

		mainSizer->Fit(this);

		wxDialog::SetSizer(mainSizer);
		wxDialog::Layout();
		wxDialog::Centre(wxBOTH);
	}

	void SetAreaLabel(const wxString& areaLabel)
	{
		m_areaInput->SetValue(areaLabel);
	}

	wxString GetAreaLabel() const
	{
		return m_areaInput->GetValue();
	}

private:

	wxTextCtrl* m_areaInput;
	wxStdDialogButtonSizer* m_sdbSizer;
	wxButton* m_sdbSizerOK;
	wxButton* m_sdbSizerCancel;
};

// An array of cell coords...
//
WX_DECLARE_OBJARRAY_WITH_DECL(wxGridExtCellArea, wxGridExtCellAreaArray,
	class);

// ----------------------------------------------------------------------------
// wxGridExtBlocks: a range of grid blocks that can be iterated over
// ----------------------------------------------------------------------------

class wxGridExtBlocks
{
	typedef wxGridExtBlockCoordsVector::const_iterator iterator_impl;

public:
	class iterator
	{
	public:
#if wxUSE_STD_CONTAINERS_COMPATIBLY
		typedef std::forward_iterator_tag iterator_category;
#endif
		typedef ptrdiff_t difference_type;
		typedef wxGridExtBlockCoords value_type;
		typedef const value_type& reference;
		typedef const value_type* pointer;

		iterator() : m_it() {}

		reference operator*() const { return *m_it; }
		pointer operator->() const { return &*m_it; }

		iterator& operator++()
		{
			++m_it; return *this;
		}
		iterator operator++(int)
		{
			iterator tmp = *this; ++m_it; return tmp;
		}

		bool operator==(const iterator& it) const
		{
			return m_it == it.m_it;
		}
		bool operator!=(const iterator& it) const
		{
			return m_it != it.m_it;
		}

	private:
		explicit iterator(iterator_impl it) : m_it(it) {}

		iterator_impl m_it;

		friend class wxGridExtBlocks;
	};

	iterator begin() const
	{
		return m_begin;
	}

	iterator end() const
	{
		return m_end;
	}

private:
	wxGridExtBlocks() :
		m_begin(),
		m_end()
	{
	}

	wxGridExtBlocks(iterator_impl ibegin, iterator_impl iend) :
		m_begin(ibegin),
		m_end(iend)
	{
	}

	const iterator m_begin;
	const iterator m_end;

	friend class wxGridExt;
};

// For comparisons...
//
extern wxGridExtCellCoords  wxGridExtNoCellCoords;
extern wxGridExtBlockCoords wxGridExtNoBlockCoords;
extern wxRect wxGridExtNoCellRect;

// An array of cell coords...
//
WX_DECLARE_OBJARRAY_WITH_DECL(wxGridExtCellCoords, wxGridExtCellCoordsArray,
	class);

// An array of cell params...
//
WX_DECLARE_OBJARRAY_WITH_DECL(wxGridExtCellCache, wxGridExtCellCacheArray,
	class);

// ----------------------------------------------------------------------------
// Grid table classes
// ----------------------------------------------------------------------------

// the abstract base class
class wxGridExtTableBase : public wxObject,
	public wxClientDataContainer
{
public:
	wxGridExtTableBase();
	virtual ~wxGridExtTableBase();

	// You must override these functions in a derived table class
	//

	// return the number of rows and columns in this table
	virtual int GetNumberRows() = 0;
	virtual int GetNumberCols() = 0;

	// the methods above are unfortunately non-const even though they should
	// have been const -- but changing it now is not possible any longer as it
	// would break the existing code overriding them, so instead we provide
	// these const synonyms which can be used from const-correct code
	int GetRowsCount() const
	{
		return const_cast<wxGridExtTableBase*>(this)->GetNumberRows();
	}

	int GetColsCount() const
	{
		return const_cast<wxGridExtTableBase*>(this)->GetNumberCols();
	}

	virtual bool IsEmptyCell(int row, int col)
	{
		wxString result;
		GetValue(row, col, result);
		return result.IsEmpty();
	}

	bool IsEmpty(const wxGridExtCellCoords& coord)
	{
		return IsEmptyCell(coord.GetRow(), coord.GetCol());
	}

	wxString GetValue(int row, int col) {
		wxString result;
		GetValue(row, col, result);
		return result;
	}

	virtual void GetValue(int row, int col, wxString& value) = 0;
	virtual void SetValue(int row, int col, const wxString& value) = 0;

	// Data type determination and value access
	virtual bool GetTypeName(int row, int col, wxString& typeName);
	virtual bool CanGetValueAs(int row, int col, const wxString& typeName);
	virtual bool CanSetValueAs(int row, int col, const wxString& typeName);

	virtual long GetValueAsLong(int row, int col);
	virtual double GetValueAsDouble(int row, int col);
	virtual bool GetValueAsBool(int row, int col);

	virtual void SetValueAsLong(int row, int col, long value);
	virtual void SetValueAsDouble(int row, int col, double value);
	virtual void SetValueAsBool(int row, int col, bool value);

	// For user defined types
	virtual void* GetValueAsCustom(int row, int col, const wxString& typeName);
	virtual void  SetValueAsCustom(int row, int col, const wxString& typeName, void* value);


	// Overriding these is optional
	//
	virtual void SetView(wxGridExt* grid) { m_view = grid; }
	virtual wxGridExt* GetView() const { return m_view; }

	virtual void Clear() {}
	virtual bool InsertRows(size_t pos = 0, size_t numRows = 1);
	virtual bool AppendRows(size_t numRows = 1);
	virtual bool DeleteRows(size_t pos = 0, size_t numRows = 1);
	virtual bool InsertCols(size_t pos = 0, size_t numCols = 1);
	virtual bool AppendCols(size_t numCols = 1);
	virtual bool DeleteCols(size_t pos = 0, size_t numCols = 1);

	virtual wxString GetRowLabelValue(int row);
	virtual wxString GetColLabelValue(int col);
	virtual wxString GetCornerLabelValue() const;
	virtual void SetRowLabelValue(int WXUNUSED(row), const wxString&) {}
	virtual void SetColLabelValue(int WXUNUSED(col), const wxString&) {}
	virtual void SetCornerLabelValue(const wxString&) {}

	// Attribute handling
	//

	// give us the attr provider to use - we take ownership of the pointer
	void SetAttrProvider(wxGridExtCellAttrProvider* attrProvider);

	// get the currently used attr provider (may be NULL)
	wxGridExtCellAttrProvider* GetAttrProvider() const { return m_attrProvider; }

	// Does this table allow attributes?  Default implementation creates
	// a wxGridExtCellAttrProvider if necessary.
	virtual bool CanHaveAttributes();

	// by default forwarded to wxGridExtCellAttrProvider if any. May be
	// overridden to handle attributes directly in the table.
	virtual wxGridExtCellAttr* GetAttr(int row, int col,
		wxGridExtCellAttr::wxAttrKind  kind);

	wxGridExtCellAttrPtr GetAttrPtr(int row, int col,
		wxGridExtCellAttr::wxAttrKind  kind)
	{
		return wxGridExtCellAttrPtr(GetAttr(row, col, kind));
	}

	// This is an optimization for a common case when the entire column uses
	// roughly the same attribute, which can thus be reused for measuring all
	// the cells in this column. Override this to return true (possibly for
	// some columns only) to speed up AutoSizeColumns() for the grids using
	// this table.
	virtual bool CanMeasureColUsingSameAttr(int WXUNUSED(col)) const
	{
		return false;
	}

	// these functions take ownership of the pointer
	virtual void SetAttr(wxGridExtCellAttr* attr, int row, int col);
	virtual void SetRowAttr(wxGridExtCellAttr* attr, int row);
	virtual void SetColAttr(wxGridExtCellAttr* attr, int col);

private:
	wxGridExt* m_view;
	wxGridExtCellAttrProvider* m_attrProvider;

	wxDECLARE_ABSTRACT_CLASS(wxGridExtTableBase);
	wxDECLARE_NO_COPY_CLASS(wxGridExtTableBase);
};

// ----------------------------------------------------------------------------
// wxGridExtTableMessage
// ----------------------------------------------------------------------------

// IDs for messages sent from grid table to view
//
enum wxGridExtTableRequest
{
	// The first two requests never did anything, simply don't use them.
#if WXWIN_COMPATIBILITY_3_0
	wxGRIDTABLE_REQUEST_VIEW_GET_VALUES = 2000,
	wxGRIDTABLE_REQUEST_VIEW_SEND_VALUES,
#endif // WXWIN_COMPATIBILITY_3_0
	wxGRIDTABLE_NOTIFY_ROWS_INSERTED = 2002,
	wxGRIDTABLE_NOTIFY_ROWS_APPENDED,
	wxGRIDTABLE_NOTIFY_ROWS_DELETED,
	wxGRIDTABLE_NOTIFY_COLS_INSERTED,
	wxGRIDTABLE_NOTIFY_COLS_APPENDED,
	wxGRIDTABLE_NOTIFY_COLS_DELETED
};

class wxGridExtTableMessage
{
public:
	wxGridExtTableMessage();
	wxGridExtTableMessage(wxGridExtTableBase* table, int id,
		int comInt1 = -1,
		int comInt2 = -1);

	void SetTableObject(wxGridExtTableBase* table) { m_table = table; }
	wxGridExtTableBase* GetTableObject() const { return m_table; }
	void SetId(int id) { m_id = id; }
	int GetId() const { return m_id; }
	void SetCommandInt(int comInt1) { m_comInt1 = comInt1; }
	int GetCommandInt() const { return m_comInt1; }
	void SetCommandInt2(int comInt2) { m_comInt2 = comInt2; }
	int GetCommandInt2() const { return m_comInt2; }

private:
	wxGridExtTableBase* m_table;
	int m_id;
	int m_comInt1;
	int m_comInt2;

	wxDECLARE_NO_COPY_CLASS(wxGridExtTableMessage);
};



// ------ wxGridExtStringArray
// A 2-dimensional array of strings for data values
//

WX_DECLARE_OBJARRAY_WITH_DECL(wxArrayString, wxGridExtStringArray,
	class);



// ------ wxGridExtStringTable
//
// Simplest type of data table for a grid for small tables of strings
// that are stored in memory
//

class wxGridExtStringTable : public wxGridExtTableBase
{
public:
	wxGridExtStringTable();
	wxGridExtStringTable(int numRows, int numCols);

	// these are pure virtual in wxGridExtTableBase
	//
	virtual int GetNumberRows() wxOVERRIDE { return static_cast<int>(m_data.size()); }
	virtual int GetNumberCols() wxOVERRIDE { return m_numCols; }

	virtual bool IsEmptyCell(int row, int col)
	{
		wxCHECK_MSG((row >= 0 && row < GetNumberRows()) &&
			(col >= 0 && col < GetNumberCols()),
			wxEmptyString,
			wxT("invalid row or column index in wxGridExtStringTable"));

		return m_data[row][col].IsEmpty();
	}

	virtual void GetValue(int row, int col, wxString& s) wxOVERRIDE;
	virtual void SetValue(int row, int col, const wxString& s) wxOVERRIDE;

	// overridden functions from wxGridExtTableBase
	//
	void Clear() wxOVERRIDE;
	bool InsertRows(size_t pos = 0, size_t numRows = 1) wxOVERRIDE;
	bool AppendRows(size_t numRows = 1) wxOVERRIDE;
	bool DeleteRows(size_t pos = 0, size_t numRows = 1) wxOVERRIDE;
	bool InsertCols(size_t pos = 0, size_t numCols = 1) wxOVERRIDE;
	bool AppendCols(size_t numCols = 1) wxOVERRIDE;
	bool DeleteCols(size_t pos = 0, size_t numCols = 1) wxOVERRIDE;

	void SetRowLabelValue(int row, const wxString&) wxOVERRIDE;
	void SetColLabelValue(int col, const wxString&) wxOVERRIDE;
	void SetCornerLabelValue(const wxString&) wxOVERRIDE;
	wxString GetRowLabelValue(int row) wxOVERRIDE;
	wxString GetColLabelValue(int col) wxOVERRIDE;
	wxString GetCornerLabelValue() const wxOVERRIDE;

protected:
	wxGridExtStringArray m_data;
private:

	// notice that while we don't need to store the number of our rows as it's
	// always equal to the size of m_data array, we do need to store the number
	// of our columns as we can't retrieve it from m_data when the number of
	// rows is 0 (see #10818)
	int m_numCols;

	// These only get used if you set your own labels, otherwise the
	// GetRow/ColLabelValue functions return wxGridExtTableBase defaults
	//
	wxArrayString     m_rowLabels;
	wxArrayString     m_colLabels;

	wxString m_cornerLabel;

	wxDECLARE_DYNAMIC_CLASS_NO_COPY(wxGridExtStringTable);
};



// ============================================================================
//  Grid view classes
// ============================================================================

// ----------------------------------------------------------------------------
// wxGridExtSizesInfo stores information about sizes of the rows or columns.
//
// It assumes that most of the columns or rows have default size and so stores
// the default size separately and uses a hash to map column or row numbers to
// their non default size for those which don't have the default size.
// ----------------------------------------------------------------------------

// hash map to store positions as the keys and sizes as the values
WX_DECLARE_HASH_MAP_WITH_DECL(unsigned, int, wxIntegerHash, wxIntegerEqual,
	wxUnsignedToIntHashMap, class);

struct wxGridExtSizesInfo
{
	// default ctor, initialize m_sizeDefault and m_customSizes later
	wxGridExtSizesInfo() {}

	// ctor used by wxGridExt::Get{Col,Row}Sizes()
	wxGridExtSizesInfo(int defSize, const wxArrayInt& allSizes);

	// default copy ctor, assignment operator and dtor are ok

	// Get the size of the element with the given index
	int GetSize(unsigned pos) const;


	// default size
	int m_sizeDefault;

	// position -> size map containing all elements with non-default size
	wxUnsignedToIntHashMap m_customSizes;
};

// ----------------------------------------------------------------------------
// wxGridExt
// ----------------------------------------------------------------------------

class wxGridExt : public wxScrolledCanvas
{
public:
	// possible selection modes
	enum wxGridExtSelectionModes
	{
		wxGridExtSelectCells = 0,  // allow selecting anything
		wxGridExtSelectRows = 1,  // allow selecting only entire rows
		wxGridExtSelectColumns = 2,  // allow selecting only entire columns
		wxGridExtSelectRowsOrColumns = wxGridExtSelectRows | wxGridExtSelectColumns,
		wxGridExtSelectNone = 4   // disallow selecting anything
	};

	// Different behaviour of the TAB key when the end (or the beginning, for
	// Shift-TAB) of the current row is reached:
	enum TabBehaviour
	{
		Tab_Stop,   // Do nothing, this is default.
		Tab_Wrap,   // Move to the next (or previous) row.
		Tab_Leave   // Move to the next (or previous) control.
	};

	// creation and destruction
	// ------------------------

	// ctor and Create() create the grid window, as with the other controls
	wxGridExt() { Init(); }

	wxGridExt(wxWindow* parent,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxWANTS_CHARS,
		const wxString& name = wxASCII_STR(wxGridExtNameStr))
	{
		Init();

		Create(parent, id, pos, size, style, name);
	}

	bool Create(wxWindow* parent,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxWANTS_CHARS,
		const wxString& name = wxASCII_STR(wxGridExtNameStr));

	virtual ~wxGridExt();

	// however to initialize grid data either CreateGrid() (to use a simple
	// default table class) or {Set,Assign}Table() (to use a custom table) must
	// be also called

	// this is basically equivalent to
	//
	//   AssignTable(new wxGridExtStringTable(numRows, numCols), selmode)
	//
	bool CreateGrid(int numRows, int numCols,
		wxGridExtSelectionModes selmode = wxGridExtSelectCells);

	bool SetTable(wxGridExtTableBase* table,
		bool takeOwnership = false,
		wxGridExtSelectionModes selmode = wxGridExtSelectCells);

	void AssignTable(wxGridExtTableBase* table,
		wxGridExtSelectionModes selmode = wxGridExtSelectCells);

	bool ProcessTableMessage(wxGridExtTableMessage&);

	wxGridExtTableBase* GetTable() const { return m_table; }

	void SetSelectionMode(wxGridExtSelectionModes selmode);
	wxGridExtSelectionModes GetSelectionMode() const;

	// ------ grid dimensions
	//
	int      GetNumberRows() const { return  m_numRows; }
	int      GetNumberCols() const { return  m_numCols; }

	int      GetNumberFrozenRows() const { return m_numFrozenRows; }
	int      GetNumberFrozenCols() const { return m_numFrozenCols; }

	// ------ display update functions
	//
	wxArrayInt CalcRowLabelsExposed(const wxRegion& reg,
		wxGridExtWindow* gridWindow = NULL) const;
	wxArrayInt CalcColLabelsExposed(const wxRegion& reg,
		wxGridExtWindow* gridWindow = NULL) const;
	void CalcCellsExposed(const wxRegion& reg, wxGridExtCellCoordsArray& cellsExposed,
		wxGridExtWindow* gridWindow = NULL) const;

	void PrepareDCFor(wxDC& dc, wxGridExtWindow* gridWindow);

	void ClearGrid();
	bool InsertRows(int pos = 0, int numRows = 1, bool updateLabels = true)
	{
		return DoModifyLines(&wxGridExtTableBase::InsertRows,
			pos, numRows, updateLabels);
	}
	bool InsertCols(int pos = 0, int numCols = 1, bool updateLabels = true)
	{
		return DoModifyLines(&wxGridExtTableBase::InsertCols,
			pos, numCols, updateLabels);
	}

	bool AppendRows(int numRows = 1, bool updateLabels = true)
	{
		return DoAppendLines(&wxGridExtTableBase::AppendRows, numRows, updateLabels);
	}
	bool AppendCols(int numCols = 1, bool updateLabels = true)
	{
		return DoAppendLines(&wxGridExtTableBase::AppendCols, numCols, updateLabels);
	}

	bool DeleteRows(int pos = 0, int numRows = 1, bool updateLabels = true)
	{
		return DoModifyLines(&wxGridExtTableBase::DeleteRows,
			pos, numRows, updateLabels);
	}
	bool DeleteCols(int pos = 0, int numCols = 1, bool updateLabels = true)
	{
		return DoModifyLines(&wxGridExtTableBase::DeleteCols,
			pos, numCols, updateLabels);
	}

	bool FreezeTo(int row, int col);
	bool FreezeTo(const wxGridExtCellCoords& coords)
	{
		return FreezeTo(coords.GetRow(), coords.GetCol());
	}

	bool IsFrozen() const;

	void DrawGridCellArea(wxDC& dc, const wxGridExtCellCoordsArray& cells, wxGridExtCellCacheArray& storage = wxGridExtCellCacheArray());
	void DrawGridSpace(wxDC& dc, wxGridExtWindow* gridWindow);
	void DrawAllGridLines();
	void DrawAllGridWindowLines(wxDC& dc, const wxRegion& reg, wxGridExtWindow* gridWindow);
	void DrawCell(wxDC& dc, const wxGridExtCellCoords&, wxGridExtCellCache& param = wxGridExtCellCache());
	void DrawBorder(wxDC& dc, const wxGridExtCellCacheArray& storage);
	void DrawHighlight(wxDC& dc, const wxGridExtCellCoordsArray& cells);
	void DrawFrozenBorder(wxDC& dc, wxGridExtWindow* gridWindow);
	void DrawLabelFrozenBorder(wxDC& dc, wxWindow* window, bool isRow);

	void ScrollWindow(int dx, int dy, const wxRect* rect) wxOVERRIDE;

	void UpdateGridWindows() const;

	// this function is called when the current cell highlight must be redrawn
	// and may be overridden by the user
	virtual void DrawCellBorder(wxDC& dc, const wxGridExtCellCoords& coords, const wxRect& rect, const wxGridExtCellAttr* attr);
	virtual void DrawCellHighlight(wxDC& dc, int row, int col, const wxGridExtCellAttr* attr);

	virtual void DrawRowAreas(wxDC& dc, const wxArrayInt& rows);
	virtual void DrawRowArea(wxDC& dc, int row);

	virtual void DrawRowLabels(wxDC& dc, const wxArrayInt& rows);
	virtual void DrawRowLabel(wxDC& dc, int row);

	virtual void DrawColAreas(wxDC& dc, const wxArrayInt& cols);
	virtual void DrawColArea(wxDC& dc, int col);

	virtual void DrawColLabels(wxDC& dc, const wxArrayInt& cols);
	virtual void DrawColLabel(wxDC& dc, int col);

	virtual void DrawCornerLabel(wxDC& dc);

	// ------ Cell text drawing functions
	//
	static void DrawTextRectangle(wxDC& dc, const wxString&, const wxRect&,
		int horizontalAlignment = wxALIGN_LEFT,
		int verticalAlignment = wxALIGN_TOP,
		int textOrientation = wxHORIZONTAL);

	static void DrawTextRectangle(wxDC& dc, const wxArrayString& lines, const wxRect&,
		int horizontalAlignment = wxALIGN_LEFT,
		int verticalAlignment = wxALIGN_TOP,
		int textOrientation = wxHORIZONTAL);

	static void DrawTextRectangle(wxDC& dc,
		const wxString& text,
		const wxRect& rect,
		const wxGridExtCellAttr& attr,
		int defaultHAlign = wxALIGN_INVALID,
		int defaultVAlign = wxALIGN_INVALID);

	// ------ grid render function for printing
	//
	void Render(wxDC& dc,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		const wxGridExtCellCoords& topLeft = wxGridExtCellCoords(-1, -1),
		const wxGridExtCellCoords& bottomRight = wxGridExtCellCoords(-1, -1),
		int style = wxGRID_DRAW_DEFAULT);

	// Split a string containing newline characters into an array of
	// strings and return the number of lines
	//
	static void ParseLines(const wxString& value, wxArrayString& lines);
	static void GetTextBoxSize(const wxDC& dc,
		const wxArrayString& lines,
		wxArrayInt* arrRow, wxArrayInt* arrCol,
		long* width, long* height);

	static void GetTextBoxSize(const wxDC& dc,
		const wxArrayString& lines,
		long* width, long* height)
	{
		GetTextBoxSize(dc, lines, NULL, NULL, width, height);
	}

	// If bottomRight is invalid, i.e. == wxGridExtNoCellCoords, it defaults to
	// topLeft. If topLeft itself is invalid, the function simply returns.
	void RefreshBlock(const wxGridExtCellCoords& topLeft,
		const wxGridExtCellCoords& bottomRight);

	void RefreshBlock(int topRow, int leftCol,
		int bottomRow, int rightCol);

	// ------
	// Code that does a lot of grid modification can be enclosed
	// between BeginBatch() and EndBatch() calls to avoid screen
	// flicker
	//
	void     BeginBatch() { m_batchCount++; }
	void     EndBatch();

	int      GetBatchCount() const { return m_batchCount; }

	virtual void Refresh(bool eraseb = true, const wxRect* rect = NULL) wxOVERRIDE;

	// Use this, rather than wxWindow::Refresh(), to force an
	// immediate repainting of the grid. Has no effect if you are
	// already inside a BeginBatch / EndBatch block.
	//
	// This function is necessary because wxGridExt has a minimal OnPaint()
	// handler to reduce screen flicker.
	//
	void     ForceRefresh();


	// ------ edit control functions
	//
	bool IsEditable() const { return m_editable; }
	void EnableEditing(bool edit);

	void EnableCellEditControl(bool enable = true);
	void DisableCellEditControl() { EnableCellEditControl(false); }
	bool CanEnableCellControl() const;
	bool IsCellEditControlEnabled() const { return m_cellEditCtrlEnabled; }
	bool IsCellEditControlShown() const;

	bool IsCurrentCellReadOnly() const;

	void ShowCellEditControl(); // Use EnableCellEditControl() instead.
	void HideCellEditControl();
	void SaveEditControlValue();


	// ------ grid location functions
	//  Note that all of these functions work with the logical coordinates of
	//  grid cells and labels so you will need to convert from device
	//  coordinates for mouse events etc.
	//
	wxGridExtCellCoords XYToCell(int x, int y, wxGridExtWindow* gridWindow = NULL) const;
	void XYToCell(int x, int y,
		wxGridExtCellCoords& coords,
		wxGridExtWindow* gridWindow = NULL) const
	{
		coords = XYToCell(x, y, gridWindow);
	}

	wxGridExtCellCoords XYToCell(const wxPoint& pos,
		wxGridExtWindow* gridWindow = NULL) const
	{
		return XYToCell(pos.x, pos.y, gridWindow);
	}

	// these functions return the index of the row/columns corresponding to the
	// given logical position in pixels
	//
	// if clipToMinMax is false (default, wxNOT_FOUND is returned if the
	// position is outside any row/column, otherwise the first/last element is
	// returned in this case
	int  YToRow(int y, bool clipToMinMax = false, wxGridExtWindow* gridWindow = NULL) const;
	int  XToCol(int x, bool clipToMinMax = false, wxGridExtWindow* gridWindow = NULL) const;

	int  YToEdgeOfRow(int y) const;
	int  XToEdgeOfCol(int x) const;

	wxRect CellToRect(int row, int col) const;
	wxRect CellToRect(const wxGridExtCellCoords& coords) const
	{
		return CellToRect(coords.GetRow(), coords.GetCol());
	}

	wxGridExtWindow* CellToGridWindow(int row, int col) const;
	wxGridExtWindow* CellToGridWindow(const wxGridExtCellCoords& coords) const
	{
		return CellToGridWindow(coords.GetRow(), coords.GetCol());
	}

	const wxGridExtCellCoords& GetGridCursorCoords() const
	{
		return m_currentCellCoords;
	}

	int  GetGridCursorRow() const { return m_currentCellCoords.GetRow(); }
	int  GetGridCursorCol() const { return m_currentCellCoords.GetCol(); }

	void    GetGridWindowOffset(const wxGridExtWindow* gridWindow, int& x, int& y) const;
	wxPoint GetGridWindowOffset(const wxGridExtWindow* gridWindow) const;

	wxGridExtWindow* DevicePosToGridWindow(wxPoint pos) const;
	wxGridExtWindow* DevicePosToGridWindow(int x, int y) const;

	void    CalcGridWindowUnscrolledPosition(int x, int y, int* xx, int* yy,
		const wxGridExtWindow* gridWindow) const;
	wxPoint CalcGridWindowUnscrolledPosition(const wxPoint& pt,
		const wxGridExtWindow* gridWindow) const;

	void    CalcGridWindowScrolledPosition(int x, int y, int* xx, int* yy,
		const wxGridExtWindow* gridWindow) const;
	wxPoint CalcGridWindowScrolledPosition(const wxPoint& pt,
		const wxGridExtWindow* gridWindow) const;

	// check to see if a cell is either wholly visible (the default arg) or
	// at least partially visible in the grid window
	//
	bool IsVisible(int row, int col, bool wholeCellVisible = true) const;
	bool IsVisible(const wxGridExtCellCoords& coords, bool wholeCellVisible = true) const
	{
		return IsVisible(coords.GetRow(), coords.GetCol(), wholeCellVisible);
	}
	void MakeCellVisible(int row, int col);
	void MakeCellVisible(const wxGridExtCellCoords& coords)
	{
		MakeCellVisible(coords.GetRow(), coords.GetCol());
	}

	// Returns the topmost row of the current visible area.
	int GetFirstFullyVisibleRow() const;
	// Returns the leftmost column of the current visible area.
	int GetFirstFullyVisibleColumn() const;

	// ------ grid cursor movement functions
	//
	void SetGridCursor(int row, int col) { SetCurrentCell(row, col); }
	void SetGridCursor(const wxGridExtCellCoords& c) { SetCurrentCell(c); }

	void GoToCell(int row, int col)
	{
		if (SetCurrentCell(row, col))
			MakeCellVisible(row, col);
	}

	void GoToCell(const wxGridExtCellCoords& coords)
	{
		if (SetCurrentCell(coords))
			MakeCellVisible(coords);
	}

	bool MoveCursorUp(bool expandSelection);
	bool MoveCursorDown(bool expandSelection);
	bool MoveCursorLeft(bool expandSelection);
	bool MoveCursorRight(bool expandSelection);
	bool MovePageDown();
	bool MovePageUp();
	bool MoveCursorUpBlock(bool expandSelection);
	bool MoveCursorDownBlock(bool expandSelection);
	bool MoveCursorLeftBlock(bool expandSelection);
	bool MoveCursorRightBlock(bool expandSelection);

	void SetTabBehaviour(TabBehaviour behaviour) { m_tabBehaviour = behaviour; }


	// ------ label and gridline formatting
	//
	int      GetDefaultRowLabelSize() const { return FromDIP(WXGRID_DEFAULT_ROW_LABEL_WIDTH); }
	int      GetRowLabelSize() const { return m_rowLabelWidth; }
	int      GetDefaultColLabelSize() const { return FromDIP(WXGRID_DEFAULT_COL_LABEL_HEIGHT); }
	int      GetColLabelSize() const { return m_colLabelHeight; }
	wxColour GetLabelBackgroundColour() const { return m_labelBackgroundColour; }
	wxColour GetLabelTextColour() const { return m_labelTextColour; }
	wxFont   GetLabelFont() const { return m_labelFont; }
	void     GetRowLabelAlignment(int* horiz, int* vert) const;
	void     GetColLabelAlignment(int* horiz, int* vert) const;
	void     GetCornerLabelAlignment(int* horiz, int* vert) const;
	int      GetColLabelTextOrientation() const;
	int      GetCornerLabelTextOrientation() const;

	int		 GetRowAreaValue(int row, wxString* areaLabel = nullptr) const;
	wxString GetRowLabelValue(int row) const;
	int		 GetColAreaValue(int col, wxString* areaLabel = nullptr) const;
	wxString GetColLabelValue(int col) const;
	wxString GetCornerLabelValue() const;

	wxColour GetCellHighlightColour() const { return m_cellHighlightColour; }
	int      GetCellHighlightPenWidth() const { return m_cellHighlightPenWidth; }
	int      GetCellHighlightROPenWidth() const { return m_cellHighlightROPenWidth; }
	wxColor  GetGridFrozenBorderColour() const { return m_gridFrozenBorderColour; }
	int      GetGridFrozenBorderPenWidth() const { return m_gridFrozenBorderPenWidth; }

	// this one will use wxHeaderCtrl for the column labels
	bool UseNativeColHeader(bool native = true);

	// this one will still draw them manually but using the native renderer
	// instead of using the same appearance as for the row labels
	void SetUseNativeColLabels(bool native = true);

	void     SetRowLabelSize(int width);
	void     SetColLabelSize(int height);
	void     HideRowLabels() { SetRowLabelSize(0); }
	void     HideColLabels() { SetColLabelSize(0); }
	void     SetLabelBackgroundColour(const wxColour&);
	void     SetLabelTextColour(const wxColour&);
	void     SetLabelFont(const wxFont&);
	void     SetRowLabelAlignment(int horiz, int vert);
	void     SetColLabelAlignment(int horiz, int vert);
	void     SetCornerLabelAlignment(int horiz, int vert);
	void     SetColLabelTextOrientation(int textOrientation);
	void     SetCornerLabelTextOrientation(int textOrientation);
	void     SetRowLabelValue(int row, const wxString&);
	void     SetColLabelValue(int col, const wxString&);
	void     SetCornerLabelValue(const wxString&);
	void     SetCellHighlightColour(const wxColour&);
	void     SetCellHighlightPenWidth(int width);
	void     SetCellHighlightROPenWidth(int width);
	void     SetGridFrozenBorderColour(const wxColour&);
	void     SetGridFrozenBorderPenWidth(int width);

	// interactive grid mouse operations control
	// -----------------------------------------

	// functions globally enabling row/column interactive resizing (enabled by
	// default)
	void     EnableDragRowSize(bool enable = true);
	void     DisableDragRowSize() { EnableDragRowSize(false); }

	void     EnableDragColSize(bool enable = true);
	void     DisableDragColSize() { EnableDragColSize(false); }

	// if interactive resizing is enabled, some rows/columns can still have
	// fixed size
	void DisableRowResize(int row) { DoDisableLineResize(row, m_setFixedRows); }
	void DisableColResize(int col) { DoDisableLineResize(col, m_setFixedCols); }

	// These function return true if resizing rows/columns by dragging
	// their edges inside the grid is enabled. Note that this doesn't cover
	// dragging their separators in the label windows (which can be enabled
	// for the columns even if dragging inside the grid is not), nor checks
	// whether a particular row/column is resizeable or not, so you should
	// always check CanDrag{Row,Col}Size() below too.
	bool CanDragGridRowEdges() const
	{
		return m_canDragGridSize && m_canDragRowSize;
	}

	bool CanDragGridColEdges() const
	{
		// When using the native header window we can only resize the columns by
		// dragging the dividers in the header itself, but not by dragging them
		// in the grid because we can't make the native control enter into the
		// column resizing mode programmatically.
		return m_canDragGridSize && m_canDragColSize && !m_useNativeHeader;
	}

	// These functions return whether the given row/column can be
	// effectively resized: for this interactive resizing must be enabled
	// and this index must not have been passed to DisableRow/ColResize()
	bool CanDragRowSize(int row) const
	{
		return m_canDragRowSize && DoCanResizeLine(row, m_setFixedRows);
	}
	bool CanDragColSize(int col) const
	{
		return m_canDragColSize && DoCanResizeLine(col, m_setFixedCols);
	}

	// interactive row reordering (disabled by default)
	bool     EnableDragRowMove(bool enable = true);
	void     DisableDragRowMove() { EnableDragRowMove(false); }
	bool     CanDragRowMove() const { return m_canDragRowMove; }

	// interactive column reordering (disabled by default)
	bool     EnableDragColMove(bool enable = true);
	void     DisableDragColMove() { EnableDragColMove(false); }
	bool     CanDragColMove() const { return m_canDragColMove; }

	// interactive column hiding (enabled by default, works only for native header)
	bool     EnableHidingColumns(bool enable = true);
	void     DisableHidingColumns() { EnableHidingColumns(false); }
	bool     CanHideColumns() const { return m_canHideColumns; }

	// interactive resizing of grid cells (enabled by default)
	void     EnableDragGridSize(bool enable = true);
	void     DisableDragGridSize() { EnableDragGridSize(false); }
	bool     CanDragGridSize() const { return m_canDragGridSize; }

	// interactive dragging of cells (disabled by default)
	void     EnableDragCell(bool enable = true);
	void     DisableDragCell() { EnableDragCell(false); }
	bool     CanDragCell() const { return m_canDragCell; }

	// grid areas
	// ----------

	// enable or disable drawing of the lines
	void EnableGridArea(bool enable = true)
	{
		m_areaEnabled = enable;
		CalcDimensions();
	}

	bool GridRowAreaEnabled() const {
		return m_areaEnabled && m_rowAreaAt.Count() > 0;
	}

	bool GridColAreaEnabled() const {
		return m_areaEnabled && m_colAreaAt.Count() > 0;
	}

	// grid lines
	// ----------

	// enable or disable drawing of the lines
	void EnableGridLines(bool enable = true);
	bool GridLinesEnabled() const { return m_gridLinesEnabled; }

	// by default grid lines stop at last column/row, but this may be changed
	void ClipHorzGridLines(bool clip)
	{
		DoClipGridLines(m_gridLinesClipHorz, clip);
	}
	void ClipVertGridLines(bool clip)
	{
		DoClipGridLines(m_gridLinesClipVert, clip);
	}
	bool AreHorzGridLinesClipped() const { return m_gridLinesClipHorz; }
	bool AreVertGridLinesClipped() const { return m_gridLinesClipVert; }

	// this can be used to change the global grid lines colour
	void SetGridLineColour(const wxColour& col);
	wxColour GetGridLineColour() const { return m_gridLineColour; }

	// these methods may be overridden to customize individual grid lines
	// appearance
	virtual wxPen GetDefaultGridLinePen();
	virtual wxPen GetRowGridLinePen(int row);
	virtual wxPen GetColGridLinePen(int col);


	// attributes
	// ----------

	// this sets the specified attribute for this cell or in this row/col
	void     SetAttr(int row, int col, wxGridExtCellAttr* attr);
	void     SetRowAttr(int row, wxGridExtCellAttr* attr);
	void     SetColAttr(int col, wxGridExtCellAttr* attr);

	// the grid can cache attributes for the recently used cells (currently it
	// only caches one attribute for the most recently used one) and might
	// notice that its value in the attribute provider has changed -- if this
	// happens, call this function to force it
	void RefreshAttr(int row, int col);

	// returns the attribute we may modify in place: a new one if this cell
	// doesn't have any yet or the existing one if it does
	//
	// DecRef() must be called on the returned pointer, as usual
	wxGridExtCellAttr* GetOrCreateCellAttr(int row, int col) const;

	wxGridExtCellAttrPtr GetOrCreateCellAttrPtr(int row, int col) const
	{
		return wxGridExtCellAttrPtr(GetOrCreateCellAttr(row, col));
	}


	// shortcuts for setting the column parameters

	// set the format for the data in the column: default is string
	void     SetColFormatBool(int col);
	void     SetColFormatNumber(int col);
	void     SetColFormatFloat(int col, int width = -1, int precision = -1);
	void     SetColFormatDate(int col, const wxString& format = wxString());
	void     SetColFormatCustom(int col, const wxString& typeName);

	// ------ row and col formatting
	//
	int			GetDefaultRowSize() const;
	int			GetRowSize(int row) const;
	bool		IsRowShown(int row) const { return GetRowSize(row) != 0; }
	int			GetDefaultColSize() const;
	int			GetColSize(int col) const;
	bool		IsColShown(int col) const { return GetColSize(col) != 0; }
	wxColour	GetDefaultCellBackgroundColour() const;
	wxColour	GetCellBackgroundColour(int row, int col) const;
	wxColour	GetDefaultCellTextColour() const;
	wxColour	GetCellTextColour(int row, int col) const;
	int			GetCellTextOrient(int row, int col) const;
	wxFont		GetDefaultCellFont() const;
	wxFont		GetCellFont(int row, int col) const;
	void		GetDefaultCellAlignment(int* horiz, int* vert) const;
	void		GetCellAlignment(int row, int col, int* horiz, int* vert) const;

	wxGridExtCellBorder	GetDefaultCellBorderLeft() const;
	wxGridExtCellBorder	GetCellBorderLeft(int row, int col) const;
	wxGridExtCellBorder	GetDefaultCellBorderRight() const;
	wxGridExtCellBorder	GetCellBorderRight(int row, int col) const;
	wxGridExtCellBorder	GetDefaultCellBorderTop() const;
	wxGridExtCellBorder	GetCellBorderTop(int row, int col) const;
	wxGridExtCellBorder	GetDefaultCellBorderBottom() const;
	wxGridExtCellBorder	GetCellBorderBottom(int row, int col) const;

	wxGridExtFitMode GetDefaultCellFitMode() const;
	wxGridExtFitMode GetCellFitMode(int row, int col) const;

	wxGridExtCellArea* GetRowArea(int row) const;
	wxGridExtCellArea* GetColArea(int col) const;

	bool     GetDefaultCellOverflow() const
	{
		return GetDefaultCellFitMode().IsOverflow();
	}
	bool     GetCellOverflow(int row, int col) const
	{
		return GetCellFitMode(row, col).IsOverflow();
	}

	// this function returns 1 in num_rows and num_cols for normal cells,
	// positive numbers for a cell spanning multiple columns/rows (as set with
	// SetCellSize()) and _negative_ numbers corresponding to the offset of the
	// top left cell of the span from this one for the other cells covered by
	// this cell
	//
	// the return value is CellSpan_None, CellSpan_Main or CellSpan_Inside for
	// each of these cases respectively
	enum CellSpan
	{
		CellSpan_Inside = -1,
		CellSpan_None = 0,
		CellSpan_Main
	};

	CellSpan GetCellSize(int row, int col, int* num_rows, int* num_cols) const;

	wxSize GetCellSize(const wxGridExtCellCoords& coords) const
	{
		wxSize s;
		GetCellSize(coords.GetRow(), coords.GetCol(), &s.x, &s.y);
		return s;
	}

	// ------ row and col sizes
	void     SetDefaultRowSize(int height, bool resizeExistingRows = false);
	void     SetRowSize(int row, int height);
	void     HideRow(int row) { DoSetRowSize(row, 0); }
	void     ShowRow(int row) { DoSetRowSize(row, -1); }

	void     SetDefaultColSize(int width, bool resizeExistingCols = false);
	void     SetColSize(int col, int width);
	void     HideCol(int col) { DoSetColSize(col, 0); }
	void     ShowCol(int col) { DoSetColSize(col, -1); }

	// the row and column sizes can be also set all at once using
	// wxGridExtSizesInfo which holds all of them at once

	wxGridExtSizesInfo GetColSizes() const
	{
		return wxGridExtSizesInfo(GetDefaultColSize(), m_colWidths);
	}
	wxGridExtSizesInfo GetRowSizes() const
	{
		return wxGridExtSizesInfo(GetDefaultRowSize(), m_rowHeights);
	}

	void SetColSizes(const wxGridExtSizesInfo& sizeInfo);
	void SetRowSizes(const wxGridExtSizesInfo& sizeInfo);


	// ------- rows and columns reordering

	// rows and columns index <-> positions mapping: by default, the position
	// is the same as its index, but they can also be reordered
	// (either by calling SetRowPos()/SetColPos() explicitly or by the user
	// dragging around) in which case their indices don't correspond to their
	// positions on display any longer
	//
	// internally we always work with indices except for the functions which
	// have "Pos" in their names (and which work with rows and columns, not
	// pixels) and only the display and hit testing code really cares about
	// display positions at all

	// set the positions of all rows or columns at once (this method uses the
	// same conventions as wxHeaderCtrl::SetColumnsOrder() for the order array)
	void SetRowsOrder(const wxArrayInt& order);
	void SetColumnsOrder(const wxArrayInt& order);

	// return the row index corresponding to the given (valid) position
	int GetRowAt(int pos) const
	{
		return m_rowAt.empty() ? pos : m_rowAt[pos];
	}

	// return the column index corresponding to the given (valid) position
	int GetColAt(int pos) const
	{
		return m_colAt.empty() ? pos : m_colAt[pos];
	}

	// reorder the rows so that the row with the given index is now shown
	// as the position pos
	void SetRowPos(int idx, int pos);

	// reorder the columns so that the column with the given index is now shown
	// as the position pos
	void SetColPos(int idx, int pos);

	// return the position at which the row with the given index is
	// displayed: notice that this is a slow operation as we don't maintain the
	// reverse mapping currently
	int GetRowPos(int idx) const;

	// return the position at which the column with the given index is
	// displayed: notice that this is a slow operation as we don't maintain the
	// reverse mapping currently
	int GetColPos(int idx) const;

	// reset the rows or columns positions to the default order
	void ResetRowPos();
	void ResetColPos();


	// automatically size the column or row to fit to its contents, if
	// setAsMin is true, this optimal width will also be set as minimal width
	// for this column
	void     AutoSizeColumn(int col, bool setAsMin = true)
	{
		AutoSizeColOrRow(col, setAsMin, wxGRID_COLUMN);
	}
	void     AutoSizeRow(int row, bool setAsMin = true)
	{
		AutoSizeColOrRow(row, setAsMin, wxGRID_ROW);
	}

	// auto size all columns (very ineffective for big grids!)
	void     AutoSizeColumns(bool setAsMin = true);
	void     AutoSizeRows(bool setAsMin = true);

	// auto size the grid, that is make the columns/rows of the "right" size
	// and also set the grid size to just fit its contents
	void     AutoSize();

	// Note for both AutoSizeRowLabelSize and AutoSizeColLabelSize:
	// If col equals to wxGRID_AUTOSIZE value then function autosizes labels column
	// instead of data column. Note that this operation may be slow for large
	// tables.
	// autosize row height depending on label text
	void     AutoSizeRowLabelSize(int row);

	// autosize column width depending on label text
	void     AutoSizeColLabelSize(int col);

	// column won't be resized to be lesser width - this must be called during
	// the grid creation because it won't resize the column if it's already
	// narrower than the minimal width
	void     SetColMinimalWidth(int col, int width);
	void     SetRowMinimalHeight(int row, int width);

	/*  These members can be used to query and modify the minimal
	 *  acceptable size of grid rows and columns. Call this function in
	 *  your code which creates the grid if you want to display cells
	 *  with a size smaller than the default acceptable minimum size.
	 *  Like the members SetColMinimalWidth and SetRowMinimalWidth,
	 *  the existing rows or columns will not be checked/resized.
	 */
	void     SetColMinimalAcceptableWidth(int width);
	void     SetRowMinimalAcceptableHeight(int width);
	int      GetColMinimalAcceptableWidth() const;
	int      GetRowMinimalAcceptableHeight() const;

	void     SetDefaultCellBackgroundColour(const wxColour&);
	void     SetCellBackgroundColour(int row, int col, const wxColour&);

	void     SetDefaultCellTextColour(const wxColour&);
	void     SetCellTextColour(int row, int col, const wxColour&);
	void     SetDefaultCellTextOrient(const int&);
	void	 SetCellTextOrient(int row, int col, const int&);
	void     SetDefaultCellFont(const wxFont&);
	void     SetCellFont(int row, int col, const wxFont&);
	void     SetDefaultCellAlignment(int horiz, int vert);
	void     SetCellAlignment(int row, int col, int horiz, int vert);

	void	 SetDefaultCellBorderLeft(wxPenStyle style, const wxColour& colour, int width = 1);
	void	 SetCellBorderLeft(int row, int col, wxPenStyle style, const wxColour& colour, int width = 1);
	void	 SetDefaultCellBorderRight(wxPenStyle style, const wxColour& colour, int width = 1);
	void	 SetCellBorderRight(int row, int col, wxPenStyle style, const wxColour& colour, int width = 1);
	void	 SetDefaultCellBorderTop(wxPenStyle style, const wxColour& colour, int width = 1);
	void	 SetCellBorderTop(int row, int col, wxPenStyle style, const wxColour& colour, int width = 1);
	void	 SetDefaultCellBorderBottom(wxPenStyle style, const wxColour& colour, int width = 1);
	void	 SetCellBorderBottom(int row, int col, wxPenStyle style, const wxColour& colour, int width = 1);

	void     SetDefaultCellFitMode(wxGridExtFitMode fitMode);
	void     SetCellFitMode(int row, int col, wxGridExtFitMode fitMode);
	void     SetDefaultCellOverflow(bool allow)
	{
		SetDefaultCellFitMode(wxGridExtFitMode::FromOverflowFlag(allow));
	}
	void     SetCellOverflow(int row, int col, bool allow)
	{
		SetCellFitMode(row, col, wxGridExtFitMode::FromOverflowFlag(allow));
	}

	void     SetCellSize(int row, int col, int num_rows, int num_cols);

	// takes ownership of the pointer
	void SetDefaultRenderer(wxGridExtCellRenderer* renderer);
	void SetCellRenderer(int row, int col, wxGridExtCellRenderer* renderer);
	wxGridExtCellRenderer* GetDefaultRenderer() const;
	wxGridExtCellRenderer* GetCellRenderer(int row, int col) const;

	// takes ownership of the pointer
	void SetDefaultEditor(wxGridExtCellEditor* editor);
	void SetCellEditor(int row, int col, wxGridExtCellEditor* editor);
	wxGridExtCellEditor* GetDefaultEditor() const;
	wxGridExtCellEditor* GetCellEditor(int row, int col) const;


	// ------ cell area accessors
	//
	void AddAreaRow(int start, int end);
	void AddAreaCol(int start, int end);

	void AddArea();

	void DeleteAreaRow(int start, int end);
	void DeleteAreaCol(int start, int end);

	void DeleteArea();

	//make new name
	bool MakeRowAreaLabel(wxGridExtCellArea* rowArea);
	bool MakeColAreaName(wxGridExtCellArea* colArea);

	// ------ cell brake accessors
	//
	//support printing 
	void AddRowBrake(int row) { m_rowBrakeAt.Add(row); }
	void AddColBrake(int col) { m_colBrakeAt.Add(col); }

	void SetRowBrake(int row) {

		if (m_table)
		{
			if (!m_rowBrakeAt.IsEmpty())
				m_rowBrakeAt[m_rowBrakeAt.Count() - 1] = wxMax(wxMin(m_rowBrakeAt.Last(), m_table->GetRowsCount() - 1), row);
			else
				m_rowBrakeAt.Add(row);
		}
	}

	void SetColBrake(int col)
	{
		if (m_table)
		{
			if (!m_colBrakeAt.IsEmpty())
				m_colBrakeAt[m_colBrakeAt.Count() - 1] = wxMax(wxMin(m_colBrakeAt.Last(), m_table->GetColsCount() - 1), col);
			else
				m_colBrakeAt.Add(col);
		}
	}

	bool IsColBrake(int col) const {
		return m_colBrakeAt.Index(col) != wxNOT_FOUND;
	}

	bool IsRowBrake(int row) const {
		return m_rowBrakeAt.Index(row) != wxNOT_FOUND;
	}

	int GetMaxRowBrake() const {
		if (!m_rowBrakeAt.IsEmpty())
			return m_rowBrakeAt.Last();
		return 0;
	}

	int GetMaxColBrake() const {
		if (!m_colBrakeAt.IsEmpty())
			return m_colBrakeAt.Last();
		return 0;
	}

	// ------ cell value accessors
	//
	bool IsEmptyCell(int row, int col) const
	{
		if (m_table)
		{
			return m_table->IsEmptyCell(row, col);
		}
		else
		{
			return true;
		}
	}

	wxString GetCellValue(int row, int col) const
	{
		if (m_table)
		{
			wxString text;
			m_table->GetValue(row, col, text);
			return text;
		}
		else
		{
			return wxEmptyString;
		}
	}

	void GetCellValue(int row, int col, wxString& s) const
	{
		if (m_table)
		{
			m_table->GetValue(row, col, s);
		}
		else
		{
			s = wxEmptyString;
		}
	}

	wxString GetCellValue(const wxGridExtCellCoords& coords) const
	{
		wxString text;
		GetCellValue(coords.GetRow(), coords.GetCol(), text);
		return text;
	}

	void GetCellValue(const wxGridExtCellCoords& coords, wxString& s) const
	{
		GetCellValue(coords.GetRow(), coords.GetCol(), s);
	}

	void SetCellValue(int row, int col, const wxString& s);
	void SetCellValue(const wxGridExtCellCoords& coords, const wxString& s)
	{
		SetCellValue(coords.GetRow(), coords.GetCol(), s);
	}

	// returns true if the cell can't be edited
	bool IsReadOnly(int row, int col) const;

	// make the cell editable/readonly
	void SetReadOnly(int row, int col, bool isReadOnly = true);

	// ------ select blocks of cells
	//
	void SelectRow(int row, bool addToSelected = false);
	void SelectCol(int col, bool addToSelected = false);

	void SelectBlock(int topRow, int leftCol, int bottomRow, int rightCol,
		bool addToSelected = false);

	void SelectBlock(const wxGridExtCellCoords& topLeft,
		const wxGridExtCellCoords& bottomRight,
		bool addToSelected = false)
	{
		SelectBlock(topLeft.GetRow(), topLeft.GetCol(),
			bottomRight.GetRow(), bottomRight.GetCol(),
			addToSelected);
	}

	void SelectAll();

	bool IsSelection() const;

	// ------ deselect blocks or cells
	//
	void DeselectRow(int row);
	void DeselectCol(int col);
	void DeselectCell(int row, int col);

	void ClearSelection();

	bool IsInSelection(int row, int col) const;

	bool IsInSelection(const wxGridExtCellCoords& coords) const
	{
		return IsInSelection(coords.GetRow(), coords.GetCol());
	}

	// Efficient methods returning the selected blocks (there are few of those).
	wxGridExtBlocks GetSelectedBlocks() const;
	wxGridExtBlockCoordsVector GetSelectedRowBlocks() const;
	wxGridExtBlockCoordsVector GetSelectedColBlocks() const;
	wxGridExtBlockCoords GetSelectedCellRange() const;

	// Less efficient (but maybe more convenient methods) returning all
	// selected cells, rows or columns -- there can be many and many of those.
	wxGridExtCellCoordsArray GetSelectedCells() const;
	wxGridExtCellCoordsArray GetSelectionBlockTopLeft() const;
	wxGridExtCellCoordsArray GetSelectionBlockBottomRight() const;
	wxArrayInt GetSelectedRows() const;
	wxArrayInt GetSelectedCols() const;

	// This function returns the rectangle that encloses the block of cells
	// limited by TopLeft and BottomRight cell in device coords and clipped
	//  to the client size of the grid window.
	//
	wxRect BlockToDeviceRect(const wxGridExtCellCoords& topLeft,
		const wxGridExtCellCoords& bottomRight,
		const wxGridExtWindow* gridWindow = NULL) const;

	// Access or update the selection fore/back colours
	wxColour GetSelectionBackground() const
	{
		return m_selectionBackground;
	}
	wxColour GetSelectionForeground() const
	{
		return m_selectionForeground;
	}

	void SetSelectionBackground(const wxColour& c) { m_selectionBackground = c; }
	void SetSelectionForeground(const wxColour& c) { m_selectionForeground = c; }


	// Methods for a registry for mapping data types to Renderers/Editors
	void RegisterDataType(const wxString& typeName,
		wxGridExtCellRenderer* renderer,
		wxGridExtCellEditor* editor);
	// DJC MAPTEK
	virtual wxGridExtCellEditor* GetDefaultEditorForCell(int row, int col) const;
	wxGridExtCellEditor* GetDefaultEditorForCell(const wxGridExtCellCoords& c) const
	{
		return GetDefaultEditorForCell(c.GetRow(), c.GetCol());
	}
	virtual wxGridExtCellRenderer* GetDefaultRendererForCell(int row, int col) const;
	virtual wxGridExtCellEditor* GetDefaultEditorForType(const wxString& typeName) const;
	virtual wxGridExtCellRenderer* GetDefaultRendererForType(const wxString& typeName) const;

	// grid may occupy more space than needed for its rows/columns, this
	// function allows to set how big this extra space is
	void SetMargins(int extraWidth, int extraHeight)
	{
		m_extraWidth = extraWidth;
		m_extraHeight = extraHeight;

		CalcDimensions();
	}

	// Accessors for component windows
	wxWindow* GetGridWindow() const { return (wxWindow*)m_gridWin; }
	wxWindow* GetFrozenCornerGridWindow()const { return (wxWindow*)m_frozenCornerGridWin; }
	wxWindow* GetFrozenRowGridWindow() const { return (wxWindow*)m_frozenRowGridWin; }
	wxWindow* GetFrozenColGridWindow() const { return (wxWindow*)m_frozenColGridWin; }
	wxWindow* GetGridRowLabelWindow() const { return (wxWindow*)m_rowLabelWin; }
	wxWindow* GetGridColLabelWindow() const { return m_colLabelWin; }
	wxWindow* GetGridCornerLabelWindow() const { return (wxWindow*)m_cornerLabelWin; }

	// Return true if native header is used by the grid.
	bool IsUsingNativeHeader() const { return m_useNativeHeader; }

	// This one can only be called if we are using the native header window
	wxHeaderCtrl* GetGridColHeader() const
	{
		wxASSERT_MSG(m_useNativeHeader, "no column header window");

		// static_cast<> doesn't work without the full class declaration in
		// view and we prefer to avoid adding more compile-time dependencies
		// even at the cost of using reinterpret_cast<>
		return reinterpret_cast<wxHeaderCtrl*>(m_colLabelWin);
	}

	// Allow adjustment of scroll increment. The default is (15, 15).
	void SetScrollLineX(int x) { m_xScrollPixelsPerLine = x; }
	void SetScrollLineY(int y) { m_yScrollPixelsPerLine = y; }
	int GetScrollLineX() const { return m_xScrollPixelsPerLine; }
	int GetScrollLineY() const { return m_yScrollPixelsPerLine; }

	// ------- drag and drop
#if wxUSE_DRAG_AND_DROP
	virtual void SetDropTarget(wxDropTarget* dropTarget) wxOVERRIDE;
#endif // wxUSE_DRAG_AND_DROP


	// ------- sorting support

	// wxGridExt doesn't support sorting on its own but it can indicate the sort
	// order in the column header (currently only if native header control is
	// used though)

	// return the column currently displaying the sort indicator or wxNOT_FOUND
	// if none
	int GetSortingColumn() const { return m_sortCol; }

	// return true if this column is currently used for sorting
	bool IsSortingBy(int col) const { return GetSortingColumn() == col; }

	// return the current sorting order (on GetSortingColumn()): true for
	// ascending sort and false for descending; it doesn't make sense to call
	// it if GetSortingColumn() returns wxNOT_FOUND
	bool IsSortOrderAscending() const { return m_sortIsAscending; }

	// set the sorting column (or unsets any existing one if wxNOT_FOUND) and
	// the order in which to sort
	void SetSortingColumn(int col, bool ascending = true);

	// unset any existing sorting column
	void UnsetSortingColumn() { SetSortingColumn(wxNOT_FOUND); }

#if WXWIN_COMPATIBILITY_2_8
	// ------ For compatibility with previous wxGridExt only...
	//
	//  ************************************************
	//  **  Don't use these in new code because they  **
	//  **  are liable to disappear in a future       **
	//  **  revision                                  **
	//  ************************************************
	//

	wxGridExt(wxWindow* parent,
		int x, int y, int w = wxDefaultCoord, int h = wxDefaultCoord,
		long style = wxWANTS_CHARS,
		const wxString& name = wxASCII_STR(wxPanelNameStr))
	{
		Init();
		Create(parent, wxID_ANY, wxPoint(x, y), wxSize(w, h), style, name);
	}

	void SetCellValue(const wxString& val, int row, int col)
	{
		SetCellValue(row, col, val);
	}

	void UpdateDimensions()
	{
		CalcDimensions();
	}

	int GetRows() const { return GetNumberRows(); }
	int GetCols() const { return GetNumberCols(); }
	int GetCursorRow() const { return GetGridCursorRow(); }
	int GetCursorColumn() const { return GetGridCursorCol(); }

	int GetScrollPosX() const { return 0; }
	int GetScrollPosY() const { return 0; }

	void SetScrollX(int WXUNUSED(x)) {}
	void SetScrollY(int WXUNUSED(y)) {}

	void SetColumnWidth(int col, int width)
	{
		SetColSize(col, width);
	}

	int GetColumnWidth(int col) const
	{
		return GetColSize(col);
	}

	void SetRowHeight(int row, int height)
	{
		SetRowSize(row, height);
	}

	// GetRowHeight() is below

	int GetViewHeight() const // returned num whole rows visible
	{
		return 0;
	}

	int GetViewWidth() const // returned num whole cols visible
	{
		return 0;
	}

	void SetLabelSize(int orientation, int sz)
	{
		if (orientation == wxHORIZONTAL)
			SetColLabelSize(sz);
		else
			SetRowLabelSize(sz);
	}

	int GetLabelSize(int orientation) const
	{
		if (orientation == wxHORIZONTAL)
			return GetColLabelSize();
		else
			return GetRowLabelSize();
	}

	void SetLabelAlignment(int orientation, int align)
	{
		if (orientation == wxHORIZONTAL)
			SetColLabelAlignment(align, wxALIGN_INVALID);
		else
			SetRowLabelAlignment(align, wxALIGN_INVALID);
	}

	int GetLabelAlignment(int orientation, int WXUNUSED(align)) const
	{
		int h, v;
		if (orientation == wxHORIZONTAL)
		{
			GetColLabelAlignment(&h, &v);
			return h;
		}
		else
		{
			GetRowLabelAlignment(&h, &v);
			return h;
		}
	}

	void SetLabelValue(int orientation, const wxString& val, int pos)
	{
		if (orientation == wxHORIZONTAL)
			SetColLabelValue(pos, val);
		else
			SetRowLabelValue(pos, val);
	}

	wxString GetLabelValue(int orientation, int pos) const
	{
		if (orientation == wxHORIZONTAL)
			return GetColLabelValue(pos);
		else
			return GetRowLabelValue(pos);
	}

	wxFont GetCellTextFont() const
	{
		return m_defaultCellAttr->GetFont();
	}

	wxFont GetCellTextFont(int WXUNUSED(row), int WXUNUSED(col)) const
	{
		return m_defaultCellAttr->GetFont();
	}

	void SetCellTextFont(const wxFont& fnt)
	{
		SetDefaultCellFont(fnt);
	}

	void SetCellTextFont(const wxFont& fnt, int row, int col)
	{
		SetCellFont(row, col, fnt);
	}

	void SetCellTextColour(const wxColour& val, int row, int col)
	{
		SetCellTextColour(row, col, val);
	}

	void SetCellTextColour(const wxColour& col)
	{
		SetDefaultCellTextColour(col);
	}

	void SetCellBackgroundColour(const wxColour& col)
	{
		SetDefaultCellBackgroundColour(col);
	}

	void SetCellBackgroundColour(const wxColour& colour, int row, int col)
	{
		SetCellBackgroundColour(row, col, colour);
	}

	bool GetEditable() const { return IsEditable(); }
	void SetEditable(bool edit = true) { EnableEditing(edit); }
	bool GetEditInPlace() const { return IsCellEditControlEnabled(); }

	void SetEditInPlace(bool WXUNUSED(edit) = true) {}

	void SetCellAlignment(int align, int row, int col)
	{
		SetCellAlignment(row, col, align, wxALIGN_CENTER);
	}
	void SetCellAlignment(int WXUNUSED(align)) {}
	void SetCellBitmap(wxBitmap* WXUNUSED(bitmap), int WXUNUSED(row), int WXUNUSED(col))
	{
	}
	void SetDividerPen(const wxPen& WXUNUSED(pen)) {}
	wxPen& GetDividerPen() const;
	void OnActivate(bool WXUNUSED(active)) {}

	// ******** End of compatibility functions **********



	// ------ control IDs
	enum {
		wxGRID_CELLCTRL = 2000,
		wxGRID_TOPCTRL
	};

	// ------ control types
	enum {
		wxGRID_TEXTCTRL = 2100,
		wxGRID_CHECKBOX,
		wxGRID_CHOICE,
		wxGRID_COMBOBOX
	};

	wxDEPRECATED_INLINE(bool CanDragRowSize() const, return m_canDragRowSize; )
		wxDEPRECATED_INLINE(bool CanDragColSize() const, return m_canDragColSize; )
#endif // WXWIN_COMPATIBILITY_2_8


		// override some base class functions
		virtual void Fit() wxOVERRIDE;
	virtual void SetFocus() wxOVERRIDE;

	// implementation only
	void CancelMouseCapture();

protected:
	virtual wxSize DoGetBestSize() const wxOVERRIDE;
	virtual void DoEnable(bool enable) wxOVERRIDE;

	bool m_created;

	wxGridExtWindow* m_gridWin;
	wxGridExtWindow* m_frozenColGridWin;
	wxGridExtWindow* m_frozenRowGridWin;
	wxGridExtWindow* m_frozenCornerGridWin;

	wxGridExtCornerLabelWindow* m_cornerLabelWin;

	wxGridExtRowAreaWindow* m_rowAreaWin;
	wxGridExtRowAreaWindow* m_rowFrozenAreaWin;
	wxGridExtRowLabelWindow* m_rowLabelWin;
	wxGridExtRowLabelWindow* m_rowFrozenLabelWin;

	// the real type of the column window depends on m_useNativeHeader value:
	// if it is true, its dynamic type is wxHeaderCtrl, otherwise it is
	// wxGridExtColLabelWindow, use accessors below when the real type matters
	wxWindow* m_colAreaWin;
	wxWindow* m_colFrozenAreaWin;
	wxWindow* m_colLabelWin;
	wxWindow* m_colFrozenLabelWin;

	wxGridExtColLabelWindow* GetColLabelWindow() const
	{
		wxASSERT_MSG(!m_useNativeHeader, "no column label window");

		return reinterpret_cast<wxGridExtColLabelWindow*>(m_colLabelWin);
	}

	wxGridExtTableBase* m_table;
	bool                      m_ownTable;

	int m_numRows;
	int m_numCols;

	// Number of frozen rows/columns in the beginning of the grid, 0 if none.
	int m_numFrozenRows;
	int m_numFrozenCols;

	wxGridExtCellCoords m_currentCellCoords;

	// the corners of the block being currently selected or wxGridExtNoCellCoords
	wxGridExtCellCoords m_selectedBlockTopLeft;
	wxGridExtCellCoords m_selectedBlockBottomRight;

	// when selecting blocks of cells (either from the keyboard using Shift
	// with cursor keys, or by dragging the mouse), the selection is anchored
	// at m_currentCellCoords which defines one of the corners of the rectangle
	// being selected -- and this variable defines the other corner, i.e. it's
	// either m_selectedBlockTopLeft or m_selectedBlockBottomRight depending on
	// which of them is not m_currentCellCoords
	//
	// if no block selection is in process, it is set to wxGridExtNoCellCoords
	wxGridExtCellCoords m_selectedBlockCorner;

	wxGridExtSelection* m_selection;

	wxColour    m_selectionBackground;
	wxColour    m_selectionForeground;

	// NB: *never* access m_row/col arrays directly because they are created
	//     on demand, *always* use accessor functions instead!

	// init the m_rowHeights/Bottoms arrays with default values
	void InitRowHeights();

	int        m_defaultRowHeight;
	int        m_minAcceptableRowHeight;
	wxArrayInt m_rowHeights;
	wxArrayInt m_rowBottoms;

	// init the m_colWidths/Rights arrays
	void InitColWidths();

	int        m_defaultColWidth;
	int        m_minAcceptableColWidth;
	wxArrayInt m_colWidths;
	wxArrayInt m_colRights;

	int m_sortCol;
	bool m_sortIsAscending;

	bool m_useNativeHeader,
		m_nativeColumnLabels;

	// get the col/row coords
	int GetColWidth(int col) const;
	int GetColLeft(int col) const;
	int GetColRight(int col) const;

	// this function must be public for compatibility...
public:
	int GetRowHeight(int row) const;
protected:

	int GetRowTop(int row) const;
	int GetRowBottom(int row) const;

	int m_rowAreaWidth;
	int m_rowLabelWidth;
	int m_colAreaHeight;
	int m_colLabelHeight;

	// the size of the margin left to the right and bottom of the cell area
	int m_extraWidth,
		m_extraHeight;

	wxColour   m_labelBackgroundColour;
	wxColour   m_labelTextColour;
	wxFont     m_labelFont;

	int        m_rowLabelHorizAlign;
	int        m_rowLabelVertAlign;
	int        m_colLabelHorizAlign;
	int        m_colLabelVertAlign;
	int        m_colLabelTextOrientation;
	int        m_cornerLabelHorizAlign;
	int        m_cornerLabelVertAlign;
	int        m_cornerLabelTextOrientation;

	bool       m_defaultRowLabelValues;
	bool       m_defaultColLabelValues;

	wxColour   m_gridLineColour;
	bool       m_gridLinesEnabled;
	bool       m_gridLinesClipHorz,
		m_gridLinesClipVert;
	wxColour   m_cellHighlightColour;
	int        m_cellHighlightPenWidth;
	int        m_cellHighlightROPenWidth;
	wxColour   m_gridFrozenBorderColour;
	int        m_gridFrozenBorderPenWidth;

	// common part of AutoSizeColumn/Row()
	void AutoSizeColOrRow(int n, bool setAsMin, wxGridExtDirection direction);

	// Calculate the minimum acceptable size for labels area
	wxCoord CalcColOrRowLabelAreaMinSize(wxGridExtDirection direction);

	// if a column has a minimal width, it will be the value for it in this
	// hash table
	wxLongToLongHashMap m_colMinWidths,
		m_rowMinHeights;

	// get the minimal width of the given column/row
	int GetColMinimalWidth(int col) const;
	int GetRowMinimalHeight(int col) const;

	// do we have some place to store attributes in?
	bool CanHaveAttributes() const;

	// cell attribute cache (currently we only cache 1, may be will do
	// more/better later)
	struct CachedAttr
	{
		int             row, col;
		wxGridExtCellAttr* attr;
	} m_attrCache;

	// invalidates the attribute cache
	void ClearAttrCache();

	// adds an attribute to cache
	void CacheAttr(int row, int col, wxGridExtCellAttr* attr) const;

	// looks for an attr in cache, returns true if found
	bool LookupAttr(int row, int col, wxGridExtCellAttr** attr) const;

	// looks for the attr in cache, if not found asks the table and caches the
	// result
	wxGridExtCellAttr* GetCellAttr(int row, int col) const;
	wxGridExtCellAttr* GetCellAttr(const wxGridExtCellCoords& coords) const
	{
		return GetCellAttr(coords.GetRow(), coords.GetCol());
	}

	wxGridExtCellAttrPtr GetCellAttrPtr(int row, int col) const
	{
		return wxGridExtCellAttrPtr(GetCellAttr(row, col));
	}
	wxGridExtCellAttrPtr GetCellAttrPtr(const wxGridExtCellCoords& coords) const
	{
		return wxGridExtCellAttrPtr(GetCellAttr(coords));
	}


	// the default cell attr object for cells that don't have their own
	wxGridExtCellAttr* m_defaultCellAttr;


	int  m_batchCount;


	wxGridExtTypeRegistry* m_typeRegistry;

	enum CursorMode
	{
		WXGRID_CURSOR_SELECT_CELL,
		WXGRID_CURSOR_RESIZE_ROW,
		WXGRID_CURSOR_RESIZE_COL,
		WXGRID_CURSOR_SELECT_ROW,
		WXGRID_CURSOR_SELECT_COL,
		WXGRID_CURSOR_MOVE_ROW,
		WXGRID_CURSOR_MOVE_COL
	};

	// this method not only sets m_cursorMode but also sets the correct cursor
	// for the given mode and, if captureMouse is not false releases the mouse
	// if it was captured and captures it if it must be captured
	//
	// for this to work, you should always use it and not set m_cursorMode
	// directly!
	void ChangeCursorMode(CursorMode mode,
		wxWindow* win = NULL,
		bool captureMouse = true);

	wxWindow* m_winCapture;     // the window which captured the mouse

	// this variable is used not for finding the correct current cursor but
	// mainly for finding out what is going to happen if the mouse starts being
	// dragged right now
	//
	// by default it is WXGRID_CURSOR_SELECT_CELL meaning that nothing else is
	// going on, and it is set to one of RESIZE/SELECT/MOVE values while the
	// corresponding operation will be started if the user starts dragging the
	// mouse from the current position
	CursorMode m_cursorMode;

	//Area row, col positions
	wxGridExtCellAreaArray m_rowAreaAt;
	wxGridExtCellAreaArray m_colAreaAt;

	//Row positions
	wxArrayInt m_rowBrakeAt;

	//Row positions
	wxArrayInt m_colBrakeAt;

	//Row positions
	wxArrayInt m_rowAt;

	//Column positions
	wxArrayInt m_colAt;

	bool    m_canDragRowSize;
	bool    m_canDragColSize;
	bool    m_canDragRowMove;
	bool    m_canDragColMove;
	bool    m_canHideColumns;
	bool    m_canDragGridSize;
	bool    m_canDragCell;

	// Index of the row or column being drag-moved or -1 if there is no move
	// operation in progress.
	int     m_dragMoveRowOrCol;

	// Last drag marker position while drag-moving a row or column.
	int     m_dragLastPos;

	// Last drag marker colour while drag-moving a row or column.
	const wxColour* m_dragLastColour;

	// Row or column (depending on m_cursorMode value) currently being resized
	// or -1 if there is no resize operation in progress.
	int     m_dragRowOrCol;

	// Original row or column size when resizing; used when the user cancels
	int     m_dragRowOrColOldSize;

	// true if a drag operation is in progress; when this is true,
	// m_startDragPos is valid, i.e. not wxDefaultPosition
	bool    m_isDragging;

	// true if a drag operation was canceled
	// (mouse event Dragging() might still be active until LeftUp)
	// m_isDragging can only be set after m_cancelledDragging is cleared.
	// This is done when a mouse event happens with left button up.
	bool    m_cancelledDragging;

	// the position (in physical coordinates) where the user started dragging
	// the mouse or wxDefaultPosition if mouse isn't being dragged
	//
	// notice that this can be != wxDefaultPosition while m_isDragging is still
	// false because we wait until the mouse is moved some distance away before
	// setting m_isDragging to true
	wxPoint m_startDragPos;

	// the position of the last mouse event
	// used for detection of the movement direction
	wxPoint m_lastMousePos;

	bool    m_waitForSlowClick;

	wxCursor m_rowResizeCursor;
	wxCursor m_colResizeCursor;

	bool       m_editable;              // applies to whole grid
	bool       m_cellEditCtrlEnabled;   // is in-place edit currently shown?
	bool       m_areaEnabled;			// area is enable?

	TabBehaviour m_tabBehaviour;        // determines how the TAB key behaves

	void Init();        // common part of all ctors
	void Create();
	void CreateColumnWindow();
	void CalcDimensions();
	void CalcWindowSizes();
	bool Redimension(wxGridExtTableMessage&);


	enum EventResult
	{
		Event_Vetoed = -1,
		Event_Unhandled,
		Event_Handled,
		Event_CellDeleted   // Event handler deleted the cell.
	};

	// Send the given grid event and returns one of the event handling results
	// defined above.
	EventResult DoSendEvent(wxGridExtEvent& gridEvt);

	// Generate an event of the given type and call DoSendEvent().
	EventResult SendEvent(wxEventType evtType,
		int row, int col,
		const wxMouseEvent& e);
	EventResult SendEvent(wxEventType evtType,
		const wxGridExtCellCoords& coords,
		const wxMouseEvent& e)
	{
		return SendEvent(evtType, coords.GetRow(), coords.GetCol(), e);
	}
	EventResult SendEvent(wxEventType evtType,
		int row, int col,
		const wxString& s = wxString());
	EventResult SendEvent(wxEventType evtType,
		const wxGridExtCellCoords& coords,
		const wxString& s = wxString())
	{
		return SendEvent(evtType, coords.GetRow(), coords.GetCol(), s);
	}
	EventResult SendEvent(wxEventType evtType, const wxString& s = wxString())
	{
		return SendEvent(evtType, m_currentCellCoords, s);
	}

	// send wxEVT_GRID_{ROW,COL}_SIZE or wxEVT_GRID_{ROW,COL}_AUTO_SIZE, return true
	// if the event was processed, false otherwise
	bool SendGridSizeEvent(wxEventType type,
		int rowOrCol,
		const wxMouseEvent& mouseEv);

	void OnSize(wxSizeEvent&);
	void OnKeyDown(wxKeyEvent&);
	void OnKeyUp(wxKeyEvent&);
	void OnChar(wxKeyEvent&);


	bool SetCurrentCell(const wxGridExtCellCoords& coords);
	bool SetCurrentCell(int row, int col)
	{
		return SetCurrentCell(wxGridExtCellCoords(row, col));
	}


	virtual bool ShouldScrollToChildOnFocus(wxWindow* WXUNUSED(win)) wxOVERRIDE
	{
		return false;
	}

	friend class wxGridExtSelection;
	friend class wxGridExtOperations;
	friend class wxGridExtRowOperations;
	friend class wxGridExtColumnOperations;

	// they call our private Process{{Corner,Col,Row}Label,GridCell}MouseEvent()
	friend class wxGridExtCornerLabelWindow;
	friend class wxGridExtColAreaWindow;
	friend class wxGridExtColLabelWindow;
	friend class wxGridExtRowAreaWindow;
	friend class wxGridExtRowLabelWindow;
	friend class wxGridExtWindow;
	friend class wxGridExtHeaderRenderer;

	friend class wxGridExtHeaderColumn;
	friend class wxGridExtHeaderCtrl;

private:
	// This is called from both Create() and OnDPIChanged() to (re)initialize
	// the values in pixels, which depend on the current DPI.
	void InitPixelFields();

	// Event handler for DPI change event recomputes pixel values and relays
	// out the grid.
	void OnDPIChanged(wxDPIChangedEvent& event);

	// implement wxScrolledCanvas method to return m_gridWin size
	virtual wxSize GetSizeAvailableForScrollTarget(const wxSize& size) wxOVERRIDE;

	// depending on the values of m_numFrozenRows and m_numFrozenCols, it will
	// create and initialize or delete the frozen windows
	void InitializeFrozenWindows();

	// redraw the grid lines, should be called after changing their attributes
	void RedrawGridLines();

	// draw all grid lines in the given cell region (unlike the public
	// DrawAllGridLines() which just draws all of them)
	void DrawRangeGridLines(wxDC& dc, const wxRegion& reg,
		const wxGridExtCellCoords& topLeft,
		const wxGridExtCellCoords& bottomRight);

	// draw all lines from top to bottom row and left to right column in the
	// rectangle determined by (top, left)-(bottom, right) -- but notice that
	// the caller must have set up the clipping correctly, this rectangle is
	// only used here for optimization
	void DoDrawGridLines(wxDC& dc,
		int top, int left,
		int bottom, int right,
		int topRow, int leftCol,
		int bottomRight, int rightCol);

	void DoDrawGridNonClippingRegionLines(wxDC& dc,
		int top, int left,
		int bottom, int right,
		int topRow, int leftCol,
		int bottomRight, int rightCol);

	// common part of Clip{Horz,Vert}GridLines
	void DoClipGridLines(bool& var, bool clip);

	// Redimension() helper: update m_currentCellCoords if necessary after a
	// grid size change
	void UpdateCurrentCellOnRedim();

	// update the sorting indicator shown in the specified column (whose index
	// must be valid)
	//
	// this will use GetSortingColumn() and IsSortOrderAscending() to determine
	// the sorting indicator to effectively show
	void UpdateColumnSortingIndicator(int col);

	// update the grid after changing the rows or columns order (common part
	// of Set{Row,Col}Pos() and Reset{Row,Col}Pos())
	void RefreshAfterRowPosChange();
	void RefreshAfterColPosChange();

	// reset the variables used during dragging operations after it ended,
	// either because we called EndDraggingIfNecessary() ourselves or because
	// we lost mouse capture
	void DoAfterDraggingEnd();

	// release the mouse capture if it's currently captured
	void EndDraggingIfNecessary();

	// helper for Process...MouseEvent to block re-triggering m_isDragging
	bool CheckIfDragCancelled(wxMouseEvent* event);

	// helper for Process...MouseEvent to scroll
	void CheckDoDragScroll(wxGridExtSubwindow* eventGridWindow, wxGridExtSubwindow* gridWindow,
		wxPoint posEvent, int direction);

	// helper for Process...LabelMouseEvent to check whether a drag operation
	// would end at the source line, i.e. have no effect
	bool CheckIfAtDragSourceLine(const wxGridExtOperations& oper, int coord);

	// return true if the grid should be refreshed right now
	bool ShouldRefresh() const
	{
		return !GetBatchCount() && IsShownOnScreen();
	}


	// return the position (not index) of the row or column at the given logical
	// pixel position
	//
	// this always returns a valid position, even if the coordinate is out of
	// bounds (in which case first/last row/column is returned)
	int YToPos(int y, wxGridExtWindow* gridWindow) const;
	int XToPos(int x, wxGridExtWindow* gridWindow) const;

	// event handlers and their helpers
	// --------------------------------

	// process mouse drag event in WXGRID_CURSOR_SELECT_CELL mode
	bool DoGridCellDrag(wxMouseEvent& event,
		const wxGridExtCellCoords& coords,
		bool isFirstDrag);

	// process mouse drag event in the grid window, return false if starting
	// dragging was vetoed by the user-defined wxEVT_GRID_CELL_BEGIN_DRAG
	// handler
	bool DoGridDragEvent(wxMouseEvent& event,
		const wxGridExtCellCoords& coords,
		bool isFirstDrag,
		wxGridExtWindow* gridWindow);

	// Update the width/height of the column/row being drag-resized.
	// Should be only called when m_dragRowOrCol != -1, i.e. dragging is
	// actually in progress.
	void DoGridDragResize(const wxPoint& position,
		const wxGridExtOperations& oper,
		wxGridExtWindow* gridWindow);

	// process different clicks on grid cells
	void DoGridCellLeftDown(wxMouseEvent& event,
		const wxGridExtCellCoords& coords,
		const wxPoint& pos);
	void DoGridCellLeftDClick(wxMouseEvent& event,
		const wxGridExtCellCoords& coords,
		const wxPoint& pos);
	void DoGridCellLeftUp(wxMouseEvent& event,
		const wxGridExtCellCoords& coords,
		wxGridExtWindow* gridWindow);

	// process movement (but not dragging) event in the grid cell area
	void DoGridMouseMoveEvent(wxMouseEvent& event,
		const wxGridExtCellCoords& coords,
		const wxPoint& pos,
		wxGridExtWindow* gridWindow);

	// process mouse events in the grid window
	void ProcessGridCellMouseEvent(wxMouseEvent& event, wxGridExtWindow* gridWindow);

	// process mouse events in the row/column labels/corner windows
	void ProcessRowColLabelMouseEvent(const wxGridExtOperations& oper,
		wxMouseEvent& event,
		wxGridExtSubwindow* areaWin);

	void ProcessRowColAreaMouseEvent(const wxGridExtOperations& oper,
		wxMouseEvent& event,
		wxGridExtSubwindow* areaWin);

	void ProcessCornerLabelMouseEvent(wxMouseEvent& event);

	void HandleRowAutosize(int col, const wxMouseEvent& event);
	void HandleColumnAutosize(int col, const wxMouseEvent& event);

	void DoColHeaderClick(int col);

	void DoStartResizeRowOrCol(int col, int size);
	void DoStartMoveRowOrCol(int col);

	// These functions should only be called when actually resizing/moving,
	// i.e. m_dragRowOrCol and m_dragMoveCol, respectively, are valid.
	void DoEndDragResizeRow(const wxMouseEvent& event, wxGridExtWindow* gridWindow);
	void DoEndDragResizeCol(const wxMouseEvent& event, wxGridExtWindow* gridWindow);
	void DoEndMoveRow(int pos);
	void DoEndMoveCol(int pos);

	// Helper function returning the position (only the horizontal component
	// really counts) corresponding to the given column drag-resize event.
	//
	// It's a bit ugly to create a phantom mouse position when we really only
	// need the column width anyhow, but wxGridExt code was originally written to
	// expect the position and not the width and it's simpler to keep it happy
	// by giving it the position than to change it.
	wxPoint GetPositionForResizeEvent(int width) const;

	// functions called by wxGridExtHeaderCtrl while resizing m_dragRowOrCol
	void DoHeaderStartDragResizeCol(int col);
	void DoHeaderDragResizeCol(int width);
	void DoHeaderEndDragResizeCol(int width);

	// process a TAB keypress
	void DoGridProcessTab(wxKeyboardState& kbdState);

	// common implementations of methods defined for both rows and columns
	int PosToLinePos(int pos, bool clipToMinMax,
		const wxGridExtOperations& oper,
		wxGridExtWindow* gridWindow) const;
	int PosToLine(int pos, bool clipToMinMax,
		const wxGridExtOperations& oper,
		wxGridExtWindow* gridWindow) const;
	int PosToEdgeOfLine(int pos, const wxGridExtOperations& oper) const;

	void DoMoveCursorFromKeyboard(const wxKeyboardState& kbdState,
		const wxGridExtDirectionOperations& diroper);
	bool DoMoveCursor(const wxKeyboardState& kbdState,
		const wxGridExtDirectionOperations& diroper);
	bool DoMoveCursorByPage(const wxKeyboardState& kbdState,
		const wxGridExtDirectionOperations& diroper);
	bool AdvanceByPage(wxGridExtCellCoords& coords,
		const wxGridExtDirectionOperations& diroper);
	bool DoMoveCursorByBlock(const wxKeyboardState& kbdState,
		const wxGridExtDirectionOperations& diroper);
	void AdvanceToNextNonEmpty(wxGridExtCellCoords& coords,
		const wxGridExtDirectionOperations& diroper);
	bool AdvanceByBlock(wxGridExtCellCoords& coords,
		const wxGridExtDirectionOperations& diroper);

	// common part of {Insert,Delete}{Rows,Cols}
	bool DoModifyLines(bool (wxGridExtTableBase::* funcModify)(size_t, size_t),
		int pos, int num, bool updateLabels);
	// Append{Rows,Cols} is a bit different because of one less parameter
	bool DoAppendLines(bool (wxGridExtTableBase::* funcAppend)(size_t),
		int num, bool updateLabels);

	// common part of Set{Col,Row}Sizes
	void DoSetSizes(const wxGridExtSizesInfo& sizeInfo,
		const wxGridExtOperations& oper);

	// common part of Disable{Row,Col}Resize and CanDrag{Row,Col}Size
	void DoDisableLineResize(int line, wxGridExtFixedIndicesSet*& setFixed);
	bool DoCanResizeLine(int line, const wxGridExtFixedIndicesSet* setFixed) const;

	// Helper of Render(): get grid size, origin offset and fill cell arrays
	void GetRenderSizes(const wxGridExtCellCoords& topLeft,
		const wxGridExtCellCoords& bottomRight,
		wxPoint& pointOffSet, wxSize& sizeGrid,
		wxGridExtCellCoordsArray& renderCells,
		wxArrayInt& arrayCols, wxArrayInt& arrayRows) const;

	// Helper of Render(): set the scale to draw the cells at the right size.
	void SetRenderScale(wxDC& dc, const wxPoint& pos, const wxSize& size,
		const wxSize& sizeGrid);

	// Helper of Render(): get render start position from passed parameter
	wxPoint GetRenderPosition(wxDC& dc, const wxPoint& position);

	// Helper of Render(): draws a box around the rendered area
	void DoRenderBox(wxDC& dc, const int& style,
		const wxPoint& pointOffSet,
		const wxSize& sizeCellArea,
		const wxGridExtCellCoords& topLeft,
		const wxGridExtCellCoords& bottomRight);

	// Implementation of public Set{Row,Col}Size() and {Hide,Show}{Row,Col}().
	// They interpret their height or width parameter slightly different from
	// the public methods where -1 in it means "auto fit to the label" for the
	// compatibility reasons. Here it means "show a previously hidden row or
	// column" while 0 means "hide it" just as in the public methods. And any
	// positive values are handled naturally, i.e. they just specify the size.
	void DoSetRowSize(int row, int height);
	void DoSetColSize(int col, int width);

	// These methods can only be called when m_useNativeHeader is true and call
	// SetColumnCount() and Set- or ResetColumnsOrder() as necessary on the
	// native wxHeaderCtrl being used. Note that the first one already calls
	// the second one, so it's never necessary to call both of them.
	void SetNativeHeaderColCount();
	void SetNativeHeaderColOrder();

	// Return the editor which should be used for the current cell.
	wxGridExtCellEditorPtr GetCurrentCellEditorPtr() const
	{
		return GetCellAttrPtr(m_currentCellCoords)->GetEditorPtr
		(
			this,
			m_currentCellCoords.GetRow(),
			m_currentCellCoords.GetCol()
		);
	}

	// Show/hide the cell editor for the current cell unconditionally.

	// Return false if the editor was activated instead of being shown and also
	// sets m_cellEditCtrlEnabled to true when it returns true as a side effect.
	bool DoShowCellEditControl(const wxGridExtActivationSource& actSource);
	void DoHideCellEditControl();

	// Unconditionally try showing the editor for the current cell.
	//
	// Returns false if the user code vetoed wxEVT_GRID_EDITOR_SHOWN or if the
	// editor was simply activated and won't be permanently shown.
	bool DoEnableCellEditControl(const wxGridExtActivationSource& actSource);

	// Unconditionally disable (accepting the changes) the editor.
	void DoDisableCellEditControl();

	// Accept the changes in the edit control, i.e. save them to the table and
	// dismiss the editor. Also reset m_cellEditCtrlEnabled.
	void DoAcceptCellEditControl();

	// As above, but do nothing if the control is not currently shown.
	void AcceptCellEditControlIfShown();

	// Unlike the public SaveEditControlValue(), this method doesn't check if
	// the edit control is shown, but just supposes that it is.
	void DoSaveEditControlValue();

	// these sets contain the indices of fixed, i.e. non-resizable
	// interactively, grid rows or columns and are NULL if there are no fixed
	// elements (which is the default)
	wxGridExtFixedIndicesSet* m_setFixedRows,
		* m_setFixedCols;

	//wxDECLARE_DYNAMIC_CLASS(wxGridExt);
	wxDECLARE_EVENT_TABLE();
	wxDECLARE_NO_COPY_CLASS(wxGridExt);
};

// ----------------------------------------------------------------------------
// wxGridExtUpdateLocker prevents updates to a grid during its lifetime
// ----------------------------------------------------------------------------

class wxGridExtUpdateLocker
{
public:
	// if the pointer is NULL, Create() can be called later
	wxGridExtUpdateLocker(wxGridExt* grid = NULL)
	{
		Init(grid);
	}

	// can be called if ctor was used with a NULL pointer, must not be called
	// more than once
	void Create(wxGridExt* grid)
	{
		wxASSERT_MSG(!m_grid, wxT("shouldn't be called more than once"));

		Init(grid);
	}

	~wxGridExtUpdateLocker()
	{
		if (m_grid)
			m_grid->EndBatch();
	}

private:
	void Init(wxGridExt* grid)
	{
		m_grid = grid;
		if (m_grid)
			m_grid->BeginBatch();
	}

	wxGridExt* m_grid;

	wxDECLARE_NO_COPY_CLASS(wxGridExtUpdateLocker);
};

// ----------------------------------------------------------------------------
// Grid event class and event types
// ----------------------------------------------------------------------------

class wxGridExtEvent : public wxNotifyEvent,
	public wxKeyboardState
{
public:
	wxGridExtEvent()
		: wxNotifyEvent()
	{
		Init(-1, -1, -1, -1, false);
	}

	wxGridExtEvent(int id,
		wxEventType type,
		wxObject* obj,
		int row = -1, int col = -1,
		int x = -1, int y = -1,
		bool sel = true,
		const wxKeyboardState& kbd = wxKeyboardState())
		: wxNotifyEvent(type, id),
		wxKeyboardState(kbd)
	{
		Init(row, col, x, y, sel);
		SetEventObject(obj);
	}

	// explicitly specifying inline allows gcc < 3.4 to
	// handle the deprecation attribute even in the constructor.
	wxDEPRECATED_CONSTRUCTOR(
		wxGridExtEvent(int id,
			wxEventType type,
			wxObject* obj,
			int row, int col,
			int x, int y,
			bool sel,
			bool control,
			bool shift = false, bool alt = false, bool meta = false));

	int GetRow() const { return m_row; }
	int GetCol() const { return m_col; }
	wxPoint GetPosition() const { return wxPoint(m_x, m_y); }
	bool Selecting() const { return m_selecting; }

	virtual wxEvent* Clone() const wxOVERRIDE { return new wxGridExtEvent(*this); }

protected:
	int         m_row;
	int         m_col;
	int         m_x;
	int         m_y;
	bool        m_selecting;

private:
	void Init(int row, int col, int x, int y, bool sel)
	{
		m_row = row;
		m_col = col;
		m_x = x;
		m_y = y;
		m_selecting = sel;
	}

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN_DEF_COPY(wxGridExtEvent);
};

class wxGridExtSizeEvent : public wxNotifyEvent,
	public wxKeyboardState
{
public:
	wxGridExtSizeEvent()
		: wxNotifyEvent()
	{
		Init(-1, -1, -1);
	}

	wxGridExtSizeEvent(int id,
		wxEventType type,
		wxObject* obj,
		int rowOrCol = -1,
		int x = -1, int y = -1,
		const wxKeyboardState& kbd = wxKeyboardState())
		: wxNotifyEvent(type, id),
		wxKeyboardState(kbd)
	{
		Init(rowOrCol, x, y);

		SetEventObject(obj);
	}

	wxDEPRECATED_CONSTRUCTOR(
		wxGridExtSizeEvent(int id,
			wxEventType type,
			wxObject* obj,
			int rowOrCol,
			int x, int y,
			bool control,
			bool shift = false,
			bool alt = false,
			bool meta = false));

	int GetRowOrCol() const { return m_rowOrCol; }
	wxPoint GetPosition() const { return wxPoint(m_x, m_y); }

	virtual wxEvent* Clone() const wxOVERRIDE { return new wxGridExtSizeEvent(*this); }

protected:
	int         m_rowOrCol;
	int         m_x;
	int         m_y;

private:
	void Init(int rowOrCol, int x, int y)
	{
		m_rowOrCol = rowOrCol;
		m_x = x;
		m_y = y;
	}

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN_DEF_COPY(wxGridExtSizeEvent);
};


class wxGridExtRangeSelectEvent : public wxNotifyEvent,
	public wxKeyboardState
{
public:
	wxGridExtRangeSelectEvent()
		: wxNotifyEvent()
	{
		Init(wxGridExtNoCellCoords, wxGridExtNoCellCoords, false);
	}

	wxGridExtRangeSelectEvent(int id,
		wxEventType type,
		wxObject* obj,
		const wxGridExtCellCoords& topLeft,
		const wxGridExtCellCoords& bottomRight,
		bool sel = true,
		const wxKeyboardState& kbd = wxKeyboardState())
		: wxNotifyEvent(type, id),
		wxKeyboardState(kbd)
	{
		Init(topLeft, bottomRight, sel);

		SetEventObject(obj);
	}

	wxDEPRECATED_CONSTRUCTOR(
		wxGridExtRangeSelectEvent(int id,
			wxEventType type,
			wxObject* obj,
			const wxGridExtCellCoords& topLeft,
			const wxGridExtCellCoords& bottomRight,
			bool sel,
			bool control,
			bool shift = false,
			bool alt = false,
			bool meta = false));

	wxGridExtCellCoords GetTopLeftCoords() const { return m_topLeft; }
	wxGridExtCellCoords GetBottomRightCoords() const { return m_bottomRight; }
	int GetTopRow() const { return m_topLeft.GetRow(); }
	int GetBottomRow() const { return m_bottomRight.GetRow(); }
	int GetLeftCol() const { return m_topLeft.GetCol(); }
	int GetRightCol() const { return m_bottomRight.GetCol(); }
	bool Selecting() const { return m_selecting; }

	virtual wxEvent* Clone() const wxOVERRIDE { return new wxGridExtRangeSelectEvent(*this); }

protected:
	void Init(const wxGridExtCellCoords& topLeft,
		const wxGridExtCellCoords& bottomRight,
		bool selecting)
	{
		m_topLeft = topLeft;
		m_bottomRight = bottomRight;
		m_selecting = selecting;
	}

	wxGridExtCellCoords  m_topLeft;
	wxGridExtCellCoords  m_bottomRight;
	bool              m_selecting;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN_DEF_COPY(wxGridExtRangeSelectEvent);
};


class wxGridExtEditorCreatedEvent : public wxCommandEvent
{
public:
	wxGridExtEditorCreatedEvent()
		: wxCommandEvent()
	{
		m_row = 0;
		m_col = 0;
		m_window = NULL;
	}

	wxGridExtEditorCreatedEvent(int id, wxEventType type, wxObject* obj,
		int row, int col, wxWindow* window);

	int GetRow() const { return m_row; }
	int GetCol() const { return m_col; }
	wxWindow* GetWindow() const { return m_window; }
	void SetRow(int row) { m_row = row; }
	void SetCol(int col) { m_col = col; }
	void SetWindow(wxWindow* window) { m_window = window; }

	// These functions exist only for backward compatibility, use Get and
	// SetWindow() instead in the new code.
	wxControl* GetControl() { return wxDynamicCast(m_window, wxControl); }
	void SetControl(wxControl* ctrl) { m_window = ctrl; }

	virtual wxEvent* Clone() const wxOVERRIDE { return new wxGridExtEditorCreatedEvent(*this); }

private:
	int m_row;
	int m_col;
	wxWindow* m_window;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN_DEF_COPY(wxGridExtEditorCreatedEvent);
};


wxDECLARE_EVENT(wxEVT_GRID_CELL_LEFT_CLICK, wxGridExtEvent);
wxDECLARE_EVENT(wxEVT_GRID_CELL_RIGHT_CLICK, wxGridExtEvent);
wxDECLARE_EVENT(wxEVT_GRID_CELL_LEFT_DCLICK, wxGridExtEvent);
wxDECLARE_EVENT(wxEVT_GRID_CELL_RIGHT_DCLICK, wxGridExtEvent);
wxDECLARE_EVENT(wxEVT_GRID_LABEL_LEFT_CLICK, wxGridExtEvent);
wxDECLARE_EVENT(wxEVT_GRID_LABEL_RIGHT_CLICK, wxGridExtEvent);
wxDECLARE_EVENT(wxEVT_GRID_LABEL_LEFT_DCLICK, wxGridExtEvent);
wxDECLARE_EVENT(wxEVT_GRID_LABEL_RIGHT_DCLICK, wxGridExtEvent);
wxDECLARE_EVENT(wxEVT_GRID_ROW_SIZE, wxGridExtSizeEvent);
wxDECLARE_EVENT(wxEVT_GRID_ROW_AUTO_SIZE, wxGridExtSizeEvent);
wxDECLARE_EVENT(wxEVT_GRID_COL_SIZE, wxGridExtSizeEvent);
wxDECLARE_EVENT(wxEVT_GRID_COL_AUTO_SIZE, wxGridExtSizeEvent);
wxDECLARE_EVENT(wxEVT_GRID_RANGE_SELECTING, wxGridExtRangeSelectEvent);
wxDECLARE_EVENT(wxEVT_GRID_RANGE_SELECTED, wxGridExtRangeSelectEvent);
wxDECLARE_EVENT(wxEVT_GRID_CELL_CHANGING, wxGridExtEvent);
wxDECLARE_EVENT(wxEVT_GRID_CELL_CHANGED, wxGridExtEvent);
wxDECLARE_EVENT(wxEVT_GRID_SELECT_CELL, wxGridExtEvent);
wxDECLARE_EVENT(wxEVT_GRID_EDITOR_SHOWN, wxGridExtEvent);
wxDECLARE_EVENT(wxEVT_GRID_EDITOR_HIDDEN, wxGridExtEvent);
wxDECLARE_EVENT(wxEVT_GRID_EDITOR_CREATED, wxGridExtEditorCreatedEvent);
wxDECLARE_EVENT(wxEVT_GRID_CELL_BEGIN_DRAG, wxGridExtEvent);
wxDECLARE_EVENT(wxEVT_GRID_ROW_MOVE, wxGridExtEvent);
wxDECLARE_EVENT(wxEVT_GRID_COL_MOVE, wxGridExtEvent);
wxDECLARE_EVENT(wxEVT_GRID_COL_SORT, wxGridExtEvent);
wxDECLARE_EVENT(wxEVT_GRID_CHANGED, wxGridExtEvent);
wxDECLARE_EVENT(wxEVT_GRID_TABBING, wxGridExtEvent);

typedef void (wxEvtHandler::* wxGridExtEventFunction)(wxGridExtEvent&);
typedef void (wxEvtHandler::* wxGridExtSizeEventFunction)(wxGridExtSizeEvent&);
typedef void (wxEvtHandler::* wxGridExtRangeSelectEventFunction)(wxGridExtRangeSelectEvent&);
typedef void (wxEvtHandler::* wxGridExtEditorCreatedEventFunction)(wxGridExtEditorCreatedEvent&);

#define wxGridExtEventHandler(func) \
    wxEVENT_HANDLER_CAST(wxGridExtEventFunction, func)

#define wxGridExtSizeEventHandler(func) \
    wxEVENT_HANDLER_CAST(wxGridExtSizeEventFunction, func)

#define wxGridExtRangeSelectEventHandler(func) \
    wxEVENT_HANDLER_CAST(wxGridExtRangeSelectEventFunction, func)

#define wxGridExtEditorCreatedEventHandler(func) \
    wxEVENT_HANDLER_CAST(wxGridExtEditorCreatedEventFunction, func)

#define wx__DECLARE_GRIDEVT(evt, id, fn) \
    wx__DECLARE_EVT1(wxEVT_GRID_ ## evt, id, wxGridExtEventHandler(fn))

#define wx__DECLARE_GRIDSIZEEVT(evt, id, fn) \
    wx__DECLARE_EVT1(wxEVT_GRID_ ## evt, id, wxGridExtSizeEventHandler(fn))

#define wx__DECLARE_GRIDRANGESELEVT(evt, id, fn) \
    wx__DECLARE_EVT1(wxEVT_GRID_ ## evt, id, wxGridExtRangeSelectEventHandler(fn))

#define wx__DECLARE_GRIDEDITOREVT(evt, id, fn) \
    wx__DECLARE_EVT1(wxEVT_GRID_ ## evt, id, wxGridExtEditorCreatedEventHandler(fn))

#define EVT_GRID_CMD_CELL_LEFT_CLICK(id, fn)     wx__DECLARE_GRIDEVT(CELL_LEFT_CLICK, id, fn)
#define EVT_GRID_CMD_CELL_RIGHT_CLICK(id, fn)    wx__DECLARE_GRIDEVT(CELL_RIGHT_CLICK, id, fn)
#define EVT_GRID_CMD_CELL_LEFT_DCLICK(id, fn)    wx__DECLARE_GRIDEVT(CELL_LEFT_DCLICK, id, fn)
#define EVT_GRID_CMD_CELL_RIGHT_DCLICK(id, fn)   wx__DECLARE_GRIDEVT(CELL_RIGHT_DCLICK, id, fn)
#define EVT_GRID_CMD_LABEL_LEFT_CLICK(id, fn)    wx__DECLARE_GRIDEVT(LABEL_LEFT_CLICK, id, fn)
#define EVT_GRID_CMD_LABEL_RIGHT_CLICK(id, fn)   wx__DECLARE_GRIDEVT(LABEL_RIGHT_CLICK, id, fn)
#define EVT_GRID_CMD_LABEL_LEFT_DCLICK(id, fn)   wx__DECLARE_GRIDEVT(LABEL_LEFT_DCLICK, id, fn)
#define EVT_GRID_CMD_LABEL_RIGHT_DCLICK(id, fn)  wx__DECLARE_GRIDEVT(LABEL_RIGHT_DCLICK, id, fn)
#define EVT_GRID_CMD_ROW_SIZE(id, fn)            wx__DECLARE_GRIDSIZEEVT(ROW_SIZE, id, fn)
#define EVT_GRID_CMD_COL_SIZE(id, fn)            wx__DECLARE_GRIDSIZEEVT(COL_SIZE, id, fn)
#define EVT_GRID_CMD_COL_AUTO_SIZE(id, fn)       wx__DECLARE_GRIDSIZEEVT(COL_AUTO_SIZE, id, fn)
#define EVT_GRID_CMD_ROW_MOVE(id, fn)            wx__DECLARE_GRIDEVT(ROW_MOVE, id, fn)
#define EVT_GRID_CMD_COL_MOVE(id, fn)            wx__DECLARE_GRIDEVT(COL_MOVE, id, fn)
#define EVT_GRID_CMD_COL_SORT(id, fn)            wx__DECLARE_GRIDEVT(COL_SORT, id, fn)
#define EVT_GRID_CMD_RANGE_SELECTING(id, fn)     wx__DECLARE_GRIDRANGESELEVT(RANGE_SELECTING, id, fn)
#define EVT_GRID_CMD_RANGE_SELECTED(id, fn)      wx__DECLARE_GRIDRANGESELEVT(RANGE_SELECTED, id, fn)
#define EVT_GRID_CMD_CELL_CHANGING(id, fn)       wx__DECLARE_GRIDEVT(CELL_CHANGING, id, fn)
#define EVT_GRID_CMD_CELL_CHANGED(id, fn)        wx__DECLARE_GRIDEVT(CELL_CHANGED, id, fn)
#define EVT_GRID_CMD_SELECT_CELL(id, fn)         wx__DECLARE_GRIDEVT(SELECT_CELL, id, fn)
#define EVT_GRID_CMD_EDITOR_SHOWN(id, fn)        wx__DECLARE_GRIDEVT(EDITOR_SHOWN, id, fn)
#define EVT_GRID_CMD_EDITOR_HIDDEN(id, fn)       wx__DECLARE_GRIDEVT(EDITOR_HIDDEN, id, fn)
#define EVT_GRID_CMD_EDITOR_CREATED(id, fn)      wx__DECLARE_GRIDEDITOREVT(EDITOR_CREATED, id, fn)
#define EVT_GRID_CMD_CELL_BEGIN_DRAG(id, fn)     wx__DECLARE_GRIDEVT(CELL_BEGIN_DRAG, id, fn)
#define EVT_GRID_CMD_CHANGED(id, fn)             wx__DECLARE_GRIDEVT(CHANGED, id, fn)
#define EVT_GRID_CMD_TABBING(id, fn)             wx__DECLARE_GRIDEVT(TABBING, id, fn)

// same as above but for any id (exists mainly for backwards compatibility but
// then it's also true that you rarely have multiple grid in the same window)
#define EVT_GRID_CELL_LEFT_CLICK(fn)     EVT_GRID_CMD_CELL_LEFT_CLICK(wxID_ANY, fn)
#define EVT_GRID_CELL_RIGHT_CLICK(fn)    EVT_GRID_CMD_CELL_RIGHT_CLICK(wxID_ANY, fn)
#define EVT_GRID_CELL_LEFT_DCLICK(fn)    EVT_GRID_CMD_CELL_LEFT_DCLICK(wxID_ANY, fn)
#define EVT_GRID_CELL_RIGHT_DCLICK(fn)   EVT_GRID_CMD_CELL_RIGHT_DCLICK(wxID_ANY, fn)
#define EVT_GRID_LABEL_LEFT_CLICK(fn)    EVT_GRID_CMD_LABEL_LEFT_CLICK(wxID_ANY, fn)
#define EVT_GRID_LABEL_RIGHT_CLICK(fn)   EVT_GRID_CMD_LABEL_RIGHT_CLICK(wxID_ANY, fn)
#define EVT_GRID_LABEL_LEFT_DCLICK(fn)   EVT_GRID_CMD_LABEL_LEFT_DCLICK(wxID_ANY, fn)
#define EVT_GRID_LABEL_RIGHT_DCLICK(fn)  EVT_GRID_CMD_LABEL_RIGHT_DCLICK(wxID_ANY, fn)
#define EVT_GRID_ROW_SIZE(fn)            EVT_GRID_CMD_ROW_SIZE(wxID_ANY, fn)
#define EVT_GRID_COL_SIZE(fn)            EVT_GRID_CMD_COL_SIZE(wxID_ANY, fn)
#define EVT_GRID_COL_AUTO_SIZE(fn)       EVT_GRID_CMD_COL_AUTO_SIZE(wxID_ANY, fn)
#define EVT_GRID_ROW_MOVE(fn)            EVT_GRID_CMD_ROW_MOVE(wxID_ANY, fn)
#define EVT_GRID_COL_MOVE(fn)            EVT_GRID_CMD_COL_MOVE(wxID_ANY, fn)
#define EVT_GRID_COL_SORT(fn)            EVT_GRID_CMD_COL_SORT(wxID_ANY, fn)
#define EVT_GRID_RANGE_SELECTING(fn)     EVT_GRID_CMD_RANGE_SELECTING(wxID_ANY, fn)
#define EVT_GRID_RANGE_SELECTED(fn)      EVT_GRID_CMD_RANGE_SELECTED(wxID_ANY, fn)
#define EVT_GRID_CELL_CHANGING(fn)       EVT_GRID_CMD_CELL_CHANGING(wxID_ANY, fn)
#define EVT_GRID_CELL_CHANGED(fn)        EVT_GRID_CMD_CELL_CHANGED(wxID_ANY, fn)
#define EVT_GRID_SELECT_CELL(fn)         EVT_GRID_CMD_SELECT_CELL(wxID_ANY, fn)
#define EVT_GRID_EDITOR_SHOWN(fn)        EVT_GRID_CMD_EDITOR_SHOWN(wxID_ANY, fn)
#define EVT_GRID_EDITOR_HIDDEN(fn)       EVT_GRID_CMD_EDITOR_HIDDEN(wxID_ANY, fn)
#define EVT_GRID_EDITOR_CREATED(fn)      EVT_GRID_CMD_EDITOR_CREATED(wxID_ANY, fn)
#define EVT_GRID_CELL_BEGIN_DRAG(fn)     EVT_GRID_CMD_CELL_BEGIN_DRAG(wxID_ANY, fn)
#define EVT_GRID_CHANGED(fn)             EVT_GRID_CMD_CHANGED(wxID_ANY, fn)
#define EVT_GRID_TABBING(fn)             EVT_GRID_CMD_TABBING(wxID_ANY, fn)

// we used to have a single wxEVT_GRID_CELL_CHANGE event but it was split into
// wxEVT_GRID_CELL_CHANGING and CHANGED ones in wx 2.9.0, however the CHANGED
// is basically the same as the old CHANGE event so we keep the name for
// compatibility
#define wxEVT_GRID_CELL_CHANGE wxEVT_GRID_CELL_CHANGED

#define EVT_GRID_CMD_CELL_CHANGE EVT_GRID_CMD_CELL_CHANGED
#define EVT_GRID_CELL_CHANGE EVT_GRID_CELL_CHANGED

// same as above: RANGE_SELECT was split in RANGE_SELECTING and SELECTED in 3.2,
// but we keep the old name for compatibility
#define wxEVT_GRID_RANGE_SELECT wxEVT_GRID_RANGE_SELECTED

#define EVT_GRID_CMD_RANGE_SELECT EVT_GRID_CMD_RANGE_SELECTED
#define EVT_GRID_RANGE_SELECT EVT_GRID_RANGE_SELECTED


#if 0  // TODO: implement these ?  others ?

extern const int wxEVT_GRID_CREATE_CELL;
extern const int wxEVT_GRID_CHANGE_LABELS;
extern const int wxEVT_GRID_CHANGE_SEL_LABEL;

#define EVT_GRID_CREATE_CELL(fn)      wxDECLARE_EVENT_TABLE_ENTRY( wxEVT_GRID_CREATE_CELL,      wxID_ANY, wxID_ANY, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxGridExtEventFunction, &fn ), NULL ),
#define EVT_GRID_CHANGE_LABELS(fn)    wxDECLARE_EVENT_TABLE_ENTRY( wxEVT_GRID_CHANGE_LABELS,    wxID_ANY, wxID_ANY, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxGridExtEventFunction, &fn ), NULL ),
#define EVT_GRID_CHANGE_SEL_LABEL(fn) wxDECLARE_EVENT_TABLE_ENTRY( wxEVT_GRID_CHANGE_SEL_LABEL, wxID_ANY, wxID_ANY, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxGridExtEventFunction, &fn ), NULL ),

#endif

#endif // wxUSE_GRID

#endif // _WX_GENERIC_GRID_H_
