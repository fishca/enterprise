#include "controltextEditor.h"

//text event 
wxDEFINE_EVENT(wxEVT_CONTROL_TEXT_ENTER, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CONTROL_TEXT_INPUT, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CONTROL_TEXT_CLEAR, wxCommandEvent);

//button event
wxDEFINE_EVENT(wxEVT_CONTROL_BUTTON_OPEN, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CONTROL_BUTTON_SELECT, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CONTROL_BUTTON_CLEAR, wxCommandEvent);

wxBEGIN_EVENT_TABLE(wxControlTextEditor, wxWindow)
EVT_SIZE(wxControlTextEditor::OnSize)
EVT_DPI_CHANGED(wxControlTextEditor::OnDPIChanged)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(wxControlTextEditor::wxControlCustomTextEditor, wxTextCtrl)
EVT_TEXT(wxID_ANY, wxControlTextEditor::wxControlCustomTextEditor::OnText)
EVT_TEXT_ENTER(wxID_ANY, wxControlTextEditor::wxControlCustomTextEditor::OnTextEnter)
EVT_TEXT_MAXLEN(wxID_ANY, wxControlTextEditor::wxControlCustomTextEditor::OnText)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(wxControlTextEditor::wxControlCompositeEditor::wxControlEditorButtonCtrl, wxButton)
EVT_LEFT_UP(wxControlTextEditor::wxControlCompositeEditor::wxControlEditorButtonCtrl::OnLeftUp)
wxEND_EVENT_TABLE()

wxIMPLEMENT_DYNAMIC_CLASS(wxControlTextEditor, wxWindow);

// accessors
// ---------

wxString wxControlTextEditor::DoGetValue() const
{
	return m_text->GetValue();
}
wxString wxControlTextEditor::GetRange(long from, long to) const
{
	return m_text->GetRange(from, to);
}

int wxControlTextEditor::GetLineLength(long lineNo) const
{
	return m_text->GetLineLength(lineNo);
}
wxString wxControlTextEditor::GetLineText(long lineNo) const
{
	return m_text->GetLineText(lineNo);
}
int wxControlTextEditor::GetNumberOfLines() const
{
	return m_text->GetNumberOfLines();
}

bool wxControlTextEditor::IsModified() const
{
	return m_text->IsModified();
}
bool wxControlTextEditor::IsEditable() const
{
	return m_text->IsEditable();
}

// more readable flag testing methods
bool wxControlTextEditor::IsSingleLine() const
{
	return m_text->IsSingleLine();
}
bool wxControlTextEditor::IsMultiLine() const
{
	return m_text->IsMultiLine();
}

// If the return values from and to are the same, there is no selection.
void wxControlTextEditor::GetSelection(long* from, long* to) const
{
	m_text->GetSelection(from, to);
}

wxString wxControlTextEditor::GetStringSelection() const
{
	return m_text->GetStringSelection();
}

// operations
// ----------

// editing
void wxControlTextEditor::Clear()
{
	m_text->Clear();
}
void wxControlTextEditor::Replace(long from, long to, const wxString& value)
{
	m_text->Replace(from, to, value);
}
void wxControlTextEditor::Remove(long from, long to)
{
	m_text->Remove(from, to);
}

// load/save the controls contents from/to the file
bool wxControlTextEditor::LoadFile(const wxString& file)
{
	return m_text->LoadFile(file);
}
bool wxControlTextEditor::SaveFile(const wxString& file)
{
	return m_text->SaveFile(file);
}

// sets/clears the dirty flag
void wxControlTextEditor::MarkDirty()
{
	m_text->MarkDirty();
}
void wxControlTextEditor::DiscardEdits()
{
	m_text->DiscardEdits();
}

// set the max number of characters which may be entered in a single line
// text control
void wxControlTextEditor::SetMaxLength(unsigned long len)
{
	m_text->SetMaxLength(len);
}

// writing text inserts it at the current position, appending always
// inserts it at the end
void wxControlTextEditor::WriteText(const wxString& text)
{
	m_text->WriteText(text);
}
void wxControlTextEditor::AppendText(const wxString& text)
{
	m_text->AppendText(text);
}

// insert the character which would have resulted from this key event,
// return true if anything has been inserted
bool wxControlTextEditor::EmulateKeyPress(const wxKeyEvent& event)
{
	return m_text->EmulateKeyPress(event);
}

// text control under some platforms supports the text styles: these
// methods allow to apply the given text style to the given selection or to
// set/get the style which will be used for all appended text
bool wxControlTextEditor::SetStyle(long start, long end, const wxTextAttr& style)
{
	return m_text->SetStyle(start, end, style);
}
bool wxControlTextEditor::GetStyle(long position, wxTextAttr& style)
{
	return m_text->GetStyle(position, style);
}
bool wxControlTextEditor::SetDefaultStyle(const wxTextAttr& style)
{
	return m_text->SetDefaultStyle(style);
}
const wxTextAttr& wxControlTextEditor::GetDefaultStyle() const
{
	return m_text->GetDefaultStyle();
}

