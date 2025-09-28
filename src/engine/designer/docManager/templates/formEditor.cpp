#include "formEditor.h"
#include "frontend/mainFrame/mainFrame.h"

wxIMPLEMENT_DYNAMIC_CLASS(CFormEditView, CMetaView);

enum {
	wxID_ADD_COMMENTS = wxID_HIGHEST + 10000,
	wxID_REMOVE_COMMENTS,
	wxID_SYNTAX_CONTROL,
	wxID_GOTOLINE,
	wxID_PROCEDURES_FUNCTIONS,

	wxID_TEST_FORM = 15001
};

// ----------------------------------------------------------------------------
// CTextEditView implementation
// ----------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(CFormEditView, CMetaView)

EVT_MENU(wxID_COPY, CFormEditView::OnCopy)
EVT_MENU(wxID_PASTE, CFormEditView::OnPaste)
EVT_MENU(wxID_SELECTALL, CFormEditView::OnSelectAll)
EVT_FIND(wxID_ANY, CFormEditView::OnFind)
EVT_FIND_NEXT(wxID_ANY, CFormEditView::OnFind)

EVT_MENU(wxID_ADD_COMMENTS, CFormEditView::OnMenuEvent)
EVT_MENU(wxID_REMOVE_COMMENTS, CFormEditView::OnMenuEvent)
EVT_MENU(wxID_SYNTAX_CONTROL, CFormEditView::OnMenuEvent)
EVT_MENU(wxID_GOTOLINE, CFormEditView::OnMenuEvent)
EVT_MENU(wxID_PROCEDURES_FUNCTIONS, CFormEditView::OnMenuEvent)
EVT_MENU(wxID_TEST_FORM, CFormEditView::OnMenuEvent)

wxEND_EVENT_TABLE()

static wxWindowID control_id = wxID_HIGHEST + 3000;

#include "frontend/artProvider/artProvider.h"

bool CFormEditView::OnCreate(CMetaDocument* doc, long flags)
{
	m_visualNotebook = new CVisualEditorNotebook(doc, m_viewFrame, wxID_ANY, flags);

	wxWindowID id = control_id;

	for (auto controlClass : CValue::GetListCtorsByType(eCtorObjectType::eCtorObjectType_object_control)) {
		IControlTypeCtor* objectSingle = dynamic_cast<IControlTypeCtor*>(controlClass);
		wxASSERT(objectSingle);
		if (!objectSingle->IsControlSystem()) {
			wxBitmap controlImage = objectSingle->GetClassIcon();
			if (controlImage.IsOk()) {
				m_controlDataArray.push_back({ controlClass->GetClassName(), controlImage, id++ });
			}
		}
	}

	Bind(wxEVT_MENU, &CFormEditView::OnMenuEvent, this, control_id, id);

	if (!m_visualNotebook->LoadForm())
		return false;

	m_visualNotebook->RefreshEditor();
	return CMetaView::OnCreate(doc, flags);
}

void CFormEditView::OnCreateToolbar(wxAuiToolBar* toolbar)
{
	if (m_visualNotebook->GetSelection() == wxNOTEBOOK_PAGE_CODE_EDITOR) {
		if (!toolbar->GetToolCount()) {
			toolbar->AddTool(wxID_ADD_COMMENTS, _("Add comments"), wxArtProvider::GetBitmap(wxART_ADD_COMMENT, wxART_DOC_MODULE), _("Add"), wxItemKind::wxITEM_NORMAL);
			toolbar->EnableTool(wxID_ADD_COMMENTS, m_visualNotebook->IsEditable());
			toolbar->AddTool(wxID_REMOVE_COMMENTS, _("Remove comments"), wxArtProvider::GetBitmap(wxART_REMOVE_COMMENT, wxART_DOC_MODULE), _("Remove"), wxItemKind::wxITEM_NORMAL);
			toolbar->EnableTool(wxID_REMOVE_COMMENTS, m_visualNotebook->IsEditable());
			toolbar->AddSeparator();
			toolbar->AddTool(wxID_SYNTAX_CONTROL, _("Syntax control"), wxArtProvider::GetBitmap(wxART_SYNTAX_CONTROL, wxART_DOC_MODULE), _("Syntax"), wxItemKind::wxITEM_NORMAL);
			toolbar->EnableTool(wxID_SYNTAX_CONTROL, m_visualNotebook->IsEditable());
			toolbar->AddSeparator();
			toolbar->AddTool(wxID_GOTOLINE, _("Goto line"), wxArtProvider::GetBitmap(wxART_GOTO_LINE, wxART_DOC_MODULE), _("Goto"), wxItemKind::wxITEM_NORMAL);
			toolbar->AddTool(wxID_PROCEDURES_FUNCTIONS, _("Procedures and functions"), wxArtProvider::GetBitmap(wxART_PROC_AND_FUNC, wxART_DOC_MODULE), _("Procedures and functions"), wxItemKind::wxITEM_NORMAL);
		}
	}
	else {
		for (auto control : m_controlDataArray) {
			toolbar->AddTool(control.m_id, control.m_name, control.m_bmp, control.m_name);
			toolbar->EnableTool(control.m_id, m_visualNotebook->IsEditable());
		}
	}
}

