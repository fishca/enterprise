#ifndef __BACKEND_TYPE_H__
#define __BACKEND_TYPE_H__

#include "backend/typeDescription.h"

//////////////////////////////////////////////////////////////

class BACKEND_API ibBackendTypeFactory {
public:

#pragma region clsid 

	void SetDefaultMetaType(const ibValueTypes& valType = ibValueTypes::TYPE_EMPTY) { GetTypeDesc().SetDefaultMetaType(valType); }

	void SetDefaultMetaType(const ibClassID& clsid) { GetTypeDesc().SetDefaultMetaType(clsid); }
	void SetDefaultMetaType(const ibClassID& clsid, const ibTypeDescription::ibTypeData& descr) { GetTypeDesc().SetDefaultMetaType(clsid); }

	void SetDefaultMetaType(const std::vector<ibClassID>& array) { GetTypeDesc().SetDefaultMetaType(array); }
	void SetDefaultMetaType(const std::vector<ibClassID>& array, const ibTypeDescription::ibTypeData& descr) { GetTypeDesc().SetDefaultMetaType(array, descr); }
	void SetDefaultMetaType(const std::vector<ibClassID>& array, const ibQualifierNumber& qNumber, const ibQualifierDate& qDate, ibQualifierString& qString) { GetTypeDesc().SetDefaultMetaType(array, qNumber, qDate, qString); }

	void SetDefaultMetaType(const ibTypeDescription& typeDesc) { GetTypeDesc().SetDefaultMetaType(typeDesc); }

	void ClearMetaType() { GetTypeDesc().ClearMetaType(); }

	ibClassID GetFirstClsid() const { return GetTypeDesc().GetFirstClsid(); }
	ibClassID GetByIdx(unsigned int idx) const { return GetTypeDesc().GetByIdx(idx); }

	unsigned int GetClsidCount() const { return GetTypeDesc().GetClsidCount(); }

#pragma endregion

	//Create value by selected type
	virtual ibValue CreateValue() const;
	virtual ibValue* CreateValueRef() const;

	//convert value
	template<class retType = ibValue>
	retType* CreateAndConvertValueRef() {
		ibValue* retVal = CreateValueRef();
		if (retVal != nullptr)
			return CastValue<retType>(retVal);
		return (retType*)nullptr;
	}

	//Adjust value
	virtual ibValue AdjustValue() const;
	virtual ibValue AdjustValue(const ibValue& varValue) const;

	//get type description 
	virtual ibTypeDescription& GetTypeDesc() const = 0;
};

//////////////////////////////////////////////////////////////

enum ibSelectorDataType {
	ibSelectorDataType_any,
	ibSelectorDataType_boolean,
	ibSelectorDataType_reference,
	ibSelectorDataType_table,
	ibSelectorDataType_resource,
};

//////////////////////////////////////////////////////////////

class BACKEND_API ibBackendTypeConfigFactory :
	public ibBackendTypeFactory {
public:

	virtual ibSelectorDataType GetFilterDataType() const {
		return ibSelectorDataType::ibSelectorDataType_reference;
	}

	//Create value by selected type
	virtual ibValue CreateValue() const;
	virtual ibValue* CreateValueRef() const;

	//Adjust value
	virtual ibValue AdjustValue() const;
	virtual ibValue AdjustValue(const ibValue& varValue) const;

	//get metadata 
	virtual class ibMetaData* GetMetaData() const = 0;
};

//////////////////////////////////////////////////////////////

enum ibSourceDataType {
	ibSourceDataType_table,
	ibSourceDataType_tableColumn,
	ibSourceDataType_attribute,
};

//////////////////////////////////////////////////////////////

class BACKEND_API ibBackendTypeSourceFactory :
	public ibBackendTypeConfigFactory {
public:

	virtual ibSourceDataType GetFilterSourceDataType() const {
		return ibSourceDataType::ibSourceDataType_attribute;
	}

	//Get source object 
	virtual class ibSourceObject* GetSourceObject() const = 0;

	// filter data 
	virtual bool FilterSource(const class CSourceExplorer& src, const ibMetaID& id) const;
};

#endif