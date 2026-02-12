#ifndef __COMMON_OBJECT_H__
#define __COMMON_OBJECT_H__

#include "reference/reference.h"

//special object 
#include "backend/metaCollection/metaModuleObject.h"
#include "backend/metaCollection/metaFormObject.h"
#include "backend/metaCollection/metaSpreadsheetObject.h"

//interface
#include "backend/metaCollection/metaInterfaceObject.h"

//attributes 
#include "backend/metaCollection/attribute/metaAttributeObject.h"
#include "backend/metaCollection/dimension/metaDimensionObject.h"
#include "backend/metaCollection/resource/metaResourceObject.h"

//enumeration 
#include "backend/metaCollection/enumeration/metaEnumObject.h"

//tables 
#include "backend/metaCollection/table/metaTableObject.h"

//guid 
#include "backend/system/value/valueGuid.h"

#include "backend/actionInfo.h"
#include "backend/moduleInfo.h"
#include "backend/valueInfo.h"
#include "backend/tableInfo.h"

//********************************************************************************************
//*                                     Defines                                              *
//********************************************************************************************

class BACKEND_API CValueMetaObjectConstant;

class BACKEND_API IValueMetaObjectGenericData;
class BACKEND_API IValueMetaObjectRecordData;
class BACKEND_API IValueMetaObjectRecordDataRef;
class BACKEND_API IValueMetaObjectRegisterData;

class BACKEND_API ISourceDataObject;

class BACKEND_API IValueManagerDataObject;

class BACKEND_API IValueRecordDataObject;
class BACKEND_API IValueRecordDataObjectExt;
class BACKEND_API IValueRecordDataObjectRef;
class BACKEND_API IValueRecordDataObjectFolderRef;

class BACKEND_API CValueRecordKeyObject;
class BACKEND_API IValueRecordManagerObject;
class BACKEND_API IValueRecordSetObject;

class BACKEND_API CSourceExplorer;

//special names 
#define guidName wxT("uuid")

//********************************************************************************************
//*                                  Factory & metaData                                      *
//********************************************************************************************

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

#include "backend/metaCollection/metaObjectComposite.h"

class BACKEND_API IValueMetaObjectGenericData
	: public IValueMetaObjectCompositeData, public IBackendCommandItem {
	wxDECLARE_ABSTRACT_CLASS(IValueMetaObjectGenericData);
public:
	friend class IMetaData;
public:

#pragma region access_generic
	virtual bool AccessRight_Show() const { return true; }
#pragma endregion

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

#pragma region __array_h__

	//form
	std::vector<IValueMetaObjectForm*> GetFormArrayObject(
		std::vector<IValueMetaObjectForm*>& array = std::vector<IValueMetaObjectForm*>()) const {
		FillArrayObjectByFilter<IValueMetaObjectForm>(array, { g_metaFormCLSID });
		return array;
	}

	//grid
	std::vector<IValueMetaObjectSpreadsheet*> GetTemplateArrayObject(
		std::vector<IValueMetaObjectSpreadsheet*> array = std::vector<IValueMetaObjectSpreadsheet*>()) const {
		FillArrayObjectByFilter<IValueMetaObjectSpreadsheet>(array, { g_metaTemplateCLSID });
		return array;
	}

#pragma endregion
#pragma region __filter_h__

	//form
	template <typename _T1>
	IValueMetaObjectForm* FindFormObjectByFilter(const _T1& id, const form_identifier_t& form_id = wxNOT_FOUND) const {
		IValueMetaObjectForm* founded = FindObjectByFilter<IValueMetaObjectForm>(id, { g_metaCommonFormCLSID, g_metaFormCLSID });
		if (founded != nullptr && form_id == founded->GetTypeForm() || form_id == wxNOT_FOUND)
			return founded;
		return nullptr;
	}

	//grid
	template <typename _T1>
	IValueMetaObjectSpreadsheet* FindTemplateObjectByFilter(const _T1& id) const {
		return FindObjectByFilter<IValueMetaObjectSpreadsheet>(id, { g_metaCommonTemplateCLSID, g_metaTemplateCLSID });
	}

#pragma endregion 

	//form events 
	virtual void OnCreateFormObject(IValueMetaObjectForm* metaForm) {}
	virtual void OnRemoveMetaForm(IValueMetaObjectForm* metaForm) {}

	//Get form type
	virtual CFormTypeList GetFormType() const = 0;

	//Get metaObject by def id
	virtual IValueMetaObjectForm* GetDefaultFormByID(const form_identifier_t& id) const { return nullptr; }

	//create form with data 
	virtual IBackendValueForm* CreateObjectForm(IValueMetaObjectForm* metaForm) {
		return IValueMetaObjectGenericData::CreateAndBuildForm(
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

#pragma region _template_builder_h_

	class CValueSpreadsheet* GetTemplate(const wxString& strFormName);

#pragma endregion

	virtual IValueManagerDataObject* CreateManagerDataObjectValue() = 0;

protected:

	//create object data with meta form
	virtual ISourceDataObject* CreateSourceObject(IValueMetaObjectForm* metaObject) { return nullptr; }
};

class BACKEND_API IValueMetaObjectRecordData
	: public IValueMetaObjectGenericData {

	wxDECLARE_ABSTRACT_CLASS(IValueMetaObjectRecordData);

public:

	virtual bool FilterChild(const class_identifier_t& clsid) const {
		if (
			clsid == g_metaAttributeCLSID ||
			clsid == g_metaTableCLSID
			)
			return true;
		return IValueMetaObjectGenericData::FilterChild(clsid);
	}

	//get data selector 
	virtual eSelectorDataType GetFilterDataType() const {
		return eSelectorDataType::eSelectorDataType_any;
	}

	//get module object in compose object 
	virtual CValueMetaObjectModule* GetModuleObject() const { return nullptr; }
	virtual CValueMetaObjectCommonModule* GetModuleManager() const { return nullptr; }

	//meta events
	virtual bool OnLoadMetaObject(IMetaData* metaData);
	virtual bool OnSaveMetaObject(int flags);
	virtual bool OnDeleteMetaObject();

	//module manager is started or exit 
	virtual bool OnBeforeRunMetaObject(int flags);
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

	//any attribute 
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

	//table
	std::vector<CValueMetaObjectTableData*> GetTableArrayObject(
		std::vector<CValueMetaObjectTableData*>& array = std::vector<CValueMetaObjectTableData*>()) const {
		FillArrayObjectByFilter<CValueMetaObjectTableData>(array, { g_metaTableCLSID });
		return array;
	}

#pragma endregion
#pragma region __filter_h__

	//any attribute 
	template <typename _T1>
	IValueMetaObjectAttribute* FindAnyAttributeObjectByFilter(const _T1& id) const {
		return FindObjectByFilter<IValueMetaObjectAttribute>(id, { g_metaAttributeCLSID, g_metaPredefinedAttributeCLSID });
	}

	//attribute 
	template <typename _T1>
	IValueMetaObjectAttribute* FindAttributeObjectByFilter(const _T1& id) const {
		return FindObjectByFilter<IValueMetaObjectAttribute>(id, { g_metaAttributeCLSID });
	}

	//table
	template <typename _T1>
	CValueMetaObjectTableData* FindTableObjectByFilter(const _T1& id) const {
		return FindObjectByFilter<CValueMetaObjectTableData>(id, { g_metaTableCLSID });
	}

#pragma endregion 

	//create single object
	virtual IValueRecordDataObject* CreateRecordDataObjectValue() = 0;

#pragma region _form_builder_h_
	//support form 
	virtual IBackendValueForm* GetObjectForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid) = 0;
#pragma endregion 

protected:

	//get default form 
	virtual IBackendValueForm* GetFormByCommandType(EInterfaceCommandType cmdType = EInterfaceCommandType::EInterfaceCommandType_Default) {

		if (cmdType == EInterfaceCommandType::EInterfaceCommandType_Create) {
			return GetObjectForm();
		}

		return GetObjectForm();
	}
};

enum eObjectMode {
	OBJECT_ITEM = 1,
	OBJECT_FOLDER
};

//metaObject with file 
class BACKEND_API IValueMetaObjectRecordDataExt : public IValueMetaObjectRecordData {
	wxDECLARE_ABSTRACT_CLASS(IValueMetaObjectRecordDataExt);
public:

#pragma region access_generic
	virtual bool AccessRight_Show() const { return AccessRight_Use(); }
#pragma endregion
#pragma region access
	bool AccessRight_Use() const { return IsFullAccess() || AccessRight(m_roleUse); }
#pragma endregion

	//ctor
	IValueMetaObjectRecordDataExt();

	//ñreate from file?
	virtual bool IsExternalCreate() const { return false; }

	//module manager is started or exit 
	virtual bool OnBeforeRunMetaObject(int flags);
	virtual bool OnAfterCloseMetaObject();

	//create associate value 
	IValueRecordDataObjectExt* CreateObjectValue(); //create object
	IValueRecordDataObjectExt* CreateObjectValue(IValueRecordDataObjectExt* objSrc);

	//create single object
	virtual IValueRecordDataObject* CreateRecordDataObjectValue();

	//get command section 
	virtual EInterfaceCommandSection GetCommandSection() const { return EInterfaceCommandSection::EInterfaceCommandSection_Service; }

protected:

	//create empty object
	virtual IValueRecordDataObjectExt* CreateObjectExtValue() = 0;  //create object 

private:
#pragma region role
	CRole* m_roleUse = IValueMetaObject::CreateRole(wxT("use"), _("Use"));
#pragma endregion
};

//metaObject with reference 
class BACKEND_API IValueMetaObjectRecordDataRef : public IValueMetaObjectRecordData {
	wxDECLARE_ABSTRACT_CLASS(IValueMetaObjectRecordDataRef);

protected:
	//ctor
	IValueMetaObjectRecordDataRef();
	virtual ~IValueMetaObjectRecordDataRef();
public:

	virtual CValueMetaObjectAttributePredefined* GetDataReference() const { return m_propertyAttributeReference->GetMetaObject(); }
	virtual bool IsDataReference(const meta_identifier_t& id) const { return id == (*m_propertyAttributeReference)->GetMetaID(); }

	virtual bool HasQuickChoice() const {
		return m_propertyQuickChoice->GetValueAsBoolean();
	}

	//get data selector 
	virtual eSelectorDataType GetFilterDataType() const { return eSelectorDataType::eSelectorDataType_reference; }

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
	virtual IValueRecordDataObject* CreateRecordDataObjectValue() {
		wxASSERT_MSG(false, "IValueMetaObjectRecordDataRef::CreateRecordDataObjectValue");
		return nullptr;
	}

	//process choice 
	virtual bool ProcessChoice(IBackendControlFrame* ownerValue,
		const wxString& strFormName = wxEmptyString, eSelectMode selMode = eSelectMode::eSelectMode_Items);

#pragma region __generic_h__

	//searched 
	virtual std::vector<IValueMetaObjectAttribute*> GetSearchedAttributeObjectArray(
		std::vector<IValueMetaObjectAttribute*>& array = std::vector<IValueMetaObjectAttribute*>()) const {
		FillArrayObjectBySearched(array);
		return array;
	}

#pragma endregion 

	//get attribute code 
	virtual IValueMetaObjectAttribute* GetAttributeForCode() const { return nullptr; }

	//find object value
	virtual class CValueReferenceDataObject* FindObjectValue(const CGuid& guid); //find by guid and ret reference 

#pragma region _form_builder_h_
	//support form 
	virtual IBackendValueForm* GetListForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid) = 0;
	virtual IBackendValueForm* GetSelectForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid) = 0;
