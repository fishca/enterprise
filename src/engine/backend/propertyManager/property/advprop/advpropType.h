#ifndef __ADVPROP_TYPE_H__
#define __ADVPROP_TYPE_H__

#include <wx/propgrid/propgrid.h>
#include "backend/backend_type.h"

class BACKEND_API IPropertyObject;

// -----------------------------------------------------------------------
// wxPGTypeProperty
// -----------------------------------------------------------------------

class BACKEND_API wxPGTypeProperty : public wxPGProperty {
	WX_PG_DECLARE_PROPERTY_CLASS(wxPGTypeProperty);
private:
	wxPGChoices GetDateTime();
	void FillByClsid(const eSelectorDataType &selectorDataType, const class_identifier_t& clsid);
public:

	const IPropertyObject* GetPropertyObject() const { return m_ownerProperty; }

	wxPGTypeProperty(const IPropertyObject* property = nullptr, const eSelectorDataType& selectorDataType = eSelectorDataType::eSelectorDataType_any, const wxString& label = wxPG_LABEL,
		const wxString& name = wxPG_LABEL, const wxVariant& value = wxNullVariant);

	virtual wxString ValueToString(wxVariant& value, int argFlags = 0) const override { return value.GetString(); }
	virtual bool StringToValue(wxVariant& variant,
		const wxString& text,
		int argFlags = 0) const override { return false; }

	virtual bool IntToValue(wxVariant& value,
		int number,
		int argFlags = 0) const override;

	virtual wxVariant ChildChanged(wxVariant& thisValue,
		int childIndex,
		wxVariant& childValue) const override;

	virtual void RefreshChildren() override;

	// GetChoiceSelection needs to overridden since m_choices is
	// used and value is integer, but it is not index.
	virtual int GetChoiceSelection() const override { return wxNOT_FOUND; }

	virtual wxPGEditorDialogAdapter* GetEditorDialog() const override;

protected:

	const IPropertyObject* m_ownerProperty = nullptr;

	wxUIntProperty* m_precision;
	wxUIntProperty* m_scale;
	wxEnumProperty* m_date_time;
	wxUIntProperty* m_length;

	std::map<int, class_identifier_t> m_valChoices;
};

#endif