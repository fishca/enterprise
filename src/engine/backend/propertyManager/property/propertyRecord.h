#ifndef __PROPERTY_RECORD_H__
#define __PROPERTY_RECORD_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropRecord.h"

//base property for "record"
class BACKEND_API CPropertyRecord : public IProperty {
	wxVariantData* CreateVariantData(IPropertyObject* property, const CMetaDescription& typeDesc = CMetaDescription()) const;
public:

	CMetaDescription& GetValueAsMetaDesc() const;
	CMetaDescription& GetValueAsMetaDesc(const wxVariant& val) const;
	void SetValue(const CMetaDescription& val);

	CPropertyRecord(CPropertyCategory* cat, const wxString& name) : IProperty(cat, name, CreateVariantData(cat->GetPropertyObject())) {}
	CPropertyRecord(CPropertyCategory* cat, const wxString& name, const wxString& label) : IProperty(cat, name, label, CreateVariantData(cat->GetPropertyObject())) {}
	CPropertyRecord(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString) : IProperty(cat, name, label, helpString, CreateVariantData(cat->GetPropertyObject())) {}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxPGRecordProperty(m_owner, m_propLabel, m_propName, m_propValue);
	}

	// set/get property data
	virtual bool SetDataValue(const CValue& varPropVal);
	virtual bool GetDataValue(CValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer);
};

#endif