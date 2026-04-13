#include "predefinedEditor.h"

#include "frontend/visualView/ctrl/frame.h"
#include "frontend/win/theme/luna_toolbarart.h"

enum
{
	model_name,
	model_code,
	model_description
};

void CDialogPredefinedEditor::CDataViewPredefinedTreeStore::GetValue(wxVariant& variant,
	const wxDataViewExtItem& item, unsigned int col) const
{
	if (item.m_pItem == (void*)1)
	{
		if (col == model_name)
			variant = _("Predefined items");

		return;
	}

	CPredefinedValueObject* predefined =
		static_cast<CPredefinedValueObject*>(item.GetID());

	if (predefined != nullptr) {
		if (col == model_name)
			variant = predefined->GetPredefinedName();
		else if (col == model_code)
			variant = predefined->GetPredefinedCode();
		else if (col == model_description)
			variant = predefined->GetPredefinedDescription();
	}
}

bool CDialogPredefinedEditor::CDataViewPredefinedTreeStore::IsContainer(const wxDataViewExtItem& item) const
{
	if (item.m_pItem == (void*)1)
		return true;

	CPredefinedValueObject* predefined =
		static_cast<CPredefinedValueObject*>(item.GetID());

	if (predefined != nullptr)
		return predefined->IsPredefinedFolder();

	return true;
}

unsigned int CDialogPredefinedEditor::CDataViewPredefinedTreeStore::GetChildren(const wxDataViewExtItem& parent,
	wxDataViewExtItemArray& array) const
{
	//if (!parent.IsOk())
	//{
	//	array.Add(wxDataViewExtItem((void*)1));
	//	return 1;
	//}

	for (auto object : m_valueMetaObjectHierarchy->GetPredefinedValueArray())
	{
		array.Add(wxDataViewExtItem(object.get()));
	}

	return array.Count();
}

/////////////////////////////////////////////////////////////////////////////////////

void CDialogPredefinedEditor::CreateDialogView()
{
	wxDialog::SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxBoxSizer* sizerList = new wxBoxSizer(wxVERTICAL);

	m_toolbarTableEditor = new wxAuiToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_HORZ_TEXT);
	m_toolbarTableEditor->SetArtProvider(new wxAuiLunaToolBarArt());

	m_toolAdd = m_toolbarTableEditor->AddTool(wxID_TOOL_ADD, _("Add"), CBackendPicture::GetPicture(g_picAddCLSID), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, nullptr);
	m_toolCopy = m_toolbarTableEditor->AddTool(wxID_TOOL_COPY, _("Copy"), CBackendPicture::GetPicture(g_picCopyCLSID), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, nullptr);
	m_toolEdit = m_toolbarTableEditor->AddTool(wxID_TOOL_EDIT, _("Edit"), CBackendPicture::GetPicture(g_picEditCLSID), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, nullptr);
	m_toolDelete = m_toolbarTableEditor->AddTool(wxID_TOOL_DELETE, _("Delete"), CBackendPicture::GetPicture(g_picDeleteCLSID), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, nullptr);

	m_toolbarTableEditor->EnableTool(wxID_TOOL_ADD, m_tableModelStore->IsEnabled(wxDataViewExtItem(nullptr), model_name));
	m_toolbarTableEditor->EnableTool(wxID_TOOL_COPY, m_tableModelStore->IsEnabled(wxDataViewExtItem(nullptr), model_name));
	m_toolbarTableEditor->EnableTool(wxID_TOOL_EDIT, m_tableModelStore->IsEnabled(wxDataViewExtItem(nullptr), model_name));
	m_toolbarTableEditor->EnableTool(wxID_TOOL_DELETE, m_tableModelStore->IsEnabled(wxDataViewExtItem(nullptr), model_name));

	m_toolbarTableEditor->SetForegroundColour(wxDefaultStypeFGColour);
	m_toolbarTableEditor->SetBackgroundColour(wxDefaultStypeBGColour);

	m_toolbarTableEditor->Realize();
	m_toolbarTableEditor->Connect(wxEVT_MENU, wxCommandEventHandler(CDialogPredefinedEditor::OnCommandMenu), nullptr, this);

	sizerList->Add(m_toolbarTableEditor, 0, wxALL | wxEXPAND, 0);

	wxDataViewExtColumn* columnName = new wxDataViewExtColumn(_("Name"), new wxDataViewExtTextRenderer(wxT("string"), wxDATAVIEW_CELL_INERT), model_name, FromDIP(125), wxALIGN_LEFT,
		wxDATAVIEW_COL_SORTABLE);

	wxDataViewExtColumn* columnCode = new wxDataViewExtColumn(_("Code"), new wxDataViewExtTextRenderer(wxT("string"), wxDATAVIEW_CELL_INERT), model_code, FromDIP(100), wxALIGN_LEFT,
		wxDATAVIEW_COL_SORTABLE);

	wxDataViewExtColumn* columnDescription = new wxDataViewExtColumn(_("Description"), new wxDataViewExtTextRenderer(wxT("string"), wxDATAVIEW_CELL_INERT), model_description, FromDIP(175), wxALIGN_LEFT,
		wxDATAVIEW_COL_SORTABLE);

	m_tableEditor = new wxDataViewExtCtrl(this, wxID_ANY);

	m_tableEditor->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &CDialogPredefinedEditor::OnItemActivated, this);
	m_tableEditor->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &CDialogPredefinedEditor::OnContextMenu, this);

	m_tableEditor->Connect(wxEVT_MENU, wxCommandEventHandler(CDialogPredefinedEditor::OnCommandMenu), nullptr, this);

	m_tableEditor->AppendColumn(columnName);
	m_tableEditor->AppendColumn(columnCode);
	m_tableEditor->AppendColumn(columnDescription);

	m_tableEditor->SetForegroundColour(wxDefaultStypeFGColour);
	//m_tableEditor->SetBackgroundColour(wxDefaultStypeBGColour);

	m_tableEditor->AssociateModel(m_tableModelStore);

	sizerList->Add(m_tableEditor, 1, wxALL | wxEXPAND, 5);

	wxDialog::SetSizer(sizerList);
	wxDialog::Layout();
	wxDialog::Centre(wxBOTH);

	wxIcon dlg_icon;
	dlg_icon.CopyFromBitmap(CBackendPicture::GetPicture(g_metaAttributeCLSID));

	wxDialog::SetIcon(dlg_icon);
	wxDialog::SetFocus();
}

