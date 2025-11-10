#ifndef __COMMON_OBJECT_H__
#define __COMMON_OBJECT_H__

#include "reference/reference.h"
#include "backend/metaDefines.h"

#include "backend/system/value/valueGuid.h"

#include "backend/actionInfo.h"
#include "backend/moduleInfo.h"
#include "backend/valueInfo.h"
#include "backend/tableInfo.h"

//********************************************************************************************
//*                                     Defines                                              *
//********************************************************************************************

class BACKEND_API CMetaObjectConstant;

class BACKEND_API IMetaObjectGenericData;
class BACKEND_API IMetaObjectRecordData;
class BACKEND_API IMetaObjectRecordDataRef;
class BACKEND_API IMetaObjectRegisterData;

class BACKEND_API ISourceDataObject;

class BACKEND_API IRecordDataObject;
class BACKEND_API IRecordDataObjectExt;
class BACKEND_API IRecordDataObjectRef;
class BACKEND_API IRecordDataObjectFolderRef;

class BACKEND_API CRecordKeyObject;
class BACKEND_API IRecordManagerObject;
class BACKEND_API IRecordSetObject;

class BACKEND_API CSourceExplorer;

//special names 
#define guidName wxT("uuid")

//********************************************************************************************
//*                                  Factory & metaData                                      *
//********************************************************************************************

class BACKEND_API IBackendCommandData {
public:

	virtual ~IBackendCommandData() {}

	//get default form 
	virtual IBackendValueForm* GetDefaultCommandForm() = 0;
};

class BACKEND_API CFormTypeList {

	struct CFormTypeItem {
		bool m_isOk;
		wxString m_strName;
		wxString m_strLabel;
		wxString m_strHelp;
		long m_id;
	public:

		CFormTypeItem() :
			m_strName(), m_strLabel(), m_id(-1), m_isOk(true)
		{
		}

		CFormTypeItem(const wxString& name, const long& l) :
			m_strName(name), m_strLabel(name), m_id(l), m_isOk(true)
		{
		}

		CFormTypeItem(const wxString& name, const wxString& label, const long& l) :
			m_strName(name), m_strLabel(label), m_id(l), m_isOk(true)
		{
		}

		CFormTypeItem(const wxString& name, const wxString& label, const wxString& help, const long& l) :
			m_strName(name), m_strLabel(label), m_strHelp(help), m_id(l), m_isOk(true)
		{
		}

		CFormTypeItem(const CFormTypeItem& item) :
			m_strName(item.m_strName), m_strLabel(item.m_strLabel), m_strHelp(item.m_strHelp), m_id(item.m_id), m_isOk(true)
		{
		}

		CFormTypeItem& operator = (const CFormTypeItem& src) {
			m_strName = src.m_strName;
			m_strLabel = src.m_strLabel;
			m_strHelp = src.m_strHelp;
			m_id = src.m_id;
			return *this;
		}

		operator const long() const { return m_id; }
	};

	CFormTypeItem GetItemAt(const unsigned int idx) const {
		if (idx > m_listTypeForm.size())
			return CFormTypeItem();
		auto it = m_listTypeForm.begin();
		std::advance(it, idx);
		return *it;
	};

	CFormTypeItem GetItemById(const long& id) const {
		auto it = std::find_if(m_listTypeForm.begin(), m_listTypeForm.end(),
			[id](const CFormTypeItem& p) { return id == p.m_id; }
		);
		if (it != m_listTypeForm.end())
			return *it;
		return CFormTypeItem();
	};

public:

	void ResetListItem() { m_listTypeForm.clear(); }

	void AppendItem(const wxString& name, const int& l) { (void)m_listTypeForm.emplace_back(name, l); }
	void AppendItem(const wxString& name, const wxString& label, const int& l) { (void)m_listTypeForm.emplace_back(name, label, l); }
	void AppendItem(const wxString& name, const wxString& label, const wxString& help, const int& l) { (void)m_listTypeForm.emplace_back(label, help, l); }

	bool HasValue(const long& l) const { return GetItemById(l); }

	wxString GetItemName(const unsigned int idx) const { return GetItemAt(idx).m_strName; }
	wxString GetItemLabel(const unsigned int idx) const { return GetItemAt(idx).m_strLabel; }
	wxString GetItemHelp(const unsigned int idx) const { return GetItemAt(idx).m_strHelp; }
	long GetItemId(const unsigned int idx) const { return GetItemAt(idx).m_id; }

	unsigned int GetItemCount() const { return (unsigned int)m_listTypeForm.size(); }

private:
	std::vector<CFormTypeItem> m_listTypeForm;
};

#include "backend/metaCollection/metaSource.h"

class BACKEND_API IMetaObjectGenericData
	: public IMetaObjectSourceData, public IBackendCommandData {
	wxDECLARE_ABSTRACT_CLASS(IMetaObjectGenericData);
public:
	friend class IMetaData;
public:

	virtual bool FilterChild(const class_identifier_t& clsid) const {
		if (
			clsid == g_metaFormCLSID ||
			clsid == g_metaTemplateCLSID
			)
			return true;
		return false;
	}

	//get data selector 
	virtual eSelectorDataType GetFilterDataType() const {
		return eSelectorDataType::eSelectorDataType_reference;
	}

	//override base objects 
	virtual std::vector<IMetaObjectAttribute*> GetGenericAttributes() const = 0;

	virtual std::vector<CMetaObjectForm*> GetGenericForms() const = 0;
	virtual std::vector<CMetaObjectGrid*> GetObjectTemplates() const = 0;

	//find attributes, tables etc 
	virtual IMetaObjectAttribute* FindGenericAttributeByGuid(const wxString& strDocPath) const {
		for (auto& obj : GetGenericAttributes()) {
			if (strDocPath == obj->GetDocPath())
				return obj;
		}
		return nullptr;
	}

	//find in current metaObject
	virtual IMetaObjectAttribute* FindGenericAttribute(const meta_identifier_t& id) const;

	//form events 
	virtual void OnCreateFormObject(IMetaObjectForm* metaForm) {}
	virtual void OnRemoveMetaForm(IMetaObjectForm* metaForm) {}

	//Get form type
	virtual CFormTypeList GetFormType() const = 0;

	//Get metaObject by def id
	virtual IMetaObjectForm* GetDefaultFormByID(const form_identifier_t& id) { return nullptr; }

	//create form with data 
	virtual IBackendValueForm* CreateObjectForm(IMetaObjectForm* metaForm) {
		return IMetaObjectGenericData::CreateAndBuildForm(
			metaForm != nullptr ? metaForm->GetName() : wxEmptyString,
			metaForm != nullptr ? metaForm->GetTypeForm() : defaultFormType,
			nullptr,
			CreateSourceObject(metaForm),
			metaForm != nullptr ? metaForm->GetGuid() : wxNullGuid
		);
	}

#pragma region _form_builder_h_
	//support form 
	IBackendValueForm* GetGenericForm(const wxString& strFormName = wxEmptyString,
		IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);
#pragma endregion

#pragma region _form_creator_h_
	IBackendValueForm* CreateAndBuildForm(const wxString& strFormName, const form_identifier_t& form_id = defaultFormType,
		IBackendControlFrame* ownerControl = nullptr,
		ISourceDataObject* srcObject = nullptr,
		const CUniqueKey& formGuid = wxNullGuid
	);
#pragma endregion

protected:

	//create object data with meta form
	virtual ISourceDataObject* CreateSourceObject(IMetaObjectForm* metaObject) { return nullptr; }
};

