#ifndef __ADVPROP_LIST_H__
#define __ADVPROP_LIST_H__

#include <wx/propgrid/propgrid.h>
#include "backend/backend_type.h"

// -----------------------------------------------------------------------
// wxPGListProperty
// -----------------------------------------------------------------------

class BACKEND_API wxPGListProperty : public wxPGProperty {

public:
	wxPGListProperty(const wxString& label = wxPG_LABEL,
		const wxString& name = wxPG_LABEL, wxPGChoices& choices = wxPGChoices(), int value = 0);

	virtual wxString ValueToString(wxVariant& value, int argFlags = 0) const override;
	virtual bool StringToValue(wxVariant& variant,
		const wxString& text,
		int argFlags = 0) const override;

	virtual bool IntToValue(wxVariant& value,
		int number,
		int argFlags = 0) const override;

	virtual wxPGEditorDialogAdapter* GetEditorDialog() const override;

private:
	WX_PG_DECLARE_PROPERTY_CLASS(wxPGListProperty);
};

#endif