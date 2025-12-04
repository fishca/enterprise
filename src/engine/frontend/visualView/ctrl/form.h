#ifndef __FORM_VALUE_H__
#define __FORM_VALUE_H__

#include "frontend/docView/docView.h"
#include "frontend/visualView/ctrl/control.h"

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

class FRONTEND_API CVisualCommandProcessor : public wxCommandProcessor {

public:
	virtual bool CanUndo() const { return false; }
	virtual bool CanRedo() const { return false; }
};

class FRONTEND_API CVisualDocument : public CMetaDocument {
public:

	CVisualView* GetFirstView() const;

	CValueForm* GetValueForm() const;
	const CUniqueKey& GetFormKey() const;

	bool CompareFormKey(const CUniqueKey& formKey) const;

	CVisualDocument(CValueForm* valueForm);
	virtual ~CVisualDocument();

	virtual bool IsVisualDemonstrationDoc() const { return false; }

	virtual bool OnCreate(const wxString& WXUNUSED(path), long flags) override;
	virtual bool OnCloseDocument() override;

	virtual bool IsCloseOnOwnerClose() const override;

	virtual bool IsModified() const override { return m_documentModified; }
	virtual void Modify(bool modify) override;
	virtual bool Save() override;
	virtual bool SaveAs() override { return true; }

	virtual void SetDocParent(CMetaDocument* docParent) override;

protected:
	virtual CMetaView* DoCreateView();
private:
	CValuePtr<CValueForm> m_valueForm;
};

class FRONTEND_API CVisualDemoDocument : public CVisualDocument {
public:

	CVisualDemoDocument(CValueForm* valueForm) :
		CVisualDocument(valueForm)
	{
	}

	virtual bool IsVisualDemonstrationDoc() const { return true; }
};

class FRONTEND_API CVisualView : public CMetaView {
public:

	CVisualView() : m_visualHost(nullptr) {}
	virtual ~CVisualView();

	virtual wxPrintout* OnCreatePrintout() override;

	virtual bool OnCreate(CMetaDocument* doc, long flags) override;
	virtual void OnUpdate(wxView* sender, wxObject* hint = nullptr) override;
	virtual bool OnClose(bool deleteWindow = true) override;

	virtual void OnClosingDocument() override;

	CVisualHost* GetVisualHost() const { return m_visualHost; }

private:
	CVisualHost* m_visualHost;
};

//********************************************************************************************
//*                                      Value Frame                                         *
//********************************************************************************************

class FRONTEND_API CValueForm : public IBackendValueForm, public IValueFrame, public IModuleDataObject {
	wxDECLARE_DYNAMIC_CLASS(CValueForm);

private:

	enum {
		eSystem = eSizerItem + 1,
		eProcUnit,
		eAttribute
	};

protected:
	CPropertyCategory* m_categoryFrame = IPropertyObject::CreatePropertyCategory(wxT("frame"), _("frame"));
	CPropertyCaption* m_propertyCaption = IPropertyObject::CreateProperty<CPropertyCaption>(m_categoryFrame, wxT("caption"), _("caption"), wxEmptyString);
	CPropertyColour* m_propertyFG = IPropertyObject::CreateProperty<CPropertyColour>(m_categoryFrame, wxT("fg"), wxDefaultStypeFGColour);
	CPropertyColour* m_propertyBG = IPropertyObject::CreateProperty<CPropertyColour>(m_categoryFrame, wxT("bg"), wxDefaultStypeBGColour);
	CPropertyBoolean* m_propertyEnabled = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryFrame, wxT("enabled"), _("enabled"), true);
	CPropertyCategory* m_categorySizer = IPropertyObject::CreatePropertyCategory(wxT("sizer"), _("sizer"));
	CPropertyEnum<CValueEnumOrient>* m_propertyOrient = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumOrient>>(m_categorySizer, wxT("orient"), _("orient"), wxVERTICAL);
public:

	const CUniqueKey& GetFormKey() const { return m_formKey; }
	bool CompareFormKey(const CUniqueKey& formKey) const { return m_formKey == formKey; }

public:

	void SetCaption(const wxString& caption) { return m_propertyCaption->SetValue(caption); }
	wxString GetCaption() const { return m_propertyCaption->GetValueAsString(); }

	wxColour GetForegroundColour() const { return m_propertyFG->GetValueAsColour(); }
	wxColour GetBackgroundColour() const { return m_propertyBG->GetValueAsColour(); }

	bool IsFormEnabled() const { return m_propertyEnabled->GetValueAsBoolean(); }

	wxOrientation GetOrient() const { return m_propertyOrient->GetValueAsEnum(); }

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
	static IValueFrame* PasteObject(CValueForm* dstForm, IValueFrame* dstParent);

