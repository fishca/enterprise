#include "textEditor.h"

//text event 
wxDEFINE_EVENT(wxEVT_CONTROL_TEXT_ENTER, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CONTROL_TEXT_INPUT, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CONTROL_TEXT_CLEAR, wxCommandEvent);

//button event
wxDEFINE_EVENT(wxEVT_CONTROL_BUTTON_OPEN, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CONTROL_BUTTON_SELECT, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CONTROL_BUTTON_CLEAR, wxCommandEvent);

wxBEGIN_EVENT_TABLE(wxControlEditorCtrl, wxWindow)
EVT_SIZE(wxControlEditorCtrl::OnSize)
EVT_DPI_CHANGED(wxControlEditorCtrl::OnDPIChanged)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(wxControlEditorCtrl::wxControlTextEditorCtrl, wxTextCtrl)
EVT_TEXT(wxID_ANY, wxControlEditorCtrl::wxControlTextEditorCtrl::OnText)
EVT_TEXT_ENTER(wxID_ANY, wxControlEditorCtrl::wxControlTextEditorCtrl::OnTextEnter)
EVT_TEXT_MAXLEN(wxID_ANY, wxControlEditorCtrl::wxControlTextEditorCtrl::OnText)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(wxControlEditorCtrl::wxControlCompositeEditorCtrl::wxControlEditorButtonCtrl, wxButton)
EVT_LEFT_UP(wxControlEditorCtrl::wxControlCompositeEditorCtrl::wxControlEditorButtonCtrl::OnLeftUp)
wxEND_EVENT_TABLE()

wxIMPLEMENT_DYNAMIC_CLASS(wxControlEditorCtrl, wxWindow);

// accessors
// ---------

wxString wxControlEditorCtrl::DoGetValue() const
{
	return m_text->GetValue();
}
wxString wxControlEditorCtrl::GetRange(long from, long to) const
{
	return m_text->GetRange(from, to);
}

int wxControlEditorCtrl::GetLineLength(long lineNo) const
{
	return m_text->GetLineLength(lineNo);
}
wxString wxControlEditorCtrl::GetLineText(long lineNo) const
{
	return m_text->GetLineText(lineNo);
}
int wxControlEditorCtrl::GetNumberOfLines() const
{
	return m_text->GetNumberOfLines();
}

bool wxControlEditorCtrl::IsModified() const
{
	return m_text->IsModified();
}
bool wxControlEditorCtrl::IsEditable() const
{
	return m_text->IsEditable();
}

// more readable flag testing methods
bool wxControlEditorCtrl::IsSingleLine() const
{
	return m_text->IsSingleLine();
}
bool wxControlEditorCtrl::IsMultiLine() const
{
	return m_text->IsMultiLine();
}

// If the return values from and to are the same, there is no selection.
void wxControlEditorCtrl::GetSelection(long* from, long* to) const
{
	m_text->GetSelection(from, to);
}

wxString wxControlEditorCtrl::GetStringSelection() const
{
	return m_text->GetStringSelection();
}

// operations
// ----------

// editing
void wxControlEditorCtrl::Clear()
{
	m_text->Clear();
}
void wxControlEditorCtrl::Replace(long from, long to, const wxString& value)
{
	m_text->Replace(from, to, value);
}
void wxControlEditorCtrl::Remove(long from, long to)
{
	m_text->Remove(from, to);
}

// load/save the controls contents from/to the file
bool wxControlEditorCtrl::LoadFile(const wxString& file)
{
	return m_text->LoadFile(file);
}
bool wxControlEditorCtrl::SaveFile(const wxString& file)
{
	return m_text->SaveFile(file);
}

// sets/clears the dirty flag
void wxControlEditorCtrl::MarkDirty()
{
	m_text->MarkDirty();
}
void wxControlEditorCtrl::DiscardEdits()
{
	m_text->DiscardEdits();
}

// set the max number of characters which may be entered in a single line
// text control
void wxControlEditorCtrl::SetMaxLength(unsigned long len)
{
	m_text->SetMaxLength(len);
}

// writing text inserts it at the current position, appending always
// inserts it at the end
void wxControlEditorCtrl::WriteText(const wxString& text)
{
	m_text->WriteText(text);
}
void wxControlEditorCtrl::AppendText(const wxString& text)
{
	m_text->AppendText(text);
}

// insert the character which would have resulted from this key event,
// return true if anything has been inserted
bool wxControlEditorCtrl::EmulateKeyPress(const wxKeyEvent& event)
{
	return m_text->EmulateKeyPress(event);
}

// text control under some platforms supports the text styles: these
// methods allow to apply the given text style to the given selection or to
// set/get the style which will be used for all appended text
bool wxControlEditorCtrl::SetStyle(long start, long end, const wxTextAttr& style)
{
	return m_text->SetStyle(start, end, style);
}
bool wxControlEditorCtrl::GetStyle(long position, wxTextAttr& style)
{
	return m_text->GetStyle(position, style);
}
bool wxControlEditorCtrl::SetDefaultStyle(const wxTextAttr& style)
{
	return m_text->SetDefaultStyle(style);
}
const wxTextAttr& wxControlEditorCtrl::GetDefaultStyle() const
{
	return m_text->GetDefaultStyle();
}

