#ifndef _BASE__FRAME__H__
#define _BASE__FRAME__H__

#include <wx/wx.h>

#include "frontend/frontend.h"

#include "backend/compiler/value.h"
#include "backend/propertyManager/propertyManager.h"

#include "backend/backend_type.h"
#include "backend/backend_form.h"

#include "frontend/visualView/formdefs.h"
#include "frontend/visualView/controlCtor.h"
#include "frontend/visualView/visual.h"

class BACKEND_API CSourceExplorer;
class BACKEND_API CProcUnit;

class BACKEND_API IMetaObjectForm;

class BACKEND_API ISourceDataObject;
class BACKEND_API IListDataObject;
class BACKEND_API IRecordDataObject;

class FRONTEND_API CValueForm;

class FRONTEND_API CVisualEditorHost;
class FRONTEND_API CVisualHost;

#include "backend/actionInfo.h"
#include "backend/moduleInfo.h"

#include "backend/fileSystem/fs.h"

class FRONTEND_API CVisualDocument;

#include "frontend/visualView/special/enum/valueEnum.h"

class FRONTEND_API IControlFrame : public IBackendControlFrame {
public:

	//get value control and guid 
	virtual bool GetControlValue(CValue& pvarControlVal) const { return false; }
	virtual CGuid GetControlGuid() const { return CGuid::newGuid(); }

	//get owner form 
	virtual CValueForm* GetOwnerForm() const { return nullptr; }

	//get ref class 
	virtual class_identifier_t GetClassType() const { return 0; }

	//get visual document
	virtual CVisualDocument* GetVisualDocument() const { return nullptr; }

	virtual bool HasQuickChoice() const = 0;
	virtual void ChoiceProcessing(CValue& vSelected) = 0;
};

class FRONTEND_API IValueFrame : public CValue,
	public IPropertyObjectHelper<IValueFrame>,
	public IControlFrame,
	public IActionDataObject {
	wxDECLARE_ABSTRACT_CLASS(IValueFrame);
protected:

	enum {
		eProperty,
		eControl,
		eEvent,
		eSizerItem,
	};

private:

	IValueFrame* DoFindControlByID(const form_identifier_t& id, IValueFrame* control);
	IValueFrame* DoFindControlByName(const wxString& controlName, IValueFrame* control);
	void DoGenerateNewID(form_identifier_t& id, IValueFrame* top);

public:

	void SetControlName(const wxString& controlName) { SetControlNameAsString(controlName); }
	wxString GetControlName() const {
		wxString result;
		GetControlNameAsString(result);
		return result;
	}

	IValueFrame();
	virtual ~IValueFrame();

	//system override 
	virtual wxString GetClassName() const final;
	virtual wxString GetObjectTypeName() const final;

	/**
	* Support generate id
	*/
	virtual form_identifier_t GenerateNewID();

	/**
	* Support get/set object id
	*/
	virtual bool SetControlID(const form_identifier_t& id) {
		if (id > 0) {
			IValueFrame* foundedControl =
				FindControlByID(id);
			wxASSERT(foundedControl == nullptr);
			if (foundedControl == nullptr) {
				m_controlId = id;
				return true;
			}
		}
		else {
			m_controlId = id;
			return true;
		}
		return false;
	}

	virtual form_identifier_t GetControlID() const { return m_controlId; }

	/**
	* Support control name
	*/

	virtual bool GetControlNameAsString(wxString& result) const {
		result = GetObjectTypeName();
		return true;
	}

	virtual bool SetControlNameAsString(const wxString& result) const {
		return false;
	}

	// get control caption
	virtual wxString GetControlCaption() const {
		return stringUtils::GenerateSynonym(GetClassName());
	}

	virtual CGuid GetControlGuid() const { return m_controlGuid; }

	/**
	* Find by control id
	*/
	virtual IValueFrame* FindControlByName(const wxString& controlName);
	virtual IValueFrame* FindControlByID(const form_identifier_t& id);

	/**
	* Support form
	*/
	virtual CValueForm* GetOwnerForm() const = 0;
	virtual void SetOwnerForm(CValueForm* ownerForm) {};

	/**
	* Support default menu
	*/
	virtual void PrepareDefaultMenu(wxMenu* menu) {}
	virtual void ExecuteMenu(IVisualHost* visualHost, int id) {}

	/**
	* Get wxObject from visual view (if exist)
	*/
	wxObject* GetWxObject() const;

	/**
	Sets whether the object is expanded in the object tree or not.
	*/
	void SetExpanded(bool expanded) { m_expanded = expanded; }

	/**
	Gets whether the object is expanded in the object tree or not.
	*/
	bool GetExpanded() const { return m_expanded; }

	//get metaData
	virtual IMetaData* GetMetaData() const = 0;

	/**
	* Can delete object
	*/
	virtual bool CanDeleteControl() const = 0;

public:

	// before/after run 
	virtual bool InitializeControl() { return true; }

public:

	/**
	* Create an instance of the wxObject and return a pointer
	*/
	virtual wxObject* Create(wxWindow* wndParent, IVisualHost* visualHost) {
		return new wxNoObject;
	}

	/**
	* Allows components to do something after they have been created.
	* For example, Abstract components like NotebookPage and SizerItem can
	* add the actual widget to the Notebook or sizer.
	*
	* @param wxobject The object which was just created.
	* @param wxparent The wxWidgets parent - the wxObject that the created object was added to.
	*/
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool firstСreated) {};

	/**
	* Allows components to respond when selected in object tree.
	* For example, when a wxNotebook's page is selected, it can switch to that page
	*/
	virtual void OnSelected(wxObject* wxobject) {};

	/**
	* Allows components to do something after they have been updated.
	*/
	virtual void Update(wxObject* wxobject, IVisualHost* visualHost) {};

	/**
	* Allows components to do something after they have been updated.
	* For example, Abstract components like NotebookPage and SizerItem can
	* add the actual widget to the Notebook or sizer.
	*
	* @param wxobject The object which was just updated.
	* @param wxparent The wxWidgets parent - the wxObject that the updated object was added to.
	*/
	virtual void OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost) {};

	/**
	 * Cleanup (do the reverse of Create)
	 */
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) {};

