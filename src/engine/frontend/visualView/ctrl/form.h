#ifndef __FORM_H__
#define __FORM_H__

#include "control.h"
#include "frontend/docView/docView.h"

#define defaultFormId 1
#define thisForm wxT("thisForm")

//********************************************************************************************
//*                                     Defines                                              *
//********************************************************************************************

class BACKEND_API CValueType;
class BACKEND_API CUniqueKey;

class CVisualView;

class BACKEND_API IMetaObjectForm;
class BACKEND_API IMetaObjectGenericData;

//********************************************************************************************
//*                                 define commom clsid									     *
//********************************************************************************************

//COMMON FORM
const class_identifier_t g_controlFormCLSID = string_to_clsid("CT_FRME");

//********************************************************************************************
//*                                  Visual Document & View                                  *
//********************************************************************************************

#include "backend/valueInfo.h"

class CVisualDocument : public CMetaDocument {
	CVisualHost* m_visualHost;
public:

	CVisualView* GetFirstView() const;

	//is demomode
	CVisualDocument() : CMetaDocument(), m_guidForm(CUniqueKey()) {
		m_visualHost = nullptr;
	}

	//other cases 
	CVisualDocument(const CUniqueKey& guid) : CMetaDocument(), m_guidForm(guid) {
		m_visualHost = nullptr;
	}

	virtual ~CVisualDocument();

	virtual bool OnSaveModified() override;
	virtual bool OnCloseDocument() override;

	virtual bool IsCloseOnOwnerClose() const override;

	virtual bool IsModified() const override;
	virtual void Modify(bool modify) override;
	virtual bool Save() override;
	virtual bool SaveAs() override;

	void SetVisualView(CVisualHost* visualHost);

	CVisualHost* GetVisualView() const { return m_visualHost; }
	CUniqueKey GetGuid() const { return m_guidForm; }

protected:

	CUniqueKey m_guidForm;
};

class CVisualView : public CMetaView {
	CValueForm* m_valueForm;
public:

	CVisualView(CValueForm* valueForm) : m_valueForm(valueForm) {}

	virtual wxPrintout* OnCreatePrintout() override;
	virtual void OnUpdate(wxView* sender, wxObject* hint = nullptr) override;
	virtual bool OnClose(bool deleteWindow = true) override;

	CValueForm* GetValueForm() const {
		return m_valueForm;
	}
};

//********************************************************************************************
//*                                      Value Frame                                         *
//********************************************************************************************

class FRONTEND_API CValueForm : public IBackendValueForm, public IValueFrame, public IModuleDataObject {
	wxDECLARE_DYNAMIC_CLASS(CValueForm);
private:
	enum {
		eSystem = eSizerItem + 1,
		eProcUnit
	};
protected:
	CPropertyCategory* m_categoryFrame = IPropertyObject::CreatePropertyCategory(wxT("frame"), _("frame"));
	CPropertyCaption* m_propertyCaption = IPropertyObject::CreateProperty<CPropertyCaption>(m_categoryFrame, wxT("caption"), _("caption"), _("Frame"));
	CPropertyColour* m_propertyFG = IPropertyObject::CreateProperty<CPropertyColour>(m_categoryFrame, wxT("fg"), wxColour(0, 120, 215));
	CPropertyColour* m_propertyBG = IPropertyObject::CreateProperty<CPropertyColour>(m_categoryFrame, wxT("bg"), wxColour(240, 240, 240));
	CPropertyBoolean* m_propertyEnabled = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryFrame, wxT("enabled"), _("enabled"), true);
	CPropertyCategory* m_categorySizer = IPropertyObject::CreatePropertyCategory(wxT("sizer"), _("sizer"));
	CPropertyEnum<CValueEnumOrient>* m_propertyOrient = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumOrient>>(m_categorySizer, wxT("orient"), _("orient"), wxVERTICAL);
private:

	bool m_formModified;

	IMetaObjectForm* m_metaFormObject; // ref to metaData
	ISourceDataObject* m_sourceObject;