class BACKEND_API IMetaObjectRecordData
	: public IMetaObjectGenericData {
	wxDECLARE_ABSTRACT_CLASS(IMetaObjectRecordData);

public:

	virtual bool FilterChild(const class_identifier_t& clsid) const {
		if (
			clsid == g_metaAttributeCLSID ||
			clsid == g_metaTableCLSID
			)
			return true;
		return IMetaObjectGenericData::FilterChild(clsid);
	}

	//get data selector 
	virtual eSelectorDataType GetFilterDataType() const {
		return eSelectorDataType::eSelectorDataType_any;
	}

	//get module object in compose object 
	virtual CMetaObjectModule* GetModuleObject() const { return nullptr; }

	virtual CMetaObjectCommonModule* GetModuleManager() const { return nullptr; }

	//meta events
	virtual bool OnLoadMetaObject(IMetaData* metaData);
	virtual bool OnSaveMetaObject(int flags);
	virtual bool OnDeleteMetaObject();

	//module manager is started or exit 
	virtual bool OnBeforeRunMetaObject(int flags);
	virtual bool OnAfterCloseMetaObject();

	//override base objects 
	virtual std::vector<IMetaObjectAttribute*> GetGenericAttributes() const { return GetObjectAttributes(); }

	virtual std::vector<CMetaObjectForm*> GetGenericForms() const { return GetObjectForms(); }

	//get default attributes
	virtual std::vector<IMetaObjectAttribute*> GetDefaultAttributes() const { return std::vector<IMetaObjectAttribute*>(); }

	//get attributes, form etc.. 
	virtual std::vector<IMetaObjectAttribute*> GetObjectAttributes() const;
	virtual std::vector<CMetaObjectTableData*> GetObjectTables() const;
	virtual std::vector<CMetaObjectForm*> GetObjectForms() const;
	virtual std::vector<CMetaObjectGrid*> GetObjectTemplates() const;

	//find attributes, tables etc 
	virtual IMetaObjectAttribute* FindDefAttributeById(const meta_identifier_t& id) const {
		for (auto& obj : GetDefaultAttributes()) {
			if (id == obj->GetMetaID())
				return obj;
		}
		return nullptr;
	}

	virtual IMetaObjectAttribute* FindDefAttributeByGuid(const wxString& strDocPath) const {
		for (auto& obj : GetDefaultAttributes()) {
			if (strDocPath == obj->GetDocPath())
				return obj;
		}
		return nullptr;
	}

	virtual IMetaObjectAttribute* FindAttributeById(const meta_identifier_t& id) const {
		for (auto& obj : GetObjectAttributes()) {
			if (id == obj->GetMetaID())
				return obj;
		}
		return nullptr;
	}

	virtual IMetaObjectAttribute* FindAttributeByGuid(const wxString& strDocPath) const {
		for (auto& obj : GetObjectAttributes()) {
			if (strDocPath == obj->GetDocPath())
				return obj;
		}
		return nullptr;
	}

	virtual CMetaObjectTableData* FindTableById(const meta_identifier_t& id) const {
		for (auto& obj : GetObjectTables()) {
			if (id == obj->GetMetaID())
				return obj;
		}
		return nullptr;
	}

	virtual CMetaObjectTableData* FindTableByGuid(const wxString& strDocPath) const {
		for (auto& obj : GetObjectTables()) {
			if (strDocPath == obj->GetDocPath())
				return obj;
		}
		return nullptr;
	}

	//create single object
	virtual IRecordDataObject* CreateRecordDataObject() = 0;

	//Get metaObject by def id
	virtual CMetaObjectForm* GetDefaultFormByID(const form_identifier_t& id) = 0;

	//get default form 
	virtual IBackendValueForm* GetDefaultCommandForm() { return GetObjectForm(); }

#pragma region _form_builder_h_
	//support form 
	virtual IBackendValueForm* GetObjectForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid) = 0;
#pragma endregion 
};

enum
{
	METAOBJECT_NORMAL = 1,
	METAOBJECT_EXTERNAL,
};

enum eObjectMode {
	OBJECT_ITEM = 1,
	OBJECT_FOLDER
};

//metaObject with file 
class BACKEND_API IMetaObjectRecordDataExt : public IMetaObjectRecordData {
	wxDECLARE_ABSTRACT_CLASS(IMetaObjectRecordDataExt);
protected:
	CRole* m_roleUse = IMetaObject::CreateRole("use", _("use"));
protected:
	//external or default dataProcessor
	int m_objMode;
public:
	//get object mode 
	int GetObjectMode() const {
		return m_objMode;
	}
	//ctor
	IMetaObjectRecordDataExt(int objMode = METAOBJECT_NORMAL);

	//create associate value 
	IRecordDataObjectExt* CreateObjectValue(); //create object
	IRecordDataObjectExt* CreateObjectValue(IRecordDataObjectExt* objSrc);

	//create single object
	virtual IRecordDataObject* CreateRecordDataObject();

protected:
	//create empty object
	virtual IRecordDataObjectExt* CreateObjectExtValue() = 0;  //create object 
};

//metaObject with reference 
class BACKEND_API IMetaObjectRecordDataRef : public IMetaObjectRecordData {
	wxDECLARE_ABSTRACT_CLASS(IMetaObjectRecordDataRef);
protected:

	CPropertyCategory* m_categoryData = IPropertyObject::CreatePropertyCategory(wxT("data"), _("data"));
	CPropertyCategory* m_categoryPresentation = IPropertyObject::CreatePropertyCategory(wxT("presentation"), _("presentation"));
	CPropertyBoolean* m_propertyQuickChoice = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryPresentation, wxT("quickChoice"), _("quick choice"), false);

	CPropertyInnerAttribute<>* m_propertyAttributeReference = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IMetaObjectSourceData::CreateSpecialType(wxT("reference"), _("Reference"), wxEmptyString, CValue::GetIDByVT(eValueTypes::TYPE_EMPTY)));

protected:
	//ctor
	IMetaObjectRecordDataRef();
	virtual ~IMetaObjectRecordDataRef();
public:

	virtual CMetaObjectAttributeDefault* GetDataReference() const { return m_propertyAttributeReference->GetMetaObject(); }
	virtual bool IsDataReference(const meta_identifier_t& id) const { return id == (*m_propertyAttributeReference)->GetMetaID(); }

	virtual bool HasQuickChoice() const { return m_propertyQuickChoice->GetValueAsBoolean(); }

	//get data selector 
	virtual eSelectorDataType GetFilterDataType() const {
		return eSelectorDataType::eSelectorDataType_reference;
	}

	//meta events
	virtual bool OnCreateMetaObject(IMetaData* metaData, int flags);
	virtual bool OnLoadMetaObject(IMetaData* metaData);
	virtual bool OnSaveMetaObject(int flags);
	virtual bool OnDeleteMetaObject();

	//module manager is started or exit 
	//after and before for designer 
	virtual bool OnBeforeRunMetaObject(int flags);
	virtual bool OnAfterRunMetaObject(int flags);

	virtual bool OnBeforeCloseMetaObject();
	virtual bool OnAfterCloseMetaObject();

	//create single object
	virtual IRecordDataObject* CreateRecordDataObject() {
		wxASSERT_MSG(false, "IMetaObjectRecordDataRef::CreateRecordDataObject");
		return nullptr;
	}

	//process choice 
	virtual bool ProcessChoice(IBackendControlFrame* ownerValue,
		const wxString& strFormName = wxEmptyString, eSelectMode selMode = eSelectMode::eSelectMode_Items);

	//find attributes, tables etc 
	virtual CMetaObjectEnum* FindEnumByGuid(const wxString& strDocPath) const {
		for (auto& obj : GetObjectEnums()) {
			if (strDocPath == obj->GetDocPath())
				return obj;
		}
		return nullptr;
	}

	//override base objects 
	virtual std::vector<IMetaObjectAttribute*> GetGenericAttributes() const;
	virtual std::vector<CMetaObjectForm*> GetGenericForms() const { return GetObjectForms(); }

	//get attribute code 
	virtual IMetaObjectAttribute* GetAttributeForCode() const { return nullptr; }

	//override base objects 
	virtual std::vector<IMetaObjectAttribute*> GetDefaultAttributes() const = 0;
	//other objects
	virtual std::vector<CMetaObjectEnum*> GetObjectEnums() const;
	//searched attributes 
	virtual std::vector<IMetaObjectAttribute*> GetSearchedAttributes() const = 0;

	//get default form 
	virtual IBackendValueForm* GetDefaultCommandForm() { return GetListForm(); }

	virtual class CReferenceDataObject* FindObjectValue(const CGuid& guid); //find by guid and ret reference 

