#include "advpropString.h"

// -----------------------------------------------------------------------
// wxGeneralStringProperty
// -----------------------------------------------------------------------

wxPG_IMPLEMENT_PROPERTY_CLASS(wxGeneralStringProperty, wxStringProperty, TextCtrl)

wxString wxGeneralStringProperty::ValueToString(wxVariant& value, int argFlags) const
{
	wxString s = value.GetString();

	if (GetChildCount() && HasFlag(wxPG_PROP_COMPOSED_VALUE))
	{
		// Value stored in m_value is non-editable, non-full value
		if ((argFlags & wxPG_FULL_VALUE) ||
			(argFlags & wxPG_EDITABLE_VALUE) ||
			s.empty())
		{
			// Calling this under incorrect conditions will fail
			wxASSERT_MSG(argFlags & wxPG_VALUE_IS_CURRENT,
				wxS("Sorry, currently default wxPGProperty::ValueToString() ")
				wxS("implementation only works if value is m_value."));

			DoGenerateComposedValue(s, argFlags);
		}

		return s;
	}

	// If string is password and value is for visual purposes,
	// then return asterisks instead the actual string.
	if ((m_flags & wxPG_PROP_PASSWORD) && !(argFlags & (wxPG_FULL_VALUE | wxPG_EDITABLE_VALUE)))
		return wxString(wxS('*'), s.Length());

	return s;
}

bool wxGeneralStringProperty::StringToValue(wxVariant& variant,
	const wxString& text,
	int argFlags) const
{
	if (stringUtils::CheckCorrectName(text) >= 0) {
		wxMessageBox(wxT("You can enter only numbers, letters and the symbol \"_\""), wxT("Error entering value"));
		return false;
	}

	if (GetChildCount() && HasFlag(wxPG_PROP_COMPOSED_VALUE))
		return wxPGProperty::StringToValue(variant, text, argFlags);

	if (variant != text) {
		variant = text;
		return text.length() > 0;
	}

	return false;
}

// -----------------------------------------------------------------------
// wxTranslateStringProperty
// -----------------------------------------------------------------------

wxPG_IMPLEMENT_PROPERTY_CLASS(wxTranslateStringProperty, wxLongStringProperty, TextCtrlAndButton)

wxString wxTranslateStringProperty::ValueToString(wxVariant& value, int argFlags) const
{
	return CBackendLocalization::GetTranslateGetRawLocText(value.GetString());
}

bool wxTranslateStringProperty::StringToValue(wxVariant& variant, const wxString& text, int argFlags) const
{
	CBackendLocalizationEntryArray array;

	if (CBackendLocalization::CreateLocalizationArray(variant.GetString(), array)) {
		const wxString& strLangCode = CBackendLocalization::GetUserLanguage();
		const auto iterator = std::find_if(array.begin(), array.end(),
			[strLangCode](const CBackendLocalizationEntry& entry) {
				return stringUtils::CompareString(entry.m_code, strLangCode); });
		if (iterator == array.end()) {
			CBackendLocalizationEntry entry;
			entry.m_code = strLangCode;
			entry.m_data = text;
			array.emplace_back(entry);
		}
		else {
			iterator->m_data = text;
		}
		variant = CBackendLocalization::GetRawLocText(array);
		return true;
	}

	CBackendLocalizationEntry entry;
	entry.m_code = CBackendLocalization::GetUserLanguage();
	entry.m_data = text;
	array.emplace_back(entry);

	variant = CBackendLocalization::GetRawLocText(array);
	return true;
}

#include "backend/metadata.h"
#include "backend/metaCollection/metaLanguageObject.h"

