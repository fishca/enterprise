#ifndef _VALUE_TYPE_H__
#define _VALUE_TYPE_H__

#include "backend/compiler/value.h"
#include "backend/backend_type.h"

class BACKEND_API CValueType : public CValue {
	wxDECLARE_DYNAMIC_CLASS(CValueType);
public:

	class_identifier_t GetOwnerTypeClass() const { return m_clsid; }
	CTypeDescription GetOwnerTypeDescription() const { return CTypeDescription(GetOwnerTypeClass()); }

	CValueType(const class_identifier_t& clsid = 0);
	CValueType(const CValue& cObject);
	CValueType(const CValueType& typeObject);

	CValueType(const wxString& typeName);

	virtual inline bool IsEmpty() const { return false; }

	virtual bool CompareValueEQ(const CValue& cParam) const {
		const CValueType* rValue = value_cast<CValueType>(cParam);
		wxASSERT(rValue);
		return m_clsid == rValue->m_clsid;
	}

	//operator '!='
	virtual bool CompareValueNE(const CValue& cParam) const {
		const CValueType* rValue = value_cast<CValueType>(cParam);
		wxASSERT(rValue);
		return m_clsid != rValue->m_clsid;
	}

	virtual wxString GetString() const;

private:
	class_identifier_t m_clsid;
};

class BACKEND_API CValueQualifierNumber : public CValue {
	wxDECLARE_DYNAMIC_CLASS(CValueQualifierNumber);
public:
	CQualifierNumber m_qNumber;
public:

	CValueQualifierNumber() : CValue(eValueTypes::TYPE_VALUE, true) {}
	CValueQualifierNumber(unsigned char precision, unsigned char scale) : CValue(eValueTypes::TYPE_VALUE, true),
		m_qNumber(precision, scale)
	{
	}

	operator CQualifierNumber() const { return m_qNumber; }
};

class BACKEND_API CValueQualifierDate : public CValue {
	wxDECLARE_DYNAMIC_CLASS(CValueQualifierDate);
public:
	CQualifierDate m_qDate;
public:

	CValueQualifierDate() : CValue(eValueTypes::TYPE_VALUE, true) {}
	CValueQualifierDate(eDateFractions dateTime) : CValue(eValueTypes::TYPE_VALUE, true),
		m_qDate(dateTime)
	{
	}

	operator CQualifierDate() const { return m_qDate; }
};

class BACKEND_API CValueQualifierString : public CValue {
	wxDECLARE_DYNAMIC_CLASS(CValueQualifierString);
public:
	CQualifierString m_qString;
public:

	CValueQualifierString() : CValue(eValueTypes::TYPE_VALUE, true) {}
	CValueQualifierString(unsigned short length) : CValue(eValueTypes::TYPE_VALUE, true),
		m_qString(length)
	{
	}

	operator CQualifierString() const { return m_qString; }
};

class BACKEND_API CValueTypeDescription : public CValue {
	wxDECLARE_DYNAMIC_CLASS_NO_COPY(CValueTypeDescription);
private:
	CMethodHelper* m_methodHelper;
public:
	CTypeDescription m_typeDesc;
public:

	// these methods need to be overridden in your aggregate objects:
	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}
	virtual void PrepareNames() const;
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);

public:

	static CValue AdjustValue(const CTypeDescription& typeDescription,
		class IMetaData* metaData = nullptr);

	static CValue AdjustValue(const CTypeDescription& typeDescription, const CValue& varValue,
		class IMetaData* metaData = nullptr);

	CValueTypeDescription();

	CValueTypeDescription(class CValueType* valueType);
	CValueTypeDescription(class CValueType* valueType,
		CValueQualifierNumber* qNumber, CValueQualifierDate* qDate, CValueQualifierString* qString);

	CValueTypeDescription(const CTypeDescription& typeDescription);

	CValueTypeDescription(const std::set<class_identifier_t>& clsid);
	CValueTypeDescription(const std::set<class_identifier_t>& clsid,
		CValueQualifierNumber* qNumber, CValueQualifierDate* qDate, CValueQualifierString* qString);

	virtual ~CValueTypeDescription();

	virtual bool Init() { return false; }
	virtual bool Init(CValue** paParams, const long lSizeArray);

public:

	operator CTypeDescription() const { return GetTypeDesc(); }

	std::set<class_identifier_t> GetClsidList() { return m_typeDesc.m_listTypeClass; }
	const CTypeDescription& GetTypeDesc() const { return m_typeDesc; }

public:

	bool ContainType(const CValue& cType) const;
	CValue AdjustValue() const;
	CValue AdjustValue(const CValue& varValue) const;
	CValue Types() const;
};

#endif