#pragma region _form_builder_h_
	//support form 
	virtual IBackendValueForm* GetListForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid) = 0;
	virtual IBackendValueForm* GetSelectForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid) = 0;
#pragma endregion 

	//descriptions...
	virtual wxString GetDataPresentation(const IValueDataObject* objValue) const = 0;

	//special functions for DB 
	virtual wxString GetTableNameDB() const;

	//create and update table 
	virtual bool CreateAndUpdateTableDB(IMetaDataConfiguration* srcMetaData, IMetaObject* srcMetaObject, int flags);

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());
	virtual bool DeleteData() { return true; }

	//process default query
	int ProcessAttribute(const wxString& tableName, IMetaObjectAttribute* srcAttr, IMetaObjectAttribute* dstAttr);
	int ProcessEnumeration(const wxString& tableName, CMetaObjectEnum* srcEnum, CMetaObjectEnum* dstEnum);
	int ProcessTable(const wxString& tabularName, CMetaObjectTableData* srcTable, CMetaObjectTableData* dstTable);
};

//metaObject with reference - for enumeration
class BACKEND_API IMetaObjectRecordDataEnumRef : public IMetaObjectRecordDataRef {
	wxDECLARE_ABSTRACT_CLASS(IMetaObjectRecordDataEnumRef);
private:

	//default attributes 
	CPropertyInnerAttribute<>* m_propertyAttributeOrder = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IMetaObjectSourceData::CreateNumber(wxT("order"), _("Order"), wxEmptyString, 6, true));

protected:

	//ctor
	IMetaObjectRecordDataEnumRef();
	virtual ~IMetaObjectRecordDataEnumRef();

public:

	CMetaObjectAttributeDefault* GetDataOrder() const { return m_propertyAttributeOrder->GetMetaObject(); }
	bool IsDataOrder(const meta_identifier_t& id) const { return id == (*m_propertyAttributeOrder)->GetMetaID(); }

	//events: 
	virtual bool OnCreateMetaObject(IMetaData* metaData, int flags);
	virtual bool OnLoadMetaObject(IMetaData* metaData);
	virtual bool OnSaveMetaObject(int flags);
	virtual bool OnDeleteMetaObject();

	//module manager is started or exit 
	virtual bool OnBeforeRunMetaObject(int flags);
	virtual bool OnAfterRunMetaObject(int flags);

	virtual bool OnBeforeCloseMetaObject();
	virtual bool OnAfterCloseMetaObject();

protected:

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());
};

//metaObject with reference and deletion mark 
class BACKEND_API IMetaObjectRecordDataMutableRef : public IMetaObjectRecordDataRef {
	wxDECLARE_ABSTRACT_CLASS(IMetaObjectRecordDataMutableRef);
private:
	CRole* m_roleRead = IMetaObject::CreateRole("read", _("read"));
	CRole* m_roleInsert = IMetaObject::CreateRole("insert", _("insert"));
	CRole* m_roleUpdate = IMetaObject::CreateRole("update", _("update"));
	CRole* m_roleDelete = IMetaObject::CreateRole("delete", _("delete"));
protected:

	CPropertyGeneration* m_propertyGeneration = IPropertyObject::CreateProperty<CPropertyGeneration>(m_categoryData, wxT("listGeneration"), _("list generation"));
	CPropertyInnerAttribute<>* m_propertyAttributeDataVersion = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IMetaObjectSourceData::CreateString(wxT("dataVersion"), _("Data version"), wxEmptyString, 12, eItemMode_Folder_Item));
	CPropertyInnerAttribute<>* m_propertyAttributeDeletionMark = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IMetaObjectSourceData::CreateBoolean(wxT("deletionMark"), _("Deletion mark"), wxEmptyString));

protected:
	//ctor
	IMetaObjectRecordDataMutableRef();
	virtual ~IMetaObjectRecordDataMutableRef();
public:

	CMetaDescription& GetGenerationDescription() const { return m_propertyGeneration->GetValueAsMetaDesc(); }

	CMetaObjectAttributeDefault* GetDataVersion() const { return m_propertyAttributeDataVersion->GetMetaObject(); }
	bool IsDataVersion(const meta_identifier_t& id) const { return id == (*m_propertyAttributeDataVersion)->GetMetaID(); }

	CMetaObjectAttributeDefault* GetDataDeletionMark() const { return m_propertyAttributeDeletionMark->GetMetaObject(); }
	bool IsDataDeletionMark(const meta_identifier_t& id) const { return id == (*m_propertyAttributeDeletionMark)->GetMetaID(); }

	virtual bool HasQuickChoice() const { return false; }

	//events: 
	virtual bool OnCreateMetaObject(IMetaData* metaData, int flags);
	virtual bool OnLoadMetaObject(IMetaData* metaData);
	virtual bool OnSaveMetaObject(int flags);
	virtual bool OnDeleteMetaObject();

	//module manager is started or exit 
	virtual bool OnBeforeRunMetaObject(int flags);
	virtual bool OnAfterRunMetaObject(int flags);

	virtual bool OnBeforeCloseMetaObject();
	virtual bool OnAfterCloseMetaObject();

	//create associate value 
	IRecordDataObjectRef* CreateObjectValue();
	IRecordDataObjectRef* CreateObjectValue(const CGuid& guid);
	IRecordDataObjectRef* CreateObjectValue(IRecordDataObjectRef* objSrc, bool generate = false);

	//copy associate value 	
	IRecordDataObjectRef* CopyObjectValue(const CGuid& guid);

	//create single object
	virtual IRecordDataObject* CreateRecordDataObject();

protected:

	/**
	* Property events
	*/
	virtual void OnPropertyCreated(IProperty* property);
	virtual void OnPropertyRefresh(class wxPropertyGridManager* pg, class wxPGProperty* pgProperty, IProperty* property);
	virtual bool OnPropertyChanging(IProperty* property, const wxVariant& newValue);
	virtual void OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue);

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

	//create empty object
	virtual IRecordDataObjectRef* CreateObjectRefValue(const CGuid& objGuid = wxNullGuid) = 0; //create object and read by guid 
};

//metaObject with reference and deletion mark and group/object type
class BACKEND_API IMetaObjectRecordDataFolderMutableRef : public IMetaObjectRecordDataMutableRef {
	wxDECLARE_ABSTRACT_CLASS(IMetaObjectRecordDataFolderMutableRef);
protected:

	//create default attributes
	CPropertyInnerAttribute<>* m_propertyAttributeCode = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IMetaObjectSourceData::CreateString(wxT("code"), _("Code"), wxEmptyString, 8, true, eItemMode::eItemMode_Folder_Item));
	CPropertyInnerAttribute<>* m_propertyAttributeDescription = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IMetaObjectSourceData::CreateString(wxT("description"), _("Description"), wxEmptyString, 150, true, eItemMode::eItemMode_Folder_Item));
	CPropertyInnerAttribute<>* m_propertyAttributeParent = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IMetaObjectSourceData::CreateEmptyType(wxT("parent"), _("Parent"), wxEmptyString, false, eItemMode::eItemMode_Folder_Item, eSelectMode::eSelectMode_Folders));
	CPropertyInnerAttribute<>* m_propertyAttributeIsFolder = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IMetaObjectSourceData::CreateBoolean(wxT("isFolder"), _("Is folder"), wxEmptyString, eItemMode::eItemMode_Folder_Item));

