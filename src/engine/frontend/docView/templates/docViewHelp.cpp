#include "docViewHelp.h"
#include "frontend/mainFrame/mainFrame.h"

// ----------------------------------------------------------------------------
// ibHelpEditView implementation
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(ibHelpEditView, ibMetaView);

wxBEGIN_EVENT_TABLE(ibHelpEditView, ibMetaView)
EVT_MENU(wxID_COPY, ibHelpEditView::OnCopy)
EVT_MENU(wxID_PASTE, ibHelpEditView::OnPaste)
EVT_MENU(wxID_SELECTALL, ibHelpEditView::OnSelectAll)
wxEND_EVENT_TABLE()

bool ibHelpEditView::OnCreate(ibMetaDocument* doc, long flags)
{
	m_textEditor = new ibTextEditor(doc, m_viewFrame, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_THEME);

	m_textEditor->SetEditorSettings(mainFrame->GetEditorSettings());
	m_textEditor->SetFontColorSettings(mainFrame->GetFontColorSettings());

	m_textEditor->SetReadOnly(flags == wxDOC_READONLY);

	return ibMetaView::OnCreate(doc, flags);
}

void ibHelpEditView::OnDraw(wxDC* WXUNUSED(dc))
{
	// nothing to do here, wxTextCtrl draws itself
}

bool ibHelpEditView::OnClose(bool deleteWindow)
{
	//Activate(false);

	if (deleteWindow) {
		GetFrame()->Destroy();
		SetFrame(nullptr);
	}

	if (ibMetaView::OnClose(deleteWindow)) {
		
		m_textEditor->Freeze();
		
		m_textEditor->Destroy();
		m_textEditor = nullptr;

		return true;
	}

	return false;
}

#include "frontend/win/editor/textEditor/textEditorPrintOut.h"

wxPrintout* ibHelpEditView::OnCreatePrintout()
{
	return new ibTextEditorPrintout(m_textEditor, m_viewDocument->GetTitle());
}

void ibHelpEditView::OnFind(wxFindDialogEvent& event)
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
// ibHelpDocument: wxDocument and wxTextCtrl married
// ----------------------------------------------------------------------------

wxIMPLEMENT_CLASS(ibHelpDocument, ibMetaDocument);

wxCommandProcessor* ibHelpDocument::OnCreateCommandProcessor()
{
	ibTextCommandProcessor* commandProcessor = new ibTextCommandProcessor(GetTextCtrl());
	commandProcessor->SetEditMenu(mainFrame->GetDefaultMenu(wxID_EDIT));
	commandProcessor->Initialize();
	return commandProcessor;
}

ibTextEditor* ibHelpDocument::GetTextCtrl() const
{
	wxView* view = GetFirstView();
	return view ? wxDynamicCast(view, ibHelpEditView)->GetText() : nullptr;
}

// ----------------------------------------------------------------------------
// ibHelpFilibDocument implementation
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(ibHelpFilibDocument, ibHelpDocument);

bool ibHelpFilibDocument::OnCreate(const wxString& path, long flags)
{
	if (!ibMetaDocument::OnCreate(path, flags))
		return false;

	return true;
}

// Since text windows have their own method for saving to/loading from files,
// we override DoSave/OpenDocument instead of Save/LoadObject
bool ibHelpFilibDocument::DoSaveDocument(const wxString& filename)
{
	return GetTextCtrl()->SaveFile(filename);
}

bool ibHelpFilibDocument::DoOpenDocument(const wxString& filename)
{
	if (!GetTextCtrl()->LoadFile(filename))
		return false;

	Modify(false);
	return true;
}