// translate between the position (which is just an index in the text ctrl
// considering all its contents as a single strings) and (x, y) coordinates
// which represent column and line.
long wxControlTextEditor::XYToPosition(long x, long y) const
{
	return m_text->XYToPosition(x, y);
}
bool wxControlTextEditor::PositionToXY(long pos, long* x, long* y) const
{
	return m_text->PositionToXY(pos, x, y);
}

void wxControlTextEditor::ShowPosition(long pos)
{
	m_text->ShowPosition(pos);
}

// find the character at position given in pixels
//
// NB: pt is in device coords (not adjusted for the client area origin nor
//     scrolling)
wxTextCtrlHitTestResult wxControlTextEditor::HitTest(const wxPoint& pt, long* pos) const
{
	return m_text->HitTest(pt, pos);
}
wxTextCtrlHitTestResult wxControlTextEditor::HitTest(const wxPoint& pt,
	wxTextCoord* col,
	wxTextCoord* row) const
{
	return m_text->HitTest(pt, col, row);
}

// Clipboard operations
void wxControlTextEditor::Copy()
{
	m_text->Copy();
}
void wxControlTextEditor::Cut()
{
	m_text->Cut();
}
void wxControlTextEditor::Paste()
{
	m_text->Paste();
}

bool wxControlTextEditor::CanCopy() const
{
	return m_text->CanCopy();
}
bool wxControlTextEditor::CanCut() const
{
	return m_text->CanCut();
}
bool wxControlTextEditor::CanPaste() const
{
	return m_text->CanPaste();
}

// Undo/redo
void wxControlTextEditor::Undo()
{
	m_text->Undo();
}
void wxControlTextEditor::Redo()
{
	m_text->Redo();
}

bool wxControlTextEditor::CanUndo() const
{
	return m_text->CanUndo();
}
bool wxControlTextEditor::CanRedo() const
{
	return m_text->CanRedo();
}

// Insertion point
void wxControlTextEditor::SetInsertionPoint(long pos)
{
	m_text->SetInsertionPoint(pos);
}
void wxControlTextEditor::SetInsertionPointEnd()
{
	m_text->SetInsertionPointEnd();
}
long wxControlTextEditor::GetInsertionPoint() const
{
	return m_text->GetInsertionPoint();
}
long wxControlTextEditor::GetLastPosition() const
{
	return m_text->GetLastPosition();
}

void wxControlTextEditor::SetSelection(long from, long to)
{
	m_text->SetSelection(from, to);
}
void wxControlTextEditor::SelectAll()
{
	m_text->SelectAll();
}

void wxControlTextEditor::SetEditable(bool editable)
{
	m_text->SetEditable(editable);
}

// Autocomplete
bool wxControlTextEditor::DoAutoCompleteStrings(const wxArrayString& choices)
{
	return m_text->AutoComplete(choices);
}

bool wxControlTextEditor::DoAutoCompleteFileNames(int flags)
{
	return flags == wxFILE ? m_text->AutoCompleteFileNames() : m_text->AutoCompleteDirectories();
}

bool wxControlTextEditor::DoAutoCompleteCustom(wxTextCompleter* completer)
{
	return m_text->AutoComplete(completer);
}

// Note that overriding DoSetValue() is currently insufficient because the base
// class ChangeValue() only updates m_hintData of this object (which is null
// anyhow), instead of updating m_text->m_hintData, see #16998.
void wxControlTextEditor::ChangeValue(const wxString& value)
{
	m_text->ChangeValue(value);
}

void wxControlTextEditor::DoSetValue(const wxString& value, int flags)
{
	m_text->DoSetValue(value, flags);
}

bool wxControlTextEditor::DoLoadFile(const wxString& file, int fileType)
{
	return m_text->DoLoadFile(file, fileType);
}

bool wxControlTextEditor::DoSaveFile(const wxString& file, int fileType)
{
	return m_text->DoSaveFile(file, fileType);
}

// ----------------------------------------------------------------------------
// geometry
// ----------------------------------------------------------------------------

wxSize wxControlTextEditor::DoGetBestClientSize() const
{
	wxSize size = m_text->GetBestSize();
	wxSize labelSize = m_label->GetBestSize();

	if (size.y < labelSize.y)
		size.y = labelSize.y;

	size.x += labelSize.x;
	return size;
}

wxWindowList wxControlTextEditor::GetCompositeWindowParts() const
{
	wxWindowList parts;
	parts.push_back(m_label);
	parts.push_back(m_text);
	parts.push_back(m_winButton);
	return parts;
}

bool wxControlTextEditor::ShouldInheritColours() const
{
	return true;
}

// ----------------------------------------------------------------------------
// label
// ----------------------------------------------------------------------------

void wxControlTextEditor::CalculateLabelSize(wxCoord* w, wxCoord* h) const
{
	m_label->SetMinSize(wxDefaultSize);
	m_label->GetBestSize(w, h);
}

void wxControlTextEditor::ApplyLabelSize(const wxSize& s)
{
	m_label->SetMinSize(s);
}