#pragma endregion 

	//descriptions...
	virtual wxString GetDataPresentation(const IValueDataObject* objValue) const = 0;

	//special functions for DB 
	virtual wxString GetTableNameDB() const;

protected:

	//get default form 
	virtual IBackendValueForm* GetFormByCommandType(EInterfaceCommandType cmdType = EInterfaceCommandType::EInterfaceCommandType_Default) {

		if (cmdType == EInterfaceCommandType::EInterfaceCommandType_Create)
			return GetObjectForm();
		else if (cmdType == EInterfaceCommandType::EInterfaceCommandType_List)
			return GetListForm();
		else if (cmdType == EInterfaceCommandType::EInterfaceCommandType_Select)
			return GetSelectForm();

		return GetListForm();
	}

	//searched array 
	virtual bool FillArrayObjectBySearched(std::vector<IValueMetaObjectAttribute*>& array) const { return true; }

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());
	virtual bool DeleteData() { return true; }

protected:

	CPropertyCategory* m_categoryData = IPropertyObject::CreatePropertyCategory(wxT("data"), _("Data"));
	CPropertyCategory* m_categoryPresentation = IPropertyObject::CreatePropertyCategory(wxT("presentation"), _("Presentation"));
	CPropertyBoolean* m_propertyQuickChoice = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryPresentation, wxT("quickChoice"), _("Quick choice"), false);
	CPropertyInnerAttribute<>* m_propertyAttributeReference = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IValueMetaObjectCompositeData::CreateSpecialType(wxT("reference"), _("Reference"), wxEmptyString, CValue::GetIDByVT(eValueTypes::TYPE_EMPTY)));
};

//metaObject with reference - for enumeration
class BACKEND_API IValueMetaObjectRecordDataEnumRef : public IValueMetaObjectRecordDataRef {
	wxDECLARE_ABSTRACT_CLASS(IValueMetaObjectRecordDataEnumRef);
protected:

	//ctor
	IValueMetaObjectRecordDataEnumRef();
	virtual ~IValueMetaObjectRecordDataEnumRef();

public:

	CValueMetaObjectAttributePredefined* GetDataOrder() const { return m_propertyAttributeOrder->GetMetaObject(); }
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

#pragma region __array_h__

	//enum
	std::vector<CValueMetaObjectEnum*> GetEnumObjectArray(
		std::vector<CValueMetaObjectEnum*>& array = std::vector<CValueMetaObjectEnum*>()) const {
		FillArrayObjectByFilter<CValueMetaObjectEnum>(array, { g_metaEnumCLSID });
		return array;
	}

#pragma endregion
#pragma region __filter_h__

