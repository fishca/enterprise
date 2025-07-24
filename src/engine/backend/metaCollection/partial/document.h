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

	CPropertyInnerModule<CMetaObjectModule>* m_propertyModuleObject = IPropertyObject::CreateProperty<CPropertyInnerModule<CMetaObjectModule>>(m_categorySecondary, IMetaObjectSourceData::CreateMetaObjectAndSetParent<CMetaObjectModule>(wxT("objectModule"), _("object module")));
	CPropertyInnerModule<CMetaObjectManagerModule>* m_propertyModuleManager = IPropertyObject::CreateProperty<CPropertyInnerModule<CMetaObjectManagerModule>>(m_categorySecondary, IMetaObjectSourceData::CreateMetaObjectAndSetParent<CMetaObjectManagerModule>(wxT("managerModule"), _("manager module")));

	CPropertyCategory* m_categoryForm = IPropertyObject::CreatePropertyCategory(wxT("defaultForms"), _("default forms"));

	CPropertyList* m_propertyDefFormObject = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormObject"), _("default object"), &CMetaObjectDocument::GetFormObject, wxNOT_FOUND);
	CPropertyList* m_propertyDefFormList = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormList"), _("default list"), &CMetaObjectDocument::GetFormList, wxNOT_FOUND);
	CPropertyList* m_propertyDefFormSelect = IPropertyObject::CreateProperty<CPropertyList>(m_categoryForm, wxT("defaultFormSelect"), _("default select"), &CMetaObjectDocument::GetFormSelect, wxNOT_FOUND);

	CPropertyRecord* m_propertyRegisterRecord = IPropertyObject::CreateProperty<CPropertyRecord>(m_categoryData, wxT("listRegisterRecord"), _("list register record"));

	//create default attributes
	//CMetaObjectAttributeDefault* m_attributeNumber = IMetaObjectSourceData::CreateString(wxT("number"), _("Number"), wxEmptyString, 11, true);
	//CMetaObjectAttributeDefault* m_attributeDate = IMetaObjectSourceData::CreateDate(wxT("date"), _("Date"), wxEmptyString, eDateFractions::eDateFractions_DateTime, true);
	//CMetaObjectAttributeDefault* m_attributePosted = IMetaObjectSourceData::CreateBoolean(wxT("posted"), _("Posted"), wxEmptyString);

	CPropertyInnerAttribute<>* m_propertyAttributeNumber = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IMetaObjectSourceData::CreateString(wxT("number"), _("Number"), wxEmptyString, 11, true));
	CPropertyInnerAttribute<>* m_propertyAttributeDate = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IMetaObjectSourceData::CreateDate(wxT("date"), _("Date"), wxEmptyString, eDateFractions::eDateFractions_DateTime, true));
	CPropertyInnerAttribute<>* m_propertyAttributePosted = IPropertyObject::CreateProperty<CPropertyInnerAttribute<>>(m_categoryCommon, IMetaObjectSourceData::CreateBoolean(wxT("posted"), _("Posted"), wxEmptyString));

private:
	bool GetFormObject(CPropertyList* prop);
	bool GetFormList(CPropertyList* prop);
	bool GetFormSelect(CPropertyList* prop);
public:

	CMetaDescription& GetRecordDescription() const { return m_propertyRegisterRecord->GetValueAsMetaDesc(); }

	CMetaObjectAttributeDefault* GetDocumentNumber() const { return m_propertyAttributeNumber->GetMetaObject(); }
	CMetaObjectAttributeDefault* GetDocumentDate() const { return m_propertyAttributeDate->GetMetaObject(); }
	CMetaObjectAttributeDefault* GetDocumentPosted() const { return m_propertyAttributePosted->GetMetaObject(); }

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

	//paste property from data
	virtual void OnPropertyPasted(IProperty* property);

	//events: 
	virtual bool OnCreateMetaObject(IMetaData* metaData, int flags);
	virtual bool OnLoadMetaObject(IMetaData* metaData);
	virtual bool OnSaveMetaObject();
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

	//override base objects 
	virtual std::vector<IMetaObjectAttribute*> GetDefaultAttributes() const override;

	//searched attributes 
	virtual std::vector<IMetaObjectAttribute*> GetSearchedAttributes() const override;

	//create associate value 
	virtual CMetaObjectForm* GetDefaultFormByID(const form_identifier_t& id);

	//create object data with metaForm
	virtual ISourceDataObject* CreateObjectData(IMetaObjectForm* metaObject);

	//create form with data 
	virtual IBackendValueForm* CreateObjectForm(IMetaObjectForm* metaForm) override {
		return metaForm->GenerateFormAndRun(
			nullptr, CreateObjectData(metaForm)
		);
	}

	//support form 
	virtual IBackendValueForm* GetObjectForm(const wxString& formName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);
	virtual IBackendValueForm* GetListForm(const wxString& formName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);
	virtual IBackendValueForm* GetSelectForm(const wxString& formName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr, const CUniqueKey& formGuid = wxNullGuid);

	//descriptions...
	wxString GetDataPresentation(const IValueDataObject* objValue) const;

	//get module object in compose object 
	virtual CMetaObjectModule* GetModuleObject() const { return m_propertyModuleObject->GetMetaObject(); }
	virtual CMetaObjectCommonModule* GetModuleManager() const { return m_propertyModuleManager->GetMetaObject(); }

	//prepare menu for item
	virtual bool PrepareContextMenu(wxMenu* defaultMenu);
	virtual void ProcessCommand(unsigned int id);

protected:

	//create empty object
	virtual IRecordDataObjectRef* CreateObjectRefValue(const Guid& objGuid = wxNullGuid);

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

protected:
	friend class IMetaData;
	friend class CRecordDataObjectDocument;
};

//********************************************************************************************
//*                                      Object                                              *
//********************************************************************************************

class CRecordDataObjectDocument : public IRecordDataObjectRef {
public:
	class CRecorderRegisterDocument : public CValue {
		wxDECLARE_DYNAMIC_CLASS(CRecorderRegisterDocument);
	private:
		CRecordDataObjectDocument* m_document;
		std::map<meta_identifier_t, IRecordSetObject*> m_records;
	public:

		void CreateRecordSet();
		bool WriteRecordSet();
		bool DeleteRecordSet();
		void ClearRecordSet();

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
		virtual inline bool IsEmpty() const {
			return false;
		}
	protected:
		CMethodHelper* m_methodHelper;
	};
private:
	CRecorderRegisterDocument* m_registerRecords;
protected:
	CRecordDataObjectDocument(CMetaObjectDocument* metaObject = nullptr, const Guid& guid = wxNullGuid);
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

	//support show 
	virtual void ShowFormValue(const wxString& formName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr);
	virtual IBackendValueForm* GetFormValue(const wxString& formName = wxEmptyString, IBackendControlFrame* ownerControl = nullptr);

	//support actionData
	virtual CActionCollection GetActionCollection(const form_identifier_t& formType);
	virtual void ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm);

public:
	virtual void SetDeletionMark(bool deletionMark = true);
protected:
	friend class IMetaData;
	friend class CMetaObjectDocument;
};

#endif