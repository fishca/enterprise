////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : dataprocessor events
////////////////////////////////////////////////////////////////////////////

#include "dataProcessorWnd.h"

void CDataProcessorTree::CDataProcessorTreeCtrl::OnLeftDClick(wxMouseEvent& event)
{
	const wxTreeItemId curItem = HitTest(event.GetPosition());
	if (curItem.IsOk()) {
		SelectItem(curItem); m_ownerTree->ActivateItem(curItem);
	}
	//event.Skip();
}

#include "frontend/mainFrame/mainFrame.h"

void CDataProcessorTree::CDataProcessorTreeCtrl::OnLeftUp(wxMouseEvent& event)
{
	event.Skip();
}

void CDataProcessorTree::CDataProcessorTreeCtrl::OnLeftDown(wxMouseEvent& event)
{
	const wxTreeItemId curItem = HitTest(event.GetPosition());
	if (curItem.IsOk() && curItem == GetSelection()) m_ownerTree->SelectItem();
	event.Skip();
}

void CDataProcessorTree::CDataProcessorTreeCtrl::OnRightUp(wxMouseEvent& event)
{
	wxTreeItemId curItem = HitTest(event.GetPosition());
	if (curItem.IsOk()) {
		SelectItem(curItem); SetFocus();
		wxMenu* defaultMenu = new wxMenu;
		m_ownerTree->PrepareContextMenu(defaultMenu, curItem);
		for (auto def_menu : defaultMenu->GetMenuItems()) {
			if (def_menu->GetId() == ID_METATREE_NEW
				|| def_menu->GetId() == ID_METATREE_EDIT
				|| def_menu->GetId() == ID_METATREE_DELETE
				|| def_menu->GetId() == ID_METATREE_PROPERTY) {
				continue;
			}
			GetEventHandler()->Bind(wxEVT_MENU, &CDataProcessorTree::CDataProcessorTreeCtrl::OnCommandItem, this, def_menu->GetId());
		}
		PopupMenu(defaultMenu, event.GetPosition());
		for (auto def_menu : defaultMenu->GetMenuItems()) {
			if (def_menu->GetId() == ID_METATREE_NEW
				|| def_menu->GetId() == ID_METATREE_EDIT
				|| def_menu->GetId() == ID_METATREE_DELETE
				|| def_menu->GetId() == ID_METATREE_PROPERTY) {
				continue;
			}
			GetEventHandler()->Unbind(wxEVT_MENU, &CDataProcessorTree::CDataProcessorTreeCtrl::OnCommandItem, this, def_menu->GetId());
		}
		delete defaultMenu;
	}

	//m_ownerTree->SelectItem(); event.Skip();
	event.Skip();
}

void CDataProcessorTree::CDataProcessorTreeCtrl::OnRightDown(wxMouseEvent& event)
{
	wxTreeItemId curItem = HitTest(event.GetPosition());
	if (curItem.IsOk()) {
		SelectItem(curItem); SetFocus();
		wxMenu* defaultMenu = new wxMenu;
		m_ownerTree->PrepareContextMenu(defaultMenu, curItem);
		for (auto def_menu : defaultMenu->GetMenuItems()) {
			if (def_menu->GetId() == ID_METATREE_NEW
				|| def_menu->GetId() == ID_METATREE_EDIT
				|| def_menu->GetId() == ID_METATREE_DELETE
				|| def_menu->GetId() == ID_METATREE_PROPERTY) {
				continue;
			}
			GetEventHandler()->Bind(wxEVT_MENU, &CDataProcessorTree::CDataProcessorTreeCtrl::OnCommandItem, this, def_menu->GetId());
		}
		PopupMenu(defaultMenu, event.GetPosition());
		for (auto def_menu : defaultMenu->GetMenuItems()) {
			if (def_menu->GetId() == ID_METATREE_NEW
				|| def_menu->GetId() == ID_METATREE_EDIT
				|| def_menu->GetId() == ID_METATREE_DELETE
				|| def_menu->GetId() == ID_METATREE_PROPERTY) {
				continue;
			}
			GetEventHandler()->Unbind(wxEVT_MENU, &CDataProcessorTree::CDataProcessorTreeCtrl::OnCommandItem, this, def_menu->GetId());
		}
		delete defaultMenu;
	}

	//m_ownerTree->SelectItem(); event.Skip();
	event.Skip();
}

