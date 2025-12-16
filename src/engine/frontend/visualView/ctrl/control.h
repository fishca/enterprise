#ifndef _BASE_CONTROL_H_
#define _BASE_CONTROL_H_

#include "frame.h"

class FRONTEND_API IValueControl : public IValueFrame {
	wxDECLARE_ABSTRACT_CLASS(IValueControl);
protected:
	CPropertyName* m_propertyName = IPropertyObject::CreateProperty<CPropertyName>(m_category, wxT("name"), _("Name"), _("The name of the control."), wxEmptyString);
public:

	IValueControl();
	virtual ~IValueControl();

	/**
	* Support control name
	*/

	virtual bool GetControlNameAsString(wxString& result) const {
		return m_propertyName->GetValueAsString(result);
	}

	virtual bool SetControlNameAsString(const wxString& result) const { 
		m_propertyName->SetValue(result); 
		return true; 
	}

	/**
	* Property events
	*/
	virtual bool OnPropertyChanging(IProperty* property, const wxVariant& newValue);
	virtual void OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue);

	/**
	* Support form
	*/
	virtual CValueForm* GetOwnerForm() const {
		return m_formOwner;
	}

	virtual void SetOwnerForm(CValueForm* ownerForm);

	// allow getting value in control
	virtual bool HasValueInControl() const { return false; }

	/*
	* Get/set value in control
	*/
	virtual bool SetControlValue(const CValue& varControlVal = CValue()) { return false; }
	virtual bool GetControlValue(CValue& pvarControlVal) const {
		return false;
	}

	//get metaData
	virtual IMetaData* GetMetaData() const override;

	/**
	* Can delete object
	*/
	virtual bool CanDeleteControl() const {
		return true;
	}

	//runtime 
	virtual CProcUnit* GetFormProcUnit() const;

	/**
	* Get type form
	*/
	virtual form_identifier_t GetTypeForm() const;

protected:

	//frame owner 
	CValueForm* m_formOwner;
};

#endif // !_BASE_H_
