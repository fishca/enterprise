////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : dataprocessor events
////////////////////////////////////////////////////////////////////////////

#include "dataReportWnd.h"

void CDataReportTree::CDataReportTreeWnd::OnLeftDClick(wxMouseEvent &event)
{
	const wxTreeItemId curItem = HitTest(event.GetPosition());
	if (curItem.IsOk()) {
		SelectItem(curItem); m_ownerTree->ActivateItem(curItem);
	}
	//event.Skip();
}

#include "frontend/mainFrame/mainFrame.h"

void CDataReportTree::CDataReportTreeWnd::OnLeftUp(wxMouseEvent &event)
{
	event.Skip();
}

void CDataReportTree::CDataReportTreeWnd::OnLeftDown(wxMouseEvent &event)
{
	const wxTreeItemId curItem = HitTest(event.GetPosition());
	if (curItem.IsOk() && curItem == GetSelection()) m_ownerTree->SelectItem();
	event.Skip();
}

void CDataReportTree::CDataReportTreeWnd::OnRightUp(wxMouseEvent &event)
{
	wxTreeItemId curItem = HitTest(event.GetPosition());

	if (curItem.IsOk())
	{
		SelectItem(curItem); SetFocus();
		wxMenu *innerMenu = new wxMenu;
		m_ownerTree->PrepareContextMenu(innerMenu, curItem);

		for (auto def_menu : innerMenu->GetMenuItems())
		{
			if (def_menu->GetId() == ID_METATREE_NEW
				|| def_menu->GetId() == ID_METATREE_EDIT
				|| def_menu->GetId() == ID_METATREE_REMOVE
				|| def_menu->GetId() == ID_METATREE_PROPERTY)
			{
				continue;
			}

			GetEventHandler()->Bind(wxEVT_MENU, &CDataReportTree::CDataReportTreeWnd::OnCommandItem, this, def_menu->GetId());
		}

		PopupMenu(innerMenu, event.GetPosition());

		for (auto def_menu : innerMenu->GetMenuItems())
		{
			if (def_menu->GetId() == ID_METATREE_NEW
				|| def_menu->GetId() == ID_METATREE_EDIT
				|| def_menu->GetId() == ID_METATREE_REMOVE
				|| def_menu->GetId() == ID_METATREE_PROPERTY)
			{
				continue;
			}

			GetEventHandler()->Unbind(wxEVT_MENU, &CDataReportTree::CDataReportTreeWnd::OnCommandItem, this, def_menu->GetId());
		}

		delete innerMenu;
	}

	m_ownerTree->SelectItem(); event.Skip();
}

void CDataReportTree::CDataReportTreeWnd::OnRightDown(wxMouseEvent &event)
{
	wxTreeItemId curItem = HitTest(event.GetPosition());

	if (curItem.IsOk())
	{
		SelectItem(curItem); SetFocus();
		wxMenu *defaultMenu = new wxMenu;
		m_ownerTree->PrepareContextMenu(defaultMenu, curItem);

		for (auto def_menu : defaultMenu->GetMenuItems())
		{
			if (def_menu->GetId() == ID_METATREE_NEW
				|| def_menu->GetId() == ID_METATREE_EDIT
				|| def_menu->GetId() == ID_METATREE_REMOVE
				|| def_menu->GetId() == ID_METATREE_PROPERTY)
			{
				continue;
			}

			GetEventHandler()->Bind(wxEVT_MENU, &CDataReportTree::CDataReportTreeWnd::OnCommandItem, this, def_menu->GetId());
		}

		PopupMenu(defaultMenu, event.GetPosition());

		for (auto def_menu : defaultMenu->GetMenuItems())
		{
			if (def_menu->GetId() == ID_METATREE_NEW
				|| def_menu->GetId() == ID_METATREE_EDIT
				|| def_menu->GetId() == ID_METATREE_REMOVE
				|| def_menu->GetId() == ID_METATREE_PROPERTY)
			{
				continue;
			}

			GetEventHandler()->Unbind(wxEVT_MENU, &CDataReportTree::CDataReportTreeWnd::OnCommandItem, this, def_menu->GetId());
		}

		delete defaultMenu;
	}

	//m_ownerTree->SelectItem(); event.Skip();
	event.Skip();
}

void CDataReportTree::CDataReportTreeWnd::OnRightDClick(wxMouseEvent &event)
{
	event.Skip();
}

void CDataReportTree::CDataReportTreeWnd::OnKeyUp(wxKeyEvent &event)
{
	event.Skip();
}

void CDataReportTree::CDataReportTreeWnd::OnKeyDown(wxKeyEvent &event)
{
	event.Skip();
}

void CDataReportTree::CDataReportTreeWnd::OnMouseMove(wxMouseEvent &event)
{
	event.Skip();
}

void CDataReportTree::CDataReportTreeWnd::OnCreateItem(wxCommandEvent &event)
{
	m_ownerTree->CreateItem(); event.Skip();
}