public:

	CMetaObjectAttributeDefault* GetDataCode() const { return m_propertyAttributeCode->GetMetaObject(); }
	virtual bool IsDataCode(const meta_identifier_t& id) const { return id == (*m_propertyAttributeCode)->GetMetaID(); }

	CMetaObjectAttributeDefault* GetDataDescription() const { return m_propertyAttributeDescription->GetMetaObject(); }
	virtual bool IsDataDescription(const meta_identifier_t& id) const { return id == (*m_propertyAttributeDescription)->GetMetaID(); }

	CMetaObjectAttributeDefault* GetDataParent() const { return m_propertyAttributeParent->GetMetaObject(); }
	virtual bool IsDataParent(const meta_identifier_t& id) const { return id == (*m_propertyAttributeParent)->GetMetaID(); }

	CMetaObjectAttributeDefault* GetDataIsFolder() const { return m_propertyAttributeIsFolder->GetMetaObject(); }
	virtual bool IsDataFolder(const meta_identifier_t& id) const { return id == (*m_propertyAttributeIsFolder)->GetMetaID(); }

	virtual bool HasQuickChoice() const { return m_propertyQuickChoice->GetValueAsBoolean(); }

	IMetaObjectRecordDataFolderMutableRef();
	virtual ~IMetaObjectRecordDataFolderMutableRef();

	//process choice 
	virtual bool ProcessChoice(IBackendControlFrame* ownerValue,
		const wxString& strFormName = wxEmptyString, eSelectMode selMode = eSelectMode::eSelectMode_Items);

	virtual bool FilterChild(const class_identifier_t& clsid) const {
		if (clsid == g_metaAttributeCLSID ||
			clsid == g_metaTableCLSID)
			return true;
		return IMetaObjectGenericData::FilterChild(clsid);
	}

	//events: 
	virtual bool OnCreateMetaObject(IMetaData* metaData, int flags);
	virtual bool OnLoadMetaObject(IMetaData* metaData);
	virtual bool OnSaveMetaObject(int flags);
	virtual bool OnDeleteMetaObject();

	//module manager is started or exit 
	virtual bool OnBeforeRunMetaObject(int flags);
	virtual bool OnAfterRunMetaObject(int flags);

	virtual bool OnBeforeCloseMetaObject();
	virtual bool OnAfterCloseMetaObject();

	//create associate value 	
	IRecordDataObjectFolderRef* CreateObjectValue(eObjectMode mode);
	IRecordDataObjectFolderRef* CreateObjectValue(eObjectMode mode, const CGuid& guid);
	IRecordDataObjectFolderRef* CreateObjectValue(eObjectMode mode, IRecordDataObjectRef* objSrc, bool generate = false);

	//copy associate value 	
	IRecordDataObjectFolderRef* CopyObjectValue(eObjectMode mode, const CGuid& guid);

#pragma region _form_builder_h_
	//support form 
	virtual IBackendValueForm* GetFolderForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid) = 0;
	virtual IBackendValueForm* GetFolderSelectForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid) = 0;
#pragma endregion 

protected:

	/**
	* Property events
	*/
	virtual void OnPropertyRefresh(class wxPropertyGridManager* pg, class wxPGProperty* pgProperty, IProperty* property);

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

	//create empty object
	virtual IRecordDataObjectFolderRef* CreateObjectRefValue(eObjectMode mode, const CGuid& objGuid = wxNullGuid) = 0; //create object and read by guid 
	virtual IRecordDataObjectRef* CreateObjectRefValue(const CGuid& objGuid = wxNullGuid) final;
};

//metaObject with key   
class BACKEND_API IMetaObjectRegisterData :
	public IMetaObjectGenericData {
	wxDECLARE_ABSTRACT_CLASS(IMetaObjectRegisterData);
protected:

	//create default attributes
	CPropertyInnerAttribute<>* m_propertyAttributeLineActive = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IMetaObjectSourceData::CreateBoolean(wxT("active"), _("Active"), wxEmptyString, false, true));
	CPropertyInnerAttribute<>* m_propertyAttributePeriod = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IMetaObjectSourceData::CreateDate(wxT("period"), _("Period"), wxEmptyString, eDateFractions::eDateFractions_DateTime, true));
	CPropertyInnerAttribute<>* m_propertyAttributeRecorder = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IMetaObjectSourceData::CreateEmptyType(wxT("recorder"), _("Recorder"), wxEmptyString));
	CPropertyInnerAttribute<>* m_propertyAttributeLineNumber = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IMetaObjectSourceData::CreateNumber(wxT("lineNumber"), _("Line number"), wxEmptyString, 15, 0));

private:
	CRole* m_roleRead = IMetaObject::CreateRole("read", _("read"));
	CRole* m_roleUpdate = IMetaObject::CreateRole("update", _("update"));
protected:
	IMetaObjectRegisterData();
	virtual ~IMetaObjectRegisterData();
public:

	CMetaObjectAttributeDefault* GetRegisterActive() const { return m_propertyAttributeLineActive->GetMetaObject(); }
	bool IsRegisterActive(const meta_identifier_t& id) const { return id == (*m_propertyAttributeLineActive)->GetMetaID(); }
	CMetaObjectAttributeDefault* GetRegisterPeriod() const { return m_propertyAttributePeriod->GetMetaObject(); }
	bool IsRegisterPeriod(const meta_identifier_t& id) const { return id == (*m_propertyAttributePeriod)->GetMetaID(); }
	CMetaObjectAttributeDefault* GetRegisterRecorder() const { return m_propertyAttributeRecorder->GetMetaObject(); }
	bool IsRegisterRecorder(const meta_identifier_t& id) const { return id == (*m_propertyAttributeRecorder)->GetMetaID(); }
	CMetaObjectAttributeDefault* GetRegisterLineNumber() const { return m_propertyAttributeLineNumber->GetMetaObject(); }
	bool IsRegisterLineNumber(const meta_identifier_t& id) const { return id == (*m_propertyAttributeLineNumber)->GetMetaID(); }

	///////////////////////////////////////////////////////////////////

	//override base objects 
	virtual std::vector<IMetaObjectAttribute*> GetGenericAttributes() const;
	virtual std::vector<CMetaObjectForm*> GetGenericForms() const { return GetObjectForms(); }

	//get default attributes
	virtual std::vector<IMetaObjectAttribute*> GetDefaultAttributes() const = 0;

	//get attributes, form etc.. 
	virtual std::vector<IMetaObjectAttribute*> GetObjectDimensions() const;
	virtual std::vector<IMetaObjectAttribute*> GetObjectResources() const;
	virtual std::vector<IMetaObjectAttribute*> GetObjectAttributes() const;

	//get dimension keys 
	virtual std::vector<IMetaObjectAttribute*> GetGenericDimensions() const = 0;

	virtual std::vector<CMetaObjectForm*> GetObjectForms() const;
	virtual std::vector<CMetaObjectGrid*> GetObjectTemplates() const;

	//find attributes, tables etc 
	virtual IMetaObjectAttribute* FindDefAttributeByGuid(const wxString& strDocPath) const {
		for (auto& obj : GetDefaultAttributes()) {
			if (strDocPath == obj->GetDocPath())
				return obj;
		}
		return nullptr;
	}

	virtual IMetaObjectAttribute* FindDimensionByGuid(const wxString& strDocPath) const {
		for (auto& obj : GetObjectDimensions()) {
			if (strDocPath == obj->GetDocPath())
				return obj;
		}
		return nullptr;
	}

	virtual IMetaObjectAttribute* FindResourceByGuid(const wxString& strDocPath) const {
		for (auto& obj : GetObjectResources()) {
			if (strDocPath == obj->GetDocPath())
				return obj;
		}

		return nullptr;
	}

	virtual IMetaObjectAttribute* FindAttributeByGuid(const wxString& strDocPath) const {
		for (auto& obj : GetObjectAttributes()) {
			if (strDocPath == obj->GetDocPath())
				return obj;
		}
		return nullptr;
	}

	//find in current metaObject
	virtual IMetaObjectAttribute* FindProp(const meta_identifier_t& id) const;

	//has record manager 
	virtual bool HasRecordManager() const { return false; }

	//has recorder and period 
	virtual bool HasPeriod() const { return false; }
	virtual bool HasRecorder() const { return true; }

	CRecordKeyObject* CreateRecordKeyObjectValue();

	IRecordSetObject* CreateRecordSetObjectValue(bool needInitialize = true);
	IRecordSetObject* CreateRecordSetObjectValue(const CUniquePairKey& uniqueKey, bool needInitialize = true);
	IRecordSetObject* CreateRecordSetObjectValue(IRecordSetObject* source, bool needInitialize = true);

	IRecordSetObject* CopyRecordSetObjectValue(const CUniquePairKey& uniqueKey);

	IRecordManagerObject* CreateRecordManagerObjectValue();
	IRecordManagerObject* CreateRecordManagerObjectValue(const CUniquePairKey& uniqueKey);
	IRecordManagerObject* CreateRecordManagerObjectValue(IRecordManagerObject* source);

	IRecordManagerObject* CopyRecordManagerObjectValue(const CUniquePairKey& uniqueKey);

	//get module object in compose object 
	virtual CMetaObjectModule* GetModuleObject() const { return nullptr; }
	virtual CMetaObjectCommonModule* GetModuleManager() const { return nullptr; }

	//Get metaObject by def id
	virtual CMetaObjectForm* GetDefaultFormByID(const form_identifier_t& id) = 0;

	virtual bool FilterChild(const class_identifier_t& clsid) const {
		if (
			clsid == g_metaDimensionCLSID ||
			clsid == g_metaResourceCLSID ||
			clsid == g_metaAttributeCLSID
			)
			return true;
		return IMetaObjectGenericData::FilterChild(clsid);
	}

	//meta events
	virtual bool OnCreateMetaObject(IMetaData* metaData, int flags);
	virtual bool OnLoadMetaObject(IMetaData* metaData);
	virtual bool OnSaveMetaObject(int flags);
	virtual bool OnDeleteMetaObject();

	//module manager is started or exit 
	virtual bool OnBeforeRunMetaObject(int flags);
	virtual bool OnAfterCloseMetaObject();

	//get default form 
	virtual IBackendValueForm* GetDefaultCommandForm() { return GetListForm(); }

