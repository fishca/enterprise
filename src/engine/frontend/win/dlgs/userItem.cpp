#include "userItem.h"

#include "backend/appData.h"
#include "backend/utils/wxmd5.hpp"

#include "backend/metadataConfiguration.h"

bool CDialogUserItem::ReadUserData(const CGuid& userGuid, bool copy)
{
	if (m_userGuid.isValid())
		return false;

	const CApplicationDataUserInfo& userInfo = appData->ReadUserData(userGuid);
	if (userInfo.IsOk()) {

		m_textName->SetValue(userInfo.m_strUserName);
		m_textFullName->SetValue(userInfo.m_strUserFullName);

		if (userInfo.IsSetPassword()) {
			m_bInitialized = false;
			m_textPassword->SetValue(
				wxT("12345678")
			);
			m_bInitialized = true;
		}

		if (userInfo.IsSetLanguage()) {

			auto iterator = std::find_if(m_languageArray.begin(), m_languageArray.end(),
				[userInfo](const auto& pair) { return userInfo.m_strLanguageGuid == pair.second.m_strLanguageGuid; });

			if (iterator != m_languageArray.end()) {
				m_choiceLanguage->SetSelection(iterator->first);
			}
			else if (activeMetaData != nullptr) {
				const wxString& strLanguageCode = activeMetaData->GetLangCode();
				auto iterator_active_metadata = std::find_if(m_languageArray.begin(), m_languageArray.end(),
					[strLanguageCode](const auto& pair) { return strLanguageCode == pair.second.m_strLanguageCode; });
				m_choiceLanguage->SetSelection(iterator_active_metadata->first);
			}
			else {
				m_choiceLanguage->SetSelection(0);
			}
		}

		for (const auto role : userInfo.m_roleArray) {

			auto iterator = std::find_if(m_roleArray.begin(), m_roleArray.end(),
				[role](const auto& pair) { return role.m_strRoleGuid == pair.second.m_strRoleGuid; });

			if (iterator != m_roleArray.end())
				m_choiceRole->Check(iterator->first, true);
		}

		if (!copy)
			m_userGuid = userInfo.m_strUserGuid;

		return true;
	}

	return false;
}

#include "backend/metaCollection/metaRoleObject.h"
#include "backend/metaCollection/metaLanguageObject.h"

