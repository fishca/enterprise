#ifndef __CATALOG_H__
#define __CATALOG_H__

#include "commonObject.h"
#include "reference/reference.h"

//********************************************************************************************
//*                                  Factory & metaData                                      *
//********************************************************************************************

class CMetaObjectCatalog : public IMetaObjectRecordDataFolderMutableRef {
    wxDECLARE_DYNAMIC_CLASS(CMetaObjectCatalog);
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
        eFormGroup,
        eFormFolderSelect
    };

    virtual CFormTypeList GetFormType() const override {
        CFormTypeList formList;
        formList.AppendItem(wxT("formObject"), _("Form object"), eFormObject);
        formList.AppendItem(wxT("formFolder"), _("Form group"), eFormGroup);
        formList.AppendItem(wxT("formList"), _("Form list"), eFormList);
        formList.AppendItem(wxT("formSelect"), _("Form select"), eFormSelect);
        formList.AppendItem(wxT("formGroupSelect"), _("Form group select"), eFormFolderSelect);
        return formList;
    }

protected:

    CPropertyInnerModule<CMetaObjectModule>* m_propertyModuleObject = IPropertyObject::CreateProperty<CPropertyInnerModule<CMetaObjectModule>>(m_categorySecondary, IMetaObjectSourceData::CreateMetaObjectAndSetParent<CMetaObjectModule>(wxT("objectModule"), _("object module")));
    CPropertyInnerModule<CMetaObjectManagerModule>* m_propertyModuleManager = IPropertyObject::CreateProperty<CPropertyInnerModule<CMetaObjectManagerModule>>(m_categorySecondary, IMetaObjectSourceData::CreateMetaObjectAndSetParent<CMetaObjectManagerModule>(wxT("managerModule"), _("manager module")));

    CPropertyCategory* m_categoryForm = IPropertyObject::CreatePropertyCategory(wxT("defaultForms"), _("default forms"));

    CPropertyList* m_propertyDefFormObject = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormObject"), _("default object"), &CMetaObjectCatalog::GetFormObject, wxNOT_FOUND);
    CPropertyList* m_propertyDefFormFolder = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormFolder"), _("default folder"), &CMetaObjectCatalog::GetFormFolder, wxNOT_FOUND);
    CPropertyList* m_propertyDefFormList = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormList"), _("default list"), &CMetaObjectCatalog::GetFormList, wxNOT_FOUND);
    CPropertyList* m_propertyDefFormSelect = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormSelect"), _("default select"), &CMetaObjectCatalog::GetFormSelect, wxNOT_FOUND);
    CPropertyList* m_propertyDefFormFolderSelect = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormFolderSelect"), _("default folder select"), &CMetaObjectCatalog::GetFormFolderSelect, wxNOT_FOUND);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CPropertyOwner* m_propertyOwner = IPropertyObject::CreateProperty<CPropertyOwner>(m_categoryData, wxT("listOwner"), _("list owner"));

    //default attributes 
    //CMetaObjectAttributeDefault* m_attributeOwner = IMetaObjectSourceData::CreateEmptyType(wxT("owner"), _("Owner"), wxEmptyString, true, eItemMode::eItemMode_Folder_Item);
    CPropertyInnerAttribute<>* m_propertyAttributeOwner = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IMetaObjectSourceData::CreateEmptyType(wxT("owner"), _("Owner"), wxEmptyString, true, eItemMode::eItemMode_Folder_Item));

private:
    bool GetFormObject(CPropertyList* prop);
    bool GetFormFolder(CPropertyList* prop);
    bool GetFormList(CPropertyList* prop);
    bool GetFormSelect(CPropertyList* prop);
    bool GetFormFolderSelect(CPropertyList* prop);
