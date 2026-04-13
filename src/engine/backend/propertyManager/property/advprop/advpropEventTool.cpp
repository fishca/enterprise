#include "advpropEventTool.h"

#include "backend/propertyManager/property/eventAction.h"
#include "backend/propertyManager/property/variant/variantAction.h"
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

	m_choices.Assign(choices);
	m_value = wxPGVariant_Zero;
	
	const CActionDescription& actionDesc = dataAction->GetValueAsActionDesc();
	if (actionDesc.GetSystemAction() != wxNOT_FOUND) {
		for (unsigned int i = 0; i < m_choices.GetCount(); i++) {
			const int val = m_choices.GetValue(i);
			if (val == actionDesc.GetSystemAction()) {
				m_flags &= ~(wxPG_PROP_ACTIVE_BTN);
				m_valueBitmapBundle = m_choices.Item(i).GetBitmap();
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
		m_valueBitmapBundle = wxNullBitmap;
		SetFlag(wxPG_PROP_ACTIVE_BTN); // Property button always enabled.
	}
	else {
		m_valueBitmapBundle = wxNullBitmap;
		for (unsigned int i = 0; i < m_choices.GetCount(); i++) {
			const int val = m_choices.GetValue(i);
			if (val == m_actionData.GetNumber()) {
				m_valueBitmapBundle = m_choices.Item(i).GetBitmap();
				break;
			}
		}
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
				wxPGProperty* pgProp = pg->GetPropertyByName(wxT("Name"));
				const wxString& strActionEvent =
					(pgProp ? pgProp->GetDisplayedString() : wxEmptyString) + prop->GetName();
				SetValue(new wxVariantDataAction(strActionEvent));
				return true;
			}

			SetValue(new wxVariantDataAction(strActionEvent));
			return true;
		}
	};
	return new wxPGEditorEventDialogAdapter();
}