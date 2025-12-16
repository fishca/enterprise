#ifndef _NOTEBOOKS_H__
#define _NOTEBOOKS_H__

#include "window.h"
#include <wx/aui/auibook.h>

class CValueNotebookPage;

//********************************************************************************************
//*                                 define commom clsid									     *
//********************************************************************************************

//COMMON FORM
const class_identifier_t g_controlNotebookCLSID = string_to_clsid("CT_NTBK");
const class_identifier_t g_controlNotebookPageCLSID = string_to_clsid("CT_NTPG");

//********************************************************************************************
//*                                 Value Notebook                                           *
//********************************************************************************************

class CValueNotebook : public IValueWindow {
	wxDECLARE_DYNAMIC_CLASS(CValueNotebook);
protected:

	friend class CValueNotebookPage;

	CPropertyCategory* m_categoryNotebook = IPropertyObject::CreatePropertyCategory(wxT("notebook"), _("Notebook"));
	CPropertyEnum<CValueEnumOrientNotebookPage>* m_propertyOrient = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumOrientNotebookPage>>(m_categoryNotebook, wxT("orientPage"), _("Orient page"), wxAUI_NB_TOP);
	CPropertyCategory* m_categoryEvent = IPropertyObject::CreatePropertyCategory(wxT("event"), _("Event"));
	CEventControl* m_eventOnPageChanged = IPropertyObject::CreateEvent<CEventControl>(m_categoryEvent, wxT("onPageChanged"), _("Page changed"), wxArrayString{ wxT("page") });

private:
	CValueNotebookPage* m_activePage;
	std::vector< CValueNotebookPage*> m_aPages;
public:

	CValueNotebook();

	//get caption 
	virtual wxString GetControlCaption() const {
		return _("Notebook");
	}

	//control factory 
	virtual wxObject* Create(wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
	virtual void OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void OnSelected(wxObject* wxobject) override;
	virtual void Update(wxObject* wxobject, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	//methods 
	virtual void PrepareNames() const;                          // this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray) override;       //method call

	/**
	* Support default menu
	*/
	virtual void PrepareDefaultMenu(wxMenu* m_menu);
	virtual void ExecuteMenu(IVisualHost* visualHost, int id);

	void AddNotebookPage();

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

protected:

	//Events
	void OnPageChanged(wxAuiNotebookEvent& event);
	void OnBGDClick(wxAuiNotebookEvent& event);
	void OnEndDrag(wxAuiNotebookEvent& event);
};

class CValueNotebookPage : public IValueControl {
	wxDECLARE_DYNAMIC_CLASS(CValueNotebookPage);
protected:
	CPropertyCategory* m_categoryPage = IPropertyObject::CreatePropertyCategory(wxT("page"), _("Page"));
	CPropertyCaption* m_propertyCaption = IPropertyObject::CreateProperty<CPropertyCaption>(m_categoryPage, wxT("caption"), _("Caption"), _("New page"));
	CPropertyBoolean* m_propertyVisible = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryPage, wxT("visible"), _("Visible"), true);
	CPropertyPicture* m_propertyIcon = IPropertyObject::CreateProperty<CPropertyPicture>(m_categoryPage, wxT("icon"), _("Icon"));
	CPropertyCategory* m_categorySizer = IPropertyObject::CreatePropertyCategory(wxT("sizer"), _("sizer"));
	CPropertyEnum<CValueEnumOrient>* m_propertyOrient = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumOrient>>(m_categorySizer, wxT("orient"), _("Orient"), wxVERTICAL);
public:

	CValueNotebookPage();

	//get caption 
	virtual wxString GetControlCaption() const {
		if (!m_propertyCaption->IsEmptyProperty())
			return _("Page item: ") + m_propertyCaption->GetValueAsString();
		return _("Page item: ") + _("<empty caption>");
	}

	//control factory 
	virtual wxObject* Create(wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
	virtual void OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void OnSelected(wxObject* wxobject) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	virtual bool CanDeleteControl() const;

	virtual int GetComponentType() const { return COMPONENT_TYPE_WINDOW; }

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

	friend class CValueNotebook;
};

#endif 