// translate between the position (which is just an index in the text ctrl
// considering all its contents as a single strings) and (x, y) coordinates
// which represent column and line.
long wxControlEditorCtrl::XYToPosition(long x, long y) const
{
	return m_text->XYToPosition(x, y);
}
bool wxControlEditorCtrl::PositionToXY(long pos, long* x, long* y) const
{
	return m_text->PositionToXY(pos, x, y);
}

void wxControlEditorCtrl::ShowPosition(long pos)
{
	m_text->ShowPosition(pos);
}

// find the character at position given in pixels
//
// NB: pt is in device coords (not adjusted for the client area origin nor
//     scrolling)
wxTextCtrlHitTestResult wxControlEditorCtrl::HitTest(const wxPoint& pt, long* pos) const
{
	return m_text->HitTest(pt, pos);
}
wxTextCtrlHitTestResult wxControlEditorCtrl::HitTest(const wxPoint& pt,
	wxTextCoord* col,
	wxTextCoord* row) const
{
	return m_text->HitTest(pt, col, row);
}

// Clipboard operations
void wxControlEditorCtrl::Copy()
{
	m_text->Copy();
}
void wxControlEditorCtrl::Cut()
{
	m_text->Cut();
}
void wxControlEditorCtrl::Paste()
{
	m_text->Paste();
}

bool wxControlEditorCtrl::CanCopy() const
{
	return m_text->CanCopy();
}
bool wxControlEditorCtrl::CanCut() const
{
	return m_text->CanCut();
}
bool wxControlEditorCtrl::CanPaste() const
{
	return m_text->CanPaste();
}

// Undo/redo
void wxControlEditorCtrl::Undo()
{
	m_text->Undo();
}
void wxControlEditorCtrl::Redo()
{
	m_text->Redo();
}

bool wxControlEditorCtrl::CanUndo() const
{
	return m_text->CanUndo();
}
bool wxControlEditorCtrl::CanRedo() const
{
	return m_text->CanRedo();
}

// Insertion point
void wxControlEditorCtrl::SetInsertionPoint(long pos)
{
	m_text->SetInsertionPoint(pos);
}
void wxControlEditorCtrl::SetInsertionPointEnd()
{
	m_text->SetInsertionPointEnd();
}
long wxControlEditorCtrl::GetInsertionPoint() const
{
	return m_text->GetInsertionPoint();
}
long wxControlEditorCtrl::GetLastPosition() const
{
	return m_text->GetLastPosition();
}

void wxControlEditorCtrl::SetSelection(long from, long to)
{
	m_text->SetSelection(from, to);
}
void wxControlEditorCtrl::SelectAll()
{
	m_text->SelectAll();
}

void wxControlEditorCtrl::SetEditable(bool editable)
{
	m_text->SetEditable(editable);
}

// Autocomplete
bool wxControlEditorCtrl::DoAutoCompleteStrings(const wxArrayString& choices)
{
	return m_text->AutoComplete(choices);
}

bool wxControlEditorCtrl::DoAutoCompleteFileNames(int flags)
{
	return flags == wxFILE ? m_text->AutoCompleteFileNames() : m_text->AutoCompleteDirectories();
}

bool wxControlEditorCtrl::DoAutoCompleteCustom(wxTextCompleter* completer)
{
	return m_text->AutoComplete(completer);
}

// Note that overriding DoSetValue() is currently insufficient because the base
// class ChangeValue() only updates m_hintData of this object (which is null
// anyhow), instead of updating m_text->m_hintData, see #16998.
void wxControlEditorCtrl::ChangeValue(const wxString& value)
{
	m_text->ChangeValue(value);
}

void wxControlEditorCtrl::DoSetValue(const wxString& value, int flags)
{
	m_text->DoSetValue(value, flags);
}

bool wxControlEditorCtrl::DoLoadFile(const wxString& file, int fileType)
{
	return m_text->DoLoadFile(file, fileType);
}

bool wxControlEditorCtrl::DoSaveFile(const wxString& file, int fileType)
{
	return m_text->DoSaveFile(file, fileType);
}

// ----------------------------------------------------------------------------
// geometry
// ----------------------------------------------------------------------------

wxSize wxControlEditorCtrl::DoGetBestClientSize() const
{
	wxSize size = m_text->GetBestSize();
	wxSize labelSize = m_label->GetBestSize();

	if (size.y < labelSize.y)
		size.y = labelSize.y;

	size.x += labelSize.x;
	return size;
}

wxWindowList wxControlEditorCtrl::GetCompositeWindowParts() const
{
	wxWindowList parts;
	parts.push_back(m_label);
	parts.push_back(m_text);
	parts.push_back(m_winButton);
	return parts;
}

bool wxControlEditorCtrl::ShouldInheritColours() const
{
	return true;
}