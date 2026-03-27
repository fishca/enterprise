#ifndef __PROPERTY_SIZE_H__
#define __PROPERTY_SIZE_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropSize.h"

//base property for "size"
class BACKEND_API ibPropertySize : public ibProperty {
	wxVariant CreateVariantData(const wxSize& val) const {
		wxVariant newValue;
		newValue << val;
		return newValue;
	}
public:
	wxSize GetValueAsSize() const { 
		wxSize size;
		size << m_propValue;
		return size;
	}
	wxString GetValueAsString() const { return typeConv::SizeToString(GetValueAsSize()); }
	
	void SetValue(const wxSize& val) { ibProperty::SetValue(CreateVariantData(val)); }
	void SetValue(const wxString& val) { SetValue(typeConv::StringToSize(val)); }

	ibPropertySize(ibPropertyCategory* cat, const wxString& name, const wxSize &s = wxDefaultSize)
		: ibProperty(cat, name, CreateVariantData(s))
	{
	}

	ibPropertySize(ibPropertyCategory* cat, const wxString& name, const wxString& label, const wxSize& s = wxDefaultSize)
		: ibProperty(cat, name, label, CreateVariantData(s))
	{
	}

	ibPropertySize(ibPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString, const wxSize& s = wxDefaultSize)
		: ibProperty(cat, name, label, helpString, CreateVariantData(s))
	{
	}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxPGSizeProperty(m_propLabel, m_propName, GetValueAsSize());
	}

	// set/get property data
	virtual bool SetDataValue(const ibValue& varPropVal);
	virtual bool GetDataValue(ibValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(ibReaderMemory& reader);
	virtual bool SaveData(ibWriterMemory& writer);
};

#endif