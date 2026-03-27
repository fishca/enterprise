#include "controltextEditor.h"

//text event 
wxDEFINE_EVENT(wxEVT_CONTROL_TEXT_ENTER, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CONTROL_TEXT_INPUT, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CONTROL_TEXT_CLEAR, wxCommandEvent);

//button event
wxDEFINE_EVENT(wxEVT_CONTROL_BUTTON_OPEN, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CONTROL_BUTTON_SELECT, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CONTROL_BUTTON_CLEAR, wxCommandEvent);

wxBEGIN_EVENT_TABLE(ibControlTextEditor, wxWindow)
EVT_SIZE(ibControlTextEditor::OnSize)
EVT_DPI_CHANGED(ibControlTextEditor::OnDPIChanged)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(ibControlTextEditor::ibControlCustomTextEditor, wxTextCtrl)
EVT_TEXT(wxID_ANY, ibControlTextEditor::ibControlCustomTextEditor::OnText)
EVT_TEXT_ENTER(wxID_ANY, ibControlTextEditor::ibControlCustomTextEditor::OnTextEnter)
EVT_TEXT_MAXLEN(wxID_ANY, ibControlTextEditor::ibControlCustomTextEditor::OnText)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(ibControlTextEditor::ibControlCompositeEditor::ibControlEditorButtonCtrl, wxButton)
EVT_LEFT_UP(ibControlTextEditor::ibControlCompositeEditor::ibControlEditorButtonCtrl::OnLeftUp)
wxEND_EVENT_TABLE()

wxIMPLEMENT_DYNAMIC_CLASS(ibControlTextEditor, wxWindow);

// accessors
// ---------

wxString ibControlTextEditor::DoGetValue() const
{
	return m_text->GetValue();
}
wxString ibControlTextEditor::GetRange(long from, long to) const
{
	return m_text->GetRange(from, to);
}

int ibControlTextEditor::GetLineLength(long lineNo) const
{
	return m_text->GetLineLength(lineNo);
}
wxString ibControlTextEditor::GetLineText(long lineNo) const
{
	return m_text->GetLineText(lineNo);
}
int ibControlTextEditor::GetNumberOfLines() const
{
	return m_text->GetNumberOfLines();
}

bool ibControlTextEditor::IsModified() const
{
	return m_text->IsModified();
}
bool ibControlTextEditor::IsEditable() const
{
	return m_text->IsEditable();
}

// more readable flag testing methods
bool ibControlTextEditor::IsSingleLine() const
{
	return m_text->IsSingleLine();
}
bool ibControlTextEditor::IsMultiLine() const
{
	return m_text->IsMultiLine();
}

// If the return values from and to are the same, there is no selection.
void ibControlTextEditor::GetSelection(long* from, long* to) const
{
	m_text->GetSelection(from, to);
}

wxString ibControlTextEditor::GetStringSelection() const
{
	return m_text->GetStringSelection();
}

// operations
// ----------

// editing
void ibControlTextEditor::Clear()
{
	m_text->Clear();
}
void ibControlTextEditor::Replace(long from, long to, const wxString& value)
{
	m_text->Replace(from, to, value);
}
void ibControlTextEditor::Remove(long from, long to)
{
	m_text->Remove(from, to);
}

// load/save the controls contents from/to the file
bool ibControlTextEditor::LoadFile(const wxString& file)
{
	return m_text->LoadFile(file);
}
bool ibControlTextEditor::SaveFile(const wxString& file)
{
	return m_text->SaveFile(file);
}

// sets/clears the dirty flag
void ibControlTextEditor::MarkDirty()
{
	m_text->MarkDirty();
}
void ibControlTextEditor::DiscardEdits()
{
	m_text->DiscardEdits();
}

// set the max number of characters which may be entered in a single line
// text control
void ibControlTextEditor::SetMaxLength(unsigned long len)
{
	m_text->SetMaxLength(len);
}

// writing text inserts it at the current position, appending always
// inserts it at the end
void ibControlTextEditor::WriteText(const wxString& text)
{
	m_text->WriteText(text);
}
void ibControlTextEditor::AppendText(const wxString& text)
{
	m_text->AppendText(text);
}

// insert the character which would have resulted from this key event,
// return true if anything has been inserted
bool ibControlTextEditor::EmulateKeyPress(const wxKeyEvent& event)
{
	return m_text->EmulateKeyPress(event);
}

// text control under some platforms supports the text styles: these
// methods allow to apply the given text style to the given selection or to
// set/get the style which will be used for all appended text
bool ibControlTextEditor::SetStyle(long start, long end, const wxTextAttr& style)
{
	return m_text->SetStyle(start, end, style);
}
bool ibControlTextEditor::GetStyle(long position, wxTextAttr& style)
{
	return m_text->GetStyle(position, style);
}
bool ibControlTextEditor::SetDefaultStyle(const wxTextAttr& style)
{
	return m_text->SetDefaultStyle(style);
}
const wxTextAttr& ibControlTextEditor::GetDefaultStyle() const
{
	return m_text->GetDefaultStyle();
}

