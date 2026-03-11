#include "tableView.h"
#include "backend/tableInfo.h"

wxIMPLEMENT_DYNAMIC_CLASS(wxTableViewCtrl, wxDataViewExtCtrl);

#include "backend/metadataConfiguration.h"

#include "frontend/win/ctrls/controlTextEditor.h"
#include "frontend/visualView/ctrl/typeControl.h"
#include "frontend/visualView/ctrl/form.h"

#include "backend/objCtor.h"

bool wxTableViewCtrl::ShowFilter(struct CFilterRow& filter)
{
	enum {
		eModelUse = 1,
		eModelName,
		eModelComparison,
		eModelValue
	};

	class wxFilterDialog : public wxDialog {

		class wxDataViewFilterModel : public wxDataViewExtVirtualListModel {
			CFilterRow m_filter;
		public:

			inline void CopyValue(wxVariant& variant, const CValue& cValue) const {
				variant = cValue.GetString();
			}

			void SetFilter(const CFilterRow& filter) {
				m_filter = filter;
				Reset(filter.m_filters.size());
			};

			CFilterRow& GetFilter() {
				return m_filter;
			};

			wxDataViewFilterModel() :
				wxDataViewExtVirtualListModel() {
			}

			virtual void GetValueByRow(wxVariant& variant,
				unsigned row, unsigned col) const {
				if (col == eModelUse) {
					variant = m_filter.m_filters[row].m_filterUse;
				}
				else if (col == eModelName) {
					variant = m_filter.m_filters[row].m_filterPresentation;
				}
				else if (col == eModelComparison) {
					variant = (long)m_filter.m_filters[row].m_filterComparison;
				}
				else if (col == eModelValue) {
					CopyValue(variant, m_filter.m_filters[row].m_filterValue);
				}
			};

			virtual bool SetValueByRow(const wxVariant& variant,
				unsigned row, unsigned col) {

				if (col == eModelUse) {
					m_filter.m_filters[row].m_filterUse = variant.GetBool();
					return true;
				}
				else if (col == eModelName) {
					m_filter.m_filters[row].m_filterName = variant.GetString();
					return true;
				}
				else if (col == eModelComparison) {
					m_filter.m_filters[row].m_filterComparison = (eComparisonType)variant.GetLong();
					return true;
				}
				else if (col == eModelValue) {
					const CValue& selValue = m_filter.m_filters[row].m_filterValue;
					const CValue& newValue = activeMetaData->CreateObject(selValue.GetClassType());
					const wxString& strData = variant.GetString();
					if (strData.Length() > 0) {
						std::vector<CValue> listValue;
						if (newValue.FindValue(strData, listValue)) {
							m_filter.m_filters[row].m_filterValue = CValueTypeDescription::AdjustValue(
								m_filter.m_filters[row].m_filterTypeDescription, listValue.at(0)
							);
						}
						else {
							return false;
						}
					}
					else {
						m_filter.m_filters[row].m_filterValue = CValueTypeDescription::AdjustValue(
							m_filter.m_filters[row].m_filterTypeDescription, newValue
						);
					}
					m_filter.m_filters[row].m_filterUse = true;
					return true;
				}
				return false;
			};
		};

		class wxValueViewRenderer : public wxDataViewExtCustomRenderer,
			public IControlFrame {
			wxFilterDialog* m_filterDialog;
		public:

			void FinishSelecting() {
				if (m_editorCtrl != nullptr) {
					// Remove our event handler first to prevent it from (recursively) calling
					// us again as it would do via a call to FinishEditing() when the editor
					// loses focus when we hide it below.
					wxEvtHandler* const handler = m_editorCtrl->PopEventHandler();

					// Hide the control immediately but don't delete it yet as there could be
					// some pending messages for it.
					m_editorCtrl->Hide();

					wxPendingDelete.Append(handler);
					wxPendingDelete.Append(m_editorCtrl);

					// Ensure that DestroyEditControl() is not called again for this control.
					m_editorCtrl.Release();
				}

				DoHandleEditingDone(nullptr);
			}

			// This renderer can be either activatable or editable, for demonstration
			// purposes. In real programs, you should select whether the user should be
			// able to activate or edit the cell and it doesn't make sense to switch
			// between the two -- but this is just an example, so it doesn't stop us.
			wxValueViewRenderer(wxFilterDialog* filterDialog)
				: wxDataViewExtCustomRenderer(wxT("string"), wxDATAVIEW_CELL_EDITABLE, wxALIGN_LEFT), m_filterDialog(filterDialog) {
			}

			virtual bool Render(wxRect rect, wxDC* dc, int state) override {
				RenderText(m_valueVariant,
					0, // no offset
					rect,
					dc,
					state);

				return true;
			}

			virtual bool ActivateCell(const wxRect& cell,
				wxDataViewExtModel* model,
				const wxDataViewExtItem& item,
				unsigned int col,
				const wxMouseEvent* mouseEvent) override {
				return false;
			}

			virtual wxSize GetSize() const override {
				if (!m_valueVariant.IsNull()) {
					return GetTextExtent(m_valueVariant);
				}
				else {
					return GetView()->FromDIP(wxSize(wxDVC_DEFAULT_RENDERER_SIZE,
						wxDVC_DEFAULT_RENDERER_SIZE));
				}
			}

			virtual bool SetValue(const wxVariant& value) override {
				m_valueVariant = value.GetString();
				return true;
			}

			virtual bool GetValue(wxVariant& WXUNUSED(value)) const override {
				return true;
			}

#if wxUSE_ACCESSIBILITY
			virtual wxString GetAccessibleDescription() const override {
				return m_valueVariant;
			}
#endif // wxUSE_ACCESSIBILITY

			virtual bool HasEditorCtrl() const override {
				return true;
			}

			virtual wxWindow* CreateEditorCtrl(wxWindow* dv,
				wxRect labelRect,
				const wxVariant& value) override {

				wxControlTextEditor* textEditor = new wxControlTextEditor;
				textEditor->SetDVCMode(true);

				// create the window hidden to prevent flicker
				textEditor->Show(false);

				bool result = textEditor->Create(dv, wxID_ANY, value,
					labelRect.GetPosition(),
					labelRect.GetSize());

				if (!result)
					return nullptr;

				textEditor->ShowSelectButton(true);
				textEditor->ShowClearButton(true);
				textEditor->ShowOpenButton(false);

				wxDataViewExtCtrl* parentWnd = dynamic_cast<wxDataViewExtCtrl*>(dv->GetParent());
				if (parentWnd != nullptr) {
					textEditor->SetBackgroundColour(parentWnd->GetBackgroundColour());
					textEditor->SetForegroundColour(parentWnd->GetForegroundColour());
					textEditor->SetFont(parentWnd->GetFont());
				}
				else {
					textEditor->SetBackgroundColour(dv->GetBackgroundColour());
					textEditor->SetForegroundColour(dv->GetForegroundColour());
					textEditor->SetFont(dv->GetFont());
				}

				textEditor->SetPasswordMode(false);
				textEditor->SetMultilineMode(false);
				textEditor->SetTextEditMode(true);

				textEditor->Bind(wxEVT_CONTROL_BUTTON_SELECT, &wxValueViewRenderer::OnSelectButtonPressed, this);
				textEditor->Bind(wxEVT_CONTROL_BUTTON_CLEAR, &wxValueViewRenderer::OnClearButtonPressed, this);

				textEditor->LayoutControls();
				textEditor->Show(true);

				textEditor->SetInsertionPointEnd();
				return textEditor;
			}

			virtual bool GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value) override {
				wxControlTextEditor* textEditor = wxDynamicCast(ctrl, wxControlTextEditor);
				if (textEditor == nullptr)
					return false;
				value = textEditor->GetValue();
				return true;
			}

			// Counter reference
			virtual void ControlIncrRef() {}
			virtual void ControlDecrRef() {}

		public:

			virtual bool GetControlValue(CValue& pvarControlVal) const {
				const wxDataViewExtItem& item = m_filterDialog->m_dataViewFilter->GetSelection();
				if (!item.IsOk())
					return false;
				size_t index = reinterpret_cast<size_t>(item.GetID());
				CFilterRow& filter = m_filterDialog->m_filterModel->GetFilter();
				CFilterRow::CFilterData& filterData = filter.m_filters[index - 1];
				pvarControlVal = filterData.m_filterValue;
				return true;
			}

			virtual CGuid GetControlGuid() const {
				const wxDataViewExtItem& item = m_filterDialog->m_dataViewFilter->GetSelection();
				if (!item.IsOk())
					return wxNullGuid;
				size_t index = reinterpret_cast<size_t>(item.GetID());
				CFilterRow& filter = m_filterDialog->m_filterModel->GetFilter();
				CFilterRow::CFilterData& filterData = filter.m_filters[index - 1];
				return filterData.m_filterGuid;
			}

			virtual bool SetControlValue(const CValue& varValue) const {
				const wxDataViewExtItem& item = m_filterDialog->m_dataViewFilter->GetSelection();
				if (!item.IsOk())
					return false;
				size_t index = reinterpret_cast<size_t>(item.GetID());
				CFilterRow& filter = m_filterDialog->m_filterModel->GetFilter();
				CFilterRow::CFilterData& filterData = filter.m_filters[index - 1];
				filterData.m_filterValue = varValue;
				return true;
			}

			virtual bool HasQuickChoice() const {
				CValue selValue; GetControlValue(selValue);
				const IAbstractTypeCtor* so = activeMetaData->GetAvailableCtor(selValue.GetClassType());
				if (so != nullptr && so->GetObjectTypeCtor() == eCtorObjectType_object_primitive) {
					return true;
				}
				else if (so != nullptr && so->GetObjectTypeCtor() == eCtorObjectType_object_enum) {
					return true;
				}
				else if (so != nullptr && so->GetObjectTypeCtor() == eCtorObjectType_object_meta_value) {
					const IMetaValueTypeCtor* meta_so = dynamic_cast<const IMetaValueTypeCtor*>(so);
					if (meta_so != nullptr) {
						IValueMetaObjectRecordDataRef* metaObject = dynamic_cast<IValueMetaObjectRecordDataRef*>(meta_so->GetMetaObject());
						if (metaObject != nullptr)
							return metaObject->HasQuickChoice();
					}
				}
				return false;
			}

		private:

			//events
			void OnSelectButtonPressed(wxCommandEvent& event) {
				const wxDataViewExtItem& item = m_filterDialog->m_dataViewFilter->GetSelection();
				if (!item.IsOk())
					return;
				size_t index = reinterpret_cast<size_t>(item.GetID());
				CFilterRow& filter = m_filterDialog->m_filterModel->GetFilter();
				CFilterRow::CFilterData& filterData = filter.m_filters[index - 1];
				CTypeDescription& typeDescription = filterData.m_filterTypeDescription;
				if (filterData.m_filterValue.GetType() == eValueTypes::TYPE_EMPTY) {
					const class_identifier_t& clsid = ITypeControlFactory::ShowSelectType(activeMetaData,
						filterData.m_filterTypeDescription
					);
					if (clsid != 0 && activeMetaData->IsRegisterCtor(clsid)) {
						filterData.m_filterValue = activeMetaData->CreateObject(clsid);
					}
					return;
				}
				const class_identifier_t& clsid = filterData.m_filterValue.GetClassType();
				if (!ITypeControlFactory::QuickChoice(this, clsid, GetEditorCtrl())) {
					const IMetaValueTypeCtor* singleValue = activeMetaData->GetTypeCtor(clsid);
					if (singleValue != nullptr) {
						IValueMetaObject* metaObject = singleValue->GetMetaObject();
						wxASSERT(metaObject);
						const IValueMetaObjectAttribute* attribute = activeMetaData->FindAnyObjectByFilter<IValueMetaObjectAttribute>(filterData.m_filterModel, true);

						eSelectMode selMode = eSelectMode::eSelectMode_Items;
						if (attribute != nullptr)
							selMode = attribute->GetSelectMode();
						metaObject->ProcessChoice(this, wxEmptyString, selMode);
					}
				}
			}

			void OnClearButtonPressed(wxCommandEvent& event) {
				const wxDataViewExtItem& item = m_filterDialog->m_dataViewFilter->GetSelection();
				if (!item.IsOk())
					return;
				size_t index = reinterpret_cast<size_t>(item.GetID());
				CFilterRow& filter = m_filterDialog->m_filterModel->GetFilter();
				CFilterRow::CFilterData& filterData = filter.m_filters[index - 1];
				filterData.m_filterValue = CValueTypeDescription::AdjustValue(filterData.m_filterTypeDescription);
				filterData.m_filterUse = true;
				FinishSelecting();
			}

			virtual void ChoiceProcessing(CValue& vSelected) {
				const wxDataViewExtItem& item = m_filterDialog->m_dataViewFilter->GetSelection();
				if (!item.IsOk())
					return;
				size_t index = reinterpret_cast<size_t>(item.GetID());
				CFilterRow& filter = m_filterDialog->m_filterModel->GetFilter();
				CFilterRow::CFilterData& filterData = filter.m_filters[index - 1];
				filterData.m_filterValue = vSelected;
				filterData.m_filterUse = true;
				FinishSelecting();
			}

		private:
			wxVariant m_valueVariant;
		};

	private:

		wxDataViewExtCtrl* m_dataViewFilter;
		wxDataViewExtColumn* m_dataViewColumnUse;
		wxDataViewExtColumn* m_dataViewColumnName;
		wxDataViewExtColumn* m_dataViewColumnComparison;
		wxDataViewExtColumn* m_dataViewColumnValue;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		wxDataViewFilterModel* m_filterModel;

	public:

		void SetFilter(const CFilterRow& filter) {
			m_filterModel->SetFilter(filter);
		}

		CFilterRow GetFilter() const {
			if (m_filterModel != nullptr)
				return m_filterModel->GetFilter();
			return CFilterRow();
		}

		wxFilterDialog::wxFilterDialog(wxWindow* parent, wxWindowID id,
			const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(600, 250), long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) :
			wxDialog(parent, id, _("Filter"), pos, size, style)
		{
			wxDialog::SetSizeHints(wxDefaultSize, wxDefaultSize);

			m_dataViewFilter = new wxDataViewExtCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_ROW_LINES | wxDV_SINGLE);

			m_dataViewFilter->SetBackgroundColour(parent->GetBackgroundColour());
			m_dataViewFilter->SetForegroundColour(parent->GetForegroundColour());

			m_dataViewFilter->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &wxFilterDialog::OnItemActivated, this);

			wxArrayString arr;
			arr.push_back("Equal");
			arr.push_back("Not equal");

			m_dataViewColumnUse = m_dataViewFilter->AppendToggleColumn(_("Use"), eModelUse, wxDATAVIEW_CELL_ACTIVATABLE, wxNOT_FOUND, wxAlignment::wxALIGN_LEFT);
			m_dataViewColumnName = m_dataViewFilter->AppendTextColumn(_("Name"), eModelName, wxDATAVIEW_CELL_INERT, wxNOT_FOUND, wxAlignment::wxALIGN_LEFT);

			m_dataViewColumnComparison = new wxDataViewExtColumn(_("Comparison"),
				new wxDataViewExtChoiceByIndexRenderer(arr, wxDATAVIEW_CELL_EDITABLE, wxAlignment::wxALIGN_LEFT),
				eModelComparison,
				wxNOT_FOUND,
				wxAlignment::wxALIGN_LEFT
			);
			m_dataViewFilter->AppendColumn(m_dataViewColumnComparison);

			m_dataViewColumnValue = new wxDataViewExtColumn(_("Value"),
				new wxValueViewRenderer(this),
				eModelValue,
				wxNOT_FOUND,
				wxAlignment::wxALIGN_LEFT
			);
			m_dataViewFilter->AppendColumn(m_dataViewColumnValue);

			m_sdbSizer = new wxStdDialogButtonSizer;
			m_sdbSizerOK = new wxButton(this, wxID_OK);
			m_sdbSizer->AddButton(m_sdbSizerOK);
			m_sdbSizerCancel = new wxButton(this, wxID_CANCEL);
			m_sdbSizer->AddButton(m_sdbSizerCancel);
			m_sdbSizer->Realize();

			wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
			mainSizer->Add(m_dataViewFilter, 1, wxALL | wxEXPAND, 5);
			mainSizer->Add(m_sdbSizer, 0, wxEXPAND, 5);
			wxDialog::SetSizer(mainSizer);
			wxDialog::Layout();
			mainSizer->Fit(this);

			m_filterModel = new wxDataViewFilterModel();
			m_dataViewFilter->AssociateModel(m_filterModel);

			wxIcon dlg_icon;
			dlg_icon.CopyFromBitmap(CBackendPicture::GetPicture(g_picFilterCLSID));

			wxDialog::SetIcon(dlg_icon);

			wxDialog::SetMinSize(size);
			wxDialog::Centre(wxBOTH);
		}

		virtual ~wxFilterDialog() {
			m_dataViewFilter->AssociateModel(nullptr);
			delete m_filterModel;
		}

		void OnItemActivated(wxDataViewExtEvent& event) {
			m_dataViewFilter->EditItem(event.GetItem(), event.GetDataViewColumn());
			event.Skip();
		}
	};
	wxFilterDialog* dialog =
		new wxFilterDialog(this, wxID_ANY);
	dialog->SetFilter(filter);
	bool result = false;
	if (dialog->ShowModal() == wxID_OK) {
		filter = dialog->GetFilter(); result = true;
	}
	dialog->Destroy();
	return result;
}
