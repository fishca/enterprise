#ifndef __ADVPROP_STRING_H__
#define __ADVPROP_STRING_H__

#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>

#include "backend/backend_core.h"
#include "backend/backend_localization.h"

// -----------------------------------------------------------------------
// wxGeneralStringProperty
// -----------------------------------------------------------------------

class BACKEND_API wxGeneralStringProperty : public wxStringProperty {
public:

	wxGeneralStringProperty(const wxString& label = wxPG_LABEL,
		const wxString& name = wxPG_LABEL,
		const wxString& value = wxEmptyString) : wxStringProperty(label, name, value) {
	}

	virtual wxString ValueToString(wxVariant& value, int argFlags = 0) const override;
	virtual bool StringToValue(wxVariant& variant,
		const wxString& text,
		int argFlags = 0) const override;

private:
	WX_PG_DECLARE_PROPERTY_CLASS(wxGeneralStringProperty);
};

// -----------------------------------------------------------------------
// wxTranslateStringProperty
// -----------------------------------------------------------------------

class BACKEND_API IPropertyObject;

class BACKEND_API wxTranslateStringProperty : public wxLongStringProperty {
public:
	wxTranslateStringProperty(const IPropertyObject* property = nullptr, const wxString& label = wxPG_LABEL,
		const wxString& name = wxPG_LABEL,
		const wxString& value = wxEmptyString) :
		wxLongStringProperty(label, name, value), m_ownerProperty(property)
	{
	}

	virtual wxString ValueToString(wxVariant& value, int argFlags = 0) const override;
	virtual bool StringToValue(wxVariant& variant,
		const wxString& text,
		int argFlags = 0) const override;

protected:
	virtual bool DisplayEditorDialog(wxPropertyGrid* pg, wxVariant& value) override;
private:

	const IPropertyObject* m_ownerProperty = nullptr;

	WX_PG_DECLARE_PROPERTY_CLASS(wxTranslateStringProperty);
};

// -----------------------------------------------------------------------
// wxMultilineStringProperty
// -----------------------------------------------------------------------

class BACKEND_API wxMultilineStringProperty : public wxLongStringProperty {
public:
	wxMultilineStringProperty(const wxString& label = wxPG_LABEL,
		const wxString& name = wxPG_LABEL,
		const wxString& value = wxEmptyString) :
		wxLongStringProperty(label, name, value)
	{
	}

protected:

	virtual bool DisplayEditorDialog(wxPropertyGrid* pg, wxVariant& value) override;

private:

	WX_PG_DECLARE_PROPERTY_CLASS(wxMultilineStringProperty);
};

#endif