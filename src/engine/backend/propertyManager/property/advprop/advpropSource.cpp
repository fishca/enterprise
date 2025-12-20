#include "advpropSource.h"

#include "backend/propertyManager/property/variant/variantSource.h"
#include "backend/system/value/valueTable.h"

#define icon_size 16

// -----------------------------------------------------------------------
// wxPGSourceDataProperty
// -----------------------------------------------------------------------

wxPG_IMPLEMENT_PROPERTY_CLASS(wxPGSourceDataProperty, wxPGProperty, TextCtrlAndButton)

wxPGSourceDataProperty::wxPGSourceDataProperty(IPropertyObject* property, const wxString& label, const wxString& strName,
	const wxVariant& value) : wxPGProperty(label, strName)
{
	wxVariantDataSource* dataSource = property_cast(value, wxVariantDataSource);
	wxASSERT(dataSource);

	IBackendTypeSourceFactory* typeFactory = dynamic_cast<IBackendTypeSourceFactory*>(property);
	wxASSERT(typeFactory);
	m_typeSelector = new wxPGTypeProperty(property, typeFactory != nullptr ? typeFactory->GetFilterDataType() : eSelectorDataType::eSelectorDataType_reference, _("Type"), wxT("type"), dataSource->CloneSourceAttribute());
	AddPrivateChild(m_typeSelector);

	//m_flags |= wxPG_PROP_READONLY;
	m_flags |= wxPG_PROP_ACTIVE_BTN; // Property button always enabled.

	SetValue(value);
}

wxString wxPGSourceDataProperty::ValueToString(wxVariant& variant,
	int WXUNUSED(argFlags)) const
{
	return variant.GetString();
}

bool wxPGSourceDataProperty::StringToValue(wxVariant& variant, const wxString& text, int argFlags) const
{
	if (text.IsEmpty()) {
		wxVariantDataSource* dataSource = property_cast(variant, wxVariantDataSource);
		if (dataSource != nullptr) {
			dataSource->SetSource(wxNOT_FOUND);
			return true;
		}
		return false;
	}
	return text.IsEmpty();
}

void wxPGSourceDataProperty::RefreshChildren()
{
	const wxVariantDataSource* dataSource = property_cast(m_value, wxVariantDataSource);
	if (dataSource != nullptr) {
		const meta_identifier_t& id = dataSource->GetSource();
		if (id != wxNOT_FOUND) m_typeSelector->SetValue(dataSource->CloneSourceAttribute(id));
		else m_typeSelector->SetValue(dataSource->CloneSourceAttribute());
	}
	m_typeSelector->SetFlagRecursively(wxPG_PROP_READONLY, dataSource != nullptr ? !dataSource->IsPropAllowed() : false);
	wxPGSourceDataProperty::SetExpanded(true);
}

wxVariant wxPGSourceDataProperty::ChildChanged(wxVariant& thisValue, int childIndex, wxVariant& childValue) const
{
	wxVariantDataSource* dataSource = property_cast(thisValue, wxVariantDataSource);
	if (dataSource != nullptr && childIndex == 0) {
		wxVariantDataAttributeSource* attrSource = property_cast(childValue, wxVariantDataAttributeSource);
		if (attrSource != nullptr) {
			wxVariantDataSource* cloneDataSource = dataSource->Clone();
			if (!m_typeSelector->HasFlag(wxPG_PROP_READONLY)) cloneDataSource->SetSource(wxNOT_FOUND);
			cloneDataSource->SetSourceAttribute(attrSource);
			return cloneDataSource;
		}
	}
	return wxVariant();
}

#include <wx/treectrl.h>

#include "backend/metaData.h"
#include "backend/objCtor.h"

