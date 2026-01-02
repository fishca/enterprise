#ifndef _TOOLBAR_H__
#define _TOOLBAR_H__

#include "window.h"
#include "frontend/win/ctrls/toolBar.h"

class CValueToolBarItem;
class CValueToolBarSeparator;

//********************************************************************************************
//*                                 define commom clsid									     *
//********************************************************************************************

//COMMON FORM
const class_identifier_t g_controlToolBarCLSID = string_to_clsid("CT_TLBR");
const class_identifier_t g_controlToolBarItemCLSID = string_to_clsid("CT_TLIT");
const class_identifier_t g_controlToolBarSeparatorCLSID = string_to_clsid("CT_TLIS");

//********************************************************************************************
//*                                 Value Toolbar                                            *
//********************************************************************************************

class CValueToolbar : public IValueWindow {
	wxDECLARE_DYNAMIC_CLASS(CValueToolbar);
protected:
	bool GetActionSource(CPropertyList*);
protected:
	CPropertyCategory* m_categoryAction = IPropertyObject::CreatePropertyCategory(wxT("action"), _("Toolbar"));
	CPropertyList* m_actSource = IPropertyObject::CreateProperty<CPropertyList>(m_categoryAction, wxT("actionSource"), _("Source"), &CValueToolbar::GetActionSource, wxNOT_FOUND);
public:

	void SetActionSrc(const form_identifier_t& action) { return m_actSource->SetValue(action); }
	form_identifier_t GetActionSrc() const { return m_actSource->GetValueAsInteger(); }

	CValueToolbar();

	//get caption 
	virtual wxString GetControlCaption() const {

		if (!m_actSource->IsEmptyProperty()) {
			CValue pvarPropVal;
			if (m_actSource->GetDataValue(pvarPropVal))
				return _("ToolBar: ") + stringUtils::GenerateSynonym(pvarPropVal.GetClassName());
		}

		return _("ToolBar:") + _("<empty source>");
	}

	//control factory 
	virtual wxObject* Create(wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
	virtual void Update(wxObject* wxobject, IVisualHost* visualHost) override;
	virtual void OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	/**
	* Property events
	*/
	virtual void OnPropertyCreated(IProperty* property);
	virtual void OnPropertySelected(IProperty* property);
	virtual void OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue);

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

	/**
	* Support default menu
	*/
	virtual void PrepareDefaultMenu(wxMenu* m_menu);
	virtual void ExecuteMenu(IVisualHost* visualHost, int id);

	//specific function 
	void AddToolItem();
	void AddToolSeparator();

	//array of the commands 
	const CActionCollection& GetActionArray() const { return m_actionArray; }

protected:

	//events 
	void OnToolBarLeftDown(wxMouseEvent& event);
	void OnTool(wxCommandEvent& event);
	void OnToolDropDown(wxAuiToolBarEvent& event);
	void OnRightDown(wxMouseEvent& event);

	//storage for action array 
	CActionCollection m_actionArray;

	friend class CValueForm;
};

#include "frontend/artProvider/null/null.xpm"

class CValueToolBarItem : public IValueControl {
	wxDECLARE_DYNAMIC_CLASS(CValueToolBarItem);
private:
	bool GetToolAction(CEventAction* evtList);
private:

	CPropertyCategory* m_categoryToolbar = IPropertyObject::CreatePropertyCategory(wxT("toolBarItem"), _("Item"));

	CPropertyTString* m_propertyCaption = IPropertyObject::CreateProperty<CPropertyTString>(m_categoryToolbar, wxT("caption"), _("Caption"), wxT(""));
	CPropertyEnum<CValueEnumRepresentation>* m_propertyRepresentation = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumRepresentation>>(m_categoryToolbar, wxT("representation"), _("Representation"), enRepresentation::eRepresentation_Auto);
	CPropertyPicture* m_propertyPicture = IPropertyObject::CreateProperty<CPropertyPicture>(m_categoryToolbar, wxT("picture"), _("Picture"));
	CPropertyBoolean* m_propertyContextMenu = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryToolbar, wxT("contextMenu"), _("Context menu"), false);
	CPropertyTString* m_properyTooltip = IPropertyObject::CreateProperty<CPropertyTString>(m_categoryToolbar, wxT("tooltip"), _("Tooltip"), wxEmptyString);
	CPropertyBoolean* m_propertyEnabled = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryToolbar, wxT("enabled"), _("Enabled"), true);

	CEventAction* m_eventAction = IPropertyObject::CreateEvent<CEventAction>(m_categoryToolbar, wxT("action"), _("Action"), wxArrayString{ wxT("control") }, &CValueToolBarItem::GetToolAction, wxNOT_FOUND);

