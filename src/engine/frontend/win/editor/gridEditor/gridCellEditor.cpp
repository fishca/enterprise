#include "gridEditor.h"

#if wxUSE_TEXTCTRL

// ----------------------------------------------------------------------------
// CGridEditor::CGridEditorCellTextEditor
// ----------------------------------------------------------------------------

CGridEditor::CGridEditorCellTextEditor::CGridEditorCellTextEditor(const CGridEditorCellTextEditor& other)
	: wxGridExtCellEditor(other),
	m_maxChars(other.m_maxChars),
	m_value(other.m_value)
{
#if wxUSE_VALIDATORS
	if (other.m_validator)
	{
		SetValidator(*other.m_validator);
	}
#endif
}

void CGridEditor::CGridEditorCellTextEditor::Create(wxWindow* parent,
	wxWindowID id,
	wxEvtHandler* evtHandler)
{
	DoCreate(parent, id, evtHandler);
}

void CGridEditor::CGridEditorCellTextEditor::DoCreate(wxWindow* parent,
	wxWindowID id,
	wxEvtHandler* evtHandler,
	long style)
{
	wxTextCtrl* const text = new wxTextCtrl(parent, id, wxEmptyString,
		wxDefaultPosition, wxDefaultSize,
		style | wxTE_MULTILINE | wxTE_NO_VSCROLL | wxTE_PROCESS_ENTER | wxTE_PROCESS_TAB | wxNO_BORDER);

	text->SetMargins(0, 0);

	// set max length allowed in the textctrl, if the parameter was set
	if (m_maxChars != 0)
	{
		Text()->SetMaxLength(m_maxChars);
	}

	m_control = text;

	evtHandler->Bind(wxEVT_KEY_DOWN,
		[text](wxKeyEvent& e) {

			static wxMemoryDC dc;

			static int intStringX, intStringY, intLineY;
			static int intSizeX, intSizeY;

			const int insertion = text->GetInsertionPoint();

			switch (e.GetKeyCode())
			{
			case WXK_RETURN:
			case WXK_NUMPAD_ENTER:
				if (e.ShiftDown()) {

					wxString strValue = text->GetValue();
					if (insertion < (int)strValue.size())
						strValue.insert(insertion, wxT('\n'));
					else
						strValue.append(wxT('\n'));

					dc.SetFont(text->GetFont());
					dc.GetMultiLineTextExtent(strValue, &intStringX, &intStringY, &intLineY);

					text->GetSize(&intSizeX, &intSizeY);
					text->SetSize(intSizeX, intStringY > intSizeY ? intStringY : intSizeY);

					text->WriteText(wxT('\n'));
				}
				else e.Skip();
				break;
			default:

				wxString strValue = text->GetValue();
				if (insertion < (int)strValue.size())
					strValue.insert(insertion, static_cast<wchar_t>(e.GetRawKeyCode()));
				else
					strValue.append(static_cast<wchar_t>(e.GetRawKeyCode()));

				dc.SetFont(text->GetFont());
				dc.GetMultiLineTextExtent(strValue, &intStringX, &intStringY, &intLineY);

				text->GetSize(&intSizeX, &intSizeY);
				text->SetSize(intStringX > (intSizeX - 6) ? intStringX + 6 : intSizeX, intSizeY);

				e.Skip();
				break;
			}
		}
	);

#if wxUSE_VALIDATORS
	// validate text in textctrl, if validator is set
	if (m_validator)
	{
		Text()->SetValidator(*m_validator);
	}
#endif

	wxGridExtCellEditor::Create(parent, id, evtHandler);
}

void CGridEditor::CGridEditorCellTextEditor::SetSize(const wxRect& rectOrig)
{
	wxRect rect(rectOrig);

	// Make the edit control large enough to allow for internal margins
	//
	// TODO: remove this if the text ctrl sizing is improved
	//
#if defined(__WXMSW__)

	rect.x--;
	rect.y--;

	const wxString& strValue = Text()->GetValue();

	if (!strValue.IsEmpty()) {

		static wxMemoryDC dc;

		dc.SetFont(m_control->GetFont());

		wxCoord x = 0, y = 0;
		dc.GetMultiLineTextExtent(strValue, &x, &y);

		rect.width = x + 1;
		rect.height = y + 1;
	}

#elif !defined(__WXGTK__)
	int extra_x = 2;
	int extra_y = 2;

#if defined(__WXMOTIF__)
	extra_x *= 2;
	extra_y *= 2;
#endif

	rect.SetLeft(wxMax(0, rect.x - extra_x));
	rect.SetTop(wxMax(0, rect.y - extra_y));
	rect.SetRight(rect.GetRight() + 2 * extra_x);
	rect.SetBottom(rect.GetBottom() + 2 * extra_y);
#endif

	wxGridExtCellEditor::SetSize(rect);
}

void CGridEditor::CGridEditorCellTextEditor::BeginEdit(int row, int col, wxGridExt* grid)
{
	wxASSERT_MSG(m_control, wxT("The wxGridExtCellEditor must be created first!"));

	m_value = grid->GetTable()->GetValue(row, col);

	DoBeginEdit(m_value);
}