#pragma region _form_builder_h_
	//support form 
	virtual IBackendValueForm* GetListForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid) = 0;
#pragma endregion 

	//special functions for DB 
	virtual wxString GetTableNameDB() const;

	//create and update table 
	virtual bool CreateAndUpdateTableDB(IMetaDataConfiguration* srcMetaData, IMetaObject* srcMetaObject, int flags);

	//process default query
	int ProcessDimension(const wxString& tableName, IMetaObjectAttribute* srcAttr, IMetaObjectAttribute* dstAttr);
	int ProcessResource(const wxString& tableName, IMetaObjectAttribute* srcAttr, IMetaObjectAttribute* dstAttr);
	int ProcessAttribute(const wxString& tableName, IMetaObjectAttribute* srcAttr, IMetaObjectAttribute* dstAttr);

protected:

	bool UpdateCurrentRecords(const wxString& tableName, IMetaObjectRegisterData* dst);

	///////////////////////////////////////////////////////////////////

	virtual IRecordSetObject* CreateRecordSetObjectRegValue(const CUniquePairKey& uniqueKey = wxNullUniquePairKey) = 0;
	virtual IRecordManagerObject* CreateRecordManagerObjectRegValue(const CUniquePairKey& uniqueKey = wxNullUniquePairKey) { return nullptr; }

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());
	virtual bool DeleteData() { return true; }
};

//********************************************************************************************
//*                                      Base data                                           *
//********************************************************************************************

#include "backend/srcExplorer.h"

class BACKEND_API ISourceDataObject : public ISourceObject {
public:

	virtual ~ISourceDataObject() {}

	//override default type object 
	virtual bool IsNewObject() const { return true; }
	virtual bool SaveModify() { return true; }

	//standart override 
	virtual inline bool IsEmpty() const = 0;

	//standart override 
	virtual bool IsModified() const { return false; }

	//get unique identifier 
	virtual CUniqueKey GetGuid() const = 0;

	//get metaData from object 
	virtual IMetaObjectGenericData* GetSourceMetaObject() const = 0;

	//support source data 
	virtual CSourceExplorer GetSourceExplorer() const = 0;
	virtual bool GetModel(IValueModel*& tableValue, const meta_identifier_t& id) = 0;

	//support source set/get data 
	virtual bool SetValueByMetaID(const meta_identifier_t& id, const CValue& varMetaVal) { return false; }
	virtual bool GetValueByMetaID(const meta_identifier_t& id, CValue& pvarMetaVal) const { return false; }

	//counter
	virtual void SourceIncrRef() = 0;
	virtual void SourceDecrRef() = 0;

	//operator 
	virtual operator CValue() const = 0;
};

//********************************************************************************************
//*                                      Object                                              *
//********************************************************************************************

#pragma region __transaction_guard_h__

template <typename db_type = class BACKEND_API IDatabaseLayer>
struct CTransactionGuard
{
	CTransactionGuard() : m_db(CApplicationData::GetDatabaseLayer()), m_active_transaction(false) {}
	CTransactionGuard(std::shared_ptr<db_type>& db) : m_db(db), m_active_transaction(false) {}
	~CTransactionGuard() { RollBackTransaction(); }

	/// Begin a transaction
	void BeginTransaction() {
		if (!m_active_transaction && m_db != nullptr)
			m_db->BeginTransaction();
		m_active_transaction = true;
	}

	/// Commit the current transaction
	void CommitTransaction() {
		if (m_active_transaction && m_db != nullptr)
			m_db->Commit();
		m_active_transaction = false;
	}

	/// Rollback the current transaction
	void RollBackTransaction() {
		if (m_active_transaction && m_db != nullptr)
			m_db->RollBack();
		m_active_transaction = false;
	}

private:
	bool m_active_transaction;
	std::shared_ptr<db_type> m_db;
};

#pragma endregion 

#pragma region objects 
//Object with metaobject 
class BACKEND_API IRecordDataObject : public CValue, public IActionDataObject,
	public ISourceDataObject, public IValueDataObject, public IModuleDataObject {
	wxDECLARE_ABSTRACT_CLASS(IRecordDataObject);
protected:
	enum helperAlias {
		eSystem,
		eProperty,
		eTable,
		eProcUnit
	};
	enum helperProp {
		eThisObject
	};
	enum helperFunc {
		eGetFormObject,
		eGetMetadata
	};
protected:
	//override copy constructor
	IRecordDataObject(const CGuid& objGuid, bool newObject);
	IRecordDataObject(const IRecordDataObject& source);

	//standart override 
	virtual CMethodHelper* GetPMethods() const final { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}
public:
	virtual ~IRecordDataObject();

	//support actionData 
	virtual CActionCollection GetActionCollection(const form_identifier_t& formType) { return CActionCollection(); }
	virtual void ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm) {}

	virtual IRecordDataObject* CopyObjectValue() = 0;

	//standart override 
	virtual void PrepareNames() const;

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);

	virtual bool CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray);
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);

	//check is empty
	virtual inline bool IsEmpty() const { return CValue::IsEmpty(); }

	//get metaData from object 
	virtual IMetaObjectGenericData* GetSourceMetaObject() const final { return GetMetaObject(); }

	//Get ref class 
	virtual class_identifier_t GetSourceClassType() const final { return GetClassType(); }

	//Get presentation 
	virtual wxString GetSourceCaption() const {
		return GetMetaObject() ? stringUtils::GenerateSynonym(GetMetaObject()->GetClassName()) + wxT(": ") + GetMetaObject()->GetSynonym() : GetString();
	}

	//support source data 
	virtual CSourceExplorer GetSourceExplorer() const;
	virtual bool GetModel(IValueModel*& tableValue, const meta_identifier_t& id);

	//support source set/get data 
	virtual bool SetValueByMetaID(const meta_identifier_t& id, const CValue& varMetaVal);
	virtual bool GetValueByMetaID(const meta_identifier_t& id, CValue& pvarMetaVal) const;

	CValue GetValueByMetaID(const meta_identifier_t& id) const {
		CValue retValue;
		if (GetValueByMetaID(id, retValue))
			return retValue;
		return CValue();
	}

	//support tabular section 
	virtual IValueTable* GetTableByMetaID(const meta_identifier_t& id) const;

	//counter
	virtual void SourceIncrRef() { CValue::IncrRef(); }
	virtual void SourceDecrRef() { CValue::DecrRef(); }

	//get metaData from object 
	virtual IMetaObjectRecordData* GetMetaObject() const = 0;

	//get unique identifier 
	virtual CUniqueKey GetGuid() const = 0;

	//get frame
	virtual IBackendValueForm* GetForm() const;

