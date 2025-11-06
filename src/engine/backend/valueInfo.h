#ifndef __VALUE_INFO_H__
#define __VALUE_INFO_H__

#include "backend/uniqueKey.h"

//reference data
struct reference_t {

	reference_t(const meta_identifier_t& id, const guid_t& guid) : m_id(id), m_guid(guid) {}

	guid_t m_guid;
	meta_identifier_t m_id; // id of metadata 
};

#define reference_size_t int(sizeof(reference_t))

///////////////////

class IValueDataObject {
public:

	IValueDataObject(const CGuid& objGuid = wxNullGuid, bool newObject = true) : m_objGuid(objGuid), m_newObject(newObject) {}

	//support source set/get data 
	virtual bool SetValueByMetaID(const meta_identifier_t& id, const CValue& varMetaVal) { return false; }
	virtual bool GetValueByMetaID(const meta_identifier_t& id, CValue& pvarMetaVal) const { return false; }

	//get unique identifier 
	virtual CUniqueKey GetGuid() const { return m_objGuid; }

	//is new object?
	virtual bool IsNewObject() const { return m_newObject; }

	//get metaData from object 
	virtual class IMetaObjectRecordData* GetMetaObject() const = 0;

	//set modify 
	virtual void Modify(bool mod) {}

protected:
	bool m_newObject;
	valueArray_t m_listObjectValue;
	CGuid m_objGuid;
};

#endif