#include "docViewText.h"
#include "frontend/mainFrame/mainFrame.h"

// ----------------------------------------------------------------------------
// ibTextEditView implementation
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(ibTextEditView, ibMetaView);

wxBEGIN_EVENT_TABLE(ibTextEditView, ibMetaView)
EVT_MENU(wxID_COPY, ibTextEditView::OnCopy)
EVT_MENU(wxID_PASTE, ibTextEditView::OnPaste)
EVT_MENU(wxID_SELECTALL, ibTextEditView::OnSelectAll)
wxEND_EVENT_TABLE()

bool ibTextEditView::OnCreate(ibMetaDocument* doc, long flags)
{
	m_textEditor = new ibTextEditor(doc, m_viewFrame, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_THEME);

	m_textEditor->SetEditorSettings(mainFrame->GetEditorSettings());
	m_textEditor->SetFontColorSettings(mainFrame->GetFontColorSettings());

	m_textEditor->SetReadOnly(flags == wxDOC_READONLY);

	return ibMetaView::OnCreate(doc, flags);
}

void ibTextEditView::OnDraw(wxDC* WXUNUSED(dc))
{
	// nothing to do here, wxTextCtrl draws itself
}

bool ibTextEditView::OnClose(bool deleteWindow)
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

wxPrintout* ibTextEditView::OnCreatePrintout()
{
	return new ibTextEditorPrintout(m_textEditor, m_viewDocument->GetTitle());
}

void ibTextEditView::OnFind(wxFindDialogEvent& event)
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
// ITextDocument: wxDocument and wxTextCtrl married
// ----------------------------------------------------------------------------

wxIMPLEMENT_CLASS(ITextDocument, ibMetaDocument);

wxCommandProcessor* ITextDocument::OnCreateCommandProcessor()
{
	ibTextCommandProcessor* commandProcessor = new ibTextCommandProcessor(GetTextCtrl());
	commandProcessor->SetEditMenu(mainFrame->GetDefaultMenu(wxID_EDIT));
	commandProcessor->Initialize();
	return commandProcessor;
}

ibTextEditor* ITextDocument::GetTextCtrl() const
{
	wxView* view = GetFirstView();
	return view ? wxDynamicCast(view, ibTextEditView)->GetText() : nullptr;
}

// ----------------------------------------------------------------------------
// ibTextFilibDocument implementation
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(ibTextFilibDocument, ITextDocument);

bool ibTextFilibDocument::OnCreate(const wxString& path, long flags)
{
	if (!ibMetaDocument::OnCreate(path, flags))
		return false;

	return true;
}

// Since text windows have their own method for saving to/loading from files,
// we override DoSave/OpenDocument instead of Save/LoadObject
bool ibTextFilibDocument::DoSaveDocument(const wxString& filename)
{
	return GetTextCtrl()->SaveFile(filename);
}

bool ibTextFilibDocument::DoOpenDocument(const wxString& filename)
{
	if (!GetTextCtrl()->LoadFile(filename))
		return false;

	Modify(false);
	return true;
}