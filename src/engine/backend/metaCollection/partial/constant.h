#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#include "backend/metaCollection/partial/commonObject.h"

class BACKEND_API CValueRecordDataObjectConstant;

class BACKEND_API CValueMetaObjectConstant : 
	public CValueMetaObjectAttribute, public IBackendCommandItem {
	
	wxDECLARE_DYNAMIC_CLASS(CValueMetaObjectConstant);
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

	CValueMetaObjectConstant();
	virtual ~CValueMetaObjectConstant();

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
	virtual CValueMetaObjectModule* GetModuleObject() const { return m_propertyModule->GetMetaObject(); }

	//create empty object
	virtual CValueRecordDataObjectConstant* CreateRecordDataObjectValue();

	//support form 
	virtual IBackendValueForm* GetObjectForm();

	//create constant table  
	static bool CreateConstantSQLTable();
	static bool DeleteConstantSQLTable();

	//create and update table 
	virtual bool CreateAndUpdateTableDB(IMetaDataConfiguration* srcMetaData, IValueMetaObject* srcMetaObject, int flags);

	//get command section 
	virtual EInterfaceCommandSection GetCommandSection() const { return EInterfaceCommandSection::EInterfaceCommandSection_Create; }

	//process default query
	int ProcessAttribute(const wxString& tableName, IValueMetaObjectAttribute* srcAttr, IValueMetaObjectAttribute* dstAttr);

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

	CPropertyInnerModule<CValueMetaObjectModule>* m_propertyModule = IPropertyObject::CreateProperty<CPropertyInnerModule<CValueMetaObjectModule>>(m_categorySecondary, IValueMetaObject::CreateMetaObjectAndSetParent<CValueMetaObjectModule>(wxT("RecordModule"), _("Record module")));

#pragma region role 
	CRole* m_roleRead = IValueMetaObject::CreateRole(wxT("Read"), _("Read"));
	CRole* m_roleWrite = IValueMetaObject::CreateRole(wxT("Write"), _("Write"));
#pragma endregion

	friend class CValueRecordDataObjectConstant;
	friend class IMetaData;
};

#include "backend/moduleInfo.h"

class BACKEND_API CValueRecordDataObjectConstant : public CValue, public IActionDataObject,
	public ISourceDataObject, public IModuleDataObject {
	virtual bool InitializeObject(const CValueRecordDataObjectConstant* source = nullptr);
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
	CValueRecordDataObjectConstant(CValueMetaObjectConstant* metaObject);
	CValueRecordDataObjectConstant(const CValueRecordDataObjectConstant& source);

	//standart override 
	virtual CMethodHelper* GetPMethods() const final { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}

public:

	virtual ~CValueRecordDataObjectConstant();

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
	virtual IValueMetaObjectGenericData* GetSourceMetaObject() const final { return GetMetaObject(); }

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
	virtual IValueMetaObjectGenericData* GetMetaObject() const {
		return (IValueMetaObjectGenericData*)m_metaObject;
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
	CValueMetaObjectConstant* m_metaObject;
	CValue m_constValue;

	friend class CValue;
};

#endif