#pragma region _form_builder_h_
	//support show 
	virtual void ShowFormValue(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr) = 0;
	virtual IBackendValueForm* GetFormValue(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr) = 0;
#pragma endregion 

	//default showing
	virtual void ShowValue() override { ShowFormValue(); }

	//save modify 
	virtual bool SaveModify() override { return true; }

	//Get ref class 
	virtual class_identifier_t GetClassType() const;

	virtual wxString GetClassName() const;
	virtual wxString GetString() const;

	//operator 
	virtual operator CValue() const {
		return this;
	}

	//Working with iterators
	virtual bool HasIterator() const { return true; }
	virtual CValue GetIteratorAt(unsigned int lPropNum) { CValue retValue; GetPropVal(lPropNum, retValue); return retValue; }
	virtual unsigned int GetIteratorCount() const { return m_methodHelper != nullptr ? m_methodHelper->GetNProps() : 0; }

protected:
	virtual void PrepareEmptyObject();
protected:
	friend class IMetaData;
	friend class IMetaObjectRecordData;
protected:
	CMethodHelper* m_methodHelper;
};

//Object with file
class BACKEND_API IRecordDataObjectExt : public IRecordDataObject {
	wxDECLARE_ABSTRACT_CLASS(IRecordDataObjectExt);
protected:
	//override copy constructor
	IRecordDataObjectExt(IMetaObjectRecordDataExt* metaObject);
	IRecordDataObjectExt(const IRecordDataObjectExt& source);
public:
	virtual ~IRecordDataObjectExt();

	virtual bool InitializeObject();
	virtual bool InitializeObject(IRecordDataObjectExt* source);

	//get unique identifier 
	virtual CUniqueKey GetGuid() const {
		return m_objGuid;
	}

	//check is empty
	virtual inline bool IsEmpty() const {
		return false;
	}

	//copy new object
	virtual IRecordDataObjectExt* CopyObjectValue();

	//get metaData from object 
	virtual IMetaObjectRecordDataExt* GetMetaObject() const {
		return m_metaObject;
	}
protected:
	IMetaObjectRecordDataExt* m_metaObject;
};

//Object with reference type 
class BACKEND_API IRecordDataObjectRef : public IRecordDataObject {
	wxDECLARE_ABSTRACT_CLASS(IRecordDataObjectRef);
protected:

	// code generator
	static CValue GenerateNextCode(
		IMetaObjectRecordDataMutableRef* metaObject, IMetaObjectAttribute* attribute);

protected:
	IRecordDataObjectRef(IMetaObjectRecordDataMutableRef* metaObject, const CGuid& objGuid);
	IRecordDataObjectRef(const IRecordDataObjectRef& src);
public:

	virtual ~IRecordDataObjectRef();

	virtual bool InitializeObject(const CGuid& copyGuid = wxNullGuid);
	virtual bool InitializeObject(IRecordDataObjectRef* source, bool generate = false);

	virtual bool WriteObject() = 0;
	virtual bool DeleteObject() = 0;

	//Get ref class 
	virtual class_identifier_t GetClassType() const;

	virtual wxString GetClassName() const;
	virtual wxString GetString() const;

	//check is empty
	virtual inline bool IsEmpty() const {
		return !m_objGuid.isValid();
	}

	//is modified 
	virtual bool IsModified() const { return m_objModified; }

	//Get presentation 
	virtual wxString GetSourceCaption() const {
		return m_metaObject->GetSynonym() + wxT(": ") + (IsNewObject() ? _("Creating") : GetString());
	}

	//support source data 
	virtual CSourceExplorer GetSourceExplorer() const;
	virtual bool GetModel(IValueModel*& tableValue, const meta_identifier_t& id);

	//get metaData from object 
	virtual IMetaObjectRecordDataMutableRef* GetMetaObject() const { return m_metaObject; }

	//set modify 
	virtual void Modify(bool mod);

	//default methods
	virtual bool Generate();

	//filling object 
	virtual bool Filling(CValue& cValue = CValue()) const;

	//support source set/get data 
	virtual bool SetValueByMetaID(const meta_identifier_t& id, const CValue& varMetaVal);
	virtual bool GetValueByMetaID(const meta_identifier_t& id, CValue& pvarMetaVal) const;

	CValue GetValueByMetaID(const meta_identifier_t& id) const {
		CValue retValue;
		if (GetValueByMetaID(id, retValue))
			return retValue;
		return CValue();
	}

	//get unique identifier 
	virtual CUniqueKey GetGuid() const {
		return m_objGuid;
	}

	//copy new object
	virtual IRecordDataObjectRef* CopyObjectValue();

	//is new object?
	virtual bool IsNewObject() const { return m_newObject; }

	//get reference
	virtual class CReferenceDataObject* GetReference() const;

public:
	virtual void SetDeletionMark(bool deletionMark = true);
protected:
	virtual bool ReadData();
	virtual bool ReadData(const CGuid& srcGuid);
	virtual bool SaveData();
	virtual bool DeleteData();
protected:
	virtual void PrepareEmptyObject();
	virtual void PrepareEmptyObject(const IRecordDataObjectRef* source);
protected:

	friend class CTabularSectionDataObjectRef;

	bool m_objModified;
	IMetaObjectRecordDataMutableRef* m_metaObject;
	reference_t* m_reference_impl;
};

//Object with reference type and group/object type 
class BACKEND_API IRecordDataObjectFolderRef : public IRecordDataObjectRef {
	wxDECLARE_ABSTRACT_CLASS(IRecordDataObjectFolderRef);
protected:
	IRecordDataObjectFolderRef(IMetaObjectRecordDataFolderMutableRef* metaObject, const CGuid& objGuid, eObjectMode objMode = eObjectMode::OBJECT_ITEM);
	IRecordDataObjectFolderRef(const IRecordDataObjectFolderRef& src);
public:
	virtual ~IRecordDataObjectFolderRef();

	//support source data 
	virtual CSourceExplorer GetSourceExplorer() const;
	virtual bool GetModel(IValueModel*& tableValue, const meta_identifier_t& id);

	//Get presentation 
	virtual wxString GetSourceCaption() const {
		return m_metaObject->GetSynonym() + wxT(": ") + (IsNewObject() ? _("Creating") : GetString());
	}

	//get metaData from object 
	virtual IMetaObjectRecordDataFolderMutableRef* GetMetaObject() const {
		return (IMetaObjectRecordDataFolderMutableRef*)m_metaObject;
	};

	//copy new object
	virtual IRecordDataObjectRef* CopyObjectValue();

	//support source set/get data 
	virtual bool SetValueByMetaID(const meta_identifier_t& id, const CValue& varMetaVal);
	virtual bool GetValueByMetaID(const meta_identifier_t& id, CValue& pvarMetaVal) const;

	CValue GetValueByMetaID(const meta_identifier_t& id) const {
		CValue retValue;
		if (GetValueByMetaID(id, retValue))
			return retValue;
		return CValue();
	}

protected:
	virtual bool ReadData();
	virtual bool ReadData(const CGuid& srcGuid);
protected:
	virtual void PrepareEmptyObject();
	virtual void PrepareEmptyObject(const IRecordDataObjectRef* source);
protected:
	//folder or object catalog
	eObjectMode m_objMode;
};
#pragma endregion

//Object with register type 
#pragma region registers 
class BACKEND_API CRecordKeyObject : public CValue {
	wxDECLARE_ABSTRACT_CLASS(CRecordKeyObject);
public:
	CRecordKeyObject(IMetaObjectRegisterData* metaObject);
	virtual ~CRecordKeyObject();

