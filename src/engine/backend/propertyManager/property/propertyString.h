#ifndef __PROPERTY_STRING_H__
#define __PROPERTY_STRING_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropString.h"

class BACKEND_API ibPropertyStringBase : public ibProperty {
public:

	wxString GetValueAsString() const {
		static wxString result;
		GetValueAsString(result);
		return std::move(result);
	}

	bool GetValueAsString(wxString& result) const {
		if (!m_propValue.IsNull())
			return m_propValue.GetData()->Write(result);
		return false;
	}

	void SetValue(const wxString& strValue) { m_propValue = strValue; }

	ibPropertyStringBase(ibPropertyCategory* cat, const wxString& name,
		const wxString& value) : ibProperty(cat, name, value)
	{
	}

	ibPropertyStringBase(ibPropertyCategory* cat, const wxString& name, const wxString& label,
		const wxString& value) : ibProperty(cat, name, label, value)
	{
	}

	ibPropertyStringBase(ibPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const wxString& value) : ibProperty(cat, name, label, helpString, value)
	{
	}

	virtual bool IsEmptyProperty() const { return GetValueAsString().IsEmpty(); }

	// set/get property data
	virtual bool SetDataValue(const ibValue& varPropVal);
	virtual bool GetDataValue(ibValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(ibReaderMemory& reader);
	virtual bool SaveData(ibWriterMemory& writer);
};

//base property for "string"
class BACKEND_API ibPropertyString : public ibPropertyStringBase {
public:

	ibPropertyString(ibPropertyCategory* cat, const wxString& name,
		const wxString& value) : ibPropertyStringBase(cat, name, value)
	{
	}

	ibPropertyString(ibPropertyCategory* cat, const wxString& name, const wxString& label,
		const wxString& value) : ibPropertyStringBase(cat, name, label, value)
	{
	}

	ibPropertyString(ibPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const wxString& value) : ibPropertyStringBase(cat, name, label, helpString, value)
	{
	}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxStringProperty(m_propLabel, m_propName, GetValueAsString());
	}
};

//base property for "general" - unique name 
class BACKEND_API ibPropertyUString : public ibPropertyStringBase {
public:

	ibPropertyUString(ibPropertyCategory* cat, const wxString& name,
		const wxString& value) : ibPropertyStringBase(cat, name, value)
	{
	}

	ibPropertyUString(ibPropertyCategory* cat, const wxString& name, const wxString& label,
		const wxString& value) : ibPropertyStringBase(cat, name, label, value)
	{
	}

	ibPropertyUString(ibPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const wxString& value) : ibPropertyStringBase(cat, name, label, helpString, value)
	{
	}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxGeneralStringProperty(m_propLabel, m_propName, GetValueAsString());
	}
};

//base property for "general" - unique name or empty value 
class BACKEND_API ibPropertyUEString : public ibPropertyUString {
public:

	ibPropertyUEString(ibPropertyCategory* cat, const wxString& name,
		const wxString& value) : ibPropertyUString(cat, name, value)
	{
	}

	ibPropertyUEString(ibPropertyCategory* cat, const wxString& name, const wxString& label,
		const wxString& value) : ibPropertyUString(cat, name, label, value)
	{
	}

	ibPropertyUEString(ibPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const wxString& value) : ibPropertyUString(cat, name, label, helpString, value)
	{
	}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxGeneralStringProperty(m_propLabel, m_propName, GetValueAsString(),
			wxGeneralStringProperty::allow_empty);
	}
};


//base property for "caption" - for translate 
class BACKEND_API ibPropertyTString : public ibPropertyStringBase {
public:

	wxString GetValueAsTranslateString() const {
		static wxString result;
		GetValueAsTranslateString(result);
		return std::move(result);
	}

	bool ibPropertyTString::GetValueAsTranslateString(wxString& result) const {
		if (GetValueAsString(result))
			return ibBackendLocalization::GetTranslateGetRawLocText(result, result);
		return false;
	}

	ibPropertyTString(ibPropertyCategory* cat, const wxString& name,
		const wxString& value) : ibPropertyStringBase(cat, name, ibBackendLocalization::CreateLocalizationRawLocText(value))
	{
	}

	ibPropertyTString(ibPropertyCategory* cat, const wxString& name, const wxString& label,
		const wxString& value) : ibPropertyStringBase(cat, name, label, ibBackendLocalization::CreateLocalizationRawLocText(value))
	{
	}

	ibPropertyTString(ibPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const wxString& value) : ibPropertyStringBase(cat, name, label, helpString, ibBackendLocalization::CreateLocalizationRawLocText(value))
	{
	}

	virtual bool IsEmptyProperty() const { return GetValueAsTranslateString().IsEmpty(); }

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxTranslateStringProperty(m_owner, m_propLabel, m_propName, GetValueAsString());
	}

	// set/get property data
	virtual bool SetDataValue(const ibValue& varPropVal);
	virtual bool GetDataValue(ibValue& pvarPropVal) const;
};

//base property for "text"
class BACKEND_API ibPropertyMString : public ibPropertyStringBase {
public:

	ibPropertyMString(ibPropertyCategory* cat, const wxString& name,
		const wxString& value) : ibPropertyStringBase(cat, name, value)
	{
	}

	ibPropertyMString(ibPropertyCategory* cat, const wxString& name, const wxString& label,
		const wxString& value) : ibPropertyStringBase(cat, name, label, value)
	{
	}

	ibPropertyMString(ibPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const wxString& value) : ibPropertyStringBase(cat, name, label, helpString, value)
	{
	}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxMultilineStringProperty(m_propLabel, m_propName, GetValueAsString());
	}
};

#endif