bool wxTranslateStringProperty::DisplayEditorDialog(wxPropertyGrid* pg, wxVariant& value)
{
	class wxTranslateTextCtrl : public wxTextCtrl {
	public:
		wxTranslateTextCtrl() : wxTextCtrl() {}
		wxTranslateTextCtrl(wxWindow* parent, wxWindowID id,
			const wxString& value = wxEmptyString,
			const wxPoint& pos = wxDefaultPosition,
			const wxSize& size = wxDefaultSize,
			long style = 0,
			const wxValidator& validator = wxDefaultValidator,
			const wxString& name = wxASCII_STR(wxTextCtrlNameStr))
			:
			wxTextCtrl(parent, id, value, pos, size, style, validator, name)
		{
			InvalidateBestSize();
		}
	protected:
		virtual wxSize DoGetBestSize() const override {
			wxSize best_size = wxTextCtrl::DoGetBestSize();
			if (best_size.y > 38)
				best_size.y = 38;
			return best_size;
		}
	};

	wxASSERT_MSG(value.IsType(wxS("string")), "Function called for incompatible property");

	if (m_ownerProperty != nullptr) {

		std::map<CValueMetaObjectLanguage*, wxTextCtrl*> locArray;

		// launch editor dialog
		wxDialog* dlg = new wxDialog(pg->GetPanel(), wxID_ANY,
			m_dlgTitle.empty() ? GetLabel() : m_dlgTitle,
			wxDefaultPosition, wxDefaultSize, m_dlgStyle);

		dlg->SetFont(pg->GetFont()); // To allow entering chars of the same set as the propGrid

		// Multi-line text editor dialog.
		const int spacing = wxPropertyGrid::IsSmallScreen() ? 4 : 8;
		wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
		long edStyle = wxTE_MULTILINE;
		if (HasFlag(wxPG_PROP_READONLY))
			edStyle |= wxTE_READONLY;

		IMetaData* metaData = m_ownerProperty->GetMetaData();
		if (metaData != nullptr) {

			wxBoxSizer* rowsizer = new wxBoxSizer(wxVERTICAL);

			CBackendLocalizationEntryArray array;
			CBackendLocalization::CreateLocalizationArray(
				m_value.GetString(), array);

			IMetaData* owner = nullptr;
			metaData->GetOwner(owner);
			if (owner == nullptr) { owner = metaData; }

			auto arrayLanguage = owner->GetAnyArrayObject<CValueMetaObjectLanguage>(g_metaLanguageCLSID);
			for (const auto language : arrayLanguage) {

				auto iterator = std::find_if(array.begin(), array.end(),
					[language](const CBackendLocalizationEntry& entry) {
						return stringUtils::CompareString(entry.m_code, language->GetLangCode()); });

				const wxString& strTranslate =
					iterator != array.end() ? iterator->m_data : wxEmptyString;

				if (arrayLanguage.size() > 1) {

					wxStaticText* ss = new wxStaticText(dlg, wxID_ANY, language->GetSynonym(),
						wxDefaultPosition, wxDefaultSize);

					wxTextCtrl* ed = new wxTranslateTextCtrl(dlg, language->GetMetaID(), strTranslate,
						wxDefaultPosition, wxDefaultSize, edStyle);

					if (m_maxLen > 0)
						ed->SetMaxLength(m_maxLen);

					rowsizer->Add(ss, wxSizerFlags(0).Border(wxLEFT | wxRIGHT, spacing));
					rowsizer->Add(ed, wxSizerFlags(1).Expand().Border(wxALL, spacing));

					locArray.emplace(language, ed);
				}
				else {
					wxTextCtrl* ed = new wxTranslateTextCtrl(dlg, language->GetMetaID(), strTranslate,
						wxDefaultPosition, wxDefaultSize, edStyle);

					if (m_maxLen > 0)
						ed->SetMaxLength(m_maxLen);

					rowsizer->Add(ed, wxSizerFlags(1).Expand().Border(wxALL, spacing));
					locArray.emplace(language, ed);
				}
			}

			topsizer->Add(rowsizer, wxSizerFlags(1).Expand());
		}

		long btnSizerFlags = wxCANCEL;
		if (!HasFlag(wxPG_PROP_READONLY))
			btnSizerFlags |= wxOK;
		wxStdDialogButtonSizer* buttonSizer = dlg->CreateStdDialogButtonSizer(btnSizerFlags);
		topsizer->Add(buttonSizer, wxSizerFlags(0).Right().Border(wxBOTTOM | wxRIGHT, spacing));

		dlg->SetSizer(topsizer);
		topsizer->SetSizeHints(dlg);

		if (!wxPropertyGrid::IsSmallScreen()) {
			dlg->SetSize(400, 300);
			dlg->Move(pg->GetGoodEditorDialogPosition(this, dlg->GetSize()));
		}

		const int res = dlg->ShowModal();

		if (res == wxID_OK) {
			wxString strLocalization;
			for (const auto pair : locArray) {
				const auto ml = pair.first;	const auto ed = pair.second;
				strLocalization += wxString::Format(wxT("%s = '%s';"),
					ml->GetLangCode(),
					ed->GetValue()
				);
			}
			value = strLocalization;

			dlg->Destroy();
			return true;
		}

		dlg->Destroy();
		return false;
	}

	return false;
}

// -----------------------------------------------------------------------
// wxMultilineStringProperty
// -----------------------------------------------------------------------

wxPG_IMPLEMENT_PROPERTY_CLASS(wxMultilineStringProperty, wxLongStringProperty, TextCtrlAndButton)

bool wxMultilineStringProperty::DisplayEditorDialog(wxPropertyGrid* pg, wxVariant& value)
{
	wxASSERT_MSG(value.IsType(wxS("string")), "Function called for incompatible property");

	// launch editor dialog
	wxDialog* dlg = new wxDialog(pg->GetPanel(), wxID_ANY,
		m_dlgTitle.empty() ? GetLabel() : m_dlgTitle,
		wxDefaultPosition, wxDefaultSize, m_dlgStyle);

	dlg->SetFont(pg->GetFont()); // To allow entering chars of the same set as the propGrid

	// Multi-line text editor dialog.
	const int spacing = wxPropertyGrid::IsSmallScreen() ? 4 : 8;
	wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* rowsizer = new wxBoxSizer(wxHORIZONTAL);
	long edStyle = wxTE_MULTILINE;
	if (HasFlag(wxPG_PROP_READONLY))
		edStyle |= wxTE_READONLY;
	wxTextCtrl* ed = new wxTextCtrl(dlg, wxID_ANY, value.GetString(),
		wxDefaultPosition, wxDefaultSize, edStyle);
	if (m_maxLen > 0)
		ed->SetMaxLength(m_maxLen);

	rowsizer->Add(ed, wxSizerFlags(1).Expand().Border(wxALL, spacing));
	topsizer->Add(rowsizer, wxSizerFlags(1).Expand());

	long btnSizerFlags = wxCANCEL;
	if (!HasFlag(wxPG_PROP_READONLY))
		btnSizerFlags |= wxOK;
	wxStdDialogButtonSizer* buttonSizer = dlg->CreateStdDialogButtonSizer(btnSizerFlags);
	topsizer->Add(buttonSizer, wxSizerFlags(0).Right().Border(wxBOTTOM | wxRIGHT, spacing));

	dlg->SetSizer(topsizer);
	topsizer->SetSizeHints(dlg);

	if (!wxPropertyGrid::IsSmallScreen())
	{
		dlg->SetSize(400, 300);
		dlg->Move(pg->GetGoodEditorDialogPosition(this, dlg->GetSize()));
	}

	int res = dlg->ShowModal();

	if (res == wxID_OK)
	{
		value = ed->GetValue();
		dlg->Destroy();
		return true;
	}

	dlg->Destroy();
	return false;
}
