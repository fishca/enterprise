#ifndef __META_TABLE_H__
#define __META_TABLE_H__

#include "backend/metaCollection/metaSource.h"

class BACKEND_API CMetaObjectTableData : public IMetaObjectSourceData {
    wxDECLARE_DYNAMIC_CLASS(CMetaObjectTableData);
private:
    //CMetaObjectAttributeDefault* m_numberLine = IMetaObjectSourceData::CreateNumber("numberLine", _("N"), wxEmptyString, 6, 0);
protected:
   
    CPropertyCategory* m_categoryGroup = IPropertyObject::CreatePropertyCategory("group", _("group"));
    CPropertyEnum<CValueEnumItemMode>* m_propertyUse = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumItemMode>>(m_categoryGroup, wxT("itemMode"), _("item mode"), eItemMode::eItemMode_Item);
    CPropertyInnerAttribute<>* m_propertyNumberLine = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryGroup, IMetaObjectSourceData::CreateNumber("numberLine", _("N"), wxEmptyString, 6, 0));

public:

    eItemMode GetTableUse() const { return m_propertyUse->GetValueAsEnum(); }

    CMetaObjectAttributeDefault* GetNumberLine() const { return m_propertyNumberLine->GetMetaObject(); }
    bool IsNumberLine(const meta_identifier_t& id) const { return id == (*m_propertyNumberLine)->GetMetaID(); }

    //get table class
    CTypeDescription GetTypeDesc() const;

    virtual bool FilterChild(const class_identifier_t& clsid) const {
        if (clsid == g_metaAttributeCLSID)
            return true;
        return false;
    }

    //ctor 
    CMetaObjectTableData();
    virtual ~CMetaObjectTableData();

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
    //after and before for designer 
    virtual bool OnBeforeRunMetaObject(int flags);
    virtual bool OnAfterRunMetaObject(int flags);

    //after and before for designer 
    virtual bool OnAfterCloseMetaObject();

    //override base objects 
    virtual std::vector<IMetaObjectAttribute*> GetGenericAttributes() const { return GetObjectAttributes(); }

    //get attributes, form etc.. 
    virtual std::vector<IMetaObjectAttribute*> GetObjectAttributes() const;

    //find attributes, tables etc 
    virtual IMetaObjectAttribute* FindAttributeByGuid(const wxString& strDocPath) const {
        for (auto& obj : GetObjectAttributes()) {
            if (strDocPath == obj->GetDocPath())
                return obj;
        }
        return nullptr;
    }

    //find in current metaObject
    virtual IMetaObjectAttribute* FindProp(const meta_identifier_t& id) const;

    //special functions for DB 
    virtual wxString GetTableNameDB() const {
        IMetaObject* parentMeta = GetParent();
        wxASSERT(parentMeta);
        return wxString::Format(wxT("%s%i_VT%i"),
            parentMeta->GetClassName(),
            parentMeta->GetMetaID(),
            GetMetaID()
        );
    }

    /**
    * Property events
    */
    virtual void OnPropertyRefresh(class wxPropertyGridManager* pg, class wxPGProperty* pgProperty, IProperty* property);

protected:

    virtual bool LoadData(CMemoryReader& reader);
    virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());
};

#endif