#ifndef __PROPERTY_GENERATION_H__
#define __PROPERTY_GENERATION_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropGeneration.h"

//base property for "generation"
class BACKEND_API CPropertyGeneration : public IProperty {
	wxVariantData* CreateVariantData(IPropertyObject* property, const CMetaDescription& typeDesc = CMetaDescription()) const;
public:

	CMetaDescription& GetValueAsMetaDesc() const;
	void SetValue(const CMetaDescription& val);


	CPropertyGeneration(CPropertyCategory* cat, const wxString& name)
		: IProperty(cat, name, CreateVariantData(cat->GetPropertyObject()))
	{
	}

	CPropertyGeneration(CPropertyCategory* cat, const wxString& name, const wxString& label)
		: IProperty(cat, name, label, CreateVariantData(cat->GetPropertyObject()))
	{
	}

	CPropertyGeneration(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString)
		: IProperty(cat, name, label, helpString, CreateVariantData(cat->GetPropertyObject()))
	{
	}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxPGGenerationProperty(m_owner, m_propLabel, m_propName, m_propValue);
	}

	// set/get property data
	virtual bool SetDataValue(const CValue& varPropVal);
	virtual bool GetDataValue(CValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer);
};

#endif