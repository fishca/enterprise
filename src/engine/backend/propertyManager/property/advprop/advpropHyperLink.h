#ifndef __ADVPROP_HYPERLINK_H__
#define __ADVPROP_HYPERLINK_H__

#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>

#include "backend/backend_core.h"

class BACKEND_API IPropertyObject;

// -----------------------------------------------------------------------
// wxPGHyperLinkProperty
// -----------------------------------------------------------------------

class BACKEND_API wxPGHyperLinkProperty : public wxPGProperty {
	WX_PG_DECLARE_PROPERTY_CLASS(wxPGHyperLinkProperty)
public:

	wxPGHyperLinkProperty(IPropertyObject* property = nullptr, const wxString& label = wxPG_LABEL,
		const wxString& name = wxPG_LABEL, const wxVariant& value = wxNullVariant);
	virtual ~wxPGHyperLinkProperty();

	virtual wxString ValueToString(wxVariant& value, int argFlags = 0) const override;
	virtual bool StringToValue(wxVariant& variant,
		const wxString& text,
		int argFlags = 0) const override;

	virtual void OnSetValue();

protected:

	IPropertyObject* m_ownerProperty = nullptr;
};

#endif