public:

	void SetCaption(const wxString& caption) { return m_propertyCaption->SetValue(caption); }
	wxString GetCaption() const { return m_propertyCaption->GetValueAsTranslateString(); }

	void SetToolTip(const wxString& caption) { return m_properyTooltip->SetValue(caption); }
	wxString GetToolTip() const { return m_properyTooltip->GetValueAsTranslateString(); }

	void SetAction(const CActionDescription& action) { return m_eventAction->SetValue(action); }
	const CActionDescription& GetAction() const { return m_eventAction->GetValueAsActionDesc(); }

	///////////////////////////////////////////////////////////////////////

	CValueToolbar* GetOwner() const { return m_parent->ConvertToType<CValueToolbar>(); }

#pragma region __tool_item_desc_h__
	wxBitmap GetItemPicture(const CActionCollection& collection) const {
		const CActionDescription& actionDesc = m_eventAction->GetValueAsActionDesc();
		if (m_propertyPicture->IsEmptyProperty()) {
			const action_identifier_t selected = actionDesc.GetSystemAction();
			if (selected != wxNOT_FOUND) {
				for (unsigned int i = 0; i < collection.GetCount(); i++) {
					const action_identifier_t& id = collection.GetID(i);
					if (selected == collection.GetID(i)) {
						const CPictureDescription& pictureDesc = collection.GetPictureByID(actionDesc.GetSystemAction());
						if (pictureDesc.IsEmptyPicture())
							return wxNullBitmap;
						return CBackendPicture::CreatePicture(pictureDesc, GetMetaData());
					}
				}
			}
			else if (m_propertyCaption->IsEmptyProperty()) {
				return wxBitmap(s_null_xpm);
			}
		}

		return m_propertyPicture->GetValueAsBitmap();
	}

	wxString GetItemCaption(const CActionCollection& collection) const {
		const CActionDescription& actionDesc = m_eventAction->GetValueAsActionDesc();
		if (m_propertyCaption->IsEmptyProperty()) {
			const action_identifier_t selected = actionDesc.GetSystemAction();
			if (selected != wxNOT_FOUND) {
				for (unsigned int i = 0; i < collection.GetCount(); i++) {
					const action_identifier_t& id = collection.GetID(i);
					if (selected == collection.GetID(i)) {
						return collection.GetCaptionByID(selected);
					}
				}
			}
		}
		return m_propertyCaption->GetValueAsTranslateString();
	}

	wxString GetItemToolTip(const CActionCollection& collection) const {
		const CActionDescription& actionDesc = m_eventAction->GetValueAsActionDesc();
		if (m_properyTooltip->IsEmptyProperty()) {
			const action_identifier_t selected = actionDesc.GetSystemAction();
			if (selected != wxNOT_FOUND) {
				for (unsigned int i = 0; i < collection.GetCount(); i++) {
					const action_identifier_t& id = collection.GetID(i);
					if (selected == collection.GetID(i)) {
						return collection.GetCaptionByID(selected);
					}
				}
			}
		}
		return m_properyTooltip->GetValueAsTranslateString();
	}

	enRepresentation GetItemRepresentation(const CActionCollection& collection) const {
		const CActionDescription& actionDesc = m_eventAction->GetValueAsActionDesc();
		if (m_propertyPicture->IsEmptyProperty()) {
			const action_identifier_t selected = actionDesc.GetSystemAction();
			if (selected != wxNOT_FOUND) {
				for (unsigned int i = 0; i < collection.GetCount(); i++) {
					const action_identifier_t& id = collection.GetID(i);
					if (selected == collection.GetID(i)) {
						const CPictureDescription& pictureDesc = collection.GetPictureByID(actionDesc.GetSystemAction());
						if (pictureDesc.IsEmptyPicture())
							return enRepresentation::eRepresentation_PictureAndText;
						return collection.IsCreatePictureAndText(id) ?
							enRepresentation::eRepresentation_PictureAndText : enRepresentation::eRepresentation_Picture;
					}
				}
			}
		}
		return enRepresentation::eRepresentation_PictureAndText;
	}

#pragma endregion 

	///////////////////////////////////////////////////////////////////////

	CValueToolBarItem();

	//get caption 
	virtual wxString GetControlCaption() const {
		if (!m_propertyCaption->IsEmptyProperty())
			return GetCaption();
		return _("<empty caption>");
	}

	//control factory
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
	virtual void OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	virtual bool CanDeleteControl() const;
	virtual int GetComponentType() const { return COMPONENT_TYPE_ABSTRACT; }

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

	friend class CValueForm;
	friend class CValueToolbar;
};

class CValueToolBarSeparator : public IValueControl {
	wxDECLARE_DYNAMIC_CLASS(CValueToolBarSeparator);
public:

	///////////////////////////////////////////////////////////////////////

	CValueToolbar* GetOwner() const {
		return m_parent->ConvertToType<CValueToolbar>();
	}

	///////////////////////////////////////////////////////////////////////

	CValueToolBarSeparator();

	//get caption 
	virtual wxString GetControlCaption() const {
		return _("Separator");
	}

	//control factory
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
	virtual void OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	virtual bool CanDeleteControl() const;

	virtual int GetComponentType() const { return COMPONENT_TYPE_ABSTRACT; }

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

	friend class CValueForm;
	friend class CValueToolbar;
};

#endif

