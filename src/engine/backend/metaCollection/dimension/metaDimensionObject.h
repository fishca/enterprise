#ifndef _DIMENSION_H__
#define _DIMENSION_H__

#include "backend/metaCollection/attribute/metaAttributeObject.h"

class BACKEND_API CValueMetaObjectDimension : public CValueMetaObjectAttribute {
	wxDECLARE_DYNAMIC_CLASS(CValueMetaObjectDimension);
public:

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();
};

#endif