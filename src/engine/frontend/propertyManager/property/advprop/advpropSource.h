#ifndef __ADVPROP_SOURCE_H__
#define __ADVPROP_SOURCE_H__

#include "frontend/propertyManager/property/advprop/advpropType.h"

class BACKEND_API ibPropertyObject;

// -----------------------------------------------------------------------
// ibPGDataSourceProperty
// -----------------------------------------------------------------------

class ibPGDataSourceProperty : public wxPGProperty {
public:

	const ibPropertyObject* GetPropertyObject() const { return m_typeSelector->GetPropertyObject(); }

	ibPGDataSourceProperty(const ibPropertyObject* property = nullptr, const wxString& label = wxPG_LABEL,
		const wxString& name = wxPG_LABEL,
		const wxVariant& value = wxNullVariant);

	virtual wxString ValueToString(wxVariant& value,
		wxPGPropValFormatFlags flags = wxPGPropValFormatFlags::Null) const override;

	virtual bool StringToValue(wxVariant& variant,
		const wxString& text,
		wxPGPropValFormatFlags flags = wxPGPropValFormatFlags::Null) const override;

	virtual wxVariant ChildChanged(wxVariant& thisValue,
		int childIndex,
		wxVariant& childValue) const override;

	virtual void RefreshChildren() override;
	virtual wxPGEditorDialogAdapter* GetEditorDialog() const override;

protected:
	ibPGTypeProperty* m_typeSelector;
private:
	WX_PG_DECLARE_PROPERTY_CLASS(ibPGDataSourceProperty)
};

#endif