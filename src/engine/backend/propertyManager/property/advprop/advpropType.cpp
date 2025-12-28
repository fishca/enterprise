#include "advpropType.h"

#include "backend/propertyManager/property/propertyType.h"
#include "backend/propertyManager/property/variant/variantType.h"
#include "backend/propertyManager/propertyEditor.h"

#define icon_size 16

// -----------------------------------------------------------------------
// wxPGTypeProperty
// -----------------------------------------------------------------------

wxPG_IMPLEMENT_PROPERTY_CLASS(wxPGTypeProperty, wxStringProperty, ComboBoxAndButton)

wxPGChoices wxPGTypeProperty::GetDateTime()
{
	wxPGChoices choices;
	choices.Add(_("Date"), eDateFractions::eDateFractions_Date);
	choices.Add(_("Date and time"), eDateFractions::eDateFractions_DateTime);
	choices.Add(_("Time"), eDateFractions::eDateFractions_Time);
	return choices;
}

#include "backend/metadata.h"
#include "backend/objCtor.h"

void wxPGTypeProperty::FillByClsid(const eSelectorDataType& selectorDataType, const class_identifier_t& clsid)
{
	const IAbstractTypeCtor* so = CValue::GetAvailableCtor(clsid);
	wxASSERT(so);
	if (so->GetObjectTypeCtor() == eCtorObjectType::eCtorObjectType_object_metadata) {
		const IMetaData* metaData = dynamic_cast<const IBackendTypeConfigFactory*>(m_ownerProperty)->GetMetaData();
		wxASSERT(metaData);
		if (metaData != nullptr) {
			if (selectorDataType == eSelectorDataType::eSelectorDataType_reference) {
				for (auto so : metaData->GetListCtorsByType(clsid, eCtorMetaType::eCtorMetaType_Reference)) {
					auto metaObject = so->GetMetaObject();
					auto choice = m_choices.Add(so->GetClassName(), metaObject->GetIcon());
					m_valChoices.insert_or_assign(
						choice.GetValue(), so->GetClassType()
					);
				}
			}
			else if (selectorDataType == eSelectorDataType::eSelectorDataType_table) {
				for (auto so : metaData->GetListCtorsByType(clsid, eCtorMetaType::eCtorMetaType_List)) {
					auto metaObject = so->GetMetaObject();
					auto choice = m_choices.Add(so->GetClassName(), metaObject->GetIcon());
					m_valChoices.insert_or_assign(
						choice.GetValue(), so->GetClassType()
					);
				}
			}
			else if (selectorDataType == eSelectorDataType::eSelectorDataType_any) {
				for (auto so : metaData->GetListCtorsByType(clsid, eCtorMetaType::eCtorMetaType_Object)) {
					auto metaObject = so->GetMetaObject();
					auto choice = m_choices.Add(so->GetClassName(), metaObject->GetIcon());
					m_valChoices.insert_or_assign(
						choice.GetValue(), so->GetClassType()
					);
				}
				for (auto so : metaData->GetListCtorsByType(clsid, eCtorMetaType::eCtorMetaType_Reference)) {
					auto metaObject = so->GetMetaObject();
					auto choice = m_choices.Add(so->GetClassName(), metaObject->GetIcon());
					m_valChoices.insert_or_assign(
						choice.GetValue(), so->GetClassType()
					);
				}
				for (auto so : metaData->GetListCtorsByType(clsid, eCtorMetaType::eCtorMetaType_RecordManager)) {
					auto metaObject = so->GetMetaObject();
					auto choice = m_choices.Add(so->GetClassName(), metaObject->GetIcon());
					m_valChoices.insert_or_assign(
						choice.GetValue(), so->GetClassType()
					);
				}
			}
		}
	}
	else {
		auto choice = m_choices.Add(so->GetClassName(), so->GetClassIcon());
		m_valChoices.insert_or_assign(
			choice.GetValue(), so->GetClassType()
		);
	}
}

#include "backend/system/value/valueTable.h"

