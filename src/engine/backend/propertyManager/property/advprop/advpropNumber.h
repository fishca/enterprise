#ifndef __ADVPROP_NUMBER_H__
#define __ADVPROP_NUMBER_H__

#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>

#include "backend/backend_core.h"

// -----------------------------------------------------------------------
// wxNumberProperty
// -----------------------------------------------------------------------

class BACKEND_API wxNumberProperty : public wxNumericProperty {
public:
	wxNumberProperty(const wxString& label = wxPG_LABEL,
		const wxString& name = wxPG_LABEL,
		const number_t& value = 0.0);
	virtual ~wxNumberProperty();

	virtual wxString ValueToString(wxVariant& value, int argFlags = 0) const wxOVERRIDE;
	virtual bool StringToValue(wxVariant& variant,
		const wxString& text,
		int argFlags = 0) const wxOVERRIDE;
	virtual bool DoSetAttribute(const wxString& name, wxVariant& value) wxOVERRIDE;

	virtual bool ValidateValue(wxVariant& value,
		wxPGValidationInfo& validationInfo) const wxOVERRIDE;

	static wxValidator* GetClassValidator();

	virtual wxValidator* DoGetValidator() const wxOVERRIDE;
	virtual wxVariant AddSpinStepValue(long stepScale) const wxOVERRIDE;

protected:

	// Common validation code to be called in ValidateValue() implementations.
	// Note that 'value' is reference on purpose, so we can write
	// back to it when mode is wxPG_PROPERTY_VALIDATION_SATURATE or wxPG_PROPERTY_VALIDATION_WRAP.
	bool DoNumericValidation(number_t& value, wxPGValidationInfo* pValidationInfo, int mode) const
	{
		if (value.IsNan()) {
			if (value < 0) {
				if (mode == wxPG_PROPERTY_VALIDATION_ERROR_MESSAGE) {
					wxString msg = wxString::Format(_("Value must be higher."));
					pValidationInfo->SetFailureMessage(msg);
				}
				value = 0;
				return false;
			}
			if (value > 0) {
				if (mode == wxPG_PROPERTY_VALIDATION_ERROR_MESSAGE) {
					wxString msg = wxString::Format(_("Value must be less."));
					pValidationInfo->SetFailureMessage(msg);
				}
				value = 0;
				return false;
			}
		}
		return true;
	}

	int m_precision;

private:
	WX_PG_DECLARE_PROPERTY_CLASS(wxNumberProperty);
};

#endif