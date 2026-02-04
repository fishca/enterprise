#ifndef _RESOURCE_H__
#define _RESOURCE_H__

#include "backend/metaCollection/attribute/metaAttributeObject.h"

class BACKEND_API CValueMetaObjectResource : public CValueMetaObjectAttribute {
	wxDECLARE_DYNAMIC_CLASS(CValueMetaObjectResource);
public:

	CValueMetaObjectResource() : CValueMetaObjectAttribute(eValueTypes::TYPE_NUMBER) {
	}

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//get data selector 
	virtual eSelectorDataType GetFilterDataType() const;
};

#endif