public:

    CMetaObjectAttributeDefault* GetCatalogOwner() const { return m_propertyAttributeOwner->GetMetaObject(); }

    //default constructor 
    CMetaObjectCatalog();
    virtual ~CMetaObjectCatalog();

    //support icons
    virtual wxIcon GetIcon() const;
    static wxIcon GetIconGroup();

    //events: 
    virtual bool OnCreateMetaObject(IMetaData* metaData, int flags);
    virtual bool OnLoadMetaObject(IMetaData* metaData);
    virtual bool OnSaveMetaObject();
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

    //get attribute code 
    virtual IMetaObjectAttribute* GetAttributeForCode() const {
        return m_propertyAttributeCode->GetMetaObject();
    }

    //override base objects 
    virtual std::vector<IMetaObjectAttribute*> GetDefaultAttributes() const override;

    //searched attributes 
    virtual std::vector<IMetaObjectAttribute*> GetSearchedAttributes() const override;

    //create associate value 
    virtual CMetaObjectForm* GetDefaultFormByID(const form_identifier_t& id);

    //create object data with metaForm
    virtual ISourceDataObject* CreateObjectData(IMetaObjectForm* metaObject);

    //create form with data 
    virtual IBackendValueForm* CreateObjectForm(IMetaObjectForm* metaForm) override {
        return metaForm->GenerateFormAndRun(
            nullptr, CreateObjectData(metaForm)
        );
    }

    //support form 
    virtual IBackendValueForm* GetObjectForm(const wxString& formName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);
    virtual IBackendValueForm* GetFolderForm(const wxString& formName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);
    virtual IBackendValueForm* GetListForm(const wxString& formName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);
    virtual IBackendValueForm* GetSelectForm(const wxString& formName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);
    virtual IBackendValueForm* GetFolderSelectForm(const wxString& formName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);

    //descriptions...
    wxString GetDataPresentation(const IValueDataObject* objValue) const;

    //get module object in compose object 
    virtual CMetaObjectModule* GetModuleObject() const { return m_propertyModuleObject->GetMetaObject(); }
    virtual CMetaObjectCommonModule* GetModuleManager() const { return m_propertyModuleManager->GetMetaObject(); }

    /**
    * Property events
    */
    virtual void OnPropertyCreated(IProperty* property);
    virtual bool OnPropertyChanging(IProperty* property, const wxVariant& newValue);
    virtual void OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue);

    //paste property from data
    virtual void OnPropertyPasted(IProperty* property);

protected:
    //create empty object
    virtual IRecordDataObjectFolderRef* CreateObjectRefValue(eObjectMode mode, const Guid& guid = wxNullGuid);

    //load & save metaData from DB 
    virtual bool LoadData(CMemoryReader& reader);
    virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

    //prepare menu for item
    virtual bool PrepareContextMenu(wxMenu* defaultMenu);
    virtual void ProcessCommand(unsigned int id);

protected:
    friend class IMetaData;
    friend class CRecordDataObjectCatalog;
};

//********************************************************************************************
//*                                      Object                                              *
//********************************************************************************************

class CRecordDataObjectCatalog : public IRecordDataObjectFolderRef {
    CRecordDataObjectCatalog(CMetaObjectCatalog* metaObject, const Guid& objGuid = wxNullGuid, eObjectMode objMode = eObjectMode::OBJECT_ITEM);
    CRecordDataObjectCatalog(const CRecordDataObjectCatalog& source);
public:

    //****************************************************************************
    //*                              Support id's                                *
    //****************************************************************************

    //save modify 
    virtual bool SaveModify() {
        return WriteObject();
    }

    //default methods
    virtual bool FillObject(CValue& vFillObject) const {
        return Filling(vFillObject);
    }
    virtual IRecordDataObjectRef* CopyObject(bool showValue = false) {
        IRecordDataObjectRef* objectRef = CopyObjectValue();
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

    //support show 
    virtual void ShowFormValue(const wxString& formName = wxEmptyString, IBackendControlFrame* owner = nullptr);
    virtual IBackendValueForm* GetFormValue(const wxString& formName = wxEmptyString, IBackendControlFrame* owner = nullptr);

    //support actionData
    virtual CActionCollection GetActionCollection(const form_identifier_t& formType);
    virtual void ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm);

protected:
    friend class IMetaData;
    friend class CMetaObjectCatalog;
};

#endif