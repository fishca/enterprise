#include "docViewModuleEditor.h"
#include "frontend/mainFrame/mainFrame.h"

enum {
	wxID_ADD_COMMENTS = wxID_HIGHEST + 10000,
	wxID_REMOVE_COMMENTS,
	wxID_SYNTAX_CONTROL,
	wxID_GOTOLINE,
	wxID_PROCEDURES_FUNCTIONS
};

// ----------------------------------------------------------------------------
// ibTextEditView implementation
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(ibModuleEditView, ibMetaView);

wxBEGIN_EVENT_TABLE(ibModuleEditView, ibMetaView)
EVT_MENU(wxID_COPY, ibModuleEditView::OnCopy)
EVT_MENU(wxID_PASTE, ibModuleEditView::OnPaste)
EVT_MENU(wxID_SELECTALL, ibModuleEditView::OnSelectAll)
EVT_FIND(wxID_ANY, ibModuleEditView::OnFind)
EVT_FIND_NEXT(wxID_ANY, ibModuleEditView::OnFind)

EVT_MENU(wxID_ADD_COMMENTS, ibModuleEditView::OnMenuEvent)
EVT_MENU(wxID_REMOVE_COMMENTS, ibModuleEditView::OnMenuEvent)
EVT_MENU(wxID_SYNTAX_CONTROL, ibModuleEditView::OnMenuEvent)
EVT_MENU(wxID_GOTOLINE, ibModuleEditView::OnMenuEvent)
EVT_MENU(wxID_PROCEDURES_FUNCTIONS, ibModuleEditView::OnMenuEvent)

wxEND_EVENT_TABLE()

bool ibModuleEditView::OnCreate(ibMetaDocument* doc, long flags)
{
	m_codeEditor = new ibCodeEditor(doc, m_viewFrame, wxID_ANY,
		wxDefaultPosition, wxDefaultSize, wxBORDER_THEME);

	m_codeEditor->SetReadOnly(flags == wxDOC_READONLY);
	m_codeEditor->SetSTCFocus(true);

	return ibMetaView::OnCreate(doc, flags)
		&& m_codeEditor->LoadModule();
}

void ibModuleEditView::OnActivateView(bool activate, wxView* activeView, wxView* deactiveView)
{
	if (activate) m_codeEditor->ActivateEditor();
}

void ibModuleEditView::OnDraw(wxDC* WXUNUSED(dc))
{
	// nothing to do here, wxTextCtrl draws itself
}

void ibModuleEditView::OnUpdate(wxView* sender, wxObject* hint)
{
	if (m_codeEditor != nullptr)
		m_codeEditor->RefreshEditor();
}

bool ibModuleEditView::OnClose(bool deleteWindow)
{
	//Activate(false);

	if (deleteWindow) {
		GetFrame()->Destroy();
		SetFrame(nullptr);
	}

	if (ibMetaView::OnClose(deleteWindow)) {

		m_codeEditor->Freeze();

		m_codeEditor->Destroy();
		m_codeEditor = nullptr;

		return true;
	}

	return false;
}

#include "win/editor/codeEditor/codeEditorPrintOut.h"

wxPrintout* ibModuleEditView::OnCreatePrintout()
{
	return new CCodeEditorPrintout(m_codeEditor, m_viewDocument->GetTitle());
}

#include "frontend/artProvider/artProvider.h"

void ibModuleEditView::OnCreateToolbar(wxAuiToolBar* toolbar)
{
	if (m_codeEditor == nullptr)
		return;

	if (!toolbar->GetToolCount()) {
		toolbar->AddTool(wxID_ADD_COMMENTS, _("Add comments"), wxArtProvider::GetBitmap(wxART_ADD_COMMENT, wxART_DOC_MODULE), _("Add"), wxItemKind::wxITEM_NORMAL);
		toolbar->EnableTool(wxID_ADD_COMMENTS, m_codeEditor->IsEditable());
		toolbar->AddTool(wxID_REMOVE_COMMENTS, _("Remove comments"), wxArtProvider::GetBitmap(wxART_REMOVE_COMMENT, wxART_DOC_MODULE), _("Remove"), wxItemKind::wxITEM_NORMAL);
		toolbar->EnableTool(wxID_REMOVE_COMMENTS, m_codeEditor->IsEditable());
		toolbar->AddSeparator();
		toolbar->AddTool(wxID_SYNTAX_CONTROL, _("Syntax control"), wxArtProvider::GetBitmap(wxART_SYNTAX_CONTROL, wxART_DOC_MODULE), _("Syntax"), wxItemKind::wxITEM_NORMAL);
		toolbar->EnableTool(wxID_SYNTAX_CONTROL, m_codeEditor->IsEditable());
		toolbar->AddSeparator();
		toolbar->AddTool(wxID_GOTOLINE, _("Goto line"), wxArtProvider::GetBitmap(wxART_GOTO_LINE, wxART_DOC_MODULE), _("Goto"), wxItemKind::wxITEM_NORMAL);
		toolbar->AddTool(wxID_PROCEDURES_FUNCTIONS, _("Procedures and functions"), wxArtProvider::GetBitmap(wxART_PROC_AND_FUNC, wxART_DOC_MODULE), _("Procedures and functions"), wxItemKind::wxITEM_NORMAL);
	}
}

#include "win/dlg/lineInput/lineInput.h"
#include "win/dlg/functionSearcher/functionSearcher.h"

