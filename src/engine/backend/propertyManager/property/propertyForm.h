#ifndef __PROPERTY_FORM_H__
#define __PROPERTY_FORM_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropHyperLink.h"

//base property for "form"
class BACKEND_API CPropertyForm : public IProperty {
	wxVariantData* CreateVariantData();
public:

	wxString GetValueAsString() const;
	wxMemoryBuffer& GetValueAsMemoryBuffer() const;
	void SetValue(const wxString& val);
	void SetValue(const wxMemoryBuffer& val);

	CPropertyForm(CPropertyCategory* cat, const wxString& name)
		: IProperty(cat, name, CreateVariantData())
	{
	}

	CPropertyForm(CPropertyCategory* cat, const wxString& name, const wxString& label)
		: IProperty(cat, name, label, CreateVariantData())
	{
	}

	CPropertyForm(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString)
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

	//copy & paste object in control 
	virtual bool PasteData(CMemoryReader& reader); 
	virtual bool CopyData(CMemoryWriter& writer); 
};

#endif