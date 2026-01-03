////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : metatree window
////////////////////////////////////////////////////////////////////////////

#include "metatreeWnd.h"
#include "backend/debugger/debugClient.h"
#include "frontend/win/theme/luna_toolbarart.h"

wxIMPLEMENT_ABSTRACT_CLASS(IMetaDataTree, wxPanel);
wxIMPLEMENT_DYNAMIC_CLASS(CMetadataTree, IMetaDataTree);

//**********************************************************************************
//*                                  metatree									   *
//**********************************************************************************

#define ICON_SIZE 16

#include <wx/artprov.h>

void IMetaDataTree::CreateToolBar(wxWindow* parent)
{
	wxASSERT(m_metaTreeToolbar);

	m_metaTreeToolbar = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_HORZ_LAYOUT);
	m_metaTreeToolbar->AddTool(ID_METATREE_NEW, _("New"), wxArtProvider::GetBitmap(wxART_PLUS, wxART_MENU), _("New item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_EDIT, _("Edit"), wxArtProvider::GetBitmap(wxART_EDIT, wxART_MENU), _("Edit item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_REMOVE, _("Remove"), wxArtProvider::GetBitmap(wxART_DELETE, wxART_MENU), _("Remove item"));
	m_metaTreeToolbar->AddSeparator();
	m_metaTreeToolbar->AddTool(ID_METATREE_UP, _("Up"), wxArtProvider::GetBitmap(wxART_GO_UP, wxART_MENU), _("Up item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_DOWM, _("Down"), wxArtProvider::GetBitmap(wxART_GO_DOWN, wxART_MENU), _("Down item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_SORT, _("Sort"), wxArtProvider::GetBitmap(wxART_HELP_SETTINGS, wxART_MENU), _("Sort item"));
	m_metaTreeToolbar->Realize();

	m_metaTreeToolbar->SetArtProvider(new wxAuiLunaToolBarArt());
}

CMetadataTree::CMetadataTree()
	: IMetaDataTree(), m_metaData(nullptr)
{
	m_metaTreeToolbar = nullptr;
	m_metaTreeCtrl = nullptr;
}

CMetadataTree::CMetadataTree(wxWindow* parent, int id)
	: IMetaDataTree(parent, id), m_metaData(nullptr)
{
	m_metaTreeToolbar = new wxAuiToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_HORZ_LAYOUT);
	m_metaTreeToolbar->AddTool(ID_METATREE_NEW, _("New"), wxArtProvider::GetBitmap(wxART_PLUS, wxART_MENU), _("New item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_EDIT, _("Edit"), wxArtProvider::GetBitmap(wxART_EDIT, wxART_MENU), _("Edit item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_REMOVE, _("Remove"), wxArtProvider::GetBitmap(wxART_DELETE, wxART_MENU), _("Remove item"));
	m_metaTreeToolbar->AddSeparator();
	m_metaTreeToolbar->AddTool(ID_METATREE_UP, _("Up"), wxArtProvider::GetBitmap(wxART_GO_UP, wxART_MENU), _("Up item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_DOWM, _("Down"), wxArtProvider::GetBitmap(wxART_GO_DOWN, wxART_MENU), _("Down item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_SORT, _("Sort"), wxArtProvider::GetBitmap(wxART_HELP_SETTINGS, wxART_MENU), _("Sort item"));
	m_metaTreeToolbar->Realize();

	m_metaTreeToolbar->SetArtProvider(new wxAuiLunaToolBarArt());

	//Create main tree
	m_metaTreeCtrl = new CMetaTreeCtrl(this);
	m_metaTreeCtrl->SetBackgroundColour(RGB(250, 250, 250));

	//set image list
	m_metaTreeCtrl->SetImageList(
		new wxImageList(ICON_SIZE, ICON_SIZE)
	);

	m_searchTree = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	m_searchTree->Bind(wxEVT_SEARCH, &CMetadataTree::CMetaTreeCtrl::OnStartSearch, m_metaTreeCtrl);
	m_searchTree->Bind(wxEVT_COMMAND_TEXT_UPDATED, &CMetadataTree::CMetaTreeCtrl::OnCancelSearch, m_metaTreeCtrl);
	m_searchTree->ShowCancelButton(true);

	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnCreateItem, m_metaTreeCtrl, ID_METATREE_NEW);
	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnEditItem, m_metaTreeCtrl, ID_METATREE_EDIT);
	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnRemoveItem, m_metaTreeCtrl, ID_METATREE_REMOVE);

	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnUpItem, m_metaTreeCtrl, ID_METATREE_UP);
	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnDownItem, m_metaTreeCtrl, ID_METATREE_DOWM);
	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnSortItem, m_metaTreeCtrl, ID_METATREE_SORT);

	// Set up the sizer for the panel
	wxBoxSizer* panelSizer = new wxBoxSizer(wxVERTICAL);
	panelSizer->Add(m_searchTree, 0, wxEXPAND, 1);
	panelSizer->Add(m_metaTreeToolbar, 0, wxEXPAND, 1);
	panelSizer->Add(m_metaTreeCtrl, 1, wxEXPAND, 1);
	SetSizer(panelSizer);

	//Init tree
	InitTree();
}

