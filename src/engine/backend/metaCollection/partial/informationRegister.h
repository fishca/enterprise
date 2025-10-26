#ifndef __INFORMATION_REGISTER_H__
#define __INFORMATION_REGISTER_H__

#include "commonObject.h"
#include "informationRegisterEnum.h"

class CMetaObjectInformationRegister : public IMetaObjectRegisterData {
	wxDECLARE_DYNAMIC_CLASS(CMetaObjectInformationRegister);
private:
	enum
	{
		eFormRecord = 1,
		eFormList = 2,
	};

	virtual CFormTypeList GetFormType() const override {
		CFormTypeList formList;
		formList.AppendItem(wxT("formRecord"), _("Form record"), eFormRecord);
		formList.AppendItem(wxT("formList"), _("Form list"), eFormList);
		return formList;
	}

	enum
	{
		ID_METATREE_OPEN_MODULE = 19000,
		ID_METATREE_OPEN_MANAGER = 19001,
	};

public:
	class CMetaObjectRecordManager : public IMetaObject {
		wxDECLARE_DYNAMIC_CLASS(CMetaObjectRecordManager);
	public:
		CMetaObjectRecordManager() : IMetaObject() {}
	};
private:
	
	CMetaObjectRecordManager* m_metaRecordManager;

protected:

	CPropertyInnerModule<CMetaObjectModule>* m_propertyModuleObject = IPropertyObject::CreateProperty<CPropertyInnerModule<CMetaObjectModule>>(m_categorySecondary, IMetaObjectSourceData::CreateMetaObjectAndSetParent<CMetaObjectModule>(wxT("recordSetModule"), _("record set module")));
	CPropertyInnerModule<CMetaObjectManagerModule>* m_propertyModuleManager = IPropertyObject::CreateProperty<CPropertyInnerModule<CMetaObjectManagerModule>>(m_categorySecondary, IMetaObjectSourceData::CreateMetaObjectAndSetParent<CMetaObjectManagerModule>(wxT("managerModule"), _("manager module")));

	CPropertyCategory* m_categoryForm = IPropertyObject::CreatePropertyCategory(wxT("defaultForms"), _("default forms"));
	CPropertyList* m_propertyDefFormRecord = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormRecord"), _("default record"), &CMetaObjectInformationRegister::GetFormRecord, wxNOT_FOUND);
	CPropertyList* m_propertyDefFormList = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormList"), _("default list"), &CMetaObjectInformationRegister::GetFormList, wxNOT_FOUND);

	CPropertyCategory* m_categoryData = IPropertyObject::CreatePropertyCategory(wxT("data"), _("data"));
	CPropertyEnum<CValueEnumPeriodicity>* m_propertyPeriodicity = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumPeriodicity>>(m_categoryData, wxT("periodicity"), _("periodicity"), ePeriodicity::eNonPeriodic);
	CPropertyEnum<CValueEnumWriteRegisterMode>* m_propertyWriteMode = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumWriteRegisterMode>>(m_categoryData, wxT("writeMode"), _("write mode"), eWriteRegisterMode::eIndependent);

private:
	bool GetFormRecord(CPropertyList* prop);
	bool GetFormList(CPropertyList* prop);
public:

	CMetaObjectInformationRegister();
	virtual ~CMetaObjectInformationRegister();

	eWriteRegisterMode GetWriteRegisterMode() const {
		return m_propertyWriteMode->GetValueAsEnum();
	}

	ePeriodicity GetPeriodicity() const {
		return m_propertyPeriodicity->GetValueAsEnum();
	}

	bool CreateAndUpdateSliceFirstTableDB(IMetaDataConfiguration* srcMetaData, IMetaObject* srcMetaObject, int flags);
	bool CreateAndUpdateSliceLastTableDB(IMetaDataConfiguration* srcMetaData, IMetaObject* srcMetaObject, int flags);

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
	virtual bool HasRecordManager() const { return GetWriteRegisterMode() == eWriteRegisterMode::eIndependent; }

	//has recorder and period 
	virtual bool HasPeriod() const { return GetPeriodicity() != ePeriodicity::eNonPeriodic; }
	virtual bool HasRecorder() const { return GetWriteRegisterMode() == eWriteRegisterMode::eSubordinateRecorder; }

	//get module object in compose object 
	virtual CMetaObjectModule* GetModuleObject() const { return m_propertyModuleObject->GetMetaObject(); }
	virtual CMetaObjectCommonModule* GetModuleManager() const { return m_propertyModuleManager->GetMetaObject(); }

	//create associate value 
	virtual CMetaObjectForm* GetDefaultFormByID(const form_identifier_t& id);

#pragma region _form_builder_h_
	//support form 
	virtual IBackendValueForm* GetRecordForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniquePairKey& formGuid = wxNullUniquePairKey);
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
	virtual IRecordManagerObject* CreateRecordManagerObjectRegValue(const CUniquePairKey& uniqueKey = wxNullUniquePairKey);

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

protected:

	//support form 
	virtual IBackendValueForm* GetRecordForm(const meta_identifier_t& id, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid);

protected:
	friend class IMetaData;
	friend class CRecordSetObjectInformationRegister;
	friend class CRecordManagerObjectInformationRegister;
};

//********************************************************************************************
//*                                      Object                                              *
//********************************************************************************************

class CRecordSetObjectInformationRegister : public IRecordSetObject {
	CRecordSetObjectInformationRegister(CMetaObjectInformationRegister* metaObject, const CUniquePairKey& uniqueKey = wxNullUniquePairKey) :
		IRecordSetObject(metaObject, uniqueKey) {
	}
	CRecordSetObjectInformationRegister(const CRecordSetObjectInformationRegister& source) :
		IRecordSetObject(source) {
	}
public:

	//default methods
	virtual IRecordSetObject* CopyRegisterValue() {
		return new CRecordSetObjectInformationRegister(*this);
	}

	virtual bool WriteRecordSet(bool replace = true, bool clearTable = true);
	virtual bool DeleteRecordSet();

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

protected:
	friend class IMetaData;
	friend class CMetaObjectInformationRegister;
};

class CRecordManagerObjectInformationRegister : public IRecordManagerObject {
public:
	CRecordManagerObjectInformationRegister(CMetaObjectInformationRegister* metaObject, const CUniquePairKey& uniqueKey = wxNullUniquePairKey) :
		IRecordManagerObject(metaObject, uniqueKey)
	{
	}
	CRecordManagerObjectInformationRegister(const CRecordManagerObjectInformationRegister& source) :
		IRecordManagerObject(source)
	{
	}
	virtual IRecordManagerObject* CopyRegister(bool showValue = false) {
		IRecordManagerObject* objectRef = CopyRegisterValue();
		if (objectRef != nullptr && showValue)
			objectRef->ShowFormValue();
		return objectRef;
	}
	virtual bool WriteRegister(bool replace = true);
	virtual bool DeleteRegister();

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
	virtual void ShowFormValue(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr);
	virtual IBackendValueForm* GetFormValue(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr);
#pragma endregion

	//support actionData
	virtual CActionCollection GetActionCollection(const form_identifier_t& formType);
	virtual void ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm);

protected:
	friend class IMetaData;
	friend class CMetaObjectInformationRegister;
};

#endif 