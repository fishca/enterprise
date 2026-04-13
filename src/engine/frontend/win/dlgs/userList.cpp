#include "userList.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "frontend/win/theme/luna_toolbarart.h"
#include "backend/metadataConfiguration.h"
#include "backend/appData.h"

#define colUserName 1
#define colUserFullName 2

class CUserListModel : public wxDataViewExtIndexListModel {
	wxDECLARE_NO_COPY_CLASS(CUserListModel);
public:

	CUserListModel() { PrepareData(); }

	void PrepareData() {
		m_aUsersData.clear();
		IDatabaseResultSet* resultSet =
			db_query->RunQueryWithResults("SELECT guid FROM %s;", user_table);
		while (resultSet->Next()) {
			m_aUsersData.push_back(
				resultSet->GetResultString("guid")
			);
		}
		resultSet->Close();
		Reset(m_aUsersData.size());
	}

	void DeleteRow(const wxDataViewExtItem& item) {
		unsigned int row = GetRow(item);
		db_query->RunQuery("DELETE FROM %s WHERE guid = '%s';", user_table, m_aUsersData[row].str());
		m_aUsersData.erase(m_aUsersData.begin() + row);
	}

	CGuid GetGuidByRow(const wxDataViewExtItem& item) const {
		return m_aUsersData.at(GetRow(item));
	}

	// Implement base class pure virtual methods.
	unsigned GetCount() const override { return m_aUsersData.size(); }

	void GetValueByRow(wxVariant& val, unsigned row, unsigned col) const override {
		IDatabaseResultSet* resultSet = db_query->RunQueryWithResults("SELECT name, fullName FROM %s WHERE guid = '%s';", user_table, m_aUsersData[row].str());
		if (resultSet->Next()) {
			if (col == colUserName) {
				val = resultSet->GetResultString("name");
			}
			else if (col == colUserFullName) {
				val = resultSet->GetResultString("fullName");
			}
		}
		resultSet->Close();
	}

	bool SetValueByRow(const wxVariant&, unsigned, unsigned) override {
		return false;
	}

private:
	std::vector<CGuid> m_aUsersData;
};

enum {
	wxID_USERS_TOOL_ADD = wxID_HIGHEST + 1,
	wxID_USERS_TOOL_COPY,
	wxID_USERS_TOOL_EDIT,
	wxID_USERS_TOOL_DELETE,
};

#include "frontend/visualView/ctrl/frame.h"

