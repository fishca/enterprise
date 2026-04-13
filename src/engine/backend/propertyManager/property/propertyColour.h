#ifndef __PROPERTY_COLOUR_H__
#define __PROPERTY_COLOUR_H__

#include "backend/propertyManager/propertyObject.h"

#include <wx/propgrid/advprops.h>

//base property for "colour"
class BACKEND_API CPropertyColour : public IProperty {
	wxVariant CreateVariantData(const wxColour& val) const {
		wxVariant newValue;
		newValue << val;
		return newValue;
	}
public:

	wxColour GetValueAsColour() const {
		wxColour colour;
		colour << m_propValue;
		return colour;
	}
	
	wxString GetValueAsString() const { return typeConv::ColourToString(GetValueAsColour()); }

	void SetValue(const wxColour& val) { IProperty::SetValue(CreateVariantData(val)); }
	void SetValue(const wxString& val) { SetValue(typeConv::StringToColour(val)); }

	CPropertyColour(CPropertyCategory* cat, const wxString& name, const wxColour& c = wxNullColour)
		: IProperty(cat, name, CreateVariantData(c))
	{
	}

	CPropertyColour(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxColour& c = wxNullColour)
		: IProperty(cat, name, label, CreateVariantData(c))
	{
	}

	CPropertyColour(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString, const wxColour& c = wxNullColour)
		: IProperty(cat, name, label, helpString, CreateVariantData(c))
	{
	}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxColourProperty(m_propLabel, m_propName, GetValueAsColour());
	}

	// set/get property data
	virtual bool SetDataValue(const CValue& varPropVal);
	virtual bool GetDataValue(CValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer);
};

#endif