CDialogUserItem::CDialogUserItem(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) :
	wxDialog(parent, id, title, pos, size, style), m_bInitialized(false)
{
	this->SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	m_mainNotebook = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TOP | wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS);
	m_mainNotebook->SetArtProvider(new wxAuiLunaTabArt());
	m_main = new wxPanel(m_mainNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	wxBoxSizer* sizerUser = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* sizerUserTop = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* sizerLabel = new wxBoxSizer(wxVERTICAL);

	m_staticName = new wxStaticText(m_main, wxID_ANY, _("Name:"), wxDefaultPosition, wxDefaultSize, 0);
	m_staticName->Wrap(-1);
	sizerLabel->Add(m_staticName, 0, wxALL, 9);

	m_staticFullName = new wxStaticText(m_main, wxID_ANY, _("Full name:"), wxDefaultPosition, wxDefaultSize, 0);
	m_staticFullName->Wrap(-1);
	sizerLabel->Add(m_staticFullName, 0, wxALL, 9);

	sizerUserTop->Add(sizerLabel, 0, 0, 5);

	wxBoxSizer* sizerText = new wxBoxSizer(wxVERTICAL);
	m_textName = new wxTextCtrl(m_main, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	sizerText->Add(m_textName, 0, wxALL | wxEXPAND, 5);
	m_textFullName = new wxTextCtrl(m_main, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	sizerText->Add(m_textFullName, 0, wxALL | wxEXPAND, 5);
	sizerUserTop->Add(sizerText, 1, wxEXPAND, 5);
	sizerUser->Add(sizerUserTop, 0, wxEXPAND, 5);

	m_staticline = new wxStaticLine(m_main, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);
	sizerUser->Add(m_staticline, 0, wxALL | wxEXPAND, 5);

	wxBoxSizer* sizerUserBottom = new wxBoxSizer(wxHORIZONTAL);
	m_staticPassword = new wxStaticText(m_main, wxID_ANY, _("Password:"), wxDefaultPosition, wxDefaultSize, 0);
	m_staticPassword->Wrap(-1);
	sizerUserBottom->Add(m_staticPassword, 0, wxALL, 10);
	m_textPassword = new wxTextCtrl(m_main, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
	sizerUserBottom->Add(m_textPassword, 1, wxALL, 5);

	sizerUser->Add(sizerUserBottom, 1, wxEXPAND, 5);

	m_main->SetSizer(sizerUser);
	m_main->Layout();
	sizerUser->Fit(m_main);
	m_mainNotebook->AddPage(m_main, _("User"), false, wxNullBitmap);
	m_other = new wxPanel(m_mainNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	wxBoxSizer* sizerOther = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* sizerLabels = new wxBoxSizer(wxVERTICAL);

	m_staticRole = new wxStaticText(m_other, wxID_ANY, _("Role:"), wxDefaultPosition, wxDefaultSize, 0);
	m_staticRole->Wrap(-1);
	m_staticLanguage = new wxStaticText(m_other, wxID_ANY, _("Language:"), wxDefaultPosition, wxDefaultSize, 0);
	m_staticLanguage->Wrap(-1);

	wxImageList* imageList = new wxImageList(16, 16);

	m_choiceRole = new wxCheckTree(m_other, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HIDE_ROOT | wxTR_ROW_LINES | wxTR_SINGLE | wxCR_EMPTY_CHECK | wxTR_TWIST_BUTTONS);
	m_choiceRole->SetImageList(imageList);

	const wxTreeItemId& root = m_choiceRole->AddRoot(wxT(""));

	for (const auto object : activeMetaData->GetAnyArrayObject<CMetaObjectRole>(g_metaRoleCLSID)) {
		CDataUserRole entry;
		entry.m_strRoleGuid = object->GetDocPath();
		entry.m_strRoleName = object->GetName();
		entry.m_miRoleId = object->GetMetaID();
		const int image = imageList->Add(object->GetIcon());
		const wxTreeItemId& id = m_choiceRole->AppendItem(root, object->GetSynonym(), image, image);
		m_choiceRole->SetItemState(id, wxCheckTree::UNCHECKED);
		m_roleArray.insert_or_assign(id, entry);
	}

	m_choiceLanguage = new wxChoice(m_other, wxID_ANY, wxDefaultPosition, wxDefaultSize);
	for (const auto object : activeMetaData->GetAnyArrayObject<CMetaObjectLanguage>(g_metaLanguageCLSID)) {

		CDataUserLanguageItem entry;
		entry.m_strLanguageGuid = object->GetDocPath();
		entry.m_strLanguageCode = object->GetLangCode();

		const int index = m_choiceLanguage->Append(object->GetSynonym());
		m_languageArray.insert_or_assign(index, std::move(entry));
	}

	m_choiceLanguage->SetSelection(0);

	sizerLabels->Add(m_staticRole, 0, wxEXPAND, 5);
	sizerLabels->Add(m_choiceRole, 1, wxEXPAND, 5);
	sizerOther->Add(sizerLabels, 1, wxEXPAND, 5);

	wxBoxSizer* sizerChoice = new wxBoxSizer(wxVERTICAL);

	sizerChoice->Add(m_staticLanguage, 0, wxEXPAND, 5);
	sizerChoice->Add(m_choiceLanguage, 0, wxEXPAND, 5);

	sizerOther->Add(sizerChoice, 0, wxEXPAND, 5);
	m_other->SetSizer(sizerOther);

	m_mainNotebook->AddPage(m_other, _("Other"), true, wxNullBitmap);
	mainSizer->Add(m_mainNotebook, 1, wxEXPAND | wxALL, 5);

	m_bottom = new wxStdDialogButtonSizer();
	m_bottomOK = new wxButton(this, wxID_OK);
	m_bottom->AddButton(m_bottomOK);
	m_bottomCancel = new wxButton(this, wxID_CANCEL);
	m_bottom->AddButton(m_bottomCancel);
	m_bottom->Realize();

	m_mainNotebook->SetSelection(0);
	mainSizer->Add(m_bottom, 0, wxEXPAND, 5);

	this->SetSizer(mainSizer);
	this->Layout();

	this->Centre(wxBOTH);

	wxIcon dlg_icon;
	dlg_icon.CopyFromBitmap(CBackendPicture::GetPicture(g_picUserCLSID));

	wxDialog::SetIcon(dlg_icon);
	wxDialog::SetFocus();

	// Connect Events
	m_textPassword->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(CDialogUserItem::OnPasswordText), nullptr, this);

	m_bottomOK->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CDialogUserItem::OnOKButtonClick), nullptr, this);
	m_bottomCancel->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CDialogUserItem::OnCancelButtonClick), nullptr, this);

	m_bInitialized = true;
}

void CDialogUserItem::OnPasswordText(wxCommandEvent& event)
{
	if (m_bInitialized) {
		const wxString& strUserPassword = m_textPassword->GetValue();
		if (!strUserPassword.IsEmpty()) {
			m_strUserPassword = wxMD5::ComputeMd5(strUserPassword);
		}
		else {
			m_strUserPassword.Clear();
		}
	}

	event.Skip();
}

void CDialogUserItem::OnOKButtonClick(wxCommandEvent& event)
{
	if (m_textName->IsEmpty())
		return;

	CApplicationDataUserInfo userInfo;

	if (!m_userGuid.isValid())
		m_userGuid = wxNewUniqueGuid;

	userInfo.m_strUserGuid = m_userGuid.str();
	userInfo.m_strUserName = m_textName->GetValue();
	userInfo.m_strUserFullName = m_textFullName->GetValue();
	userInfo.m_strUserPassword = m_strUserPassword;

	const int selection = m_choiceLanguage->GetSelection();
	if (selection != wxNOT_FOUND) {
		const CDataUserLanguageItem& info = m_languageArray.at(selection);
		userInfo.m_strLanguageGuid = info.m_strLanguageGuid;
		userInfo.m_strLanguageName = info.m_strLanguageName;
		userInfo.m_strLanguageCode = info.m_strLanguageCode;
	}

	const wxTreeItemId& root = m_choiceRole->GetRootItem();

	wxTreeItemIdValue coockie;
	wxTreeItemId item = m_choiceRole->GetFirstChild(root, coockie);

	while (item.IsOk()) {
		if (wxCheckTree::CHECKED == m_choiceRole->GetItemState(item)) {
			const CDataUserRole& info = m_roleArray.at(item);
			auto& entry = userInfo.m_roleArray.emplace_back();
			entry.m_strRoleGuid = info.m_strRoleGuid;
			entry.m_strRoleName = info.m_strRoleName;
			entry.m_miRoleId = info.m_miRoleId;
		}
		item = m_choiceRole->GetNextChild(item, coockie);
	}

	if (!appData->SaveUserData(userInfo))
		return;

	event.Skip();
}

void CDialogUserItem::OnCancelButtonClick(wxCommandEvent& event)
{
	event.Skip();
}