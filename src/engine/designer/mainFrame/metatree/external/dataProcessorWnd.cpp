////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : dataprocessor window
////////////////////////////////////////////////////////////////////////////

#include "dataProcessorWnd.h"
#include "backend/debugger/debugClient.h"
#include "frontend/win/theme/luna_toolbarart.h"

#include "docManager/templates/dataProcessorFile.h"

#include <wx/artprov.h>

wxIMPLEMENT_DYNAMIC_CLASS(CDataProcessorTree, wxPanel);

#define ICON_SIZE 16

CDataProcessorTree::CDataProcessorTree(CMetaDocument* docParent, wxWindow* parent, wxWindowID id)
	: IMetaDataTree(docParent, parent, id), m_metaData(nullptr), m_initialized(false)
{
	this->SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxBoxSizer* bSizerMain = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* bSizerHeader = new wxBoxSizer(wxHORIZONTAL);

	wxBoxSizer* bSizerCaption = new wxBoxSizer(wxVERTICAL);

	m_nameCaption = new wxStaticText(this, wxID_ANY, wxT("Name"), wxDefaultPosition, wxDefaultSize, 0);
	m_nameCaption->Wrap(-1);

	bSizerCaption->Add(m_nameCaption, 0, wxALL, 5);

	m_synonymCaption = new wxStaticText(this, wxID_ANY, wxT("Synonym"), wxDefaultPosition, wxDefaultSize, 0);
	m_synonymCaption->Wrap(-1);

	bSizerCaption->Add(m_synonymCaption, 0, wxALL, 5);

	m_commentCaption = new wxStaticText(this, wxID_ANY, wxT("Comment"), wxDefaultPosition, wxDefaultSize, 0);
	m_commentCaption->Wrap(-1);

	bSizerCaption->Add(m_commentCaption, 0, wxALL, 5);

	m_defaultForm = new wxStaticText(this, wxID_ANY, wxT("Default form:"), wxDefaultPosition, wxDefaultSize, 0);
	m_defaultForm->Wrap(-1);

	bSizerCaption->Add(m_defaultForm, 0, wxALL, 5);
	bSizerHeader->Add(bSizerCaption, 0, wxEXPAND, 5);

	wxBoxSizer* bSizerValue = new wxBoxSizer(wxVERTICAL);

	m_nameValue = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	bSizerValue->Add(m_nameValue, 1, wxALL | wxEXPAND, 1);
	m_nameValue->Connect(wxEVT_TEXT, wxCommandEventHandler(CDataProcessorTree::OnEditCaptionName), nullptr, this);

	m_synonymValue = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	bSizerValue->Add(m_synonymValue, 1, wxALL | wxEXPAND, 1);
	m_synonymValue->Connect(wxEVT_TEXT, wxCommandEventHandler(CDataProcessorTree::OnEditCaptionSynonym), nullptr, this);

	m_commentValue = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	bSizerValue->Add(m_commentValue, 1, wxALL | wxEXPAND, 1);
	m_commentValue->Connect(wxEVT_TEXT, wxCommandEventHandler(CDataProcessorTree::OnEditCaptionComment), nullptr, this);

	m_defaultFormValue = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, NULL, 0);
	m_defaultFormValue->AppendString(_("<not selected>"));
	m_defaultFormValue->SetSelection(0);

	m_defaultFormValue->Connect(wxEVT_CHOICE, wxCommandEventHandler(CDataProcessorTree::OnChoiceDefForm), nullptr, this);

	bSizerValue->Add(m_defaultFormValue, 1, wxALL | wxEXPAND, 1);
	bSizerHeader->Add(bSizerValue, 1, 0, 5);
	bSizerMain->Add(bSizerHeader, 0, wxEXPAND, 5);

	wxStaticBoxSizer* sbSizerTree = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, wxT("")), wxVERTICAL);

	m_metaTreeToolbar = new wxAuiToolBar(sbSizerTree->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_HORZ_LAYOUT);
	m_metaTreeToolbar->AddTool(ID_METATREE_NEW, _("New"), wxArtProvider::GetBitmap(wxART_PLUS, wxART_MENU), _("New item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_EDIT, _("Edit"), wxArtProvider::GetBitmap(wxART_EDIT, wxART_MENU), _("Edit item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_REMOVE, _("remove"), wxArtProvider::GetBitmap(wxART_DELETE, wxART_MENU), _("Remove item"));
	m_metaTreeToolbar->AddSeparator();
	m_metaTreeToolbar->AddTool(ID_METATREE_UP, _("Up"), wxArtProvider::GetBitmap(wxART_GO_UP, wxART_MENU), _("Up item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_DOWM, _("Down"), wxArtProvider::GetBitmap(wxART_GO_DOWN, wxART_MENU), _("Down item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_SORT, _("Sort"), wxArtProvider::GetBitmap(wxART_HELP_SETTINGS, wxART_MENU), _("Sort item"));
	m_metaTreeToolbar->Realize();

	m_metaTreeToolbar->SetArtProvider(new wxAuiLunaToolBarArt());

	sbSizerTree->Add(m_metaTreeToolbar, 0, wxALL | wxEXPAND, 0);

	m_metaTreeWnd = new CDataProcessorTreeCtrl(sbSizerTree->GetStaticBox(), this);
	m_metaTreeWnd->SetBackgroundColour(RGB(250, 250, 250));

	//set image list
	m_metaTreeWnd->SetImageList(
		new wxImageList(ICON_SIZE, ICON_SIZE)
	);

	m_metaTreeToolbar->Bind(wxEVT_MENU, &CDataProcessorTree::CDataProcessorTreeCtrl::OnCreateItem, m_metaTreeWnd, ID_METATREE_NEW);
	m_metaTreeToolbar->Bind(wxEVT_MENU, &CDataProcessorTree::CDataProcessorTreeCtrl::OnEditItem, m_metaTreeWnd, ID_METATREE_EDIT);
	m_metaTreeToolbar->Bind(wxEVT_MENU, &CDataProcessorTree::CDataProcessorTreeCtrl::OnRemoveItem, m_metaTreeWnd, ID_METATREE_REMOVE);

	m_metaTreeToolbar->Bind(wxEVT_MENU, &CDataProcessorTree::CDataProcessorTreeCtrl::OnUpItem, m_metaTreeWnd, ID_METATREE_UP);
	m_metaTreeToolbar->Bind(wxEVT_MENU, &CDataProcessorTree::CDataProcessorTreeCtrl::OnDownItem, m_metaTreeWnd, ID_METATREE_DOWM);
	m_metaTreeToolbar->Bind(wxEVT_MENU, &CDataProcessorTree::CDataProcessorTreeCtrl::OnSortItem, m_metaTreeWnd, ID_METATREE_SORT);

	sbSizerTree->Add(m_metaTreeWnd, 1, wxALL | wxEXPAND, 0);

	bSizerMain->Add(sbSizerTree, 1, wxEXPAND, 5);

	CMetaDataDataProcessor* metaData = ((CDataProcessorFileDocument*)docParent)->GetMetaData();
	CMetaObjectDataProcessor* commonMeta = metaData->GetDataProcessor();
	CMetaObjectModule *moduleMeta = commonMeta->GetModuleObject();

	m_buttonModule = new wxButton(this, wxID_ANY, _("Open module"));
	m_buttonModule->Connect(wxEVT_BUTTON, wxCommandEventHandler(CDataProcessorTree::OnButtonModuleClicked), nullptr, this);
	m_buttonModule->SetBitmap(moduleMeta->GetIcon());

	bSizerMain->Add(m_buttonModule, 0, wxALL);

	this->SetSizer(bSizerMain);
	this->Layout();

	this->Centre(wxBOTH);

	//Initialize tree
	InitTree();
}

CDataProcessorTree::~CDataProcessorTree()
{
	m_nameValue->Disconnect(wxEVT_TEXT, wxCommandEventHandler(CDataProcessorTree::OnEditCaptionName), nullptr, this);
	m_synonymValue->Disconnect(wxEVT_TEXT, wxCommandEventHandler(CDataProcessorTree::OnEditCaptionSynonym), nullptr, this);
	m_commentValue->Disconnect(wxEVT_TEXT, wxCommandEventHandler(CDataProcessorTree::OnEditCaptionComment), nullptr, this);

	m_metaTreeToolbar->Unbind(wxEVT_MENU, &CDataProcessorTree::CDataProcessorTreeCtrl::OnCreateItem, m_metaTreeWnd, ID_METATREE_NEW);
	m_metaTreeToolbar->Unbind(wxEVT_MENU, &CDataProcessorTree::CDataProcessorTreeCtrl::OnEditItem, m_metaTreeWnd, ID_METATREE_EDIT);
	m_metaTreeToolbar->Unbind(wxEVT_MENU, &CDataProcessorTree::CDataProcessorTreeCtrl::OnRemoveItem, m_metaTreeWnd, ID_METATREE_REMOVE);

	m_metaTreeToolbar->Unbind(wxEVT_MENU, &CDataProcessorTree::CDataProcessorTreeCtrl::OnUpItem, m_metaTreeWnd, ID_METATREE_UP);
	m_metaTreeToolbar->Unbind(wxEVT_MENU, &CDataProcessorTree::CDataProcessorTreeCtrl::OnDownItem, m_metaTreeWnd, ID_METATREE_DOWM);
	m_metaTreeToolbar->Unbind(wxEVT_MENU, &CDataProcessorTree::CDataProcessorTreeCtrl::OnSortItem, m_metaTreeWnd, ID_METATREE_SORT);

	m_buttonModule->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(CDataProcessorTree::OnButtonModuleClicked), nullptr, this);
}