CMetadataTree::CMetadataTree(CMetaDocument* docParent, wxWindow* parent, int id)
	: IMetaDataTree(docParent, parent, id), m_metaData(nullptr)
{
	wxASSERT(m_metaTreeToolbar);

	m_metaTreeToolbar = new wxAuiToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_HORZ_LAYOUT);
	m_metaTreeToolbar->AddTool(ID_METATREE_NEW, _("New"), wxArtProvider::GetBitmap(wxART_PLUS, wxART_MENU), _("New item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_EDIT, _("Edit"), wxArtProvider::GetBitmap(wxART_EDIT, wxART_MENU), _("Edit item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_REMOVE, _("Remove"), wxArtProvider::GetBitmap(wxART_DELETE, wxART_MENU), _("Remove item"));
	m_metaTreeToolbar->AddSeparator();
	m_metaTreeToolbar->AddTool(ID_METATREE_UP, _("Up"), wxArtProvider::GetBitmap(wxART_GO_UP, wxART_MENU), _("Up item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_DOWM, _("Down"), wxArtProvider::GetBitmap(wxART_GO_DOWN, wxART_MENU), _("Down item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_SORT, _("Sort"), wxArtProvider::GetBitmap(wxART_HELP_SETTINGS, wxART_MENU), _("Sort item"));
	m_metaTreeToolbar->Realize();

	m_metaTreeToolbar->SetArtProvider(new wxAuiLunaToolBarArt());

	//Create main tree
	m_metaTreeCtrl = new CMetaTreeCtrl(this);
	m_metaTreeCtrl->SetBackgroundColour(RGB(250, 250, 250));

	//set image list
	m_metaTreeCtrl->SetImageList(
		new wxImageList(ICON_SIZE, ICON_SIZE)
	);

	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnCreateItem, m_metaTreeCtrl, ID_METATREE_NEW);
	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnEditItem, m_metaTreeCtrl, ID_METATREE_EDIT);
	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnRemoveItem, m_metaTreeCtrl, ID_METATREE_REMOVE);

	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnUpItem, m_metaTreeCtrl, ID_METATREE_UP);
	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnDownItem, m_metaTreeCtrl, ID_METATREE_DOWM);
	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnSortItem, m_metaTreeCtrl, ID_METATREE_SORT);

	// Set up the sizer for the panel
	wxBoxSizer* panelSizer = new wxBoxSizer(wxVERTICAL);
	panelSizer->Add(m_metaTreeToolbar, 0, wxEXPAND, 1);
	panelSizer->Add(m_metaTreeCtrl, 1, wxEXPAND, 1);
	SetSizer(panelSizer);

	//Init tree
	InitTree();
}

CMetadataTree::~CMetadataTree()
{
	if (m_metaData != nullptr) m_metaData->SetMetaTree(nullptr);

	m_metaTreeToolbar->Unbind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnCreateItem, m_metaTreeCtrl, ID_METATREE_NEW);
	m_metaTreeToolbar->Unbind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnEditItem, m_metaTreeCtrl, ID_METATREE_EDIT);
	m_metaTreeToolbar->Unbind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnRemoveItem, m_metaTreeCtrl, ID_METATREE_REMOVE);

	m_metaTreeToolbar->Unbind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnUpItem, m_metaTreeCtrl, ID_METATREE_UP);
	m_metaTreeToolbar->Unbind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnDownItem, m_metaTreeCtrl, ID_METATREE_DOWM);
	m_metaTreeToolbar->Unbind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnSortItem, m_metaTreeCtrl, ID_METATREE_SORT);
}

//**********************************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CMetadataTree::CMetaTreeCtrl, wxTreeCtrl);

//**********************************************************************************
//*                                  metatree window						       *
//**********************************************************************************

wxBEGIN_EVENT_TABLE(CMetadataTree::CMetaTreeCtrl, wxTreeCtrl)

