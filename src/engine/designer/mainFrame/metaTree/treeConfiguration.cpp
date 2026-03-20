////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : metaTree window
////////////////////////////////////////////////////////////////////////////

#include "treeConfiguration.h"
#include "backend/debugger/debugClient.h"
#include "frontend/win/theme/luna_toolbarart.h"

wxIMPLEMENT_ABSTRACT_CLASS(IMetaDataTree, wxPanel);
wxIMPLEMENT_DYNAMIC_CLASS(CMetadataTree, IMetaDataTree);

//**********************************************************************************
//*                                  metaTree									   *
//**********************************************************************************

#define ICON_SIZE 16

#include "frontend/artProvider/artProvider.h"

void IMetaDataTree::CreateToolBar(wxWindow* parent)
{
	wxASSERT(m_metaTreeToolbar);

	m_metaTreeToolbar = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_HORZ_LAYOUT);
	m_metaTreeToolbar->AddTool(ID_METATREE_NEW, _("New"), wxArtProvider::GetBitmap(wxART_ADD, wxART_FRONTEND, wxSize(16, 16)), _("New item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_EDIT, _("Edit"), wxArtProvider::GetBitmap(wxART_EDIT, wxART_FRONTEND, wxSize(16, 16)), _("Edit item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_DELETE, _("Delete"), wxArtProvider::GetBitmap(wxART_DELETE, wxART_FRONTEND, wxSize(16, 16)), _("Delete item"));
	m_metaTreeToolbar->AddSeparator();
	m_metaTreeToolbar->AddTool(ID_METATREE_UP, _("Up"), wxArtProvider::GetBitmap(wxART_UP, wxART_FRONTEND, wxSize(16, 16)), _("Up item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_DOWM, _("Down"), wxArtProvider::GetBitmap(wxART_DOWN, wxART_FRONTEND, wxSize(16, 16)), _("Down item"));
	m_metaTreeToolbar->AddSeparator();
	m_metaTreeToolbar->AddTool(ID_METATREE_SORT, _("Sort"), wxArtProvider::GetBitmap(wxART_SORT, wxART_FRONTEND, wxSize(16, 16)), _("Sort item"));
	m_metaTreeToolbar->Realize();

	m_metaTreeToolbar->SetArtProvider(new wxAuiLunaToolBarArt());
}

CMetadataTree::CMetadataTree()
	: IMetaDataTree(), m_metaData(nullptr)
{
	m_metaTreeToolbar = nullptr;
	m_metaTreeCtrl = nullptr;
}

CMetadataTree::CMetadataTree(wxWindow* parent, int id)
	: IMetaDataTree(parent, id), m_metaData(nullptr)
{
	m_metaTreeToolbar = new wxAuiToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_HORZ_LAYOUT);
	m_metaTreeToolbar->AddTool(ID_METATREE_NEW, _("New"), wxArtProvider::GetBitmap(wxART_ADD, wxART_FRONTEND, wxSize(16, 16)), _("New item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_EDIT, _("Edit"), wxArtProvider::GetBitmap(wxART_EDIT, wxART_FRONTEND, wxSize(16, 16)), _("Edit item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_DELETE, _("Delete"), wxArtProvider::GetBitmap(wxART_DELETE, wxART_FRONTEND, wxSize(16, 16)), _("Delete item"));
	m_metaTreeToolbar->AddSeparator();
	m_metaTreeToolbar->AddTool(ID_METATREE_UP, _("Up"), wxArtProvider::GetBitmap(wxART_UP, wxART_FRONTEND, wxSize(16, 16)), _("Up item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_DOWM, _("Down"), wxArtProvider::GetBitmap(wxART_DOWN, wxART_FRONTEND, wxSize(16, 16)), _("Down item"));
	m_metaTreeToolbar->AddSeparator();
	m_metaTreeToolbar->AddTool(ID_METATREE_SORT, _("Sort"), wxArtProvider::GetBitmap(wxART_SORT, wxART_FRONTEND, wxSize(16, 16)), _("Sort item"));
	m_metaTreeToolbar->Realize();

	m_metaTreeToolbar->SetArtProvider(new wxAuiLunaToolBarArt());

	//Create main tree
	m_metaTreeCtrl = new CMetaTreeCtrl(this);
	m_metaTreeCtrl->SetBackgroundColour(RGB(250, 250, 250));

	//set image list
	m_metaTreeCtrl->AssignImageList(
		new wxImageList(ICON_SIZE, ICON_SIZE)
	);

	m_searchTree = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	m_searchTree->Bind(wxEVT_SEARCH, &CMetadataTree::CMetaTreeCtrl::OnStartSearch, m_metaTreeCtrl);
	m_searchTree->Bind(wxEVT_COMMAND_TEXT_UPDATED, &CMetadataTree::CMetaTreeCtrl::OnCancelSearch, m_metaTreeCtrl);
	m_searchTree->ShowCancelButton(true);

	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnCreateItem, m_metaTreeCtrl, ID_METATREE_NEW);
	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnEditItem, m_metaTreeCtrl, ID_METATREE_EDIT);
	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnRemoveItem, m_metaTreeCtrl, ID_METATREE_DELETE);

	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnUpItem, m_metaTreeCtrl, ID_METATREE_UP);
	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnDownItem, m_metaTreeCtrl, ID_METATREE_DOWM);
	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnSortItem, m_metaTreeCtrl, ID_METATREE_SORT);

	// Set up the sizer for the panel
	wxBoxSizer* panelSizer = new wxBoxSizer(wxVERTICAL);
	panelSizer->Add(m_searchTree, 0, wxEXPAND, 1);
	panelSizer->Add(m_metaTreeToolbar, 0, wxEXPAND, 1);
	panelSizer->Add(m_metaTreeCtrl, 1, wxEXPAND, 1);
	SetSizer(panelSizer);

	//Init tree
	InitTree();
}

