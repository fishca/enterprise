#ifndef __INFORMATION_REGISTER_H__
#define __INFORMATION_REGISTER_H__

#include "commonObject.h"
#include "informationRegisterEnum.h"

class CValueMetaObjectInformationRegister : public IValueMetaObjectRegisterData {
	wxDECLARE_DYNAMIC_CLASS(CValueMetaObjectInformationRegister);
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
	class CValueMetaObjectRecordManager : public IValueMetaObject {
		wxDECLARE_DYNAMIC_CLASS(CValueMetaObjectRecordManager);
	public:
		CValueMetaObjectRecordManager() : IValueMetaObject() {}
	};

public:

	CValueMetaObjectInformationRegister();
	virtual ~CValueMetaObjectInformationRegister();

	eWriteRegisterMode GetWriteRegisterMode() const {
		return m_propertyWriteMode->GetValueAsEnum();
	}

	ePeriodicity GetPeriodicity() const {
		return m_propertyPeriodicity->GetValueAsEnum();
	}

	bool CreateAndUpdateSliceFirstTableDB(IMetaDataConfiguration* srcMetaData, IValueMetaObject* srcMetaObject, int flags);
	bool CreateAndUpdateSliceLastTableDB(IMetaDataConfiguration* srcMetaData, IValueMetaObject* srcMetaObject, int flags);

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

	//has record manager 
	virtual bool HasRecordManager() const { return GetWriteRegisterMode() == eWriteRegisterMode::eIndependent; }

	//has recorder and period 
	virtual bool HasPeriod() const { return GetPeriodicity() != ePeriodicity::eNonPeriodic; }
	virtual bool HasRecorder() const { return GetWriteRegisterMode() == eWriteRegisterMode::eSubordinateRecorder; }

	//get module object in compose object 
	virtual CValueMetaObjectModule* GetModuleObject() const { return m_propertyModuleObject->GetMetaObject(); }
	virtual CValueMetaObjectCommonModule* GetModuleManager() const { return m_propertyModuleManager->GetMetaObject(); }

	//create associate value 
	virtual IValueMetaObjectForm* GetDefaultFormByID(const form_identifier_t& id) const;

#pragma region _form_builder_h_
	//support form 
	virtual IBackendValueForm* GetRecordForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullUniqueKey);
	virtual IBackendValueForm* GetListForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullUniqueKey);
#pragma endregion

	//prepare menu for item
	virtual bool PrepareContextMenu(wxMenu* defaultMenu);
	virtual void ProcessCommand(unsigned int id);

	//create and update table 
	virtual bool CreateAndUpdateTableDB(IMetaDataConfiguration* srcMetaData, IValueMetaObject* srcMetaObject, int flags);

	/**
	* Property events
	*/
	virtual void OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue);

protected:

	//get default attributes
	virtual bool FillArrayObjectByPredefined(std::vector<IValueMetaObjectAttribute*>& array) const {

		if (GetWriteRegisterMode() == eWriteRegisterMode::eSubordinateRecorder) {
			array.emplace_back(m_propertyAttributeLineActive->GetMetaObject());
		}

		if (GetPeriodicity() != ePeriodicity::eNonPeriodic ||
			GetWriteRegisterMode() == eWriteRegisterMode::eSubordinateRecorder) {
			array.emplace_back(m_propertyAttributePeriod->GetMetaObject());
		}

		if (GetWriteRegisterMode() == eWriteRegisterMode::eSubordinateRecorder) {
			array.emplace_back(m_propertyAttributeRecorder->GetMetaObject());
			array.emplace_back(m_propertyAttributeLineNumber->GetMetaObject());
		}

		return true;
	}

	//get dimension keys 
	virtual bool FillArrayObjectByDimention(
		std::vector<IValueMetaObjectAttribute*>& array) const {

		if (GetWriteRegisterMode() != eWriteRegisterMode::eSubordinateRecorder) {

			if (GetPeriodicity() != ePeriodicity::eNonPeriodic) {
				array.emplace_back(m_propertyAttributePeriod->GetMetaObject());
			}

			FillArrayObjectByFilter<IValueMetaObjectAttribute>(array, { g_metaDimensionCLSID });
		}
		else {
			array = { m_propertyAttributeRecorder->GetMetaObject() };
		}
		return true;
	}

	//create manager
	virtual IValueManagerDataObject* CreateManagerDataObjectValue();

	//create record set
	virtual IValueRecordSetObject* CreateRecordSetObjectRegValue(const CUniquePairKey& uniqueKey = wxNullUniquePairKey);
	virtual IValueRecordManagerObject* CreateRecordManagerObjectRegValue(const CUniquePairKey& uniqueKey = wxNullUniquePairKey);

	//create object data with meta form
	virtual ISourceDataObject* CreateSourceObject(IValueMetaObjectForm* metaObject);

	//get command section 
	virtual EInterfaceCommandSection GetCommandSection() const { return EInterfaceCommandSection::EInterfaceCommandSection_Combined; }

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