public:

	void SetCaption(const wxString& caption) {
		return m_propertyCaption->SetValue(caption);
	}

	wxString GetCaption() const {
		return m_propertyCaption->GetValueAsString();
	}

	wxColour GetForegroundColour() const {
		return m_propertyFG->GetValueAsColour();
	}

	wxColour GetBackgroundColour() const {
		return m_propertyBG->GetValueAsColour();
	}

	bool IsFormEnabled() const {
		return m_propertyEnabled->GetValueAsBoolean();
	}

	wxOrientation GetOrient() const {
		return m_propertyOrient->GetValueAsEnum();
	}

	IValueFrame* NewObject(const class_identifier_t& clsid, IValueFrame* parentControl = nullptr, const CValue& generateId = true);
	IValueFrame* NewObject(const wxString& classControl, IValueFrame* controlParent, const CValue& generateId = true) {
		const class_identifier_t& clsid = CValue::GetIDObjectFromString(classControl);
		if (clsid > 0) {
			return NewObject(
				CValue::GetIDObjectFromString(classControl),
				controlParent,
				generateId
			);
		}
		return nullptr;
	}

	template <typename retType>
	inline retType* NewObject(const class_identifier_t& clsid, IValueFrame* parentControl = nullptr, const CValue& generateId = true) {
		return wxDynamicCast(
			NewObject(clsid, parentControl, generateId), retType);
	}

	template <typename retType>
	inline retType* NewObject(const wxString& className, IValueFrame* parentControl = nullptr, const CValue& generateId = true) {
		return wxDynamicCast(
			NewObject(className, parentControl, generateId), retType);
	}

	/**
	* Resuelve un posible conflicto de nombres.
	* @note el objeto a comprobar debe estar insertado en proyecto, por tanto
	*       no es válida para arboles "flotantes".
	*/
	void ResolveNameConflict(IValueFrame* control);

	/**
	* Fabrica de objetos.
	* A partir del nombre de la clase se crea una nueva instancia de un objeto.
	*/
	IValueFrame* CreateObject(const wxString& className, IValueFrame* parentControl = nullptr);

	/**
	* Crea un objeto como copia de otro.
	*/
	static bool CopyObject(IValueFrame* srcControl, bool copyOnPaste = true);
	static IValueFrame* PasteObject(CValueForm *dstForm, IValueFrame* dstParent);

public:

	CValueForm(IControlFrame* ownerControl = nullptr, IMetaObjectForm* metaForm = nullptr,
		ISourceDataObject* ownerSrc = nullptr, const CUniqueKey& formGuid = wxNullUniqueKey, bool readOnly = false);

	virtual ~CValueForm();

	//****************************************************************************
	//*                              Override attribute                          *
	//****************************************************************************

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

	//****************************************************************************
	//*                              Support methods                             *
	//****************************************************************************

	virtual void PrepareNames() const;

	virtual bool CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray);
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);

	//****************************************************************************
	//*                              Support form context                        *
	//****************************************************************************

	virtual void BuildForm(const form_identifier_t& formType);
	virtual void InitializeForm(IControlFrame* ownerControl, IMetaObjectForm* metaForm,
		ISourceDataObject* ownerSrc, const CUniqueKey& formGuid, bool readOnly = false);
	virtual bool InitializeFormModule();

	//get metaData
	virtual IMetaData* GetMetaData() const;

	//runtime 
	virtual CProcUnit* GetFormProcUnit() const {
		return m_procUnit;
	}

	virtual CValueForm* GetImplValueRef() const override {
		return const_cast<CValueForm*>(this);
	}

	virtual ISourceDataObject* GetSourceObject() const { return m_sourceObject; }
	virtual IMetaObjectForm* GetFormMetaObject() const { return m_metaFormObject; }

	IMetaObjectGenericData* GetMetaObject() const;

	/**
	* Support form
	*/
	virtual wxString GetControlName() const { return GetObjectTypeName(); }
	virtual CValueForm* GetOwnerForm() const { return const_cast<CValueForm*>(this); }

	CValue GetCreatedValue() const { return m_createdValue; }

	IValueFrame* GetOwnerControl() const { return (IValueFrame*)m_controlOwner; }

	/**
	* Get type form
	*/
	virtual form_identifier_t GetTypeForm() const;

	/**
	* Can delete object
	*/
	virtual bool CanDeleteControl() const { return false; }

public:

	std::vector<IValueControl*> m_listControl;

	class CValueFormCollectionControl : public CValue {
		wxDECLARE_DYNAMIC_CLASS(CValueFormCollectionControl);
	public:
		CValueFormCollectionControl();
		CValueFormCollectionControl(CValueForm* ownerFrame);
		virtual ~CValueFormCollectionControl();

		virtual CMethodHelper* GetPMethods() const {  // get a reference to the class helper for parsing attribute and method names
			//PrepareNames(); 
			return m_methodHelper;
		}
		virtual void PrepareNames() const;

		virtual bool CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray);
		virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);

		virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal); //attribute value
		virtual bool GetAt(const CValue& varKeyValue, CValue& pvarValue);

		//Расширенные методы:
		bool Property(const CValue& varKeyValue, CValue& cValueFound);
		unsigned int Count() const { return m_formOwner->m_listControl.size(); }

		//Работа с итераторами:
		virtual bool HasIterator() const { return true; }
		virtual CValue GetIteratorEmpty();
		virtual CValue GetIteratorAt(unsigned int idx);
		virtual unsigned int GetIteratorCount() const { return Count(); }
	private:
		CValueForm* m_formOwner;
		CMethodHelper* m_methodHelper;
	};

	class CValueFormCollectionData : public CValue {
		wxDECLARE_DYNAMIC_CLASS(CValueFormCollectionData);
	public:
		CValueFormCollectionData();
		CValueFormCollectionData(CValueForm* ownerFrame);
		virtual ~CValueFormCollectionData();

		virtual CMethodHelper* GetPMethods() const {  // get a reference to the class helper for parsing attribute and method names
			//PrepareNames(); 
			return m_methodHelper;
		}
		virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.
		virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);

		virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
		virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value
		virtual bool SetAt(const CValue& varKeyValue, const CValue& varValue);
		virtual bool GetAt(const CValue& varKeyValue, CValue& pvarValue);

		//Расширенные методы:
		bool Property(const CValue& varKeyValue, CValue& cValueFound);
		unsigned int Count() const;

		//Работа с итераторами:
		virtual bool HasIterator() const { return true; }
		virtual CValue GetIteratorEmpty();
		virtual CValue GetIteratorAt(unsigned int idx);
		virtual unsigned int GetIteratorCount() const { return Count(); }
	private:
		CValueForm* m_formOwner;
		CMethodHelper* m_methodHelper;
	};

	CValueFormCollectionControl* m_formCollectionControl;
	CValueFormCollectionData* m_formCollectionData;

