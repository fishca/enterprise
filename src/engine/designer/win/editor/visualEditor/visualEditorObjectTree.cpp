////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxFormBuilder
//	Description : visual editor 
////////////////////////////////////////////////////////////////////////////

#include "visualEditor.h"
#include "backend/propertyManager/propertyManager.h"

#include <wx/imaglist.h>

void CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::RebuildTree()
{
	m_tcObjects->Freeze();

	Disconnect(wxID_ANY, wxEVT_COMMAND_TREE_ITEM_EXPANDED, wxTreeEventHandler(CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnExpansionChange));
	Disconnect(wxID_ANY, wxEVT_COMMAND_TREE_ITEM_COLLAPSED, wxTreeEventHandler(CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnExpansionChange));

	IValueFrame* valueForm =
		m_formHandler->GetValueForm();

	// Clear the old tree and map
	m_tcObjects->DeleteAllItems();
	m_listItem.clear();

	if (valueForm != nullptr) {
		wxTreeItemId dummy;
		AddChildren(valueForm, dummy, true);
		// Expand items that were previously expanded
		RestoreItemStatus(valueForm);
	}

	Connect(wxID_ANY, wxEVT_COMMAND_TREE_ITEM_COLLAPSED, wxTreeEventHandler(CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnExpansionChange));
	Connect(wxID_ANY, wxEVT_COMMAND_TREE_ITEM_EXPANDED, wxTreeEventHandler(CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnExpansionChange));

	m_tcObjects->Thaw();
}

void CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::AddChildren(IValueFrame* obj, const wxTreeItemId& parent, bool is_root)
{
	if (obj->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
		if (obj->GetChildCount() > 0) {
			AddChildren(obj->GetChild(0), parent);
		}
		else {
			// Si hemos llegado aquí ha sido porque el arbol no está bien formado
			// y habrá que revisar cómo se ha creado.
			wxString msg;
			IValueFrame* itemParent = obj->GetParent();
			assert(parent);

			msg = wxString::Format(wxT("Item without object as child of \'%s:%s\'"),
				itemParent->GetControlName().c_str(),
				itemParent->GetClassName().c_str());

			wxLogError(msg);
		}
	}
	else {
		wxTreeItemId new_parent;
		CVisualEditorObjectTreeItemData* item_data = new CVisualEditorObjectTreeItemData(obj);
		if (is_root) {
			new_parent = m_tcObjects->AddRoot(wxT(""), wxNOT_FOUND, wxNOT_FOUND, item_data);
		}
		else {
			unsigned int pos = 0;

			IValueFrame* parent_obj = obj->GetParent();

			// find a proper position where the added object should be displayed at
			if (parent_obj && parent_obj->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
				parent_obj = parent_obj->GetParent();
				if (parent_obj) {
					pos = parent_obj->GetChildPosition(obj->GetParent());
				}
			}
			else if (parent_obj) {
				pos = parent_obj->GetChildPosition(obj);
			}

			// insert tree item to proper position
			if (pos > 0) {
				new_parent = m_tcObjects->InsertItem(parent, pos, wxT(""), wxNOT_FOUND, wxNOT_FOUND, item_data);
			}
			else {
				new_parent = m_tcObjects->AppendItem(parent, wxT(""), wxNOT_FOUND, wxNOT_FOUND, item_data);
			}
		}

		// Add the item to the map
		m_listItem.insert(
			std::map< IValueFrame*, wxTreeItemId>::value_type(obj, new_parent)
		);

		// Set the image
		int image_idx = GetImageIndex(obj->GetClassName());

		if (image_idx != wxNOT_FOUND) {
			m_tcObjects->SetItemImage(new_parent, image_idx);
		}

		// Set the name
		UpdateItem(new_parent, obj);

		// Add the rest of the children
		unsigned int count = obj->GetChildCount();

		for (unsigned int i = 0; i < count; i++) {
			IValueFrame* child = obj->GetChild(i);
			AddChildren(child, new_parent);
		}
	}
}

#define ICON_SIZE 16

void CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::CreateTree()
{
	if (m_iconList != nullptr)
		delete m_iconList;

	m_iconList = new wxImageList(ICON_SIZE, ICON_SIZE);
	
	for (auto objClass : CValue::GetListCtorsByType(eCtorObjectType::eCtorObjectType_object_control)) {
		const wxIcon& controlIcon = objClass->GetClassIcon();
		if (controlIcon.IsOk()) {
			const int retIndex = m_iconList->Add(controlIcon);
			if (retIndex != wxNOT_FOUND) {
				m_iconIdx.insert(
					std::map<wxString, int>::value_type(objClass->GetClassName(), retIndex)
				);
			}
		}
	}

	m_tcObjects->AssignImageList(m_iconList);
}

void CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::RestoreItemStatus(IValueFrame* obj)
{
	std::map< IValueFrame*, wxTreeItemId>::iterator item_it = m_listItem.find(obj);
	if (item_it != m_listItem.end()) {
		wxTreeItemId id = item_it->second;

		if (obj->GetExpanded()) {
			m_tcObjects->Expand(id);
		}
	}

	unsigned int i, count = obj->GetChildCount();

	for (i = 0; i < count; i++) {
		RestoreItemStatus(obj->GetChild(i));
	}
}

void CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::AddItem(IValueFrame* item, IValueFrame* parent)
{
	if (item && parent) {
		// find parent item displayed in the object tree
		while (parent && parent->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
			parent = parent->GetParent();
		}

		// add new item to the object tree
		std::map< IValueFrame*, wxTreeItemId>::iterator it = m_listItem.find(parent);
		if ((it != m_listItem.end()) && it->second.IsOk()) {
			AddChildren(item, it->second, false);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Enterprise IEvent Handlers
/////////////////////////////////////////////////////////////////////////////

void CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnEditorLoaded()
{
	RebuildTree();
}

void CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnEditorRefresh()
{
	RebuildTree();
}

void CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnObjectCreated(IValueFrame* obj)
{
	RebuildTree();
}

void CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnObjectSelected(IValueFrame* obj)
{
	// Find the tree item associated with the object and select it
	std::map< IValueFrame*, wxTreeItemId>::iterator it = m_listItem.find(obj);
	if (it != m_listItem.end()) {

		m_notifySelecting = true;

		// Ignore expand/collapse events	
		Disconnect(wxID_ANY, wxEVT_COMMAND_TREE_ITEM_EXPANDED, wxTreeEventHandler(CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnExpansionChange));
		Disconnect(wxID_ANY, wxEVT_COMMAND_TREE_ITEM_COLLAPSED, wxTreeEventHandler(CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnExpansionChange));

		m_tcObjects->EnsureVisible(it->second);
		m_tcObjects->SelectItem(it->second);

		// Restore event handling
		Connect(wxID_ANY, wxEVT_COMMAND_TREE_ITEM_EXPANDED, wxTreeEventHandler(CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnExpansionChange));
		Connect(wxID_ANY, wxEVT_COMMAND_TREE_ITEM_COLLAPSED, wxTreeEventHandler(CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnExpansionChange));

		m_notifySelecting = false;
	}
	else {
		wxLogError(wxT("There is no tree item associated with this object.\n\tClass: %s\n\tName: %s"), obj->GetClassName().c_str(), obj->GetControlName().c_str());
	}
}

void CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnObjectExpanded(IValueFrame* obj)
{
	std::map< IValueFrame*, wxTreeItemId>::iterator it = m_listItem.find(obj);
	if (it != m_listItem.end())
	{
		if (m_tcObjects->IsExpanded(it->second) != obj->GetExpanded())
		{
			if (obj->GetExpanded()) {
				m_tcObjects->Expand(it->second);
			}
			else {
				m_tcObjects->Collapse(it->second);
			}
		}
	}
}

void CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnObjectRemoved(IValueFrame* obj)
{
	RemoveItem(obj);
}

void CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnPropertyModified(IProperty* prop)
{
	std::map< IValueFrame*, wxTreeItemId>::iterator it =
		m_listItem.find((IValueFrame*)prop->GetPropertyObject());
	if (it != m_listItem.end()) {
		UpdateItem(it->second, it->first);
	}
}

wxBEGIN_EVENT_TABLE(CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree, wxPanel)
EVT_TREE_SEL_CHANGED(wxID_ANY, CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnSelChanged)
EVT_TREE_ITEM_RIGHT_CLICK(wxID_ANY, CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnRightClick)
EVT_TREE_BEGIN_DRAG(wxID_ANY, CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnBeginDrag)
EVT_TREE_END_DRAG(wxID_ANY, CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnEndDrag)
EVT_TREE_KEY_DOWN(wxID_ANY, CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnKeyDown)
wxEND_EVENT_TABLE()

CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::CVisualEditorObjectTree(CVisualEditor* handler, wxWindow* parent, int id) :
	wxPanel(parent, id),
	m_formHandler(handler)
{
	m_tcObjects = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS | wxSIMPLE_BORDER | wxTR_TWIST_BUTTONS);

	wxBoxSizer* sizerMain = new wxBoxSizer(wxVERTICAL);
	sizerMain->Add(m_tcObjects, 1, wxEXPAND, 0);
	wxPanel::SetAutoLayout(true);
	wxPanel::SetSizer(sizerMain);
	sizerMain->Fit(this);
	sizerMain->SetSizeHints(this);

	m_tcObjects->SetDoubleBuffered(true);

	Connect(wxID_ANY, wxEVT_COMMAND_TREE_ITEM_EXPANDED, wxTreeEventHandler(CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnExpansionChange));
	Connect(wxID_ANY, wxEVT_COMMAND_TREE_ITEM_COLLAPSED, wxTreeEventHandler(CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnExpansionChange));

	m_tcObjects->Bind(wxEVT_LEFT_DOWN,
		[&](wxMouseEvent& event) {
			const wxTreeItemId& id = m_tcObjects->HitTest(event.GetPosition());
			if (id.IsOk() && id == m_tcObjects->GetSelection()) {
				wxTreeItemData* item_data = m_tcObjects->GetItemData(id);
				if (item_data != nullptr) {
					IValueFrame* obj(((CVisualEditorObjectTreeItemData*)item_data)->GetObject());
					assert(obj);
					m_formHandler->SelectObject(obj);
					m_formHandler->ScrollToObject(obj);
				}
			}
			event.Skip();
		}
	);

	m_tcObjects->Bind(wxEVT_RIGHT_DOWN,
		[&](wxMouseEvent& event) {
			const wxTreeItemId& id = m_tcObjects->HitTest(event.GetPosition());
			if (id.IsOk() && id == m_tcObjects->GetSelection()) {
				wxTreeItemData* item_data = m_tcObjects->GetItemData(id);
				if (item_data != nullptr) {
					IValueFrame* obj(((CVisualEditorObjectTreeItemData*)item_data)->GetObject());
					assert(obj);
					m_formHandler->SelectObject(obj);
					m_formHandler->ScrollToObject(obj);
				}
			}
			event.Skip();
		}
	);

	m_altKeyIsDown = false;
	m_notifySelecting = false;

	CreateTree();
}

void CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnKeyDown(wxTreeEvent& event)
{
	if (event.GetKeyEvent().AltDown() && event.GetKeyCode() != WXK_ALT)
	{
#ifdef __WXGTK__
		switch (event.GetKeyCode())
		case WXK_UP:
		{
			m_formHandler->MovePosition(GetObjectFromTreeItem(m_tcObjects->GetSelection()), false);
			return;
		case WXK_DOWN:
			m_formHandler->MovePosition(GetObjectFromTreeItem(m_tcObjects->GetSelection()), true);
			return;
		case WXK_RIGHT:
			m_formHandler->MoveHierarchy(GetObjectFromTreeItem(m_tcObjects->GetSelection()), false);
			return;
		case WXK_LEFT:
			m_formHandler->MoveHierarchy(GetObjectFromTreeItem(m_tcObjects->GetSelection()), true);
			return;
		}
#endif
		event.Skip();
	}
	else
	{
		event.Skip();
	}
}

void CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnSelChanged(wxTreeEvent& event)
{
	// Make selected items bold
	wxTreeItemId oldId = event.GetOldItem();
	if (oldId.IsOk()) m_tcObjects->SetItemBold(oldId, false);

	const wxTreeItemId& id = event.GetItem();
	if (!id.IsOk())
		return;
	m_tcObjects->SetItemBold(id);

	wxTreeItemData* item_data = m_tcObjects->GetItemData(id);

	if (m_notifySelecting)
		return;

	if (item_data != nullptr) {
		IValueFrame* obj(((CVisualEditorObjectTreeItemData*)item_data)->GetObject());
		assert(obj);
		m_formHandler->SelectObject(obj);
		m_formHandler->ScrollToObject(obj);
	}
}

#include "frontend/mainFrame/objinspect/objinspect.h"

void CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnRightClick(wxTreeEvent& event)
{
	wxTreeItemId id = event.GetItem();
	wxTreeItemData* item_data = m_tcObjects->GetItemData(id);
	if (item_data != nullptr) {
		IValueFrame* obj(((CVisualEditorObjectTreeItemData*)item_data)->GetObject());
		assert(obj);
		m_formHandler->SelectObject(obj);
		wxMenu* menu = new CVisualEditorItemPopupMenu(m_formHandler, this, obj);
		wxPoint pos = event.GetPoint();
		menu->UpdateUI(menu); PopupMenu(menu, pos.x, pos.y);
	}
}

void CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnBeginDrag(wxTreeEvent& event)
{
	// need to explicitly allow drag
	if (event.GetItem() == m_tcObjects->GetRootItem())
		return;

	m_draggedItem = event.GetItem();
	event.Allow();
}

void CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnEndDrag(wxTreeEvent& event)
{
	bool copy = ::wxGetKeyState(WXK_CONTROL);
	wxTreeItemId itemSrc = m_draggedItem, itemDst = event.GetItem();
	m_draggedItem = (wxTreeItemId)0l;

	// ensure that itemDst is not itemSrc or a child of itemSrc
	wxTreeItemId item = itemDst;
	while (item.IsOk()) {
		if (item == itemSrc)
			return;
		item = m_tcObjects->GetItemParent(item);
	}

	IValueFrame* objSrc =
		GetObjectFromTreeItem(itemSrc);
	if (!objSrc)
		return;

	IValueFrame* objDst =
		GetObjectFromTreeItem(itemDst);

	if (!objDst)
		return;

	// set object to clipboard
	if (copy) {
		m_formHandler->CopyObject(objSrc);
	}
	else {
		m_formHandler->CutObject(objSrc, true);
	}

	if (!copy && !m_formHandler->PasteObject(objDst)) {
		m_formHandler->Undo();
	}
}

void CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnExpansionChange(wxTreeEvent& event)
{
	wxTreeItemId id = event.GetItem();
	wxTreeItemData* item_data = m_tcObjects->GetItemData(id);

	if (item_data != nullptr) {
		IValueFrame* obj(((CVisualEditorObjectTreeItemData*)item_data)->GetObject());
		assert(obj);
		Disconnect(wxID_ANY, wxEVT_COMMAND_TREE_ITEM_EXPANDED, wxTreeEventHandler(CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnExpansionChange));
		m_formHandler->ExpandObject(obj, m_tcObjects->IsExpanded(id));
		Connect(wxID_ANY, wxEVT_COMMAND_TREE_ITEM_EXPANDED, wxTreeEventHandler(CVisualEditorNotebook::CVisualEditor::CVisualEditorObjectTree::OnExpansionChange));
	}
}

///////////////////////////////////////////////////////////////////////////////

enum {
	MENU_MOVE_UP = wxID_HIGHEST + 2000,
	MENU_MOVE_DOWN,
	MENU_CUT,
	MENU_PASTE,
	MENU_EDIT_MENUS,
	MENU_COPY,
	MENU_MOVE_NEW_BOXSIZER,
	MENU_DELETE,
	MENU_PROPERTIES,
};

wxBEGIN_EVENT_TABLE(CVisualEditorNotebook::CVisualEditor::CVisualEditorItemPopupMenu, wxMenu)
EVT_MENU(wxID_ANY, CVisualEditorNotebook::CVisualEditor::CVisualEditorItemPopupMenu::OnMenuEvent)
EVT_UPDATE_UI(wxID_ANY, CVisualEditorNotebook::CVisualEditor::CVisualEditorItemPopupMenu::OnUpdateEvent)
wxEND_EVENT_TABLE()

#include "frontend/artProvider/artProvider.h"

bool CVisualEditorNotebook::CVisualEditor::CVisualEditorItemPopupMenu::HasDeleteObject() {
	return m_selID == MENU_DELETE;
}

CVisualEditorNotebook::CVisualEditor::CVisualEditorItemPopupMenu::CVisualEditorItemPopupMenu(CVisualEditor* handler, wxWindow* parent, IValueFrame* obj)
	: wxMenu(), m_object(obj), m_formHandler(handler)
{
	obj->PrepareDefaultMenu(this);

	wxMenuItem* item = nullptr;
	
	item = Append(MENU_CUT, wxT("Cut\tCtrl+X"));
	item->SetBitmap(wxArtProvider::GetBitmap(wxART_CUT, wxART_MENU));
	item = Append(MENU_COPY, wxT("Copy\tCtrl+C"));
	item->SetBitmap(wxArtProvider::GetBitmap(wxART_COPY, wxART_MENU));
	item = Append(MENU_PASTE, wxT("Paste\tCtrl+V"));
	item->SetBitmap(wxArtProvider::GetBitmap(wxART_PASTE, wxART_MENU));
	AppendSeparator();
	item = Append(MENU_DELETE, wxT("Delete\tCtrl+D"));
	item->SetBitmap(wxArtProvider::GetBitmap(wxART_DELETE, wxART_MENU));
	AppendSeparator();
	item = Append(MENU_MOVE_UP, wxT("Move Up\tAlt+Up"));
	item->SetBitmap(wxArtProvider::GetBitmap(wxART_GO_UP, wxART_MENU));
	item = Append(MENU_MOVE_DOWN, wxT("Move Down\tAlt+Down"));
	item->SetBitmap(wxArtProvider::GetBitmap(wxART_GO_DOWN, wxART_MENU));
	AppendSeparator();
	item = Append(MENU_PROPERTIES, wxT("Properties"));
	item->SetBitmap(wxArtProvider::GetBitmap(wxART_PROPERTY, wxART_SERVICE));
}

void CVisualEditorNotebook::CVisualEditor::CVisualEditorItemPopupMenu::OnMenuEvent(wxCommandEvent& event)
{
	m_selID = event.GetId();

	switch (m_selID)
	{
	case MENU_CUT: m_formHandler->CutObject(m_formHandler->GetSelectedObject()); break;
	case MENU_COPY: m_formHandler->CopyObject(m_formHandler->GetSelectedObject()); break;
	case MENU_PASTE: m_formHandler->PasteObject(m_formHandler->GetSelectedObject()); break;
	case MENU_DELETE: m_formHandler->RemoveObject(m_formHandler->GetSelectedObject()); break;
	case MENU_MOVE_UP: m_formHandler->MovePosition(m_object, false); break;
	case MENU_MOVE_DOWN: m_formHandler->MovePosition(m_object, true); break;

	case MENU_PROPERTIES:
		if (!objectInspector->IsShownProperty()) objectInspector->ShowProperty();
		m_formHandler->SelectObject(m_object, true);
		break;

	default: { m_object->ExecuteMenu(m_formHandler->GetVisualEditor(), m_selID); }
	}
}

void CVisualEditorNotebook::CVisualEditor::CVisualEditorItemPopupMenu::OnUpdateEvent(wxUpdateUIEvent& e)
{
	IValueFrame* currentControl = m_formHandler->GetSelectedObject();

	switch (e.GetId())
	{
	case MENU_CUT:
	case MENU_COPY:
	case MENU_MOVE_UP:
	case MENU_MOVE_DOWN:
	case MENU_MOVE_NEW_BOXSIZER: e.Enable(m_formHandler->CanCopyObject() && m_formHandler->IsEditable()); break;
	case MENU_DELETE: e.Enable(m_formHandler->CanCopyObject() && m_formHandler->IsEditable() && currentControl->CanDeleteControl()); break;
	case MENU_PASTE: e.Enable(m_formHandler->CanPasteObject() && m_formHandler->IsEditable()); break;
	}
}