protected:

	//get default form 
	virtual IBackendValueForm* GetFormByCommandType(EInterfaceCommandType cmdType = EInterfaceCommandType::EInterfaceCommandType_Default) {

		if (cmdType == EInterfaceCommandType::EInterfaceCommandType_Create)
			return GetRecordForm();
		else if (cmdType == EInterfaceCommandType::EInterfaceCommandType_List)
			return GetListForm();

		return GetListForm();
	}

private:

	bool FillFormRecord(CPropertyList* prop) {
		for (auto object : GetFormArrayObject()) {
			if (!object->IsAllowed()) continue;
			if (eFormRecord == object->GetTypeForm()) {
				prop->AppendItem(
					object->GetName(),
					object->GetMetaID(),
					object->GetIcon(),
					object);
			}
		}
		return true;
	}

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

	CValueMetaObjectRecordManager* m_metaRecordManager;

	CPropertyInnerModule<CValueMetaObjectModule>* m_propertyModuleObject = IPropertyObject::CreateProperty<CPropertyInnerModule<CValueMetaObjectModule>>(m_categorySecondary, IValueMetaObjectCompositeData::CreateMetaObjectAndSetParent<CValueMetaObjectModule>(wxT("recordSetModule"), _("Record set module")));
	CPropertyInnerModule<CValueMetaObjectManagerModule>* m_propertyModuleManager = IPropertyObject::CreateProperty<CPropertyInnerModule<CValueMetaObjectManagerModule>>(m_categorySecondary, IValueMetaObjectCompositeData::CreateMetaObjectAndSetParent<CValueMetaObjectManagerModule>(wxT("managerModule"), _("Manager module")));

	CPropertyCategory* m_categoryForm = IPropertyObject::CreatePropertyCategory(wxT("presetValues"), _("Preset values"));
	CPropertyList* m_propertyDefFormRecord = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormRecord"), _("Default Record Form"), &CValueMetaObjectInformationRegister::FillFormRecord);
	CPropertyList* m_propertyDefFormList = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormList"), _("Default List Form"), &CValueMetaObjectInformationRegister::FillFormList);

	CPropertyCategory* m_categoryData = IPropertyObject::CreatePropertyCategory(wxT("data"), _("data"));
	CPropertyEnum<CValueEnumPeriodicity>* m_propertyPeriodicity = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumPeriodicity>>(m_categoryData, wxT("periodicity"), _("Periodicity"), ePeriodicity::eNonPeriodic);
	CPropertyEnum<CValueEnumWriteRegisterMode>* m_propertyWriteMode = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumWriteRegisterMode>>(m_categoryData, wxT("writeMode"), _("Write mode"), eWriteRegisterMode::eIndependent);

	friend class CValueRecordSetObjectInformationRegister;
	friend class CValueRecordManagerObjectInformationRegister;

	friend class IMetaData;
};

//********************************************************************************************
//*                                      Object                                              *
//********************************************************************************************

class CValueRecordSetObjectInformationRegister : public IValueRecordSetObject {
	CValueRecordSetObjectInformationRegister(CValueMetaObjectInformationRegister* metaObject, const CUniquePairKey& uniqueKey = wxNullUniquePairKey) :
		IValueRecordSetObject(metaObject, uniqueKey) {
	}
	CValueRecordSetObjectInformationRegister(const CValueRecordSetObjectInformationRegister& source) :
		IValueRecordSetObject(source) {
	}
public:

	//default methods
	virtual IValueRecordSetObject* CopyRegisterValue() {
		return new CValueRecordSetObjectInformationRegister(*this);
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
	friend class CValue;
	friend class CValueMetaObjectInformationRegister;
};

class CValueRecordManagerObjectInformationRegister : public IValueRecordManagerObject {
public:
	CValueRecordManagerObjectInformationRegister(CValueMetaObjectInformationRegister* metaObject, const CUniquePairKey& uniqueKey = wxNullUniquePairKey) :
		IValueRecordManagerObject(metaObject, uniqueKey)
	{
	}
	CValueRecordManagerObjectInformationRegister(const CValueRecordManagerObjectInformationRegister& source) :
		IValueRecordManagerObject(source)
	{
	}
	virtual IValueRecordManagerObject* CopyRegister(bool showValue = false) {
		IValueRecordManagerObject* objectRef = CopyRegisterValue();
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
	friend class CValue;
	friend class CValueMetaObjectInformationRegister;
};

#endif 