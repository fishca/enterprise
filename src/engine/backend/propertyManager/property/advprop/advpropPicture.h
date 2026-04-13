#ifndef __ADVPROP_PICTURE_H__
#define __ADVPROP_PICTURE_H__

#include <wx/propgrid/propgrid.h>

#include "backend/backend_core.h"
#include "backend/pictureDescription.h"

// -----------------------------------------------------------------------
// wxPGExternalImageProperty
// -----------------------------------------------------------------------

class wxPGExternalImageProperty : public wxEditorDialogProperty {
public:

	wxPGExternalImageProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL, const wxVariant& value = wxVariant());

	virtual wxSize OnMeasureImage(int item) const override { return wxSize(16, 16); }
	virtual void OnCustomPaint(wxDC& dc,
		const wxRect& rect, wxPGPaintData& paintdata) override;

	virtual wxString ValueToString(wxVariant& value, int argFlags = 0) const override { return value.GetString(); }
	virtual bool StringToValue(wxVariant& variant,
		const wxString& text,
		int argFlags = 0) const override;

	virtual bool DisplayEditorDialog(wxPropertyGrid* pg, wxVariant& value) override;

protected:

	wxString m_basePath;
	wxString m_initialPath;
	wxString m_wildcard;

	int m_indFilter;

	wxBitmap   m_bitmap; // final thumbnail area

private:
	WX_PG_DECLARE_PROPERTY_CLASS(wxPGExternalImageProperty);
};

// -----------------------------------------------------------------------
// wxPGPictureProperty
// -----------------------------------------------------------------------

class BACKEND_API wxPGPictureProperty : public wxPGProperty {
public:

	wxPGPictureProperty(const wxString& label = wxPG_LABEL,
		const wxString& name = wxPG_LABEL,
		const wxVariant& value = wxVariant());

	virtual ~wxPGPictureProperty() {}

	virtual wxString ValueToString(wxVariant& value, int argFlags = 0) const override { return value.GetString(); }
	virtual bool StringToValue(wxVariant& variant,
		const wxString& text,
		int argFlags = 0) const override {
		return false;
	}

	virtual void RefreshChildren() override;
	virtual wxVariant ChildChanged(wxVariant& thisValue, int childIndex,
		wxVariant& childValue) const override;

protected:

	wxEnumProperty* m_propertySource;

	wxEditEnumProperty* m_propertyBackend;
	wxEditEnumProperty* m_propertyConfiguration;
	wxPGExternalImageProperty* m_propertyFile;

	std::map<int, class_identifier_t> m_valChoices;
	std::map<int, CGuid> m_confChoices;

private:
	WX_PG_DECLARE_PROPERTY_CLASS(wxPGPictureProperty);
};

#endif