	//enum
	template <typename _T1>
	CValueMetaObjectEnum* FindEnumObjectByFilter(const _T1& id) const {
		return FindObjectByFilter<CValueMetaObjectEnum>(id, { g_metaEnumCLSID });
	}

#pragma endregion 

protected:

	//predefined array 
	virtual bool FillArrayObjectByPredefined(std::vector<IValueMetaObjectAttribute*>& array) const {
		array = { m_propertyAttributeReference->GetMetaObject() };
		return true;
	}

	//searched array 
	virtual bool FillArrayObjectBySearched(std::vector<IValueMetaObjectAttribute*>& array) const { return true; }

	//create and update table 
	virtual bool CreateAndUpdateTableDB(IMetaDataConfiguration* srcMetaData, IValueMetaObject* srcMetaObject, int flags);

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

	//process default query
	int ProcessEnumeration(const wxString& tableName, const CValueMetaObjectEnum* srcEnum, const CValueMetaObjectEnum* dstEnum);

private:

	//default attributes 
	CPropertyInnerAttribute<>* m_propertyAttributeOrder = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IValueMetaObjectCompositeData::CreateNumber(wxT("order"), _("Order"), wxEmptyString, 6, true));
};

//metaObject with reference and deletion mark 
class BACKEND_API IValueMetaObjectRecordDataMutableRef : public IValueMetaObjectRecordDataRef {
	wxDECLARE_ABSTRACT_CLASS(IValueMetaObjectRecordDataMutableRef);
protected:
	//ctor
	IValueMetaObjectRecordDataMutableRef();
	virtual ~IValueMetaObjectRecordDataMutableRef();
public:

#pragma region access_generic
	virtual bool AccessRight_Show() const { return AccessRight_Read(); }
#pragma endregion

	CMetaDescription& GetGenerationDescription() const { return m_propertyGeneration->GetValueAsMetaDesc(); }

#pragma region access
	bool AccessRight_Read() const { return IsFullAccess() || AccessRight(m_roleRead); }
	bool AccessRight_Write() const { return IsFullAccess() || AccessRight(m_roleWrite); }
	bool AccessRight_Delete() const { return IsFullAccess() || AccessRight(m_roleDelete); }
#pragma endregion

	CValueMetaObjectAttributePredefined* GetDataVersion() const { return m_propertyAttributeDataVersion->GetMetaObject(); }
	bool IsDataVersion(const meta_identifier_t& id) const { return id == (*m_propertyAttributeDataVersion)->GetMetaID(); }

	CValueMetaObjectAttributePredefined* GetDataDeletionMark() const { return m_propertyAttributeDeletionMark->GetMetaObject(); }
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
	IValueRecordDataObjectRef* CreateObjectValue();
	IValueRecordDataObjectRef* CreateObjectValue(const CGuid& guid);
	IValueRecordDataObjectRef* CreateObjectValue(IValueRecordDataObjectRef* objSrc, bool generate = false);

	//copy associate value 	
	IValueRecordDataObjectRef* CopyObjectValue(const CGuid& guid);

	//create single object
	virtual IValueRecordDataObject* CreateRecordDataObjectValue();

	//get command section 
	virtual EInterfaceCommandSection GetCommandSection() const { return EInterfaceCommandSection::EInterfaceCommandSection_Combined; }

protected:

	//predefined array 
	virtual bool FillArrayObjectByPredefined(std::vector<IValueMetaObjectAttribute*>& array) const {

		array = {
			m_propertyAttributeReference->GetMetaObject(),
			m_propertyAttributeDeletionMark->GetMetaObject()
		};

		return true;
	}

	//searched array 
	virtual bool FillArrayObjectBySearched(std::vector<IValueMetaObjectAttribute*>& array) const { return true; }

	/**
	* Property events
	*/
	virtual void OnPropertyCreated(IProperty* property);
	virtual void OnPropertyRefresh(class wxPropertyGridManager* pg, class wxPGProperty* pgProperty, IProperty* property);
	virtual bool OnPropertyChanging(IProperty* property, const wxVariant& newValue);
	virtual void OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue);

	//create and update table 
	virtual bool CreateAndUpdateTableDB(IMetaDataConfiguration* srcMetaData, IValueMetaObject* srcMetaObject, int flags);

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

	//process default query
	int ProcessAttribute(const wxString& tableName, const IValueMetaObjectAttribute* srcAttr, const IValueMetaObjectAttribute* dstAttr);
	int ProcessTable(const wxString& tabularName, const CValueMetaObjectTableData* srcTable, const CValueMetaObjectTableData* dstTable);

	//create empty object
	virtual IValueRecordDataObjectRef* CreateObjectRefValue(const CGuid& objGuid = wxNullGuid) = 0; //create object and read by guid 

protected:

	CPropertyGeneration* m_propertyGeneration = IPropertyObject::CreateProperty<CPropertyGeneration>(m_categoryData, wxT("listGeneration"), _("List generation"));
	CPropertyInnerAttribute<>* m_propertyAttributeDataVersion = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IValueMetaObjectCompositeData::CreateString(wxT("dataVersion"), _("Data version"), wxEmptyString, 12, eItemMode_Folder_Item));
	CPropertyInnerAttribute<>* m_propertyAttributeDeletionMark = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IValueMetaObjectCompositeData::CreateBoolean(wxT("deletionMark"), _("Deletion mark"), wxEmptyString));

private:

#pragma region role
	CRole* m_roleRead = IValueMetaObject::CreateRole(wxT("read"), _("Read"));
	CRole* m_roleWrite = IValueMetaObject::CreateRole(wxT("wrire"), _("Write"));
	CRole* m_roleDelete = IValueMetaObject::CreateRole(wxT("delete"), _("Delete"));
#pragma endregion
};

//metaObject with reference and deletion mark and group/object type
class BACKEND_API IValueMetaObjectRecordDataHierarchyMutableRef : public IValueMetaObjectRecordDataMutableRef {

	wxDECLARE_ABSTRACT_CLASS(IValueMetaObjectRecordDataHierarchyMutableRef);

	struct CPredefinedObjectValue {

		CGuid m_valueGuid;

		CValue m_valueCode;
		CValue m_valueDescription;
		CValue m_valueIsFolder;
	};

public:

	CValueMetaObjectAttributePredefined* GetDataPredefinedName() const { return m_propertyAttributePredefined->GetMetaObject(); }
	virtual bool IsDataPredefinedName(const meta_identifier_t& id) const { return id == (*m_propertyAttributePredefined)->GetMetaID(); }

	CValueMetaObjectAttributePredefined* GetDataCode() const { return m_propertyAttributeCode->GetMetaObject(); }
	virtual bool IsDataCode(const meta_identifier_t& id) const { return id == (*m_propertyAttributeCode)->GetMetaID(); }

	CValueMetaObjectAttributePredefined* GetDataDescription() const { return m_propertyAttributeDescription->GetMetaObject(); }
	virtual bool IsDataDescription(const meta_identifier_t& id) const { return id == (*m_propertyAttributeDescription)->GetMetaID(); }