public:

	CValueForm(const IMetaObjectForm* creator = nullptr, IControlFrame* ownerControl = nullptr,
		ISourceDataObject* srcObject = nullptr, const CUniqueKey& formGuid = wxNullUniqueKey);

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
	virtual void InitializeForm(const IMetaObjectForm* creator, IControlFrame* ownerControl,
		ISourceDataObject* srcObject, const CUniqueKey& formGuid);

	virtual bool InitializeFormModule();

	//get metaData
	virtual IMetaData* GetMetaData() const;

	//runtime 
	virtual CProcUnit* GetFormProcUnit() const { return m_procUnit; }
	virtual CValueForm* GetImplValueRef() const override {
		return const_cast<CValueForm*>(this);
	}

	virtual ISourceDataObject* GetSourceObject() const { return m_sourceObject; }
	virtual const IMetaObjectForm* GetFormMetaObject() const { return m_metaFormObject; }

	IMetaObjectGenericData* GetMetaObject() const;

	// get control caption
	virtual wxString GetControlCaption() const;

	CValue GetCreatedValue() const { return m_createdValue; }
	CValue GetChangedValue() const { return m_changedValue; }

	virtual CValueForm* GetOwnerForm() const {
		return const_cast<CValueForm*>(this);
	}

	IValueFrame* GetOwnerControl() const {
		return dynamic_cast<IValueFrame*>(m_controlOwner);
	}

	/**
	* Get type form
	*/
	virtual form_identifier_t GetTypeForm() const;

	/**
	* Can delete object
	*/
	virtual bool CanDeleteControl() const { return false; }

	/**
	* Is editable object?
	*/
	virtual bool IsEditable() const;

public:

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

public:

	IValueFrame* CreateControl(const wxString& classControl, IValueFrame* control = nullptr);
	void RemoveControl(IValueFrame* control);

public:

	virtual bool LoadForm(const wxMemoryBuffer& formData);
	bool LoadChildForm(CMemoryReader& readerData, IValueFrame* controlParent);
	virtual wxMemoryBuffer SaveForm();
	bool SaveChildForm(CMemoryWriter& writterData, IValueFrame* controlParent);

	static CUniqueKey CreateFormUniqueKey(const IBackendControlFrame* ownerControl,
		const ISourceDataObject* sourceObject, const CUniqueKey& formGuid);

	static CValueForm* FindFormByUniqueKey(const IBackendControlFrame* ownerControl,
		const ISourceDataObject* sourceObject, const CUniqueKey& formGuid);

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

	virtual void ActivateForm() { ActivateDocForm(); }
	virtual void RefreshForm() { RefreshDocForm(); }
	virtual void UpdateForm();
	virtual bool CloseForm(bool force = false);
	virtual void HelpForm();
	virtual void ChangeForm();

	virtual bool GenerateForm(IRecordDataObjectRef* obj) const;
	virtual void ShowForm(IBackendMetaDocument* docParent = nullptr, bool createContext = true) override;

	//set & get modify 
	virtual void Modify(bool modify = true) {
		if (IsShown()) {
			GetVisualDocument()->Modify(modify);
		}
		m_formModified = modify;
	}

	virtual bool IsModified() const { return m_formModified; }

	//shown form 
	virtual bool IsShown() const { return GetVisualDocument() != nullptr; }

	//support close form
	virtual void CloseOnChoice(bool close = true) { m_closeOnChoice = close; }
	virtual bool IsCloseOnChoice() const { return m_closeOnChoice; }

	virtual void CloseOnOwnerClose(bool close = true) { m_closeOnOwnerClose = close; }
	virtual bool IsCloseOnOwnerClose() const { return m_closeOnOwnerClose; }

	//timers 
	void AttachIdleHandler(const wxString& procedureName, int interval, bool single);
	void DetachIdleHandler(const wxString& procedureName);

	//get visual document
	virtual CVisualDocument* GetVisualDocument() const;

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

	void ClearRecursive(IValueFrame* control);

	//doc event
	bool CreateDocForm(CMetaDocument* docParent, bool createContext = true);
	void ActivateDocForm();
	void ChoiceDocForm(CValue& vSelected);
	void RefreshDocForm();
	bool CloseDocForm();

	void OnIdleHandler(wxTimerEvent& event) {

		if (m_procUnit != nullptr) {

			auto iterator = std::find_if(m_idleHandlerArray.begin(), m_idleHandlerArray.end(),
				[event](const auto pair) { return pair.second == event.GetEventObject(); }
			);

			if (iterator != m_idleHandlerArray.end())
				CallAsEvent(iterator->first);
		}

		event.Skip();
	}

	enum
	{
		eDataBlock = 0x3550,
		eChildBlock = 0x3570
	};

	CValue					m_createdValue;
	CValue					m_changedValue;

	form_identifier_t		m_formType;
	CUniqueKey				m_formKey;

	bool					m_formModified;

	bool					m_closeOnChoice;
	bool					m_closeOnOwnerClose;

	const IMetaObjectForm* m_metaFormObject; // ref to metaData

	IControlFrame* m_controlOwner;
	ISourceDataObject* m_sourceObject;

	std::set<IValueControl*> m_listControl;
	std::map<wxString, wxTimer*> m_idleHandlerArray;

	CValuePtr<CValueFormCollectionControl> m_formCollectionControl;

	friend class IValueControl;
	friend class CValueFormCollectionControl;

	friend class CVisualDocument;
	friend class CVisualView;
};

#endif 
