#ifndef __REPORT_H__
#define __REPORT_H__

#include "commonObject.h"

class CMetaObjectReport : public IMetaObjectRecordDataExt {
	wxDECLARE_DYNAMIC_CLASS(CMetaObjectReport);
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

protected:

	CPropertyInnerModule<CMetaObjectModule>* m_propertyModuleObject = IPropertyObject::CreateProperty<CPropertyInnerModule<CMetaObjectModule>>(m_categorySecondary, IMetaObjectCompositeData::CreateMetaObjectAndSetParent<CMetaObjectModule>(wxT("objectModule"), _("object module")));
	CPropertyInnerModule<CMetaObjectManagerModule>* m_propertyModuleManager = IPropertyObject::CreateProperty<CPropertyInnerModule<CMetaObjectManagerModule>>(m_categorySecondary, IMetaObjectCompositeData::CreateMetaObjectAndSetParent<CMetaObjectManagerModule>(wxT("managerModule"), _("manager module")));

	CPropertyCategory* m_categoryForm = IPropertyObject::CreatePropertyCategory(wxT("defaultForms"), _("default forms"));
	CPropertyList* m_propertyDefFormObject = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormObject"), _("default object"), &CMetaObjectReport::GetFormObject, wxNOT_FOUND);

private:
	bool GetFormObject(CPropertyList* prop);
public:

	form_identifier_t GetDefFormObject() const {
		return m_propertyDefFormObject->GetValueAsInteger();
	}

	void SetDefFormObject(const form_identifier_t& id) const {
		m_propertyDefFormObject->SetValue(id);
	}

	CMetaObjectReport(int objMode = METAOBJECT_NORMAL);
	virtual ~CMetaObjectReport();

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
	//support form
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
	friend class CRecordDataObjectReport;
};

#define default_meta_id 10 //for reports

class CMetaObjectReportExternal : public CMetaObjectReport {
	wxDECLARE_DYNAMIC_CLASS(CMetaObjectReportExternal);
public:
	CMetaObjectReportExternal() : CMetaObjectReport(METAOBJECT_EXTERNAL) {
		m_metaId = default_meta_id;
	}
};

//********************************************************************************************
//*                                      Object                                              *
//********************************************************************************************

class CRecordDataObjectReport : public IRecordDataObjectExt {
	CRecordDataObjectReport(const CRecordDataObjectReport& source);
	CRecordDataObjectReport(CMetaObjectReport* metaObject);
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
	friend class CMetaObjectReport;
	friend class CModuleManagerExternalReport;
};

#endif