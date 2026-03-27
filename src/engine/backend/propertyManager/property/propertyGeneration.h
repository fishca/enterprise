#ifndef __PROPERTY_GENERATION_H__
#define __PROPERTY_GENERATION_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropGeneration.h"

//base property for "generation"
class BACKEND_API ibPropertyGeneration : public ibProperty {
	wxVariantData* CreateVariantData(ibPropertyObject* property, const ibMetaDescription& typeDesc = ibMetaDescription()) const;
public:

	ibMetaDescription& GetValueAsMetaDesc() const;
	void SetValue(const ibMetaDescription& val);


	ibPropertyGeneration(ibPropertyCategory* cat, const wxString& name)
		: ibProperty(cat, name, CreateVariantData(cat->GetPropertyObject()))
	{
	}

	ibPropertyGeneration(ibPropertyCategory* cat, const wxString& name, const wxString& label)
		: ibProperty(cat, name, label, CreateVariantData(cat->GetPropertyObject()))
	{
	}

	ibPropertyGeneration(ibPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString)
		: ibProperty(cat, name, label, helpString, CreateVariantData(cat->GetPropertyObject()))
	{
	}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxPGGenerationProperty(m_owner, m_propLabel, m_propName, m_propValue);
	}

	// set/get property data
	virtual bool SetDataValue(const ibValue& varPropVal);
	virtual bool GetDataValue(ibValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(ibReaderMemory& reader);
	virtual bool SaveData(ibWriterMemory& writer);
};

#endif