	CValueMetaObjectAttributePredefined* GetDataParent() const { return m_propertyAttributeParent->GetMetaObject(); }
	virtual bool IsDataParent(const meta_identifier_t& id) const { return id == (*m_propertyAttributeParent)->GetMetaID(); }

	CValueMetaObjectAttributePredefined* GetDataIsFolder() const { return m_propertyAttributeIsFolder->GetMetaObject(); }
	virtual bool IsDataFolder(const meta_identifier_t& id) const { return id == (*m_propertyAttributeIsFolder)->GetMetaID(); }

	virtual bool HasQuickChoice() const { return m_propertyQuickChoice->GetValueAsBoolean(); }

	IValueMetaObjectRecordDataHierarchyMutableRef();
	virtual ~IValueMetaObjectRecordDataHierarchyMutableRef();

	//process choice 
	virtual bool ProcessChoice(IBackendControlFrame* ownerValue,
		const wxString& strFormName = wxEmptyString, eSelectMode selMode = eSelectMode::eSelectMode_Items);

	virtual bool FilterChild(const class_identifier_t& clsid) const {
		if (clsid == g_metaAttributeCLSID ||
			clsid == g_metaTableCLSID)
			return true;
		return IValueMetaObjectGenericData::FilterChild(clsid);
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

	//is predefined value? 
	bool IsPredefinedValue(const CGuid& valueGuid) const { return FindPredefinedValue(valueGuid) != nullptr; }

	//create associate value 	
	IValueRecordDataObjectFolderRef* CreateObjectValue(eObjectMode mode);
	IValueRecordDataObjectFolderRef* CreateObjectValue(eObjectMode mode, const CGuid& guid);
	IValueRecordDataObjectFolderRef* CreateObjectValue(eObjectMode mode, IValueRecordDataObjectRef* objSrc, bool generate = false);

	//copy associate value 	
	IValueRecordDataObjectFolderRef* CopyObjectValue(eObjectMode mode, const CGuid& guid);

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

	//create and update table 
	virtual bool CreateAndUpdateTableDB(IMetaDataConfiguration* srcMetaData, IValueMetaObject* srcMetaObject, int flags);

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

	//create empty object
	virtual IValueRecordDataObjectFolderRef* CreateObjectRefValue(eObjectMode mode, const CGuid& objGuid = wxNullGuid) = 0; //create object and read by guid 
	virtual IValueRecordDataObjectRef* CreateObjectRefValue(const CGuid& objGuid = wxNullGuid) final;

protected:

	//predefined array 
	virtual bool FillArrayObjectByPredefined(std::vector<IValueMetaObjectAttribute*>& array) const {

		array = {
			m_propertyAttributePredefined->GetMetaObject(),
			m_propertyAttributeCode->GetMetaObject(),
			m_propertyAttributeDescription->GetMetaObject(),
			m_propertyAttributeParent->GetMetaObject(),
			m_propertyAttributeIsFolder->GetMetaObject(),
			m_propertyAttributeReference->GetMetaObject(),
			m_propertyAttributeDeletionMark->GetMetaObject(),
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

	//find predefined value
	const CPredefinedObjectValue* FindPredefinedValue(const CGuid& valueGuid) const {

		auto iterator = std::find_if(m_predefinedObjectVector.begin(), m_predefinedObjectVector.end(),
			[valueGuid](const auto& value) { return value.m_valueGuid == valueGuid; });

		if (iterator != m_predefinedObjectVector.end())
			return &(*iterator);

		return nullptr;
	}

	//process default query
	int ProcessPredefinedValue(const wxString& tableName, const CPredefinedObjectValue* srcPredefined, const CPredefinedObjectValue* dstPredefined);

	//create default attributes
	CPropertyInnerAttribute<>* m_propertyAttributePredefined = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IValueMetaObjectCompositeData::CreateString(wxT("predefinedName"), _("Predefined name"), wxEmptyString, 150, eItemMode::eItemMode_Folder_Item));
	CPropertyInnerAttribute<>* m_propertyAttributeCode = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IValueMetaObjectCompositeData::CreateString(wxT("code"), _("Code"), wxEmptyString, 8, true, eItemMode::eItemMode_Folder_Item));
	CPropertyInnerAttribute<>* m_propertyAttributeDescription = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IValueMetaObjectCompositeData::CreateString(wxT("description"), _("Description"), wxEmptyString, 150, true, eItemMode::eItemMode_Folder_Item));
	CPropertyInnerAttribute<>* m_propertyAttributeParent = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IValueMetaObjectCompositeData::CreateEmptyType(wxT("parent"), _("Parent"), wxEmptyString, eItemMode::eItemMode_Folder_Item, eSelectMode::eSelectMode_Folders));
	CPropertyInnerAttribute<>* m_propertyAttributeIsFolder = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IValueMetaObjectCompositeData::CreateBoolean(wxT("isFolder"), _("Is folder"), wxEmptyString, eItemMode::eItemMode_Folder_Item));

	//predefinded vector
	std::vector<CPredefinedObjectValue> m_predefinedObjectVector;
};

