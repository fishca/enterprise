/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/datavcmn.cpp
// Purpose:     wxDataViewExtCtrl base classes and common parts
// Author:      Robert Roebling
// Created:     2006/02/20
// Copyright:   (c) 2006, Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#if wxUSE_DATAVIEWCTRL

#include "dataview.h"

#ifndef WX_PRECOMP
#include <wx/dc.h>
#include <wx/settings.h>
#include <wx/log.h>
#include <wx/crt.h>
#endif

#include <wx/datectrl.h>
#include <wx/except.h>
#include <wx/spinctrl.h>
#include <wx/choice.h>
#include <wx/imaglist.h>
#include <wx/renderer.h>
#include <wx/uilocale.h>

#if wxUSE_ACCESSIBILITY
#include <wx/access.h>
#endif // wxUSE_ACCESSIBILITY

// Uncomment this line to, for custom renderers, visually show the extent
// of both a cell and its item.
//#define DEBUG_RENDER_EXTENTS

const char wxDataViewExtCtrlNameStr[] = "dataviewCtrl";

namespace
{

	// Custom handler pushed on top of the edit control used by wxDataViewExtCtrl to
	// forward some events to the main control itself.
	class wxDataViewExtEditorCtrlEvtHandler : public wxEvtHandler
	{
	public:
		wxDataViewExtEditorCtrlEvtHandler(wxWindow* editor, wxDataViewExtRenderer* owner)
		{
			m_editorCtrl = editor;
			m_owner = owner;

			m_finished = false;
		}

		void SetFocusOnIdle(bool focus = true) { m_focusOnIdle = focus; }

	protected:
		void OnChar(wxKeyEvent& event);
		void OnTextEnter(wxCommandEvent& event);
		void OnKillFocus(wxFocusEvent& event);
		void OnIdle(wxIdleEvent& event);

	private:
		bool IsEditorSubControl(wxWindow* win) const;

		wxDataViewExtRenderer* m_owner;
		wxWindow* m_editorCtrl;
		bool                    m_finished;
		bool                    m_focusOnIdle;

	private:
		wxDECLARE_EVENT_TABLE();
	};

} // anonymous namespace

// ---------------------------------------------------------
// wxDataViewExtIndexListModel
// ---------------------------------------------------------

static int my_sort(int* v1, int* v2)
{
	return *v2 - *v1;
}

wxDataViewExtIndexListModel::wxDataViewExtIndexListModel(unsigned int initial_size)
{
	// IDs are ordered until an item gets deleted or inserted
	m_ordered = true;

	// build initial index
	unsigned int i;
	for (i = 1; i < initial_size + 1; i++)
		m_hash.Add(wxDataViewExtItem(wxUIntToPtr(i)));
	m_nextFreeID = initial_size + 1;
}

void wxDataViewExtIndexListModel::Reset(unsigned int new_size)
{
	/* wxDataViewExtModel:: */ BeforeReset();

	m_hash.Clear();

	// IDs are ordered until an item gets deleted or inserted
	m_ordered = true;

	// build initial index
	unsigned int i;
	for (i = 1; i < new_size + 1; i++)
		m_hash.Add(wxDataViewExtItem(wxUIntToPtr(i)));

	m_nextFreeID = new_size + 1;

	/* wxDataViewExtModel:: */ AfterReset();
}

void wxDataViewExtIndexListModel::RowPrepended()
{
	m_ordered = false;

	unsigned int id = m_nextFreeID;
	m_nextFreeID++;

	wxDataViewExtItem item(wxUIntToPtr(id));
	m_hash.Insert(item, 0);
	ItemAdded(wxDataViewExtItem(0), item);

}

void wxDataViewExtIndexListModel::RowInserted(unsigned int before)
{
	m_ordered = false;

	unsigned int id = m_nextFreeID;
	m_nextFreeID++;

	wxDataViewExtItem item(wxUIntToPtr(id));
	m_hash.Insert(item, before);
	ItemAdded(wxDataViewExtItem(0), item);
}

void wxDataViewExtIndexListModel::RowAppended()
{
	unsigned int id = m_nextFreeID;
	m_nextFreeID++;

	wxDataViewExtItem item(wxUIntToPtr(id));
	m_hash.Add(item);
	ItemAdded(wxDataViewExtItem(0), item);
}

void wxDataViewExtIndexListModel::RowDeleted(unsigned int row)
{
	m_ordered = false;

	wxDataViewExtItem item(m_hash[row]);
	m_hash.RemoveAt(row);
	/* wxDataViewExtModel:: */ ItemDeleted(wxDataViewExtItem(0), item);
}

void wxDataViewExtIndexListModel::RowsDeleted(const wxArrayInt& rows)
{
	m_ordered = false;

	wxDataViewExtItemArray array;
	unsigned int i;
	for (i = 0; i < rows.GetCount(); i++)
	{
		wxDataViewExtItem item(m_hash[rows[i]]);
		array.Add(item);
	}

	wxArrayInt sorted = rows;
	sorted.Sort(my_sort);
	for (i = 0; i < sorted.GetCount(); i++)
		m_hash.RemoveAt(sorted[i]);

	/* wxDataViewExtModel:: */ ItemsDeleted(wxDataViewExtItem(0), array);
}

void wxDataViewExtIndexListModel::RowChanged(unsigned int row)
{
	/* wxDataViewExtModel:: */ ItemChanged(GetItem(row));
}

void wxDataViewExtIndexListModel::RowValueChanged(unsigned int row, unsigned int col)
{
	/* wxDataViewExtModel:: */ ValueChanged(GetItem(row), col);
}

unsigned int wxDataViewExtIndexListModel::GetRow(const wxDataViewExtItem& item) const
{
	if (m_ordered)
		return wxPtrToUInt(item.GetID()) - 1;

	// assert for not found
	return (unsigned int)m_hash.Index(item);
}

wxDataViewExtItem wxDataViewExtIndexListModel::GetItem(unsigned int row) const
{
	wxCHECK_MSG(row < m_hash.GetCount(), wxDataViewExtItem(), wxS("invalid index"));
	return wxDataViewExtItem(m_hash[row]);
}

unsigned int wxDataViewExtIndexListModel::GetChildren(const wxDataViewExtItem& item, wxDataViewExtItemArray& children) const
{
	if (item.IsOk())
		return 0;

	children = m_hash;

	return m_hash.GetCount();
}

// ---------------------------------------------------------
// wxDataViewExtVirtualListModel
// ---------------------------------------------------------

#ifndef __WXMAC__

wxDataViewExtVirtualListModel::wxDataViewExtVirtualListModel(unsigned int initial_size)
{
	m_size = initial_size;
}

void wxDataViewExtVirtualListModel::Reset(unsigned int new_size)
{
	/* wxDataViewExtModel:: */ BeforeReset();

	m_size = new_size;

	/* wxDataViewExtModel:: */ AfterReset();
}

void wxDataViewExtVirtualListModel::RowPrepended()
{
	m_size++;
	wxDataViewExtItem item(wxUIntToPtr(1));
	ItemAdded(wxDataViewExtItem(0), item);
}

void wxDataViewExtVirtualListModel::RowInserted(unsigned int before)
{
	m_size++;
	wxDataViewExtItem item(wxUIntToPtr(before + 1));
	ItemAdded(wxDataViewExtItem(0), item);
}

void wxDataViewExtVirtualListModel::RowAppended()
{
	m_size++;
	wxDataViewExtItem item(wxUIntToPtr(m_size));
	ItemAdded(wxDataViewExtItem(0), item);
}

void wxDataViewExtVirtualListModel::RowDeleted(unsigned int row)
{
	m_size--;
	wxDataViewExtItem item(wxUIntToPtr(row + 1));
	/* wxDataViewExtModel:: */ ItemDeleted(wxDataViewExtItem(0), item);
}

void wxDataViewExtVirtualListModel::RowsDeleted(const wxArrayInt& rows)
{
	m_size -= rows.GetCount();

	wxArrayInt sorted = rows;
	sorted.Sort(my_sort);

	wxDataViewExtItemArray array;
	unsigned int i;
	for (i = 0; i < sorted.GetCount(); i++)
	{
		wxDataViewExtItem item(wxUIntToPtr(sorted[i] + 1));
		array.Add(item);
	}
	/* wxDataViewExtModel:: */ ItemsDeleted(wxDataViewExtItem(0), array);
}

void wxDataViewExtVirtualListModel::RowChanged(unsigned int row)
{
	/* wxDataViewExtModel:: */ ItemChanged(GetItem(row));
}

void wxDataViewExtVirtualListModel::RowValueChanged(unsigned int row, unsigned int col)
{
	/* wxDataViewExtModel:: */ ValueChanged(GetItem(row), col);
}

unsigned int wxDataViewExtVirtualListModel::GetRow(const wxDataViewExtItem& item) const
{
	return wxPtrToUInt(item.GetID()) - 1;
}

wxDataViewExtItem wxDataViewExtVirtualListModel::GetItem(unsigned int row) const
{
	return wxDataViewExtItem(wxUIntToPtr(row + 1));
}

bool wxDataViewExtVirtualListModel::HasDefaultCompare() const
{
	return true;
}

int wxDataViewExtVirtualListModel::Compare(const wxDataViewExtItem& item1,
	const wxDataViewExtItem& item2,
	unsigned int WXUNUSED(column),
	bool ascending) const
{
	unsigned int pos1 = wxPtrToUInt(item1.GetID());  // -1 not needed here
	unsigned int pos2 = wxPtrToUInt(item2.GetID());  // -1 not needed here

	if (ascending)
		return pos1 - pos2;
	else
		return pos2 - pos1;
}

unsigned int wxDataViewExtVirtualListModel::GetChildren(const wxDataViewExtItem& WXUNUSED(item), wxDataViewExtItemArray& WXUNUSED(children)) const
{
	return 0;  // should we report an error ?
}

#endif  // __WXMAC__

//-----------------------------------------------------------------------------
// wxDataViewExtIconText
//-----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(wxDataViewExtIconText, wxObject);

IMPLEMENT_VARIANT_OBJECT(wxDataViewExtIconText)

// ---------------------------------------------------------
// wxDataViewExtRendererBase
// ---------------------------------------------------------

wxIMPLEMENT_ABSTRACT_CLASS(wxDataViewExtRendererBase, wxObject);

wxDataViewExtRendererBase::wxDataViewExtRendererBase(const wxString& varianttype,
	wxDataViewExtCellMode WXUNUSED(mode),
	int WXUNUSED(align))
	: m_variantType(varianttype)
{
	m_owner = NULL;
	m_valueAdjuster = NULL;
}

wxDataViewExtRendererBase::~wxDataViewExtRendererBase()
{
	if (m_editorCtrl)
		DestroyEditControl();
	delete m_valueAdjuster;
}