void CDataReportTree::CDataReportTreeWnd::OnEditItem(wxCommandEvent &event)
{
	m_ownerTree->EditItem(); event.Skip();
}

void CDataReportTree::CDataReportTreeWnd::OnRemoveItem(wxCommandEvent &event)
{
	m_ownerTree->RemoveItem(); event.Skip();
}

void CDataReportTree::CDataReportTreeWnd::OnPropertyItem(wxCommandEvent &event)
{
	m_ownerTree->PropertyItem(); event.Skip();
}

void CDataReportTree::CDataReportTreeWnd::OnUpItem(wxCommandEvent& event)
{
	m_ownerTree->UpItem(); event.Skip();
}

void CDataReportTree::CDataReportTreeWnd::OnDownItem(wxCommandEvent& event)
{
	m_ownerTree->DownItem(); event.Skip();
}

void CDataReportTree::CDataReportTreeWnd::OnSortItem(wxCommandEvent& event)
{
	m_ownerTree->SortItem(); event.Skip();
}

void CDataReportTree::CDataReportTreeWnd::OnCommandItem(wxCommandEvent &event)
{
	m_ownerTree->CommandItem(event.GetId()); event.Skip();
}

#include <wx/clipbrd.h>

void CDataReportTree::CDataReportTreeWnd::OnCopyItem(wxCommandEvent &event)
{
	const wxTreeItemId& item = GetSelection();
	if (!item.IsOk())
		return;

	// Write some text to the clipboard
	if (wxTheClipboard->Open()) {

		IMetaObject* metaObject = m_ownerTree->GetMetaObject(item);
		if (metaObject != nullptr) {

			CMemoryWriter dataWritter;
			if (metaObject->CopyObject(dataWritter)) {

				wxDataObjectComposite* composite_object = new wxDataObjectComposite;
				wxCustomDataObject* custom_object = new wxCustomDataObject(oes_clipboard_metadata);
				custom_object->SetData(dataWritter.size(), dataWritter.pointer()); // the +1 is used to force copy of the \0 character		

				composite_object->Add(custom_object);
				composite_object->Add(new wxTextDataObject(metaObject->GetName()), true);

				// tell clipboard 
				wxTheClipboard->SetData(composite_object);
			}

			wxTheClipboard->Close();
		}
	}

	event.Skip();
}

void CDataReportTree::CDataReportTreeWnd::OnPasteItem(wxCommandEvent &event)
{
	if (!m_ownerTree->IsEditable())
		return;

	const wxTreeItemId& item = m_ownerTree->GetSelectionIdentifier();
	if (!item.IsOk())
		return;
	if (wxTheClipboard->Open()
		&& wxTheClipboard->IsSupported(oes_clipboard_metadata)) {
		wxCustomDataObject data(oes_clipboard_metadata);
		if (wxTheClipboard->GetData(data)) {

			IMetaObject* metaObject = m_ownerTree->NewItem(
				m_ownerTree->GetClassIdentifier(),
				m_ownerTree->GetMetaIdentifier(),
				false
			);

			if (metaObject != nullptr) {
				CMemoryReader reader(data.GetData(), data.GetDataSize());
				if (metaObject->PasteObject(reader)) 		
					m_ownerTree->FillItem(metaObject, item);
				objectInspector->SelectObject(metaObject);
			}
		}
		wxTheClipboard->Close();
	}

	Update();
	event.Skip();
}

#include "frontend/docView/docManager.h"
#include "frontend/mainFrame/mainFrameChild.h"

void CDataReportTree::CDataReportTreeWnd::OnSetFocus(wxFocusEvent& event)
{
	if (event.GetEventType() == wxEVT_SET_FOCUS) {
		docManager->ActivateView(m_metaView);
	}
	else if (event.GetEventType() == wxEVT_KILL_FOCUS) {
		const CAuiDocChildFrame* child =
			static_cast<CAuiDocChildFrame*>(mainFrame->GetActiveChild());
		wxView* view = child ? child->GetView() : docManager->GetAnyUsableView();
		if (view != nullptr && view != docManager->GetCurrentView())
			view->Activate(true);
		docManager->ActivateView(view);
	}

	event.Skip();
}

void CDataReportTree::CDataReportTreeWnd::OnSelecting(wxTreeEvent &event)
{
	event.Skip();
}

void CDataReportTree::CDataReportTreeWnd::OnSelected(wxTreeEvent &event)
{
	m_ownerTree->SelectItem(); event.Skip();
}

void CDataReportTree::CDataReportTreeWnd::OnCollapsing(wxTreeEvent &event)
{
	if (GetRootItem() != event.GetItem()) {
		m_ownerTree->Collapse(); event.Skip();
	}
	else {
		event.Veto();
	}
}

void CDataReportTree::CDataReportTreeWnd::OnExpanding(wxTreeEvent &event)
{
	m_ownerTree->Expand(); event.Skip();
}