wxPGEditorDialogAdapter* wxPGSourceDataProperty::GetEditorDialog() const
{
	enum {
		icon_attribute = 0,
		icon_table
	};

	class wxPGSourceEventAdapter : public wxPGEditorDialogAdapter {

		wxString MakeTypeString(const IMetaData* metaData, const CTypeDescription& typeDesc) const {
			wxString strDescr;
			for (auto clsid : typeDesc.GetClsidList()) {
				if (metaData->IsRegisterCtor(clsid) && strDescr.IsEmpty()) {
					strDescr = metaData->GetNameObjectFromID(clsid);
				}
				else if (metaData->IsRegisterCtor(clsid)) {
					strDescr = strDescr + wxT(", ") + metaData->GetNameObjectFromID(clsid);
				}
			}
			if (strDescr.IsEmpty()) return wxT("<empty>");
			return strDescr;
		}

		class wxTreeItemSourceData : public wxTreeItemData {
			const wxString m_nameProp;
			const meta_identifier_t m_id;
			const bool m_tableSection;
		public:
			wxTreeItemSourceData(const wxString& nameProp, const meta_identifier_t& id, bool tableSection) : wxTreeItemData(), m_nameProp(nameProp), m_id(id), m_tableSection(tableSection) {};

			const wxString& GetPropName() const { return m_nameProp; }
			const meta_identifier_t& GetID() const { return m_id; }
			const bool IsTableSection() const { return m_tableSection; }
		};

		wxImageList* GetSourceImageList() const {
			wxImageList* list = new wxImageList(icon_size, icon_size);
			list->Add(CValue::GetIconGroup());
			list->Add(CValueTable::GetIconGroup());
			return list;
		}

		bool ProcessAttribute(wxPropertyGrid* pg, wxPGProperty* dlgProp, IBackendTypeSourceFactory* typeFactory, wxVariantDataSource* srcData) {

			const meta_identifier_t& dataSource = srcData != nullptr ? srcData->GetSource() : wxNOT_FOUND;

			IMetaData* metaData = typeFactory->GetMetaData();
			if (metaData == nullptr) return false;

			// launch editor dialog
			wxDialog* dlg = new wxDialog(pg, wxID_ANY, _("Choice source"), wxDefaultPosition, wxDefaultSize,
				wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxCLIP_CHILDREN);

			dlg->SetFont(pg->GetFont()); // To allow entering chars of the same set as the propGrid

			// Multi-line text editor dialog.
			const int spacing = wxPropertyGrid::IsSmallScreen() ? 4 : 8;

			wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
			wxBoxSizer* rowsizer = new wxBoxSizer(wxHORIZONTAL);

			const bool is_tableBox = typeFactory->GetFilterSourceDataType() == eSourceDataType::eSourceDataVariant_table;

			wxTreeCtrl* tc = new wxTreeCtrl(dlg, wxID_ANY,
				wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_NO_LINES | wxSUNKEN_BORDER | wxTR_TWIST_BUTTONS);

			// Make an state image list containing small icons
			tc->SetImageList(GetSourceImageList());

			rowsizer->Add(tc, wxSizerFlags(1).Expand().Border(wxALL, spacing));
			topsizer->Add(rowsizer, wxSizerFlags(1).Expand());

			tc->SetDoubleBuffered(true);
			tc->Enable(!dlgProp->HasFlag(wxPG_PROP_READONLY));

			wxStdDialogButtonSizer* buttonSizer = dlg->CreateStdDialogButtonSizer(wxOK | wxCANCEL);
			topsizer->Add(buttonSizer, wxSizerFlags(0).Right().Border(wxBOTTOM | wxRIGHT, spacing));

			dlg->SetSizer(topsizer);
			topsizer->SetSizeHints(dlg);

			if (!wxPropertyGrid::IsSmallScreen()) {
				dlg->SetSize(400, 300);
				dlg->Move(pg->GetGoodEditorDialogPosition(dlgProp, dlg->GetSize()));
			}

			tc->SetFocus();

			const ISourceDataObject* srcObject = dynamic_cast<ISourceDataObject*>(typeFactory->GetSourceObject());
			if (srcObject != nullptr) {

				bool allow_create = true;

				const IMetaValueTypeCtor* typeCtor = metaData->GetTypeCtor(srcObject->GetSourceClassType());
				if (typeCtor != nullptr) {
					if (typeFactory->GetFilterSourceDataType() == eSourceDataType::eSourceDataVariant_attribute) {
						bool is_list_source = typeCtor->GetMetaTypeCtor() == eCtorMetaType::eCtorMetaType_List;
						allow_create = !is_list_source;
					}
				}

				const CSourceExplorer& srcExplorer = srcObject->GetSourceExplorer();
				wxTreeItemSourceData* srcItemData = nullptr;
				if (typeFactory->FilterSource(srcExplorer, srcExplorer.GetSourceId())) {
					if (srcExplorer.IsSelect()) {
						srcItemData = new wxTreeItemSourceData(
							srcExplorer.GetSourceName() + wxT(" (") + MakeTypeString(typeFactory->GetMetaData(), srcExplorer.GetClsidList()) + wxT(")"), srcExplorer.GetSourceId(), srcExplorer.IsTableSection()
						);
					}
				}
				wxTreeItemId rootItem = nullptr;
				if (srcExplorer.IsTableSection()) {
					rootItem = tc->AddRoot(srcExplorer.GetSourceName() + wxT(" (") + MakeTypeString(typeFactory->GetMetaData(), srcExplorer.GetClsidList()) + wxT(")"), icon_table, icon_table, srcItemData);
				}
				else {
					rootItem = tc->AddRoot(srcExplorer.GetSourceName() + wxT(" (") + MakeTypeString(typeFactory->GetMetaData(), srcExplorer.GetClsidList()) + wxT(")"), icon_attribute, icon_attribute, srcItemData);
				}

				if (allow_create) {

					if (srcExplorer.GetSourceId() == dataSource) {
						tc->SelectItem(rootItem);
					}
					for (unsigned int idx = 0; idx < srcExplorer.GetHelperCount(); idx++) {
						const CSourceExplorer& srcNextExplorer = srcExplorer.GetHelper(idx);
						wxTreeItemSourceData* itemData = nullptr;
						if (typeFactory->FilterSource(srcNextExplorer, srcExplorer.GetSourceId())) {
							if (srcNextExplorer.IsSelect()) {
								itemData = new wxTreeItemSourceData(
									srcNextExplorer.GetSourceName() + wxT(" (") + MakeTypeString(typeFactory->GetMetaData(), srcNextExplorer.GetClsidList()) + wxT(")"),
									srcNextExplorer.GetSourceId(),
									srcNextExplorer.IsTableSection()
								);
							}
						}
						wxTreeItemId newItem = nullptr;
						if (srcNextExplorer.IsTableSection()) {
							newItem = tc->AppendItem(rootItem, srcNextExplorer.GetSourceName() + wxT(" (") + MakeTypeString(typeFactory->GetMetaData(), srcNextExplorer.GetClsidList()) + wxT(")"), icon_table, icon_table, itemData);
						}
						else {
							newItem = tc->AppendItem(rootItem, srcNextExplorer.GetSourceName() + wxT(" (") + MakeTypeString(typeFactory->GetMetaData(), srcNextExplorer.GetClsidList()) + wxT(")"), icon_attribute, icon_attribute, itemData);
						}
						if (srcNextExplorer.GetSourceId() == dataSource) {
							tc->SelectItem(newItem);
						}
						bool needDelete = itemData == nullptr;
						if (srcNextExplorer.IsTableSection()) {
							tc->SetItemBold(newItem);
							for (unsigned int i = 0; i < srcNextExplorer.GetHelperCount(); i++) {
								CSourceExplorer srcColExplorer = srcNextExplorer.GetHelper(i);
								wxTreeItemSourceData* itemTableData = nullptr;
								if (typeFactory->FilterSource(srcColExplorer, srcNextExplorer.GetSourceId())) {
									itemTableData = new wxTreeItemSourceData(
										srcColExplorer.GetSourceName() + wxT(" (") + MakeTypeString(typeFactory->GetMetaData(), srcColExplorer.GetClsidList()) + wxT(")"),
										srcColExplorer.GetSourceId(),
										srcColExplorer.IsTableSection()
									);
									wxTreeItemId nextItem = tc->AppendItem(newItem, srcColExplorer.GetSourceName() + wxT(" (") + MakeTypeString(typeFactory->GetMetaData(), srcColExplorer.GetClsidList()) + wxT(")"), icon_attribute, icon_attribute, itemTableData);
									if (srcColExplorer.GetSourceId() == dataSource) {
										tc->SelectItem(nextItem);
									}
									needDelete = !typeFactory->FilterSource(srcNextExplorer, srcNextExplorer.GetSourceId());
								}
							}
						}

						if (needDelete) {
							tc->Delete(newItem);
						}
					}

					if (srcExplorer.IsTableSection()) {
						tc->SetItemBold(rootItem);
					}

					tc->Expand(rootItem);
				}

			}

			tc->ExpandAll(); int res = dlg->ShowModal();

			wxTreeItemId selItem = tc->GetSelection();
			if (selItem.IsOk()) {
				wxTreeItemData* dataItem = tc->GetItemData(selItem);
				if (dataItem != nullptr && res == wxID_OK) {
					wxTreeItemSourceData* item = dynamic_cast<wxTreeItemSourceData*>(dataItem);
					wxASSERT(item);
					if (is_tableBox == item->IsTableSection()) {
						SetValue(new wxVariantDataSource(typeFactory, item->GetID()));
					}
					else {
						dlg->Destroy();
						return false;
					}
				}
				else if (!dataItem) {
					dlg->Destroy();
					return false;
				}
			}

			dlg->Destroy();
			return res == wxID_OK
				&& selItem.IsOk();
		}

		bool ProcessTableColumn(wxPropertyGrid* pg, wxPGProperty* dlgProp, IBackendTypeSourceFactory* typeFactory, wxVariantDataSource* srcData) {

			const meta_identifier_t& dataSource = srcData != nullptr ? srcData->GetSource() : wxNOT_FOUND;

			IMetaData* metaData = typeFactory->GetMetaData();
			if (metaData == nullptr) return false;
			const ISourceObject* typeSrc = typeFactory->GetSourceObject();
			if (typeSrc == nullptr) return false;

			const class_identifier_t& clsid = typeSrc->GetSourceClassType();
			if (clsid == 0) return false;

			// launch editor dialog
			wxDialog* dlg = new wxDialog(pg, wxID_ANY, _("Choice source"), wxDefaultPosition, wxDefaultSize,
				wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxCLIP_CHILDREN);

			dlg->SetFont(pg->GetFont()); // To allow entering chars of the same set as the propGrid

			// Multi-line text editor dialog.
			const int spacing = wxPropertyGrid::IsSmallScreen() ? 4 : 8;

			wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
			wxBoxSizer* rowsizer = new wxBoxSizer(wxHORIZONTAL);

			wxTreeCtrl* tc = new wxTreeCtrl(dlg, wxID_ANY,
				wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_NO_LINES | wxSUNKEN_BORDER);

			tc->SetImageList(GetSourceImageList());

			rowsizer->Add(tc, wxSizerFlags(1).Expand().Border(wxALL, spacing));
			topsizer->Add(rowsizer, wxSizerFlags(1).Expand());

			tc->SetDoubleBuffered(true);
			tc->Enable(!dlgProp->HasFlag(wxPG_PROP_READONLY));

			wxStdDialogButtonSizer* buttonSizer = dlg->CreateStdDialogButtonSizer(wxOK | wxCANCEL);
			topsizer->Add(buttonSizer, wxSizerFlags(0).Right().Border(wxBOTTOM | wxRIGHT, spacing));

			dlg->SetSizer(topsizer);
			topsizer->SetSizeHints(dlg);

			if (!wxPropertyGrid::IsSmallScreen()) {
				dlg->SetSize(400, 300);
				dlg->Move(pg->GetGoodEditorDialogPosition(dlgProp, dlg->GetSize()));
			}

			tc->SetFocus();

			const IMetaValueTypeCtor* typeCtor = metaData->GetTypeCtor(clsid);

			if (typeCtor != nullptr) {
				const IMetaObjectCompositeData* metaObject = nullptr;
				if (typeCtor->ConvertToMetaValue(metaObject)) {
					wxTreeItemSourceData* srcItemData = new wxTreeItemSourceData(metaObject->GetName() + wxT(" (") + MakeTypeString(metaData, clsid) + wxT(")"), wxNOT_FOUND, true);
					const wxTreeItemId& rootItem = tc->AddRoot(metaObject->GetName() + wxT(" (") + MakeTypeString(metaData, clsid) + wxT(")"), icon_table, icon_table, srcItemData);
					for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
						if (!object->IsAllowed())
							continue;
						wxTreeItemSourceData* itemData = itemData = new wxTreeItemSourceData(
							object->GetName() + wxT(" (") + MakeTypeString(metaData, object->GetTypeDesc()) + wxT(")"),
							object->GetMetaID(),
							false
						);
						const wxTreeItemId& newItem = tc->AppendItem(rootItem, object->GetName() + wxT(" (") + MakeTypeString(metaData, object->GetTypeDesc()) + wxT(")"), icon_attribute, icon_attribute, itemData);
						if (dataSource == object->GetMetaID()) tc->SelectItem(newItem);
					}
				}
			}

			tc->ExpandAll(); int res = dlg->ShowModal();

			wxTreeItemId selItem = tc->GetSelection();
			if (selItem.IsOk()) {
				wxTreeItemData* dataItem = tc->GetItemData(selItem);
				if (dataItem
					&& res == wxID_OK) {
					wxTreeItemSourceData* item = dynamic_cast<wxTreeItemSourceData*>(dataItem);
					wxASSERT(item);
					SetValue(new wxVariantDataSource(typeFactory, item->GetID()));
				}
				else if (dataItem == nullptr) {
					dlg->Destroy();
					return false;
				}
			}

			dlg->Destroy();
			return res == wxID_OK
				&& selItem.IsOk();
		}

	public:

		virtual bool DoShowDialog(wxPropertyGrid* pg, wxPGProperty* prop) wxOVERRIDE
		{
			wxPGSourceDataProperty* dlgProp = wxDynamicCast(prop, wxPGSourceDataProperty);
			wxCHECK_MSG(dlgProp, false, "Function called for incompatible property");

			IBackendTypeSourceFactory* typeFactory = dynamic_cast<IBackendTypeSourceFactory*>(dlgProp->GetPropertyObject());
			if (typeFactory == nullptr) return false;

			wxVariantDataSource* srcData = property_cast(dlgProp->GetValue(), wxVariantDataSource);
			if (typeFactory->GetFilterSourceDataType() == eSourceDataType::eSourceDataVariant_attribute || typeFactory->GetFilterSourceDataType() == eSourceDataType::eSourceDataVariant_table)
				return ProcessAttribute(pg, dlgProp, typeFactory, srcData);
			else if (typeFactory->GetFilterSourceDataType() == eSourceDataType::eSourceDataVariant_tableColumn)
				return ProcessTableColumn(pg, dlgProp, typeFactory, srcData);
			return false;
		}
	};

	return new wxPGSourceEventAdapter();
}