wxDataViewExtCtrl* wxDataViewExtRendererBase::GetView() const
{
	return const_cast<wxDataViewExtRendererBase*>(this)->GetOwner()->GetOwner();
}

bool wxDataViewExtRendererBase::StartEditing(const wxDataViewExtItem& item, wxRect labelRect)
{
	wxDataViewExtColumn* const column = GetOwner();
	wxDataViewExtCtrl* const dv_ctrl = column->GetOwner();

	// Before doing anything we send an event asking if editing of this item is really wanted.
	wxDataViewExtEvent event(wxEVT_DATAVIEW_ITEM_START_EDITING, dv_ctrl, column, item);
	dv_ctrl->GetEventHandler()->ProcessEvent(event);
	if (!event.IsAllowed())
		return false;

	// Remember the item being edited for use in FinishEditing() later.
	m_item = item;

	unsigned int col = GetOwner()->GetModelColumn();
	const wxVariant& value = CheckedGetValue(dv_ctrl->GetModel(), item, col);

	m_editorCtrl = CreateEditorCtrl(
		(wxWindow*)dv_ctrl->CellToDataViewWindow(item, column), labelRect, value);

	// there might be no editor control for the given item
	if (!m_editorCtrl)
	{
		m_item = wxDataViewExtItem();
		return false;
	}

	wxDataViewExtEditorCtrlEvtHandler* handler =
		new wxDataViewExtEditorCtrlEvtHandler(m_editorCtrl, (wxDataViewExtRenderer*)this);

	m_editorCtrl->PushEventHandler(handler);

#if defined(__WXGTK20__)
	handler->SetFocusOnIdle();
#else
	m_editorCtrl->SetFocus();
#endif

	return true;
}

void wxDataViewExtRendererBase::NotifyEditingStarted(const wxDataViewExtItem& item)
{
	wxDataViewExtColumn* const column = GetOwner();
	wxDataViewExtCtrl* const dv_ctrl = column->GetOwner();

	wxDataViewExtEvent event(wxEVT_DATAVIEW_ITEM_EDITING_STARTED, dv_ctrl, column, item);
	dv_ctrl->GetEventHandler()->ProcessEvent(event);
}

void wxDataViewExtRendererBase::DestroyEditControl()
{
	// Remove our event handler first to prevent it from (recursively) calling
	// us again as it would do via a call to FinishEditing() when the editor
	// loses focus when we hide it below.
	wxEvtHandler* const handler = m_editorCtrl->PopEventHandler();

	// Hide the control immediately but don't delete it yet as there could be
	// some pending messages for it.
	m_editorCtrl->Hide();

	wxPendingDelete.Append(handler);
	wxPendingDelete.Append(m_editorCtrl);

	// Ensure that DestroyEditControl() is not called again for this control.
	m_editorCtrl.Release();
}

void wxDataViewExtRendererBase::CancelEditing()
{
	if (m_editorCtrl)
		DestroyEditControl();

	DoHandleEditingDone(NULL);
}

bool wxDataViewExtRendererBase::FinishEditing()
{
	if (!m_editorCtrl)
		return true;

	bool gotValue = false;

	wxVariant value;
	if (GetValueFromEditorCtrl(m_editorCtrl, value))
	{
		// This is the normal case and we will use this value below (if it
		// passes validation).
		gotValue = true;
	}
	//else: Not really supposed to happen, but still proceed with
	//      destroying the edit control if it does.

	DestroyEditControl();

	GetView()->GetMainWindow()->SetFocus();

	return DoHandleEditingDone(gotValue ? &value : NULL);
}

bool
wxDataViewExtRendererBase::DoHandleEditingDone(wxVariant* value)
{
	if (value)
	{
		if (!Validate(*value))
		{
			// Invalid value can't be used, so if it's the same as if we hadn't
			// got it in the first place.
			value = NULL;
		}
	}

	wxDataViewExtColumn* const column = GetOwner();
	wxDataViewExtCtrl* const dv_ctrl = column->GetOwner();
	unsigned int col = column->GetModelColumn();

	// Now we should send Editing Done event
	wxDataViewExtEvent event(wxEVT_DATAVIEW_ITEM_EDITING_DONE, dv_ctrl, column, m_item);
	if (value)
		event.SetValue(*value);
	else
		event.SetEditCancelled();

	dv_ctrl->GetEventHandler()->ProcessEvent(event);

	bool accepted = false;
	if (value && event.IsAllowed())
	{
		dv_ctrl->GetModel()->ChangeValue(*value, m_item, col);
		accepted = true;
	}

	m_item = wxDataViewExtItem();

	return accepted;
}

wxVariant
wxDataViewExtRendererBase::CheckedGetValue(const wxDataViewExtModel* model,
	const wxDataViewExtItem& item,
	unsigned column) const
{
	wxVariant value;
	// Avoid calling GetValue() if the model isn't supposed to have any values
	// in this cell (e.g. a non-first column of a container item), this could
	// be unexpected.
	if (model->HasValue(item, column))
		model->GetValue(value, item, column);

	// We always allow the cell to be null, regardless of the renderer type.
	if (!value.IsNull())
	{
		if (!IsCompatibleVariantType(value.GetType()))
		{
			// If you're seeing this message, this indicates that either your
			// renderer is using the wrong type, or your model returns values
			// of the wrong type.
			wxLogDebug("Wrong type returned from the model for column %u: "
				"%s required but actual type is %s",
				column,
				GetVariantType(),
				value.GetType());

			// Don't return data of mismatching type, this could be unexpected.
			value.MakeNull();
		}
	}

	return value;
}

bool
wxDataViewExtRendererBase::PrepareForItem(const wxDataViewExtModel* model,
	const wxDataViewExtItem& item,
	unsigned column)
{
	// This method is called by the native control, so we shouldn't allow
	// exceptions to escape from it.
	wxTRY
	{

		// Now check if we have a value and remember it if we do.
		wxVariant value = CheckedGetValue(model, item, column);

		if (!value.IsNull())
		{
			if (m_valueAdjuster)
			{
				if (IsHighlighted())
					value = m_valueAdjuster->MakeHighlighted(value);
			}

			SetValue(value);
		}

		// Also set up the attributes: note that we need to do this even for the
		// empty cells because background colour is still relevant for them.
		wxDataViewExtItemAttr attr;
		model->GetAttr(item, column, attr);
		SetAttr(attr);

		// Finally determine the enabled/disabled state and apply it, even to the
		// empty cells.
		SetEnabled(model->IsEnabled(item, column));

		return !value.IsNull();
	}
		wxCATCH_ALL
		(
			// There is not much we can do about it here, just log it and don't
			// show anything in this cell.
			wxLogDebug("Retrieving the value from the model threw an exception");
	return false;
		)
}


int wxDataViewExtRendererBase::GetEffectiveAlignment() const
{
	int alignment = GetEffectiveAlignmentIfKnown();
	wxASSERT(alignment != wxDVR_DEFAULT_ALIGNMENT);
	return alignment;
}


int wxDataViewExtRendererBase::GetEffectiveAlignmentIfKnown() const
{
	int alignment = GetAlignment();

	if (alignment == wxDVR_DEFAULT_ALIGNMENT)
	{
		if (GetOwner() != NULL)
		{
			// if we don't have an explicit alignment ourselves, use that of the
			// column in horizontal direction and default vertical alignment
			alignment = GetOwner()->GetAlignment() | wxALIGN_CENTRE_VERTICAL;
		}
	}

	return alignment;
}

// ----------------------------------------------------------------------------
// wxDataViewExtCustomRendererBase
// ----------------------------------------------------------------------------

bool wxDataViewExtCustomRendererBase::ActivateCell(const wxRect& cell,
	wxDataViewExtModel* model,
	const wxDataViewExtItem& item,
	unsigned int col,
	const wxMouseEvent* mouseEvent)
{
	return ActivateCell(cell, model, item, col, mouseEvent);
}

void wxDataViewExtCustomRendererBase::RenderBackground(wxDC* dc, const wxRect& rect)
{
	if (!m_attr.HasBackgroundColour())
		return;

	const wxColour& colour = m_attr.GetBackgroundColour();
	wxDCPenChanger changePen(*dc, colour);
	wxDCBrushChanger changeBrush(*dc, colour);

	dc->DrawRectangle(rect);
}

void
wxDataViewExtCustomRendererBase::WXCallRender(wxRect rectCell, wxDC* dc, int state)
{
	wxCHECK_RET(dc, "no DC to draw on in custom renderer?");

	// adjust the rectangle ourselves to account for the alignment
	wxRect rectItem = rectCell;
	const int align = GetEffectiveAlignment();

	const wxSize size = GetSize();

	// take alignment into account only if there is enough space, otherwise
	// show as much contents as possible
	//
	// notice that many existing renderers (e.g. wxDataViewExtSpinRenderer)
	// return hard-coded size which can be more than they need and if we
	// trusted their GetSize() we'd draw the text out of cell bounds
	// entirely

	if (size.x >= 0 && size.x < rectCell.width)
	{
		if (align & wxALIGN_CENTER_HORIZONTAL)
			rectItem.x += (rectCell.width - size.x) / 2;
		else if (align & wxALIGN_RIGHT)
			rectItem.x += rectCell.width - size.x;
		// else: wxALIGN_LEFT is the default

		rectItem.width = size.x;
	}

	if (size.y >= 0 && size.y < rectCell.height)
	{
		if (align & wxALIGN_CENTER_VERTICAL)
			rectItem.y += (rectCell.height - size.y) / 2;
		else if (align & wxALIGN_BOTTOM)
			rectItem.y += rectCell.height - size.y;
		// else: wxALIGN_TOP is the default

		rectItem.height = size.y;
	}


	// set up the DC attributes

	// override custom foreground with the standard one for the selected items
	// because we currently don't allow changing the selection background and
	// custom colours may be unreadable on it
	wxColour col;
	if (state & wxDATAVIEW_CELL_SELECTED)
		col = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
	else if (m_attr.HasColour())
		col = m_attr.GetColour();
	else // use default foreground
		col = GetOwner()->GetOwner()->GetForegroundColour();

	wxDCTextColourChanger changeFg(*dc, col);

	wxDCFontChanger changeFont(*dc);
	if (m_attr.HasFont())
		changeFont.Set(m_attr.GetEffectiveFont(dc->GetFont()));

#ifdef DEBUG_RENDER_EXTENTS
	{

		wxDCBrushChanger changeBrush(*dc, *wxTRANSPARENT_BRUSH);
		wxDCPenChanger changePen(*dc, *wxRED);

		dc->DrawRectangle(rectCell);

		dc->SetPen(*wxGREEN);
		dc->DrawRectangle(rectItem);

	}
#endif

	Render(rectItem, dc, state);
}

