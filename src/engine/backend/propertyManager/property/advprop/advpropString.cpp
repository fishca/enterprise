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
