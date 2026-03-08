#include "about.h"

#include "backend/metaCollection/metaObjectMetadata.h"
#include "backend/appData.h"

wxString strContributors =
{
	wxT("wxWidgets and wxFormBuilder, Unknown Worlds Entertaiment team\n")
	wxT("2C team, whose ideas were taken as the basis for building the interpreter.\n")
	wxT("Tomasz Sowa which developed ttmath\n")
	wxT("And also everyone who was not mentioned here")
};

CDialogAbout::CDialogAbout(wxWindow* parent, int id) : wxDialog(parent, id, _("About..."), wxDefaultPosition, wxSize(450, 400))
{
	SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);

	m_staticTextHeader = new wxStaticText(this, wxID_ANY, wxString::Format(wxT("Open enterprise solutions, build %i"), GetBuildId()), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
	m_staticTextHeader->Wrap(-1);
	m_staticTextHeader->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial")));

	topSizer->Add(m_staticTextHeader, 0, wxALL | wxEXPAND, 5);

	m_staticTextFramework = new wxStaticText(this, wxID_ANY, wxT("a RAD tool powered by wxWidgets framework"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
	m_staticTextFramework->Wrap(-1);
	m_staticTextFramework->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial")));

	topSizer->Add(m_staticTextFramework, 0, wxALL | wxEXPAND, 5);

	m_staticTextCommunity = new wxStaticText(this, wxID_ANY, wxT("(c) 2026 OES community"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);

	m_staticTextCommunity->Wrap(-1);
	m_staticTextCommunity->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, true, wxT("Arial")));
	m_staticTextCommunity->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
	m_staticTextCommunity->SetCursor(wxCURSOR_HAND);

	m_staticTextCommunity->Bind(wxEVT_LEFT_UP,
		[](wxMouseEvent& event) { wxLaunchDefaultBrowser(wxT("https://github.com/open-enterprise-solutions/enterprise")); event.Skip(); });

	m_staticTextCommunity->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial")));

	topSizer->Add(m_staticTextCommunity, 0, wxALL | wxEXPAND, 5);

	m_staticlineHeader = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);

	topSizer->Add(m_staticlineHeader, 0, wxEXPAND | wxALL, 5);

	m_staticTextThanks = new wxStaticText(this, wxID_ANY, wxT("Thanks"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
	m_staticTextThanks->Wrap(-1);
	m_staticTextThanks->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial")));

	topSizer->Add(m_staticTextThanks, 0, wxALL | wxEXPAND, 5);

	m_textCtrlContributors = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_NO_VSCROLL | wxBORDER_SIMPLE);
	m_textCtrlContributors->SetValue(strContributors);

	topSizer->Add(m_textCtrlContributors, 1, wxALL | wxEXPAND, 5);
	mainSizer->Add(topSizer, 1, wxEXPAND, 5);

	wxStaticBoxSizer* infoSizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Info")), wxVERTICAL);

	if (appData->GetDatabaseMode() != eDatabaseMode::eNONE) {

		wxBoxSizer* bSizer4 = new wxBoxSizer(wxHORIZONTAL);
		m_staticDataBaseInfo = new wxStaticText(infoSizer->GetStaticBox(), wxID_ANY, appData->GetDatabaseModeDescr() + wxT(":"), wxDefaultPosition, wxSize(100, -1));
		m_staticDataBaseInfo->Wrap(-1);
		bSizer4->Add(m_staticDataBaseInfo, 0, wxALL | wxEXPAND, 5);

		m_textCtrl1 = new wxTextCtrl(infoSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxBORDER_NONE);
		m_textCtrl1->SetValue(appData->GetDatabaseDescription());

		bSizer4->Add(m_textCtrl1, 1, wxALL, 5);
		infoSizer->Add(bSizer4, 0, wxEXPAND, 5);
	}

	wxBoxSizer* bSizer5 = new wxBoxSizer(wxHORIZONTAL);
	m_staticAppInfo = new wxStaticText(infoSizer->GetStaticBox(), wxID_ANY, _("Application") + wxT(":"), wxDefaultPosition, wxSize(100, -1));
	m_staticAppInfo->Wrap(-1);
	bSizer5->Add(m_staticAppInfo, 0, wxALL | wxEXPAND, 5);
	m_textCtrl2 = new wxTextCtrl(infoSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxBORDER_NONE);
	m_textCtrl2->SetValue(appData->GetRunModeDescr());
	bSizer5->Add(m_textCtrl2, 1, wxALL, 5);
	infoSizer->Add(bSizer5, 0, wxEXPAND, 5);

	wxBoxSizer* bSizer6 = new wxBoxSizer(wxHORIZONTAL);
	m_staticUserInfo = new wxStaticText(infoSizer->GetStaticBox(), wxID_ANY, _("User") + wxT(":"), wxDefaultPosition, wxSize(100, -1));
	m_staticUserInfo->Wrap(-1);
	bSizer6->Add(m_staticUserInfo, 0, wxALL | wxEXPAND, 5);
	m_textCtrl3 = new wxTextCtrl(infoSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxBORDER_NONE);
	m_textCtrl3->SetValue(appData->GetUserName());
	bSizer6->Add(m_textCtrl3, 1, wxALL, 5);
	infoSizer->Add(bSizer6, 0, wxEXPAND, 5);

	wxBoxSizer* bSizer7 = new wxBoxSizer(wxHORIZONTAL);
	m_staticUserInfo = new wxStaticText(infoSizer->GetStaticBox(), wxID_ANY, _("Locale") + wxT(":"), wxDefaultPosition, wxSize(100, -1));
	m_staticUserInfo->Wrap(-1);
	bSizer7->Add(m_staticUserInfo, 0, wxALL | wxEXPAND, 5);
	m_textCtrl4 = new wxTextCtrl(infoSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxBORDER_NONE);
	m_textCtrl4->SetValue(appData->GetLocale());
	bSizer7->Add(m_textCtrl4, 1, wxALL, 5);
	infoSizer->Add(bSizer7, 0, wxEXPAND, 5);

	mainSizer->Add(infoSizer, 0, wxEXPAND, 5);

	wxBoxSizer* bottomSizer = new wxBoxSizer(wxVERTICAL);




	m_buttonOK = new wxButton(this, wxID_OK, wxT("&OK"));
	bottomSizer->Add(m_buttonOK, 0, wxALL | wxEXPAND, 5);
	mainSizer->Add(bottomSizer, 0, wxEXPAND, 5);

	wxDialog::SetSizer(mainSizer);
	wxDialog::Layout();
	wxDialog::Centre(wxBOTH);

	wxIcon dlg_icon;
	dlg_icon.CopyFromBitmap(CBackendPicture::GetPicture(g_metaCommonMetadataCLSID));

	wxDialog::SetIcon(dlg_icon);
	wxDialog::SetFocus();
}
