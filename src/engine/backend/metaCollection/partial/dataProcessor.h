#ifndef __DATA_PROCESSOR_H__
#define __DATA_PROCESSOR_H__

#include "commonObject.h"

class CMetaObjectDataProcessor : public IMetaObjectRecordDataExt {
	wxDECLARE_DYNAMIC_CLASS(CMetaObjectDataProcessor);
protected:

	enum
	{
		ID_METATREE_OPEN_MODULE = 19000,
		ID_METATREE_OPEN_MANAGER = 19001,
	};

public:

	enum
	{
		eFormDataProcessor = 1,
	};

	virtual CFormTypeList GetFormType() const override {
		CFormTypeList formList;
		formList.AppendItem(wxT("formDataProcessor"), _("Form data processor"), eFormDataProcessor);
		return formList;
	}

protected:

	CPropertyInnerModule<CMetaObjectModule>* m_propertyModuleObject = IPropertyObject::CreateProperty<CPropertyInnerModule<CMetaObjectModule>>(m_categorySecondary, IMetaObjectCompositeData::CreateMetaObjectAndSetParent<CMetaObjectModule>(wxT("objectModule"), _("Object module")));
	CPropertyInnerModule<CMetaObjectManagerModule>* m_propertyModuleManager = IPropertyObject::CreateProperty<CPropertyInnerModule<CMetaObjectManagerModule>>(m_categorySecondary, IMetaObjectCompositeData::CreateMetaObjectAndSetParent<CMetaObjectManagerModule>(wxT("managerModule"), _("Manager module")));

	CPropertyCategory* m_categoryForm = IPropertyObject::CreatePropertyCategory(wxT("defaultForms"), _("Default forms"));
	CPropertyList* m_propertyDefFormObject = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormObject"), _("Default object"), &CMetaObjectDataProcessor::GetFormObject, wxNOT_FOUND);

private:
	bool GetFormObject(CPropertyList* prop);
public:

	form_identifier_t GetDefFormObject() const {
		return m_propertyDefFormObject->GetValueAsInteger();
	}

	void SetDefFormObject(const form_identifier_t& id) const {
		m_propertyDefFormObject->SetValue(id);
	}

	CMetaObjectDataProcessor();
	virtual ~CMetaObjectDataProcessor();

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//events: 
	virtual bool OnCreateMetaObject(IMetaData* metaData, int flags);
	virtual bool OnLoadMetaObject(IMetaData* metaData);
	virtual bool OnSaveMetaObject(int flags);
	virtual bool OnDeleteMetaObject();

	//for designer 
	virtual bool OnReloadMetaObject();

	//module manager is started or exit 
	virtual bool OnBeforeRunMetaObject(int flags);
	virtual bool OnAfterRunMetaObject(int flags);

	virtual bool OnBeforeCloseMetaObject();
	virtual bool OnAfterCloseMetaObject();

	//form events 
	virtual void OnCreateFormObject(IMetaObjectForm* metaForm);
	virtual void OnRemoveMetaForm(IMetaObjectForm* metaForm);

	//create associate value 
	virtual IMetaObjectForm* GetDefaultFormByID(const form_identifier_t& id) const;

#pragma region _form_builder_h_
	//suppot form
	virtual IBackendValueForm* GetObjectForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);
#pragma endregion

	//get module object in compose object 
	virtual CMetaObjectModule* GetModuleObject() const { return m_propertyModuleObject->GetMetaObject(); }
	virtual CMetaObjectCommonModule* GetModuleManager() const { return m_propertyModuleManager->GetMetaObject(); }

	//prepare menu for item
	virtual bool PrepareContextMenu(wxMenu* defaultMenu);
	virtual void ProcessCommand(unsigned int id);

protected:

	//create object data with meta form
	virtual ISourceDataObject* CreateSourceObject(IMetaObjectForm* metaObject);

	//create empty object
	virtual IRecordDataObjectExt* CreateObjectExtValue();  //create object 

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

protected:
	friend class IMetaData;
	friend class CRecordDataObjectDataProcessor;
};

#define default_meta_id 10 //for dataProcessors

class CMetaObjectExternalDataProcessor : public CMetaObjectDataProcessor {
	wxDECLARE_DYNAMIC_CLASS(CMetaObjectExternalDataProcessor);
public:
	CMetaObjectExternalDataProcessor() : CMetaObjectDataProcessor() {
		m_metaId = default_meta_id;
	}

	//ñreate from file?
	virtual bool IsExternalCreate() const { return true; }
};

//********************************************************************************************
//*                                      Object                                              *
//********************************************************************************************

class CRecordDataObjectDataProcessor : public IRecordDataObjectExt {
	CRecordDataObjectDataProcessor(CMetaObjectDataProcessor* metaObject);
	CRecordDataObjectDataProcessor(const CRecordDataObjectDataProcessor& source);
public:

#pragma region _form_builder_h_
	//support show 
	virtual void ShowFormValue(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr);
	virtual IBackendValueForm* GetFormValue(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr);
#pragma endregion

	//support actionData
	virtual CActionCollection GetActionCollection(const form_identifier_t& formType);
	virtual void ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm);

protected:

	friend class CValue;
	friend class CMetaObjectDataProcessor;
	friend class CModuleManagerExternalDataProcessor;
};

#endif