wxPGTypeProperty::wxPGTypeProperty(const IPropertyObject* property, const eSelectorDataType& selectorDataType, const wxString& label, const wxString& strName, const wxVariant& value) :
	wxPGProperty(label, strName), m_ownerProperty(property)
{
	m_precision = new wxUIntProperty(_("Precision"), wxT("precision"), 0);
	AddPrivateChild(m_precision);
	m_scale = new wxUIntProperty(_("Scale"), wxT("scale"), 0);
	AddPrivateChild(m_scale);
	m_date_time = new wxEnumProperty(_("Date time"), wxT("date_time"), GetDateTime(), eDateFractions::eDateFractions_Date);
	AddPrivateChild(m_date_time);
	m_length = new wxUIntProperty(_("Length"), wxT("length"), 0);
	AddPrivateChild(m_length);

	if (selectorDataType == eSelectorDataType::eSelectorDataType_any) {
		FillByClsid(selectorDataType, CValue::GetIDByVT(eValueTypes::TYPE_EMPTY));
	}

	if (selectorDataType == eSelectorDataType::eSelectorDataType_any || selectorDataType == eSelectorDataType::eSelectorDataType_boolean || selectorDataType == eSelectorDataType::eSelectorDataType_reference) {
		if (selectorDataType == eSelectorDataType::eSelectorDataType_boolean) {
			FillByClsid(selectorDataType, CValue::GetIDByVT(eValueTypes::TYPE_BOOLEAN));
			FillByClsid(selectorDataType, CValue::GetIDByVT(eValueTypes::TYPE_NUMBER));
		}
		else {
			FillByClsid(selectorDataType, CValue::GetIDByVT(eValueTypes::TYPE_BOOLEAN));
			FillByClsid(selectorDataType, CValue::GetIDByVT(eValueTypes::TYPE_NUMBER));
			FillByClsid(selectorDataType, CValue::GetIDByVT(eValueTypes::TYPE_DATE));
			FillByClsid(selectorDataType, CValue::GetIDByVT(eValueTypes::TYPE_STRING));
		}
	}
	else if (selectorDataType == eSelectorDataType::eSelectorDataType_resource) {
		FillByClsid(selectorDataType, CValue::GetIDByVT(eValueTypes::TYPE_NUMBER));
	}

	if (selectorDataType == eSelectorDataType::eSelectorDataType_any) {
		FillByClsid(selectorDataType, CValue::GetIDByVT(eValueTypes::TYPE_NULL));
	}

	/////////////////////////////////////////////////

	if (selectorDataType == eSelectorDataType::eSelectorDataType_table) {
		FillByClsid(selectorDataType, g_valueTableCLSID);
	}

	/////////////////////////////////////////////////

	FillByClsid(selectorDataType, g_metaCatalogCLSID);
	FillByClsid(selectorDataType, g_metaDocumentCLSID);
	FillByClsid(selectorDataType, g_metaEnumerationCLSID);

	if (selectorDataType == eSelectorDataType::eSelectorDataType_any) {
		FillByClsid(selectorDataType, g_metaDataProcessorCLSID);
		FillByClsid(selectorDataType, g_metaReportCLSID);
	}

	if (selectorDataType == eSelectorDataType::eSelectorDataType_table) {
		FillByClsid(selectorDataType, g_metaInformationRegisterCLSID);
		FillByClsid(selectorDataType, g_metaAccumulationRegisterCLSID);
	}

	SetValue(value);

	//m_flags |= wxPG_PROP_READONLY;
	m_flags |= wxPG_PROP_ACTIVE_BTN;
}

bool wxPGTypeProperty::IntToValue(wxVariant& value, int number, int argFlags) const
{
	wxVariantDataAttribute* dataType = property_cast(value, wxVariantDataAttribute);
	if (dataType != nullptr) {
		wxVariantDataAttribute* newType = dataType->Clone();
		wxASSERT(newType);
		CTypeDescription& td = newType->GetTypeDesc();
		td.SetDefaultMetaType(m_valChoices.at(number));
		value = newType;
		return true;
	}
	return false;
}

wxVariant wxPGTypeProperty::ChildChanged(wxVariant& thisValue, int childIndex, wxVariant& childValue) const
{
	wxVariantDataAttribute* dataType = property_cast(thisValue, wxVariantDataAttribute);
	if (dataType != nullptr) {
		wxVariantDataAttribute* newType = dataType->Clone();
		wxASSERT(newType);
		CTypeDescription& td = newType->GetTypeDesc();
		if (childIndex == 0 || childIndex == 1) {
			long precision = (childIndex == 0)
				? childValue : m_precision->GetValue(),
				scale = (childIndex == 1)
				? childValue : m_scale->GetValue();
			if (precision > MAX_PRECISION_NUMBER) {
				precision = m_precision->GetValue();
				scale = m_scale->GetValue();
			}
			else if (precision == 0 || precision < scale) {
				precision = m_precision->GetValue();
				scale = m_scale->GetValue();
			}
			td.SetNumber(precision, scale);
		}
		else if (childIndex == 2) {
			long dateTime = childValue;
			td.SetDate((eDateFractions)dateTime);
		}
		else if (childIndex == 3) {
			long length = childValue;
			if (length > MAX_LENGTH_STRING) {
				length = m_length->GetValue();
			}
			td.SetString(length);
		}
		return newType;
	}

	return wxNullVariant;
}

