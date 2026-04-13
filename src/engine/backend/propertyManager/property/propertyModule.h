#ifndef __PROPERTY_MODULE_H__
#define __PROPERTY_MODULE_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropHyperLink.h"

//base property for "module"
class BACKEND_API CPropertyModule : public IProperty {
	wxVariantData* CreateVariantData();
public:

	wxString GetValueAsString() const;
	void SetValue(const wxString& val);

	CPropertyModule(CPropertyCategory* cat, const wxString& name)
		: IProperty(cat, name, CreateVariantData())
	{
	}

	CPropertyModule(CPropertyCategory* cat, const wxString& name, const wxString& label)
		: IProperty(cat, name, label, CreateVariantData())
	{
	}

	CPropertyModule(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString)
		: IProperty(cat, name, label, helpString, CreateVariantData())
	{
	}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxPGHyperLinkProperty(m_owner, m_propLabel, m_propName, m_propValue);
	}

	// set/get property data
	virtual bool SetDataValue(const CValue& varPropVal);
	virtual bool GetDataValue(CValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer);
};

#endif