	//standart override 
	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}

	virtual void PrepareNames() const;
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);

	//check is empty
	virtual inline bool IsEmpty() const;

	//Get unique key 
	CUniquePairKey GetUniqueKey() { return CUniquePairKey(m_metaObject, m_keyValues); }

	//get metaData from object 
	virtual IMetaObjectRegisterData* GetMetaObject() const { return m_metaObject; };

	//Get ref class 
	virtual class_identifier_t GetClassType() const;

	virtual wxString GetClassName() const;
	virtual wxString GetString() const;

	//Working with iterators
	virtual bool HasIterator() const { return true; }
	virtual CValue GetIteratorAt(unsigned int lPropNum) { CValue retValue; GetPropVal(lPropNum, retValue); return retValue; }
	virtual unsigned int GetIteratorCount() const { return m_methodHelper != nullptr ? m_methodHelper->GetNProps() : 0; }

protected:
	IMetaObjectRegisterData* m_metaObject;
	valueArray_t m_keyValues;
	CMethodHelper* m_methodHelper;
};

class BACKEND_API IRecordSetObject : public IValueTable, public IModuleDataObject {
	wxDECLARE_ABSTRACT_CLASS(IRecordSetObject);
public:

	virtual IValueModelColumnCollection* GetColumnCollection() const { return m_recordColumnCollection; }
	virtual IValueModelReturnLine* GetRowAt(const wxDataViewItem& line) {
		if (!line.IsOk())
			return nullptr;
		return CValue::CreateAndPrepareValueRef<CRecordSetObjectRegisterReturnLine>(this, line);
	}

	class CRecordSetObjectRegisterColumnCollection : public IValueTable::IValueModelColumnCollection {
		wxDECLARE_DYNAMIC_CLASS(CRecordSetObjectRegisterColumnCollection);
	public:

		class CValueRecordSetRegisterColumnInfo : public IValueTable::IValueModelColumnCollection::IValueModelColumnInfo {
			wxDECLARE_DYNAMIC_CLASS(CValueRecordSetRegisterColumnInfo);
		public:

			CValueRecordSetRegisterColumnInfo();
			CValueRecordSetRegisterColumnInfo(IMetaObjectAttribute* metaAttribute);
			virtual ~CValueRecordSetRegisterColumnInfo();

			virtual unsigned int GetColumnID() const { return m_metaAttribute->GetMetaID(); }
			virtual wxString GetColumnName() const { return m_metaAttribute->GetName(); }
			virtual wxString GetColumnCaption() const { return m_metaAttribute->GetSynonym(); }
			virtual const CTypeDescription GetColumnType() const { return m_metaAttribute->GetTypeDesc(); }

			friend CRecordSetObjectRegisterColumnCollection;

		private:
			IMetaObjectAttribute* m_metaAttribute;
		};

	public:

		CRecordSetObjectRegisterColumnCollection();
		CRecordSetObjectRegisterColumnCollection(IRecordSetObject* ownerTable);
		virtual ~CRecordSetObjectRegisterColumnCollection();

		virtual const CTypeDescription GetColumnType(unsigned int col) const {
			CValueRecordSetRegisterColumnInfo* columnInfo = m_listColumnInfo.at(col);
			wxASSERT(columnInfo);
			return columnInfo->GetColumnType();
		}

		virtual IValueModelColumnInfo* GetColumnInfo(unsigned int idx) const {
			if (m_listColumnInfo.size() < idx)
				return nullptr;
			auto& it = m_listColumnInfo.begin();
			std::advance(it, idx);
			return it->second;
		}

		virtual unsigned int GetColumnCount() const {
			IMetaObjectGenericData* metaTable = m_ownerTable->GetMetaObject();
			wxASSERT(metaTable);
			const auto& obj = metaTable->GetGenericAttributes();
			return obj.size();
		}

		//array support 
		virtual bool SetAt(const CValue& varKeyValue, const CValue& varValue);
		virtual bool GetAt(const CValue& varKeyValue, CValue& pvarValue);

		friend class IRecordSetObject;

	protected:
		IRecordSetObject* m_ownerTable;
		std::map<meta_identifier_t, CValuePtr<CValueRecordSetRegisterColumnInfo>> m_listColumnInfo;
		CMethodHelper* m_methodHelper;
	};

	class CRecordSetObjectRegisterReturnLine : public IValueModelReturnLine {
		wxDECLARE_DYNAMIC_CLASS(CRecordSetObjectRegisterReturnLine);
	public:

		CRecordSetObjectRegisterReturnLine(IRecordSetObject* ownerTable = nullptr,
			const wxDataViewItem& line = wxDataViewItem(nullptr));
		virtual ~CRecordSetObjectRegisterReturnLine();

		virtual IValueTable* GetOwnerModel() const { return m_ownerTable; }

		virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
			//PrepareNames(); 
			return m_methodHelper;
		}

		virtual void PrepareNames() const;

		virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal); //setting attribute
		virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal); //attribute value

		//Get ref class 
		virtual class_identifier_t GetClassType() const;

		virtual wxString GetClassName() const;
		virtual wxString GetString() const;

		friend class IRecordSetObject;
	private:
		IRecordSetObject* m_ownerTable;
		CMethodHelper* m_methodHelper;
	};

	class CRecordSetObjectRegisterKeyValue : public CValue {
		wxDECLARE_DYNAMIC_CLASS(CRecordSetObjectRegisterReturnLine);
	public:
		class CRecordSetObjectRegisterKeyDescriptionValue : public CValue {
			wxDECLARE_DYNAMIC_CLASS(CRecordSetObjectRegisterKeyDescriptionValue);
		public:

			CRecordSetObjectRegisterKeyDescriptionValue(IRecordSetObject* recordSet = nullptr, const meta_identifier_t& id = wxNOT_FOUND);
			virtual ~CRecordSetObjectRegisterKeyDescriptionValue();

			virtual inline bool IsEmpty() const { return false; }

			//****************************************************************************
			//*                              Support methods                             *
			//****************************************************************************

			virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
				//PrepareNames(); 
				return m_methodHelper;
			}

			virtual void PrepareNames() const;                             // this method is automatically called to initialize attribute and method names
			virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);       // method call

			virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);//setting attribute
			virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);//attribute value

		protected:
			meta_identifier_t m_metaId;
			CMethodHelper* m_methodHelper;
			IRecordSetObject* m_recordSet;
		};
	public:

		CRecordSetObjectRegisterKeyValue(IRecordSetObject* recordSet = nullptr);
		virtual ~CRecordSetObjectRegisterKeyValue();

		virtual inline bool IsEmpty() const { return false; }

		//****************************************************************************
		//*                              Support methods                             *
		//****************************************************************************

		virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
			//PrepareNames(); 
			return m_methodHelper;
		}

		virtual void PrepareNames() const;                             // this method is automatically called to initialize attribute and method names
		virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);       // method call

		virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);//setting attribute
		virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);//attribute value

	protected:
		IRecordSetObject* m_recordSet;
		CMethodHelper* m_methodHelper;
	};

protected:
	IRecordSetObject(IMetaObjectRegisterData* metaObject, const CUniquePairKey& uniqueKey);
	IRecordSetObject(const IRecordSetObject& source);
public:
	virtual ~IRecordSetObject();

	virtual void CreateEmptyKey();
	virtual bool InitializeObject(const IRecordSetObject* source = nullptr, bool newRecord = false);

	bool FindKeyValue(const meta_identifier_t& id) const;
	void SetKeyValue(const meta_identifier_t& id, const CValue& cValue);
	CValue GetKeyValue(const meta_identifier_t& id) const;
	void EraseKeyValue(const meta_identifier_t& id);

	//copy new object
	virtual IRecordSetObject* CopyRegisterValue();

	bool Selected() const { return m_selected; }
	void Read() { ReadData(); }

	//standart override 
	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}

	//counter
	virtual void SourceIncrRef() { CValue::IncrRef(); }
	virtual void SourceDecrRef() { CValue::DecrRef(); }

	//check is empty object? 
	virtual inline bool IsEmpty() const { return !m_selected; }

	//set modify 
	virtual void Modify(bool mod) { m_objModified = mod; }

	//is modified 
	virtual bool IsModified() const { return m_objModified; }

	//get metaData from object 
	virtual IMetaObjectRegisterData* GetMetaObject() const { return m_metaObject; }

