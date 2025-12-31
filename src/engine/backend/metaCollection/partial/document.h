#ifndef __DOCUMENT_H__
#define __DOCUMENT_H__

#include "commonObject.h"

#include "documentEnum.h"

//********************************************************************************************
//*                                  Factory & metaData                                      *
//********************************************************************************************

class CMetaObjectDocument : public IMetaObjectRecordDataMutableRef {
	wxDECLARE_DYNAMIC_CLASS(CMetaObjectDocument);
private:
	enum
	{
		ID_METATREE_OPEN_MODULE = 19000,
		ID_METATREE_OPEN_MANAGER = 19001,
	};

	enum
	{
		eFormObject = 1,
		eFormList,
		eFormSelect
	};

	virtual CFormTypeList GetFormType() const override {
		CFormTypeList formList;
		formList.AppendItem(wxT("formObject"), _("Form object"), eFormObject);
		formList.AppendItem(wxT("formList"), _("Form list"), eFormList);
		formList.AppendItem(wxT("formSelect"), _("Form select"), eFormSelect);
		return formList;
	}

protected:

	CPropertyInnerModule<CMetaObjectModule>* m_propertyModuleObject = IPropertyObject::CreateProperty<CPropertyInnerModule<CMetaObjectModule>>(m_categorySecondary, IMetaObjectCompositeData::CreateMetaObjectAndSetParent<CMetaObjectModule>(wxT("objectModule"), _("Object module")));
	CPropertyInnerModule<CMetaObjectManagerModule>* m_propertyModuleManager = IPropertyObject::CreateProperty<CPropertyInnerModule<CMetaObjectManagerModule>>(m_categorySecondary, IMetaObjectCompositeData::CreateMetaObjectAndSetParent<CMetaObjectManagerModule>(wxT("managerModule"), _("Manager module")));

	CPropertyCategory* m_categoryForm = IPropertyObject::CreatePropertyCategory(wxT("presetValues"), _("Preset values"));

	CPropertyList* m_propertyDefFormObject = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormObject"), _("Default Object Form"), &CMetaObjectDocument::FillFormObject);
	CPropertyList* m_propertyDefFormList = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormList"), _("Default List Form"), &CMetaObjectDocument::FillFormList);
	CPropertyList* m_propertyDefFormSelect = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormSelect"), _("Default Select Form"), &CMetaObjectDocument::FillFormSelect);

	CPropertyRecord* m_propertyRegisterRecord = IPropertyObject::CreateProperty<CPropertyRecord>(m_categoryData, wxT("listRegisterRecord"), _("List register record"));

	//create default attributes
	//CMetaObjectAttributePredefined* m_attributeNumber = IMetaObjectCompositeData::CreateString(wxT("number"), _("Number"), wxEmptyString, 11, true);
	//CMetaObjectAttributePredefined* m_attributeDate = IMetaObjectCompositeData::CreateDate(wxT("date"), _("Date"), wxEmptyString, eDateFractions::eDateFractions_DateTime, true);
	//CMetaObjectAttributePredefined* m_attributePosted = IMetaObjectCompositeData::CreateBoolean(wxT("posted"), _("Posted"), wxEmptyString);

	CPropertyInnerAttribute<>* m_propertyAttributeNumber = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IMetaObjectCompositeData::CreateString(wxT("number"), _("Number"), wxEmptyString, 11, true));
	CPropertyInnerAttribute<>* m_propertyAttributeDate = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IMetaObjectCompositeData::CreateDate(wxT("date"), _("Date"), wxEmptyString, eDateFractions::eDateFractions_DateTime, true));
	CPropertyInnerAttribute<>* m_propertyAttributePosted = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IMetaObjectCompositeData::CreateBoolean(wxT("posted"), _("Posted"), wxEmptyString));

public:

	CMetaDescription& GetRecordDescription() const { return m_propertyRegisterRecord->GetValueAsMetaDesc(); }

	CMetaObjectAttributePredefined* GetDocumentNumber() const { return m_propertyAttributeNumber->GetMetaObject(); }
	CMetaObjectAttributePredefined* GetDocumentDate() const { return m_propertyAttributeDate->GetMetaObject(); }
	CMetaObjectAttributePredefined* GetDocumentPosted() const { return m_propertyAttributePosted->GetMetaObject(); }

	//default constructor 
	CMetaObjectDocument();
	virtual ~CMetaObjectDocument();

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	/**
	* Property events
	*/
	virtual void OnPropertyCreated(IProperty* property);
	virtual bool OnPropertyChanging(IProperty* property, const wxVariant& newValue);
	virtual void OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue);

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

	//get attribute code 
	virtual IMetaObjectAttribute* GetAttributeForCode() const { return m_propertyAttributeNumber->GetMetaObject(); }

	//create associate value 
	virtual IMetaObjectForm* GetDefaultFormByID(const form_identifier_t& id) const;

#pragma region _form_builder_h_
	//support form 
	virtual IBackendValueForm* GetObjectForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);
	virtual IBackendValueForm* GetListForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);
	virtual IBackendValueForm* GetSelectForm(const wxString& strFormName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);
