#ifndef _ENUMERATIONOBJECT_H__
#define _ENUMERATIONOBJECT_H__

#include "backend/metaCollection/metaObject.h"

class BACKEND_API CValueMetaObjectEnum : public IValueMetaObject {
	wxDECLARE_DYNAMIC_CLASS(CValueMetaObjectEnum);
public:

	CGuid GetGuid() const {
		return m_metaGuid;
	}

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());
};

#endif