void CDataProcessorTree::OnEditCaptionName(wxCommandEvent& event)
{
	wxString systemName = m_nameValue->GetValue();

	int pos = stringUtils::CheckCorrectName(systemName);
	if (pos > 0) {
		systemName = systemName.Left(pos);
		m_nameValue->SetValue(systemName);
		return;
	}

	wxString synonym = stringUtils::GenerateSynonym(systemName);

	m_docParent->SetFilename(systemName);
	m_docParent->SetTitle(systemName);

	CMetaObjectDataProcessor* dataProcessor = m_metaData->GetDataProcessor();
	wxASSERT(dataProcessor);
	dataProcessor->SetName(systemName);
	dataProcessor->SetSynonym(synonym);

	m_synonymValue->SetValue(synonym);

	m_docParent->OnChangeFilename(true);

	if (m_initialized) {
		m_metaData->Modify(true);
	}
}

void CDataProcessorTree::OnEditCaptionSynonym(wxCommandEvent& event)
{
	CMetaObjectDataProcessor* dataProcessor = m_metaData->GetDataProcessor();
	wxASSERT(dataProcessor);
	dataProcessor->SetSynonym(m_synonymValue->GetValue());

	if (m_initialized) {
		m_metaData->Modify(true);
	}
}

void CDataProcessorTree::OnEditCaptionComment(wxCommandEvent& event)
{
	CMetaObjectDataProcessor* dataProcessor = m_metaData->GetDataProcessor();
	wxASSERT(dataProcessor);
	dataProcessor->SetComment(m_commentValue->GetValue());

	if (m_initialized) {
		m_metaData->Modify(true);
	}
}

