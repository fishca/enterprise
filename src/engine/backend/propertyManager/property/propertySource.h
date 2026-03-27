#ifndef __PROPERTY_SOURCE_H__
#define __PROPERTY_SOURCE_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropSource.h"

//////////////////////////////////////////////////////////////////
struct ibTypeDescription;
//////////////////////////////////////////////////////////////////

//base property for "source"
class BACKEND_API ibPropertySource : public ibProperty {
	wxVariantData* CreateVariantData(const ibPropertyObject* property, const ibValueTypes& type) const;
	wxVariantData* CreateVariantData(const ibPropertyObject* property, const ibClassID& id) const;
	wxVariantData* CreateVariantData(const ibPropertyObject* property, const ibTypeDescription& typeDesc) const;
	wxVariantData* CreateVariantData(const ibPropertyObject* property, const ibMetaID& id) const;
	wxVariantData* CreateVariantData(const ibPropertyObject* property, const ibGuid& id, bool fillTypeDesc = true) const;
public:

#pragma region _value_
	ibMetaID GetValueAsSource() const;
	ibGuid GetValueAsSourceGuid() const;
	ibTypeDescription& GetValueAsTypeDesc(bool fillTypeDesc = true) const;

	void SetValue(const ibMetaID& val);
	void SetValue(const ibGuid& val, bool fillTypeDesc = true);
	void SetValue(const ibTypeDescription& val);
#pragma endregion 

	class ibValueMetaObjectAttributeBase* GetSourceAttributeObject() const;

	ibPropertySource(ibPropertyCategory* cat, const wxString& name, const ibValueTypes& type = ibValueTypes::TYPE_STRING)
		: ibProperty(cat, name, CreateVariantData(cat->GetPropertyObject(), type))
	{
	}

	ibPropertySource(ibPropertyCategory* cat, const wxString& name, const ibClassID& clsid)
		: ibProperty(cat, name, CreateVariantData(cat->GetPropertyObject(), clsid))
	{
	}

	ibPropertySource(ibPropertyCategory* cat, const wxString& name, const wxString& label, const ibValueTypes& type = ibValueTypes::TYPE_STRING)
		: ibProperty(cat, name, label, CreateVariantData(cat->GetPropertyObject(), type))
	{
	}

	ibPropertySource(ibPropertyCategory* cat, const wxString& name, const wxString& label, const ibClassID& clsid)
		: ibProperty(cat, name, label, CreateVariantData(cat->GetPropertyObject(), clsid))
	{
	}

	ibPropertySource(ibPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString, const ibValueTypes& type = ibValueTypes::TYPE_STRING)
		: ibProperty(cat, name, label, helpString, CreateVariantData(cat->GetPropertyObject(), type))
	{
	}

	ibPropertySource(ibPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString, const ibClassID& clsid)
		: ibProperty(cat, name, label, helpString, CreateVariantData(cat->GetPropertyObject(), clsid))
	{
	}

	virtual bool IsEmptyProperty() const;

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxPGSourceDataProperty(m_owner, m_propLabel, m_propName, m_propValue);
	}

	// set/get property data
	virtual bool SetDataValue(const ibValue& varPropVal);
	virtual bool GetDataValue(ibValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(ibReaderMemory& reader);
	virtual bool SaveData(ibWriterMemory& writer);
};

#endif