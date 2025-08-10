#ifndef _METAFORMOBJECT_H__
#define _METAFORMOBJECT_H__

#include "metaModuleObject.h"
#include "backend/uniqueKey.h"

#define defaultFormType wxNOT_FOUND
#define formDefaultName _("form")

class BACKEND_API ISourceDataObject;

class BACKEND_API IMetaObjectForm : public IMetaObjectModule {
	wxDECLARE_ABSTRACT_CLASS(IMetaObjectForm);
private:

	enum
	{
		ID_METATREE_OPEN_FORM = 19000,
	};

public:

	IMetaObjectForm(const wxString& strName = wxEmptyString, const wxString& synonym = wxEmptyString, const wxString& comment = wxEmptyString);

	bool LoadFormData(IBackendValueForm* value) const;
	bool SaveFormData(IBackendValueForm* value);

#pragma region _form_creator_h_

	static IBackendValueForm* CreateAndBuildForm(const IMetaObjectForm* creator,
		IBackendControlFrame* ownerControl = nullptr,
		ISourceDataObject* srcObject = nullptr, const CUniqueKey& formGuid = wxNullGuid);

	static IBackendValueForm* CreateAndBuildForm(const IMetaObjectForm* creator, const form_identifier_t& form_id = defaultFormType,
		IBackendControlFrame* ownerControl = nullptr,
		ISourceDataObject* srcObject = nullptr, const CUniqueKey& formGuid = wxNullGuid);

#pragma endregion 

	//set module code 
	virtual void SetModuleText(const wxString& moduleText) = 0;
	virtual wxString GetModuleText() const = 0;

	//set form data 
	virtual void SetFormData(const wxMemoryBuffer& formData) = 0;
	virtual wxMemoryBuffer GetFormData() const = 0;

	// copy form data
	wxMemoryBuffer CopyFormData() const;
	bool PasteFormData();

	/**
	* Get type form
	*/
	virtual form_identifier_t GetTypeForm() const = 0;

	//prepare menu for item
	virtual bool PrepareContextMenu(wxMenu* defaultMenu);
	virtual void ProcessCommand(unsigned int id);

protected:

	virtual bool LoadData(CMemoryReader& reader) = 0;
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter()) = 0;
};

class BACKEND_API CMetaObjectForm : public IMetaObjectForm {
	wxDECLARE_DYNAMIC_CLASS(CMetaObjectForm);
private:

	friend class CVisualEditor;

	friend class IMetaObjectRecordData;
	friend class IListDataObject;
	friend class IRecordDataObject;

private:
	bool GetFormType(CPropertyList* prop);
protected:

	CPropertyForm* m_propertyForm = IPropertyObject::CreateProperty<CPropertyForm>(m_categorySecondary, wxT("formData"), _("form"));

	CPropertyCategory* m_categoryForm = IPropertyObject::CreatePropertyCategory(wxT("form"), _("form"));
	CPropertyList* m_properyFormType = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("formType"), _("type"), &CMetaObjectForm::GetFormType, wxNOT_FOUND);

public:

	CMetaObjectForm(const wxString& strName = wxEmptyString, const wxString& synonym = wxEmptyString, const wxString& comment = wxEmptyString);

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	/**
	* Property events
	*/
	virtual void OnPropertyCreated(IProperty* property);
	virtual void OnPropertySelected(IProperty* property);
	virtual void OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue);

	//events:
	virtual bool OnCreateMetaObject(IMetaData* metaData, int flags);
	virtual bool OnLoadMetaObject(IMetaData* metaData);
	virtual bool OnSaveMetaObject();
	virtual bool OnDeleteMetaObject();

	//module manager is started or exit 
	virtual bool OnBeforeRunMetaObject(int flags);
	virtual bool OnAfterRunMetaObject(int flags);

	virtual bool OnBeforeCloseMetaObject();
	virtual bool OnAfterCloseMetaObject();

	//get property
	virtual IProperty* GetModuleProperty() const { return m_propertyForm; }

	//set module code 
	virtual void SetModuleText(const wxString& moduleText) { m_propertyForm->SetValue(moduleText); }
	virtual wxString GetModuleText() const { return m_propertyForm->GetValueAsString(); }

	//set form data 
	virtual void SetFormData(const wxMemoryBuffer& formData) { m_propertyForm->SetValue(formData); }
	virtual wxMemoryBuffer GetFormData() const { return m_propertyForm->GetValueAsMemoryBuffer(); }

	/**
	* Get type form
	*/
	virtual form_identifier_t GetTypeForm() const {
		return m_properyFormType->GetValueAsInteger();
	}

protected:

	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());
};

class BACKEND_API CMetaObjectCommonForm : public IMetaObjectForm {
	wxDECLARE_DYNAMIC_CLASS(CMetaObjectCommonForm);
protected:

	CPropertyForm* m_propertyForm = IPropertyObject::CreateProperty<CPropertyForm>(m_categorySecondary, wxT("formData"), _("form"));

	CRole* m_roleUse = IMetaObject::CreateRole("use", _("use"));

public:

	CMetaObjectCommonForm(const wxString& strName = wxEmptyString, const wxString& synonym = wxEmptyString, const wxString& comment = wxEmptyString);

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//events:
	virtual bool OnCreateMetaObject(IMetaData* metaData, int flags);

	//module manager is started or exit 
	virtual bool OnBeforeRunMetaObject(int flags);
	virtual bool OnAfterRunMetaObject(int flags);

	virtual bool OnBeforeCloseMetaObject();
	virtual bool OnAfterCloseMetaObject();

	//get property
	virtual IProperty* GetModuleProperty() const { return m_propertyForm; }

	//set module code 
	virtual void SetModuleText(const wxString& moduleText) { m_propertyForm->SetValue(moduleText); }
	virtual wxString GetModuleText() const { return m_propertyForm->GetValueAsString(); }

	//set form data 
	virtual void SetFormData(const wxMemoryBuffer& formData) { m_propertyForm->SetValue(formData); }
	virtual wxMemoryBuffer GetFormData() const { return m_propertyForm->GetValueAsMemoryBuffer(); }

	/**
	* Get type form
	*/
	virtual form_identifier_t GetTypeForm() const { return defaultFormType; }

protected:

	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());
};

#endif 