#ifndef __CATALOG_H__
#define __CATALOG_H__

#include "commonObject.h"
#include "reference/reference.h"

//********************************************************************************************
//*                                  Factory & metaData                                      *
//********************************************************************************************

class CValueMetaObjectCatalog : public IValueMetaObjectRecordDataHierarchyMutableRef {
	wxDECLARE_DYNAMIC_CLASS(CValueMetaObjectCatalog);
private:
	enum
	{
		ID_METATREE_OPEN_MODULE = 19000,
		ID_METATREE_OPEN_MANAGER = 19001,
	};

	enum
	{
		eFormObject = 1,
		eFormList,
		eFormSelect,
		eFormFolder,
		eFormFolderSelect
	};

	virtual CFormTypeList GetFormType() const override {
		CFormTypeList formList;
		formList.AppendItem(wxT("formObject"), _("Form object"), eFormObject);
		formList.AppendItem(wxT("formFolder"), _("Form group"), eFormFolder);
		formList.AppendItem(wxT("formList"), _("Form list"), eFormList);
		formList.AppendItem(wxT("formSelect"), _("Form select"), eFormSelect);
		formList.AppendItem(wxT("formGroupSelect"), _("Form group select"), eFormFolderSelect);
		return formList;
	}

public:

	CValueMetaObjectAttributePredefined* GetCatalogOwner() const { return m_propertyAttributeOwner->GetMetaObject(); }

	//default constructor 
	CValueMetaObjectCatalog();
	virtual ~CValueMetaObjectCatalog();

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

	//get attribute code 
	virtual IValueMetaObjectAttribute* GetAttributeForCode() const {
		return m_propertyAttributeCode->GetMetaObject();
	}

	//create associate value 
	virtual IValueMetaObjectForm* GetDefaultFormByID(const form_identifier_t& id) const;

#pragma region _form_builder_h_
	//support form 
	virtual IBackendValueForm* GetObjectForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);
	virtual IBackendValueForm* GetFolderForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);
	virtual IBackendValueForm* GetListForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);
	virtual IBackendValueForm* GetSelectForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);
	virtual IBackendValueForm* GetFolderSelectForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);
#pragma endregion

	//descriptions...
	wxString GetDataPresentation(const IValueDataObject* objValue) const;

	//get module object in compose object 
	virtual CValueMetaObjectModule* GetModuleObject() const { return m_propertyModuleObject->GetMetaObject(); }
	virtual CValueMetaObjectCommonModule* GetModuleManager() const { return m_propertyModuleManager->GetMetaObject(); }

	/**
	* Property events
	*/
	virtual void OnPropertyCreated(IProperty* property);
	virtual bool OnPropertyChanging(IProperty* property, const wxVariant& newValue);
	virtual void OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue);

protected:

	//predefined array 
	virtual bool FillArrayObjectByPredefined(std::vector<IValueMetaObjectAttribute*>& array) const {

		const CValueMetaObjectAttributePredefined* metaObjectAttributeOwner = GetCatalogOwner();
		if (metaObjectAttributeOwner != nullptr && metaObjectAttributeOwner->GetClsidCount() > 0) {
			array = {
				m_propertyAttributePredefined->GetMetaObject(),
				m_propertyAttributeCode->GetMetaObject(),
				m_propertyAttributeDescription->GetMetaObject(),
				m_propertyAttributeOwner->GetMetaObject(),
				m_propertyAttributeParent->GetMetaObject(),
				m_propertyAttributeIsFolder->GetMetaObject(),
				m_propertyAttributeReference->GetMetaObject(),
				m_propertyAttributeDeletionMark->GetMetaObject(),
			};
		}
		else {
			array = {
				m_propertyAttributePredefined->GetMetaObject(),
				m_propertyAttributeCode->GetMetaObject(),
				m_propertyAttributeDescription->GetMetaObject(),
				m_propertyAttributeParent->GetMetaObject(),
				m_propertyAttributeIsFolder->GetMetaObject(),
				m_propertyAttributeReference->GetMetaObject(),
				m_propertyAttributeDeletionMark->GetMetaObject(),
			};
		};

		return true;
	}

	//searched array 
	virtual bool FillArrayObjectBySearched(std::vector<IValueMetaObjectAttribute*>& array) const {

		array = {
			m_propertyAttributeCode->GetMetaObject(),
			m_propertyAttributeDescription->GetMetaObject(),
		};

		return true;
	}

	//create manager
	virtual IValueManagerDataObject* CreateManagerDataObjectValue();

	//create empty object
	virtual IValueRecordDataObjectFolderRef* CreateObjectRefValue(eObjectMode mode, const CGuid& guid = wxNullGuid);

	//create object data with meta form
	virtual ISourceDataObject* CreateSourceObject(IValueMetaObjectForm* metaObject);

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

	//prepare menu for item
	virtual bool PrepareContextMenu(wxMenu* defaultMenu);
	virtual void ProcessCommand(unsigned int id);

