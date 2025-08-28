#ifndef __PROPERTY_STRING_H__
#define __PROPERTY_STRING_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropString.h"

class BACKEND_API IPropertyString : public IProperty {
public:

	wxString GetValueAsString() const {
		return m_propValue;
	}

	void SetValue(const wxString& strValue) {
		m_propValue = strValue;
	}

	IPropertyString(CPropertyCategory* cat, const wxString& name,
		const wxString& value) : IProperty(cat, name, value)
	{
	}

	IPropertyString(CPropertyCategory* cat, const wxString& name, const wxString& label,
		const wxString& value) : IProperty(cat, name, label, value)
	{
	}

	IPropertyString(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const wxString& value) : IProperty(cat, name, label, helpString, value)
	{
	}

	virtual bool IsEmptyProperty() const { return GetValueAsString().IsEmpty(); }

	// set/get property data
	virtual bool SetDataValue(const CValue& varPropVal);
	virtual bool GetDataValue(CValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer);
};

//base property for "string"
class BACKEND_API CPropertyString : public IPropertyString {
public:

	CPropertyString(CPropertyCategory* cat, const wxString& name,
		const wxString& value) : IPropertyString(cat, name, value)
	{
	}

	CPropertyString(CPropertyCategory* cat, const wxString& name, const wxString& label,
		const wxString& value) : IPropertyString(cat, name, label, value)
	{
	}

	CPropertyString(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const wxString& value) : IPropertyString(cat, name, label, helpString, value)
	{
	}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxStringProperty(m_propLabel, m_propName, GetValueAsString());
	}
};

//base property for "general" - unique name 
class CPropertyName : public IPropertyString {
public:

	CPropertyName(CPropertyCategory* cat, const wxString& name,
		const wxString& value) : IPropertyString(cat, name, value)
	{
	}

	CPropertyName(CPropertyCategory* cat, const wxString& name, const wxString& label,
		const wxString& value) : IPropertyString(cat, name, label, value)
	{
	}

	CPropertyName(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const wxString& value) : IPropertyString(cat, name, label, helpString, value)
	{
	}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxGeneralStringProperty(m_propLabel, m_propName, GetValueAsString());
	}
};

//base property for "caption" - for translate, descr 
class CPropertyCaption : public IPropertyString {
public:

	CPropertyCaption(CPropertyCategory* cat, const wxString& name,
		const wxString& value) : IPropertyString(cat, name, value)
	{
	}

	CPropertyCaption(CPropertyCategory* cat, const wxString& name, const wxString& label,
		const wxString& value) : IPropertyString(cat, name, label, value)
	{
	}

	CPropertyCaption(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const wxString& value) : IPropertyString(cat, name, label, helpString, value)
	{
	}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxMultilineStringProperty(m_propLabel, m_propName, GetValueAsString());
	}
};

//base property for "text"
class CPropertyText : public IPropertyString {
public:

	CPropertyText(CPropertyCategory* cat, const wxString& name,
		const wxString& value) : IPropertyString(cat, name, value)
	{
	}

	CPropertyText(CPropertyCategory* cat, const wxString& name, const wxString& label,
		const wxString& value) : IPropertyString(cat, name, label, value)
	{
	}

	CPropertyText(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const wxString& value) : IPropertyString(cat, name, label, helpString, value)
	{
	}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxMultilineStringProperty(m_propLabel, m_propName, GetValueAsString());
	}
};

#endif