void CDataProcessorTree::OnChoiceDefForm(wxCommandEvent& event)
{
	CMetaObjectDataProcessor* dataProcessor = m_metaData->GetDataProcessor();
	wxASSERT(dataProcessor);

	const meta_identifier_t id = reinterpret_cast<meta_identifier_t>(event.GetClientData());
	if (id > 0) {
		dataProcessor->SetDefFormObject(id);
	}
	else {
		dataProcessor->SetDefFormObject(wxNOT_FOUND);
	}

	if (m_initialized) {
		m_metaData->Modify(true);
	}
}

void CDataProcessorTree::OnButtonModuleClicked(wxCommandEvent& event)
{
	CMetaObjectDataProcessor* dataProcessor = m_metaData->GetDataProcessor();
	wxASSERT(dataProcessor);
	OpenFormMDI(dataProcessor->GetModuleObject());
}

wxIMPLEMENT_DYNAMIC_CLASS(CDataProcessorTree::CDataProcessorTreeCtrl, wxTreeCtrl);

//**********************************************************************************
//*                                  metatree window						       *
//**********************************************************************************

wxBEGIN_EVENT_TABLE(CDataProcessorTree::CDataProcessorTreeCtrl, wxTreeCtrl)

EVT_LEFT_UP(CDataProcessorTree::CDataProcessorTreeCtrl::OnLeftUp)
EVT_LEFT_DOWN(CDataProcessorTree::CDataProcessorTreeCtrl::OnLeftDown)
EVT_LEFT_DCLICK(CDataProcessorTree::CDataProcessorTreeCtrl::OnLeftDClick)
EVT_RIGHT_UP(CDataProcessorTree::CDataProcessorTreeCtrl::OnRightUp)
EVT_RIGHT_DOWN(CDataProcessorTree::CDataProcessorTreeCtrl::OnRightDown)
EVT_RIGHT_DCLICK(CDataProcessorTree::CDataProcessorTreeCtrl::OnRightDClick)
EVT_MOTION(CDataProcessorTree::CDataProcessorTreeCtrl::OnMouseMove)
EVT_KEY_UP(CDataProcessorTree::CDataProcessorTreeCtrl::OnKeyUp)
EVT_KEY_DOWN(CDataProcessorTree::CDataProcessorTreeCtrl::OnKeyDown)

