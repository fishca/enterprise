#ifndef __PROPERTY_STRING_H__
#define __PROPERTY_STRING_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropString.h"

class BACKEND_API IPropertyString : public IProperty {
public:

	wxString GetValueAsString() const {
		wxString result;
		GetValueAsString(result);
		return result;
	}

	bool GetValueAsString(wxString& result) const {
		if (!m_propValue.IsNull())
			return m_propValue.GetData()->Write(result);
		return false;
	}

	void SetValue(const wxString& strValue) { m_propValue = strValue; }

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
class BACKEND_API CPropertyUString : public IPropertyString {
public:

	CPropertyUString(CPropertyCategory* cat, const wxString& name,
		const wxString& value) : IPropertyString(cat, name, value)
	{
	}

	CPropertyUString(CPropertyCategory* cat, const wxString& name, const wxString& label,
		const wxString& value) : IPropertyString(cat, name, label, value)
	{
	}

	CPropertyUString(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const wxString& value) : IPropertyString(cat, name, label, helpString, value)
	{
	}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxGeneralStringProperty(m_propLabel, m_propName, GetValueAsString());
	}
};

//base property for "caption" - for translate 
class BACKEND_API CPropertyTString : public IPropertyString {
public:

	wxString GetValueAsTranslateString() const {
		wxString result;
		GetValueAsTranslateString(result);
		return result;
	}

	bool CPropertyTString::GetValueAsTranslateString(wxString& result) const {
		if (GetValueAsString(result))
			return CBackendLocalization::GetTranslateGetRawLocText(result, result);
		return false;
	}

	CPropertyTString(CPropertyCategory* cat, const wxString& name,
		const wxString& value) : IPropertyString(cat, name, CBackendLocalization::CreateLocalizationRawLocText(value))
	{
	}

	CPropertyTString(CPropertyCategory* cat, const wxString& name, const wxString& label,
		const wxString& value) : IPropertyString(cat, name, label, CBackendLocalization::CreateLocalizationRawLocText(value))
	{
	}

	CPropertyTString(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const wxString& value) : IPropertyString(cat, name, label, helpString, CBackendLocalization::CreateLocalizationRawLocText(value))
	{
	}

	virtual bool IsEmptyProperty() const { return GetValueAsTranslateString().IsEmpty(); }

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxTranslateStringProperty(m_owner, m_propLabel, m_propName, GetValueAsString());
	}

	// set/get property data
	virtual bool SetDataValue(const CValue& varPropVal);
	virtual bool GetDataValue(CValue& pvarPropVal) const;
};

//base property for "text"
class BACKEND_API CPropertyMString : public IPropertyString {
public:

	CPropertyMString(CPropertyCategory* cat, const wxString& name,
		const wxString& value) : IPropertyString(cat, name, value)
	{
	}

	CPropertyMString(CPropertyCategory* cat, const wxString& name, const wxString& label,
		const wxString& value) : IPropertyString(cat, name, label, value)
	{
	}

	CPropertyMString(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const wxString& value) : IPropertyString(cat, name, label, helpString, value)
	{
	}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxMultilineStringProperty(m_propLabel, m_propName, GetValueAsString());
	}
};

#endif