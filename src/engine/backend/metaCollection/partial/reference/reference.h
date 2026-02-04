#ifndef _REFERENCE_H__
#define _REFERENCE_H__

#include "backend/compiler/value.h"
#include "backend/valueInfo.h"

//********************************************************************************************
//*                                     Defines                                              *
//********************************************************************************************

class BACKEND_API IMetaData;

//********************************************************************************************

class BACKEND_API IValueMetaObjectRecordDataRef;
class BACKEND_API IValueRecordDataObjectRef;

//********************************************************************************************

class BACKEND_API CValueReferenceDataObject : public CValue,
	public IValueDataObject {
	wxDECLARE_DYNAMIC_CLASS(CValueReferenceDataObject);
private:
	enum helperAlias {
		eProperty,
		eTable
	};
private:
	CValueReferenceDataObject() : CValue(eValueTypes::TYPE_VALUE, true), m_initializedRef(false) {}
	CValueReferenceDataObject(IValueMetaObjectRecordDataRef* metaObject, const CGuid& objGuid = wxNullGuid);
public:

	reference_t* GetReferenceData() const {
		return m_reference_impl;
	}

	void PrepareRef(bool createData = true);

	virtual ~CValueReferenceDataObject();

	static CValueReferenceDataObject* Create(IMetaData* metaData, const meta_identifier_t& id, const CGuid& objGuid = wxNullGuid);
	static CValueReferenceDataObject* Create(IValueMetaObjectRecordDataRef* metaObject, const CGuid& objGuid = wxNullGuid);

	static CValueReferenceDataObject* Create(IMetaData* metaData, void* ptr);
	static CValueReferenceDataObject* CreateFromPtr(IMetaData* metaData, void* ptr);
	static CValueReferenceDataObject* CreateFromResultSet(class IDatabaseResultSet *rs, IValueMetaObjectRecordDataRef* metaObject, const CGuid& refGuid);

	//operator '>'
	virtual bool CompareValueGT(const CValue& cParam) const {
		CValueReferenceDataObject* rhs = dynamic_cast<CValueReferenceDataObject*>(cParam.GetRef());
		if (rhs != nullptr)
			return m_metaObject == rhs->m_metaObject && m_objGuid > rhs->m_objGuid;
		return false;
	}
	
	//operator '>='
	virtual bool CompareValueGE(const CValue& cParam) const {
		CValueReferenceDataObject* rhs = dynamic_cast<CValueReferenceDataObject*>(cParam.GetRef());
		if (rhs != nullptr)
			return m_metaObject == rhs->m_metaObject && m_objGuid >= rhs->m_objGuid;
		return false;
	}

	//operator '<'
	virtual bool CompareValueLS(const CValue& cParam) const {
		CValueReferenceDataObject* rhs = dynamic_cast<CValueReferenceDataObject*>(cParam.GetRef());
		if (rhs != nullptr)
			return m_metaObject == rhs->m_metaObject && m_objGuid < rhs->m_objGuid;
		return false;
	}

	//operator '<='
	virtual bool CompareValueLE(const CValue& cParam) const {
		CValueReferenceDataObject* rhs = dynamic_cast<CValueReferenceDataObject*>(cParam.GetRef());
		if (rhs != nullptr)
			return m_metaObject == rhs->m_metaObject && m_objGuid <= rhs->m_objGuid;
		return false;
	}

	//operator '=='
	virtual bool CompareValueEQ(const CValue& cParam) const {
		CValueReferenceDataObject* rhs = dynamic_cast<CValueReferenceDataObject*>(cParam.GetRef());
		if (rhs != nullptr)
			return m_metaObject == rhs->m_metaObject && m_objGuid == rhs->m_objGuid;
		return false;
	}

	//operator '!='
	virtual bool CompareValueNE(const CValue& cParam) const {
		CValueReferenceDataObject* rhs = dynamic_cast<CValueReferenceDataObject*>(cParam.GetRef());
		if (rhs != nullptr)
			return m_metaObject != rhs->m_metaObject || m_objGuid != rhs->m_objGuid;
		return true;
	}

	virtual bool FindValue(const wxString& findData, std::vector<CValue>& listValue) const;

	//support source set/get data 
	virtual bool SetValueByMetaID(const meta_identifier_t& id, const CValue& varMetaVal);
	virtual bool GetValueByMetaID(const meta_identifier_t& id, CValue& pvarMetaVal) const;

	//get metaData from object 
	virtual IValueMetaObjectRecordData* GetMetaObject() const {
		return (IValueMetaObjectRecordData *)m_metaObject;
	}

	//support show 
	virtual void ShowValue();

	//check is empty
	virtual bool IsEmpty() const {
		return !m_objGuid.isValid();
	}

	//****************************************************************************
	//*                              Reference methods                           *
	//****************************************************************************

	bool IsEmptyRef() const {
		return IsEmpty();
	}

	IValueMetaObjectRecordDataRef* GetMetaObjectRef() const {
		return m_metaObject;
	}

	IValueRecordDataObjectRef* GetObject() const;

	//****************************************************************************
	//*                              Support methods                             *
	//****************************************************************************

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}
	virtual void PrepareNames() const;

	//****************************************************************************
	//*                              Override attribute                          *
	//****************************************************************************
	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);       
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   

	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);

	//****************************************************************************
	//*                              Override type                               *
	//****************************************************************************

	//Get ref class 
	virtual class_identifier_t GetClassType() const;

	virtual wxString GetString() const;
	virtual wxString GetClassName() const;

	friend class CValue;

private:

	bool ReadData(bool createData = true);

protected:

	bool m_initializedRef;

	CMethodHelper* m_methodHelper;
	IValueMetaObjectRecordDataRef* m_metaObject;
	reference_t* m_reference_impl;

	bool m_foundedRef;
};

#endif 