EVT_TREE_SEL_CHANGING(wxID_ANY, CDataProcessorTree::CDataProcessorTreeCtrl::OnSelecting)
EVT_TREE_SEL_CHANGED(wxID_ANY, CDataProcessorTree::CDataProcessorTreeCtrl::OnSelected)

EVT_TREE_ITEM_COLLAPSING(wxID_ANY, CDataProcessorTree::CDataProcessorTreeCtrl::OnCollapsing)
EVT_TREE_ITEM_EXPANDING(wxID_ANY, CDataProcessorTree::CDataProcessorTreeCtrl::OnExpanding)

EVT_MENU(ID_METATREE_NEW, CDataProcessorTree::CDataProcessorTreeCtrl::OnCreateItem)
EVT_MENU(ID_METATREE_EDIT, CDataProcessorTree::CDataProcessorTreeCtrl::OnEditItem)
EVT_MENU(ID_METATREE_REMOVE, CDataProcessorTree::CDataProcessorTreeCtrl::OnRemoveItem)
EVT_MENU(ID_METATREE_PROPERTY, CDataProcessorTree::CDataProcessorTreeCtrl::OnPropertyItem)

EVT_SET_FOCUS(CDataProcessorTree::CDataProcessorTreeCtrl::OnSetFocus)
EVT_KILL_FOCUS(CDataProcessorTree::CDataProcessorTreeCtrl::OnSetFocus)

EVT_MENU(wxID_COPY, CDataProcessorTree::CDataProcessorTreeCtrl::OnCopyItem)
EVT_MENU(wxID_PASTE, CDataProcessorTree::CDataProcessorTreeCtrl::OnPasteItem)

wxEND_EVENT_TABLE()

CDataProcessorTree::CDataProcessorTreeCtrl::CDataProcessorTreeCtrl()
	: wxTreeCtrl(), m_ownerTree(nullptr), m_metaView(new CMetaView)
{
	//set double buffered
	SetDoubleBuffered(true);
}

CDataProcessorTree::CDataProcessorTreeCtrl::CDataProcessorTreeCtrl(wxWindow* parentWnd, CDataProcessorTree* ownerWnd)
	: wxTreeCtrl(parentWnd, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS | wxTR_SINGLE | wxTR_HIDE_ROOT | wxTR_TWIST_BUTTONS), m_ownerTree(ownerWnd), m_metaView(new CMetaView)
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

CDataProcessorTree::CDataProcessorTreeCtrl::~CDataProcessorTreeCtrl()
{
	if (docManager != nullptr &&
		m_metaView == docManager->GetAnyUsableView()) {
		docManager->ActivateView(nullptr);
	}
	
	wxDELETE(m_metaView);
}