//metaObject with key   
class BACKEND_API IValueMetaObjectRegisterData :
	public IValueMetaObjectGenericData {
	wxDECLARE_ABSTRACT_CLASS(IValueMetaObjectRegisterData);
protected:
	IValueMetaObjectRegisterData();
	virtual ~IValueMetaObjectRegisterData();
public:

#pragma region access_generic
	virtual bool AccessRight_Show() const { return AccessRight_Read(); }
#pragma endregion

#pragma region access
	bool AccessRight_Read() const { return IsFullAccess() || AccessRight(m_roleRead); }
	bool AccessRight_Write() const { return IsFullAccess() || AccessRight(m_roleWrite); }
	bool AccessRight_Delete() const { return IsFullAccess() || AccessRight(m_roleDelete); }
#pragma endregion

	CValueMetaObjectAttributePredefined* GetRegisterActive() const { return m_propertyAttributeLineActive->GetMetaObject(); }
	bool IsRegisterActive(const meta_identifier_t& id) const { return id == (*m_propertyAttributeLineActive)->GetMetaID(); }
	CValueMetaObjectAttributePredefined* GetRegisterPeriod() const { return m_propertyAttributePeriod->GetMetaObject(); }
	bool IsRegisterPeriod(const meta_identifier_t& id) const { return id == (*m_propertyAttributePeriod)->GetMetaID(); }
	CValueMetaObjectAttributePredefined* GetRegisterRecorder() const { return m_propertyAttributeRecorder->GetMetaObject(); }
	bool IsRegisterRecorder(const meta_identifier_t& id) const { return id == (*m_propertyAttributeRecorder)->GetMetaID(); }
	CValueMetaObjectAttributePredefined* GetRegisterLineNumber() const { return m_propertyAttributeLineNumber->GetMetaObject(); }
	bool IsRegisterLineNumber(const meta_identifier_t& id) const { return id == (*m_propertyAttributeLineNumber)->GetMetaID(); }

	///////////////////////////////////////////////////////////////////

#pragma region __generic_h__

	//attribute 
	virtual std::vector<IValueMetaObjectAttribute*> GetGenericAttributeArrayObject(
		std::vector<IValueMetaObjectAttribute*>& array = std::vector<IValueMetaObjectAttribute*>()) const {
		FillArrayObjectByPredefined(array);
		FillArrayObjectByFilter<IValueMetaObjectAttribute>(array, { g_metaDimensionCLSID, g_metaResourceCLSID, g_metaAttributeCLSID });
		return array;
	}

	//dimention  
	virtual std::vector<IValueMetaObjectAttribute*> GetGenericDimentionArrayObject(
		std::vector<IValueMetaObjectAttribute*>& array = std::vector<IValueMetaObjectAttribute*>()) const {
		FillArrayObjectByDimention(array);
		return array;
	}

#pragma endregion
#pragma region __array_h__

	//any attribute 
	std::vector<IValueMetaObjectAttribute*> GetAnyAttributeArrayObject(
		std::vector<IValueMetaObjectAttribute*>& array = std::vector<IValueMetaObjectAttribute*>()) const {
		FillArrayObjectByPredefined(array);
		FillArrayObjectByFilter<IValueMetaObjectAttribute>(array, { g_metaDimensionCLSID, g_metaResourceCLSID, g_metaAttributeCLSID });
		return array;
	}

	//dimension
	std::vector<CValueMetaObjectDimension*> GetDimentionArrayObject(
		std::vector<CValueMetaObjectDimension*>& array = std::vector<CValueMetaObjectDimension*>()) const {
		FillArrayObjectByFilter<CValueMetaObjectDimension>(array, { g_metaDimensionCLSID });
		return array;
	}

	//resource
	std::vector<CValueMetaObjectResource*> GetResourceArrayObject(
		std::vector<CValueMetaObjectResource*>& array = std::vector<CValueMetaObjectResource*>()) const {
		FillArrayObjectByFilter<CValueMetaObjectResource>(array, { g_metaResourceCLSID });
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

	//any attribute 
	template <typename _T1>
	IValueMetaObjectAttribute* FindAnyAttributeObjectByFilter(const _T1& id) const {
		return FindObjectByFilter<IValueMetaObjectAttribute>(id, { g_metaDimensionCLSID, g_metaResourceCLSID, g_metaAttributeCLSID, g_metaPredefinedAttributeCLSID });
	}

	//dimension
	template <typename _T1>
	CValueMetaObjectDimension* FindDimensionObjectByFilter(const _T1& id) const {
		return FindObjectByFilter<CValueMetaObjectDimension>(id, { g_metaDimensionCLSID });
	}

	//resource
	template <typename _T1>
	CValueMetaObjectResource* FindResourceObjectByFilter(const _T1& id) const {
		return FindObjectByFilter<CValueMetaObjectResource>(id, { g_metaResourceCLSID });
	}

	//attribute
	template <typename _T1>
	IValueMetaObjectAttribute* FindAttributeObjectByFilter(const _T1& id) const {
		return FindObjectByFilter<CValueMetaObjectResource>(id, { g_metaAttributeCLSID });
	}

#pragma endregion 

	//has record manager 
	virtual bool HasRecordManager() const { return false; }

	//has recorder and period 
	virtual bool HasPeriod() const { return false; }
	virtual bool HasRecorder() const { return true; }

	CValueRecordKeyObject* CreateRecordKeyObjectValue();

	IValueRecordSetObject* CreateRecordSetObjectValue(bool needInitialize = true);
	IValueRecordSetObject* CreateRecordSetObjectValue(const CUniquePairKey& uniqueKey, bool needInitialize = true);
	IValueRecordSetObject* CreateRecordSetObjectValue(IValueRecordSetObject* source, bool needInitialize = true);

	IValueRecordSetObject* CopyRecordSetObjectValue(const CUniquePairKey& uniqueKey);

	IValueRecordManagerObject* CreateRecordManagerObjectValue();
	IValueRecordManagerObject* CreateRecordManagerObjectValue(const CUniquePairKey& uniqueKey);
	IValueRecordManagerObject* CreateRecordManagerObjectValue(IValueRecordManagerObject* source);

	IValueRecordManagerObject* CopyRecordManagerObjectValue(const CUniquePairKey& uniqueKey);

	//get module object in compose object 
	virtual CValueMetaObjectModule* GetModuleObject() const { return nullptr; }
	virtual CValueMetaObjectCommonModule* GetModuleManager() const { return nullptr; }

	virtual bool FilterChild(const class_identifier_t& clsid) const {
		if (
			clsid == g_metaDimensionCLSID ||
			clsid == g_metaResourceCLSID ||
			clsid == g_metaAttributeCLSID
			)
			return true;
		return IValueMetaObjectGenericData::FilterChild(clsid);
	}

	//meta events
	virtual bool OnCreateMetaObject(IMetaData* metaData, int flags);
	virtual bool OnLoadMetaObject(IMetaData* metaData);
	virtual bool OnSaveMetaObject(int flags);
	virtual bool OnDeleteMetaObject();

	//module manager is started or exit 
	virtual bool OnBeforeRunMetaObject(int flags);
	virtual bool OnAfterCloseMetaObject();

#pragma region _form_builder_h_
	//support form 
	virtual IBackendValueForm* GetListForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid) = 0;
#pragma endregion 

	//special functions for DB 
	virtual wxString GetTableNameDB() const;

protected:

	//update current record
	bool UpdateCurrentRecords(const wxString& tableName, IValueMetaObjectRegisterData* dst);

	//get default form 
	virtual IBackendValueForm* GetFormByCommandType(EInterfaceCommandType cmdType = EInterfaceCommandType::EInterfaceCommandType_Default) {

		//if (cmdType == EInterfaceCommandType::EInterfaceCommandType_Create)
		//	return GetObjectRecord();
		//else if (cmdType == EInterfaceCommandType::EInterfaceCommandType_List)
		//	return GetListForm();

		return GetListForm();
	}

	///////////////////////////////////////////////////////////////////

	//get default attributes
	virtual bool FillArrayObjectByPredefined(std::vector<IValueMetaObjectAttribute*>& array) const {
		return false;
	}

	//get dimension keys 
	virtual bool FillArrayObjectByDimention(
		std::vector<IValueMetaObjectAttribute*>& array) const {
		FillArrayObjectByFilter<IValueMetaObjectAttribute>(array, { g_metaDimensionCLSID });
		return true;
	}

	///////////////////////////////////////////////////////////////////

	virtual IValueRecordSetObject* CreateRecordSetObjectRegValue(const CUniquePairKey& uniqueKey = wxNullUniquePairKey) = 0;
	virtual IValueRecordManagerObject* CreateRecordManagerObjectRegValue(const CUniquePairKey& uniqueKey = wxNullUniquePairKey) { return nullptr; }

	//create and update table 
	virtual bool CreateAndUpdateTableDB(IMetaDataConfiguration* srcMetaData, IValueMetaObject* srcMetaObject, int flags);

	//process default query
	int ProcessDimension(const wxString& tableName, const IValueMetaObjectAttribute* srcAttr, const IValueMetaObjectAttribute* dstAttr);
	int ProcessResource(const wxString& tableName, const IValueMetaObjectAttribute* srcAttr, const IValueMetaObjectAttribute* dstAttr);
	int ProcessAttribute(const wxString& tableName, const IValueMetaObjectAttribute* srcAttr, const IValueMetaObjectAttribute* dstAttr);

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());
	virtual bool DeleteData() { return true; }

