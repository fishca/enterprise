#ifndef __ENUMERATION_H__
#define __ENUMERATION_H__

#include "commonObject.h"

class CValueMetaObjectEnumeration : public IValueMetaObjectRecordDataEnumRef {
	wxDECLARE_DYNAMIC_CLASS(CValueMetaObjectEnumeration);

	enum
	{
		ID_METATREE_OPEN_MANAGER = 19000,
	};

	enum
	{
		eFormList = 2,
		eFormSelect
	};

	virtual CFormTypeList GetFormType() const override {
		CFormTypeList formList;
		formList.AppendItem(wxT("FormList"), _("Form list"), eFormList);
		formList.AppendItem(wxT("FormSelect"), _("Form select"), eFormSelect);
		return formList;
	}

public:

	virtual bool FilterChild(const class_identifier_t& clsid) const {
		if (clsid == g_metaEnumCLSID)
			return true;
		return IValueMetaObjectGenericData::FilterChild(clsid);
	}

	CValueMetaObjectEnumeration();
	virtual ~CValueMetaObjectEnumeration();

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
	virtual IBackendValueForm* GetObjectForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid) { return nullptr; }
	virtual IBackendValueForm* GetListForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);
	virtual IBackendValueForm* GetSelectForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);
#pragma endregion

	//get module object in compose object 
	virtual CValueMetaObjectModule* GetModuleObject() const { return nullptr; }
	virtual CValueMetaObjectCommonModule* GetModuleManager() const { return m_propertyModuleManager->GetMetaObject(); }

	//descriptions...
	wxString GetDataPresentation(const IValueDataObject* objValue) const;

	//prepare menu for item
	virtual bool PrepareContextMenu(wxMenu* defaultMenu);
	virtual void ProcessCommand(unsigned int id);

protected:

	//predefined array 
	virtual bool FillArrayObjectByPredefined(std::vector<IValueMetaObjectAttribute*>& array) const {
		array = { m_propertyAttributeReference->GetMetaObject() };
		return true;
	}

	//searched array 
	virtual bool FillArrayObjectBySearched(std::vector<IValueMetaObjectAttribute*>& array) const {
		return true;
	}

	//create manager
	virtual IValueManagerDataObject* CreateManagerDataObjectValue();

	//create object data with meta form
	virtual ISourceDataObject* CreateSourceObject(IValueMetaObjectForm* metaObject);

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

private:

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

	CPropertyInnerModule<CValueMetaObjectManagerModule>* m_propertyModuleManager = IPropertyObject::CreateProperty<CPropertyInnerModule<CValueMetaObjectManagerModule>>(m_categorySecondary, IValueMetaObjectCompositeData::CreateMetaObjectAndSetParent<CValueMetaObjectManagerModule>(wxT("ManagerModule"), _("Manager module")));

	CPropertyCategory* m_categoryForm = IPropertyObject::CreatePropertyCategory(wxT("PresetValues"), _("Preset values"));
	CPropertyList* m_propertyDefFormList = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("DefaultFormList"), _("Default List Form"), &CValueMetaObjectEnumeration::FillFormList);
	CPropertyList* m_propertyDefFormSelect = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("DefaultFormSelect"), _("Default Select Form"), &CValueMetaObjectEnumeration::FillFormSelect);
};

#endif