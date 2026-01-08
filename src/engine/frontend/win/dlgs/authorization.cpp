#include "authorization.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/appData.h"

///////////////////////////////////////////////////////////////////////////

#include "frontend/visualView/ctrl/frame.h"

CDialogAuthentication::CDialogAuthentication(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxDialog(parent, id, title, pos, size, style)
{
	wxDialog::SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxBoxSizer* bSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* bSizerCtrl = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* bSizerHeader = new wxBoxSizer(wxVERTICAL);

	m_staticTextLogin = new wxStaticText(this, wxID_ANY, _("Login:"), wxDefaultPosition, wxDefaultSize, 0);
	m_staticTextLogin->Wrap(-1);
	bSizerHeader->Add(m_staticTextLogin, 0, wxALL, 10);

	m_staticTextPassword = new wxStaticText(this, wxID_ANY, _("Password:"), wxDefaultPosition, wxDefaultSize, 0);
	m_staticTextPassword->Wrap(-1);
	bSizerHeader->Add(m_staticTextPassword, 0, wxALL, 9);

	bSizerCtrl->Add(bSizerHeader, 0, 0, 5);

	wxBoxSizer* bSizerBottom = new wxBoxSizer(wxVERTICAL);

	m_comboBoxLogin = new wxBitmapComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize);
	for (const auto userInfo : appData->GetAllowedUser()) {
		m_comboBoxLogin->Append(userInfo.m_strUserName, CBackendPicture::GetPicture(g_picUserCLSID));
	}

	bSizerBottom->Add(m_comboBoxLogin, 0, wxALL | wxEXPAND, 5);
	m_textCtrlPassword = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
	bSizerBottom->Add(m_textCtrlPassword, 0, wxALL | wxEXPAND, 5);
	bSizerCtrl->Add(bSizerBottom, 1, wxEXPAND, 5);
	bSizer->Add(bSizerCtrl, 0, wxEXPAND, 5);

	wxStdDialogButtonSizer* bSizerButtons = new wxStdDialogButtonSizer();
	m_buttonOK = new wxButton(this, wxID_OK);
	bSizerButtons->AddButton(m_buttonOK);
	m_buttonCancel = new wxButton(this, wxID_CANCEL);
	bSizerButtons->AddButton(m_buttonCancel);
	bSizerButtons->Realize();

	bSizer->Add(bSizerButtons, 1, wxALIGN_RIGHT, 5);

	wxDialog::SetSizer(bSizer);
	wxDialog::Layout();

	wxDialog::Centre(wxBOTH);

	wxIcon dlg_icon;
	dlg_icon.CopyFromBitmap(CBackendPicture::GetPicture(g_picAuthenticationCLSID));

	wxDialog::SetIcon(dlg_icon);
	wxDialog::SetFocus();

	// Connect Events
	m_buttonOK->Bind(wxEVT_COMMAND_BUTTON_CLICKED, [&](wxCommandEvent& event) {
		bool success = appData->AuthenticationAndSetUser(m_comboBoxLogin->GetValue(), m_textCtrlPassword->GetValue());
		if (success) event.Skip();
		else wxMessageBox(_("Wrong user or password entered!"));
		}
	);
}