protected:

	//create default attributes
	CPropertyInnerAttribute<>* m_propertyAttributeLineActive = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IValueMetaObjectCompositeData::CreateBoolean(wxT("active"), _("Active"), wxEmptyString, false, true));
	CPropertyInnerAttribute<>* m_propertyAttributePeriod = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IValueMetaObjectCompositeData::CreateDate(wxT("period"), _("Period"), wxEmptyString, eDateFractions::eDateFractions_DateTime, true));
	CPropertyInnerAttribute<>* m_propertyAttributeRecorder = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IValueMetaObjectCompositeData::CreateEmptyType(wxT("recorder"), _("Recorder"), wxEmptyString));
	CPropertyInnerAttribute<>* m_propertyAttributeLineNumber = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IValueMetaObjectCompositeData::CreateNumber(wxT("lineNumber"), _("Line number"), wxEmptyString, 15, 0));

private:

#pragma region role
	CRole* m_roleRead = IValueMetaObject::CreateRole(wxT("read"), _("Read"));
	CRole* m_roleWrite = IValueMetaObject::CreateRole(wxT("write"), _("Write"));
	CRole* m_roleDelete = IValueMetaObject::CreateRole(wxT("delete"), _("Delete"));
#pragma endregion
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

	//standart override 
	virtual bool IsEmpty() const = 0;

	//standart override 
	virtual bool IsModified() const { return false; }
	virtual void Modify(bool mod) {}

	//save source modify  
	virtual bool SaveModify() { return true; }

	//check is changes data in db
	virtual bool ModifiesData() { return false; }

	//get unique identifier 
	virtual CUniqueKey GetGuid() const = 0;

	//get metaData from object 
	virtual IValueMetaObjectGenericData* GetSourceMetaObject() const = 0;

	//support source data 
	virtual CSourceExplorer GetSourceExplorer() const = 0;
	virtual bool GetModel(IValueModel*& tableValue, const meta_identifier_t& id) = 0;

	//support source set/get data 
	virtual bool SetValueByMetaID(const meta_identifier_t& id, const CValue& varMetaVal) { return false; }
	virtual bool GetValueByMetaID(const meta_identifier_t& id, CValue& pvarMetaVal) const { return false; }

	//counter
	virtual void SourceIncrRef() = 0;
	virtual void SourceDecrRef() = 0;
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

// Manager with metaobject 
#pragma region managers
class BACKEND_API IValueManagerDataObject : public CValue {
public:

	IValueManagerDataObject() : CValue(eValueTypes::TYPE_VALUE, true) {}
	virtual ~IValueManagerDataObject() {}

	virtual CValueMetaObjectCommonModule* GetModuleManager() const = 0;
	virtual IValueMetaObject* GetMetaObject() const = 0;
};
#pragma endregion 

//Object with metaobject 
#pragma region objects 
class BACKEND_API IValueRecordDataObject : public CValue, public IActionDataObject,
	public ISourceDataObject, public IValueDataObject, public IModuleDataObject {
	wxDECLARE_ABSTRACT_CLASS(IValueRecordDataObject);
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
	IValueRecordDataObject(const CGuid& objGuid, bool newObject);
	IValueRecordDataObject(const IValueRecordDataObject& source);

	//standart override 
	virtual CMethodHelper* GetPMethods() const final { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}
public:
	virtual ~IValueRecordDataObject();

	//support actionData 
	virtual CActionCollection GetActionCollection(const form_identifier_t& formType) { return CActionCollection(); }
	virtual void ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm) {}

	virtual IValueRecordDataObject* CopyObjectValue() = 0;

	//standart override 
	virtual void PrepareNames() const;

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);

	virtual bool CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray);
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);

	//check is empty
	virtual bool IsEmpty() const { return CValue::IsEmpty(); }

	//get metaData from object 
	virtual IValueMetaObjectGenericData* GetSourceMetaObject() const final { return GetMetaObject(); }

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
	virtual IValueMetaObjectRecordData* GetMetaObject() const = 0;

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

	//Working with iterators
	virtual bool HasIterator() const { return true; }
	virtual CValue GetIteratorAt(unsigned int lPropNum) { CValue retValue; GetPropVal(lPropNum, retValue); return retValue; }
	virtual unsigned int GetIteratorCount() const { return m_methodHelper != nullptr ? m_methodHelper->GetNProps() : 0; }

protected:
	virtual void PrepareEmptyObject();
protected:
	friend class IMetaData;
	friend class IValueMetaObjectRecordData;
protected:
	CMethodHelper* m_methodHelper;
};

//Object with file
class BACKEND_API IValueRecordDataObjectExt : public IValueRecordDataObject {
	wxDECLARE_ABSTRACT_CLASS(IValueRecordDataObjectExt);
protected:
	//override copy constructor
	IValueRecordDataObjectExt(IValueMetaObjectRecordDataExt* metaObject);
	IValueRecordDataObjectExt(const IValueRecordDataObjectExt& source);
public:
	virtual ~IValueRecordDataObjectExt();

	bool InitializeObject();
	bool InitializeObject(IValueRecordDataObjectExt* source);

	//get unique identifier 
	virtual CUniqueKey GetGuid() const { return m_objGuid; }

	//check is empty
	virtual bool IsEmpty() const { return false; }

	//copy new object
	virtual IValueRecordDataObjectExt* CopyObjectValue();

	//get metaData from object 
	virtual IValueMetaObjectRecordDataExt* GetMetaObject() const { return m_metaObject; }

protected:
	IValueMetaObjectRecordDataExt* m_metaObject;
};

//Object with reference type 
class BACKEND_API IValueRecordDataObjectRef : public IValueRecordDataObject {
	wxDECLARE_ABSTRACT_CLASS(IValueRecordDataObjectRef);
protected:

	// code generator
	CValue GenerateNextIdentifier(
		IValueMetaObjectAttribute* attribute, const wxString& strPrefix);

protected:
	IValueRecordDataObjectRef(IValueMetaObjectRecordDataMutableRef* metaObject, const CGuid& objGuid);
	IValueRecordDataObjectRef(const IValueRecordDataObjectRef& src);
public:

	virtual ~IValueRecordDataObjectRef();

	bool InitializeObject(const CGuid& copyGuid = wxNullGuid);
	bool InitializeObject(IValueRecordDataObjectRef* source, bool generate = false);

	virtual bool WriteObject() = 0;
	virtual bool DeleteObject() = 0;

	//Get ref class 
	virtual class_identifier_t GetClassType() const;

	virtual wxString GetClassName() const;
	virtual wxString GetString() const;

	//is new object?
	virtual bool IsNewObject() const { return m_newObject; }

	//check is empty
	virtual bool IsEmpty() const {
		return !m_objGuid.isValid();
	}


