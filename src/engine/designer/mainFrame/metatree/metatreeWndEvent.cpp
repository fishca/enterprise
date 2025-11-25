////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : metatree events
////////////////////////////////////////////////////////////////////////////

#include "metaTreeWnd.h"

void CMetadataTree::CMetadataTreeWnd::OnLeftDClick(wxMouseEvent& event)
{
	const wxTreeItemId curItem = HitTest(event.GetPosition());
	if (curItem.IsOk()) {
		SelectItem(curItem);
		m_ownerTree->ActivateItem(curItem);
	}
	//event.Skip();
}

#include "frontend/mainFrame/mainFrame.h"

void CMetadataTree::CMetadataTreeWnd::OnLeftUp(wxMouseEvent& event)
{
	event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnLeftDown(wxMouseEvent& event)
{
	const wxTreeItemId curItem = HitTest(event.GetPosition());
	if (curItem.IsOk() && curItem == GetSelection()) m_ownerTree->SelectItem();
	event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnRightUp(wxMouseEvent& event)
{
	wxTreeItemId curItem = HitTest(event.GetPosition());

	if (curItem.IsOk())
	{
		SelectItem(curItem); SetFocus();
		wxMenu* innerMenu = new wxMenu;
		m_ownerTree->PrepareContextMenu(innerMenu, curItem);

		for (auto def_menu : innerMenu->GetMenuItems())
		{
			if (
				def_menu->GetId() == ID_METATREE_NEW
				|| def_menu->GetId() == ID_METATREE_EDIT
				|| def_menu->GetId() == ID_METATREE_REMOVE
				|| def_menu->GetId() == ID_METATREE_PROPERTY

				|| def_menu->GetId() == ID_METATREE_INSERT
				|| def_menu->GetId() == ID_METATREE_REPLACE
				|| def_menu->GetId() == ID_METATREE_SAVE
				)
			{
				continue;
			}

			GetEventHandler()->Bind(wxEVT_MENU, &CMetadataTree::CMetadataTreeWnd::OnCommandItem, this, def_menu->GetId());
		}

		PopupMenu(innerMenu, event.GetPosition());

		for (auto def_menu : innerMenu->GetMenuItems())
		{
			if (
				def_menu->GetId() == ID_METATREE_NEW
				|| def_menu->GetId() == ID_METATREE_EDIT
				|| def_menu->GetId() == ID_METATREE_REMOVE
				|| def_menu->GetId() == ID_METATREE_PROPERTY

				|| def_menu->GetId() == ID_METATREE_INSERT
				|| def_menu->GetId() == ID_METATREE_REPLACE
				|| def_menu->GetId() == ID_METATREE_SAVE
				)
			{
				continue;
			}

			GetEventHandler()->Unbind(wxEVT_MENU, &CMetadataTree::CMetadataTreeWnd::OnCommandItem, this, def_menu->GetId());
		}

		delete innerMenu;
	}

	//m_ownerTree->SelectItem(); event.Skip();
	event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnRightDown(wxMouseEvent& event)
{
	wxTreeItemId curItem = HitTest(event.GetPosition());

	if (curItem.IsOk())
	{
		SelectItem(curItem); SetFocus();
		wxMenu* innerMenu = new wxMenu;
		m_ownerTree->PrepareContextMenu(innerMenu, curItem);

		for (auto def_menu : innerMenu->GetMenuItems())
		{
			if (
				def_menu->GetId() == ID_METATREE_NEW
				|| def_menu->GetId() == ID_METATREE_EDIT
				|| def_menu->GetId() == ID_METATREE_REMOVE
				|| def_menu->GetId() == ID_METATREE_PROPERTY

				|| def_menu->GetId() == ID_METATREE_INSERT
				|| def_menu->GetId() == ID_METATREE_REPLACE
				|| def_menu->GetId() == ID_METATREE_SAVE
				)
			{
				continue;
			}

			GetEventHandler()->Bind(wxEVT_MENU, &CMetadataTree::CMetadataTreeWnd::OnCommandItem, this, def_menu->GetId());
		}

		PopupMenu(innerMenu, event.GetPosition());

		for (auto def_menu : innerMenu->GetMenuItems())
		{
			if (
				def_menu->GetId() == ID_METATREE_NEW
				|| def_menu->GetId() == ID_METATREE_EDIT
				|| def_menu->GetId() == ID_METATREE_REMOVE
				|| def_menu->GetId() == ID_METATREE_PROPERTY

				|| def_menu->GetId() == ID_METATREE_INSERT
				|| def_menu->GetId() == ID_METATREE_REPLACE
				|| def_menu->GetId() == ID_METATREE_SAVE
				)
			{
				continue;
			}

			GetEventHandler()->Unbind(wxEVT_MENU, &CMetadataTree::CMetadataTreeWnd::OnCommandItem, this, def_menu->GetId());
		}

		delete innerMenu;
	}

	//m_ownerTree->SelectItem(); event.Skip();
	event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnRightDClick(wxMouseEvent& event)
{
	event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnKeyUp(wxKeyEvent& event)
{
	event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnKeyDown(wxKeyEvent& event)
{
	event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnMouseMove(wxMouseEvent& event)
{
	event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnBeginDrag(wxTreeEvent& event) {

	wxTreeItemId curItem = event.GetItem();
	if (!curItem.IsOk())
		return;
	// need to explicitly allow drag
	if (curItem == GetRootItem())
		return;
	IMetaObject* metaObject = m_ownerTree->GetMetaObject(curItem);
	if (metaObject == nullptr)
		return;
	m_draggedItem = curItem;
	event.Allow();
}

void CMetadataTree::CMetadataTreeWnd::OnEndDrag(wxTreeEvent& event) {

	bool copy = ::wxGetKeyState(WXK_CONTROL);
	wxTreeItemId itemSrc = m_draggedItem, itemDst = event.GetItem();
	m_draggedItem = (wxTreeItemId)0l;

	if (!m_ownerTree->IsEditable())
		return;

	// ensure that itemDst is not itemSrc or a child of itemSrc
	IMetaObject* metaSrcObject = m_ownerTree->GetMetaObject(itemSrc);

	if (metaSrcObject != nullptr) {

		const wxTreeItemId& item = m_ownerTree->GetSelectionIdentifier(itemDst);

		if (!item.IsOk())
			return;

		IMetaObject* createdMetaObject = m_ownerTree->NewItem(
			m_ownerTree->GetClassIdentifier(item),
			m_ownerTree->GetMetaIdentifier(item),
			false
		);

		if (createdMetaObject != nullptr) {

			CMemoryWriter dataWritter;
			if (metaSrcObject->CopyObject(dataWritter)) {

				CMemoryReader reader(dataWritter.pointer(), dataWritter.size());
				if (createdMetaObject->PasteObject(reader)) {
					m_ownerTree->FillItem(createdMetaObject, item);
				}
			}
		}
	}

	Update();
	event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnStartSearch(wxCommandEvent& event)
{
	m_ownerTree->Search(event.GetString()); //Fill all data from metaData
	event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnCancelSearch(wxCommandEvent& event)
{
	const wxString& strSearch = event.GetString();
	if (strSearch.IsEmpty())
		m_ownerTree->Search(wxEmptyString); //Fill all data from metaData	
	event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnCreateItem(wxCommandEvent& event)
{
	m_ownerTree->CreateItem(); event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnEditItem(wxCommandEvent& event)
{
	m_ownerTree->EditItem(); event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnRemoveItem(wxCommandEvent& event)
{
	m_ownerTree->RemoveItem(); event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnPropertyItem(wxCommandEvent& event)
{
	m_ownerTree->PropertyItem(); event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnUpItem(wxCommandEvent& event)
{
	m_ownerTree->UpItem();
	event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnDownItem(wxCommandEvent& event)
{
	m_ownerTree->DownItem();
	event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnSortItem(wxCommandEvent& event)
{
	m_ownerTree->SortItem();
	event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnInsertItem(wxCommandEvent& event)
{
	m_ownerTree->InsertItem();
	event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnReplaceItem(wxCommandEvent& event)
{
	m_ownerTree->ReplaceItem();
	event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnSaveItem(wxCommandEvent& event)
{
	m_ownerTree->SaveItem();
	event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnCommandItem(wxCommandEvent& event)
{
	m_ownerTree->CommandItem(event.GetId());
	event.Skip();
}

#include <wx/clipbrd.h>

void CMetadataTree::CMetadataTreeWnd::OnCopyItem(wxCommandEvent& event)
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

void CMetadataTree::CMetadataTreeWnd::OnPasteItem(wxCommandEvent& event)
{
	if (!m_ownerTree->IsEditable())
		return;

	const wxTreeItemId& item = m_ownerTree->GetSelectionIdentifier();
	if (!item.IsOk())
		return;

	if (wxTheClipboard->Open() && wxTheClipboard->IsSupported(oes_clipboard_metadata)) {

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

void CMetadataTree::CMetadataTreeWnd::OnSetFocus(wxFocusEvent& event)
{
	if (event.GetEventType() == wxEVT_SET_FOCUS) {

		const wxTreeItemId& item = GetSelection();

		wxView* view = docManager->GetCurrentView();
		if (m_ownerTree->m_docParent == nullptr && 
			m_metaView != view) {
			if (view != nullptr) view->Activate(false);
			m_metaView->Activate(true);
		}
	}
	else if (event.GetEventType() == wxEVT_KILL_FOCUS) {

		wxWindow* focus_win = event.GetWindow();
		while (focus_win != nullptr && !focus_win->IsKindOf(CLASSINFO(wxAuiMDIChildFrame))) {
			focus_win = focus_win->GetParent();
		}

		if (focus_win != nullptr) {
			
			const CAuiDocChildFrame* focus_child_win = 
				static_cast<CAuiDocChildFrame *>(mainFrame->GetActiveChild());

			wxView* view = focus_child_win ? focus_child_win->GetView() : docManager->GetAnyUsableView();
			if (m_ownerTree->m_docParent == nullptr &&
				m_metaView != view &&
				m_metaView == docManager->GetCurrentView()) {
				m_metaView->Activate(false);
				if (view != nullptr) view->Activate(true);
			}
		}
	}

	event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnSelecting(wxTreeEvent& event)
{
	event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnSelected(wxTreeEvent& event)
{
	m_ownerTree->SelectItem(); event.Skip();
}

void CMetadataTree::CMetadataTreeWnd::OnCollapsing(wxTreeEvent& event)
{
	if (GetRootItem() != event.GetItem()) {
		m_ownerTree->Collapse(); event.Skip();
	}
	else {
		event.Veto();
	}
}

void CMetadataTree::CMetadataTreeWnd::OnExpanding(wxTreeEvent& event)
{
	m_ownerTree->Expand(); event.Skip();
}
