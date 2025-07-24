#ifndef __BACKEND_TYPE_H__
#define __BACKEND_TYPE_H__

#include "backend/typeDescription.h"

//////////////////////////////////////////////////////////////

class BACKEND_API IBackendTypeFactory {
public:

#pragma region clsid 

	void SetDefaultMetaType(const eValueTypes& valType = eValueTypes::TYPE_EMPTY) { GetTypeDesc().SetDefaultMetaType(valType); }
	
	void SetDefaultMetaType(const class_identifier_t& clsid) { GetTypeDesc().SetDefaultMetaType(clsid); }
	void SetDefaultMetaType(const class_identifier_t& clsid, const CTypeDescription::CTypeData& descr) { GetTypeDesc().SetDefaultMetaType(clsid); }
	
	void SetDefaultMetaType(const std::set<class_identifier_t>& clsids) { GetTypeDesc().SetDefaultMetaType(clsids); }
	void SetDefaultMetaType(const std::set<class_identifier_t>& clsids, const CTypeDescription::CTypeData& descr) { GetTypeDesc().SetDefaultMetaType(clsids, descr); }
	void SetDefaultMetaType(const std::set<class_identifier_t>& clsids, const CQualifierNumber& qNumber, const CQualifierDate& qDate, CQualifierString& qString) { GetTypeDesc().SetDefaultMetaType(clsids, qNumber, qDate, qString); }

	void SetDefaultMetaType(const CTypeDescription& typeDesc) { GetTypeDesc().SetDefaultMetaType(typeDesc); }

	void ClearMetaType() { GetTypeDesc().ClearMetaType(); }

	class_identifier_t GetFirstClsid() const { return GetTypeDesc().GetFirstClsid(); }
	class_identifier_t GetByIdx(unsigned int idx) const { return GetTypeDesc().GetByIdx(idx); }

	unsigned int GetClsidCount() const { return GetTypeDesc().GetClsidCount(); }

#pragma endregion

	//Create value by selected type
	virtual CValue CreateValue() const;
	virtual CValue* CreateValueRef() const;

	//convert value
	template<class retType = CValue>
	retType* CreateAndConvertValueRef() {
		CValue* retVal = CreateValueRef();
		if (retVal != nullptr)
			return value_cast<retType>(retVal);
		return (retType*)nullptr;
	}

	//Adjust value
	virtual CValue AdjustValue() const;
	virtual CValue AdjustValue(const CValue& varValue) const;

	//get type description 
	virtual CTypeDescription& GetTypeDesc() const = 0;
};

//////////////////////////////////////////////////////////////

enum eSelectorDataType {
	eSelectorDataType_any,
	eSelectorDataType_boolean,
	eSelectorDataType_reference,
	eSelectorDataType_table,
	eSelectorDataType_resource,
};

//////////////////////////////////////////////////////////////

class BACKEND_API IBackendTypeConfigFactory : public IBackendTypeFactory {
public:

	virtual eSelectorDataType GetFilterDataType() const {
		return eSelectorDataType::eSelectorDataType_reference;
	}

	//Create value by selected type
	virtual CValue CreateValue() const;
	virtual CValue* CreateValueRef() const;

	//Adjust value
	virtual CValue AdjustValue() const;
	virtual CValue AdjustValue(const CValue& varValue) const;

	//get metadata 
	virtual class IMetaData* GetMetaData() const = 0;
};

//////////////////////////////////////////////////////////////

enum eSourceDataType {
	eSourceDataVariant_table,
	eSourceDataVariant_tableColumn,
	eSourceDataVariant_attribute,
};

//////////////////////////////////////////////////////////////

class BACKEND_API IBackendTypeSourceFactory : public IBackendTypeConfigFactory {
public:

	virtual eSourceDataType GetFilterSourceDataType() const {
		return eSourceDataType::eSourceDataVariant_attribute;
	}

	//Get source object 
	virtual class ISourceObject* GetSourceObject() const = 0;

	// filter data 
	virtual bool FilterSource(const class CSourceExplorer& src, const meta_identifier_t& id);
};

#endif