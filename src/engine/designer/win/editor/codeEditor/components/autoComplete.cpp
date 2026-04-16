#include "autoComplete.h"
#include "frontend/mainFrame/mainFrame.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>

ibAutoComplete::ibAutoComplete(wxStyledTextCtrl* textCtrl) :
	active(false),
	lb(nullptr),
	m_visualData(new ibListBoxVisualData(5)),
	m_evtHandler(nullptr),
	m_owner(textCtrl),
	posStart(0),
	startLen(0),
	cancelAtStartPos(true),
	dropRestOfWord(false)
{
}

ibAutoComplete::~ibAutoComplete()
{
	if (lb)
	{
		lb->GetListBox()->Destroy();
		lb->Destroy();
	}

	delete m_visualData;
}

bool ibAutoComplete::Active() const
{
	return active;
}

void ibAutoComplete::Start(const wxString& strCurWord,
	int position, int startLen_,
	int lineHeight)
{
	if (active) Cancel();

	lb = new ibOESListBoxWin(m_owner, m_visualData, m_owner->TextHeight(m_owner->GetCurrentLine()));
	lb->SetSize(225, 200);

	active = true;
	startLen = startLen_;
	posStart = position;

	strCurrentWord = strCurWord;

	ibListBox* m_listBox = lb->GetListBox();
	m_listBox->SetListBoxFont(m_owner->StyleGetFont(wxSTC_STYLE_DEFAULT));

	m_listBox->Bind(wxEVT_LISTBOX, &ibAutoComplete::OnSelection, this);
	m_listBox->Bind(wxEVT_LISTBOX_DCLICK, &ibAutoComplete::OnSelection, this);

	m_listBox->Bind(wxEVT_MOTION, &ibAutoComplete::OnMouseMotion, this);

	m_evtHandler = new wxEvtHandler;
	m_evtHandler->Bind(wxEVT_KEY_DOWN, &ibAutoComplete::OnKeyDown, this);

	//focus kill/set
	m_evtHandler->Bind(wxEVT_SET_FOCUS, &ibAutoComplete::OnProcessFocus, this);
	m_evtHandler->Bind(wxEVT_KILL_FOCUS, &ibAutoComplete::OnProcessFocus, this);
	m_evtHandler->Bind(wxEVT_CHILD_FOCUS, &ibAutoComplete::OnProcessChildFocus, this);

	//on sizing 
	m_evtHandler->Bind(wxEVT_SIZE, &ibAutoComplete::OnProcessSize, this);
	m_evtHandler->Bind(wxEVT_SIZING, &ibAutoComplete::OnProcessSize, this);

	//on mouse event
	m_evtHandler->Bind(wxEVT_LEFT_DCLICK, &ibAutoComplete::OnProcessMouse, this);
	m_evtHandler->Bind(wxEVT_RIGHT_DCLICK, &ibAutoComplete::OnProcessMouse, this);
	m_evtHandler->Bind(wxEVT_MIDDLE_DCLICK, &ibAutoComplete::OnProcessMouse, this);

	m_evtHandler->Bind(wxEVT_LEFT_UP, &ibAutoComplete::OnProcessMouse, this);
	m_evtHandler->Bind(wxEVT_RIGHT_UP, &ibAutoComplete::OnProcessMouse, this);
	m_evtHandler->Bind(wxEVT_MIDDLE_UP, &ibAutoComplete::OnProcessMouse, this);
}

void ibAutoComplete::Append(short type, const wxString& strName, const wxString& desc)
{
	if (strName.IsEmpty())
		return;

	if (!strCurrentWord.IsEmpty()) {
		const wxString &nameUpper = stringUtils::MakeUpper(strName);
		if (nameUpper.Find(stringUtils::MakeUpper(strCurrentWord.Upper())) < 0)
			return;
	}

	const wxBitmap* bitmap = m_visualData->GetImage(type);
	if (bitmap == nullptr)
		m_visualData->RegisterImage(type, GetImageByType(type));

	m_aKeywords.push_back(
		{ type, strName, desc }
	);
}

int ibAutoComplete::GetSelection() const
{
	return lb->GetListBox()->GetSelection();
}

wxString ibAutoComplete::GetValue(int item) const
{
	return lb->GetListBox()->GetValue(item);
}

void ibAutoComplete::Show(const wxPoint& position)
{
	if (!active) return;
	if (!m_aKeywords.size()) {
		Cancel(); return;
	}

	ibListBox* m_listBox = lb->GetListBox();

	std::sort(m_aKeywords.begin(), m_aKeywords.end(),
		[](ibKeywordElement a, ibKeywordElement b) {
			return a.m_name.CompareTo(b.m_name, wxString::caseCompare::ignoreCase) < 0;
		}
	);

	lb->SetPosition(position);

	for (auto keyword : m_aKeywords)
		m_listBox->Append(keyword.m_name, keyword.m_type);

	if (m_aKeywords.size() == 1) Select(0);
	else if (lb->Show()) m_listBox->Select(0);
}

