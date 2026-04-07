#ifndef __ADVPROP_EVENT_H__
#define __ADVPROP_EVENT_H__

#include <wx/propgrid/propgrid.h>

// -----------------------------------------------------------------------
// ibPGEventProperty
// -----------------------------------------------------------------------

class ibPGEventProperty : public wxStringProperty {
public:

	ibPGEventProperty(const wxString& label = wxPG_LABEL,
		const wxString& name = wxPG_LABEL,
		const wxString& value = wxEmptyString);

	virtual wxString ValueToString(wxVariant& value,
		wxPGPropValFormatFlags flags = wxPGPropValFormatFlags::Null) const override;

	virtual bool StringToValue(wxVariant& variant,
		const wxString& text,
		wxPGPropValFormatFlags flags = wxPGPropValFormatFlags::Null) const override;

	virtual wxPGEditorDialogAdapter* GetEditorDialog() const override;

private:
	WX_PG_DECLARE_PROPERTY_CLASS(ibPGEventProperty);
};

#endif