void wxPGTypeProperty::RefreshChildren()
{
	wxVariantDataAttribute* varData = property_cast(m_value, wxVariantDataAttribute);

	if (varData != nullptr) {
		const CTypeDescription& td = varData->GetTypeDesc();
		if (td.GetClsidCount() < 2) {
			eValueTypes id = CValue::GetVTByID(td.GetFirstClsid());
			if (id == eValueTypes::TYPE_NUMBER) {
				m_precision->Hide(false);
				m_precision->SetExpanded(true);
				m_scale->Hide(false);
				m_scale->SetExpanded(true);
				m_date_time->Hide(true);
				m_date_time->SetExpanded(false);
				m_length->Hide(true);
				m_length->SetExpanded(false);
			}
			else if (id == eValueTypes::TYPE_DATE) {
				m_precision->Hide(true);
				m_precision->SetExpanded(false);
				m_scale->Hide(true);
				m_scale->SetExpanded(false);
				m_date_time->Hide(false);
				m_precision->SetExpanded(true);
				m_length->Hide(true);
				m_length->SetExpanded(false);
			}
			else if (id == eValueTypes::TYPE_STRING) {
				m_precision->Hide(true);
				m_precision->SetExpanded(false);
				m_scale->Hide(true);
				m_scale->SetExpanded(false);
				m_date_time->Hide(true);
				m_date_time->SetExpanded(false);
				m_length->Hide(false);
				m_length->SetExpanded(true);
			}
			else {
				m_precision->Hide(true);
				m_precision->SetExpanded(false);
				m_scale->Hide(true);
				m_scale->SetExpanded(false);
				m_date_time->Hide(true);
				m_date_time->SetExpanded(false);
				m_length->Hide(true);
				m_length->SetExpanded(false);
			}
		}
		else {
			m_precision->Hide(true);
			m_precision->SetExpanded(false);
			m_scale->Hide(true);
			m_scale->SetExpanded(false);
			m_date_time->Hide(true);
			m_date_time->SetExpanded(false);
			m_length->Hide(true);
			m_length->SetExpanded(false);
		}

		for (unsigned int idx = 0; idx < td.GetClsidCount(); idx++) {
			eValueTypes id = CValue::GetVTByID(td.GetByIdx(idx));
			if (id == eValueTypes::TYPE_NUMBER) {
				m_precision->SetValue(td.GetPrecision());
				m_scale->SetValue(td.GetScale());
			}
			else if (id == eValueTypes::TYPE_DATE) {
				m_date_time->SetValue(td.GetDateFraction());
			}
			else if (id == eValueTypes::TYPE_STRING) {
				m_length->SetValue(td.GetLength());
			}
		}
	}
	else {
		m_precision->Hide(true);
		m_precision->SetExpanded(false);
		m_scale->Hide(true);
		m_scale->SetExpanded(false);
		m_date_time->Hide(true);
		m_date_time->SetExpanded(false);
		m_length->Hide(true);
		m_length->SetExpanded(false);
	}

	wxPGTypeProperty::SetExpanded(true);
}

#include <wx/spinctrl.h>

#include "backend/propertyManager/property/advprop/ctrl/checktree.h"

