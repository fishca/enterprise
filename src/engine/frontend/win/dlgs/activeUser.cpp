#include "activeUser.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/metadataConfiguration.h"
#include "backend/appData.h"
#include "backend/session/session.h"
#include "backend/session/sessionRegistry.h"

#include <wx/menu.h>

void ibDialogActiveUser::RefreshActiveUserTable()
{
	const ibApplicationDataSessionArray& arr = appData->GetSessionArray();

	if (m_sessionArrayHash != arr.GetSessionArrayHash()) {

		wxString current_session;
		const long selected = m_activeTable->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		// Session guid column moved to index 5 to make room for the new
		// "Type" column (Server vs Client — matters for web where a
		// single process owns one Server row + N Client rows).
		if (selected != -1) current_session = m_activeTable->GetItemText(selected, 5);

		m_activeTable->ClearAll();

		wxImageList* imageList = new wxImageList(16, 16);

		m_activeTable->AppendColumn(_("User"), wxLIST_FORMAT_LEFT, 190);
		m_activeTable->AppendColumn(_("Application"), wxLIST_FORMAT_LEFT, 120);
		m_activeTable->AppendColumn(_("Type"), wxLIST_FORMAT_LEFT, 70);
		m_activeTable->AppendColumn(_("Started"), wxLIST_FORMAT_LEFT, 120);
		m_activeTable->AppendColumn(_("Computer"), wxLIST_FORMAT_LEFT, 145);
		m_activeTable->AppendColumn(_("Session"), wxLIST_FORMAT_LEFT, 0); //hide

		m_activeTable->AssignImageList(imageList, wxIMAGE_LIST_SMALL);

		const int imageUser =
			imageList->Add(ibBackendPicture::GetPicture(g_picUserCLSID));

		for (unsigned int idx = 0; idx < arr.GetSessionCount(); idx++) {

			const long index = m_activeTable->InsertItem(m_activeTable->GetItemCount(), arr.GetUserName(idx));

			m_activeTable->SetItem(index, 0, arr.GetUserName(idx), imageUser);
			m_activeTable->SetItem(index, 1, arr.GetApplication(idx));
			m_activeTable->SetItem(index, 2, arr.GetSessionKindDescr(idx));
			m_activeTable->SetItem(index, 3, arr.GetStartedDate(idx));
			m_activeTable->SetItem(index, 4, arr.GetComputerName(idx));
			m_activeTable->SetItem(index, 5, arr.GetSession(idx));

			if (current_session == arr.GetSession(idx))
				m_activeTable->SetItemState(index, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);

		}

	}

	m_sessionArrayHash = arr.GetSessionArrayHash();
}

#include "frontend/visualView/ctrl/frame.h"

ibDialogActiveUser::ibDialogActiveUser(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) :
	wxDialog(parent, id, title, pos, size, style), m_activeTableScanner(new wxTimer)
{
	wxDialog::SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	m_activeTable = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	mainSizer->Add(m_activeTable, 1, wxALL | wxEXPAND, FromDIP(5));

	wxDialog::SetSizer(mainSizer);
	wxDialog::Layout();
	wxDialog::Centre(wxBOTH);

	wxIcon dlg_icon;
	dlg_icon.CopyFromBitmap(ibBackendPicture::GetPicture(g_picUserActiveCLSID));

	wxDialog::SetIcon(dlg_icon);
	wxDialog::SetFocus();

	m_activeTable->SetForegroundColour(wxDefaultStypeFGColour);
	//m_activeTable->SetBackgroundColour(wxDefaultStypeBGColour);

	RefreshActiveUserTable();

	m_activeTableScanner->Bind(wxEVT_TIMER, &ibDialogActiveUser::OnIdleHandler, this);
	m_activeTableScanner->Start(1000);

	// Right-click on a row shows the admin context menu. Bound after the
	// initial refresh so m_activeTable definitely exists.
	m_activeTable->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK,
		&ibDialogActiveUser::OnContextMenu, this);
}

