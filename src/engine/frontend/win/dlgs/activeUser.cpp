#include "activeUser.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/metadataConfiguration.h"
#include "backend/appData.h"

void CDialogActiveUser::RefreshActiveUserTable()
{
	const CApplicationDataSessionArray& arr = appData->GetSessionArray();

	if (m_sessionArrayHash != arr.GetSessionArrayHash()) {

		wxString current_session;
		const long selected = m_activeTable->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (selected != -1) current_session = m_activeTable->GetItemText(selected, 4);

		m_activeTable->ClearAll();

		m_activeTable->AppendColumn(_("User"), wxLIST_FORMAT_LEFT, 190);
		m_activeTable->AppendColumn(_("Application"), wxLIST_FORMAT_LEFT, 120);
		m_activeTable->AppendColumn(_("Started"), wxLIST_FORMAT_LEFT, 120);
		m_activeTable->AppendColumn(_("Computer"), wxLIST_FORMAT_LEFT, 145);
		m_activeTable->AppendColumn(_("Session"), wxLIST_FORMAT_LEFT, 0); //hide 

		for (unsigned int idx = 0; idx < arr.GetSessionCount(); idx++) {

			const long index = m_activeTable->InsertItem(m_activeTable->GetItemCount(), arr.GetUserName(idx));

			m_activeTable->SetItem(index, 0, arr.GetUserName(idx));
			m_activeTable->SetItem(index, 1, arr.GetApplication(idx));
			m_activeTable->SetItem(index, 2, arr.GetStartedDate(idx));
			m_activeTable->SetItem(index, 3, arr.GetComputerName(idx));
			m_activeTable->SetItem(index, 4, arr.GetSession(idx));

			if (current_session == arr.GetSession(idx))
				m_activeTable->SetItemState(index, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);

		}
	
	}

	m_sessionArrayHash = arr.GetSessionArrayHash();
}

CDialogActiveUser::CDialogActiveUser(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) :
	wxDialog(parent, id, title, pos, size, style), m_activeTableScanner(new wxTimer)
{
	this->SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	m_activeTable = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	mainSizer->Add(m_activeTable, 1, wxALL | wxEXPAND, 5);

	this->SetSizer(mainSizer);
	this->Layout();
	this->Centre(wxBOTH);

	RefreshActiveUserTable();

	m_activeTableScanner->Bind(wxEVT_TIMER, &CDialogActiveUser::OnIdleHandler, this);
	m_activeTableScanner->Start(1000);
}

CDialogActiveUser::~CDialogActiveUser()
{
	if (m_activeTableScanner->IsRunning())  m_activeTableScanner->Stop();
	m_activeTableScanner->Unbind(wxEVT_TIMER, &CDialogActiveUser::OnIdleHandler, this);
}
