#ifndef __PROPERTY_FORM_H__
#define __PROPERTY_FORM_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropHyperLink.h"

//base property for "form"
class BACKEND_API ibPropertyForm : public ibProperty {
	wxVariantData* CreateVariantData();
public:

	wxString GetValueAsString() const;
	wxMemoryBuffer& GetValueAsMemoryBuffer() const;
	void SetValue(const wxString& val);
	void SetValue(const wxMemoryBuffer& val);

	ibPropertyForm(ibPropertyCategory* cat, const wxString& name)
		: ibProperty(cat, name, CreateVariantData())
	{
	}

	ibPropertyForm(ibPropertyCategory* cat, const wxString& name, const wxString& label)
		: ibProperty(cat, name, label, CreateVariantData())
	{
	}

	ibPropertyForm(ibPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString)
		: ibProperty(cat, name, label, helpString, CreateVariantData())
	{
	}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxPGHyperLinkProperty(m_owner, m_propLabel, m_propName, m_propValue);
	}

	// set/get property data
	virtual bool SetDataValue(const ibValue& varPropVal);
	virtual bool GetDataValue(ibValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(ibReaderMemory& reader);
	virtual bool SaveData(ibWriterMemory& writer);

	//copy & paste object in control 
	virtual bool PasteData(ibReaderMemory& reader); 
	virtual bool CopyData(ibWriterMemory& writer); 
};

#endif