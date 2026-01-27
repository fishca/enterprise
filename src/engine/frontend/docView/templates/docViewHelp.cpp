#include "docViewHelp.h"
#include "frontend/mainFrame/mainFrame.h"

// ----------------------------------------------------------------------------
// CHelpEditView implementation
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(CHelpEditView, CMetaView);

wxBEGIN_EVENT_TABLE(CHelpEditView, CMetaView)
EVT_MENU(wxID_COPY, CHelpEditView::OnCopy)
EVT_MENU(wxID_PASTE, CHelpEditView::OnPaste)
EVT_MENU(wxID_SELECTALL, CHelpEditView::OnSelectAll)
wxEND_EVENT_TABLE()

bool CHelpEditView::OnCreate(CMetaDocument* doc, long flags)
{
	m_textEditor = new CTextEditor(doc, m_viewFrame, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_THEME);

	m_textEditor->SetEditorSettings(mainFrame->GetEditorSettings());
	m_textEditor->SetFontColorSettings(mainFrame->GetFontColorSettings());

	m_textEditor->SetReadOnly(flags == wxDOC_READONLY);

	return CMetaView::OnCreate(doc, flags);
}

void CHelpEditView::OnDraw(wxDC* WXUNUSED(dc))
{
	// nothing to do here, wxTextCtrl draws itself
}

bool CHelpEditView::OnClose(bool deleteWindow)
{
	//Activate(false);

	if (deleteWindow) {
		GetFrame()->Destroy();
		SetFrame(nullptr);
	}

	if (CMetaView::OnClose(deleteWindow)) {
		m_textEditor->Freeze();
		return m_textEditor->Destroy();
	}

	return false;
}

#include "frontend/win/editor/textEditor/textEditorPrintOut.h"

wxPrintout* CHelpEditView::OnCreatePrintout()
{
	return new CTextEditorPrintout(m_textEditor, m_viewDocument->GetTitle());
}

void CHelpEditView::OnFind(wxFindDialogEvent& event)
{
	int wxflags = event.GetFlags();
	int sciflags = 0;
	if ((wxflags & wxFR_WHOLEWORD) != 0)
	{
		sciflags |= wxSTC_FIND_WHOLEWORD;
	}
	if ((wxflags & wxFR_MATCHCASE) != 0)
	{
		sciflags |= wxSTC_FIND_MATCHCASE;
	}
	int result;
	if ((wxflags & wxFR_DOWN) != 0)
	{
		m_textEditor->SetSelectionStart(m_textEditor->GetSelectionEnd());
		m_textEditor->SearchAnchor();
		result = m_textEditor->SearchNext(sciflags, event.GetFindString());
	}
	else
	{
		m_textEditor->SetSelectionEnd(m_textEditor->GetSelectionStart());
		m_textEditor->SearchAnchor();
		result = m_textEditor->SearchPrev(sciflags, event.GetFindString());
	}
	if (wxSTC_INVALID_POSITION == result)
	{
		wxMessageBox(wxString::Format(_("\"%s\" not found!"), event.GetFindString().c_str()), _("Not Found!"), wxICON_ERROR, (wxWindow*)event.GetClientData());
	}
	else
	{
		m_textEditor->EnsureCaretVisible();
	}
}

// ----------------------------------------------------------------------------
// IHelpDocument: wxDocument and wxTextCtrl married
// ----------------------------------------------------------------------------

wxIMPLEMENT_CLASS(IHelpDocument, CMetaDocument);

wxCommandProcessor* IHelpDocument::OnCreateCommandProcessor()
{
	CTextCommandProcessor* commandProcessor = new CTextCommandProcessor(GetTextCtrl());
	commandProcessor->SetEditMenu(mainFrame->GetDefaultMenu(wxID_EDIT));
	commandProcessor->Initialize();
	return commandProcessor;
}

CTextEditor* IHelpDocument::GetTextCtrl() const
{
	wxView* view = GetFirstView();
	return view ? wxDynamicCast(view, CHelpEditView)->GetText() : nullptr;
}

// ----------------------------------------------------------------------------
// CHelpFileDocument implementation
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(CHelpFileDocument, IHelpDocument);

bool CHelpFileDocument::OnCreate(const wxString& path, long flags)
{
	if (!CMetaDocument::OnCreate(path, flags))
		return false;

	return true;
}

// Since text windows have their own method for saving to/loading from files,
// we override DoSave/OpenDocument instead of Save/LoadObject
bool CHelpFileDocument::DoSaveDocument(const wxString& filename)
{
	return GetTextCtrl()->SaveFile(filename);
}

bool CHelpFileDocument::DoOpenDocument(const wxString& filename)
{
	if (!GetTextCtrl()->LoadFile(filename))
		return false;

	Modify(false);
	return true;
}