#pragma endregion
	//descriptions...
	wxString GetDataPresentation(const IValueDataObject* objValue) const;

	//get module object in compose object 
	virtual CMetaObjectModule* GetModuleObject() const { return m_propertyModuleObject->GetMetaObject(); }
	virtual CMetaObjectCommonModule* GetModuleManager() const { return m_propertyModuleManager->GetMetaObject(); }

	//prepare menu for item
	virtual bool PrepareContextMenu(wxMenu* defaultMenu);
	virtual void ProcessCommand(unsigned int id);

protected:

	//predefined array 
	virtual bool FillArrayObjectByPredefined(std::vector<IMetaObjectAttribute*>& array) const {

		array = {
			m_propertyAttributeNumber->GetMetaObject(),
			m_propertyAttributeDate->GetMetaObject(),
			m_propertyAttributePosted->GetMetaObject(),
			m_propertyAttributeReference->GetMetaObject(),
			m_propertyAttributeDeletionMark->GetMetaObject()
		};

		return true;
	}

	//searched array 
	virtual bool FillArrayObjectBySearched(std::vector<IMetaObjectAttribute*>& array) const {

		array = {
			m_propertyAttributeNumber->GetMetaObject(),
			m_propertyAttributeDate->GetMetaObject()
		};

		return true;
	}


	//create object data with meta form
	virtual ISourceDataObject* CreateSourceObject(IMetaObjectForm* metaObject);

	//create empty object
	virtual IRecordDataObjectRef* CreateObjectRefValue(const CGuid& objGuid = wxNullGuid);

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

protected:
	friend class IMetaData;
	friend class CRecordDataObjectDocument;

private:

	bool FillFormObject(CPropertyList* prop) {
		for (auto object : GetFormArrayObject()) {
			if (!object->IsAllowed()) continue;
			if (eFormObject == object->GetTypeForm()) {
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

	bool FillFormSelect(CPropertyList* prop) {
		for (auto object : GetFormArrayObject()) {
			if (!object->IsAllowed()) continue;
			if (eFormSelect == object->GetTypeForm()) {
				prop->AppendItem(
					object->GetName(),
					object->GetMetaID(),
					object->GetIcon(),
					object);
			}
		}
		return true;
	}
};

//********************************************************************************************
//*                                      Object                                              *
//********************************************************************************************

class CRecordDataObjectDocument : public IRecordDataObjectRef {
public:
	class CRecorderRegisterDocument : public CValue {
		wxDECLARE_DYNAMIC_CLASS(CRecorderRegisterDocument);
	public:

		void CreateRecordSet();
		bool WriteRecordSet();
		bool DeleteRecordSet();
		void ClearRecordSet();

		void RefreshRecordSet();

		CRecorderRegisterDocument(CRecordDataObjectDocument* currentDoc = nullptr);
		virtual ~CRecorderRegisterDocument();

		//standart override 
		virtual CMethodHelper* GetPMethods() const {
			//PrepareNames();
			return m_methodHelper;
		}

		virtual void PrepareNames() const;
		virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);

		virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);
		virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);

		//check is empty
		virtual inline bool IsEmpty() const { return false; }

	private:
		CRecordDataObjectDocument* m_document;
		std::map<meta_identifier_t, CValuePtr<IRecordSetObject>> m_records;
		CMethodHelper* m_methodHelper;
	};
protected:
	CRecordDataObjectDocument(CMetaObjectDocument* metaObject = nullptr, const CGuid& guid = wxNullGuid);
	CRecordDataObjectDocument(const CRecordDataObjectDocument& source);
public:
	virtual ~CRecordDataObjectDocument();

	bool IsPosted() const;

	void ClearRecordSet() {
		wxASSERT(m_registerRecords);
		m_registerRecords->ClearRecordSet();
	}

	void UpdateRecordSet() {
		wxASSERT(m_registerRecords);
		m_registerRecords->ClearRecordSet();
		m_registerRecords->CreateRecordSet();
	}

	//****************************************************************************
	//*                              Support id's                                *
	//****************************************************************************

	//save modify 
	virtual bool SaveModify() {
		return WriteObject(
			IsPosted() ? eDocumentWriteMode::eDocumentWriteMode_Posting : eDocumentWriteMode::eDocumentWriteMode_Write,
			eDocumentPostingMode::eDocumentPostingMode_Regular
		);
	}

	//default methods
	virtual bool FillObject(CValue& vFillObject) const {
		return Filling(vFillObject);
	}

	virtual IRecordDataObjectRef* CopyObject(bool showValue = false) {
		IRecordDataObjectRef* objectRef = CopyObjectValue();
		if (objectRef != nullptr && showValue)
			objectRef->ShowFormValue();
		return objectRef;
	}

	virtual bool WriteObject() {
		return WriteObject(
			IsPosted() ? eDocumentWriteMode::eDocumentWriteMode_Posting : eDocumentWriteMode::eDocumentWriteMode_Write,
			eDocumentPostingMode::eDocumentPostingMode_Regular
		);
	}
	virtual bool WriteObject(eDocumentWriteMode writeMode, eDocumentPostingMode postingMode);
	virtual bool DeleteObject();

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

public:
	virtual void SetDeletionMark(bool deletionMark = true);
private:
	CValuePtr<CRecorderRegisterDocument> m_registerRecords;
	friend class CValue;
};

#endif