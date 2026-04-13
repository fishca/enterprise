#ifndef __PROPERTY_TYPE_H__
#define __PROPERTY_TYPE_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropType.h"

//base property for "type"
class BACKEND_API CPropertyType : public IProperty {
	wxVariantData* CreateVariantData(IPropertyObject *property, const eValueTypes type) const;
	wxVariantData* CreateVariantData(IPropertyObject* property, const class_identifier_t& clsid) const;
	wxVariantData* CreateVariantData(IPropertyObject* property, const CTypeDescription& typeDesc) const;
public:

	CTypeDescription& GetValueAsTypeDesc() const;
	void SetValue(const CTypeDescription& val);

	CPropertyType(CPropertyCategory* cat, const wxString& name, const eValueTypes type) : IProperty(cat, name, CreateVariantData(cat->GetPropertyObject(), type)) {}
	CPropertyType(CPropertyCategory* cat, const wxString& name, const class_identifier_t& clsid) : IProperty(cat, name, CreateVariantData(cat->GetPropertyObject(), clsid)) {}
	CPropertyType(CPropertyCategory* cat, const wxString& name, const wxString& label, const eValueTypes type) : IProperty(cat, name, label, CreateVariantData(cat->GetPropertyObject(), type)) {}

	CPropertyType(CPropertyCategory* cat, const wxString& name, const wxString& label, const class_identifier_t& clsid) : IProperty(cat, name, label, CreateVariantData(cat->GetPropertyObject(), clsid)) {}
	CPropertyType(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString, const eValueTypes type) : IProperty(cat, name, label, helpString, CreateVariantData(cat->GetPropertyObject(), type)) {}
	CPropertyType(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString, const class_identifier_t& clsid) : IProperty(cat, name, label, helpString, CreateVariantData(cat->GetPropertyObject(), clsid)) {}

	virtual bool IsEmptyProperty() const { return !GetValueAsTypeDesc().IsOk(); }

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxPGTypeProperty(m_owner, GetFilterDataType(), m_propLabel, m_propName, m_propValue);
	}

	//set/get property data
	virtual bool SetDataValue(const CValue& varPropVal);
	virtual bool GetDataValue(CValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer);

protected:
	eSelectorDataType GetFilterDataType() const;
};

#endif