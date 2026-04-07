#ifndef __ADVPROP_POINT_H__
#define __ADVPROP_POINT_H__

#include <wx/propgrid/propgrid.h>
#include "backend/backend_core.h"

// -----------------------------------------------------------------------
// ibPGPointProperty
// -----------------------------------------------------------------------

class ibPGPointProperty : public wxPGProperty {
public:
	ibPGPointProperty(const wxString& label = wxPG_LABEL,
		const wxString& name = wxPG_LABEL,
		const wxPoint& value = wxPoint());
	virtual ~ibPGPointProperty() override;

	virtual wxVariant ChildChanged(wxVariant& thisValue, int childIndex,
		wxVariant& childValue) const override;

	virtual void RefreshChildren() override;

protected:
	virtual void DoSetValue(const wxPoint& value) { m_value = WXVARIANT(value); }
private:
	WX_PG_DECLARE_PROPERTY_CLASS(ibPGPointProperty);
};

#endif