	//is modified 
	virtual bool IsModified() const { return m_objModified; }

	//set modify 
	virtual void Modify(bool mod);

	//Get presentation 
	virtual wxString GetSourceCaption() const {
		return m_metaObject->GetSynonym() + wxT(": ") + (IsNewObject() ? _("Creating") : GetString());
	}

	//support source data 
	virtual CSourceExplorer GetSourceExplorer() const;
	virtual bool GetModel(IValueModel*& tableValue, const meta_identifier_t& id);

	//get metaData from object 
	virtual IValueMetaObjectRecordDataMutableRef* GetMetaObject() const { return m_metaObject; }

	//check is changes data in db
	virtual bool ModifiesData() { return true; }

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
	virtual IValueRecordDataObjectRef* CopyObjectValue();

	//get reference
	virtual class CValueReferenceDataObject* GetReference() const;

public:
	virtual void SetDeletionMark(bool deletionMark = true);
protected:

	virtual bool ReadData();
	virtual bool ReadData(const CGuid& srcGuid);
	virtual bool SaveData();
	virtual bool DeleteData();

	//code/number generator 
	virtual bool IsSetUniqueIdentifier() const;

	virtual bool GenerateUniqueIdentifier(const wxString& strPrefix);
	virtual bool ResetUniqueIdentifier();

protected:
	virtual void PrepareEmptyObject();
	virtual void PrepareEmptyObject(const IValueRecordDataObjectRef* source);
protected:

	friend class CValueTabularSectionDataObjectRef;

	bool m_objModified;
	IValueMetaObjectRecordDataMutableRef* m_metaObject;
	reference_t* m_reference_impl;
};

//Object with reference type and group/object type 
class BACKEND_API IValueRecordDataObjectFolderRef : public IValueRecordDataObjectRef {
	wxDECLARE_ABSTRACT_CLASS(IValueRecordDataObjectFolderRef);
protected:
	IValueRecordDataObjectFolderRef(IValueMetaObjectRecordDataHierarchyMutableRef* metaObject, const CGuid& objGuid, eObjectMode objMode = eObjectMode::OBJECT_ITEM);
	IValueRecordDataObjectFolderRef(const IValueRecordDataObjectFolderRef& src);
public:
	virtual ~IValueRecordDataObjectFolderRef();

	//support source data 
	virtual CSourceExplorer GetSourceExplorer() const;
	virtual bool GetModel(IValueModel*& tableValue, const meta_identifier_t& id);

	//Get presentation 
	virtual wxString GetSourceCaption() const {
		return m_metaObject->GetSynonym() + wxT(": ") + (IsNewObject() ? _("Creating") : GetString());
	}

	//get metaData from object 
	virtual IValueMetaObjectRecordDataHierarchyMutableRef* GetMetaObject() const {
		return (IValueMetaObjectRecordDataHierarchyMutableRef*)m_metaObject;
	};

	//copy new object
	virtual IValueRecordDataObjectRef* CopyObjectValue();

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
	virtual void PrepareEmptyObject(const IValueRecordDataObjectRef* source);
protected:
	//folder or object catalog
	eObjectMode m_objMode;
};
#pragma endregion

//Object with register type 
#pragma region registers 
class BACKEND_API CValueRecordKeyObject : public CValue {
	wxDECLARE_ABSTRACT_CLASS(CValueRecordKeyObject);
public:
	CValueRecordKeyObject(IValueMetaObjectRegisterData* metaObject);
	virtual ~CValueRecordKeyObject();

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
	virtual bool IsEmpty() const;

	//Get unique key 
	CUniquePairKey GetUniqueKey() { return CUniquePairKey(m_metaObject, m_keyValues); }

	//get metaData from object 
	virtual IValueMetaObjectRegisterData* GetMetaObject() const { return m_metaObject; };

	//Get ref class 
	virtual class_identifier_t GetClassType() const;

	virtual wxString GetClassName() const;
	virtual wxString GetString() const;

	//Working with iterators
	virtual bool HasIterator() const { return true; }
	virtual CValue GetIteratorAt(unsigned int lPropNum) { CValue retValue; GetPropVal(lPropNum, retValue); return retValue; }
	virtual unsigned int GetIteratorCount() const { return m_methodHelper != nullptr ? m_methodHelper->GetNProps() : 0; }

protected:
	IValueMetaObjectRegisterData* m_metaObject;
	valueArray_t m_keyValues;
	CMethodHelper* m_methodHelper;
};

class BACKEND_API IValueRecordSetObject : public IValueTable, public IModuleDataObject {
	wxDECLARE_ABSTRACT_CLASS(IValueRecordSetObject);
public:

	virtual IValueModelColumnCollection* GetColumnCollection() const { return m_recordColumnCollection; }
	virtual IValueModelReturnLine* GetRowAt(const wxDataViewItem& line) {
		if (!line.IsOk())
			return nullptr;
		return CValue::CreateAndPrepareValueRef<CValueRecordSetObjectRegisterReturnLine>(this, line);
	}

	class CValueRecordSetObjectRegisterColumnCollection : public IValueTable::IValueModelColumnCollection {
		wxDECLARE_DYNAMIC_CLASS(CValueRecordSetObjectRegisterColumnCollection);
	public:

		class CValueRecordSetRegisterColumnInfo : public IValueTable::IValueModelColumnCollection::IValueModelColumnInfo {
			wxDECLARE_DYNAMIC_CLASS(CValueRecordSetRegisterColumnInfo);
		public:

			CValueRecordSetRegisterColumnInfo();
			CValueRecordSetRegisterColumnInfo(IValueMetaObjectAttribute* attribute);
			virtual ~CValueRecordSetRegisterColumnInfo();

			virtual unsigned int GetColumnID() const { return m_metaAttribute->GetMetaID(); }
			virtual wxString GetColumnName() const { return m_metaAttribute->GetName(); }
			virtual wxString GetColumnCaption() const { return m_metaAttribute->GetSynonym(); }
			virtual const CTypeDescription GetColumnType() const { return m_metaAttribute->GetTypeDesc(); }

			friend CValueRecordSetObjectRegisterColumnCollection;

		private:
			IValueMetaObjectAttribute* m_metaAttribute;
		};

	public:

		CValueRecordSetObjectRegisterColumnCollection();
		CValueRecordSetObjectRegisterColumnCollection(IValueRecordSetObject* ownerTable);
		virtual ~CValueRecordSetObjectRegisterColumnCollection();

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
			IValueMetaObjectGenericData* metaTable = m_ownerTable->GetMetaObject();
			wxASSERT(metaTable);
			const auto& obj = metaTable->GetGenericAttributeArrayObject();
			return obj.size();
		}

		//array support 
		virtual bool SetAt(const CValue& varKeyValue, const CValue& varValue);
		virtual bool GetAt(const CValue& varKeyValue, CValue& pvarValue);

		friend class IValueRecordSetObject;

