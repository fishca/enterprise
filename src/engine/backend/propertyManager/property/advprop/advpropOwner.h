#ifndef __ADVPROP_OWNER_H__
#define __ADVPROP_OWNER_H__

#include <wx/propgrid/propgrid.h>
#include "backend/backend_type.h"

class BACKEND_API IPropertyObject;

// -----------------------------------------------------------------------
// wxPGTypeProperty
// -----------------------------------------------------------------------

class BACKEND_API wxPGOwnerProperty : public wxPGProperty {
	void FillByClsid(const class_identifier_t& clsid);
public:

	IPropertyObject* GetPropertyObject() const { return m_ownerProperty; }

	wxPGOwnerProperty(IPropertyObject* property = nullptr, const wxString& label = wxPG_LABEL,
		const wxString& name = wxPG_LABEL, const wxVariant& value = wxNullVariant);

	virtual wxString ValueToString(wxVariant& value, int argFlags = 0) const override;
	virtual bool StringToValue(wxVariant& variant,
		const wxString& text,
		int argFlags = 0) const override;

	virtual bool IntToValue(wxVariant& value,
		int number,
		int argFlags = 0) const override;

	virtual wxPGEditorDialogAdapter* GetEditorDialog() const override;

protected:
	IPropertyObject* m_ownerProperty = nullptr;
private:
	WX_PG_DECLARE_PROPERTY_CLASS(wxPGOwnerProperty);
};

#endif