// translate between the position (which is just an index in the text ctrl
// considering all its contents as a single strings) and (x, y) coordinates
// which represent column and line.
long ibControlTextEditor::XYToPosition(long x, long y) const
{
	return m_text->XYToPosition(x, y);
}
bool ibControlTextEditor::PositionToXY(long pos, long* x, long* y) const
{
	return m_text->PositionToXY(pos, x, y);
}

void ibControlTextEditor::ShowPosition(long pos)
{
	m_text->ShowPosition(pos);
}

// find the character at position given in pixels
//
// NB: pt is in device coords (not adjusted for the client area origin nor
//     scrolling)
wxTextCtrlHitTestResult ibControlTextEditor::HitTest(const wxPoint& pt, long* pos) const
{
	return m_text->HitTest(pt, pos);
}
wxTextCtrlHitTestResult ibControlTextEditor::HitTest(const wxPoint& pt,
	wxTextCoord* col,
	wxTextCoord* row) const
{
	return m_text->HitTest(pt, col, row);
}

// Clipboard operations
void ibControlTextEditor::Copy()
{
	m_text->Copy();
}
void ibControlTextEditor::Cut()
{
	m_text->Cut();
}
void ibControlTextEditor::Paste()
{
	m_text->Paste();
}

bool ibControlTextEditor::CanCopy() const
{
	return m_text->CanCopy();
}
bool ibControlTextEditor::CanCut() const
{
	return m_text->CanCut();
}
bool ibControlTextEditor::CanPaste() const
{
	return m_text->CanPaste();
}

// Undo/redo
void ibControlTextEditor::Undo()
{
	m_text->Undo();
}
void ibControlTextEditor::Redo()
{
	m_text->Redo();
}

bool ibControlTextEditor::CanUndo() const
{
	return m_text->CanUndo();
}
bool ibControlTextEditor::CanRedo() const
{
	return m_text->CanRedo();
}

// Insertion point
void ibControlTextEditor::SetInsertionPoint(long pos)
{
	m_text->SetInsertionPoint(pos);
}
void ibControlTextEditor::SetInsertionPointEnd()
{
	m_text->SetInsertionPointEnd();
}
long ibControlTextEditor::GetInsertionPoint() const
{
	return m_text->GetInsertionPoint();
}
long ibControlTextEditor::GetLastPosition() const
{
	return m_text->GetLastPosition();
}

void ibControlTextEditor::SetSelection(long from, long to)
{
	m_text->SetSelection(from, to);
}
void ibControlTextEditor::SelectAll()
{
	m_text->SelectAll();
}

void ibControlTextEditor::SetEditable(bool editable)
{
	m_text->SetEditable(editable);
}

// Autocomplete
bool ibControlTextEditor::DoAutoCompleteStrings(const wxArrayString& choices)
{
	return m_text->AutoComplete(choices);
}

bool ibControlTextEditor::DoAutoCompleteFileNames(int flags)
{
	return flags == wxFILE ? m_text->AutoCompleteFileNames() : m_text->AutoCompleteDirectories();
}

bool ibControlTextEditor::DoAutoCompleteCustom(wxTextCompleter* completer)
{
	return m_text->AutoComplete(completer);
}

// Note that overriding DoSetValue() is currently insufficient because the base
// class ChangeValue() only updates m_hintData of this object (which is null
// anyhow), instead of updating m_text->m_hintData, see #16998.
void ibControlTextEditor::ChangeValue(const wxString& value)
{
	m_text->ChangeValue(value);
}

void ibControlTextEditor::DoSetValue(const wxString& value, int flags)
{
	m_text->DoSetValue(value, flags);
}

bool ibControlTextEditor::DoLoadFile(const wxString& file, int fileType)
{
	return m_text->DoLoadFile(file, fileType);
}

bool ibControlTextEditor::DoSaveFile(const wxString& file, int fileType)
{
	return m_text->DoSaveFile(file, fileType);
}

// ----------------------------------------------------------------------------
// geometry
// ----------------------------------------------------------------------------

wxSize ibControlTextEditor::DoGetBestClientSize() const
{
	wxSize size = m_text->GetBestSize();
	wxSize labelSize = m_label->GetBestSize();

	if (size.y < labelSize.y)
		size.y = labelSize.y;

	size.x += labelSize.x;
	return size;
}

wxWindowList ibControlTextEditor::GetCompositeWindowParts() const
{
	wxWindowList parts;
	parts.push_back(m_label);
	parts.push_back(m_text);
	parts.push_back(m_winButton);
	return parts;
}

bool ibControlTextEditor::ShouldInheritColours() const
{
	return true;
}

// ----------------------------------------------------------------------------
// label
// ----------------------------------------------------------------------------

void ibControlTextEditor::CalculateLabelSize(wxCoord* w, wxCoord* h) const
{
	m_label->SetMinSize(wxDefaultSize);
	m_label->GetBestSize(w, h);
}

void ibControlTextEditor::ApplyLabelSize(const wxSize& s)
{
	m_label->SetMinSize(s);
}