EVT_LEFT_UP(CMetadataTree::CMetaTreeCtrl::OnLeftUp)
EVT_LEFT_DOWN(CMetadataTree::CMetaTreeCtrl::OnLeftDown)
EVT_LEFT_DCLICK(CMetadataTree::CMetaTreeCtrl::OnLeftDClick)
EVT_RIGHT_UP(CMetadataTree::CMetaTreeCtrl::OnRightUp)
EVT_RIGHT_DOWN(CMetadataTree::CMetaTreeCtrl::OnRightDown)
EVT_RIGHT_DCLICK(CMetadataTree::CMetaTreeCtrl::OnRightDClick)
EVT_MOTION(CMetadataTree::CMetaTreeCtrl::OnMouseMove)
EVT_KEY_UP(CMetadataTree::CMetaTreeCtrl::OnKeyUp)
EVT_KEY_DOWN(CMetadataTree::CMetaTreeCtrl::OnKeyDown)

EVT_TREE_SEL_CHANGING(wxID_ANY, CMetadataTree::CMetaTreeCtrl::OnSelecting)
EVT_TREE_SEL_CHANGED(wxID_ANY, CMetadataTree::CMetaTreeCtrl::OnSelected)

EVT_TREE_ITEM_COLLAPSING(wxID_ANY, CMetadataTree::CMetaTreeCtrl::OnCollapsing)
EVT_TREE_ITEM_EXPANDING(wxID_ANY, CMetadataTree::CMetaTreeCtrl::OnExpanding)

EVT_TREE_BEGIN_DRAG(wxID_ANY, CMetadataTree::CMetaTreeCtrl::OnBeginDrag)
EVT_TREE_END_DRAG(wxID_ANY, CMetadataTree::CMetaTreeCtrl::OnEndDrag)

EVT_MENU(ID_METATREE_NEW, CMetadataTree::CMetaTreeCtrl::OnCreateItem)
EVT_MENU(ID_METATREE_EDIT, CMetadataTree::CMetaTreeCtrl::OnEditItem)
EVT_MENU(ID_METATREE_REMOVE, CMetadataTree::CMetaTreeCtrl::OnRemoveItem)
EVT_MENU(ID_METATREE_PROPERTY, CMetadataTree::CMetaTreeCtrl::OnPropertyItem)

EVT_MENU(ID_METATREE_INSERT, CMetadataTree::CMetaTreeCtrl::OnInsertItem)
EVT_MENU(ID_METATREE_REPLACE, CMetadataTree::CMetaTreeCtrl::OnReplaceItem)
EVT_MENU(ID_METATREE_SAVE, CMetadataTree::CMetaTreeCtrl::OnSaveItem)

EVT_SET_FOCUS(CMetadataTree::CMetaTreeCtrl::OnSetFocus)
EVT_KILL_FOCUS(CMetadataTree::CMetaTreeCtrl::OnSetFocus)

EVT_MENU(wxID_COPY, CMetadataTree::CMetaTreeCtrl::OnCopyItem)
EVT_MENU(wxID_PASTE, CMetadataTree::CMetaTreeCtrl::OnPasteItem)

wxEND_EVENT_TABLE()

CMetadataTree::CMetaTreeCtrl::CMetaTreeCtrl()
	: wxTreeCtrl(), m_ownerTree(nullptr), m_metaView(new CMatadataTreeView(this))
{
	//set double buffered
	SetDoubleBuffered(true);
}

CMetadataTree::CMetaTreeCtrl::CMetaTreeCtrl(CMetadataTree* parent)
	: wxTreeCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS | wxTR_SINGLE | wxTR_TWIST_BUTTONS), m_ownerTree(parent), m_metaView(new CMatadataTreeView(this))
{
	wxAcceleratorEntry entries[2];
	entries[0].Set(wxACCEL_CTRL, (int)'C', wxID_COPY);
	entries[1].Set(wxACCEL_CTRL, (int)'V', wxID_PASTE);

	wxAcceleratorTable accel(2, entries);
	SetAcceleratorTable(accel);

	//set double buffered
	SetDoubleBuffered(true);
}

#include "frontend/docView/docManager.h"

CMetadataTree::CMetaTreeCtrl::~CMetaTreeCtrl()
{
	if (docManager != nullptr && 
		m_metaView == docManager->GetAnyUsableView()) {
		docManager->ActivateView(nullptr);
	}

	wxDELETE(m_metaView);
}

/////////////////////////////////////////////////////////////////

#include "frontend/mainFrame/mainFrame.h"

void CMetadataTree::CMetaTreeCtrl::CMatadataTreeView::OnActivateView(bool activate, wxView* activeView, wxView* deactiveView)
{
	if (activate) {
		const wxTreeItemId& item = m_ownerTree->GetSelection();
		objectInspector->SelectObject(
			item.IsOk() ? m_ownerTree->GetMetaObject(item) : nullptr);
	}
}

/////////////////////////////////////////////////////////////////