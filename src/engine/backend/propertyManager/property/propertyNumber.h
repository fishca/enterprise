#ifndef __PROPERTY_NUMBER_H__
#define __PROPERTY_NUMBER_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropNumber.h"

//base property for "number"
class BACKEND_API ibPropertyNumber : public ibProperty {
	wxVariantData* CreateVariantData(const ibNumber& val);
public:

	ibNumber& GetValueAsNumber() const;
	void SetValue(const ibNumber& val);

	ibPropertyNumber(ibPropertyCategory* cat, const wxString& name,
		const ibNumber& value = 0) : ibProperty(cat, name, CreateVariantData(value))
	{
	}

	ibPropertyNumber(ibPropertyCategory* cat, const wxString& name, const wxString& label,
		const ibNumber& value = 0) : ibProperty(cat, name, label, CreateVariantData(value))
	{
	}

	ibPropertyNumber(ibPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const ibNumber& value = 0) : ibProperty(cat, name, label, helpString, CreateVariantData(value))
	{
	}

	virtual bool IsEmptyProperty() const { return GetValueAsNumber().IsZero(); }

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxNumberProperty(m_propLabel, m_propName, GetValueAsNumber());
	};

	// set/get property data
	virtual bool SetDataValue(const ibValue& varPropVal);
	virtual bool GetDataValue(ibValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(ibReaderMemory& reader);
	virtual bool SaveData(ibWriterMemory& writer);
};

//base property for "integer"
class BACKEND_API ibPropertyInteger : public ibProperty {
	wxVariant CreateVariantData(const int& val) const { return WXVARIANT(val); }
public:

	void SetValue(const int& val) { m_propValue = CreateVariantData(val); }
	int GetValueAsInteger() const { return m_propValue.GetLong(); }

	ibPropertyInteger(ibPropertyCategory* cat, const wxString& name,
		const int& value = 0) : ibProperty(cat, name, CreateVariantData(value))
	{
	}

	ibPropertyInteger(ibPropertyCategory* cat, const wxString& name, const wxString& label,
		const int& value = 0) : ibProperty(cat, name, label, CreateVariantData(value))
	{
	}

	ibPropertyInteger(ibPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const int& value = 0) : ibProperty(cat, name, label, helpString, CreateVariantData(value))
	{
	}

	virtual bool IsEmptyProperty() const { return GetValueAsInteger() == 0; }

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxIntProperty(m_propLabel, m_propName, GetValueAsInteger());
	};

	// set/get property data
	virtual bool SetDataValue(const ibValue& varPropVal) {
		SetValue(varPropVal.GetInteger());
		return true;
	}
	virtual bool GetDataValue(ibValue& pvarPropVal) const {
		pvarPropVal = GetValueAsInteger();
		return true;
	}

	//load & save object in control 
	virtual bool LoadData(ibReaderMemory& reader) {
		SetValue(reader.r_s32());
		return true;
	}

	virtual bool SaveData(ibWriterMemory& writer) {
		writer.w_s32(GetValueAsInteger());
		return true;
	}
};

//base property for "unsigned integer"
class BACKEND_API ibPropertyUInteger : public ibProperty {
	wxVariant CreateVariantData(const unsigned int& val) const { return WXVARIANT((long)val); }
public:

	void SetValue(const unsigned int& val) { m_propValue = CreateVariantData(val); }
	unsigned int GetValueAsUInteger() const { return m_propValue.GetLong(); }

	ibPropertyUInteger(ibPropertyCategory* cat, const wxString& name,
		const unsigned int& value = 0) : ibProperty(cat, name, CreateVariantData(value))
	{
	}

	ibPropertyUInteger(ibPropertyCategory* cat, const wxString& name, const wxString& label,
		const unsigned int& value = 0) : ibProperty(cat, name, label, CreateVariantData(value))
	{
	}

	ibPropertyUInteger(ibPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const unsigned int& value = 0) : ibProperty(cat, name, label, helpString, CreateVariantData(value))
	{
	}

	virtual bool IsEmptyProperty() const { return GetValueAsUInteger() == 0; }

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxUIntProperty(m_propLabel, m_propName, GetValueAsUInteger());
	};

	// set/get property data
	virtual bool SetDataValue(const ibValue& varPropVal) { 
		SetValue(varPropVal.GetUInteger());
		return true; 
	}
	
	virtual bool GetDataValue(ibValue& pvarPropVal) const {
		pvarPropVal = GetValueAsUInteger();
		return true;
	}

	//load & save object in control 
	virtual bool LoadData(ibReaderMemory& reader) {
		SetValue(reader.r_u32());
		return true;
	}
	
	virtual bool SaveData(ibWriterMemory& writer) {
		writer.w_u32(GetValueAsUInteger());
		return true;
	}
};

#endif