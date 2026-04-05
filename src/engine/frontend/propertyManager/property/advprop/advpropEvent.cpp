#include "advpropEvent.h"

#include "backend/propertyManager/property/eventControl.h"
#include "backend/stringUtils.h"

#include "frontend/propertyManager/property/private/prop.h"

// -----------------------------------------------------------------------
// wxEventProperty
// -----------------------------------------------------------------------

wxPG_IMPLEMENT_PROPERTY_CLASS(wxEventProperty, wxStringProperty, TextCtrlAndButton)

// register frontend property 
class ibPropertyEventLoader
{
public:
	ibPropertyEventLoader()
	{
		ibPG_IMPLEMENT_PROPERTY_CALLBACK(wxEventProperty, ibEventControl::ms_propertyEvent);
	}
} g_eventLoader;

wxEventProperty::wxEventProperty(const wxString& label, const wxString& name, const wxString& value)
	: wxStringProperty(label, name, value)
{
	m_flags |= wxPGPropertyFlags_ActiveButton; // Property button always enabled.
}

wxString wxEventProperty::ValueToString(wxVariant& value,
	wxPGPropValFormatFlags flags) const
{
	wxString s = value.GetString();

	if (HasAnyChild() && HasFlag(wxPGFlags::ComposedValue))
	{
		// Value stored in m_value is non-editable, non-full value
		if (!!(flags & wxPGPropValFormatFlags::FullValue) ||
			!!(flags & wxPGPropValFormatFlags::EditableValue) ||
			s.empty())
		{
			// Calling this under incorrect conditions will fail
			wxASSERT_MSG(!!(flags & wxPGPropValFormatFlags::ValueIsCurrent),
				wxS("Sorry, currently default wxPGProperty::ValueToString() ")
				wxS("implementation only works if value is m_value."));

			DoGenerateComposedValue(s, flags);
		}

		return s;
	}

	// If string is password and value is for visual purposes,
	// then return asterisks instead the actual string.
	if (!!(m_flags & wxPGPropertyFlags_Password) && !(flags & (wxPGPropValFormatFlags::FullValue | wxPGPropValFormatFlags::EditableValue)))
		return wxString(wxS('*'), s.length());

	return s;
}

bool wxEventProperty::StringToValue(wxVariant& variant,
	const wxString& text,
	wxPGPropValFormatFlags flags) const
{
	if (stringUtils::CheckCorrectName(text) > 0)
		return false;

	if (GetChildCount() && HasFlag(wxPGFlags::ComposedValue))
		return wxPGProperty::StringToValue(variant, text, flags);

	if (variant != text)
	{
		variant = text;
		return true;
	}

	return false;
}

#include <wx/propgrid/advprops.h>

wxPGEditorDialogAdapter* wxEventProperty::GetEditorDialog() const
{
	class wxPGRecordEventAdapter : public wxPGEditorDialogAdapter {
	public:

		virtual bool DoShowDialog(wxPropertyGrid* pg, wxPGProperty* prop) wxOVERRIDE
		{
			wxEventProperty* dlgProp = wxDynamicCast(prop, wxEventProperty);
			wxCHECK_MSG(dlgProp, false, "Function called for incompatible property");
			const wxString& eventName = pg->GetUncommittedPropertyValue();
			if (eventName.IsEmpty()) {

				wxPGProperty* pgProp = pg->GetPropertyByName(wxT("Name"));
				prop->SetValueFromString(
					(pgProp ? pgProp->GetDisplayedString() : wxEmptyString) + prop->GetName());

				SetValue(prop->GetValue());
			}
			else {
				SetValue(eventName);
			}

			return true;
		}
	};

	return new wxPGRecordEventAdapter();
}