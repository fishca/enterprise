#ifndef __META_TABLE_H__
#define __META_TABLE_H__

#include "backend/metaCollection/metaObjectComposite.h"

class BACKEND_API CValueMetaObjectTableData : public IValueMetaObjectCompositeData {
	wxDECLARE_DYNAMIC_CLASS(CValueMetaObjectTableData);

public:

	eItemMode GetTableUse() const { return m_propertyUse->GetValueAsEnum(); }

	CValueMetaObjectAttributePredefined* GetNumberLine() const { return m_propertyNumberLine->GetMetaObject(); }
	bool IsNumberLine(const meta_identifier_t& id) const { return id == (*m_propertyNumberLine)->GetMetaID(); }

	//get table class
	CTypeDescription GetTypeDesc() const;

	virtual bool FilterChild(const class_identifier_t& clsid) const {
		if (clsid == g_metaAttributeCLSID)
			return true;
		return false;
	}

	//ctor 
	CValueMetaObjectTableData();
	virtual ~CValueMetaObjectTableData();

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

#pragma region __generic_h__

	//attribute  
	virtual std::vector<IValueMetaObjectAttribute*> GetGenericAttributeArrayObject(
		std::vector<IValueMetaObjectAttribute*>& array = std::vector<IValueMetaObjectAttribute*>()) const {
		FillArrayObjectByPredefined(array);
		FillArrayObjectByFilter<IValueMetaObjectAttribute>(array, { g_metaAttributeCLSID });
		return array;
	}

#pragma endregion 
#pragma region __array_h__

	//any
	std::vector<IValueMetaObjectAttribute*> GetAnyAttributeArrayObject(
		std::vector<IValueMetaObjectAttribute*>& array = std::vector<IValueMetaObjectAttribute*>()) const {
		FillArrayObjectByPredefined(array);
		FillArrayObjectByFilter<IValueMetaObjectAttribute>(array, { g_metaAttributeCLSID });
		return array;
	}

	//attribute 
	std::vector<IValueMetaObjectAttribute*> GetAttributeArrayObject(
		std::vector<IValueMetaObjectAttribute*>& array = std::vector<IValueMetaObjectAttribute*>()) const {
		FillArrayObjectByFilter<IValueMetaObjectAttribute>(array, { g_metaAttributeCLSID });
		return array;
	}

#pragma endregion 
#pragma region __filter_h__

	//any 
	template <typename _T1>
	IValueMetaObjectAttribute* FindAnyAttributeObjectByFilter(const _T1& id) const {
		return FindObjectByFilter<IValueMetaObjectAttribute>(id, { g_metaAttributeCLSID, g_metaPredefinedAttributeCLSID });
	}

	//attribute 
	template <typename _T1>
	IValueMetaObjectAttribute* FindAttributeObjectByFilter(const _T1& id) const {
		return FindObjectByFilter<IValueMetaObjectAttribute>(id, { g_metaAttributeCLSID });
	}

#pragma endregion 

	//special functions for DB 
	virtual wxString GetTableNameDB() const {
		IValueMetaObject* parentMeta = GetParent();
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

	//get default attributes
	virtual bool FillArrayObjectByPredefined(std::vector<IValueMetaObjectAttribute*>& array) const {
		array = { m_propertyNumberLine->GetMetaObject() };
		return true;
	}

	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

private:

	CPropertyCategory* m_categoryGroup = IPropertyObject::CreatePropertyCategory(wxT("group"), _("Group"));
	CPropertyEnum<CValueEnumItemMode>* m_propertyUse = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumItemMode>>(m_categoryGroup, wxT("itemMode"), _("Item mode"), eItemMode::eItemMode_Item);
	CPropertyInnerAttribute<>* m_propertyNumberLine = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryGroup, IValueMetaObjectCompositeData::CreateNumber(wxT("numberLine"), _("N"), wxEmptyString, 6, 0));
};

#endif