void CDialogPredefinedEditor::OnContextMenu(wxDataViewExtEvent& event)
{
	wxMenu menu;

	for (unsigned int idx = 0; idx < m_toolbarTableEditor->GetToolCount(); idx++) {
		wxAuiToolBarItem* tool = m_toolbarTableEditor->FindToolByIndex(idx);
		if (tool) {
			wxMenuItem* item = menu.Append(tool->GetId(), tool->GetLabel());
			item->Enable(m_tableModelStore->IsEnabled(wxDataViewExtItem(nullptr), model_name));
			item->SetBitmap(tool->GetBitmapBundle());
		}
	}

	m_tableEditor->PopupMenu(&menu);
}

void CDialogPredefinedEditor::OnCommandMenu(wxCommandEvent& event)
{
	const wxDataViewExtItem& selection =
		m_tableEditor->GetSelection();

	int result = wxID_CANCEL;

	if (event.GetId() == wxID_TOOL_ADD) {
		CDialogPredefinedItem dlg(this);
		result = dlg.ShowModal();
	}
	else if (event.GetId() == wxID_TOOL_COPY) {
		if (selection.IsOk()) {

			CPredefinedValueObject* predefined =
				static_cast<CPredefinedValueObject*>(selection.GetID());

			CDialogPredefinedItem dlg(this,
				wxNewUniqueGuid,
				predefined->GetPredefinedParentName(),
				predefined->GetPredefinedName(),
				predefined->GetPredefinedCode(),
				predefined->GetPredefinedDescription());

			dlg.SetReadOnly(!m_tableModelStore->IsEnabled(selection, model_name));
			result = dlg.ShowModal();
		}
	}
	else if (event.GetId() == wxID_TOOL_EDIT) {
		if (selection.IsOk()) {

			CPredefinedValueObject* predefined =
				static_cast<CPredefinedValueObject*>(selection.GetID());

			CDialogPredefinedItem dlg(this,
				predefined->GetPredefinedGuid(),
				predefined->GetPredefinedParentName(),
				predefined->GetPredefinedName(),
				predefined->GetPredefinedCode(),
				predefined->GetPredefinedDescription());

			dlg.SetReadOnly(!m_tableModelStore->IsEnabled(selection, model_name));
			result = dlg.ShowModal();
		}
	}
	else if (event.GetId() == wxID_TOOL_DELETE) {

		if (selection.IsOk()) {
			CPredefinedValueObject* predefined =
				static_cast<CPredefinedValueObject*>(selection.GetID());

			DeletePredefinedValue(predefined->GetPredefinedGuid());
			result = wxID_OK;
		}
	}

	if (result == wxID_OK) m_tableModelStore->Cleared();

	event.Skip();
}

void CDialogPredefinedEditor::OnItemActivated(wxDataViewExtEvent& event)
{
	const wxDataViewExtItem& selection =
		m_tableEditor->GetSelection();

	CPredefinedValueObject* predefined =
		static_cast<CPredefinedValueObject*>(selection.GetID());

	CDialogPredefinedItem dlg(this,
		predefined->GetPredefinedGuid(),
		predefined->GetPredefinedParentName(),
		predefined->GetPredefinedName(),
		predefined->GetPredefinedCode(),
		predefined->GetPredefinedDescription());

	dlg.SetReadOnly(!m_tableModelStore->IsEnabled(selection, model_name));
	dlg.ShowModal();

	event.Skip();
}
