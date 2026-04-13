#ifndef __PREDEFINED_EDITOR__
#define __PREDEFINED_EDITOR__

#include <wx/aui/auibar.h>

#include "backend/metaCollection/partial/commonObject.h"
#include "frontend/win/ctrls/dataview/dataview.h"

class ibDialogPredefinedEditor : public wxDialog {

	enum {
		wxID_TOOL_ADD = wxID_HIGHEST + 1,
		wxID_TOOL_COPY,
		wxID_TOOL_EDIT,
		wxID_TOOL_DELETE,
	};

	typedef ibValueMetaObjectRecordDataHierarchyMutableRef::ibPredefinedValueObject
		ibPredefinedValueObject;

	class ibDataViewPredefinedTreeStore : public ibDataViewModel
	{
	public:

		ibDataViewPredefinedTreeStore(ibValueMetaObjectRecordDataHierarchyMutableRef* valueMetaObjectHierarchy)
			: ibDataViewModel(), m_valueMetaObjectHierarchy(valueMetaObjectHierarchy) {
		}


		// and implement some others by forwarding them to our own ones
		virtual void GetValue(wxVariant& variant,
			const ibDataViewItem& item, unsigned int col) const override;

		// return true if the given item has a value to display in the given
		// column: this is always true except for container items which by default
		// only show their label in the first column (but see HasContainerColumns())
		virtual bool HasValue(const ibDataViewItem& item, unsigned col) const override
		{
			if (HasContainerColumns(item))
				return false;
			return true;
		}

		virtual bool SetValue(const wxVariant& variant,
			const ibDataViewItem& item, unsigned int col) override {
			return false;
		}

		virtual bool GetAttr(const ibDataViewItem& item, unsigned int col,
			ibDataViewItemAttr& attr) const override {

			return false;
		}

		virtual bool IsEnabled(const ibDataViewItem& item, unsigned int col) const override
		{
			return m_valueMetaObjectHierarchy->IsEditable();
		}

		virtual ibDataViewItem GetParent(const ibDataViewItem& item) const override {
			// the invisible root node has no parent
			if (!item.IsOk())
				return ibDataViewItem(nullptr);

			return ibDataViewItem(nullptr);
		}

		virtual bool IsContainer(const ibDataViewItem& item) const override;

		// define current parent for hierarchical view 
		virtual unsigned int GetChildren(const ibDataViewItem& parent,
			ibDataViewItemArray& array) const override;

		// override sorting to always sort branches ascendingly
		virtual bool HasDefaultCompare() const override { return true; }

		virtual int Compare(const ibDataViewItem& item1, const ibDataViewItem& item2,
			unsigned int col, bool ascending) const override {

			// items must be different
			wxUIntPtr id1 = wxPtrToUInt(item1.GetID()),
				id2 = wxPtrToUInt(item2.GetID());
			return ascending ? id1 - id2 : id2 - id1;
		}

		virtual bool IsListModel() const { return false; }
		virtual bool IsVirtualListModel() const { return false; }

		//is predefined value? 
		bool HasPredefinedValue(const ibGuid& predefinedGuid) const { return m_valueMetaObjectHierarchy->HasPredefinedValue(predefinedGuid); }
		bool HasPredefinedValue(const wxString& strPredefinedName) const { return m_valueMetaObjectHierarchy->HasPredefinedValue(strPredefinedName); }

		//append predefined value
		void AppendPredefinedValue(const wxString& strPredefinedName,
			const wxString& strCode, const wxString& strDescription) {
			m_valueMetaObjectHierarchy->AppendPredefinedValue(strPredefinedName,
				strCode, strDescription);
		}

		void SetPredefinedValue(const ibGuid& predefinedGuid, const wxString& strPredefinedName,
			const wxString& strCode, const wxString& strDescription) {
			m_valueMetaObjectHierarchy->SetPredefinedValue(predefinedGuid,
				strPredefinedName, strCode, strDescription);
		}

		void DeletePredefinedValue(const ibGuid& predefinedGuid) { m_valueMetaObjectHierarchy->DeletePredefinedValue(predefinedGuid); }

		//find predefined value
		wxObjectDataPtr<ibPredefinedValueObject> FindPredefinedValue(const ibGuid& predefinedGuid) const { return m_valueMetaObjectHierarchy->FindPredefinedValue(predefinedGuid); }
		wxObjectDataPtr<ibPredefinedValueObject> FindPredefinedValue(const wxString& predefinedName) const { return m_valueMetaObjectHierarchy->FindPredefinedValue(predefinedName); }

	private:

		ibValueMetaObjectRecordDataHierarchyMutableRef* m_valueMetaObjectHierarchy;
	};

	class ibDialogPredefinedItem : public wxDialog {
	public:

		ibDialogPredefinedItem(wxWindow* parent,
			const ibGuid& itemGuid = wxNullGuid,
			const wxString& strPredefinedParentName = wxT(""),
			const wxString& strPredefinedName = wxT(""), const wxString& strCode = wxT(""), const wxString& strDescription = wxT("")) :
			wxDialog(parent, wxID_ANY, _("Predefined value"), wxDefaultPosition, wxSize(350, 200)), m_itemGuid(itemGuid)
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
			m_sdbSizerOK->Bind(wxEVT_BUTTON, &ibDialogPredefinedItem::OnCommandOK, this);
			m_sdbSizer->AddButton(m_sdbSizerOK);
			m_sdbSizerCancel = new wxButton(this, wxID_CANCEL);
			m_sdbSizerCancel->Bind(wxEVT_BUTTON, &ibDialogPredefinedItem::OnCommandCancel, this);
			m_sdbSizer->AddButton(m_sdbSizerCancel);
			m_sdbSizer->Realize();