ibDialogActiveUser::~ibDialogActiveUser()
{
	if (m_activeTableScanner->IsRunning())  m_activeTableScanner->Stop();
	m_activeTableScanner->Unbind(wxEVT_TIMER, &ibDialogActiveUser::OnIdleHandler, this);
	m_activeTable->Unbind(wxEVT_LIST_ITEM_RIGHT_CLICK,
		&ibDialogActiveUser::OnContextMenu, this);
}

namespace {
enum {
	ID_ActiveUser_Kick   = wxID_HIGHEST + 1001,
	ID_ActiveUser_Reload = wxID_HIGHEST + 1002,
};
} // namespace

void ibDialogActiveUser::OnContextMenu(wxListEvent& event)
{
	const long row = event.GetIndex();
	if (row < 0) return;
	// Session guid lives in the hidden last column (index 5 after the
	// "Type" column was inserted).
	const wxString guid = m_activeTable->GetItemText(row, 5);
	if (guid.IsEmpty()) return;

	// Kick / Reload are designer-only admin actions. enterprise.exe
	// users see the dialog read-only — runtime operators don't get to
	// boot peers off. (Permission check on top of this comes later when
	// the role system surfaces an "admin" predicate.)
	if (!appData->DesignerMode()) return;

	wxMenu menu;
	menu.Append(ID_ActiveUser_Kick,   _("Kick session"));
	menu.Append(ID_ActiveUser_Reload, _("Reload clients"));
	menu.Bind(wxEVT_MENU, &ibDialogActiveUser::OnKickSelected,   this,
		ID_ActiveUser_Kick);
	menu.Bind(wxEVT_MENU, &ibDialogActiveUser::OnReloadSelected, this,
		ID_ActiveUser_Reload);
	PopupMenu(&menu);
}

void ibDialogActiveUser::OnKickSelected(wxCommandEvent&)
{
	const long row = m_activeTable->GetNextItem(-1, wxLIST_NEXT_ALL,
		wxLIST_STATE_SELECTED);
	if (row < 0) return;
	const wxString guid = m_activeTable->GetItemText(row, 5);
	if (guid.IsEmpty()) return;

	// Self-kick guard: if the selected row's session belongs to the
	// current process, refuse — kicking yourself only triggers a forced
	// close on the kicker's own UI, not on a peer.
	if (auto* self = ibSession::Current()) {
		if (guid == wxString::FromUTF8(self->GetId().c_str())) {
			wxMessageBox(_("Cannot kick the current session. Close the application instead."),
				wxTheApp->GetAppDisplayName(), wxOK | wxICON_INFORMATION, this);
			return;
		}
	}

	const wxString user = m_activeTable->GetItemText(row, 0);
	const wxString userLabel = user.IsEmpty()
		? wxString(_("(user not specified)"))
		: user;
	const int answer = wxMessageBox(
		wxString::Format(_("Kick session of '%s'?"), userLabel),
		wxTheApp->GetAppDisplayName(), wxYES_NO | wxICON_QUESTION, this);
	if (answer != wxYES) return;

	if (!ibSessionRegistry::Instance().Kick(guid)) {
		wxLogWarning(_("Failed to queue kick for session %s"), guid);
	}
}

void ibDialogActiveUser::OnReloadSelected(wxCommandEvent&)
{
	const long row = m_activeTable->GetNextItem(-1, wxLIST_NEXT_ALL,
		wxLIST_STATE_SELECTED);
	if (row < 0) return;
	const wxString guid = m_activeTable->GetItemText(row, 5);
	if (guid.IsEmpty()) return;

	// Same self-guard as kick: reloading the current process's own
	// session via the signal path would just close the designer that
	// invoked it.
	if (auto* self = ibSession::Current()) {
		if (guid == wxString::FromUTF8(self->GetId().c_str())) {
			wxMessageBox(_("Cannot reload the current session."),
				wxTheApp->GetAppDisplayName(), wxOK | wxICON_INFORMATION, this);
			return;
		}
	}

	if (!ibSessionRegistry::Instance().Reload(guid)) {
		wxLogWarning(_("Failed to queue reload for session %s"), guid);
	}
}
