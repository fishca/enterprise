#ifndef __DATA_PROCESSOR_H__
#define __DATA_PROCESSOR_H__

#include "commonObject.h"

class CValueMetaObjectDataProcessor : public IValueMetaObjectRecordDataExt {
	wxDECLARE_DYNAMIC_CLASS(CValueMetaObjectDataProcessor);
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
		formList.AppendItem(wxT("FormDataProcessor"), _("Form data processor"), eFormDataProcessor);
		return formList;
	}

public:

	form_identifier_t GetDefFormObject() const {
		return m_propertyDefFormObject->GetValueAsInteger();
	}

	void SetDefFormObject(const form_identifier_t& id) const {
		m_propertyDefFormObject->SetValue(id);
	}

	CValueMetaObjectDataProcessor();
	virtual ~CValueMetaObjectDataProcessor();

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
	virtual void OnCreateFormObject(IValueMetaObjectForm* metaForm);
	virtual void OnRemoveMetaForm(IValueMetaObjectForm* metaForm);

	//create associate value 
	virtual IValueMetaObjectForm* GetDefaultFormByID(const form_identifier_t& id) const;

#pragma region _form_builder_h_
	//suppot form
	virtual IBackendValueForm* GetObjectForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);
#pragma endregion

	//get module object in compose object 
	virtual CValueMetaObjectModule* GetModuleObject() const { return m_propertyModuleObject->GetMetaObject(); }
	virtual CValueMetaObjectCommonModule* GetModuleManager() const { return m_propertyModuleManager->GetMetaObject(); }

	//prepare menu for item
	virtual bool PrepareContextMenu(wxMenu* defaultMenu);
	virtual void ProcessCommand(unsigned int id);

protected:

	//create manager
	virtual IValueManagerDataObject* CreateManagerDataObjectValue();

	//create empty object
	virtual IValueRecordDataObjectExt* CreateObjectExtValue();  //create object 

	//create object data with meta form
	virtual ISourceDataObject* CreateSourceObject(IValueMetaObjectForm* metaObject);

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

private:

	bool FillFormObject(CPropertyList* prop) {
		for (auto object : GetFormArrayObject()) {
			if (!object->IsAllowed()) continue;
			if (eFormDataProcessor == object->GetTypeForm()) {
				prop->AppendItem(
					object->GetName(),
					object->GetMetaID(),
					object->GetIcon(),
					object);
			}
		}
		return true;
	}

	CPropertyInnerModule<CValueMetaObjectModule>* m_propertyModuleObject = IPropertyObject::CreateProperty<CPropertyInnerModule<CValueMetaObjectModule>>(m_categorySecondary, IValueMetaObjectCompositeData::CreateMetaObjectAndSetParent<CValueMetaObjectModule>(wxT("ObjectModule"), _("Object module")));
	CPropertyInnerModule<CValueMetaObjectManagerModule>* m_propertyModuleManager = IPropertyObject::CreateProperty<CPropertyInnerModule<CValueMetaObjectManagerModule>>(m_categorySecondary, IValueMetaObjectCompositeData::CreateMetaObjectAndSetParent<CValueMetaObjectManagerModule>(wxT("ManagerModule"), _("Manager module")));

	CPropertyCategory* m_categoryForm = IPropertyObject::CreatePropertyCategory(wxT("PresetValues"), _("Preset values"));
	CPropertyList* m_propertyDefFormObject = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("DefaultFormObject"), _("Default Object Form"), &CValueMetaObjectDataProcessor::FillFormObject);

	friend class CValueRecordDataObjectDataProcessor;
	friend class IMetaData;
};

#define default_meta_id 10 //for dataProcessors

class CValueMetaObjectExternalDataProcessor : public CValueMetaObjectDataProcessor {
	wxDECLARE_DYNAMIC_CLASS(CValueMetaObjectExternalDataProcessor);
public:
	CValueMetaObjectExternalDataProcessor() : CValueMetaObjectDataProcessor() {
		m_metaId = default_meta_id;
	}

	//ñreate from file?
	virtual bool IsExternalCreate() const { return true; }
};

//********************************************************************************************
//*                                      Object                                              *
//********************************************************************************************

class CValueRecordDataObjectDataProcessor : public IValueRecordDataObjectExt {
	CValueRecordDataObjectDataProcessor(CValueMetaObjectDataProcessor* metaObject);
	CValueRecordDataObjectDataProcessor(const CValueRecordDataObjectDataProcessor& source);
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
	friend class CValueMetaObjectDataProcessor;
	friend class CValueModuleManagerExternalDataProcessor;
};

#endif