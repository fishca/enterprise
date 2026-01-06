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
	bool AccessRight_Use() const { return IsFullAccess() || AccessRight(m_roleUse); }
#pragma endregion

	virtual bool FilterChild(const class_identifier_t& clsid) const {

		if (
			clsid == g_metaInterfaceCLSID
			)
			return true;

		return false;
	}

	CMetaObjectInterface(const wxString& name = wxEmptyString, const wxString& synonym = wxEmptyString, const wxString& comment = wxEmptyString);

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//prepare menu for item
	virtual bool PrepareContextMenu(wxMenu* defaultMenu);
	virtual void ProcessCommand(unsigned int id);

#pragma region __array_h__

	//interface
	std::vector<CMetaObjectInterface*> GetInterfaceArrayObject(
		std::vector<CMetaObjectInterface*>& array = std::vector<CMetaObjectInterface*>()) const {
		FillArrayObjectByFilter<CMetaObjectInterface>(array, { g_metaInterfaceCLSID });
		return array;
	}

#pragma endregion
#pragma region __filter_h__

	//interface
	template <typename _T1>
	CMetaObjectInterface* FindInterfaceObjectByFilter(const _T1& id) const {
		return FindObjectByFilter<CMetaObjectInterface>(id, { g_metaInterfaceCLSID });
	}

protected:

	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

#pragma endregion 
private:
	CPropertyPicture* m_propertyPicture = IPropertyObject::CreateProperty<CPropertyPicture>(m_categorySecondary, wxT("picture"), _("Picture"));
#pragma region role
	CRole* m_roleUse = IMetaObject::CreateRole(wxT("use"), _("Use"));
#pragma endregion
};

#endif 