public:

	CVisualDocument* m_valueFormDocument;

protected:

	friend class CVisualDocument;
	friend class CVisualView;

	friend class CValueFormCollectionControl;

	bool CreateDocForm(CMetaDocument* docParent, bool demo = false);
	bool CloseDocForm();

public:

	IValueFrame* CreateControl(const wxString& classControl, IValueFrame* control = nullptr);
	void RemoveControl(IValueFrame* control);

private:

	void ClearRecursive(IValueFrame* control);

public:

	virtual bool LoadForm(const wxMemoryBuffer& formData);
	bool LoadChildForm(CMemoryReader& readerData, IValueFrame* controlParent);
	virtual wxMemoryBuffer SaveForm();
	bool SaveChildForm(CMemoryWriter& writterData, IValueFrame* controlParent);

	static CValueForm* FindFormByUniqueKey(const CUniqueKey& guid);
	static CValueForm* FindFormByControlUniqueKey(const CUniqueKey& guid);
	static CValueForm* FindFormBySourceUniqueKey(const CUniqueKey& guid);

	static bool UpdateFormUniqueKey(const CUniquePairKey& guid);

	//notify
	virtual void NotifyCreate(const CValue& vCreated);
	virtual void NotifyChange(const CValue& vChanged);
	virtual void NotifyDelete(const CValue& vChanged);

	virtual void NotifyChoice(CValue& vSelected);

	CValue CreateControl(const CValueType* classControl, const CValue& vControl);
	CValue FindControl(const CValue& vControl);
	void RemoveControl(const CValue& vControl);

public:

	virtual void ActivateForm();
	virtual void RefreshForm();
	virtual void UpdateForm();
	virtual bool CloseForm(bool force = false);
	virtual void HelpForm();

	virtual bool GenerateForm(IRecordDataObjectRef* obj) const;
	virtual void ShowForm(IBackendMetaDocument* docParent = nullptr, bool demo = false) override;

	//set & get modify 
	virtual void Modify(bool modify = true) {
		if (m_valueFormDocument != nullptr) {
			m_valueFormDocument->Modify(modify);
		}
		m_formModified = modify;
	}

	virtual bool IsModified() const { return m_formModified; }

	//shown form 
	virtual bool IsShown() const { return m_valueFormDocument != nullptr; }

	//support close form
	virtual void CloseOnChoice(bool close = true) { m_closeOnChoice = close; }
	virtual bool IsCloseOnChoice() const { return m_closeOnChoice; }

	virtual void CloseOnOwnerClose(bool close = true) { m_closeOnOwnerClose = close; }
	virtual bool IsCloseOnOwnerClose() const { return m_closeOnOwnerClose; }

	//timers 
	void AttachIdleHandler(const wxString& procedureName, int interval, bool single);
	void DetachIdleHandler(const wxString& procedureName);

	//get visual document
	virtual CVisualDocument* GetVisualDocument() const { return m_valueFormDocument; }

	//special proc
	virtual void Update(wxObject* wxobject, IVisualHost* visualHost);
	virtual void OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost);

	//actionData
	virtual CActionCollection GetActionCollection(const form_identifier_t& formType);
	virtual void ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm);

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

	virtual int GetComponentType() const { return COMPONENT_TYPE_FRAME; }

private:

	void OnIdleHandler(wxTimerEvent& event);

protected:

	enum
	{
		eDataBlock = 0x3550,
		eChildBlock = 0x3570
	};

	form_identifier_t		m_defaultFormType;

	CUniqueKey				m_formKey;

	CValue					m_createdValue;

	bool					m_closeOnChoice;
	bool					m_closeOnOwnerClose;

	IControlFrame* m_controlOwner;

	std::map<wxString, wxTimer*> m_aIdleHandlers;
};

#endif 