CDialogUserList::CDialogUserList(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxDialog(parent, id, title, pos, size, style)
{
	wxDialog::SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxBoxSizer* sizerList = new wxBoxSizer(wxVERTICAL);

	m_toolbarMain = new wxAuiToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_HORZ_TEXT);
	m_toolbarMain->SetArtProvider(new wxAuiLunaToolBarArt());

	m_toolAdd = m_toolbarMain->AddTool(wxID_USERS_TOOL_ADD, _("Add"), CBackendPicture::GetPicture(g_picAddCLSID), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, nullptr);
	m_toolCopy = m_toolbarMain->AddTool(wxID_USERS_TOOL_COPY, _("Copy"), CBackendPicture::GetPicture(g_picCopyCLSID), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, nullptr);
	m_toolEdit = m_toolbarMain->AddTool(wxID_USERS_TOOL_EDIT, _("Edit"), CBackendPicture::GetPicture(g_picEditCLSID), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, nullptr);
	m_toolDelete = m_toolbarMain->AddTool(wxID_USERS_TOOL_DELETE, _("Delete"), CBackendPicture::GetPicture(g_picDeleteCLSID), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, nullptr);

	m_toolbarMain->SetForegroundColour(wxDefaultStypeFGColour);
	m_toolbarMain->SetBackgroundColour(wxDefaultStypeBGColour);

	m_toolbarMain->Realize();
	m_toolbarMain->Connect(wxEVT_MENU, wxCommandEventHandler(CDialogUserList::OnCommandMenu), nullptr, this);

	sizerList->Add(m_toolbarMain, 0, wxALL | wxEXPAND, 0);

	wxDataViewExtColumn* columnName = new wxDataViewExtColumn(_("Name"), new wxDataViewExtTextRenderer(wxT("string"), wxDATAVIEW_CELL_INERT), colUserName, FromDIP(200), wxALIGN_LEFT,
		wxDATAVIEW_COL_SORTABLE);
	wxDataViewExtColumn* columnFullName = new wxDataViewExtColumn(_("Full name"), new wxDataViewExtTextRenderer(wxT("string"), wxDATAVIEW_CELL_INERT), colUserFullName, FromDIP(200), wxALIGN_LEFT,
		wxDATAVIEW_COL_SORTABLE);

	m_dataEditor = new wxDataViewExtCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);
	m_dataEditor->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &CDialogUserList::OnItemActivated, this);
	m_dataEditor->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &CDialogUserList::OnContextMenu, this);

	m_dataEditor->Bind(wxEVT_MENU, &CDialogUserList::OnCommandMenu, this);

	m_dataEditor->AppendColumn(columnName);
	m_dataEditor->AppendColumn(columnFullName);

	m_dataEditor->SetForegroundColour(wxDefaultStypeFGColour);
	//m_dataEditor->SetBackgroundColour(wxDefaultStypeBGColour);

	wxItemAttr attr(
		wxDefaultStypeFGColour,
		wxDefaultStypeBGColour,
		m_dataEditor->GetFont()
	);

	m_dataEditor->SetHeaderAttr(attr);

	m_dataEditor->AssociateModel(new CUserListModel);

	sizerList->Add(m_dataEditor, 1, wxALL | wxEXPAND, 5);

	wxDialog::SetSizer(sizerList);
	wxDialog::Layout();
	wxDialog::Centre(wxBOTH);

	wxIcon dlg_icon;
	dlg_icon.CopyFromBitmap(CBackendPicture::GetPicture(g_picUserListCLSID));

	wxDialog::SetIcon(dlg_icon);
	wxDialog::SetFocus();
}

CDialogUserList::~CDialogUserList()
{
}

#include "userItem.h"

void CDialogUserList::OnContextMenu(wxDataViewExtEvent& event)
{
	wxMenu menu;

	for (unsigned int idx = 0; idx < m_toolbarMain->GetToolCount(); idx++) {
		wxAuiToolBarItem* tool = m_toolbarMain->FindToolByIndex(idx);
		if (tool) {
			menu.Append(tool->GetId(), tool->GetLabel())->SetBitmap(tool->GetBitmapBundle());
		}
	}

	wxDataViewExtCtrl* wnd = wxDynamicCast(
		event.GetEventObject(), wxDataViewExtCtrl
	);

	wxASSERT(wnd);
	wnd->PopupMenu(&menu);
}

void CDialogUserList::OnItemActivated(wxDataViewExtEvent& event)
{
	CDialogUserItem dlg(this, wxID_ANY);

	CUserListModel* model =
		static_cast<CUserListModel*>(m_dataEditor->GetModel());

	dlg.ReadUserData(model->GetGuidByRow(event.GetItem()));
	dlg.ShowModal();

	event.Skip();
}

void CDialogUserList::OnCommandMenu(wxCommandEvent& event)
{
	wxDataViewExtItem sel = m_dataEditor->GetSelection();

	if (event.GetId() == wxID_USERS_TOOL_ADD) {
		CDialogUserItem dlg(this, wxID_ANY);
		dlg.ShowModal();
	}

	CUserListModel* model =
		dynamic_cast<CUserListModel*>(m_dataEditor->GetModel());

	if (sel.IsOk()) {
		if (event.GetId() == wxID_USERS_TOOL_EDIT) {
			CDialogUserItem dlg(this, wxID_ANY);
			dlg.ReadUserData(model->GetGuidByRow(sel));
			dlg.ShowModal();
		}

		if (event.GetId() == wxID_USERS_TOOL_COPY) {
			CDialogUserItem dlg(this, wxID_ANY);
			dlg.ReadUserData(model->GetGuidByRow(sel), true);
			dlg.ShowModal();
		}

		if (event.GetId() == wxID_USERS_TOOL_DELETE) {
			model->DeleteRow(sel);
		}
	}

	model->PrepareData(); event.Skip();
}
