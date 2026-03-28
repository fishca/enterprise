#ifndef __ADVPROP_OWNER_H__
#define __ADVPROP_OWNER_H__

#include <wx/propgrid/propgrid.h>
#include "backend/backend_type.h"

class BACKEND_API ibPropertyObject;

// -----------------------------------------------------------------------
// wxPGTypeProperty
// -----------------------------------------------------------------------

class BACKEND_API wxPGOwnerProperty : public wxPGProperty {
	void FillByClsid(const ibClassID& clsid);
public:

	const ibPropertyObject* GetPropertyObject() const { return m_ownerProperty; }

	wxPGOwnerProperty(const ibPropertyObject* property = nullptr, const wxString& label = wxPG_LABEL,
		const wxString& name = wxPG_LABEL, const wxVariant& value = wxNullVariant);

	virtual wxString ValueToString(wxVariant& value,
		wxPGPropValFormatFlags flags = wxPGPropValFormatFlags::Null) const override;

	virtual bool StringToValue(wxVariant& variant,
		const wxString& text,
		wxPGPropValFormatFlags flags = wxPGPropValFormatFlags::Null) const override;

	virtual bool IntToValue(wxVariant& value,
		int number,
		wxPGPropValFormatFlags flags = wxPGPropValFormatFlags::Null) const override;

	virtual wxPGEditorDialogAdapter* GetEditorDialog() const override;

protected:
	const ibPropertyObject* m_ownerProperty = nullptr;
private:
	WX_PG_DECLARE_PROPERTY_CLASS(wxPGOwnerProperty);
};

#endif