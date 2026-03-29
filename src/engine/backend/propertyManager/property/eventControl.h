#ifndef __EVENT_CONTROL_H__
#define __EVENT_CONTROL_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropEvent.h"

//base property for "event"
class BACKEND_API ibEventControl : public ibEvent {
public:

	ibEventControl(ibPropertyCategory* cat, const wxString& name, const wxArrayString& args)
		: ibEvent(cat, name, args, wxEmptyString)
	{
	}

	ibEventControl(ibPropertyCategory* cat, const wxString& name, const wxString& label, const wxArrayString& args)
		: ibEvent(cat, name, label, args, wxEmptyString)
	{
	}

	ibEventControl(ibPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString, const wxArrayString& args)
		: ibEvent(cat, name, label, helpString, args, wxEmptyString)
	{
	}

	virtual bool IsEmptyProperty() const { return m_propValue.GetString().IsEmpty(); }

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxEventProperty(m_propLabel, m_propName, m_propValue);
	}

	// set/get property data
	virtual bool SetDataValue(const ibValue& varPropVal);
	virtual bool GetDataValue(ibValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(ibReaderMemory& reader);
	virtual bool SaveData(ibWriterMemory& writer);
};

#endif