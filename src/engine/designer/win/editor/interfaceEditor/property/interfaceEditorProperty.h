#ifndef _INTERFACE_EDITOR_PROPERTY_H_
#define _INTERFACE_EDITOR_PROPERTY_H_

#include "backend/propertyManager/propertyManager.h"

enum eMenuType {
	eMenu,
	eSubMenu,
	eSeparator
};

#pragma region enumeration
#include "backend/compiler/enumUnit.h"
class CValueEnumMenuType : public IEnumeration<eMenuType> {
	wxDECLARE_DYNAMIC_CLASS(CValueEnumMenuType);
public:
	CValueEnumMenuType() : IEnumeration() {}
	//CValueEnumMenuType(const eMenuType& mode) : IEnumeration(mode) {}

	virtual void CreateEnumeration() {
		AddEnumeration(eMenu, wxT("menu"));
		AddEnumeration(eSubMenu, wxT("subMenu"));
		AddEnumeration(eSeparator, wxT("separator"));
	}
};
#pragma endregion 

class CPropertyInterfaceItem : public IPropertyObject {

	bool GetSubMenu(CPropertyList* prop) {
		prop->AppendItem(_("<Custom submenu>"), wxNOT_FOUND);
		prop->AppendItem(_("file"), wxID_FILE);
		prop->AppendItem(_("edit"), wxID_EDIT);
		//optionlist->AppendItem(_("all operations..."), wxID_ENTERPRISE_ALL_OPERATIONS);
		return true;
	}

protected:

	CPropertyCategory* m_interfaceCategory = IPropertyObject::CreatePropertyCategory("general");
	CPropertyEnum<CValueEnumMenuType>* m_propertyMenuType = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumMenuType>>(m_interfaceCategory, "menuType", eMenuType::eMenu);
	CPropertyList* m_propertySubMenu = IPropertyObject::CreateProperty<CPropertyList>(m_interfaceCategory, "subMenu", &CPropertyInterfaceItem::GetSubMenu, wxNOT_FOUND);
	CPropertyText* m_propertyAction = IPropertyObject::CreateProperty<CPropertyText>(m_interfaceCategory, "action", wxEmptyString);

	CPropertyCategory* m_interfacePresentation = IPropertyObject::CreatePropertyCategory("presentation");
	CPropertyCaption* m_propertyCaption = IPropertyObject::CreateProperty<CPropertyCaption>(m_interfacePresentation, "caption", wxEmptyString);
	CPropertyText* m_propertyToolTip = IPropertyObject::CreateProperty<CPropertyText>(m_interfacePresentation, "tooltip", wxEmptyString);
	CPropertyText* m_propertyDescription = IPropertyObject::CreateProperty<CPropertyText>(m_interfacePresentation, "tooltip", wxEmptyString);
	CPropertyPicture* m_propertyPicture = IPropertyObject::CreateProperty<CPropertyPicture>(m_interfacePresentation, "picture");

public:

	void SetCaption(const wxString& caption) {
		m_propertyCaption->SetValue(caption);
	}

	wxString GetCaption() const {
		return m_propertyCaption->GetValueAsString();
	}

	CPropertyInterfaceItem(const eMenuType &menuType) {
		m_propertyMenuType->SetValue(menuType);
	}

	virtual ~CPropertyInterfaceItem();

	//system override 
	virtual int GetComponentType() const { return COMPONENT_TYPE_ABSTRACT; }

	virtual wxString GetObjectTypeName() const override { return wxT("interfaceEditor"); }
	virtual wxString GetClassName() const override { return wxT("interfaceEditor"); }

	/**
	* Property events
	*/
	virtual void OnPropertyRefresh(class wxPropertyGridManager* pg,
		class wxPGProperty* pgProperty, IProperty* property);
};

#endif 