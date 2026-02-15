#ifndef __ACCUMULATION_REGISTER_H__
#define __ACCUMULATION_REGISTER_H__

#include "commonObject.h"
#include "accumulationRegisterEnum.h"

class CValueMetaObjectAccumulationRegister : public IValueMetaObjectRegisterData {
	wxDECLARE_DYNAMIC_CLASS(CValueMetaObjectAccumulationRegister);
private:
	enum
	{
		eFormList = 2,
	};

	virtual CFormTypeList GetFormType() const override {
		CFormTypeList formList;
		formList.AppendItem(wxT("FormList"), _("Form list"), eFormList);
		return formList;
	}

	enum
	{
		ID_METATREE_OPEN_MODULE = 19000,
		ID_METATREE_OPEN_MANAGER = 19001,
	};

	//private:
		//CValueMetaObjectAttributePredefined* m_attributeRecordType = IValueMetaObjectCompositeData::CreateSpecialType(wxT("recordType"), _("Record type"), wxEmptyString, g_enumRecordTypeCLSID, false, CValueEnumAccumulationRegisterRecordType::CreateDefEnumValue());

public:

	CValueMetaObjectAttributePredefined* GetRegisterRecordType() const {
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

	bool CreateAndUpdateBalancesTableDB(IMetaDataConfiguration* srcMetaData, IValueMetaObject* srcMetaObject, int flags);
	bool CreateAndUpdateTurnoverTableDB(IMetaDataConfiguration* srcMetaData, IValueMetaObject* srcMetaObject, int flags);

	///////////////////////////////////////////////////////////////////

	CValueMetaObjectAccumulationRegister();
	virtual ~CValueMetaObjectAccumulationRegister();

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
	virtual bool HasRecordManager() const { return false; }

	//has recorder and period 
	virtual bool HasPeriod() const { return true; }
	virtual bool HasRecorder() const { return true; }

	//get module object in compose object 
	virtual CValueMetaObjectModule* GetModuleObject() const { return m_propertyModuleObject->GetMetaObject(); }
	virtual CValueMetaObjectCommonModule* GetModuleManager() const { return m_propertyModuleManager->GetMetaObject(); }

	//create associate value 
	virtual IValueMetaObjectForm* GetDefaultFormByID(const form_identifier_t& id) const;

#pragma region _form_builder_h_
	//support form 
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

		if (GetRegisterType() == eRegisterType::eBalances) {
			array = {
				m_propertyAttributeLineActive->GetMetaObject(),
				m_propertyAttributePeriod->GetMetaObject(),
				m_propertyAttributeRecordType->GetMetaObject(),
				m_propertyAttributeRecorder->GetMetaObject(),
				m_propertyAttributeLineNumber->GetMetaObject()
			};
		}
		else {
			array = { m_propertyAttributeLineActive->GetMetaObject(),
				m_propertyAttributePeriod->GetMetaObject(),
				m_propertyAttributeRecorder->GetMetaObject(),
				m_propertyAttributeLineNumber->GetMetaObject()
			};
		}

		return true;
	}

	//get dimension keys 
	virtual bool FillArrayObjectByDimention(
		std::vector<IValueMetaObjectAttribute*>& array) const {
		array = { m_propertyAttributeRecorder->GetMetaObject() };
		return true;
	}

	//create manager
	virtual IValueManagerDataObject* CreateManagerDataObjectValue();

	//create record set
	virtual IValueRecordSetObject* CreateRecordSetObjectRegValue(const CUniquePairKey& uniqueKey = wxNullUniquePairKey);

	//create object data with meta form
	virtual ISourceDataObject* CreateSourceObject(IValueMetaObjectForm* metaObject);

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

private:

	bool FillFormList(CPropertyList* prop) {
		for (auto object : GetFormArrayObject()) {
			if (!object->IsAllowed()) continue;
			if (eFormList == object->GetTypeForm()) {
				prop->AppendItem(
					object->GetName(),
					object->GetMetaID(),
					object->GetIcon(),
					object
				);
			}
		}

		return true;
	}

	CPropertyInnerModule<CValueMetaObjectModule>* m_propertyModuleObject = IPropertyObject::CreateProperty<CPropertyInnerModule<CValueMetaObjectModule>>(m_categorySecondary, IValueMetaObjectCompositeData::CreateMetaObjectAndSetParent<CValueMetaObjectModule>(wxT("RecordSetModule"), _("Record set module")));
	CPropertyInnerModule<CValueMetaObjectManagerModule>* m_propertyModuleManager = IPropertyObject::CreateProperty<CPropertyInnerModule<CValueMetaObjectManagerModule>>(m_categorySecondary, IValueMetaObjectCompositeData::CreateMetaObjectAndSetParent<CValueMetaObjectManagerModule>(wxT("ManagerModule"), _("Manager module")));

	CPropertyCategory* m_categoryForm = IPropertyObject::CreatePropertyCategory(wxT("PresetValues"), _("Preset values"));
	CPropertyList* m_propertyDefFormList = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("DefaultFormList"), _("Default List Form"), &CValueMetaObjectAccumulationRegister::FillFormList);
	CPropertyCategory* m_categoryData = IPropertyObject::CreatePropertyCategory(wxT("Data"), _("Data"));
	CPropertyEnum<CValueEnumAccumulationRegisterType>* m_propertyRegisterType = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumAccumulationRegisterType>>(m_categoryData, wxT("RegisterType"), _("Register type"), eRegisterType::eBalances);

	CPropertyInnerAttribute<>* m_propertyAttributeRecordType = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IValueMetaObjectCompositeData::CreateSpecialType(wxT("RecordType"), _("Record type"), wxEmptyString, g_enumRecordTypeCLSID, false, CValueEnumAccumulationRegisterRecordType::CreateDefEnumValue()));

	friend class IMetaData;
};

//********************************************************************************************
//*                                      Object                                              *
//********************************************************************************************

class CValueRecordSetObjectAccumulationRegister : public IValueRecordSetObject {
public:
	CValueRecordSetObjectAccumulationRegister(CValueMetaObjectAccumulationRegister* metaObject, const CUniquePairKey& uniqueKey = wxNullUniquePairKey) :
		IValueRecordSetObject(metaObject, uniqueKey)
	{
	}

	CValueRecordSetObjectAccumulationRegister(const CValueRecordSetObjectAccumulationRegister& source) :
		IValueRecordSetObject(source)
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