#ifndef __PROPERTY_FONT_H__
#define __PROPERTY_FONT_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropFont.h"

//base property for "font"
class BACKEND_API ibPropertyFont : public ibProperty {
public:

	wxFont GetValueAsFont() const { return typeConv::StringToFont(m_propValue); }
	wxString GetValueAsString() const { return m_propValue; }
	void SetValue(const wxFont& val) { m_propValue = typeConv::FontToString(val); }
	void SetValue(const wxString& val) { m_propValue = val; }

	ibPropertyFont(ibPropertyCategory* cat, const wxString& name, const wxFont &f = wxNullFont)
		: ibProperty(cat, name, typeConv::FontToString(f))
	{
	}

	ibPropertyFont(ibPropertyCategory* cat, const wxString& name, const wxString& label, const wxFont& f = wxNullFont)
		: ibProperty(cat, name, label, typeConv::FontToString(f))
	{
	}

	ibPropertyFont(ibPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString, const wxFont& f = wxNullFont)
		: ibProperty(cat, name, label, helpString, typeConv::FontToString(f))
	{
	}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxPGFontProperty(m_propLabel, m_propName, GetValueAsFont());
	}

	// set/get property data
	virtual bool SetDataValue(const ibValue& varPropVal);
	virtual bool GetDataValue(ibValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(ibReaderMemory& reader);
	virtual bool SaveData(ibWriterMemory& writer);
};

#endif