wxPGEditorDialogAdapter* wxPGTypeProperty::GetEditorDialog() const
{
	class wxPGEditorTypeDialogAdapter : public wxPGEditorDialogAdapter {

		class wxTreeItemOptionData : public wxTreeItemData {
			const IAbstractTypeCtor* m_typeCtor;
		public:
			wxTreeItemOptionData(const IAbstractTypeCtor* typeCtor) : wxTreeItemData(), m_typeCtor(typeCtor) {}
			class_identifier_t GetClassType() const { return m_typeCtor->GetClassType(); }
			const IAbstractTypeCtor* GetTypeCtor() const { return m_typeCtor; }
		};

		void FillByClsid(const class_identifier_t& clsid,
			wxPropertyCheckTree* tc, wxVariantDataAttribute* data, bool allowEdit) {

			wxImageList* imageList = tc->GetImageList();
			wxASSERT(imageList);
			const IAbstractTypeCtor* so = CValue::GetAvailableCtor(clsid);
			const int groupIcon = imageList->Add(so->GetClassIcon());

			wxTreeItemOptionData* itemData = new wxTreeItemOptionData(so);
			wxTreeItemId newItem = tc->AppendItem(tc->GetRootItem(), so->GetClassName(),
				groupIcon, groupIcon,
				itemData);

			if (data != nullptr) {
				const CTypeDescription& td = data->GetTypeDesc();
				tc->SetItemState(newItem, td.ContainType(so->GetClassType()) ? allowEdit ? wxPropertyCheckTree::CHECKED : wxPropertyCheckTree::CHECKED_DISABLED : allowEdit ? wxPropertyCheckTree::UNCHECKED : wxPropertyCheckTree::UNCHECKED_DISABLED);
				tc->Check(newItem, td.ContainType(so->GetClassType()));
			}
			else {
				tc->SetItemState(newItem, allowEdit ? wxPropertyCheckTree::UNCHECKED : wxPropertyCheckTree::UNCHECKED_DISABLED);
				tc->Check(newItem, false);
			}
		}

		void FillByClsid(IMetaData* metaData, const class_identifier_t& clsid,
			wxPropertyCheckTree* tc, wxVariantDataAttribute* data, bool allowEdit) {

			wxImageList* imageList = tc->GetImageList();
			wxASSERT(imageList);
			if (metaData != nullptr && metaData->IsRegisterCtor(clsid)) {
				const IAbstractTypeCtor* so = metaData ? metaData->GetAvailableCtor(clsid) : CValue::GetAvailableCtor(clsid);
				const int groupIcon = imageList->Add(so->GetClassIcon());

				wxTreeItemOptionData* itemData = new wxTreeItemOptionData(so);
				wxTreeItemId newItem = tc->AppendItem(tc->GetRootItem(), so->GetClassName(),
					groupIcon, groupIcon,
					itemData);

				if (data != nullptr) {
					const CTypeDescription& td = data->GetTypeDesc();
					tc->SetItemState(newItem, td.ContainType(so->GetClassType()) ? allowEdit ? wxPropertyCheckTree::CHECKED : wxPropertyCheckTree::CHECKED_DISABLED : allowEdit ? wxPropertyCheckTree::UNCHECKED : wxPropertyCheckTree::UNCHECKED_DISABLED);
					tc->Check(newItem, td.ContainType(so->GetClassType()));
				}
				else {
					tc->SetItemState(newItem, allowEdit ? wxPropertyCheckTree::UNCHECKED : wxPropertyCheckTree::UNCHECKED_DISABLED);
					tc->Check(newItem, false);
				}
			}
		}

		void FillByClsid(eSelectorDataType selectorDataType, IMetaData* metaData, const class_identifier_t& clsid,
			wxPropertyCheckTree* tc, wxVariantDataAttribute* data, bool allowEdit) {

			wxImageList* imageList = tc->GetImageList();
			wxASSERT(imageList);
			if (metaData != nullptr && metaData->IsRegisterCtor(clsid)) {
				const IAbstractTypeCtor* so = CValue::GetAvailableCtor(clsid);
				if (selectorDataType == eSelectorDataType::eSelectorDataType_reference) {

					const int groupIcon = imageList->Add(so->GetClassIcon());
					const wxTreeItemId& parentID = tc->AppendItem(tc->GetRootItem(), so->GetClassName() + wxT("Ref"),
						groupIcon, groupIcon);

					for (auto so : metaData->GetListCtorsByType(clsid, eCtorMetaType::eCtorMetaType_Reference)) {
						IMetaObjectRecordDataRef* registerData = dynamic_cast<IMetaObjectRecordDataRef*>(so->GetMetaObject());
						{
							int icon = imageList->Add(registerData->GetIcon());
							wxTreeItemOptionData* itemData = new wxTreeItemOptionData(so);
							wxTreeItemId newItem = tc->AppendItem(parentID, registerData->GetName(),
								icon, icon,
								itemData);

							if (data != nullptr) {
								const CTypeDescription& td = data->GetTypeDesc();
								tc->SetItemState(newItem, td.ContainType(so->GetClassType()) ? allowEdit ? wxPropertyCheckTree::CHECKED : wxPropertyCheckTree::CHECKED_DISABLED : allowEdit ? wxPropertyCheckTree::UNCHECKED : wxPropertyCheckTree::UNCHECKED_DISABLED);
								tc->Check(newItem, td.ContainType(so->GetClassType()));
							}
							else {
								tc->SetItemState(newItem, allowEdit ? wxPropertyCheckTree::UNCHECKED : wxPropertyCheckTree::UNCHECKED_DISABLED);
								tc->Check(newItem, false);
							}
						}
					}
				}
				else if (selectorDataType == eSelectorDataType::eSelectorDataType_table) {

					const int groupIcon = imageList->Add(so->GetClassIcon());
					const wxTreeItemId& parentID = tc->AppendItem(tc->GetRootItem(), so->GetClassName() + wxT("List"),
						groupIcon, groupIcon);

					for (auto so : metaData->GetListCtorsByType(clsid, eCtorMetaType::eCtorMetaType_List)) {
						IMetaObjectGenericData* registerData = dynamic_cast<IMetaObjectGenericData*>(so->GetMetaObject());
						{
							int icon = imageList->Add(registerData->GetIcon());
							wxTreeItemOptionData* itemData = new wxTreeItemOptionData(so);
							wxTreeItemId newItem = tc->AppendItem(parentID, registerData->GetName(),
								icon, icon,
								itemData);

							if (data != nullptr) {
								const CTypeDescription& td = data->GetTypeDesc();
								tc->SetItemState(newItem, td.ContainType(so->GetClassType()) ? allowEdit ? wxPropertyCheckTree::CHECKED : wxPropertyCheckTree::CHECKED_DISABLED : allowEdit ? wxPropertyCheckTree::UNCHECKED : wxPropertyCheckTree::UNCHECKED_DISABLED);
								tc->Check(newItem, td.ContainType(so->GetClassType()));
							}
							else {
								tc->SetItemState(newItem, allowEdit ? wxPropertyCheckTree::UNCHECKED : wxPropertyCheckTree::UNCHECKED_DISABLED);
								tc->Check(newItem, false);
							}
						}
					}
				}
				else if (selectorDataType == eSelectorDataType::eSelectorDataType_any) {
					if (so->GetClassType() != g_metaEnumerationCLSID) {

						int groupIcon = imageList->Add(so->GetClassIcon());
						const wxTreeItemId& parentID = tc->AppendItem(tc->GetRootItem(), so->GetClassName() + wxT("Object"),
							groupIcon, groupIcon);

						for (auto so : metaData->GetListCtorsByType(clsid, eCtorMetaType::eCtorMetaType_Object)) {
							IMetaObjectRecordData* registerData = dynamic_cast<IMetaObjectRecordData*>(so->GetMetaObject());
							{
								int icon = imageList->Add(registerData->GetIcon());
								wxTreeItemOptionData* itemData = new wxTreeItemOptionData(so);
								wxTreeItemId newItem = tc->AppendItem(parentID, registerData->GetName(),
									icon, icon,
									itemData);

								if (data != nullptr) {
									const CTypeDescription& td = data->GetTypeDesc();
									tc->SetItemState(newItem, td.ContainType(so->GetClassType()) ? allowEdit ? wxPropertyCheckTree::CHECKED : wxPropertyCheckTree::CHECKED_DISABLED : allowEdit ? wxPropertyCheckTree::UNCHECKED : wxPropertyCheckTree::UNCHECKED_DISABLED);
									tc->Check(newItem, td.ContainType(so->GetClassType()));
								}
								else {
									tc->SetItemState(newItem, allowEdit ? wxPropertyCheckTree::UNCHECKED : wxPropertyCheckTree::UNCHECKED_DISABLED);
									tc->Check(newItem, false);
								}
							}
						}
					}
					if (so->GetClassType() != g_metaDataProcessorCLSID && so->GetClassType() != g_metaReportCLSID)
					{
						int groupIcon = imageList->Add(so->GetClassIcon());
						const wxTreeItemId& parentID = tc->AppendItem(tc->GetRootItem(), so->GetClassName() + wxT("Ref"),
							groupIcon, groupIcon);

						for (auto so : metaData->GetListCtorsByType(clsid, eCtorMetaType::eCtorMetaType_Reference)) {
							IMetaObjectRecordDataRef* registerData = dynamic_cast<IMetaObjectRecordDataRef*>(so->GetMetaObject());
							{
								int icon = imageList->Add(registerData->GetIcon());
								wxTreeItemOptionData* itemData = new wxTreeItemOptionData(so);
								wxTreeItemId newItem = tc->AppendItem(parentID, registerData->GetName(),
									icon, icon,
									itemData);

								if (data != nullptr) {
									const CTypeDescription& td = data->GetTypeDesc();
									tc->SetItemState(newItem, td.ContainType(so->GetClassType()) ? allowEdit ? wxPropertyCheckTree::CHECKED : wxPropertyCheckTree::CHECKED_DISABLED : allowEdit ? wxPropertyCheckTree::UNCHECKED : wxPropertyCheckTree::UNCHECKED_DISABLED);
									tc->Check(newItem, td.ContainType(so->GetClassType()));
								}
								else {
									tc->SetItemState(newItem, allowEdit ? wxPropertyCheckTree::UNCHECKED : wxPropertyCheckTree::UNCHECKED_DISABLED);
									tc->Check(newItem, false);
								}
							}
						}
					}
				}
			}
		}

	public:

		virtual bool DoShowDialog(wxPropertyGrid* pg, wxPGProperty* prop) wxOVERRIDE
		{
			wxPGTypeProperty* dlgProp = wxDynamicCast(prop, wxPGTypeProperty);
			wxCHECK_MSG(dlgProp, false, "Function called for incompatible property");

			const IBackendTypeConfigFactory* typeFactory = dynamic_cast<const IBackendTypeConfigFactory*>(dlgProp->GetPropertyObject());
			if (typeFactory == nullptr) return false;

			wxVariantDataAttribute* data = property_cast(dlgProp->GetValue(), wxVariantDataAttribute);
			if (data == nullptr) return false;

			const CTypeDescription& typeDesc = data->GetTypeDesc();

			// launch editor dialog
			wxDialog* dlg = new wxDialog(pg, wxID_ANY, _("Choice type"), wxDefaultPosition, wxDefaultSize,
				wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxCLIP_CHILDREN);

			dlg->SetFont(pg->GetFont()); // To allow entering chars of the same set as the propGrid

			// Multi-line text editor dialog.
			const int spacing = wxPropertyGrid::IsSmallScreen() ? 4 : 8;
			wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
			wxBoxSizer* rowsizer = new wxBoxSizer(wxHORIZONTAL);

			const eSelectorDataType& selectorDataType = typeFactory->GetFilterDataType();

			wxCheckBox* compositeDataType = new wxCheckBox(dlg, wxID_ANY,
				_("Composite data type"), wxDefaultPosition, wxDefaultSize);

			compositeDataType->Enable(!dlgProp->HasFlag(wxPG_PROP_READONLY));

			int style = wxPR_SINGLE_CHECK;

			if (selectorDataType == eSelectorDataType::eSelectorDataType_any ||
				selectorDataType == eSelectorDataType::eSelectorDataType_reference) {
				style = wxPR_MULTIPLE_CHECK;
			}

			if (data != nullptr) {
				const CTypeDescription& typeDesc = data->GetTypeDesc();
				style = typeDesc.GetClsidCount() > 1 ?
					wxPR_MULTIPLE_CHECK : wxPR_SINGLE_CHECK;
				compositeDataType->SetValue(
					typeDesc.GetClsidCount() > 1);
			}
			else {
				compositeDataType->SetValue(style != wxPR_SINGLE_CHECK);
			}

			compositeDataType->Show(selectorDataType != eSelectorDataType::eSelectorDataType_table);

			wxPropertyCheckTree* tc = new wxPropertyCheckTree(dlg, wxID_ANY,
				wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_NO_LINES | wxTR_HIDE_ROOT | style | wxSUNKEN_BORDER | wxTR_TWIST_BUTTONS);

			topsizer->Add(compositeDataType, wxSizerFlags(0).Border(wxALL, spacing));
			rowsizer->Add(tc, wxSizerFlags(1).Expand().Border(wxALL, spacing));
			topsizer->Add(rowsizer, wxSizerFlags(1).Expand());

			wxBoxSizer* stringSizer = new wxBoxSizer(wxHORIZONTAL);
			wxStaticText* stSLength = new wxStaticText(dlg, wxID_ANY, _("Length:"), wxDefaultPosition, wxDefaultSize);
			stSLength->Wrap(-1);
			stringSizer->Add(stSLength, 0, wxALL, 5);

			wxSpinCtrl* tcSLength = new wxSpinCtrl(dlg, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, MAX_LENGTH_STRING);
			stringSizer->Add(tcSLength, 0, wxBOTTOM | wxRIGHT, 0);
			tcSLength->SetValue(typeDesc.GetLength());

			tcSLength->Bind(wxEVT_COMMAND_SPINCTRL_UPDATED,
				[data](wxSpinEvent& event)
				{
					CTypeDescription& typeDesc = data->GetTypeDesc();
					typeDesc.SetString(event.GetValue());
					event.Skip();
				}
			);

			tcSLength->Enable(!dlgProp->HasFlag(wxPG_PROP_READONLY));
			topsizer->Add(stringSizer, 0, 0, 5);

			wxBoxSizer* dateSizer = new wxBoxSizer(wxHORIZONTAL);
			wxStaticText* stDDateFormat = new wxStaticText(dlg, wxID_ANY, _("Date format:"), wxDefaultPosition, wxDefaultSize);
			stDDateFormat->Wrap(-1);
			dateSizer->Add(stDDateFormat, 0, wxALL, 5);

			wxArrayString cDDateFormatChoices; auto ch = dlgProp->GetDateTime();
			for (unsigned int idx = 0; idx < ch.GetCount(); idx++) {
				cDDateFormatChoices.Add(ch.GetLabel(idx));
			}
			wxChoice* cDDateFormat = new wxChoice(dlg, wxID_ANY, wxDefaultPosition, wxDefaultSize, cDDateFormatChoices);
			cDDateFormat->SetSelection(typeDesc.GetDateFraction());
			dateSizer->Add(cDDateFormat, 0, wxALL, 0);

			cDDateFormat->Bind(wxEVT_COMMAND_CHOICE_SELECTED,
				[data](wxCommandEvent& event)
				{
					CTypeDescription& typeDesc = data->GetTypeDesc();
					typeDesc.SetDate((eDateFractions)event.GetSelection());
					event.Skip();
				}
			);

			cDDateFormat->Enable(!dlgProp->HasFlag(wxPG_PROP_READONLY));
			topsizer->Add(dateSizer, 0, 0, 5);

			wxBoxSizer* numberSizer = new wxBoxSizer(wxHORIZONTAL);
			wxStaticText* stNLength = new wxStaticText(dlg, wxID_ANY, _("Length:"), wxDefaultPosition, wxDefaultSize);
			stNLength->Wrap(-1);
			numberSizer->Add(stNLength, 0, wxALL, 5);

			wxSpinCtrl* tcNLength = new wxSpinCtrl(dlg, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, MAX_PRECISION_NUMBER);
			numberSizer->Add(tcNLength, 0, wxBOTTOM | wxRIGHT, 0);
			tcNLength->SetValue(typeDesc.GetPrecision());

			tcNLength->Bind(wxEVT_COMMAND_SPINCTRL_UPDATED,
				[data](wxSpinEvent& event)
				{
					CTypeDescription& typeDesc = data->GetTypeDesc();
					typeDesc.SetNumber(event.GetValue(), typeDesc.GetScale());
					event.Skip();
				}
			);

			tcNLength->Enable(!dlgProp->HasFlag(wxPG_PROP_READONLY));

			wxStaticText* stNScale = new wxStaticText(dlg, wxID_ANY, _("Scale:"), wxDefaultPosition, wxDefaultSize);
			stNScale->Wrap(-1);
			numberSizer->Add(stNScale, 0, wxTOP | wxBOTTOM | wxLEFT, 5);

			wxSpinCtrl* tcNScale = new wxSpinCtrl(dlg, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS);
			numberSizer->Add(tcNScale, 0, wxRIGHT | wxLEFT, 5);
			tcNScale->SetValue(typeDesc.GetScale());

			tcNScale->Bind(wxEVT_COMMAND_SPINCTRL_UPDATED,
				[data](wxSpinEvent& event)
				{
					CTypeDescription& typeDesc = data->GetTypeDesc();
					unsigned short scale = typeDesc.GetPrecision() > event.GetValue() ?
						event.GetValue() : typeDesc.GetPrecision();

					typeDesc.SetNumber(typeDesc.GetPrecision(), scale);
					event.Skip();
				}
			);

			tcNScale->Enable(!dlgProp->HasFlag(wxPG_PROP_READONLY));
			topsizer->Add(numberSizer, 0, 0, 5);

			tc->SetDoubleBuffered(true);

			wxStdDialogButtonSizer* buttonSizer = dlg->CreateStdDialogButtonSizer(wxOK | wxCANCEL);
			topsizer->Add(buttonSizer, wxSizerFlags(0).Right().Border(wxBOTTOM | wxRIGHT, spacing));

			compositeDataType->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED,
				[tc](wxCommandEvent& event)
				{
					tc->SetWindowStyle(
						event.IsChecked() ?
						wxPR_MULTIPLE_CHECK :
						wxPR_SINGLE_CHECK
					);
					event.Skip();
				}
			);

			for (unsigned int i = 0; i < numberSizer->GetItemCount(); i++) { numberSizer->Hide(i); }
			for (unsigned int i = 0; i < dateSizer->GetItemCount(); i++) { dateSizer->Hide(i); }
			for (unsigned int i = 0; i < stringSizer->GetItemCount(); i++) { stringSizer->Hide(i); }

			tc->Bind(wxEVT_COMMAND_TREE_SEL_CHANGED,
				[tc, numberSizer, dateSizer, stringSizer, topsizer](wxTreeEvent& event)
				{
					wxTreeItemOptionData* item = dynamic_cast<wxTreeItemOptionData*>(tc->GetItemData(event.GetItem()));
					if (item != nullptr) {
						const class_identifier_t& clsid = item->GetClassType();
						for (unsigned int i = 0; i < numberSizer->GetItemCount(); i++) {
							numberSizer->Show(i, CValue::GetVTByID(clsid) == eValueTypes::TYPE_NUMBER);
						}
						for (unsigned int i = 0; i < dateSizer->GetItemCount(); i++) {
							dateSizer->Show(i, CValue::GetVTByID(clsid) == eValueTypes::TYPE_DATE);
						}
						for (unsigned int i = 0; i < stringSizer->GetItemCount(); i++) {
							stringSizer->Show(i, CValue::GetVTByID(clsid) == eValueTypes::TYPE_STRING);
						}
						topsizer->Layout();
						tc->Layout();
					}
					event.Skip();
				}
			);

			const wxTreeItemId& rootItem = tc->AddRoot(wxEmptyString);

			dlg->SetSizer(topsizer);
			topsizer->SetSizeHints(dlg);

			if (!wxPropertyGrid::IsSmallScreen()) {
				dlg->SetSize(400, 300);
				dlg->Move(pg->GetGoodEditorDialogPosition(prop, dlg->GetSize()));
			}

			tc->SetFocus();

			// Make an state image list containing small icons
			tc->SetImageList(
				new wxImageList(icon_size, icon_size)
			);

			if (selectorDataType == eSelectorDataType::eSelectorDataType_any) {
				FillByClsid(CValue::GetIDByVT(eValueTypes::TYPE_EMPTY), tc, data, !dlgProp->HasFlag(wxPG_PROP_READONLY));
			}

			if (selectorDataType == eSelectorDataType::eSelectorDataType_any
				|| selectorDataType == eSelectorDataType::eSelectorDataType_reference) {
				FillByClsid(CValue::GetIDByVT(eValueTypes::TYPE_BOOLEAN), tc, data, !dlgProp->HasFlag(wxPG_PROP_READONLY));
				FillByClsid(CValue::GetIDByVT(eValueTypes::TYPE_NUMBER), tc, data, !dlgProp->HasFlag(wxPG_PROP_READONLY));
				FillByClsid(CValue::GetIDByVT(eValueTypes::TYPE_DATE), tc, data, !dlgProp->HasFlag(wxPG_PROP_READONLY));
				FillByClsid(CValue::GetIDByVT(eValueTypes::TYPE_STRING), tc, data, !dlgProp->HasFlag(wxPG_PROP_READONLY));
			}
			else if (selectorDataType == eSelectorDataType::eSelectorDataType_boolean) {
				FillByClsid(CValue::GetIDByVT(eValueTypes::TYPE_BOOLEAN), tc, data, !dlgProp->HasFlag(wxPG_PROP_READONLY));
				FillByClsid(CValue::GetIDByVT(eValueTypes::TYPE_NUMBER), tc, data, !dlgProp->HasFlag(wxPG_PROP_READONLY));
			}
			else if (selectorDataType == eSelectorDataType::eSelectorDataType_resource) {
				FillByClsid(CValue::GetIDByVT(eValueTypes::TYPE_NUMBER), tc, data, !dlgProp->HasFlag(wxPG_PROP_READONLY));
			}

			if (selectorDataType == eSelectorDataType::eSelectorDataType_any) {
				FillByClsid(CValue::GetIDByVT(eValueTypes::TYPE_NULL), tc, data, !dlgProp->HasFlag(wxPG_PROP_READONLY));
			}

			/////////////////////////////////////////////////
			if (selectorDataType == eSelectorDataType::eSelectorDataType_table) {
				FillByClsid(g_valueTableCLSID, tc, data, !dlgProp->HasFlag(wxPG_PROP_READONLY));
			}
			/////////////////////////////////////////////////

			IMetaData* metaData = typeFactory->GetMetaData();
			wxASSERT(metaData);
			if (metaData != nullptr) {
				FillByClsid(selectorDataType, metaData, g_metaCatalogCLSID, tc, data, !dlgProp->HasFlag(wxPG_PROP_READONLY));
				FillByClsid(selectorDataType, metaData, g_metaDocumentCLSID, tc, data, !dlgProp->HasFlag(wxPG_PROP_READONLY));
				FillByClsid(selectorDataType, metaData, g_metaEnumerationCLSID, tc, data, !dlgProp->HasFlag(wxPG_PROP_READONLY));
				if (selectorDataType == eSelectorDataType::eSelectorDataType_any) {
					FillByClsid(selectorDataType, metaData, g_metaDataProcessorCLSID, tc, data, !dlgProp->HasFlag(wxPG_PROP_READONLY));
					FillByClsid(selectorDataType, metaData, g_metaReportCLSID, tc, data, !dlgProp->HasFlag(wxPG_PROP_READONLY));
				}
				if (selectorDataType == eSelectorDataType::eSelectorDataType_table) {
					FillByClsid(selectorDataType, metaData, g_metaInformationRegisterCLSID, tc, data, !dlgProp->HasFlag(wxPG_PROP_READONLY));
					FillByClsid(selectorDataType, metaData, g_metaAccumulationRegisterCLSID, tc, data, !dlgProp->HasFlag(wxPG_PROP_READONLY));
				}
			}

			wxArrayTreeItemIds ids;
			unsigned int itemCount = tc->GetItems(ids);

			for (auto clsid : typeDesc.GetClsidList()) {
				bool allowType = true;
				for (const wxTreeItemId& selItem : ids) {
					if (selItem.IsOk()) {
						const wxTreeItemOptionData* item = dynamic_cast<wxTreeItemOptionData*>(tc->GetItemData(selItem));
						if (item != nullptr && clsid == item->GetClassType()) { allowType = false; break; }
					}
				}
				if (allowType) { FillByClsid(metaData, clsid, tc, data, !dlgProp->HasFlag(wxPG_PROP_READONLY)); }
			}
			tc->ExpandAll(); int res = dlg->ShowModal();
			if (res == wxID_OK) {
				wxVariantDataAttribute* clone = data->Clone();
				CTypeDescription& createdDesc = clone->GetTypeDesc();
				createdDesc.ClearMetaType();
				unsigned int selCount = tc->GetSelections(ids);
				for (const wxTreeItemId& selItem : ids) {
					if (selItem.IsOk()) {
						const wxTreeItemOptionData* item = dynamic_cast<wxTreeItemOptionData*>(tc->GetItemData(selItem));
						if (item != nullptr) createdDesc.AppendMetaType(item->GetClassType());
					}
				}
				createdDesc.SetTypeData(typeDesc.GetTypeData());
				SetValue(clone);
			}

			dlg->Destroy();
			return res == wxID_OK;
		}
	};

	return new wxPGEditorTypeDialogAdapter();
}