wxSize wxDataViewExtCustomRendererBase::GetTextExtent(const wxString& str) const
{
	const wxDataViewExtCtrl* view = GetView();

	if (m_attr.HasFont())
	{
		wxFont font(m_attr.GetEffectiveFont(view->GetFont()));
		wxSize size;
		view->GetTextExtent(str, &size.x, &size.y, NULL, NULL, &font);
		return size;
	}
	else
	{
		return view->GetTextExtent(str);
	}
}

void
wxDataViewExtCustomRendererBase::RenderText(const wxString& text,
	int xoffset,
	wxRect rect,
	wxDC* dc,
	int state)
{
	wxRect rectText = rect;
	rectText.x += xoffset;
	rectText.width -= xoffset;

	int flags = 0;
	if (state & wxDATAVIEW_CELL_SELECTED)
		flags |= wxCONTROL_SELECTED;
	if (!(GetOwner()->GetOwner()->IsEnabled() && GetEnabled()))
		flags |= wxCONTROL_DISABLED;

	wxRendererNative::Get().DrawItemText(
		GetOwner()->GetOwner(),
		*dc,
		text,
		rectText,
		GetEffectiveAlignment(),
		flags,
		GetEllipsizeMode());
}

void wxDataViewExtCustomRendererBase::SetEnabled(bool enabled)
{
	m_enabled = enabled;
}

//-----------------------------------------------------------------------------
// wxDataViewExtEditorCtrlEvtHandler
//-----------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(wxDataViewExtEditorCtrlEvtHandler, wxEvtHandler)
EVT_CHAR(wxDataViewExtEditorCtrlEvtHandler::OnChar)
EVT_KILL_FOCUS(wxDataViewExtEditorCtrlEvtHandler::OnKillFocus)
EVT_IDLE(wxDataViewExtEditorCtrlEvtHandler::OnIdle)
EVT_TEXT_ENTER(-1, wxDataViewExtEditorCtrlEvtHandler::OnTextEnter)
wxEND_EVENT_TABLE()

void wxDataViewExtEditorCtrlEvtHandler::OnIdle(wxIdleEvent& event)
{
	if (m_focusOnIdle)
	{
		m_focusOnIdle = false;

		// Ignore focused items within the compound editor control
		wxWindow* win = wxWindow::FindFocus();
		if (!IsEditorSubControl(win))
		{
			m_editorCtrl->SetFocus();
		}
	}

	event.Skip();
}

void wxDataViewExtEditorCtrlEvtHandler::OnTextEnter(wxCommandEvent& WXUNUSED(event))
{
	m_finished = true;
	m_owner->FinishEditing();
}

void wxDataViewExtEditorCtrlEvtHandler::OnChar(wxKeyEvent& event)
{
	switch (event.m_keyCode)
	{
	case WXK_ESCAPE:
		m_finished = true;
		m_owner->CancelEditing();
		break;

	case WXK_RETURN:
		if (!event.HasAnyModifiers())
		{
			m_finished = true;
			m_owner->FinishEditing();
			break;
		}
		wxFALLTHROUGH; // Ctrl/Alt/Shift-Enter is not handled specially

	default:
		event.Skip();
	}
}

void wxDataViewExtEditorCtrlEvtHandler::OnKillFocus(wxFocusEvent& event)
{
	// Ignore focus changes within the compound editor control
	wxWindow* win = event.GetWindow();
	if (IsEditorSubControl(win))
	{
		event.Skip();
		return;
	}

	if (!m_finished)
	{
		m_finished = true;
		m_owner->FinishEditing();
	}

	event.Skip();
}

bool wxDataViewExtEditorCtrlEvtHandler::IsEditorSubControl(wxWindow* win) const
{
	// Checks whether the given window belongs to the editor control
	// (is either the editor itself or a child of the compound editor).
	while (win)
	{
		if (win == m_editorCtrl)
		{
			return true;
		}

		win = win->GetParent();
	}

	return false;
}

// ---------------------------------------------------------
// wxDataViewExtColumnBase
// ---------------------------------------------------------

void wxDataViewExtColumnBase::Init(wxDataViewExtRenderer* renderer,
	unsigned int model_column)
{
	m_renderer = renderer;
	m_model_column = model_column;
	m_owner = NULL;
	m_renderer->SetOwner((wxDataViewExtColumn*)this);
}

wxDataViewExtColumnBase::~wxDataViewExtColumnBase()
{
	delete m_renderer;
}

// ---------------------------------------------------------
// wxDataViewExtCtrlBase
// ---------------------------------------------------------

wxIMPLEMENT_ABSTRACT_CLASS(wxDataViewExtCtrlBase, wxControl);

wxDataViewExtCtrlBase::wxDataViewExtCtrlBase()
{
	m_model = NULL;
	m_expander_column = 0;
	m_indent = 8;
}

wxDataViewExtCtrlBase::~wxDataViewExtCtrlBase()
{
	if (m_model)
	{
		m_model->DecRef();
		m_model = NULL;
	}
}

bool wxDataViewExtCtrlBase::AssociateModel(wxDataViewExtModel* model)
{
	if (m_model)
	{
		m_model->DecRef();   // discard old model, if any
	}

	// add our own reference to the new model:
	m_model = model;
	if (m_model)
	{
		m_model->IncRef();
	}

	return true;
}

wxDataViewExtModel* wxDataViewExtCtrlBase::GetModel()
{
	return m_model;
}

const wxDataViewExtModel* wxDataViewExtCtrlBase::GetModel() const
{
	return m_model;
}

void wxDataViewExtCtrlBase::Expand(const wxDataViewExtItem& item)
{
	ExpandAncestors(item);

	DoExpand(item, false);
}

void wxDataViewExtCtrlBase::ExpandChildren(const wxDataViewExtItem& item)
{
	ExpandAncestors(item);

	DoExpand(item, true);
}

void wxDataViewExtCtrlBase::ExpandAncestors(const wxDataViewExtItem& item)
{
	if (!m_model) return;

	if (!item.IsOk()) return;

	wxVector<wxDataViewExtItem> parentChain;

	// at first we get all the parents of the selected item
	wxDataViewExtItem parent = m_model->GetParent(item);
	while (parent.IsOk())
	{
		parentChain.push_back(parent);
		parent = m_model->GetParent(parent);
	}

	// then we expand the parents, starting at the root
	while (!parentChain.empty())
	{
		DoExpand(parentChain.back(), false);
		parentChain.pop_back();
	}
}

wxDataViewExtItem wxDataViewExtCtrlBase::GetCurrentItem() const
{
	return HasFlag(wxDV_MULTIPLE) ? DoGetCurrentItem()
		: GetSelection();
}

void wxDataViewExtCtrlBase::SetCurrentItem(const wxDataViewExtItem& item)
{
	wxCHECK_RET(item.IsOk(), "Can't make current an invalid item.");

	if (HasFlag(wxDV_MULTIPLE))
		DoSetCurrentItem(item);
	else
		Select(item);
}

wxDataViewExtItem wxDataViewExtCtrlBase::GetSelection() const
{
	if (GetSelectedItemsCount() != 1)
		return wxDataViewExtItem();

	wxDataViewExtItemArray selections;
	GetSelections(selections);
	return selections[0];
}

namespace
{

	// Helper to account for inconsistent signature of wxDataViewExtProgressRenderer
	// ctor: it takes an extra "label" argument as first parameter, unlike all the
	// other renderers.
	template <typename Renderer>
	struct RendererFactory
	{
		static Renderer*
			New(wxDataViewExtCellMode mode, int align)
		{
			return new Renderer(Renderer::GetDefaultType(), mode, align);
		}
	};

	template <>
	struct RendererFactory<wxDataViewExtProgressRenderer>
	{
		static wxDataViewExtProgressRenderer*
			New(wxDataViewExtCellMode mode, int align)
		{
			return new wxDataViewExtProgressRenderer(
				wxString(),
				wxDataViewExtProgressRenderer::GetDefaultType(),
				mode,
				align
			);
		}
	};

	template <typename Renderer, typename LabelType>
	wxDataViewExtColumn*
		CreateColumnWithRenderer(const LabelType& label,
			unsigned model_column,
			wxDataViewExtCellMode mode,
			int width,
			wxAlignment align,
			int flags)
	{
		// For compatibility reason, handle wxALIGN_NOT as wxDVR_DEFAULT_ALIGNMENT
		// when creating the renderer here because a lot of existing code,
		// including our own dataview sample, uses wxALIGN_NOT just because it's
		// the default value of the alignment argument in AppendXXXColumn()
		// methods, but this doesn't mean that it actually wants to top-align the
		// column text.
		//
		// This does make it impossible to create top-aligned text using these
		// functions, but it can always be done by creating the renderer with the
		// desired alignment explicitly and should be so rarely needed in practice
		// (without speaking that vertical alignment is completely unsupported in
		// native OS X version), that it's preferable to do the right thing by
		// default here rather than account for it.
		return new wxDataViewExtColumn(
			label,
			RendererFactory<Renderer>::New(
				mode,
				align & wxALIGN_BOTTOM
				? align
				: align | wxALIGN_CENTRE_VERTICAL
			),
			model_column,
			width,
			align,
			flags
		);
	}

	// Common implementation of all {Append,Prepend}XXXColumn() below.
	template <typename Renderer, typename LabelType>
	wxDataViewExtColumn*
		AppendColumnWithRenderer(wxDataViewExtCtrlBase* dvc,
			const LabelType& label,
			unsigned model_column,
			wxDataViewExtCellMode mode,
			int width,
			wxAlignment align,
			int flags)
	{
		wxDataViewExtColumn* const
			col = CreateColumnWithRenderer<Renderer>(
				label, model_column, mode, width, align, flags
			);

		dvc->AppendColumn(col);
		return col;
	}

	template <typename Renderer, typename LabelType>
	wxDataViewExtColumn*
		PrependColumnWithRenderer(wxDataViewExtCtrlBase* dvc,
			const LabelType& label,
			unsigned model_column,
			wxDataViewExtCellMode mode,
			int width,
			wxAlignment align,
			int flags)
	{
		wxDataViewExtColumn* const
			col = CreateColumnWithRenderer<Renderer>(
				label, model_column, mode, width, align, flags
			);

		dvc->PrependColumn(col);
		return col;
	}

} // anonymous namespace

