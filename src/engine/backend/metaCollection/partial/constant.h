#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#include "backend/metaCollection/partial/commonObject.h"

class BACKEND_API CRecordDataObjectConstant;

class BACKEND_API CMetaObjectConstant : 
	public CMetaObjectAttribute, public IBackendCommandItem {
	
	wxDECLARE_DYNAMIC_CLASS(CMetaObjectConstant);
protected:
	enum
	{
		ID_METATREE_OPEN_CONSTANT_MANAGER = 19000,
	};

public:

#pragma region access
	bool AccessRight_Read() const { return IsFullAccess() || AccessRight(m_roleRead); }
	bool AccessRight_Write() const { return IsFullAccess() || AccessRight(m_roleWrite); }
#pragma endregion

	CMetaObjectConstant();
	virtual ~CMetaObjectConstant();

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	virtual bool OnCreateMetaObject(IMetaData* metaData, int flags);
	virtual bool OnLoadMetaObject(IMetaData* metaData);
	virtual bool OnSaveMetaObject(int flags);
	virtual bool OnDeleteMetaObject();

	//module manager is started or exit 
	virtual bool OnBeforeRunMetaObject(int flags);
	virtual bool OnAfterRunMetaObject(int flags);

	virtual bool OnBeforeCloseMetaObject();
	virtual bool OnAfterCloseMetaObject();

	//get table name
	static wxString GetTableNameDB() { return wxT("sys_const"); }

	//get module object in compose object 
	virtual CMetaObjectModule* GetModuleObject() const { return m_propertyModule->GetMetaObject(); }

	//create empty object
	virtual CRecordDataObjectConstant* CreateRecordDataObjectValue();

	//support form 
	virtual IBackendValueForm* GetObjectForm();

	//create constant table  
	static bool CreateConstantSQLTable();
	static bool DeleteConstantSQLTable();

	//create and update table 
	virtual bool CreateAndUpdateTableDB(IMetaDataConfiguration* srcMetaData, IMetaObject* srcMetaObject, int flags);

	//get command section 
	virtual EInterfaceCommandSection GetCommandSection() const { return EInterfaceCommandSection::EInterfaceCommandSection_Create; }

	//process default query
	int ProcessAttribute(const wxString& tableName, IMetaObjectAttribute* srcAttr, IMetaObjectAttribute* dstAttr);

protected:

	//get default form 
	virtual IBackendValueForm* GetFormByCommandType(EInterfaceCommandType cmdType = EInterfaceCommandType::EInterfaceCommandType_Default) {

		if (cmdType == EInterfaceCommandType::EInterfaceCommandType_Create) {
			return GetObjectForm();
		}

		return GetObjectForm();
	}

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());
	virtual bool DeleteData();

	//prepare menu for item
	virtual bool PrepareContextMenu(wxMenu* defaultMenu);
	virtual void ProcessCommand(unsigned int id);

private:

	CPropertyInnerModule<CMetaObjectModule>* m_propertyModule = IPropertyObject::CreateProperty<CPropertyInnerModule<CMetaObjectModule>>(m_categorySecondary, IMetaObject::CreateMetaObjectAndSetParent<CMetaObjectModule>(wxT("recordModule"), _("Record module")));

#pragma region role 
	CRole* m_roleRead = IMetaObject::CreateRole(wxT("read"), _("Read"));
	CRole* m_roleWrite = IMetaObject::CreateRole(wxT("write"), _("Write"));
#pragma endregion

	friend class CRecordDataObjectConstant;
	friend class IMetaData;
};

#include "backend/moduleInfo.h"

class BACKEND_API CRecordDataObjectConstant : public CValue, public IActionDataObject,
	public ISourceDataObject, public IModuleDataObject {
	virtual bool InitializeObject(const CRecordDataObjectConstant* source = nullptr);
protected:
	enum helperAlias {
		eSystem,
		eProcUnit
	};
	enum helperProp {
		eValue
	};
protected:

	//override copy constructor
	CRecordDataObjectConstant(CMetaObjectConstant* metaObject);
	CRecordDataObjectConstant(const CRecordDataObjectConstant& source);

	//standart override 
	virtual CMethodHelper* GetPMethods() const final { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}

public:

	virtual ~CRecordDataObjectConstant();

	CValue GetConstValue() const;
	bool SetConstValue(const CValue& cValue);

	//standart override 
	virtual void PrepareNames() const;

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);

	//override default type object 
	virtual bool IsNewObject() const { return false; }

	//is modified 
	virtual bool IsModified() const { return m_objModified; }

	//set modify 
	virtual void Modify(bool mod);

	//check is empty
	inline virtual bool IsEmpty() const { return false; }

	//check is changes data in db
	virtual bool ModifiesData() { return true; }

	//get metaData from object 
	virtual IMetaObjectGenericData* GetSourceMetaObject() const final { return GetMetaObject(); }

	//Get ref class 
	virtual class_identifier_t GetSourceClassType() const final { return GetClassType(); };

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

	//counter
	virtual void SourceIncrRef() { CValue::IncrRef(); }
	virtual void SourceDecrRef() { CValue::DecrRef(); }

	//get metaData from object 
	virtual IMetaObjectGenericData* GetMetaObject() const {
		return (IMetaObjectGenericData*)m_metaObject;
	};

	//get unique identifier 
	virtual CUniqueKey GetGuid() const { return m_metaObject->GetGuid(); }
	virtual bool SaveModify() override { return SetConstValue(m_constValue); }

	//get frame
	virtual IBackendValueForm* GetForm() const;

#pragma region _form_builder_h_
	//support show 
	virtual void ShowFormValue();
	virtual IBackendValueForm* GetFormValue();
#pragma endregion

	//support actionData
	virtual CActionCollection GetActionCollection(const form_identifier_t& formType);
	virtual void ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm);

	//default showing
	virtual void ShowValue() override { ShowFormValue(); }

	//Get ref class 
	virtual class_identifier_t GetClassType() const;
	virtual wxString GetClassName() const;
	virtual wxString GetString() const;

protected:

	bool m_objModified;

	CMethodHelper* m_methodHelper;
	CMetaObjectConstant* m_metaObject;
	CValue m_constValue;

	friend class CValue;
};

#endif