void ibAutoComplete::Cancel()
{
	active = false;
	strCurrentWord = wxEmptyString;
	m_aKeywords.clear();

	if (lb) {
		lb->GetListBox()->Clear();
		lb->GetListBox()->Destroy();
		wxDELETE(lb);
	}
	wxDELETE(m_evtHandler);
}

void ibAutoComplete::MoveUp()
{
	int count = lb->GetListBox()->Length();
	int current = lb->GetListBox()->GetSelection();
	current -= 1;

	if (current >= count)
		current = count - 1;

	if (current < 0)
		current = 0;

	lb->GetListBox()->Select(current);
}

void ibAutoComplete::MoveDown()
{
	int count = lb->GetListBox()->Length();
	int current = lb->GetListBox()->GetSelection();
	current += 1;

	if (current >= count)
		current = count - 1;

	if (current < 0)
		current = 0;

	lb->GetListBox()->Select(current);
}

#include "../codeEditor.h"

void ibAutoComplete::Select(int index)
{
	wxString sDescription; bool m_bNeedCallTip = false;

	std::vector< ibKeywordElement>::iterator m_selectedKeyword = m_aKeywords.begin() + index;

	if (m_selectedKeyword != m_aKeywords.end())
	{
		wxString sTextComplete = m_selectedKeyword->m_name;
		wxString strShortDescription = m_selectedKeyword->m_shortDescription;

		if (m_selectedKeyword->m_type != eVariable && m_selectedKeyword->m_type != eExportVariable)
		{
			if (strShortDescription.IsEmpty()) {
				sTextComplete += "()";
			}
			else {
				sTextComplete += "(";
				if (strShortDescription.Find("()") > 0) sTextComplete += ")";
				else m_bNeedCallTip = true;
			}
		}

		m_owner->Replace(posStart - startLen, posStart, sTextComplete);
		m_owner->SetEmptySelection(posStart - startLen + sTextComplete.ToUTF8().length());

		sDescription = strShortDescription;
	}

	Cancel();

	if (m_bNeedCallTip)
	{
		ibCodeEditor* m_autoComplete = dynamic_cast<ibCodeEditor*>(m_owner);
		if (m_autoComplete) m_autoComplete->ShowCallTip(sDescription);
	}
}

bool ibAutoComplete::CallEvent(wxEvent& event)
{
	if (!active) return false;
	bool result = m_evtHandler->ProcessEvent(event);
	if (m_evtHandler) return result;

	if (event.GetEventType() == wxEVT_KEY_DOWN)
	{
		return ((wxKeyEvent&)event).GetKeyCode() == WXK_RETURN ||
			((wxKeyEvent&)event).GetKeyCode() == WXK_NUMPAD_ENTER;
	}

	return false;
}

#include "frontend/artProvider/artProvider.h"

wxBitmap ibAutoComplete::GetImageByType(short type) const
{
	if (type == eProcedure || type == eExportProcedure)
		return wxArtProvider::GetBitmapBundle(wxART_PROCEDURE_RED, wxART_AUTOCOMPLETE);
	else if (type == eFunction || type == eExportFunction)
		return wxArtProvider::GetBitmapBundle(wxART_FUNCTION_RED, wxART_AUTOCOMPLETE);
	else if (type == eVariable || type == eExportVariable)
		return wxArtProvider::GetBitmapBundle(wxART_VARIABLE_ALTERNATIVE, wxART_AUTOCOMPLETE);
	return wxNullBitmap;
}

void ibAutoComplete::OnSelection(wxCommandEvent& event)
{
	Select(event.GetSelection());
}

void ibAutoComplete::OnKeyDown(wxKeyEvent& event)
{
	ibListBox* m_listBox = lb->GetListBox();

	switch (event.GetKeyCode())
	{
	case WXK_UP: MoveUp(); break;
	case WXK_DOWN: MoveDown(); break;
	case WXK_NUMPAD_ENTER:
	case WXK_RETURN:
		Select(m_listBox->GetSelection());
		break;
	default: Cancel(); break;
	}
}

void ibAutoComplete::OnMouseMotion(wxMouseEvent& event)
{
	ibListBox* listBox = lb->GetListBox();
	int currentRow = listBox->VirtualHitTest(event.GetY());
	if (currentRow != wxNOT_FOUND) {
		std::vector< ibKeywordElement>::iterator selectedKeyword = m_aKeywords.begin() + currentRow;

		if (selectedKeyword->m_shortDescription.IsEmpty())
			listBox->SetToolTip(nullptr);
		else
			listBox->SetToolTip(selectedKeyword->m_shortDescription);
	}

	event.Skip();
}

void ibAutoComplete::OnProcessFocus(wxFocusEvent& event)
{
	Cancel();
}

void ibAutoComplete::OnProcessChildFocus(wxChildFocusEvent& event)
{
	Cancel();
}

void ibAutoComplete::OnProcessSize(wxSizeEvent& event)
{
	Cancel();
}

void ibAutoComplete::OnProcessMouse(wxMouseEvent& event)
{
	Cancel();
}
