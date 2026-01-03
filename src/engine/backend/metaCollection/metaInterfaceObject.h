#ifndef __META_INTERFACE_H__
#define __META_INTERFACE_H__

#include "metaObject.h"

class BACKEND_API CMetaObjectInterface : public IMetaObject {
	wxDECLARE_DYNAMIC_CLASS(CMetaObjectInterface);
protected:

	enum
	{
		ID_METATREE_OPEN_INTERFACE = 19000,
	};

public:

#pragma region access
	bool AccessRight_Use() const { return AccessRight(m_roleUse); }
#pragma endregion

	CMetaObjectInterface(const wxString& name = wxEmptyString, const wxString& synonym = wxEmptyString, const wxString& comment = wxEmptyString);

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//prepare menu for item
	virtual bool PrepareContextMenu(wxMenu* defaultMenu);
	virtual void ProcessCommand(unsigned int id);

private:
#pragma region role
	CRole* m_roleUse = IMetaObject::CreateRole(wxT("use"), _("Use"));
#pragma endregion
};

#endif 