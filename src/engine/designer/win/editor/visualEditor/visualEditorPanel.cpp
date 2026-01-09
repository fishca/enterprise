////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxFormBuilder
//	Description : visual editor 
////////////////////////////////////////////////////////////////////////////

#include "visualEditor.h"
#include "titleFrame.h"
#include "frontend/visualView/ctrl/form.h"

wxIMPLEMENT_DYNAMIC_CLASS(CVisualEditorNotebook::CVisualEditor, wxPanel);

CVisualEditorNotebook::CVisualEditor::CVisualEditor() :
	wxPanel(), m_visualEditor(nullptr)
{
}

CVisualEditorNotebook::CVisualEditor::CVisualEditor(CMetaDocument* document, wxWindow* parent, int id) :
	wxPanel(parent, id),
	m_document(document), m_visualEditor(nullptr), m_cmdProc(new CCommandProcessor()), m_valueForm(nullptr)
{
	CreateWideGui();
}

void CVisualEditorNotebook::CVisualEditor::CreateWideGui()
{
	wxWindow::Freeze();

	m_splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_THIN_SASH);
	m_splitter->SetSashGravity(0.5);
	m_splitter->SetMinimumPaneSize(20); // Smalest size the

	wxBoxSizer* sizerMain = new wxBoxSizer(wxVERTICAL);
	sizerMain->Add(m_splitter, 1, wxEXPAND, 0);

	wxASSERT(m_visualEditor == nullptr);

	m_visualEditor = new CVisualEditorHost(this, m_splitter);
	m_objectTree = new CVisualEditorObjectTree(this, m_splitter);

	m_splitter->SplitHorizontally(m_visualEditor, 
		CPanelTitle::CreateTitle(m_objectTree, _("Tree elements")), 200);

	SetSizer(sizerMain);

	wxAcceleratorEntry entries[2];
	entries[0].Set(wxACCEL_CTRL, (int)'C', wxID_COPY);
	entries[1].Set(wxACCEL_CTRL, (int)'V', wxID_PASTE);

	wxAcceleratorTable accel(2, entries);
	SetAcceleratorTable(accel);

	wxWindow::Thaw();
}

#include "frontend/docView/docView.h" 
#include "frontend/mainFrame/mainFrame.h"

void CVisualEditorNotebook::CVisualEditor::ActivateEditor()
{
	objectInspector->SelectObject(GetSelectedObject());
	SetFocus();
}

#include "backend/metaCollection/partial/commonObject.h"
#include "backend/metadataConfiguration.h"

bool CVisualEditorNotebook::CVisualEditor::LoadForm()
{
	if (m_document == nullptr)
		return false;

	const IMetaObjectForm* creator = m_document->GetMetaObject()->ConvertToType<IMetaObjectForm>();

	if (creator == nullptr)
		return false;

	IMetaData* metaData = creator->GetMetaData();
	wxASSERT(metaData);

	IModuleManager* moduleManager = metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!moduleManager->FindCompileModule(creator, m_valueForm)) {
		m_valueForm = new CValueForm(creator, nullptr);
		if (!creator->LoadFormData(m_valueForm)) {
			wxDELETE(m_valueForm);
			return false;
		}
	}

	m_valueForm->IncrRef();
	m_visualEditor->Freeze();
	
	NotifyEditorLoaded();

	// first create control 
	m_visualEditor->CreateVisualHost();
		
	//refresh project
	NotifyEditorRefresh();

	m_visualEditor->Thaw();
	return true;
}

bool CVisualEditorNotebook::CVisualEditor::SaveForm()
{
	IMetaObjectForm* creator = m_document->ConvertMetaObjectToType<IMetaObjectForm>();

	// Create a std::string and copy your document data in to the string    
	if (creator != nullptr) {
		creator->SaveFormData(m_valueForm);
	}

	NotifyEditorSaved();
	return true;
}

void CVisualEditorNotebook::CVisualEditor::TestForm()
{
	m_valueForm->ShowForm(m_document, false);
}

CVisualEditorNotebook::CVisualEditor::~CVisualEditor()
{
	m_visualEditor->Destroy();
	m_objectTree->Destroy();
	m_splitter->Destroy();

	if (m_valueForm != nullptr) 
		m_valueForm->DecrRef();
	
	wxDELETE(m_cmdProc);
}