public:

	// call current event
	template <typename ...Types>
	bool CallAsEvent(const IEvent* event, Types&&... args) {
		if (event == nullptr)
			return false;
		const wxString& eventValue = event->GetValue();
		CProcUnit* formProcUnit = GetFormProcUnit();
		if (formProcUnit != nullptr && !eventValue.IsEmpty()) {
			CValue eventCancel = false;
			try {
				formProcUnit->CallAsProc(
					eventValue, //event name
					args...,
					eventCancel
				);
			}
			catch (...) {
				return false;
			}
			return eventCancel.GetBoolean();
		}

		return true;
	}

	//call current form
	template <typename ...Types>
	bool CallAsEvent(const wxString& functionName, Types&&... args) {
		CProcUnit* formProcUnit = GetFormProcUnit();
		if (formProcUnit != nullptr && !functionName.IsEmpty()) {
			try {
				formProcUnit->CallAsProc(
					functionName,
					args...
				);
			}
			catch (...) {
				return false;
			}
			return true;
		}

		return true;
	}

public:

	virtual IBackendValueForm* GetBackendForm() const;

	//get visual doc
	virtual CVisualDocument* GetVisualDocument() const;

	virtual bool HasQuickChoice() const;
	virtual void ChoiceProcessing(CValue& vSelected) {}

	IVisualEditorNotebook* FindVisualEditor() const;

