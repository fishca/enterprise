#ifndef __PROPERTY_POINT_H__
#define __PROPERTY_POINT_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropPoint.h"

//base property for "size"
class BACKEND_API CPropertyPoint : public IProperty {
	wxVariant CreateVariantData(const wxPoint& val) const {
		wxVariant newValue;
		newValue << val;
		return newValue;
	}
public:

	wxPoint GetValueAsPoint() const {
		wxPoint point;
		point << m_propValue;
		return point;
	}
	wxString GetValueAsString() const { return typeConv::PointToString(GetValueAsPoint()); }
	
	void SetValue(const wxPoint& val) { IProperty::SetValue(CreateVariantData(val)); }
	void SetValue(const wxString& val) { SetValue(typeConv::StringToPoint(val)); }

	CPropertyPoint(CPropertyCategory* cat, const wxString& name, const wxPoint &p = wxDefaultPosition)
		: IProperty(cat, name, CreateVariantData(p))
	{
	}

	CPropertyPoint(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxPoint& p = wxDefaultPosition)
		: IProperty(cat, name, label, CreateVariantData(p))
	{
	}

	CPropertyPoint(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString, const wxPoint& p = wxDefaultPosition)
		: IProperty(cat, name, label, helpString, CreateVariantData(p))
	{
	}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxPGPointProperty(m_propLabel, m_propName, GetValueAsPoint());
	}

	// set/get property data
	virtual bool SetDataValue(const CValue& varPropVal);
	virtual bool GetDataValue(CValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer);
};

#endif