CMetadataTree::CMetadataTree(CMetaDocument* docParent, wxWindow* parent, int id)
	: IMetaDataTree(docParent, parent, id), m_metaData(nullptr)
{
	wxASSERT(m_metaTreeToolbar);

	m_metaTreeToolbar = new wxAuiToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_HORZ_LAYOUT);
	m_metaTreeToolbar->AddTool(ID_METATREE_NEW, _("New"), wxArtProvider::GetBitmap(wxART_ADD, wxART_FRONTEND, wxSize(16, 16)), _("New item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_EDIT, _("Edit"), wxArtProvider::GetBitmap(wxART_EDIT, wxART_FRONTEND, wxSize(16, 16)), _("Edit item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_DELETE, _("Delete"), wxArtProvider::GetBitmap(wxART_DELETE, wxART_FRONTEND, wxSize(16, 16)), _("Delete item"));
	m_metaTreeToolbar->AddSeparator();
	m_metaTreeToolbar->AddTool(ID_METATREE_UP, _("Up"), wxArtProvider::GetBitmap(wxART_UP, wxART_FRONTEND, wxSize(16, 16)), _("Up item"));
	m_metaTreeToolbar->AddTool(ID_METATREE_DOWM, _("Down"), wxArtProvider::GetBitmap(wxART_DOWN, wxART_FRONTEND, wxSize(16, 16)), _("Down item"));
	m_metaTreeToolbar->AddSeparator();
	m_metaTreeToolbar->AddTool(ID_METATREE_SORT, _("Sort"), wxArtProvider::GetBitmap(wxART_SORT, wxART_FRONTEND, wxSize(16, 16)), _("Sort item"));
	m_metaTreeToolbar->Realize();

	m_metaTreeToolbar->SetArtProvider(new wxAuiLunaToolBarArt());

	//Create main tree
	m_metaTreeCtrl = new CMetaTreeCtrl(this);
	m_metaTreeCtrl->SetBackgroundColour(RGB(250, 250, 250));

	//set image list
	m_metaTreeCtrl->AssignImageList(
		new wxImageList(ICON_SIZE, ICON_SIZE)
	);

	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnCreateItem, m_metaTreeCtrl, ID_METATREE_NEW);
	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnEditItem, m_metaTreeCtrl, ID_METATREE_EDIT);
	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnRemoveItem, m_metaTreeCtrl, ID_METATREE_DELETE);

	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnUpItem, m_metaTreeCtrl, ID_METATREE_UP);
	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnDownItem, m_metaTreeCtrl, ID_METATREE_DOWM);
	m_metaTreeToolbar->Bind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnSortItem, m_metaTreeCtrl, ID_METATREE_SORT);

	// Set up the sizer for the panel
	wxBoxSizer* panelSizer = new wxBoxSizer(wxVERTICAL);
	panelSizer->Add(m_metaTreeToolbar, 0, wxEXPAND, 1);
	panelSizer->Add(m_metaTreeCtrl, 1, wxEXPAND, 1);
	SetSizer(panelSizer);

	//Init tree
	InitTree();
}

