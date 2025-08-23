#ifndef _REFERENCE_H__
#define _REFERENCE_H__

#include "backend/compiler/value.h"
#include "backend/valueInfo.h"

//********************************************************************************************
//*                                     Defines                                              *
//********************************************************************************************

class BACKEND_API IMetaData;

//********************************************************************************************

class BACKEND_API IMetaObjectRecordDataRef;
class BACKEND_API IRecordDataObjectRef;

//********************************************************************************************

class BACKEND_API CReferenceDataObject : public CValue,
	public IValueDataObject {
	wxDECLARE_DYNAMIC_CLASS(CReferenceDataObject);
private:
	enum helperAlias {
		eProperty,
		eTable
	};
private:
	CReferenceDataObject() : CValue(eValueTypes::TYPE_VALUE, true), m_initializedRef(false) {}
	CReferenceDataObject(IMetaObjectRecordDataRef* metaObject, const CGuid& objGuid = wxNullGuid);
public:

	reference_t* GetReferenceData() const {
		return m_reference_impl;
	}

	void PrepareRef(bool createData = true);

	virtual ~CReferenceDataObject();

	static CReferenceDataObject* Create(IMetaData* metaData, const meta_identifier_t& id, const CGuid& objGuid = wxNullGuid);
	static CReferenceDataObject* Create(IMetaObjectRecordDataRef* metaObject, const CGuid& objGuid = wxNullGuid);

	static CReferenceDataObject* Create(IMetaData* metaData, void* ptr);
	static CReferenceDataObject* CreateFromPtr(IMetaData* metaData, void* ptr);
	static CReferenceDataObject* CreateFromResultSet(class IDatabaseResultSet *rs, IMetaObjectRecordDataRef* metaObject, const CGuid& refGuid);

	//operator '>'
	virtual inline bool CompareValueGT(const CValue& cParam) const {
		CReferenceDataObject* rhs = dynamic_cast<CReferenceDataObject*>(cParam.GetRef());
		if (rhs != nullptr)
			return m_metaObject == rhs->m_metaObject && m_objGuid > rhs->m_objGuid;
		return false;
	}
	
	//operator '>='
	virtual inline bool CompareValueGE(const CValue& cParam) const {
		CReferenceDataObject* rhs = dynamic_cast<CReferenceDataObject*>(cParam.GetRef());
		if (rhs != nullptr)
			return m_metaObject == rhs->m_metaObject && m_objGuid >= rhs->m_objGuid;
		return false;
	}

	//operator '<'
	virtual inline bool CompareValueLS(const CValue& cParam) const {
		CReferenceDataObject* rhs = dynamic_cast<CReferenceDataObject*>(cParam.GetRef());
		if (rhs != nullptr)
			return m_metaObject == rhs->m_metaObject && m_objGuid < rhs->m_objGuid;
		return false;
	}

	//operator '<='
	virtual inline bool CompareValueLE(const CValue& cParam) const {
		CReferenceDataObject* rhs = dynamic_cast<CReferenceDataObject*>(cParam.GetRef());
		if (rhs != nullptr)
			return m_metaObject == rhs->m_metaObject && m_objGuid <= rhs->m_objGuid;
		return false;
	}

	//operator '=='
	virtual inline bool CompareValueEQ(const CValue& cParam) const {
		CReferenceDataObject* rhs = dynamic_cast<CReferenceDataObject*>(cParam.GetRef());
		if (rhs != nullptr)
			return m_metaObject == rhs->m_metaObject && m_objGuid == rhs->m_objGuid;
		return false;
	}

	//operator '!='
	virtual inline bool CompareValueNE(const CValue& cParam) const {
		CReferenceDataObject* rhs = dynamic_cast<CReferenceDataObject*>(cParam.GetRef());
		if (rhs != nullptr)
			return m_metaObject != rhs->m_metaObject || m_objGuid != rhs->m_objGuid;
		return true;
	}

	virtual bool FindValue(const wxString& findData, std::vector<CValue>& listValue) const;

	//support source set/get data 
	virtual bool SetValueByMetaID(const meta_identifier_t& id, const CValue& varMetaVal);
	virtual bool GetValueByMetaID(const meta_identifier_t& id, CValue& pvarMetaVal) const;

	//get metaData from object 
	virtual IMetaObjectRecordData* GetMetaObject() const {
		return (IMetaObjectRecordData *)m_metaObject;
	}

	//support show 
	virtual void ShowValue();

	//check is empty
	virtual inline bool IsEmpty() const {
		return !m_objGuid.isValid();
	}

	//****************************************************************************
	//*                              Reference methods                           *
	//****************************************************************************

	bool IsEmptyRef() const {
		return IsEmpty();
	}

	IMetaObjectRecordDataRef* GetMetaObjectRef() const {
		return m_metaObject;
	}

	IRecordDataObjectRef* GetObject() const;

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

	friend class CMetaValueRefTypeCtor;
	friend class IRecordDataObjectRef;

private:

	bool ReadData(bool createData = true);

protected:

	bool m_initializedRef;

	CMethodHelper* m_methodHelper;
	IMetaObjectRecordDataRef* m_metaObject;
	reference_t* m_reference_impl;

	bool m_foundedRef;
};

#endif 