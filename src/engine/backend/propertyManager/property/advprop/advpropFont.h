#ifndef __ADVPROP_FONT_H__
#define __ADVPROP_FONT_H__

#include <wx/propgrid/propgrid.h>
#include "backend/fontcontainer.h"

#include "backend/backend_core.h"

// -----------------------------------------------------------------------
// wxPGFontProperty
// -----------------------------------------------------------------------

class BACKEND_API wxPGFontProperty : public wxPGProperty {
public:

	wxPGFontProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL, const wxFontContainer& value = *wxNORMAL_FONT);
	virtual ~wxPGFontProperty() override;

	virtual wxVariant ChildChanged(wxVariant& thisValue, int childIndex,
		wxVariant& childValue) const override;

	virtual void RefreshChildren() override;

	virtual void OnSetValue() override;
	virtual wxString GetValueAsString(int argFlags = 0) const override;

	virtual bool OnEvent(wxPropertyGrid* propgrid, wxWindow* primary, wxEvent& event) override;

private:
	WX_PG_DECLARE_PROPERTY_CLASS(wxPGFontProperty);
};

#endif