#ifndef __PROPERTY_DATE_H__
#define __PROPERTY_DATE_H__

#include "backend/propertyManager/propertyObject.h"

#include <wx/propgrid/advprops.h>

//base property for "date"
class BACKEND_API CPropertyDate : public IProperty {
public:

	wxLongLong_t GetValueAsDateTime() const { return stringUtils::StrToInt(m_propValue); }
	void SetValue(const wxLongLong_t& val = emptyDate) { m_propValue = stringUtils::IntToStr(val); }

	CPropertyDate(CPropertyCategory* cat, const wxString& name,
		const wxLongLong_t& value = emptyDate) : IProperty(cat, name, stringUtils::IntToStr(value))
	{
	}

	CPropertyDate(CPropertyCategory* cat, const wxString& name, const wxString& label,
		const wxLongLong_t& value = emptyDate) : IProperty(cat, name, label, stringUtils::IntToStr(value))
	{
	}

	CPropertyDate(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const wxLongLong_t& value = emptyDate) : IProperty(cat, name, label, helpString, stringUtils::IntToStr(value))
	{
	}

	virtual bool IsEmptyProperty() const { return GetValueAsDateTime() == emptyDate; }

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxDateProperty(m_propLabel, m_propName, GetValueAsDateTime());
	}

	// set/get property data
	virtual bool SetDataValue(const CValue& varPropVal);
	virtual bool GetDataValue(CValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer);
};

#endif