#ifndef __PROPERTY_NUMBER_H__
#define __PROPERTY_NUMBER_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropNumber.h"

//base property for "number"
class BACKEND_API CPropertyNumber : public IProperty {
	wxVariantData* CreateVariantData(const number_t& val);
public:

	number_t& GetValueAsNumber() const;
	void SetValue(const number_t& val);

	CPropertyNumber(CPropertyCategory* cat, const wxString& name,
		const number_t& value = 0) : IProperty(cat, name, CreateVariantData(value))
	{
	}

	CPropertyNumber(CPropertyCategory* cat, const wxString& name, const wxString& label,
		const number_t& value = 0) : IProperty(cat, name, label, CreateVariantData(value))
	{
	}

	CPropertyNumber(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const number_t& value = 0) : IProperty(cat, name, label, helpString, CreateVariantData(value))
	{
	}

	virtual bool IsEmptyProperty() const { return GetValueAsNumber().IsZero(); }

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxNumberProperty(m_propLabel, m_propName, GetValueAsNumber());
	};

	// set/get property data
	virtual bool SetDataValue(const CValue& varPropVal);
	virtual bool GetDataValue(CValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer);
};

//base property for "integer"
class BACKEND_API CPropertyInteger : public IProperty {
	wxVariant CreateVariantData(const int& val) const { return WXVARIANT(val); }
public:

	void SetValue(const int& val) { m_propValue = CreateVariantData(val); }
	int GetValueAsInteger() const { return m_propValue.GetLong(); }

	CPropertyInteger(CPropertyCategory* cat, const wxString& name,
		const int& value = 0) : IProperty(cat, name, CreateVariantData(value))
	{
	}

	CPropertyInteger(CPropertyCategory* cat, const wxString& name, const wxString& label,
		const int& value = 0) : IProperty(cat, name, label, CreateVariantData(value))
	{
	}

	CPropertyInteger(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const int& value = 0) : IProperty(cat, name, label, helpString, CreateVariantData(value))
	{
	}

	virtual bool IsEmptyProperty() const { return GetValueAsInteger() == 0; }

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxIntProperty(m_propLabel, m_propName, GetValueAsInteger());
	};

	// set/get property data
	virtual bool SetDataValue(const CValue& varPropVal) {
		SetValue(varPropVal.GetInteger());
		return true;
	}
	virtual bool GetDataValue(CValue& pvarPropVal) const {
		pvarPropVal = GetValueAsInteger();
		return true;
	}

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader) {
		SetValue(reader.r_s32());
		return true;
	}

	virtual bool SaveData(CMemoryWriter& writer) {
		writer.w_s32(GetValueAsInteger());
		return true;
	}
};

//base property for "unsigned integer"
class BACKEND_API CPropertyUInteger : public IProperty {
	wxVariant CreateVariantData(const unsigned int& val) const { return WXVARIANT((long)val); }
public:

	void SetValue(const unsigned int& val) { m_propValue = CreateVariantData(val); }
	unsigned int GetValueAsUInteger() const { return m_propValue.GetLong(); }

	CPropertyUInteger(CPropertyCategory* cat, const wxString& name,
		const unsigned int& value = 0) : IProperty(cat, name, CreateVariantData(value))
	{
	}

	CPropertyUInteger(CPropertyCategory* cat, const wxString& name, const wxString& label,
		const unsigned int& value = 0) : IProperty(cat, name, label, CreateVariantData(value))
	{
	}

	CPropertyUInteger(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const unsigned int& value = 0) : IProperty(cat, name, label, helpString, CreateVariantData(value))
	{
	}

	virtual bool IsEmptyProperty() const { return GetValueAsUInteger() == 0; }

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxUIntProperty(m_propLabel, m_propName, GetValueAsUInteger());
	};

	// set/get property data
	virtual bool SetDataValue(const CValue& varPropVal) { 
		SetValue(varPropVal.GetUInteger());
		return true; 
	}
	
	virtual bool GetDataValue(CValue& pvarPropVal) const {
		pvarPropVal = GetValueAsUInteger();
		return true;
	}

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader) {
		SetValue(reader.r_u32());
		return true;
	}
	
	virtual bool SaveData(CMemoryWriter& writer) {
		writer.w_u32(GetValueAsUInteger());
		return true;
	}
};

#endif