#if wxUSE_MENUS	
wxMenuBar* CFormEditView::CreateMenuBar() const
{
	if (m_visualNotebook->GetSelection() == wxNOTEBOOK_PAGE_DESIGNER) {

		wxMenuBar* mb = new wxMenuBar;

		// and its menu bar
		wxMenu* menuForm = new wxMenu;
		menuForm->Append(wxID_TEST_FORM, _("Test form\tCtrl+R"));
		menuForm->AppendSeparator();

		wxMenu* menuControl = new wxMenu;

		for (auto control : m_controlDataArray) {
			wxMenuItem* menuItem = menuControl->Append(control.m_id,
				control.m_name, control.m_name);
			menuItem->Enable(m_visualNotebook->IsEditable());
			menuItem->SetBitmap(control.m_bmp);
		}

		menuForm->AppendSubMenu(menuControl, _("Controls"));

		mb->Append(menuForm, _("Form"));
		return mb;
	}

	return nullptr;
}
#endif 

void CFormEditView::OnActivateView(bool activate, wxView* activeView, wxView* deactiveView)
{
	CMetaView::OnActivateView(activate, activeView, deactiveView);
}

void CFormEditView::OnUpdate(wxView* sender, wxObject* hint)
{
	if (m_visualNotebook != nullptr)
		m_visualNotebook->RefreshEditor();
}

void CFormEditView::OnDraw(wxDC* WXUNUSED(dc))
{
	// nothing to do here, wxTextCtrl draws itself
}

bool CFormEditView::OnClose(bool deleteWindow)
{
	//Activate(false);

	if (deleteWindow) {
		GetFrame()->Destroy();
		SetFrame(nullptr);
	}

	if (CMetaView::OnClose(deleteWindow)) {
		m_visualNotebook->Freeze();
		return m_visualNotebook->Destroy();
	}

	return false;
}

#include "win/editor/codeEditor/codeEditorPrintOut.h"
#include "win/editor/visualEditor/printout/formPrintOut.h"

wxPrintout* CFormEditView::OnCreatePrintout()
{
	if (m_visualNotebook->GetSelection() == wxNOTEBOOK_PAGE_CODE_EDITOR) {
		return new CCodeEditorPrintout(
			m_visualNotebook->GetCodeEditor(), this->GetViewName()
		);
	}

	return new CFormPrintout(m_visualNotebook->GetVisualHost());
}