public:

	//support actionData 
	virtual CActionCollection GetActionCollection(const form_identifier_t& formType) override { return CActionCollection(); }
	virtual void ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm) override {}

	class CValueEventContainer : public CValue {
		wxDECLARE_DYNAMIC_CLASS(CValueEventContainer);
	public:

		CValueEventContainer();
		CValueEventContainer(IValueFrame* ownerEvent);
		virtual ~CValueEventContainer();

		virtual CMethodHelper* GetPMethods() const {  // get a reference to the class helper for parsing attribute and method names
			//PrepareNames(); 
			return m_methodHelper;
		}

		virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.
		virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);

		virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);
		virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal); //attribute value

		virtual bool SetAt(const CValue& varKeyValue, const CValue& varValue);
		virtual bool GetAt(const CValue& varKeyValue, CValue& pvarValue);

		//Расширенные методы:
		bool Property(const CValue& varKeyValue, CValue& cValueFound);
		unsigned int Count() const { return m_controlEvent->GetEventCount(); }

		//Работа с итераторами:
		virtual bool HasIterator() const { return true; }
		virtual CValue GetIteratorEmpty();
		virtual CValue GetIteratorAt(unsigned int idx);
		virtual unsigned int GetIteratorCount() const { return Count(); }

	private:
		IValueFrame* m_controlEvent;
		CMethodHelper* m_methodHelper;
	};

	virtual bool GetControlValue(CValue& pvarControlVal) const { return false; }

	// memory reader form clpboard 
	static IValueFrame* CreatePasteObject(const CMemoryReader& reader,
		CValueForm* dstForm, IValueFrame* dstParent);

	/**
	* Property events
	*/
	virtual bool OnPropertyChanging(IProperty* property, const wxVariant& newValue);
	virtual void OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue);

	virtual bool OnEventChanging(IEvent* event, const wxString& newValue);
	virtual void OnEventChanged(IEvent* event, const wxVariant& oldValue, const wxVariant& newValue);

	/**
	* Devuelve la posicion del hijo o GetChildCount() en caso de no encontrarlo
	*/
	bool ChangeChildPosition(IValueFrame* obj, unsigned int pos);

	//copy & paste object 
	virtual bool CopyObject(CMemoryWriter& writer) const;
	virtual bool PasteObject(CMemoryReader& reader);

public:

	/**
	* Get type form
	*/
	virtual form_identifier_t GetTypeForm() const = 0;

	//runtime 
	virtual CProcUnit* GetFormProcUnit() const = 0;

	//counter
	virtual void ControlIncrRef() { CValue::IncrRef(); }
	virtual void ControlDecrRef() { CValue::DecrRef(); }

	//methods 
	virtual CMethodHelper* GetPMethods() const {  // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}

	virtual void PrepareNames() const; // this method is automatically called to initialize attribute and method names.

	//attributes 
	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);

	//check is empty
	virtual inline bool IsEmpty() const { return false; }

	virtual bool Init() final override;
	virtual bool Init(CValue** paParams, const long lSizeArray) final override;

	//Get ref class 
	virtual class_identifier_t GetClassType() const {
		return CValue::GetClassType();
	}

	virtual bool IsEditable() const;

public:

	static inline wxArrayString GetAllowedUserProperty() {

		wxArrayString arr;

		arr.Add(wxT("caption"));
		arr.Add(wxT("minimum_size"));
		arr.Add(wxT("maximum_size"));
		arr.Add(wxT("font"));
		arr.Add(wxT("fg"));
		arr.Add(wxT("bg"));
		arr.Add(wxT("align")); 
		arr.Add(wxT("stretch"));
		arr.Add(wxT("proportion"));
		arr.Add(wxT("orient"));
		arr.Add(wxT("tooltip"));
		arr.Add(wxT("visible"));

		return arr;
	}

	//load & save object in metaObject 
	bool LoadControl(const IMetaObjectForm* metaForm, CMemoryReader& dataReader);
	bool SaveControl(const IMetaObjectForm* metaForm, CMemoryWriter& dataWritter = CMemoryWriter(), bool copy_form = false);

protected:

	virtual void OnChangeChildPosition(IValueFrame* obj, unsigned int pos) {}
	virtual void OnChoiceProcessing(CValue& vSelected) {}

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader) { return true; }
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter()) { return true; }

protected:

	bool m_expanded = true; // is expanded in the object tree, allows for saving to file

	form_identifier_t	 m_controlId;
	CGuid				 m_controlGuid;

	CValuePtr<CValueEventContainer> m_valEventContainer;

	//object of methods 
	CMethodHelper* m_methodHelper;
};

#endif // !_BASE_H_