void CGridEditor::CGridEditorCellTextEditor::DoBeginEdit(const wxString& startValue)
{
	Text()->SetValue(startValue);
	Text()->SetInsertionPointEnd();
	Text()->SelectAll();
	Text()->SetFocus();
}

bool CGridEditor::CGridEditorCellTextEditor::EndEdit(int WXUNUSED(row),
	int WXUNUSED(col),
	const wxGridExt* WXUNUSED(grid),
	const wxString& WXUNUSED(oldval),
	wxString* newval)
{
	wxCHECK_MSG(m_control, false,
		"CGridEditor::CGridEditorCellTextEditor must be created first!");

	const wxString value = Text()->GetValue();
	if (value == m_value)
		return false;

	m_value = value;

	if (newval)
		*newval = m_value;

	return true;
}

void CGridEditor::CGridEditorCellTextEditor::ApplyEdit(int row, int col, wxGridExt* grid)
{
	const wxString& strValue = Text()->GetValue();

	if (!strValue.IsEmpty()) {

		static wxMemoryDC dc;
		dc.SetFont(m_control->GetFont());

		wxCoord x = 0, y = 0;
		dc.GetMultiLineTextExtent(strValue, &x, &y);

		grid->SetRowSize(row, y);
	}

	grid->GetTable()->SetValue(row, col, m_value);

	m_value.clear();
}

void CGridEditor::CGridEditorCellTextEditor::Reset()
{
	wxASSERT_MSG(m_control, "CGridEditor::CGridEditorCellTextEditor must be created first!");

	DoReset(m_value);
}

void CGridEditor::CGridEditorCellTextEditor::DoReset(const wxString& startValue)
{
	Text()->SetValue(startValue);
	Text()->SetInsertionPointEnd();
}

bool CGridEditor::CGridEditorCellTextEditor::IsAcceptedKey(wxKeyEvent& event)
{
	switch (event.GetKeyCode())
	{
	case WXK_BACK:
		return true;

	default:
		return wxGridExtCellEditor::IsAcceptedKey(event);
	}
}

void CGridEditor::CGridEditorCellTextEditor::StartingKey(wxKeyEvent& event)
{
	// Since this is now happening in the EVT_CHAR event EmulateKeyPress is no
	// longer an appropriate way to get the character into the text control.
	// Do it ourselves instead.  We know that if we get this far that we have
	// a valid character, so not a whole lot of testing needs to be done.

	wxTextCtrl* tc = Text();
	int ch;

	bool isPrintable;

#if wxUSE_UNICODE
	ch = event.GetUnicodeKey();
	if (ch != WXK_NONE)
		isPrintable = true;
	else
#endif // wxUSE_UNICODE
	{
		ch = event.GetKeyCode();
		isPrintable = ch >= WXK_SPACE && ch < WXK_START;
	}

	switch (ch)
	{
	case WXK_DELETE:
		// Delete the initial character when starting to edit with DELETE.
		tc->Remove(0, 1);
		break;

	case WXK_BACK:
		// Delete the last character when starting to edit with BACKSPACE.
	{
		const long pos = tc->GetLastPosition();
		tc->Remove(pos - 1, pos);
	}
	break;

	default:
		if (isPrintable)
			tc->WriteText(static_cast<wxChar>(ch));
		break;
	}
}

void CGridEditor::CGridEditorCellTextEditor::HandleReturn(wxKeyEvent& event)
{
#if defined(__WXMOTIF__) || defined(__WXGTK__)
	// wxMotif needs a little extra help...
	size_t pos = (size_t)(Text()->GetInsertionPoint());
	wxString s(Text()->GetValue());
	s = s.Left(pos) + wxT("\n") + s.Mid(pos);
	Text()->SetValue(s);
	Text()->SetInsertionPoint(pos);
#else
	// the other ports can handle a Return key press
	//
	event.Skip();
#endif
}

void CGridEditor::CGridEditorCellTextEditor::SetParameters(const wxString& params)
{
	if (!params)
	{
		// reset to default
		m_maxChars = 0;
	}
	else
	{
		long tmp;
		if (params.ToLong(&tmp))
		{
			m_maxChars = (size_t)tmp;
		}
		else
		{
			wxLogDebug(wxT("Invalid CGridEditor::CGridEditorCellTextEditor parameter string '%s' ignored"), params);
		}
	}
}

#if wxUSE_VALIDATORS
void CGridEditor::CGridEditorCellTextEditor::SetValidator(const wxValidator& validator)
{
	m_validator.reset(static_cast<wxValidator*>(validator.Clone()));
	if (m_validator && IsCreated())
		Text()->SetValidator(*m_validator);
}
#endif

// return the value in the text control
wxString CGridEditor::CGridEditorCellTextEditor::GetValue() const
{
	return Text()->GetValue();
}

#endif // wxUSE_TEXTCTRL