private:

	bool FillFormObject(CPropertyList* prop) {
		for (auto object : GetFormArrayObject()) {
			if (!object->IsAllowed()) continue;
			if (eFormObject == object->GetTypeForm()) {
				prop->AppendItem(
					object->GetName(),
					object->GetMetaID(),
					object->GetIcon(),
					object);
			}
		}
		return true;
	}

	bool FillFormFolder(CPropertyList* prop) {
		for (auto object : GetFormArrayObject()) {
			if (!object->IsAllowed()) continue;
			if (eFormFolder == object->GetTypeForm()) {
				prop->AppendItem(
					object->GetName(),
					object->GetMetaID(),
					object->GetIcon(),
					object);
			}
		}
		return true;
	}

	bool FillFormList(CPropertyList* prop) {
		for (auto object : GetFormArrayObject()) {
			if (!object->IsAllowed()) continue;
			if (eFormList == object->GetTypeForm()) {
				prop->AppendItem(
					object->GetName(),
					object->GetMetaID(),
					object->GetIcon(),
					object);
			}
		}
		return true;
	}

	bool FillFormSelect(CPropertyList* prop) {
		for (auto object : GetFormArrayObject()) {
			if (!object->IsAllowed()) continue;
			if (eFormSelect == object->GetTypeForm()) {
				prop->AppendItem(
					object->GetName(),
					object->GetMetaID(),
					object->GetIcon(),
					object);
			}
		}
		return true;
	}

	bool FillFormFolderSelect(CPropertyList* prop) {
		for (auto object : GetFormArrayObject()) {
			if (!object->IsAllowed()) continue;
			if (eFormFolderSelect == object->GetTypeForm()) {
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

	CPropertyList* m_propertyDefFormObject = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormObject"), _("Default Object Form"), &CValueMetaObjectCatalog::FillFormObject);
	CPropertyList* m_propertyDefFormFolder = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormFolder"), _("Default Folder Form"), &CValueMetaObjectCatalog::FillFormFolder);
	CPropertyList* m_propertyDefFormList = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormList"), _("Default List Form"), &CValueMetaObjectCatalog::FillFormList);
	CPropertyList* m_propertyDefFormSelect = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormSelect"), _("Default Select Form"), &CValueMetaObjectCatalog::FillFormSelect);
	CPropertyList* m_propertyDefFormFolderSelect = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormFolderSelect"), _("Default Folder Select Form"), &CValueMetaObjectCatalog::FillFormFolderSelect);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CPropertyOwner* m_propertyOwner = IPropertyObject::CreateProperty<CPropertyOwner>(m_categoryData, wxT("listOwner"), _("List owner"));

	//default array 
	CPropertyInnerAttribute<>* m_propertyAttributeOwner = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IValueMetaObjectCompositeData::CreateEmptyType(wxT("owner"), _("Owner"), wxEmptyString, true, eItemMode::eItemMode_Folder_Item));

	friend class CValueRecordDataObjectCatalog;
	friend class IMetaData;
};

//********************************************************************************************
//*                                      Object                                              *
//********************************************************************************************

class CValueRecordDataObjectCatalog : public IValueRecordDataObjectFolderRef {
	CValueRecordDataObjectCatalog(CValueMetaObjectCatalog* metaObject, const CGuid& objGuid = wxNullGuid, eObjectMode objMode = eObjectMode::OBJECT_ITEM);
	CValueRecordDataObjectCatalog(const CValueRecordDataObjectCatalog& source);
public:

	//****************************************************************************
	//*                              Support id's                                *
	//****************************************************************************

	//save modify 
	virtual bool SaveModify() { return WriteObject(); }

	//default methods
	virtual bool FillObject(CValue& vFillObject) const { return Filling(vFillObject); }
	virtual IValueRecordDataObjectRef* CopyObject(bool showValue = false) {
		IValueRecordDataObjectRef* objectRef = CopyObjectValue();
		if (objectRef != nullptr && showValue)
			objectRef->ShowFormValue();
		return objectRef;
	}
	virtual bool WriteObject();
	virtual bool DeleteObject();

	//****************************************************************************
	//*                              Support methods                             *
	//****************************************************************************

	virtual void PrepareNames() const;

	//****************************************************************************
	//*                              Override attribute                          *
	//****************************************************************************

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);

	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);

	//support source data 
	virtual CSourceExplorer GetSourceExplorer() const;

#pragma region _form_builder_h_
	//support show 
	virtual void ShowFormValue(const wxString& strFormName = wxEmptyString, IBackendControlFrame* owner = nullptr);
	virtual IBackendValueForm* GetFormValue(const wxString& strFormName = wxEmptyString, IBackendControlFrame* owner = nullptr);
#pragma endregion

	//support actionData
	virtual CActionCollection GetActionCollection(const form_identifier_t& formType);
	virtual void ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm);

protected:
	friend class CValue;
	friend class CValueMetaObjectCatalog;
};

#endif