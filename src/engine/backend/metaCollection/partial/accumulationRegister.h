#ifndef __ACCUMULATION_REGISTER_H__
#define __ACCUMULATION_REGISTER_H__

#include "commonObject.h"
#include "accumulationRegisterEnum.h"

class CMetaObjectAccumulationRegister : public IMetaObjectRegisterData {
	wxDECLARE_DYNAMIC_CLASS(CMetaObjectAccumulationRegister);
private:
	enum
	{
		eFormList = 2,
	};

	virtual CFormTypeList GetFormType() const override {
		CFormTypeList formList;
		formList.AppendItem(wxT("formList"), _("Form list"), eFormList);
		return formList;
	}

	enum
	{
		ID_METATREE_OPEN_MODULE = 19000,
		ID_METATREE_OPEN_MANAGER = 19001,
	};

//private:
	//CMetaObjectAttributeDefault* m_attributeRecordType = IMetaObjectSourceData::CreateSpecialType(wxT("recordType"), _("Record type"), wxEmptyString, g_enumRecordTypeCLSID, false, CValueEnumAccumulationRegisterRecordType::CreateDefEnumValue());
protected:

	CPropertyInnerModule<CMetaObjectModule>* m_propertyModuleObject = IPropertyObject::CreateProperty<CPropertyInnerModule<CMetaObjectModule>>(m_categorySecondary, IMetaObjectSourceData::CreateMetaObjectAndSetParent<CMetaObjectModule>(wxT("recordSetModule"), _("record set module")));
	CPropertyInnerModule<CMetaObjectManagerModule>* m_propertyModuleManager = IPropertyObject::CreateProperty<CPropertyInnerModule<CMetaObjectManagerModule>>(m_categorySecondary, IMetaObjectSourceData::CreateMetaObjectAndSetParent<CMetaObjectManagerModule>(wxT("managerModule"), _("manager module")));

	CPropertyCategory* m_categoryForm = IPropertyObject::CreatePropertyCategory(wxT("defaultForms"), _("default forms"));
	CPropertyList* m_propertyDefFormList = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormList"), _("default form list"), &CMetaObjectAccumulationRegister::GetFormList, wxNOT_FOUND);
	CPropertyCategory* m_categoryData = IPropertyObject::CreatePropertyCategory(wxT("data"), _("data"));
	CPropertyEnum<CValueEnumAccumulationRegisterType>* m_propertyRegisterType = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumAccumulationRegisterType>>(m_categoryData, wxT("register_type"), _("register type"), eRegisterType::eBalances);

	CPropertyInnerAttribute<>* m_propertyAttributeRecordType = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IMetaObjectSourceData::CreateSpecialType(wxT("recordType"), _("Record type"), wxEmptyString, g_enumRecordTypeCLSID, false, CValueEnumAccumulationRegisterRecordType::CreateDefEnumValue()));

private:
	bool GetFormList(CPropertyList* prop);
public:

	CMetaObjectAttributeDefault* GetRegisterRecordType() const {
		return m_propertyAttributeRecordType->GetMetaObject();
	}

	bool IsRegisterRecordType(const meta_identifier_t& id) const {
		return id == (*m_propertyAttributeRecordType)->GetMetaID();
	}

	///////////////////////////////////////////////////////////////////

	eRegisterType GetRegisterType() const {
		return m_propertyRegisterType->GetValueAsEnum();
	}

	wxString GetRegisterTableNameDB(eRegisterType rType) const {
		wxString className = GetClassName();
		wxASSERT(m_metaId != 0);

		if (rType == eRegisterType::eBalances) {
			return wxString::Format("%s%i_T",
				className, GetMetaID());
		}

		return wxString::Format("%s%i_Tn",
			className, GetMetaID());
	}

	wxString GetRegisterTableNameDB() const {
		return GetRegisterTableNameDB(GetRegisterType());
	}

	///////////////////////////////////////////////////////////////////

	bool CreateAndUpdateBalancesTableDB(IMetaDataConfiguration* srcMetaData, IMetaObject* srcMetaObject, int flags);
	bool CreateAndUpdateTurnoverTableDB(IMetaDataConfiguration* srcMetaData, IMetaObject* srcMetaObject, int flags);

	///////////////////////////////////////////////////////////////////

	CMetaObjectAccumulationRegister();
	virtual ~CMetaObjectAccumulationRegister();

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

	//get default attributes
	virtual std::vector<IMetaObjectAttribute*> GetDefaultAttributes() const;

	//get dimension keys 
	virtual std::vector<IMetaObjectAttribute*> GetGenericDimensions() const;

	//has record manager 
	virtual bool HasRecordManager() const {
		return false;
	}

	//has recorder and period 
	virtual bool HasPeriod() const {
		return true;
	}

	virtual bool HasRecorder() const {
		return true;
	}

	//get module object in compose object 
	virtual CMetaObjectModule* GetModuleObject() const { return m_propertyModuleObject->GetMetaObject(); }
	virtual CMetaObjectCommonModule* GetModuleManager() const { return m_propertyModuleManager->GetMetaObject(); }

	//create associate value 
	virtual CMetaObjectForm* GetDefaultFormByID(const form_identifier_t& id);

#pragma region _form_builder_h_
	//support form 
	virtual IBackendValueForm* GetListForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullUniqueKey);
#pragma endregion

	//prepare menu for item
	virtual bool PrepareContextMenu(wxMenu* defaultMenu);
	virtual void ProcessCommand(unsigned int id);

	//create and update table 
	virtual bool CreateAndUpdateTableDB(IMetaDataConfiguration* srcMetaData, IMetaObject* srcMetaObject, int flags);

	/**
	* Property events
	*/
	virtual void OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue);

protected:

	//create object data with meta form
	virtual ISourceDataObject* CreateSourceObject(IMetaObjectForm* metaObject);

	//create record set
	virtual IRecordSetObject* CreateRecordSetObjectRegValue(const CUniquePairKey& uniqueKey = wxNullUniquePairKey);

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

protected:
	friend class IMetaData;
};

//********************************************************************************************
//*                                      Object                                              *
//********************************************************************************************

class CRecordSetObjectAccumulationRegister : public IRecordSetObject {
public:
	CRecordSetObjectAccumulationRegister(CMetaObjectAccumulationRegister* metaObject, const CUniquePairKey& uniqueKey = wxNullUniquePairKey) :
		IRecordSetObject(metaObject, uniqueKey)
	{
	}

	CRecordSetObjectAccumulationRegister(const CRecordSetObjectAccumulationRegister& source) :
		IRecordSetObject(source)
	{
	}

	virtual bool WriteRecordSet(bool replace = true, bool clearTable = true);
	virtual bool DeleteRecordSet();

	//////////////////////////////////////////////////////////////////////////////

	virtual bool SaveVirtualTable();
	virtual bool DeleteVirtualTable();

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
};

#endif 