void ibModuleEditView::OnMenuEvent(wxCommandEvent& event)
{
	if (event.GetId() == wxID_ADD_COMMENTS) {
		int nStartLine, nEndLine;
		m_codeEditor->GetSelection(&nStartLine, &nEndLine);
		for (int line = m_codeEditor->LineFromPosition(nStartLine); line <= m_codeEditor->LineFromPosition(nEndLine); line++) {
			m_codeEditor->Replace(m_codeEditor->PositionFromLine(line), m_codeEditor->PositionFromLine(line), "//");
		}
	}
	else if (event.GetId() == wxID_REMOVE_COMMENTS) {
		int nStartLine, nEndLine;
		m_codeEditor->GetSelection(&nStartLine, &nEndLine);
		for (int line = m_codeEditor->LineFromPosition(nStartLine); line <= m_codeEditor->LineFromPosition(nEndLine); line++) {
			int startPos = m_codeEditor->PositionFromLine(line);
			wxString sLine = m_codeEditor->GetLineRaw(line);
			for (unsigned int i = 0; i < sLine.length(); i++) {
				if (sLine[i] == '/'
					&& (i + 1 < sLine.length() && sLine[i + 1] == '/')) {
					m_codeEditor->Replace(startPos + i, startPos + i + 2, wxEmptyString); break;
				}
			}
		}
	}
	else if (event.GetId() == wxID_SYNTAX_CONTROL) {
		m_codeEditor->SyntaxControl();
	}
	else if (event.GetId() == wxID_GOTOLINE) {
		m_codeEditor->ShowGotoLine();
	}
	else if (event.GetId() == wxID_PROCEDURES_FUNCTIONS) {
		m_codeEditor->ShowMethods();
	}
}

// ----------------------------------------------------------------------------
// ibModulibDocument: wxDocument and wxTextCtrl married
// ----------------------------------------------------------------------------

wxIMPLEMENT_CLASS(ibModulibDocument, ibMetaDocument);

bool ibModulibDocument::OnCreate(const wxString& path, long flags)
{
	if (!ibMetaDocument::OnCreate(path, flags))
		return false;

	return true;
}

bool ibModulibDocument::OnOpenDocument(const wxString& filename)
{
	return ibMetaDocument::OnOpenDocument(filename);
}

bool ibModulibDocument::OnSaveDocument(const wxString& filename)
{
	return GetCodeEditor()->SaveModule();
}

bool ibModulibDocument::OnSaveModified()
{
	return ibMetaDocument::OnSaveModified();
}

bool ibModulibDocument::OnCloseDocument()
{
	ibCodeEditor* codeEditor = GetCodeEditor();
	if (codeEditor != nullptr &&
		codeEditor->IsEditable()) {
		if (!codeEditor->SyntaxControl(false))
			return false;
	}
	return ibMetaDocument::OnCloseDocument();
}

wxCommandProcessor* ibModulibDocument::OnCreateCommandProcessor()
{
	ibModuleCommandProcessor* commandProcessor = new ibModuleCommandProcessor(GetCodeEditor());
	commandProcessor->SetEditMenu(mainFrame->GetDefaultMenu(wxID_EDIT));
	commandProcessor->Initialize();
	return commandProcessor;
}

// Since text windows have their own method for saving to/loading from files,
// we override DoSave/OpenDocument instead of Save/LoadObject
bool ibModulibDocument::DoSaveDocument(const wxString& filename)
{
	return GetCodeEditor()->SaveFile(filename);
}

bool ibModulibDocument::DoOpenDocument(const wxString& filename)
{
	if (!GetCodeEditor()->LoadFile(filename))
		return false;

	return true;
}

bool ibModulibDocument::IsModified() const
{
	//wxStyledTextCtrl* wnd = GetCodeEditor();
	return ibMetaDocument::IsModified();// || (wnd && wnd->IsModified());
}

void ibModulibDocument::Modify(bool modified)
{
	ibMetaDocument::Modify(modified);
}

bool ibModulibDocument::Save()
{
	ibCodeEditor* codeEditor = GetCodeEditor();
	if (codeEditor != nullptr &&
		codeEditor->IsEditable()) {
		if (!codeEditor->SyntaxControl(false))
			return false;
	}
	return ibMetaDocument::Save();
}

// ----------------------------------------------------------------------------
// ibTextFilibDocument implementation
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(ibModuleEditDocument, ibModulibDocument);

ibCodeEditor* ibModuleEditDocument::GetCodeEditor() const
{
	wxView* view = GetFirstView();
	return view ? wxDynamicCast(view, ibModuleEditView)->GetCodeEditor() : nullptr;
}

void ibModuleEditDocument::SetCurrentLine(int lineBreakpoint, bool setBreakpoint)
{
	ibCodeEditor* autoComplete = GetCodeEditor();
	wxASSERT(autoComplete);
	autoComplete->SetCurrentLine(lineBreakpoint, setBreakpoint);
}

void ibModuleEditDocument::SetToolTip(const wxString& resultStr) {
	ibCodeEditor* codeEditor = GetCodeEditor();
	wxASSERT(codeEditor);
	if (codeEditor != nullptr) {
		codeEditor->SetToolTip(resultStr);
	}
}

void ibModuleEditDocument::ShowAutoComplete(const ibDebugAutoCompleteData& debugData)
{
	ibCodeEditor* codeEditor = GetCodeEditor();
	wxASSERT(codeEditor);
	if (codeEditor != nullptr) {
		codeEditor->ShowAutoComplete(debugData);
	}
}