void CDataProcessorTree::CDataProcessorTreeCtrl::OnRightDClick(wxMouseEvent& event)
{
	event.Skip();
}

void CDataProcessorTree::CDataProcessorTreeCtrl::OnKeyUp(wxKeyEvent& event)
{
	event.Skip();
}

void CDataProcessorTree::CDataProcessorTreeCtrl::OnKeyDown(wxKeyEvent& event)
{
	event.Skip();
}

void CDataProcessorTree::CDataProcessorTreeCtrl::OnMouseMove(wxMouseEvent& event)
{
	event.Skip();
}

void CDataProcessorTree::CDataProcessorTreeCtrl::OnCreateItem(wxCommandEvent& event)
{
	m_ownerTree->CreateItem(); event.Skip();
}

void CDataProcessorTree::CDataProcessorTreeCtrl::OnEditItem(wxCommandEvent& event)
{
	m_ownerTree->EditItem(); event.Skip();
}

void CDataProcessorTree::CDataProcessorTreeCtrl::OnRemoveItem(wxCommandEvent& event)
{
	m_ownerTree->RemoveItem(); event.Skip();
}

void CDataProcessorTree::CDataProcessorTreeCtrl::OnPropertyItem(wxCommandEvent& event)
{
	m_ownerTree->PropertyItem(); event.Skip();
}

void CDataProcessorTree::CDataProcessorTreeCtrl::OnUpItem(wxCommandEvent& event)
{
	m_ownerTree->UpItem(); event.Skip();
}

void CDataProcessorTree::CDataProcessorTreeCtrl::OnDownItem(wxCommandEvent& event)
{
	m_ownerTree->DownItem(); event.Skip();
}

void CDataProcessorTree::CDataProcessorTreeCtrl::OnSortItem(wxCommandEvent& event)
{
	m_ownerTree->SortItem(); event.Skip();
}

void CDataProcessorTree::CDataProcessorTreeCtrl::OnCommandItem(wxCommandEvent& event)
{
	m_ownerTree->CommandItem(event.GetId()); event.Skip();
}

#include <wx/clipbrd.h>

void CDataProcessorTree::CDataProcessorTreeCtrl::OnCopyItem(wxCommandEvent& event)
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

void CDataProcessorTree::CDataProcessorTreeCtrl::OnPasteItem(wxCommandEvent& event)
{
	if (!m_ownerTree->IsEditable())
		return;

	const wxTreeItemId& item = m_ownerTree->GetSelectionIdentifier();
	if (!item.IsOk())
		return;

	m_ownerTree->Freeze();

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
				if (metaObject->PasteObject(reader)) {
					objectInspector->SelectObject(metaObject);
				}
				m_ownerTree->FillItem(metaObject, item);
			}
		}
		wxTheClipboard->Close();
	}

	m_ownerTree->Thaw();
	RefreshSelectedItem();

	event.Skip();
}

#include "frontend/docView/docManager.h"
#include "frontend/mainFrame/mainFrameChild.h"

void CDataProcessorTree::CDataProcessorTreeCtrl::OnSetFocus(wxFocusEvent& event)
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

void CDataProcessorTree::CDataProcessorTreeCtrl::OnSelecting(wxTreeEvent& event)
{
	event.Skip();
}

void CDataProcessorTree::CDataProcessorTreeCtrl::OnSelected(wxTreeEvent& event)
{
	m_ownerTree->SelectItem(); event.Skip();
}

void CDataProcessorTree::CDataProcessorTreeCtrl::OnCollapsing(wxTreeEvent& event)
{
	if (GetRootItem() != event.GetItem()) {
		m_ownerTree->Collapse(); event.Skip();
	}
	else {
		event.Veto();
	}
}

void CDataProcessorTree::CDataProcessorTreeCtrl::OnExpanding(wxTreeEvent& event)
{
	m_ownerTree->Expand(); event.Skip();
}