void CFormEditView::OnMenuEvent(wxCommandEvent& event)
{
	wxWindowID id = event.GetId();

	if (m_visualNotebook->GetSelection() == wxNOTEBOOK_PAGE_CODE_EDITOR) {
		CCodeEditor* codeEditor = m_visualNotebook->GetCodeEditor();
		if (id == wxID_ADD_COMMENTS) {
			int nStartLine, nEndLine;
			codeEditor->GetSelection(&nStartLine, &nEndLine);
			for (int line = codeEditor->LineFromPosition(nStartLine); line <= codeEditor->LineFromPosition(nEndLine); line++) {
				codeEditor->Replace(codeEditor->PositionFromLine(line), codeEditor->PositionFromLine(line), "//");
			}
		}
		else if (id == wxID_REMOVE_COMMENTS) {
			int nStartLine, nEndLine;
			codeEditor->GetSelection(&nStartLine, &nEndLine);
			for (int line = codeEditor->LineFromPosition(nStartLine); line <= codeEditor->LineFromPosition(nEndLine); line++) {
				int startPos = codeEditor->PositionFromLine(line);
				wxString sLine = codeEditor->GetLineRaw(line);
				for (unsigned int i = 0; i < sLine.length(); i++) {
					if (sLine[i] == '/'
						&& (i + 1 < sLine.length() && sLine[i + 1] == '/')) {
						codeEditor->Replace(startPos + i, startPos + i + 2, wxEmptyString); break;
					}
				}
			}
		}
		else if (id == wxID_SYNTAX_CONTROL) {
			m_visualNotebook->SyntaxControl();
		}
		else if (id == wxID_GOTOLINE) {
			m_visualNotebook->ShowGotoLine();
		}
		else if (id == wxID_PROCEDURES_FUNCTIONS) {
			m_visualNotebook->ShowMethods();
		}
	}
	else if (m_visualNotebook->GetSelection() == wxNOTEBOOK_PAGE_DESIGNER) {

		if (id == wxID_TEST_FORM) {
			m_visualNotebook->TestForm();
		}
		else {
			auto it = std::find_if(
				m_controlDataArray.begin(), m_controlDataArray.end(), [id](CControlData& ctrl) { return id == ctrl.m_id; });
			if (it != m_controlDataArray.end()) m_visualNotebook->CreateControl(it->m_name);
		}
	}
}

// ----------------------------------------------------------------------------
// CModuleDocument: wxDocument and wxTextCtrl married
// ----------------------------------------------------------------------------

wxIMPLEMENT_CLASS(CFormDocument, CMetaDocument);

wxCommandProcessor* CFormDocument::CreateCommandProcessor() const
{
	CVisualDesignerCommandProcessor* commandProcessor = new CVisualDesignerCommandProcessor(GetVisualNotebook());
	commandProcessor->SetEditMenu(mainFrame->GetDefaultMenu(wxID_EDIT));
	commandProcessor->Initialize();
	return commandProcessor;
}

bool CFormDocument::OnCreate(const wxString& path, long flags)
{
	if (!CMetaDocument::OnCreate(path, flags))
		return false;

	return true;
}

bool CFormDocument::OnOpenDocument(const wxString& filename)
{
	return CMetaDocument::OnOpenDocument(filename);
}

bool CFormDocument::OnSaveDocument(const wxString& filename)
{
	return GetVisualNotebook()->SaveForm();
}

bool CFormDocument::OnSaveModified()
{
	return CMetaDocument::OnSaveModified();
}

bool CFormDocument::OnCloseDocument()
{
	CVisualEditorNotebook* visualNotebook = GetVisualNotebook();
	if (visualNotebook != nullptr && visualNotebook->IsEditable()) {
		if (!visualNotebook->SyntaxControl(false))
			return false;
	}

	objectInspector->SelectObject((IPropertyObject*)m_metaObject);
	return CMetaDocument::OnCloseDocument();
}

bool CFormDocument::IsModified() const
{
	//wxStyledTextCtrl* wnd = GetCodeEditor();
	return CMetaDocument::IsModified();// || (wnd && wnd->IsModified());
}

void CFormDocument::Modify(bool modified)
{
	CMetaDocument::Modify(modified);
}

bool CFormDocument::Save()
{
	CVisualEditorNotebook* visualNotebook = GetVisualNotebook();
	if (visualNotebook != nullptr && visualNotebook->IsEditable()) {
		if (!visualNotebook->SyntaxControl(false))
			return false;
	}
	return CMetaDocument::Save();
}

// ----------------------------------------------------------------------------
// CTextEditDocument implementation
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(CFormEditDocument, CFormDocument);

CVisualEditorNotebook* CFormEditDocument::GetVisualNotebook() const
{
	wxView* view = GetFirstView();
	return view ? wxDynamicCast(view, CFormEditView)->GetVisualNotebook() : nullptr;
}