	protected:
		IValueRecordSetObject* m_ownerTable;
		std::map<meta_identifier_t, CValuePtr<CValueRecordSetRegisterColumnInfo>> m_listColumnInfo;
		CMethodHelper* m_methodHelper;
	};

	class CValueRecordSetObjectRegisterReturnLine : public IValueModelReturnLine {
		wxDECLARE_DYNAMIC_CLASS(CValueRecordSetObjectRegisterReturnLine);
	public:

		CValueRecordSetObjectRegisterReturnLine(IValueRecordSetObject* ownerTable = nullptr,
			const wxDataViewItem& line = wxDataViewItem(nullptr));
		virtual ~CValueRecordSetObjectRegisterReturnLine();

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

		friend class IValueRecordSetObject;
	private:
		IValueRecordSetObject* m_ownerTable;
		CMethodHelper* m_methodHelper;
	};

	class CValueRecordSetObjectRegisterKeyValue : public CValue {
		wxDECLARE_DYNAMIC_CLASS(CValueRecordSetObjectRegisterReturnLine);
	public:
		class CValueRecordSetObjectRegisterKeyDescriptionValue : public CValue {
			wxDECLARE_DYNAMIC_CLASS(CValueRecordSetObjectRegisterKeyDescriptionValue);
		public:

			CValueRecordSetObjectRegisterKeyDescriptionValue(IValueRecordSetObject* recordSet = nullptr, const meta_identifier_t& id = wxNOT_FOUND);
			virtual ~CValueRecordSetObjectRegisterKeyDescriptionValue();

			virtual bool IsEmpty() const { return false; }

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
			IValueRecordSetObject* m_recordSet;
		};
	public:

		CValueRecordSetObjectRegisterKeyValue(IValueRecordSetObject* recordSet = nullptr);
		virtual ~CValueRecordSetObjectRegisterKeyValue();

		virtual bool IsEmpty() const { return false; }

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
		IValueRecordSetObject* m_recordSet;
		CMethodHelper* m_methodHelper;
	};

protected:
	IValueRecordSetObject(IValueMetaObjectRegisterData* metaObject, const CUniquePairKey& uniqueKey);
	IValueRecordSetObject(const IValueRecordSetObject& source);
public:
	virtual ~IValueRecordSetObject();

	void CreateEmptyKey();
	bool InitializeObject(const IValueRecordSetObject* source = nullptr, bool newRecord = false);

	bool FindKeyValue(const meta_identifier_t& id) const { return m_keyValues.find(id) != m_keyValues.end(); }
	template <typename value>
	void SetKeyValue(const meta_identifier_t& id, value&& cValue) {
		const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(id);
		wxASSERT(attribute);
		m_keyValues.insert_or_assign(
			id, attribute != nullptr ? attribute->AdjustValue(cValue) : cValue
		);
	}

	const CValue& GetKeyValue(const meta_identifier_t& id) const { return m_keyValues.at(id); }
	void EraseKeyValue(const meta_identifier_t& id) { m_keyValues.erase(id); }

	//copy new object
	virtual IValueRecordSetObject* CopyRegisterValue();

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
	virtual bool IsEmpty() const { return !m_selected; }

	//set modify 
	virtual void Modify(bool mod) { m_objModified = mod; }

	//is modified 
	virtual bool IsModified() const { return m_objModified; }

	//check is changes data in db
	virtual bool ModifiesData() { return true; }

	//get metaData from object 
	virtual IValueMetaObjectRegisterData* GetMetaObject() const { return m_metaObject; }

#pragma region _tabular_data_
	//get metaData from object 
	virtual IValueMetaObjectCompositeData* GetSourceMetaObject() const { return GetMetaObject(); }

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

	//Working with iterators
	virtual bool HasIterator() const override { return true; }

	virtual CValue GetIteratorEmpty() override {
		return CValue::CreateAndPrepareValueRef<CValueRecordSetObjectRegisterReturnLine>(this, wxDataViewItem(nullptr));
	}

	virtual CValue GetIteratorAt(unsigned int idx) override {
		if (idx > (unsigned int)GetRowCount())
			return wxEmptyValue;
		return CValue::CreateAndPrepareValueRef<CValueRecordSetObjectRegisterReturnLine>(this, GetItem(idx));
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

	friend class IValueRecordManagerObject;

	bool m_objModified;
	bool m_selected;

	valueArray_t m_keyValues;

	IValueMetaObjectRegisterData* m_metaObject;

	CValuePtr<CValueRecordSetObjectRegisterColumnCollection> m_recordColumnCollection;
	CValuePtr<CValueRecordSetObjectRegisterKeyValue> m_recordSetKeyValue;

	CMethodHelper* m_methodHelper;
};

class BACKEND_API IValueRecordManagerObject : public CValue,
	public ISourceDataObject, public IActionDataObject {
	wxDECLARE_ABSTRACT_CLASS(IValueRecordManagerObject);
protected:
	IValueRecordManagerObject(IValueMetaObjectRegisterData* metaObject, const CUniquePairKey& uniqueKey);
	IValueRecordManagerObject(const IValueRecordManagerObject& source);
public:

	IValueRecordSetObject* GetRecordSet() const { return m_recordSet; }

	virtual ~IValueRecordManagerObject();

	virtual void CreateEmptyKey();

	bool InitializeObject(const IValueRecordManagerObject* source = nullptr, bool newRecord = false);
	bool InitializeObject(const CUniquePairKey& key);

	virtual bool IsNewObject() const { return !m_recordSet->m_selected; }

	//get metaData from object 
	virtual IValueMetaObjectGenericData* GetSourceMetaObject() const final { return GetMetaObject(); }

	//Get ref class 
	virtual class_identifier_t GetSourceClassType() const final { return GetClassType(); }

	//Get presentation 
	virtual wxString GetSourceCaption() const {
		return m_metaObject->GetSynonym() + wxT(": ") + (IsNewObject() ? _("Creating") : m_metaObject->GetSynonym());
	}

	//copy new object
	virtual IValueRecordManagerObject* CopyRegisterValue();

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

	//check is changes data in db
	virtual bool ModifiesData() { return true; }

	//default methods
	virtual bool WriteRegister(bool replace = true) = 0;
	virtual bool DeleteRegister() = 0;

	//check is empty
	virtual bool IsEmpty() const;

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
	virtual IValueMetaObjectRegisterData* GetMetaObject() const {
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

	//Working with iterators
	virtual bool HasIterator() const { return true; }
	virtual CValue GetIteratorAt(unsigned int lPropNum) { CValue retValue; GetPropVal(lPropNum, retValue); return retValue; }
	virtual unsigned int GetIteratorCount() const { return m_methodHelper != nullptr ? m_methodHelper->GetNProps() : 0; }

protected:

	virtual void PrepareEmptyObject(const IValueRecordManagerObject* source);

protected:

	virtual bool ExistData();
	virtual bool ReadData(const CUniquePairKey& key);
	virtual bool SaveData(bool replace = true);
	virtual bool DeleteData();

protected:

	CUniquePairKey m_objGuid;

	IValueMetaObjectRegisterData* m_metaObject;

	CValuePtr<IValueRecordSetObject> m_recordSet;
	CValuePtr<IValueTable::IValueModelReturnLine> m_recordLine;

	CMethodHelper* m_methodHelper;
};
#pragma endregion

#endif 