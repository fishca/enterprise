#ifndef __REPORT_H__
#define __REPORT_H__

#include "commonObject.h"

class CValueMetaObjectReport : public IValueMetaObjectRecordDataExt {
	wxDECLARE_DYNAMIC_CLASS(CValueMetaObjectReport);
protected:
	enum
	{
		ID_METATREE_OPEN_MODULE = 19000,
		ID_METATREE_OPEN_MANAGER = 19001,
	};

public:

	enum
	{
		eFormReport = 1,
	};

	virtual CFormTypeList GetFormType() const override {
		CFormTypeList formList;
		formList.AppendItem(wxT("formReport"), _("Form report"), eFormReport);
		return formList;
	}

public:

	form_identifier_t GetDefFormObject() const {
		return m_propertyDefFormObject->GetValueAsInteger();
	}

	void SetDefFormObject(const form_identifier_t& id) const {
		m_propertyDefFormObject->SetValue(id);
	}

	CValueMetaObjectReport();
	virtual ~CValueMetaObjectReport();

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
	//support form
	virtual IBackendValueForm* GetObjectForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);
#pragma endregion

	//get module object in compose object 
	virtual CValueMetaObjectModule* GetModuleObject() const { return m_propertyModuleObject->GetMetaObject(); }
	virtual CValueMetaObjectCommonModule* GetModuleManager() const { return m_propertyModuleManager->GetMetaObject(); }

	//prepare menu for item
	virtual bool PrepareContextMenu(wxMenu* defaultMenu);
	virtual void ProcessCommand(unsigned int id);

	//get command section 
	virtual EInterfaceCommandSection GetCommandSection() const { return EInterfaceCommandSection::EInterfaceCommandSection_Report; }

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
			if (eFormReport == object->GetTypeForm()) {
				prop->AppendItem(
					object->GetName(),
					object->GetMetaID(),
					object->GetIcon(),
					object);
			}
		}

		return true;
	}

	CPropertyInnerModule<CValueMetaObjectModule>* m_propertyModuleObject = IPropertyObject::CreateProperty<CPropertyInnerModule<CValueMetaObjectModule>>(m_categorySecondary, IValueMetaObjectCompositeData::CreateMetaObjectAndSetParent<CValueMetaObjectModule>(wxT("objectModule"), _("Object module")));
	CPropertyInnerModule<CValueMetaObjectManagerModule>* m_propertyModuleManager = IPropertyObject::CreateProperty<CPropertyInnerModule<CValueMetaObjectManagerModule>>(m_categorySecondary, IValueMetaObjectCompositeData::CreateMetaObjectAndSetParent<CValueMetaObjectManagerModule>(wxT("managerModule"), _("Manager module")));

	CPropertyCategory* m_categoryForm = IPropertyObject::CreatePropertyCategory(wxT("presetValues"), _("Preset values"));
	CPropertyList* m_propertyDefFormObject = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormObject"), _("Default Object Form"), &CValueMetaObjectReport::FillFormObject);

	friend class CValueRecordDataObjectReport;
	friend class IMetaData;
};

#define default_meta_id 10 //for reports

class CValueMetaObjectExternalReport : public CValueMetaObjectReport {
	wxDECLARE_DYNAMIC_CLASS(CValueMetaObjectExternalReport);
public:
	CValueMetaObjectExternalReport() : CValueMetaObjectReport() {
		m_metaId = default_meta_id;
	}

	//ñreate from file?
	virtual bool IsExternalCreate() const { return true; }
};

//********************************************************************************************
//*                                      Object                                              *
//********************************************************************************************

class CValueRecordDataObjectReport : public IValueRecordDataObjectExt {
	CValueRecordDataObjectReport(const CValueRecordDataObjectReport& source);
	CValueRecordDataObjectReport(CValueMetaObjectReport* metaObject);
public:

#pragma region _form_builder_h_
	//support show 
	virtual void ShowFormValue(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr);
	virtual IBackendValueForm* GetFormValue(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr);
#pragma endregion

	//support actionData
	virtual CActionCollection GetActionCollection(const form_identifier_t& formType);
	virtual void ExecuteAction(const action_identifier_t& action, IBackendValueForm* srcForm);

protected:
	friend class CValue;
	friend class CValueMetaObjectReport;
	friend class CValueModuleManagerExternalReport;
};

#endif