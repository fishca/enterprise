#ifndef __ATTRIBUTE_OBJECT_ENUM_H__
#define __ATTRIBUTE_OBJECT_ENUM_H__

enum eItemMode {
	eItemMode_Item,
	eItemMode_Folder,
	eItemMode_Folder_Item
};

enum eSelectMode {
	eSelectMode_Items = 1,
	eSelectMode_Folders,
	eSelectMode_FoldersAndItems
};

#pragma region enumeration
#include "backend/compiler/enumUnit.h"
class CValueEnumItemMode : public IEnumeration<eItemMode> {
	wxDECLARE_DYNAMIC_CLASS(CValueEnumItemMode);
public:
	CValueEnumItemMode() : IEnumeration() {}
	//CValueEnumItemMode(const eItemMode &mode) : IEnumeration(mode) {}

	virtual void CreateEnumeration() {
		AddEnumeration(eItemMode_Item, wxT("Items"), _("Items"));
		AddEnumeration(eItemMode_Folder, wxT("Folders"), _("Folders"));
		AddEnumeration(eItemMode_Folder_Item, wxT("FoldersAndItems"), _("Folders and items"));
	}
};
class CValueEnumSelectMode : public IEnumeration<eSelectMode> {
	wxDECLARE_DYNAMIC_CLASS(CValueEnumSelectMode);
public:
	CValueEnumSelectMode() : IEnumeration() {}
	//CValueEnumSelectMode(const eSelectMode &mode) : IEnumeration(mode) {}

	virtual void CreateEnumeration() {
		AddEnumeration(eSelectMode_Items, wxT("Items"), _("Items"));
		AddEnumeration(eSelectMode_Folders, wxT("Folders"), _("Folders"));
		AddEnumeration(eSelectMode_FoldersAndItems, wxT("FoldersAndItems"), _("Folders and items"));
	}
};
#pragma endregion 

#endif