#include "advpropEventTool.h"

#include "backend/propertyManager/property/eventAction.h"
#include "backend/propertyManager/property/variant/actionVariant.h"
#include "backend/propertyManager/propertyEditor.h"

// -----------------------------------------------------------------------
// wxEventToolProperty
// -----------------------------------------------------------------------

wxPG_IMPLEMENT_PROPERTY_CLASS(wxEventToolProperty, wxStringProperty, ComboBoxAndButton)

wxEventToolProperty::wxEventToolProperty(const wxString& label, const wxString& strName,
	const wxPGChoices& choices,
	const wxVariant& value) : wxPGProperty(label, strName)
{
	wxVariantDataAction* dataAction = property_cast(value, wxVariantDataAction);
	wxASSERT(dataAction);

	SetChoices(choices);

	const CActionDescription& actionDesc = dataAction->GetValueAsActionDesc();
	if (actionDesc.GetSystemAction() != wxNOT_FOUND) {
		for (unsigned int i = 0; i < m_choices.GetCount(); i++) {
			const int val = m_choices.GetValue(i);
			if (actionDesc.GetSystemAction() == val) {
				m_flags &= ~(wxPG_PROP_ACTIVE_BTN);
				m_actionData.SetNumber(val);
				SetValue(value);
				return;
			}
		}
		m_actionData.SetString(value);
		m_flags |= wxPG_PROP_ACTIVE_BTN; // Property button always enabled.
		SetValue(value);
	}
	else {
		m_actionData.SetString(value);
		m_flags |= wxPG_PROP_ACTIVE_BTN; // Property button always enabled.
		SetValue(value);
	}
}

wxEventToolProperty::~wxEventToolProperty()
{
}

wxString wxEventToolProperty::ValueToString(wxVariant& value, int argFlags) const
{
	wxVariantDataAction* dataAction = property_cast(value, wxVariantDataAction);
	wxASSERT(dataAction);
	const CActionDescription& actionDesc = dataAction->GetValueAsActionDesc();

	for (unsigned int i = 0; i < m_choices.GetCount(); i++) {
		const int sel_val = m_choices.GetValue(i);
		if (sel_val == actionDesc.GetSystemAction()) 
			return m_choices.GetLabel(i);		
	}

	return actionDesc.GetCustomAction();
}

bool wxEventToolProperty::StringToValue(wxVariant& variant,
	const wxString& text,
	int argFlags) const
{
	if (stringUtils::CheckCorrectName(text) > 0)
		return false;

	if (GetChildCount() && HasFlag(wxPG_PROP_COMPOSED_VALUE))
		return wxPGProperty::StringToValue(variant, text, argFlags);

	if (variant != text) {
		if (text.IsEmpty()) {
			m_actionData.SetString(text);
		}
		variant = new wxVariantDataAction(text);
		return true;
	}

	return false;
}

bool wxEventToolProperty::IntToValue(wxVariant& value, int number, int argFlags) const
{
	value = new wxVariantDataAction(m_choices.GetValue(number));
	m_actionData.SetNumber(
		m_choices.GetValue(number)
	);
	return true;
}

void wxEventToolProperty::OnSetValue()
{
	if (m_actionData.IsCustomAction()) {
		SetFlag(wxPG_PROP_ACTIVE_BTN); // Property button always enabled.
	}
	else {
		ClearFlag(wxPG_PROP_ACTIVE_BTN); // Property button always disabled.
	}
}

#include <wx/propgrid/advprops.h>

wxPGEditorDialogAdapter* wxEventToolProperty::GetEditorDialog() const
{
	class wxPGEditorEventDialogAdapter : public wxPGEditorDialogAdapter {
	public:
		virtual bool DoShowDialog(wxPropertyGrid* pg, wxPGProperty* prop) wxOVERRIDE
		{
			wxEventToolProperty* dlgProp = wxDynamicCast(prop, wxEventToolProperty);
			wxCHECK_MSG(dlgProp, false, "Function called for incompatible property");

			const wxString& strActionEvent = pg->GetUncommittedPropertyValue();		
			if (strActionEvent.IsEmpty()) {
				wxPGProperty* pgProp = pg->GetPropertyByName(wxT("name"));
				const wxString& strActionEvent =
					(pgProp ? pgProp->GetDisplayedString() : wxEmptyString) + wxT("_") + prop->GetName();
				SetValue(new wxVariantDataAction(strActionEvent));
				return true;
			}
		
			SetValue(new wxVariantDataAction(strActionEvent));
			return true;
		}
	};
	return new wxPGEditorEventDialogAdapter();
}