#ifndef __ADVPROP_EVENT_H__
#define __ADVPROP_EVENT_H__

#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>

// -----------------------------------------------------------------------
// wxEventProperty
// -----------------------------------------------------------------------

class wxEventProperty : public wxStringProperty {
public:

	wxEventProperty(const wxString& label = wxPG_LABEL,
		const wxString& name = wxPG_LABEL,
		const wxString& value = wxEmptyString) : wxStringProperty(label, name, value) {
		m_flags |= wxPG_PROP_ACTIVE_BTN; // Property button always enabled.
	}

	virtual wxString ValueToString(wxVariant& value, int argFlags = 0) const override;
	virtual bool StringToValue(wxVariant& variant,
		const wxString& text,
		int argFlags = 0) const override;

	virtual wxPGEditorDialogAdapter* GetEditorDialog() const override;

private:
	WX_PG_DECLARE_PROPERTY_CLASS(wxEventProperty);
};

#endif