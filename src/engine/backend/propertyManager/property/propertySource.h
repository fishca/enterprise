#ifndef __PROPERTY_SOURCE_H__
#define __PROPERTY_SOURCE_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropSource.h"

//////////////////////////////////////////////////////////////////
struct CTypeDescription;
//////////////////////////////////////////////////////////////////

//base property for "source"
class BACKEND_API CPropertySource : public IProperty {
	wxVariantData* CreateVariantData(const IPropertyObject* property, const eValueTypes& type) const;
	wxVariantData* CreateVariantData(const IPropertyObject* property, const class_identifier_t& id) const;
	wxVariantData* CreateVariantData(const IPropertyObject* property, const CTypeDescription& typeDesc) const;
	wxVariantData* CreateVariantData(const IPropertyObject* property, const meta_identifier_t& id) const;
	wxVariantData* CreateVariantData(const IPropertyObject* property, const CGuid& id, bool fillTypeDesc = true) const;
public:

#pragma region _value_
	meta_identifier_t GetValueAsSource() const;
	CGuid GetValueAsSourceGuid() const;
	CTypeDescription& GetValueAsTypeDesc(bool fillTypeDesc = true) const;

	void SetValue(const meta_identifier_t& val);
	void SetValue(const CGuid& val, bool fillTypeDesc = true);
	void SetValue(const CTypeDescription& val);
#pragma endregion 

	class IMetaObjectAttribute* GetSourceAttributeObject() const;

	CPropertySource(CPropertyCategory* cat, const wxString& name, const eValueTypes& type = eValueTypes::TYPE_STRING)
		: IProperty(cat, name, CreateVariantData(cat->GetPropertyObject(), type))
	{
	}

	CPropertySource(CPropertyCategory* cat, const wxString& name, const class_identifier_t& clsid)
		: IProperty(cat, name, CreateVariantData(cat->GetPropertyObject(), clsid))
	{
	}

	CPropertySource(CPropertyCategory* cat, const wxString& name, const wxString& label, const eValueTypes& type = eValueTypes::TYPE_STRING)
		: IProperty(cat, name, label, CreateVariantData(cat->GetPropertyObject(), type))
	{
	}

	CPropertySource(CPropertyCategory* cat, const wxString& name, const wxString& label, const class_identifier_t& clsid)
		: IProperty(cat, name, label, CreateVariantData(cat->GetPropertyObject(), clsid))
	{
	}

	CPropertySource(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString, const eValueTypes& type = eValueTypes::TYPE_STRING)
		: IProperty(cat, name, label, helpString, CreateVariantData(cat->GetPropertyObject(), type))
	{
	}

	CPropertySource(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString, const class_identifier_t& clsid)
		: IProperty(cat, name, label, helpString, CreateVariantData(cat->GetPropertyObject(), clsid))
	{
	}

	virtual bool IsEmptyProperty() const;

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxPGSourceDataProperty(m_owner, m_propLabel, m_propName, m_propValue);
	}

	// set/get property data
	virtual bool SetDataValue(const CValue& varPropVal);
	virtual bool GetDataValue(CValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer);
};

#endif