			mainSizer->Add(m_sdbSizer, 0, wxEXPAND, 5);

			wxDialog::SetSizer(mainSizer);
			wxDialog::Layout();
			wxDialog::Centre(wxBOTH);

			wxIcon dlg_icon;
			dlg_icon.CopyFromBitmap(ibBackendPicture::GetPicture(g_metaAttributeCLSID));

			wxDialog::SetIcon(dlg_icon);
			wxDialog::SetFocus();
		}

		void SetReadOnly(bool r = true)
		{
			m_textParent->Enable(!r);

			m_textName->Enable(!r);
			m_textCode->Enable(!r);
			m_textDescription->Enable(!r);

			m_sdbSizerOK->Enable(!r);
		}

	protected:

#include "backend/stringUtils.h"

		void OnCommandOK(wxCommandEvent& event)
		{
			const wxString& strName = m_textName->GetValue();
			const wxString& strCode = m_textCode->GetValue();
			const wxString& strDescription = m_textDescription->GetValue();

			if (strName.IsEmpty() || strCode.IsEmpty() || strDescription.IsEmpty()) {
				wxMessageBox(_("All key fields are not filled in!"));
				return;
			}

			if (stringUtils::CheckCorrectName(strName) >= 0) {
				wxMessageBox(_("The \"name\" field is filled in incorrectly!"));
				return;
			}

			if (m_itemGuid.isValid()) {

				const wxObjectDataPtr<ibPredefinedValueObject>& predefinedValue =
					GetOwner()->FindPredefinedValue(strName);

				if (predefinedValue != nullptr && predefinedValue->GetPredefinedGuid() != m_itemGuid) {
					wxMessageBox(_("The \"name\" field has not unique!"));
					return;
				}

				GetOwner()->SetPredefinedValue(
					m_itemGuid,
					strName,
					strCode,
					strDescription
				);
			}
			else {

				if (GetOwner()->HasPredefinedValue(strName)) {
					wxMessageBox(_("The \"name\" field has not unique!"));
					return;
				}

				GetOwner()->AppendPredefinedValue(
					strName,
					strCode,
					strDescription
				);
			}

			event.Skip();
		}

		void OnCommandCancel(wxCommandEvent& event) { event.Skip(); }

	private:

		ibDialogPredefinedEditor* GetOwner() const { return static_cast<ibDialogPredefinedEditor*>(m_parent); }

		ibGuid m_itemGuid;

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

public:

	// full ctor
	ibDialogPredefinedEditor(wxWindow* parent, ibValueMetaObjectRecordDataHierarchyMutableRef* valueMetaObjectHierarchy)
		:
		wxDialog(parent, wxID_ANY, _("Predefined values editor"),
			wxDefaultPosition, wxSize(500, 300), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
		m_tableModelStore(nullptr)
	{
		// add our own reference to the new model:
		m_tableModelStore = new ibDataViewPredefinedTreeStore(valueMetaObjectHierarchy);

		if (m_tableModelStore)
		{
			m_tableModelStore->IncRef();
		}

		CreateDialogView();
	}

	~ibDialogPredefinedEditor()
	{
		if (m_tableModelStore)
		{
			m_tableModelStore->DecRef();   // discard old model, if any
		}
	}

	void CreateDialogView();

	//is predefined value? 
	bool HasPredefinedValue(const ibGuid& predefinedGuid) const { return m_tableModelStore->HasPredefinedValue(predefinedGuid); }
	bool HasPredefinedValue(const wxString& strPredefinedName) const { return m_tableModelStore->HasPredefinedValue(strPredefinedName); }

	//append predefined value
	void AppendPredefinedValue(const wxString& strPredefinedName,
		const wxString& strCode, const wxString& strDescription) {
		m_tableModelStore->AppendPredefinedValue(strPredefinedName, strCode, strDescription);
	}

	void SetPredefinedValue(const ibGuid& predefinedGuid, const wxString& strPredefinedName,
		const wxString& strCode, const wxString& strDescription) {
		m_tableModelStore->SetPredefinedValue(predefinedGuid,
			strPredefinedName, strCode, strDescription);
	}

	void DeletePredefinedValue(const ibGuid& predefinedGuid) { m_tableModelStore->DeletePredefinedValue(predefinedGuid); }

	//find predefined value
	wxObjectDataPtr<ibPredefinedValueObject> FindPredefinedValue(const ibGuid& predefinedGuid) const { return m_tableModelStore->FindPredefinedValue(predefinedGuid); }
	wxObjectDataPtr<ibPredefinedValueObject> FindPredefinedValue(const wxString& predefinedName) const { return m_tableModelStore->FindPredefinedValue(predefinedName); }

protected:

	void OnContextMenu(ibDataViewEvent& event);
	void OnCommandMenu(wxCommandEvent& event);
	void OnItemActivated(ibDataViewEvent& event);

private:

	wxAuiToolBar* m_toolbarTableEditor;

	wxAuiToolBarItem* m_toolAdd;
	wxAuiToolBarItem* m_toolCopy;
	wxAuiToolBarItem* m_toolEdit;
	wxAuiToolBarItem* m_toolDelete;

	ibDataViewCtrl* m_tableEditor;

	ibDataViewPredefinedTreeStore* m_tableModelStore;
};

#endif