CMetadataTree::~CMetadataTree()
{
	if (m_metaData != nullptr) m_metaData->SetMetaTree(nullptr);

	m_metaTreeToolbar->Unbind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnCreateItem, m_metaTreeCtrl, ID_METATREE_NEW);
	m_metaTreeToolbar->Unbind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnEditItem, m_metaTreeCtrl, ID_METATREE_EDIT);
	m_metaTreeToolbar->Unbind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnRemoveItem, m_metaTreeCtrl, ID_METATREE_DELETE);

	m_metaTreeToolbar->Unbind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnUpItem, m_metaTreeCtrl, ID_METATREE_UP);
	m_metaTreeToolbar->Unbind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnDownItem, m_metaTreeCtrl, ID_METATREE_DOWM);
	m_metaTreeToolbar->Unbind(wxEVT_MENU, &CMetadataTree::CMetaTreeCtrl::OnSortItem, m_metaTreeCtrl, ID_METATREE_SORT);

	if (m_searchTree)
		m_searchTree->Destroy();
}

//**********************************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CMetadataTree::CMetaTreeCtrl, wxTreeCtrl);

//**********************************************************************************

#include <wx/dataview.h>

#include "frontend/visualView/ctrl/frame.h"

void CMetadataTree::EditPredefinedValues(IValueMetaObjectRecordDataHierarchyMutableRef* valueMetaObjectHierarchy)
{
	class CDialogPredefinedEditor : public wxDialog {

		enum {
			wxID_TOOL_ADD = wxID_HIGHEST + 1,
			wxID_USERS_COPY,
			wxID_USERS_EDIT,
			wxID_USERS_DELETE,
		};

		enum
		{
			model_name,
			model_code,
			model_description
		};

		class wxDataViewPredefinedTreeStore : public wxDataViewModel
		{
		public:

			wxDataViewPredefinedTreeStore(IValueMetaObjectRecordDataHierarchyMutableRef* valueMetaObjectHierarchy)
				: wxDataViewModel(), m_valueMetaObjectHierarchy(valueMetaObjectHierarchy)
			{

			}

			// and implement some others by forwarding them to our own ones
			virtual void GetValue(wxVariant& variant,
				const wxDataViewItem& item, unsigned int col) const override
			{
				if (item.m_pItem == (void*)1)
				{
					if (col == model_name)
						variant = _("Predefined items");

					return;
				}

				IValueMetaObjectRecordDataHierarchyMutableRef::CPredefinedValueObject* predefined =
					static_cast<IValueMetaObjectRecordDataHierarchyMutableRef::CPredefinedValueObject*>(item.GetID());

				if (predefined != nullptr) {
					if (col == model_name)
						variant = predefined->GetPredefinedName();
					else if (col == model_code)
						variant = predefined->GetPredefinedCode();
					else if (col == model_description)
						variant = predefined->GetPredefinedDescription();
				}
			}

			// return true if the given item has a value to display in the given
			// column: this is always true except for container items which by default
			// only show their label in the first column (but see HasContainerColumns())
			virtual bool HasValue(const wxDataViewItem& item, unsigned col) const override
			{
				if (HasContainerColumns(item))
					return false;
				return true;
			}

			virtual bool SetValue(const wxVariant& variant,
				const wxDataViewItem& item, unsigned int col) override {
				return false;
			}

			virtual bool GetAttr(const wxDataViewItem& item, unsigned int col,
				wxDataViewItemAttr& attr) const override {

				return false;
			}

			virtual bool IsEnabled(const wxDataViewItem& item, unsigned int col) const override
			{
				return false;
			}

			virtual wxDataViewItem GetParent(const wxDataViewItem& item) const override {
				// the invisible root node has no parent
				if (!item.IsOk())
					return wxDataViewItem(nullptr);

				return wxDataViewItem(nullptr);
			}

			virtual bool IsContainer(const wxDataViewItem& item) const override {				
				
				if (item.m_pItem == (void*)1)
					return true; 

				IValueMetaObjectRecordDataHierarchyMutableRef::CPredefinedValueObject* predefined =
					static_cast<IValueMetaObjectRecordDataHierarchyMutableRef::CPredefinedValueObject*>(item.GetID());

				if (predefined != nullptr)
					return predefined->IsPredefinedFolder();

				return true;
			}

			// define current parent for hierarchical view 
			virtual unsigned int GetChildren(const wxDataViewItem& parent,
				wxDataViewItemArray& array) const override {

				if (!parent.IsOk())
				{
					array.Add(wxDataViewItem((void *)1));
					return 1;
				}

				for (auto object : m_valueMetaObjectHierarchy->GetPredefinedValueArray())
				{
					array.Add(wxDataViewItem(object.get()));
				}

				return array.Count();
			}

			// override sorting to always sort branches ascendingly
			virtual bool HasDefaultCompare() const override { return true; }

			virtual int Compare(const wxDataViewItem& item1, const wxDataViewItem& item2,
				unsigned int col, bool ascending) const override {

				// items must be different
				wxUIntPtr id1 = wxPtrToUInt(item1.GetID()),
					id2 = wxPtrToUInt(item2.GetID());
				return ascending ? id1 - id2 : id2 - id1;
			}

			virtual bool IsListModel() const { return false; }
			virtual bool IsVirtualListModel() const { return false; }

		private:

			IValueMetaObjectRecordDataHierarchyMutableRef* m_valueMetaObjectHierarchy;
		};

	public:

		// full ctor
		CDialogPredefinedEditor(wxWindow* parent, IValueMetaObjectRecordDataHierarchyMutableRef* valueMetaObjectHierarchy)
			:
			wxDialog(parent, wxID_ANY, _("Predefined values editor"),
				wxDefaultPosition, wxSize(500, 300), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
			m_valueMetaObjectHierarchy(valueMetaObjectHierarchy)
		{
			CreateDialogView(valueMetaObjectHierarchy);
		}

		void CreateDialogView(IValueMetaObjectRecordDataHierarchyMutableRef* valueMetaObjectHierarchy)
		{
			wxDialog::SetSizeHints(wxDefaultSize, wxDefaultSize);

			wxBoxSizer* sizerList = new wxBoxSizer(wxVERTICAL);

			m_toolbarTableEditor = new wxAuiToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_HORZ_TEXT);
			m_toolbarTableEditor->SetArtProvider(new wxAuiLunaToolBarArt());

			m_toolAdd = m_toolbarTableEditor->AddTool(wxID_TOOL_ADD, _("Add"), CBackendPicture::GetPicture(g_picAddCLSID), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, nullptr);
			m_toolCopy = m_toolbarTableEditor->AddTool(wxID_USERS_COPY, _("Copy"), CBackendPicture::GetPicture(g_picCopyCLSID), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, nullptr);
			m_toolEdit = m_toolbarTableEditor->AddTool(wxID_USERS_EDIT, _("Edit"), CBackendPicture::GetPicture(g_picEditCLSID), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, nullptr);
			m_toolDelete = m_toolbarTableEditor->AddTool(wxID_USERS_DELETE, _("Delete"), CBackendPicture::GetPicture(g_picDeleteCLSID), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, nullptr);

			m_toolbarTableEditor->SetForegroundColour(wxDefaultStypeFGColour);
			m_toolbarTableEditor->SetBackgroundColour(wxDefaultStypeBGColour);

			m_toolbarTableEditor->Realize();
			m_toolbarTableEditor->Connect(wxEVT_MENU, wxCommandEventHandler(CDialogPredefinedEditor::OnCommandMenu), nullptr, this);

			sizerList->Add(m_toolbarTableEditor, 0, wxALL | wxEXPAND, 0);

			wxDataViewColumn* columnName = new wxDataViewColumn(_("Name"), new wxDataViewTextRenderer(wxT("string"), wxDATAVIEW_CELL_INERT), model_name, FromDIP(125), wxALIGN_LEFT,
				wxDATAVIEW_COL_SORTABLE);

			wxDataViewColumn* columnCode = new wxDataViewColumn(_("Code"), new wxDataViewTextRenderer(wxT("string"), wxDATAVIEW_CELL_INERT), model_code, FromDIP(100), wxALIGN_LEFT,
				wxDATAVIEW_COL_SORTABLE);

			wxDataViewColumn* columnDescription = new wxDataViewColumn(_("Description"), new wxDataViewTextRenderer(wxT("string"), wxDATAVIEW_CELL_INERT), model_description, FromDIP(175), wxALIGN_LEFT,
				wxDATAVIEW_COL_SORTABLE);

			m_tableEditor = new wxDataViewCtrl(this, wxID_ANY);

			//m_tableEditor->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &CDialogPredefinedEditor::OnItemActivated, this);
			//m_tableEditor->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &CDialogPredefinedEditor::OnContextMenu, this);

			//m_tableEditor->Bind(wxEVT_MENU, &CDialogPredefinedEditor::OnCommandMenu, this);

			m_tableEditor->AppendColumn(columnName);
			m_tableEditor->AppendColumn(columnCode);
			m_tableEditor->AppendColumn(columnDescription);

			m_tableEditor->SetForegroundColour(wxDefaultStypeFGColour);
			//m_tableEditor->SetBackgroundColour(wxDefaultStypeBGColour);

			m_tableEditor->AssociateModel(new wxDataViewPredefinedTreeStore(valueMetaObjectHierarchy));

			sizerList->Add(m_tableEditor, 1, wxALL | wxEXPAND, 5);

			wxDialog::SetSizer(sizerList);
			wxDialog::Layout();
			wxDialog::Centre(wxBOTH);

			wxIcon dlg_icon;
			dlg_icon.CopyFromBitmap(CBackendPicture::GetPicture(g_metaAttributeCLSID));

			wxDialog::SetIcon(dlg_icon);
			wxDialog::SetFocus();
		}

		class CDialogPredefinedItem : public wxDialog {
		public:

			CDialogPredefinedItem(wxWindow* parent,
				const CGuid& itemGuid = wxNullGuid,
				const wxString& strPredefinedParentName = wxT(""),
				const wxString& strPredefinedName = wxT(""), const wxString& strCode = wxT(""), const wxString& strDescription = wxT("")) :
				wxDialog(parent, wxID_ANY, _("Predefined value")), m_itemGuid(itemGuid)
			{
				wxDialog::SetSizeHints(wxDefaultSize, wxDefaultSize);

				wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

				wxBoxSizer* sizerParent = new wxBoxSizer(wxHORIZONTAL);
				m_staticTextParent = new wxStaticText(this, wxID_ANY, _("Parent"), wxDefaultPosition, wxSize(75, -1), 0);
				m_staticTextParent->Wrap(-1);
				sizerParent->Add(m_staticTextParent, 0, wxALL, 5);

				m_textParent = new wxTextCtrl(this, wxID_ANY, strPredefinedParentName, wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
				sizerParent->Add(m_textParent, 1, wxALL, 5);

				mainSizer->Add(sizerParent, 0, wxEXPAND, 5);

				wxBoxSizer* sizerName = new wxBoxSizer(wxHORIZONTAL);
				m_staticTextName = new wxStaticText(this, wxID_ANY, _("Name"), wxDefaultPosition, wxSize(75, -1), 0);
				m_staticTextName->Wrap(-1);
				sizerName->Add(m_staticTextName, 0, wxALL, 5);
				m_textName = new wxTextCtrl(this, wxID_ANY, strPredefinedName, wxDefaultPosition, wxDefaultSize, 0);
				sizerName->Add(m_textName, 1, wxALL, 5);
				mainSizer->Add(sizerName, 0, wxEXPAND, 5);

				wxBoxSizer* sizerCode = new wxBoxSizer(wxHORIZONTAL);
				m_staticCode = new wxStaticText(this, wxID_ANY, _("Code"), wxDefaultPosition, wxSize(75, -1), 0);
				m_staticCode->Wrap(-1);
				sizerCode->Add(m_staticCode, 0, wxALL, 5);
				m_textCode = new wxTextCtrl(this, wxID_ANY, strCode, wxDefaultPosition, wxDefaultSize, 0);
				sizerCode->Add(m_textCode, 1, wxALL, 5);

				mainSizer->Add(sizerCode, 0, wxEXPAND, 5);

				wxBoxSizer* sizerDescription = new wxBoxSizer(wxHORIZONTAL);
				m_staticTextDescription = new wxStaticText(this, wxID_ANY, _("Description"), wxDefaultPosition, wxSize(75, -1), 0);
				m_staticTextDescription->Wrap(-1);
				sizerDescription->Add(m_staticTextDescription, 0, wxALL, 5);

				m_textDescription = new wxTextCtrl(this, wxID_ANY, strDescription, wxDefaultPosition, wxDefaultSize, 0);
				sizerDescription->Add(m_textDescription, 1, wxALL, 5);

				mainSizer->Add(sizerDescription, 0, wxEXPAND, 5);

				m_sdbSizer = new wxStdDialogButtonSizer();
				m_sdbSizerOK = new wxButton(this, wxID_OK);
				m_sdbSizerOK->Bind(wxEVT_BUTTON, &CDialogPredefinedItem::OnCommandOK, this);
				m_sdbSizer->AddButton(m_sdbSizerOK);
				m_sdbSizerCancel = new wxButton(this, wxID_CANCEL);
				m_sdbSizerCancel->Bind(wxEVT_BUTTON, &CDialogPredefinedItem::OnCommandCancel, this);
				m_sdbSizer->AddButton(m_sdbSizerCancel);
				m_sdbSizer->Realize();

				mainSizer->Add(m_sdbSizer, 0, wxEXPAND, 5);

				wxDialog::SetSizer(mainSizer);
				wxDialog::Layout();
				wxDialog::Centre(wxBOTH);

				wxIcon dlg_icon;
				dlg_icon.CopyFromBitmap(CBackendPicture::GetPicture(g_metaAttributeCLSID));

				wxDialog::SetIcon(dlg_icon);
				wxDialog::SetFocus();
			}

		protected:

			void OnCommandOK(wxCommandEvent& event)
			{
				if (m_itemGuid.isValid()) {
					GetOwner()->SetPredefinedValue(
						m_itemGuid,
						m_textName->GetValue(),
						m_textCode->GetValue(),
						m_textDescription->GetValue()
					);
				}
				else {
					GetOwner()->AppendPredefinedValue(
						m_textName->GetValue(),
						m_textCode->GetValue(),
						m_textDescription->GetValue()
					);
				}

				event.Skip();
			}

			void OnCommandCancel(wxCommandEvent& event) { event.Skip(); }

		private:

			CDialogPredefinedEditor* GetOwner() const { return static_cast<CDialogPredefinedEditor*>(m_parent); }

			CGuid m_itemGuid;

			wxStaticText* m_staticTextParent;
			wxTextCtrl* m_textParent;
			wxStaticText* m_staticTextName;
			wxTextCtrl* m_textName;
			wxStaticText* m_staticCode;
			wxTextCtrl* m_textCode;
			wxStaticText* m_staticTextDescription;
			wxTextCtrl* m_textDescription;
			wxStdDialogButtonSizer* m_sdbSizer;
			wxButton* m_sdbSizerOK;
			wxButton* m_sdbSizerCancel;
		};

		//append predefined value
		void AppendPredefinedValue(const wxString& strPredefinedName,
			const wxString& strCode, const wxString& strDescription,
			bool valueIsFolder = false, const wxObjectDataPtr<IValueMetaObjectRecordDataHierarchyMutableRef::CPredefinedValueObject>& valueParent = wxObjectDataPtr<IValueMetaObjectRecordDataHierarchyMutableRef::CPredefinedValueObject>())
		{
			m_valueMetaObjectHierarchy->AppendPredefinedValue(strPredefinedName,
				strCode, strDescription, valueIsFolder, valueParent);
		}

		void SetPredefinedValue(const CGuid& predefinedGuid, const wxString& strPredefinedName,
			const wxString& strCode, const wxString& strDescription,
			bool valueIsFolder = false, const wxObjectDataPtr<IValueMetaObjectRecordDataHierarchyMutableRef::CPredefinedValueObject>& valueParent = wxObjectDataPtr<IValueMetaObjectRecordDataHierarchyMutableRef::CPredefinedValueObject>())
		{
			m_valueMetaObjectHierarchy->SetPredefinedValue(predefinedGuid,
				strPredefinedName, strCode, strDescription,
				valueIsFolder, valueParent);
		}

	protected:

		void OnCommandMenu(wxCommandEvent& event)
		{
			if (event.GetId() == wxID_TOOL_ADD) {
				CDialogPredefinedItem dlg(this);
				dlg.ShowModal();
			}


			event.Skip();
		}

	private:

		wxAuiToolBar* m_toolbarTableEditor;

		wxAuiToolBarItem* m_toolAdd;
		wxAuiToolBarItem* m_toolCopy;
		wxAuiToolBarItem* m_toolEdit;
		wxAuiToolBarItem* m_toolDelete;

		wxDataViewCtrl* m_tableEditor;


		IValueMetaObjectRecordDataHierarchyMutableRef* m_valueMetaObjectHierarchy;
	};

	CDialogPredefinedEditor dlg(this, valueMetaObjectHierarchy);
	dlg.ShowModal();
}

//**********************************************************************************
//*                                  metaTree window						       *
//**********************************************************************************

wxBEGIN_EVENT_TABLE(CMetadataTree::CMetaTreeCtrl, wxTreeCtrl)

EVT_LEFT_UP(CMetadataTree::CMetaTreeCtrl::OnLeftUp)
EVT_LEFT_DOWN(CMetadataTree::CMetaTreeCtrl::OnLeftDown)
EVT_LEFT_DCLICK(CMetadataTree::CMetaTreeCtrl::OnLeftDClick)
EVT_RIGHT_UP(CMetadataTree::CMetaTreeCtrl::OnRightUp)
EVT_RIGHT_DOWN(CMetadataTree::CMetaTreeCtrl::OnRightDown)
EVT_RIGHT_DCLICK(CMetadataTree::CMetaTreeCtrl::OnRightDClick)
EVT_MOTION(CMetadataTree::CMetaTreeCtrl::OnMouseMove)
EVT_KEY_UP(CMetadataTree::CMetaTreeCtrl::OnKeyUp)
EVT_KEY_DOWN(CMetadataTree::CMetaTreeCtrl::OnKeyDown)

EVT_TREE_SEL_CHANGING(wxID_ANY, CMetadataTree::CMetaTreeCtrl::OnSelecting)
EVT_TREE_SEL_CHANGED(wxID_ANY, CMetadataTree::CMetaTreeCtrl::OnSelected)

EVT_TREE_ITEM_COLLAPSING(wxID_ANY, CMetadataTree::CMetaTreeCtrl::OnCollapsing)
EVT_TREE_ITEM_EXPANDING(wxID_ANY, CMetadataTree::CMetaTreeCtrl::OnExpanding)

EVT_TREE_BEGIN_DRAG(wxID_ANY, CMetadataTree::CMetaTreeCtrl::OnBeginDrag)
EVT_TREE_END_DRAG(wxID_ANY, CMetadataTree::CMetaTreeCtrl::OnEndDrag)

EVT_MENU(ID_METATREE_NEW, CMetadataTree::CMetaTreeCtrl::OnCreateItem)
EVT_MENU(ID_METATREE_EDIT, CMetadataTree::CMetaTreeCtrl::OnEditItem)
EVT_MENU(ID_METATREE_DELETE, CMetadataTree::CMetaTreeCtrl::OnRemoveItem)
EVT_MENU(ID_METATREE_PROPERTY, CMetadataTree::CMetaTreeCtrl::OnPropertyItem)

EVT_MENU(ID_METATREE_INSERT, CMetadataTree::CMetaTreeCtrl::OnInsertItem)
EVT_MENU(ID_METATREE_REPLACE, CMetadataTree::CMetaTreeCtrl::OnReplaceItem)
EVT_MENU(ID_METATREE_SAVE, CMetadataTree::CMetaTreeCtrl::OnSaveItem)

EVT_SET_FOCUS(CMetadataTree::CMetaTreeCtrl::OnSetFocus)
EVT_KILL_FOCUS(CMetadataTree::CMetaTreeCtrl::OnSetFocus)

EVT_MENU(wxID_COPY, CMetadataTree::CMetaTreeCtrl::OnCopyItem)
EVT_MENU(wxID_PASTE, CMetadataTree::CMetaTreeCtrl::OnPasteItem)

wxEND_EVENT_TABLE()

CMetadataTree::CMetaTreeCtrl::CMetaTreeCtrl()
	: wxTreeCtrl(), m_ownerTree(nullptr), m_metaView(new CMatadataTreeView(this))
{
	//set double buffered
	SetDoubleBuffered(true);
}

CMetadataTree::CMetaTreeCtrl::CMetaTreeCtrl(CMetadataTree* parent)
	: wxTreeCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS | wxTR_SINGLE | wxTR_NO_LINES | wxTR_TWIST_BUTTONS), m_ownerTree(parent), m_metaView(new CMatadataTreeView(this))
{
	wxAcceleratorEntry entries[2];
	entries[0].Set(wxACCEL_CTRL, (int)'C', wxID_COPY);
	entries[1].Set(wxACCEL_CTRL, (int)'V', wxID_PASTE);

	wxAcceleratorTable accel(2, entries);
	SetAcceleratorTable(accel);

	//set double buffered
	SetDoubleBuffered(true);
}

#include "frontend/docView/docManager.h"

CMetadataTree::CMetaTreeCtrl::~CMetaTreeCtrl()
{
	if (docManager != nullptr &&
		m_metaView == docManager->GetAnyUsableView()) {
		docManager->ActivateView(nullptr);
	}

	wxDELETE(m_metaView);
}

/////////////////////////////////////////////////////////////////

#include "frontend/mainFrame/mainFrame.h"

void CMetadataTree::CMetaTreeCtrl::CMatadataTreeView::OnActivateView(bool activate, wxView* activeView, wxView* deactiveView)
{
	if (activate) {
		const wxTreeItemId& item = m_ownerTree->GetSelection();
		objectInspector->SelectObject(
			item.IsOk() ? m_ownerTree->GetMetaObject(item) : nullptr);
	}
}

/////////////////////////////////////////////////////////////////