wxDataViewExtColumn*
wxDataViewExtCtrlBase::AppendTextColumn(const wxString& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return AppendColumnWithRenderer<wxDataViewExtTextRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::AppendIconTextColumn(const wxString& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return AppendColumnWithRenderer<wxDataViewExtIconTextRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::AppendToggleColumn(const wxString& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return AppendColumnWithRenderer<wxDataViewExtToggleRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::AppendProgressColumn(const wxString& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return AppendColumnWithRenderer<wxDataViewExtProgressRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::AppendDateColumn(const wxString& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return AppendColumnWithRenderer<wxDataViewExtDateRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::AppendBitmapColumn(const wxString& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return AppendColumnWithRenderer<wxDataViewExtBitmapRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::AppendTextColumn(const wxBitmap& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return AppendColumnWithRenderer<wxDataViewExtTextRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::AppendIconTextColumn(const wxBitmap& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return AppendColumnWithRenderer<wxDataViewExtIconTextRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::AppendToggleColumn(const wxBitmap& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return AppendColumnWithRenderer<wxDataViewExtToggleRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::AppendProgressColumn(const wxBitmap& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return AppendColumnWithRenderer<wxDataViewExtProgressRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::AppendDateColumn(const wxBitmap& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return AppendColumnWithRenderer<wxDataViewExtDateRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::AppendBitmapColumn(const wxBitmap& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return AppendColumnWithRenderer<wxDataViewExtBitmapRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::PrependTextColumn(const wxString& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return PrependColumnWithRenderer<wxDataViewExtTextRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::PrependIconTextColumn(const wxString& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return PrependColumnWithRenderer<wxDataViewExtIconTextRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::PrependToggleColumn(const wxString& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return PrependColumnWithRenderer<wxDataViewExtToggleRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::PrependProgressColumn(const wxString& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return PrependColumnWithRenderer<wxDataViewExtProgressRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::PrependDateColumn(const wxString& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return PrependColumnWithRenderer<wxDataViewExtDateRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::PrependBitmapColumn(const wxString& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return PrependColumnWithRenderer<wxDataViewExtBitmapRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::PrependTextColumn(const wxBitmap& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return PrependColumnWithRenderer<wxDataViewExtTextRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::PrependIconTextColumn(const wxBitmap& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return PrependColumnWithRenderer<wxDataViewExtIconTextRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::PrependToggleColumn(const wxBitmap& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return PrependColumnWithRenderer<wxDataViewExtToggleRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::PrependProgressColumn(const wxBitmap& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return PrependColumnWithRenderer<wxDataViewExtProgressRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::PrependDateColumn(const wxBitmap& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return PrependColumnWithRenderer<wxDataViewExtDateRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

wxDataViewExtColumn*
wxDataViewExtCtrlBase::PrependBitmapColumn(const wxBitmap& label, unsigned int model_column,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	return PrependColumnWithRenderer<wxDataViewExtBitmapRenderer>(
		this, label, model_column, mode, width, align, flags
	);
}

bool
wxDataViewExtCtrlBase::AppendColumn(wxDataViewExtColumn* col)
{
	col->SetOwner((wxDataViewExtCtrl*)this);
	return true;
}

bool
wxDataViewExtCtrlBase::PrependColumn(wxDataViewExtColumn* col)
{
	col->SetOwner((wxDataViewExtCtrl*)this);
	return true;
}

bool
wxDataViewExtCtrlBase::InsertColumn(unsigned int WXUNUSED(pos), wxDataViewExtColumn* col)
{
	col->SetOwner((wxDataViewExtCtrl*)this);
	return true;
}

void wxDataViewExtCtrlBase::StartEditor(const wxDataViewExtItem& item, unsigned int column)
{
	EditItem(item, GetColumn(column));
}

#if wxUSE_DRAG_AND_DROP

/* static */
wxDataObjectComposite*
wxDataViewExtCtrlBase::CreateDataObject(const wxVector<wxDataFormat>& formats)
{
	if (formats.empty())
	{
		return NULL;
	}

	wxDataObjectComposite* dataObject(new wxDataObjectComposite);
	for (size_t i = 0; i < formats.size(); ++i)
	{
		switch (formats[i].GetType())
		{
		case wxDF_TEXT:
		case wxDF_OEMTEXT:
		case wxDF_UNICODETEXT:
			dataObject->Add(new wxTextDataObject);
			break;

		case wxDF_BITMAP:
		case wxDF_PNG:
			dataObject->Add(new wxBitmapDataObject);
			break;

		case wxDF_FILENAME:
			dataObject->Add(new wxFileDataObject);
			break;

		case wxDF_HTML:
			dataObject->Add(new wxHTMLDataObject);
			break;

		case wxDF_METAFILE:
		case wxDF_SYLK:
		case wxDF_DIF:
		case wxDF_TIFF:
		case wxDF_DIB:
		case wxDF_PALETTE:
		case wxDF_PENDATA:
		case wxDF_RIFF:
		case wxDF_WAVE:
		case wxDF_ENHMETAFILE:
		case wxDF_LOCALE:
		case wxDF_PRIVATE:
		default: // any other custom format
			dataObject->Add(new wxCustomDataObject(formats[i]));
			break;

		case wxDF_INVALID:
		case wxDF_MAX:
			break;
		}
	}

	return dataObject;
}

#endif // wxUSE_DRAG_AND_DROP

// ---------------------------------------------------------
// wxDataViewExtEvent
// ---------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(wxDataViewExtEvent, wxNotifyEvent);

wxDEFINE_EVENT(wxEVT_DATAVIEW_SELECTION_CHANGED, wxDataViewExtEvent);

wxDEFINE_EVENT(wxEVT_DATAVIEW_ITEM_ACTIVATED, wxDataViewExtEvent);
wxDEFINE_EVENT(wxEVT_DATAVIEW_ITEM_COLLAPSING, wxDataViewExtEvent);
wxDEFINE_EVENT(wxEVT_DATAVIEW_ITEM_COLLAPSED, wxDataViewExtEvent);
wxDEFINE_EVENT(wxEVT_DATAVIEW_ITEM_EXPANDING, wxDataViewExtEvent);
wxDEFINE_EVENT(wxEVT_DATAVIEW_ITEM_EXPANDED, wxDataViewExtEvent);
wxDEFINE_EVENT(wxEVT_DATAVIEW_ITEM_EDITING_STARTED, wxDataViewExtEvent);
wxDEFINE_EVENT(wxEVT_DATAVIEW_ITEM_START_EDITING, wxDataViewExtEvent);
wxDEFINE_EVENT(wxEVT_DATAVIEW_ITEM_EDITING_DONE, wxDataViewExtEvent);
wxDEFINE_EVENT(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewExtEvent);

wxDEFINE_EVENT(wxEVT_DATAVIEW_ITEM_START_INSERTING, wxDataViewExtEvent);
wxDEFINE_EVENT(wxEVT_DATAVIEW_ITEM_START_DELETING, wxDataViewExtEvent);

wxDEFINE_EVENT(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewExtEvent);

wxDEFINE_EVENT(wxEVT_DATAVIEW_COLUMN_HEADER_CLICK, wxDataViewExtEvent);
wxDEFINE_EVENT(wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK, wxDataViewExtEvent);
wxDEFINE_EVENT(wxEVT_DATAVIEW_COLUMN_SORTED, wxDataViewExtEvent);
wxDEFINE_EVENT(wxEVT_DATAVIEW_COLUMN_REORDERED, wxDataViewExtEvent);

wxDEFINE_EVENT(wxEVT_DATAVIEW_CACHE_HINT, wxDataViewExtEvent);

wxDEFINE_EVENT(wxEVT_DATAVIEW_ITEM_BEGIN_DRAG, wxDataViewExtEvent);
wxDEFINE_EVENT(wxEVT_DATAVIEW_ITEM_DROP_POSSIBLE, wxDataViewExtEvent);
wxDEFINE_EVENT(wxEVT_DATAVIEW_ITEM_DROP, wxDataViewExtEvent);

wxDEFINE_EVENT(wxEVT_DATAVIEW_VIEW_SET, wxDataViewExtEvent);

// Common part of non-copy ctors.
void wxDataViewExtEvent::Init(wxDataViewExtCtrlBase* dvc,
	wxDataViewExtColumn* column,
	const wxDataViewExtItem& item)
{
	m_item = item;
	m_col = column ? column->GetModelColumn() : -1;
	m_model = dvc ? dvc->GetModel() : NULL;
	m_column = column;
	m_pos = wxDefaultPosition;
	m_cacheFrom = 0;
	m_cacheTo = 0;
	m_editCancelled = false;
#if wxUSE_DRAG_AND_DROP
	m_dataObject = NULL;
	m_dataBuffer = NULL;
	m_dataSize = 0;
	m_dragFlags = 0;
	m_dropEffect = wxDragNone;
	m_proposedDropIndex = -1;
#endif // wxUSE_DRAG_AND_DROP

	m_viewMode = wxDataViewExtViewMode::wxDataViewExtList;

	SetEventObject(dvc);
}

#if wxUSE_DRAG_AND_DROP

void wxDataViewExtEvent::InitData(wxDataObjectComposite* obj, wxDataFormat format)
{
	SetDataFormat(format);

	SetDataObject(obj->GetObject(format));

	const size_t size = obj->GetDataSize(format);
	SetDataSize(size);

	if (size)
	{
		obj->GetDataHere(format, m_dataBuf.GetWriteBuf(size));
		m_dataBuf.UngetWriteBuf(size);

		SetDataBuffer(m_dataBuf.GetData());
	}
}

#endif // wxUSE_DRAG_AND_DROP

#if wxUSE_SPINCTRL

// -------------------------------------
// wxDataViewExtSpinRenderer
// -------------------------------------

wxDataViewExtSpinRenderer::wxDataViewExtSpinRenderer(int min, int max, wxDataViewExtCellMode mode, int alignment) :
	wxDataViewExtCustomRenderer(wxT("long"), mode, alignment)
{
	m_min = min;
	m_max = max;
}

wxWindow* wxDataViewExtSpinRenderer::CreateEditorCtrl(wxWindow* parent, wxRect labelRect, const wxVariant& value)
{
	long l = value;
	wxString str;
	str.Printf(wxT("%d"), (int)l);
	wxSpinCtrl* sc = new wxSpinCtrl(parent, wxID_ANY, str,
		labelRect.GetTopLeft(), labelRect.GetSize(), wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER, m_min, m_max, l);
#ifdef __WXMAC__
	const wxSize size = sc->GetSize();
	wxPoint pt = sc->GetPosition();
	sc->SetSize(pt.x - 4, pt.y - 4, size.x, size.y);
#endif

	return sc;
}

bool wxDataViewExtSpinRenderer::GetValueFromEditorCtrl(wxWindow* editor, wxVariant& value)
{
	wxSpinCtrl* sc = (wxSpinCtrl*)editor;
	long l = sc->GetValue();
	value = l;
	return true;
}

bool wxDataViewExtSpinRenderer::Render(wxRect rect, wxDC* dc, int state)
{
	wxString str;
	str.Printf(wxT("%d"), (int)m_data);
	RenderText(str, 0, rect, dc, state);
	return true;
}

wxSize wxDataViewExtSpinRenderer::GetSize() const
{
	wxSize sz = GetTextExtent(wxString::Format("%d", (int)m_data));

	// Allow some space for the spin buttons, which is approximately the size
	// of a scrollbar (and getting pixel-exact value would be complicated).
	// Also add some whitespace between the text and the button:
	sz.x += wxSystemSettings::GetMetric(wxSYS_VSCROLL_X, m_editorCtrl);
	sz.x += GetTextExtent("M").x;

	return sz;
}

bool wxDataViewExtSpinRenderer::SetValue(const wxVariant& value)
{
	m_data = value.GetLong();
	return true;
}

bool wxDataViewExtSpinRenderer::GetValue(wxVariant& value) const
{
	value = m_data;
	return true;
}

#if wxUSE_ACCESSIBILITY
wxString wxDataViewExtSpinRenderer::GetAccessibleDescription() const
{
	return wxString::Format(wxS("%li"), m_data);
}
#endif // wxUSE_ACCESSIBILITY

#endif // wxUSE_SPINCTRL

// -------------------------------------
// wxDataViewExtChoiceRenderer
// -------------------------------------

wxDataViewExtChoiceRenderer::wxDataViewExtChoiceRenderer(const wxArrayString& choices, wxDataViewExtCellMode mode, int alignment) :
	wxDataViewExtCustomRenderer(wxT("string"), mode, alignment)
{
	m_choices = choices;
}

wxWindow* wxDataViewExtChoiceRenderer::CreateEditorCtrl(wxWindow* parent, wxRect labelRect, const wxVariant& value)
{
	wxChoice* c = new wxChoice
	(
		parent,
		wxID_ANY,
		labelRect.GetTopLeft(),
		wxSize(labelRect.GetWidth(), -1),
		m_choices
	);
	c->Move(labelRect.GetRight() - c->GetRect().width, wxDefaultCoord);
	c->SetStringSelection(value.GetString());
	return c;
}

bool wxDataViewExtChoiceRenderer::GetValueFromEditorCtrl(wxWindow* editor, wxVariant& value)
{
	wxChoice* c = (wxChoice*)editor;
	wxString s = c->GetStringSelection();
	value = s;
	return true;
}

bool wxDataViewExtChoiceRenderer::Render(wxRect rect, wxDC* dc, int state)
{
	RenderText(m_data, 0, rect, dc, state);
	return true;
}

wxSize wxDataViewExtChoiceRenderer::GetSize() const
{
	wxSize sz;

	for (wxArrayString::const_iterator i = m_choices.begin(); i != m_choices.end(); ++i)
		sz.IncTo(GetTextExtent(*i));

	// Allow some space for the right-side button, which is approximately the
	// size of a scrollbar (and getting pixel-exact value would be complicated).
	// Also add some whitespace between the text and the button:
	sz.x += wxSystemSettings::GetMetric(wxSYS_VSCROLL_X, m_editorCtrl);
	sz.x += GetTextExtent("M").x;

	return sz;
}

bool wxDataViewExtChoiceRenderer::SetValue(const wxVariant& value)
{
	m_data = value.GetString();
	return true;
}

bool wxDataViewExtChoiceRenderer::GetValue(wxVariant& value) const
{
	value = m_data;
	return true;
}

#if wxUSE_ACCESSIBILITY
wxString wxDataViewExtChoiceRenderer::GetAccessibleDescription() const
{
	return m_data;
}
#endif // wxUSE_ACCESSIBILITY

// ----------------------------------------------------------------------------
// wxDataViewExtChoiceByIndexRenderer
// ----------------------------------------------------------------------------

wxDataViewExtChoiceByIndexRenderer::wxDataViewExtChoiceByIndexRenderer(const wxArrayString& choices,
	wxDataViewExtCellMode mode, int alignment) :
	wxDataViewExtChoiceRenderer(choices, mode, alignment)
{
	m_variantType = wxS("long");
}

wxWindow* wxDataViewExtChoiceByIndexRenderer::CreateEditorCtrl(wxWindow* parent, wxRect labelRect, const wxVariant& value)
{
	wxVariant string_value = value.GetLong() != wxNOT_FOUND ? GetChoice(value.GetLong())
		: wxString();

	return wxDataViewExtChoiceRenderer::CreateEditorCtrl(parent, labelRect, string_value);
}

bool wxDataViewExtChoiceByIndexRenderer::GetValueFromEditorCtrl(wxWindow* editor, wxVariant& value)
{
	wxVariant string_value;
	if (!wxDataViewExtChoiceRenderer::GetValueFromEditorCtrl(editor, string_value))
		return false;

	value = (long)GetChoices().Index(string_value.GetString());
	return true;
}

bool wxDataViewExtChoiceByIndexRenderer::SetValue(const wxVariant& value)
{
	wxVariant string_value = value.GetLong() != wxNOT_FOUND ? GetChoice(value.GetLong())
		: wxString();
	return wxDataViewExtChoiceRenderer::SetValue(string_value);
}

bool wxDataViewExtChoiceByIndexRenderer::GetValue(wxVariant& value) const
{
	wxVariant string_value;
	if (!wxDataViewExtChoiceRenderer::GetValue(string_value))
		return false;

	value = (long)GetChoices().Index(string_value.GetString());
	return true;
}

#if wxUSE_ACCESSIBILITY
wxString wxDataViewExtChoiceByIndexRenderer::GetAccessibleDescription() const
{
	wxVariant strVal;
	if (wxDataViewExtChoiceRenderer::GetValue(strVal))
		return strVal;

	return wxString::Format(wxS("%li"), (long)GetChoices().Index(strVal.GetString()));
}
#endif // wxUSE_ACCESSIBILITY

// ---------------------------------------------------------
// wxDataViewExtDateRenderer
// ---------------------------------------------------------

#if wxUSE_DATEPICKCTRL

wxDataViewExtDateRenderer::wxDataViewExtDateRenderer(const wxString& varianttype,
	wxDataViewExtCellMode mode, int align)
	: wxDataViewExtCustomRenderer(varianttype, mode, align)
{
}

wxWindow*
wxDataViewExtDateRenderer::CreateEditorCtrl(wxWindow* parent, wxRect labelRect, const wxVariant& value)
{
	return new wxDatePickerCtrl
	(
		parent,
		wxID_ANY,
		value.GetDateTime(),
		labelRect.GetTopLeft(),
		labelRect.GetSize()
	);
}

bool wxDataViewExtDateRenderer::GetValueFromEditorCtrl(wxWindow* editor, wxVariant& value)
{
	wxDatePickerCtrl* ctrl = static_cast<wxDatePickerCtrl*>(editor);
	value = ctrl->GetValue();
	return true;
}

bool wxDataViewExtDateRenderer::SetValue(const wxVariant& value)
{
	m_date = value.GetDateTime();
	return true;
}

bool wxDataViewExtDateRenderer::GetValue(wxVariant& value) const
{
	value = m_date;
	return true;
}

wxString wxDataViewExtDateRenderer::FormatDate() const
{
	return m_date.Format(wxGetUIDateFormat());
}

#if wxUSE_ACCESSIBILITY
wxString wxDataViewExtDateRenderer::GetAccessibleDescription() const
{
	return FormatDate();
}
#endif // wxUSE_ACCESSIBILITY

bool wxDataViewExtDateRenderer::Render(wxRect cell, wxDC* dc, int state)
{
	wxString tmp = FormatDate();
	RenderText(tmp, 0, cell, dc, state);
	return true;
}

wxSize wxDataViewExtDateRenderer::GetSize() const
{
	return GetTextExtent(FormatDate());
}

#endif // (defined(wxHAS_GENERIC_DATAVIEWCTRL) 

// ----------------------------------------------------------------------------
// wxDataViewExtCheckIconTextRenderer implementation
// ----------------------------------------------------------------------------

IMPLEMENT_VARIANT_OBJECT(wxDataViewExtCheckIconText)

wxIMPLEMENT_CLASS(wxDataViewExtCheckIconText, wxDataViewExtIconText);

wxIMPLEMENT_CLASS(wxDataViewExtCheckIconTextRenderer, wxDataViewExtRenderer);

wxDataViewExtCheckIconTextRenderer::wxDataViewExtCheckIconTextRenderer
(
	wxDataViewExtCellMode mode,
	int align
)
	: wxDataViewExtCustomRenderer(GetDefaultType(), mode, align)
{
	m_allow3rdStateForUser = false;
}

void wxDataViewExtCheckIconTextRenderer::Allow3rdStateForUser(bool allow)
{
	m_allow3rdStateForUser = allow;
}

bool wxDataViewExtCheckIconTextRenderer::SetValue(const wxVariant& value)
{
	m_value << value;
	return true;
}

bool wxDataViewExtCheckIconTextRenderer::GetValue(wxVariant& value) const
{
	value << m_value;
	return true;
}

#if wxUSE_ACCESSIBILITY
wxString wxDataViewExtCheckIconTextRenderer::GetAccessibleDescription() const
{
	wxString text = m_value.GetText();
	if (!text.empty())
	{
		text += wxS(" ");
	}

	switch (m_value.GetCheckedState())
	{
	case wxCHK_CHECKED:
		/* TRANSLATORS: Checkbox state name */
		text += _("checked");
		break;
	case wxCHK_UNCHECKED:
		/* TRANSLATORS: Checkbox state name */
		text += _("unchecked");
		break;
	case wxCHK_UNDETERMINED:
		/* TRANSLATORS: Checkbox state name */
		text += _("undetermined");
		break;
	}

	return text;
}
#endif // wxUSE_ACCESSIBILITY

wxSize wxDataViewExtCheckIconTextRenderer::GetSize() const
{
	wxSize size = GetCheckSize();
	size.x += MARGIN_CHECK_ICON;

	const wxBitmapBundle& bb = m_value.GetBitmapBundle();
	if (bb.IsOk())
	{
		const wxSize sizeIcon = bb.GetPreferredLogicalSizeFor(GetView());
		if (sizeIcon.y > size.y)
			size.y = sizeIcon.y;

		size.x += sizeIcon.x + MARGIN_ICON_TEXT;
	}

	wxString text = m_value.GetText();
	if (text.empty())
		text = "Dummy";

	const wxSize sizeText = GetTextExtent(text);
	if (sizeText.y > size.y)
		size.y = sizeText.y;

	size.x += sizeText.x;

	return size;
}

bool wxDataViewExtCheckIconTextRenderer::Render(wxRect cell, wxDC* dc, int state)
{
	/*
	Draw the text first because if the item has a background colour set
	then with wxGTK the entire cell is painted over during RenderText()
	when attributes are applied.
	*/

	const wxSize sizeCheck = GetCheckSize();

	int xoffset = sizeCheck.x + MARGIN_CHECK_ICON;

	wxRect rectIcon;
	const wxBitmapBundle& bb = m_value.GetBitmapBundle();
	const bool drawIcon = bb.IsOk();
	if (drawIcon)
	{
		const wxSize sizeIcon = bb.GetPreferredLogicalSizeFor(GetView());
		rectIcon = wxRect(cell.GetPosition(), sizeIcon);
		rectIcon.x += xoffset;
		rectIcon = rectIcon.CentreIn(cell, wxVERTICAL);

		xoffset += sizeIcon.x + MARGIN_ICON_TEXT;
	}

	RenderText(m_value.GetText(), xoffset, cell, dc, state);

	// Then draw the checkbox.
	int renderFlags = 0;
	switch (m_value.GetCheckedState())
	{
	case wxCHK_UNCHECKED:
		break;

	case wxCHK_CHECKED:
		renderFlags |= wxCONTROL_CHECKED;
		break;

	case wxCHK_UNDETERMINED:
		renderFlags |= wxCONTROL_UNDETERMINED;
		break;
	}

	if (state & wxDATAVIEW_CELL_PRELIT)
		renderFlags |= wxCONTROL_CURRENT;

	wxRect rectCheck(cell.GetPosition(), sizeCheck);
	rectCheck = rectCheck.CentreIn(cell, wxVERTICAL);

	wxRendererNative::Get().DrawCheckBox
	(
		GetView(), *dc, rectCheck, renderFlags
	);

	// Finally draw the icon, if any.
	if (drawIcon)
		dc->DrawIcon(bb.GetIconFor(GetView()), rectIcon.GetPosition());

	return true;
}

bool
wxDataViewExtCheckIconTextRenderer::ActivateCell(const wxRect& WXUNUSED(cell),
	wxDataViewExtModel* model,
	const wxDataViewExtItem& item,
	unsigned int col,
	const wxMouseEvent* mouseEvent)
{
	if (mouseEvent)
	{
		if (!wxRect(GetCheckSize()).Contains(mouseEvent->GetPosition()))
			return false;
	}

	// If the 3rd state is user-settable then the cycle is
	// unchecked->checked->undetermined.
	wxCheckBoxState checkedState = m_value.GetCheckedState();
	switch (checkedState)
	{
	case wxCHK_CHECKED:
		checkedState = m_allow3rdStateForUser ? wxCHK_UNDETERMINED
			: wxCHK_UNCHECKED;
		break;

	case wxCHK_UNDETERMINED:
		// Whether 3rd state is user-settable or not, the next state is
		// unchecked.
		checkedState = wxCHK_UNCHECKED;
		break;

	case wxCHK_UNCHECKED:
		checkedState = wxCHK_CHECKED;
		break;
	}

	m_value.SetCheckedState(checkedState);

	wxVariant value;
	value << m_value;

	model->ChangeValue(value, item, col);
	return true;
}

wxSize wxDataViewExtCheckIconTextRenderer::GetCheckSize() const
{
	return wxRendererNative::Get().GetCheckBoxSize(GetView());
}

//-----------------------------------------------------------------------------
// wxDataViewExtListStore
//-----------------------------------------------------------------------------

wxDataViewExtListStore::wxDataViewExtListStore()
{
}

wxDataViewExtListStore::~wxDataViewExtListStore()
{
	wxVector<wxDataViewExtListStoreLine*>::iterator it;
	for (it = m_data.begin(); it != m_data.end(); ++it)
	{
		wxDataViewExtListStoreLine* line = *it;
		delete line;
	}
}

void wxDataViewExtListStore::PrependColumn(const wxString& varianttype)
{
	m_cols.Insert(varianttype, 0);
}

void wxDataViewExtListStore::InsertColumn(unsigned int pos, const wxString& varianttype)
{
	m_cols.Insert(varianttype, pos);
}

void wxDataViewExtListStore::AppendColumn(const wxString& varianttype)
{
	m_cols.Add(varianttype);
}

unsigned int wxDataViewExtListStore::GetItemCount() const
{
	return m_data.size();
}

void wxDataViewExtListStore::AppendItem(const wxVector<wxVariant>& values, wxUIntPtr data)
{
	wxCHECK_RET(m_data.empty() || values.size() == m_data[0]->m_values.size(),
		"wrong number of values");

	wxDataViewExtListStoreLine* line = new wxDataViewExtListStoreLine(data);
	line->m_values = values;
	m_data.push_back(line);

	RowAppended();
}

void wxDataViewExtListStore::PrependItem(const wxVector<wxVariant>& values, wxUIntPtr data)
{
	wxCHECK_RET(m_data.empty() || values.size() == m_data[0]->m_values.size(),
		"wrong number of values");

	wxDataViewExtListStoreLine* line = new wxDataViewExtListStoreLine(data);
	line->m_values = values;
	m_data.insert(m_data.begin(), line);

	RowPrepended();
}

void wxDataViewExtListStore::InsertItem(unsigned int row, const wxVector<wxVariant>& values,
	wxUIntPtr data)
{
	wxCHECK_RET(m_data.empty() || values.size() == m_data[0]->m_values.size(),
		"wrong number of values");

	wxDataViewExtListStoreLine* line = new wxDataViewExtListStoreLine(data);
	line->m_values = values;
	m_data.insert(m_data.begin() + row, line);

	RowInserted(row);
}

void wxDataViewExtListStore::DeleteItem(unsigned int row)
{
	wxVector<wxDataViewExtListStoreLine*>::iterator it = m_data.begin() + row;
	delete* it;
	m_data.erase(it);

	RowDeleted(row);
}

void wxDataViewExtListStore::DeleteAllItems()
{
	wxVector<wxDataViewExtListStoreLine*>::iterator it;
	for (it = m_data.begin(); it != m_data.end(); ++it)
	{
		wxDataViewExtListStoreLine* line = *it;
		delete line;
	}

	m_data.clear();

	Reset(0);
}

void wxDataViewExtListStore::ClearColumns()
{
	m_cols.clear();
}

void wxDataViewExtListStore::SetItemData(const wxDataViewExtItem& item, wxUIntPtr data)
{
	wxDataViewExtListStoreLine* line = m_data[GetRow(item)];
	if (!line) return;

	line->SetData(data);
}

wxUIntPtr wxDataViewExtListStore::GetItemData(const wxDataViewExtItem& item) const
{
	wxDataViewExtListStoreLine* line = m_data[GetRow(item)];
	if (!line) return 0;

	return line->GetData();
}

void wxDataViewExtListStore::GetValueByRow(wxVariant& value, unsigned int row, unsigned int col) const
{
	wxDataViewExtListStoreLine* line = m_data[row];
	value = line->m_values[col];
}

bool wxDataViewExtListStore::SetValueByRow(const wxVariant& value, unsigned int row, unsigned int col)
{
	wxDataViewExtListStoreLine* line = m_data[row];
	line->m_values[col] = value;

	return true;
}

//-----------------------------------------------------------------------------
// wxDataViewExtListCtrl
//-----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(wxDataViewExtListCtrl, wxDataViewExtCtrl);

wxDataViewExtListCtrl::wxDataViewExtListCtrl()
{
}

wxDataViewExtListCtrl::wxDataViewExtListCtrl(wxWindow* parent, wxWindowID id,
	const wxPoint& pos, const wxSize& size, long style,
	const wxValidator& validator)
{
	Create(parent, id, pos, size, style, validator);
}

wxDataViewExtListCtrl::~wxDataViewExtListCtrl()
{
}

bool wxDataViewExtListCtrl::Create(wxWindow* parent, wxWindowID id,
	const wxPoint& pos, const wxSize& size, long style,
	const wxValidator& validator)
{
	if (!wxDataViewExtCtrl::Create(parent, id, pos, size, style, validator))
		return false;

	wxDataViewExtListStore* store = new wxDataViewExtListStore;
	AssociateModel(store);
	store->DecRef();

	return true;
}

bool wxDataViewExtListCtrl::AppendColumn(wxDataViewExtColumn* column, const wxString& varianttype)
{
	GetStore()->AppendColumn(varianttype);
	return wxDataViewExtCtrl::AppendColumn(column);
}

bool wxDataViewExtListCtrl::PrependColumn(wxDataViewExtColumn* column, const wxString& varianttype)
{
	GetStore()->PrependColumn(varianttype);
	return wxDataViewExtCtrl::PrependColumn(column);
}

bool wxDataViewExtListCtrl::InsertColumn(unsigned int pos, wxDataViewExtColumn* column, const wxString& varianttype)
{
	GetStore()->InsertColumn(pos, varianttype);
	return wxDataViewExtCtrl::InsertColumn(pos, column);
}

bool wxDataViewExtListCtrl::PrependColumn(wxDataViewExtColumn* col)
{
	return PrependColumn(col, col->GetRenderer()->GetVariantType());
}

bool wxDataViewExtListCtrl::InsertColumn(unsigned int pos, wxDataViewExtColumn* col)
{
	return InsertColumn(pos, col, col->GetRenderer()->GetVariantType());
}

bool wxDataViewExtListCtrl::AppendColumn(wxDataViewExtColumn* col)
{
	return AppendColumn(col, col->GetRenderer()->GetVariantType());
}

bool wxDataViewExtListCtrl::ClearColumns()
{
	GetStore()->ClearColumns();
	return wxDataViewExtCtrl::ClearColumns();
}

wxDataViewExtColumn* wxDataViewExtListCtrl::AppendTextColumn(const wxString& label,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	GetStore()->AppendColumn(wxT("string"));

	wxDataViewExtColumn* ret = new wxDataViewExtColumn(label,
		new wxDataViewExtTextRenderer(wxT("string"), mode),
		GetColumnCount(), width, align, flags);

	wxDataViewExtCtrl::AppendColumn(ret);

	return ret;
}

wxDataViewExtColumn* wxDataViewExtListCtrl::AppendToggleColumn(const wxString& label,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	GetStore()->AppendColumn(wxT("bool"));

	wxDataViewExtColumn* ret = new wxDataViewExtColumn(label,
		new wxDataViewExtToggleRenderer(wxT("bool"), mode),
		GetColumnCount(), width, align, flags);

	return wxDataViewExtCtrl::AppendColumn(ret) ? ret : NULL;
}

wxDataViewExtColumn* wxDataViewExtListCtrl::AppendProgressColumn(const wxString& label,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	GetStore()->AppendColumn(wxT("long"));

	wxDataViewExtColumn* ret = new wxDataViewExtColumn(label,
		new wxDataViewExtProgressRenderer(wxEmptyString, wxT("long"), mode),
		GetColumnCount(), width, align, flags);

	return wxDataViewExtCtrl::AppendColumn(ret) ? ret : NULL;
}

wxDataViewExtColumn* wxDataViewExtListCtrl::AppendIconTextColumn(const wxString& label,
	wxDataViewExtCellMode mode, int width, wxAlignment align, int flags)
{
	GetStore()->AppendColumn(wxT("wxDataViewExtIconText"));

	wxDataViewExtColumn* ret = new wxDataViewExtColumn(label,
		new wxDataViewExtIconTextRenderer(wxT("wxDataViewExtIconText"), mode),
		GetColumnCount(), width, align, flags);

	return wxDataViewExtCtrl::AppendColumn(ret) ? ret : NULL;
}

//-----------------------------------------------------------------------------
// wxDataViewExtTreeStore
//-----------------------------------------------------------------------------

wxDataViewExtTreeStoreNode::wxDataViewExtTreeStoreNode(
	wxDataViewExtTreeStoreNode* parent,
	const wxString& text, const wxBitmapBundle& icon, wxClientData* data)
	: m_text(text)
	, m_icon(icon)
{
	m_parent = parent;
	m_data = data;
}

wxDataViewExtTreeStoreNode::~wxDataViewExtTreeStoreNode()
{
	delete m_data;
}

wxDataViewExtTreeStoreContainerNode::wxDataViewExtTreeStoreContainerNode(
	wxDataViewExtTreeStoreNode* parent, const wxString& text,
	const wxBitmapBundle& icon, const wxBitmapBundle& expanded, wxClientData* data)
	: wxDataViewExtTreeStoreNode(parent, text, icon, data)
	, m_iconExpanded(expanded)
{
	m_isExpanded = false;
}

wxDataViewExtTreeStoreContainerNode::~wxDataViewExtTreeStoreContainerNode()
{
	DestroyChildren();
}

wxDataViewExtTreeStoreNodes::iterator
wxDataViewExtTreeStoreContainerNode::FindChild(wxDataViewExtTreeStoreNode* node)
{
	wxDataViewExtTreeStoreNodes::iterator iter;
	for (iter = m_children.begin(); iter != m_children.end(); ++iter)
	{
		if (*iter == node)
			break;
	}

	return iter;
}

void wxDataViewExtTreeStoreContainerNode::DestroyChildren()
{
	wxDataViewExtTreeStoreNodes::const_iterator iter;
	for (iter = m_children.begin(); iter != m_children.end(); ++iter)
	{
		delete* iter;
	}

	m_children.clear();
}

//-----------------------------------------------------------------------------

wxDataViewExtTreeStore::wxDataViewExtTreeStore()
{
	m_root = new wxDataViewExtTreeStoreContainerNode(NULL, wxEmptyString);
}

wxDataViewExtTreeStore::~wxDataViewExtTreeStore()
{
	delete m_root;
}

wxDataViewExtItem wxDataViewExtTreeStore::AppendItem(const wxDataViewExtItem& parent,
	const wxString& text, const wxBitmapBundle& icon, wxClientData* data)
{
	wxDataViewExtTreeStoreContainerNode* parent_node = FindContainerNode(parent);
	if (!parent_node) return wxDataViewExtItem(0);

	wxDataViewExtTreeStoreNode* node =
		new wxDataViewExtTreeStoreNode(parent_node, text, icon, data);
	parent_node->GetChildren().push_back(node);

	return node->GetItem();
}

wxDataViewExtItem wxDataViewExtTreeStore::PrependItem(const wxDataViewExtItem& parent,
	const wxString& text, const wxBitmapBundle& icon, wxClientData* data)
{
	wxDataViewExtTreeStoreContainerNode* parent_node = FindContainerNode(parent);
	if (!parent_node) return wxDataViewExtItem(0);

	wxDataViewExtTreeStoreNode* node =
		new wxDataViewExtTreeStoreNode(parent_node, text, icon, data);
	wxDataViewExtTreeStoreNodes& children = parent_node->GetChildren();
	children.insert(children.begin(), node);

	return node->GetItem();
}

wxDataViewExtItem
wxDataViewExtTreeStore::InsertItem(const wxDataViewExtItem& parent,
	const wxDataViewExtItem& previous,
	const wxString& text,
	const wxBitmapBundle& icon,
	wxClientData* data)
{
	wxDataViewExtTreeStoreContainerNode* parent_node = FindContainerNode(parent);
	if (!parent_node) return wxDataViewExtItem(0);

	wxDataViewExtTreeStoreNode* previous_node = FindNode(previous);
	wxDataViewExtTreeStoreNodes& children = parent_node->GetChildren();
	const wxDataViewExtTreeStoreNodes::iterator iter = parent_node->FindChild(previous_node);
	if (iter == children.end()) return wxDataViewExtItem(0);

	wxDataViewExtTreeStoreNode* node =
		new wxDataViewExtTreeStoreNode(parent_node, text, icon, data);
	children.insert(iter, node);

	return node->GetItem();
}

wxDataViewExtItem wxDataViewExtTreeStore::PrependContainer(const wxDataViewExtItem& parent,
	const wxString& text, const wxBitmapBundle& icon, const wxBitmapBundle& expanded,
	wxClientData* data)
{
	wxDataViewExtTreeStoreContainerNode* parent_node = FindContainerNode(parent);
	if (!parent_node) return wxDataViewExtItem(0);

	wxDataViewExtTreeStoreContainerNode* node =
		new wxDataViewExtTreeStoreContainerNode(parent_node, text, icon, expanded, data);
	wxDataViewExtTreeStoreNodes& children = parent_node->GetChildren();
	children.insert(children.begin(), node);

	return node->GetItem();
}

wxDataViewExtItem
wxDataViewExtTreeStore::AppendContainer(const wxDataViewExtItem& parent,
	const wxString& text,
	const wxBitmapBundle& icon,
	const wxBitmapBundle& expanded,
	wxClientData* data)
{
	wxDataViewExtTreeStoreContainerNode* parent_node = FindContainerNode(parent);
	if (!parent_node) return wxDataViewExtItem(0);

	wxDataViewExtTreeStoreContainerNode* node =
		new wxDataViewExtTreeStoreContainerNode(parent_node, text, icon, expanded, data);
	parent_node->GetChildren().push_back(node);

	return node->GetItem();
}

wxDataViewExtItem
wxDataViewExtTreeStore::InsertContainer(const wxDataViewExtItem& parent,
	const wxDataViewExtItem& previous,
	const wxString& text,
	const wxBitmapBundle& icon,
	const wxBitmapBundle& expanded,
	wxClientData* data)
{
	wxDataViewExtTreeStoreContainerNode* parent_node = FindContainerNode(parent);
	if (!parent_node) return wxDataViewExtItem(0);

	wxDataViewExtTreeStoreNode* previous_node = FindNode(previous);
	wxDataViewExtTreeStoreNodes& children = parent_node->GetChildren();
	const wxDataViewExtTreeStoreNodes::iterator iter = parent_node->FindChild(previous_node);
	if (iter == children.end()) return wxDataViewExtItem(0);

	wxDataViewExtTreeStoreContainerNode* node =
		new wxDataViewExtTreeStoreContainerNode(parent_node, text, icon, expanded, data);
	children.insert(iter, node);

	return node->GetItem();
}

bool wxDataViewExtTreeStore::IsContainer(const wxDataViewExtItem& item) const
{
	wxDataViewExtTreeStoreNode* node = FindNode(item);
	if (!node) return false;

	return node->IsContainer();
}

wxDataViewExtItem wxDataViewExtTreeStore::GetNthChild(const wxDataViewExtItem& parent, unsigned int pos) const
{
	wxDataViewExtTreeStoreContainerNode* parent_node = FindContainerNode(parent);
	if (!parent_node) return wxDataViewExtItem(0);

	wxDataViewExtTreeStoreNode* const node = parent_node->GetChildren()[pos];
	if (node)
		return node->GetItem();

	return wxDataViewExtItem(0);
}

int wxDataViewExtTreeStore::GetChildCount(const wxDataViewExtItem& parent) const
{
	wxDataViewExtTreeStoreNode* node = FindNode(parent);
	if (!node) return -1;

	if (!node->IsContainer())
		return 0;

	wxDataViewExtTreeStoreContainerNode* container_node = (wxDataViewExtTreeStoreContainerNode*)node;
	return (int)container_node->GetChildren().size();
}

void wxDataViewExtTreeStore::SetItemText(const wxDataViewExtItem& item, const wxString& text)
{
	wxDataViewExtTreeStoreNode* node = FindNode(item);
	if (!node) return;

	node->SetText(text);
}

wxString wxDataViewExtTreeStore::GetItemText(const wxDataViewExtItem& item) const
{
	wxDataViewExtTreeStoreNode* node = FindNode(item);
	if (!node) return wxEmptyString;

	return node->GetText();
}

void wxDataViewExtTreeStore::SetItemIcon(const wxDataViewExtItem& item, const wxBitmapBundle& icon)
{
	wxDataViewExtTreeStoreNode* node = FindNode(item);
	if (!node) return;

	node->SetIcon(icon);
}

wxIcon wxDataViewExtTreeStore::GetItemIcon(const wxDataViewExtItem& item) const
{
	wxDataViewExtTreeStoreNode* node = FindNode(item);
	if (!node) return wxNullIcon;

	return node->GetIcon();
}

void wxDataViewExtTreeStore::SetItemExpandedIcon(const wxDataViewExtItem& item, const wxBitmapBundle& icon)
{
	wxDataViewExtTreeStoreContainerNode* node = FindContainerNode(item);
	if (!node) return;

	node->SetExpandedIcon(icon);
}

wxIcon wxDataViewExtTreeStore::GetItemExpandedIcon(const wxDataViewExtItem& item) const
{
	wxDataViewExtTreeStoreContainerNode* node = FindContainerNode(item);
	if (!node) return wxNullIcon;

	return node->GetExpandedIcon();
}

void wxDataViewExtTreeStore::SetItemData(const wxDataViewExtItem& item, wxClientData* data)
{
	wxDataViewExtTreeStoreNode* node = FindNode(item);
	if (!node) return;

	node->SetData(data);
}

wxClientData* wxDataViewExtTreeStore::GetItemData(const wxDataViewExtItem& item) const
{
	wxDataViewExtTreeStoreNode* node = FindNode(item);
	if (!node) return NULL;

	return node->GetData();
}

void wxDataViewExtTreeStore::DeleteItem(const wxDataViewExtItem& item)
{
	if (!item.IsOk()) return;

	wxDataViewExtItem parent_item = GetParent(item);

	wxDataViewExtTreeStoreContainerNode* parent_node = FindContainerNode(parent_item);
	if (!parent_node) return;

	const wxDataViewExtTreeStoreNodes::iterator
		iter = parent_node->FindChild(FindNode(item));
	if (iter != parent_node->GetChildren().end())
	{
		delete* iter;
		parent_node->GetChildren().erase(iter);
	}
}

void wxDataViewExtTreeStore::DeleteChildren(const wxDataViewExtItem& item)
{
	wxDataViewExtTreeStoreContainerNode* node = FindContainerNode(item);
	if (!node) return;

	node->DestroyChildren();
}

void wxDataViewExtTreeStore::DeleteAllItems()
{
	DeleteChildren(wxDataViewExtItem(m_root));
}

void
wxDataViewExtTreeStore::GetValue(wxVariant& variant,
	const wxDataViewExtItem& item,
	unsigned int WXUNUSED(col)) const
{
	// if (col != 0) return;

	wxDataViewExtTreeStoreNode* node = FindNode(item);
	if (!node) return;

	wxBitmapBundle bb;
	if (node->IsContainer())
	{
		wxDataViewExtTreeStoreContainerNode* container = (wxDataViewExtTreeStoreContainerNode*)node;
		if (container->IsExpanded())
			bb = container->GetExpandedBitmapBundle();
	}

	if (!bb.IsOk())
		bb = node->GetBitmapBundle();

	wxDataViewExtIconText data(node->GetText(), bb);

	variant << data;
}

bool
wxDataViewExtTreeStore::SetValue(const wxVariant& variant,
	const wxDataViewExtItem& item,
	unsigned int WXUNUSED(col))
{
	// if (col != 0) return false;

	wxDataViewExtTreeStoreNode* node = FindNode(item);
	if (!node) return false;

	wxDataViewExtIconText data;

	data << variant;

	node->SetText(data.GetText());
	node->SetIcon(data.GetIcon());

	return true;
}

wxDataViewExtItem wxDataViewExtTreeStore::GetParent(const wxDataViewExtItem& item) const
{
	wxDataViewExtTreeStoreNode* node = FindNode(item);
	if (!node) return wxDataViewExtItem(0);

	wxDataViewExtTreeStoreNode* parent = node->GetParent();
	if (!parent) return wxDataViewExtItem(0);

	if (parent == m_root)
		return wxDataViewExtItem(0);

	return parent->GetItem();
}

unsigned int wxDataViewExtTreeStore::GetChildren(const wxDataViewExtItem& item, wxDataViewExtItemArray& children) const
{
	wxDataViewExtTreeStoreContainerNode* node = FindContainerNode(item);
	if (!node) return 0;

	wxDataViewExtTreeStoreNodes::iterator iter;
	for (iter = node->GetChildren().begin(); iter != node->GetChildren().end(); ++iter)
	{
		wxDataViewExtTreeStoreNode* child = *iter;
		children.Add(child->GetItem());
	}

	return node->GetChildren().size();
}

int wxDataViewExtTreeStore::Compare(const wxDataViewExtItem& item1, const wxDataViewExtItem& item2,
	unsigned int WXUNUSED(column), bool WXUNUSED(ascending)) const
{
	wxDataViewExtTreeStoreNode* node1 = FindNode(item1);
	wxDataViewExtTreeStoreNode* node2 = FindNode(item2);

	if (!node1 || !node2 || (node1 == node2))
		return 0;

	wxDataViewExtTreeStoreContainerNode* const parent =
		(wxDataViewExtTreeStoreContainerNode*)node1->GetParent();

	wxCHECK_MSG(node2->GetParent() == parent, 0,
		wxS("Comparing items with different parent."));

	if (node1->IsContainer() && !node2->IsContainer())
		return -1;

	if (node2->IsContainer() && !node1->IsContainer())
		return 1;

	wxDataViewExtTreeStoreNodes::const_iterator iter;
	for (iter = parent->GetChildren().begin(); iter != parent->GetChildren().end(); ++iter)
	{
		if (*iter == node1)
			return -1;

		if (*iter == node2)
			return 1;
	}

	wxFAIL_MSG(wxS("Unreachable"));
	return 0;
}

wxDataViewExtTreeStoreNode* wxDataViewExtTreeStore::FindNode(const wxDataViewExtItem& item) const
{
	if (!item.IsOk())
		return m_root;

	return (wxDataViewExtTreeStoreNode*)item.GetID();
}

wxDataViewExtTreeStoreContainerNode* wxDataViewExtTreeStore::FindContainerNode(const wxDataViewExtItem& item) const
{
	if (!item.IsOk())
		return (wxDataViewExtTreeStoreContainerNode*)m_root;

	wxDataViewExtTreeStoreNode* node = (wxDataViewExtTreeStoreNode*)item.GetID();

	if (!node->IsContainer())
		return NULL;

	return (wxDataViewExtTreeStoreContainerNode*)node;
}

//-----------------------------------------------------------------------------
// wxDataViewExtTreeCtrl
//-----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(wxDataViewExtTreeCtrl, wxDataViewExtCtrl);

wxBEGIN_EVENT_TABLE(wxDataViewExtTreeCtrl, wxDataViewExtCtrl)
EVT_DATAVIEW_ITEM_EXPANDED(-1, wxDataViewExtTreeCtrl::OnExpanded)
EVT_DATAVIEW_ITEM_COLLAPSED(-1, wxDataViewExtTreeCtrl::OnCollapsed)
EVT_SIZE(wxDataViewExtTreeCtrl::OnSize)
wxEND_EVENT_TABLE()

bool wxDataViewExtTreeCtrl::Create(wxWindow* parent, wxWindowID id,
	const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator)
{
	if (!wxDataViewExtCtrl::Create(parent, id, pos, size, style, validator))
		return false;

	// create the standard model and a column in the tree
	wxDataViewExtTreeStore* store = new wxDataViewExtTreeStore;
	AssociateModel(store);
	store->DecRef();

	AppendIconTextColumn
	(
		wxString(),                 // no label (header is not shown anyhow)
		0,                          // the only model column
		wxDATAVIEW_CELL_EDITABLE,
		-1,                         // default width
		wxALIGN_NOT,                //  and alignment
		0                           // not resizable
	);

	return true;
}

wxDataViewExtItem wxDataViewExtTreeCtrl::AppendItem(const wxDataViewExtItem& parent,
	const wxString& text, int iconIndex, wxClientData* data)
{
	wxDataViewExtItem res = GetStore()->
		AppendItem(parent, text, GetBitmapBundle(iconIndex), data);

	GetStore()->ItemAdded(parent, res);

	return res;
}

wxDataViewExtItem wxDataViewExtTreeCtrl::PrependItem(const wxDataViewExtItem& parent,
	const wxString& text, int iconIndex, wxClientData* data)
{
	wxDataViewExtItem res = GetStore()->
		PrependItem(parent, text, GetBitmapBundle(iconIndex), data);

	GetStore()->ItemAdded(parent, res);

	return res;
}

wxDataViewExtItem wxDataViewExtTreeCtrl::InsertItem(const wxDataViewExtItem& parent, const wxDataViewExtItem& previous,
	const wxString& text, int iconIndex, wxClientData* data)
{
	wxDataViewExtItem res = GetStore()->
		InsertItem(parent, previous, text, GetBitmapBundle(iconIndex), data);

	GetStore()->ItemAdded(parent, res);

	return res;
}

wxDataViewExtItem wxDataViewExtTreeCtrl::PrependContainer(const wxDataViewExtItem& parent,
	const wxString& text, int iconIndex, int expandedIndex, wxClientData* data)
{
	wxDataViewExtItem res = GetStore()->
		PrependContainer(parent, text,
			GetBitmapBundle(iconIndex), GetBitmapBundle(expandedIndex), data);

	GetStore()->ItemAdded(parent, res);

	return res;
}

wxDataViewExtItem wxDataViewExtTreeCtrl::AppendContainer(const wxDataViewExtItem& parent,
	const wxString& text, int iconIndex, int expandedIndex, wxClientData* data)
{
	wxDataViewExtItem res = GetStore()->
		AppendContainer(parent, text,
			GetBitmapBundle(iconIndex), GetBitmapBundle(expandedIndex), data);

	GetStore()->ItemAdded(parent, res);

	return res;
}

wxDataViewExtItem wxDataViewExtTreeCtrl::InsertContainer(const wxDataViewExtItem& parent, const wxDataViewExtItem& previous,
	const wxString& text, int iconIndex, int expandedIndex, wxClientData* data)
{
	wxDataViewExtItem res = GetStore()->
		InsertContainer(parent, previous, text,
			GetBitmapBundle(iconIndex), GetBitmapBundle(expandedIndex), data);

	GetStore()->ItemAdded(parent, res);

	return res;
}

void wxDataViewExtTreeCtrl::SetItemText(const wxDataViewExtItem& item, const wxString& text)
{
	GetStore()->SetItemText(item, text);

	// notify control
	GetStore()->ValueChanged(item, 0);
}

void wxDataViewExtTreeCtrl::SetItemIcon(const wxDataViewExtItem& item, const wxBitmapBundle& icon)
{
	GetStore()->SetItemIcon(item, icon);

	// notify control
	GetStore()->ValueChanged(item, 0);
}

void wxDataViewExtTreeCtrl::SetItemExpandedIcon(const wxDataViewExtItem& item, const wxBitmapBundle& icon)
{
	GetStore()->SetItemExpandedIcon(item, icon);

	// notify control
	GetStore()->ValueChanged(item, 0);
}

void wxDataViewExtTreeCtrl::DeleteItem(const wxDataViewExtItem& item)
{
	wxDataViewExtItem parent_item = GetStore()->GetParent(item);

	GetStore()->DeleteItem(item);

	// notify control
	GetStore()->ItemDeleted(parent_item, item);
}

void wxDataViewExtTreeCtrl::DeleteChildren(const wxDataViewExtItem& item)
{
	wxDataViewExtTreeStoreContainerNode* node = GetStore()->FindContainerNode(item);
	if (!node) return;

	wxDataViewExtItemArray array;
	wxDataViewExtTreeStoreNodes::iterator iter;
	for (iter = node->GetChildren().begin(); iter != node->GetChildren().end(); ++iter)
	{
		wxDataViewExtTreeStoreNode* child = *iter;
		array.Add(child->GetItem());
	}

	GetStore()->DeleteChildren(item);

	// notify control
	GetStore()->ItemsDeleted(item, array);
}

void  wxDataViewExtTreeCtrl::DeleteAllItems()
{
	GetStore()->DeleteAllItems();

	GetStore()->Cleared();
}

void wxDataViewExtTreeCtrl::OnExpanded(wxDataViewExtEvent& event)
{
	wxDataViewExtTreeStoreContainerNode* container = GetStore()->FindContainerNode(event.GetItem());
	if (!container) return;

	container->SetExpanded(true);

	GetStore()->ItemChanged(event.GetItem());
}

void wxDataViewExtTreeCtrl::OnCollapsed(wxDataViewExtEvent& event)
{
	wxDataViewExtTreeStoreContainerNode* container = GetStore()->FindContainerNode(event.GetItem());
	if (!container) return;

	container->SetExpanded(false);

	GetStore()->ItemChanged(event.GetItem());
}

void wxDataViewExtTreeCtrl::OnSize(wxSizeEvent& event)
{
	// automatically resize our only column to take the entire control width
	if (GetColumnCount())
	{
		wxSize size = GetClientSize();
		GetColumn(0)->SetWidth(size.x);
	}

	event.Skip(true);
}

void wxDataViewExtTreeCtrl::OnImagesChanged()
{
	Refresh();
}

#endif // wxUSE_DATAVIEWCTRL