#pragma region _tabular_data_
	//get metaData from object 
	virtual IMetaObjectSourceData* GetSourceMetaObject() const { return GetMetaObject(); }

	//Get ref class 
	virtual class_identifier_t GetSourceClassType() const { return GetClassType(); }
#pragma endregion 

	virtual bool AutoCreateColumn() const { return false; }
	virtual bool EditableLine(const wxDataViewItem& item, unsigned int col) const {
		return false;
	}

	virtual void ActivateItem(IBackendValueForm* formOwner,
		const wxDataViewItem& item, unsigned int col) {
		IValueTable::RowValueStartEdit(item, col);
	}

	virtual void AddValue(unsigned int before = 0) {}
	virtual void CopyValue() {}
	virtual void EditValue() {}
	virtual void DeleteValue() {}

	// implementation of base class virtuals to define model
	virtual void GetValueByRow(wxVariant& variant,
		const wxDataViewItem& row, unsigned int col) const override;
	virtual bool SetValueByRow(const wxVariant& variant,
		const wxDataViewItem& row, unsigned int col) override;

	//support def. methods (in runtime)
	virtual long AppendRow(unsigned int before = 0);

	virtual bool LoadDataFromTable(IValueTable* srcTable);
	virtual IValueTable* SaveDataToTable() const;

	//default methods
	virtual bool WriteRecordSet(bool replace = true, bool clearTable = true) = 0;
	virtual bool DeleteRecordSet() = 0;

	//array
	virtual bool GetAt(const CValue& varKeyValue, CValue& pvarValue);

	//Get ref class 
	virtual class_identifier_t GetClassType() const;

	virtual wxString GetClassName() const;
	virtual wxString GetString() const;

	//operator 
	virtual operator CValue() const { return this; }

	//Working with iterators
	virtual bool HasIterator() const override { return true; }

	virtual CValue GetIteratorEmpty() override {
		return CValue::CreateAndPrepareValueRef<CRecordSetObjectRegisterReturnLine>(this, wxDataViewItem(nullptr));
	}

	virtual CValue GetIteratorAt(unsigned int idx) override {
		if (idx > (unsigned int)GetRowCount())
			return wxEmptyValue;
		return CValue::CreateAndPrepareValueRef<CRecordSetObjectRegisterReturnLine>(this, GetItem(idx));
	}

	virtual unsigned int GetIteratorCount() const override { return GetRowCount(); }

protected:

	virtual bool ExistData();
	virtual bool ExistData(number_t& lastNum); //for records
	virtual bool ReadData();
	virtual bool ReadData(const CUniquePairKey& key);
	virtual bool SaveData(bool replace = true, bool clearTable = true);
	virtual bool DeleteData();

	////////////////////////////////////////

	virtual bool SaveVirtualTable() { return true; }
	virtual bool DeleteVirtualTable() { return true; }

protected:

	//set meta/get meta
	virtual bool SetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id, const CValue& varMetaVal);
	virtual bool GetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id, CValue& pvarMetaVal) const;

protected:

	friend class IRecordManagerObject;

	bool m_objModified;
	bool m_selected;

	valueArray_t m_keyValues;

	IMetaObjectRegisterData* m_metaObject;

	CValuePtr<CRecordSetObjectRegisterColumnCollection> m_recordColumnCollection;
	CValuePtr<CRecordSetObjectRegisterKeyValue> m_recordSetKeyValue;

	CMethodHelper* m_methodHelper;
};

class BACKEND_API IRecordManagerObject : public CValue,
	public ISourceDataObject, public IActionDataObject {
	wxDECLARE_ABSTRACT_CLASS(IRecordManagerObject);
protected:
	IRecordManagerObject(IMetaObjectRegisterData* metaObject, const CUniquePairKey& uniqueKey);
	IRecordManagerObject(const IRecordManagerObject& source);
public:

	IRecordSetObject* GetRecordSet() const { return m_recordSet; }

	virtual ~IRecordManagerObject();

	virtual void CreateEmptyKey();

	virtual bool InitializeObject(const IRecordManagerObject* source = nullptr, bool newRecord = false);
	virtual bool InitializeObject(const CUniquePairKey& key);

	//get metaData from object 
	virtual IMetaObjectGenericData* GetSourceMetaObject() const final { return GetMetaObject(); }

	//Get ref class 
	virtual class_identifier_t GetSourceClassType() const final { return GetClassType(); }

	//Get presentation 
	virtual wxString GetSourceCaption() const {
		return m_metaObject->GetSynonym() + wxT(": ") + (IsNewObject() ? _("Creating") : m_metaObject->GetSynonym());
	}

	//copy new object
	virtual IRecordManagerObject* CopyRegisterValue();

	//standart override 
	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}

	//counter
	virtual void SourceIncrRef() { CValue::IncrRef(); }
	virtual void SourceDecrRef() { CValue::DecrRef(); }

	//get unique identifier 
	virtual CUniqueKey GetGuid() const { return m_objGuid; };

	//save modify 
	virtual bool SaveModify() override { return WriteRegister(); }

	virtual bool IsNewObject() const { return !m_recordSet->m_selected; }

	//default methods
	virtual bool WriteRegister(bool replace = true) = 0;
	virtual bool DeleteRegister() = 0;

	//check is empty
	virtual inline bool IsEmpty() const;

	//set modify 
	virtual void Modify(bool mod);

	//is modified 
	virtual bool IsModified() const;

	//support source set/get data 
	virtual bool SetValueByMetaID(const meta_identifier_t& id, const CValue& varMetaVal);
	virtual bool GetValueByMetaID(const meta_identifier_t& id, CValue& pvarMetaVal) const;

	CValue GetValueByMetaID(const meta_identifier_t& id) const {
		CValue retValue;
		if (GetValueByMetaID(id, retValue))
			return retValue;
		return CValue();
	}

	//support source data 
	virtual CSourceExplorer GetSourceExplorer() const;
	virtual bool GetModel(IValueModel*& tableValue, const meta_identifier_t& id);

	//get metaData from object 
	virtual IMetaObjectRegisterData* GetMetaObject() const {
		return m_metaObject;
	};

	//get frame
	virtual IBackendValueForm* GetForm() const;

#pragma region _form_builder_h_
	//support show 
	virtual void ShowFormValue(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr) = 0;
	virtual IBackendValueForm* GetFormValue(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr) = 0;
#pragma endregion 

	//default showing
	virtual void ShowValue() override { ShowFormValue(); }

	//Get ref class 
	virtual class_identifier_t GetClassType() const;

	virtual wxString GetClassName() const;
	virtual wxString GetString() const;

	//operator 
	virtual operator CValue() const { return this; }

	//Working with iterators
	virtual bool HasIterator() const { return true; }
	virtual CValue GetIteratorAt(unsigned int lPropNum) { CValue retValue; GetPropVal(lPropNum, retValue); return retValue; }
	virtual unsigned int GetIteratorCount() const { return m_methodHelper != nullptr ? m_methodHelper->GetNProps() : 0; }

protected:

	virtual void PrepareEmptyObject(const IRecordManagerObject* source);

protected:

	virtual bool ExistData();
	virtual bool ReadData(const CUniquePairKey& key);
	virtual bool SaveData(bool replace = true);
	virtual bool DeleteData();

protected:

	CUniquePairKey m_objGuid;

	IMetaObjectRegisterData* m_metaObject;

	CValuePtr<IRecordSetObject> m_recordSet;
	CValuePtr<IValueTable::IValueModelReturnLine> m_recordLine;

	CMethodHelper* m_methodHelper;
};
#pragma endregion

#endif 