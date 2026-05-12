#ifndef _DIMENSION_H__
#define _DIMENSION_H__

#include "backend/metaCollection/attribute/metaAttributeObject.h"

class BACKEND_API ibValueMetaObjectDimension : public ibValueMetaObjectAttribute {
	wxDECLARE_DYNAMIC_CLASS(ibValueMetaObjectDimension);
public:

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();
};

#endif