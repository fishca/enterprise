#ifndef __PROPERTY_FONT_H__
#define __PROPERTY_FONT_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropFont.h"

//base property for "font"
class BACKEND_API CPropertyFont : public IProperty {
public:

	wxFont GetValueAsFont() const { return typeConv::StringToFont(m_propValue); }
	wxString GetValueAsString() const { return m_propValue; }
	void SetValue(const wxFont& val) { m_propValue = typeConv::FontToString(val); }
	void SetValue(const wxString& val) { m_propValue = val; }

	CPropertyFont(CPropertyCategory* cat, const wxString& name, const wxFont &f = wxNullFont)
		: IProperty(cat, name, typeConv::FontToString(f))
	{
	}

	CPropertyFont(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxFont& f = wxNullFont)
		: IProperty(cat, name, label, typeConv::FontToString(f))
	{
	}

	CPropertyFont(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString, const wxFont& f = wxNullFont)
		: IProperty(cat, name, label, helpString, typeConv::FontToString(f))
	{
	}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxPGFontProperty(m_propLabel, m_propName, GetValueAsFont());
	}

	// set/get property data
	virtual bool SetDataValue(const CValue& varPropVal);
	virtual bool GetDataValue(CValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer);
};

#endif