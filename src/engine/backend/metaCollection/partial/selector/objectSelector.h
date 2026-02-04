#ifndef _SELECTOR_H__
#define _SELECTOR_H__

#include "backend/metaCollection/partial/commonObject.h"

class BACKEND_API IValueSelectorDataObject : public CValue {
public:

	IValueSelectorDataObject();
	virtual ~IValueSelectorDataObject();

	virtual bool Next() = 0;

	//is empty
	virtual bool IsEmpty() const {
		return false;
	}

	//get metaData from object 
	virtual IValueMetaObjectGenericData* GetMetaObject() const = 0;

	//Get ref class 
	virtual class_identifier_t GetClassType() const;

	//types 
	virtual wxString GetClassName() const;
	virtual wxString GetString() const;

protected:

	virtual void Reset() = 0;
	virtual bool Read() = 0;

protected:
	CMethodHelper* m_methodHelper;
};

class BACKEND_API CValueSelectorRecordDataObject : public IValueSelectorDataObject,
	public IValueDataObject {
public:

	CValueSelectorRecordDataObject(IValueMetaObjectRecordDataMutableRef* metaObject);

	virtual bool Next();
	virtual IValueRecordDataObjectRef* GetObject(const CGuid& guid) const;

	//get metaData from object 
	virtual IValueMetaObjectRecordData* GetMetaObject() const {
		return m_metaObject;
	}

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}
	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);//method call

	//attribute
	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

protected:

	virtual void Reset();
	virtual bool Read();

protected:

	IValueMetaObjectRecordDataMutableRef* m_metaObject;
	std::vector<CGuid> m_currentValues;
};

/////////////////////////////////////////////////////////////////////////////

class BACKEND_API CValueSelectorRegisterDataObject :
	public IValueSelectorDataObject {
public:
	CValueSelectorRegisterDataObject(IValueMetaObjectRegisterData* metaObject);

	virtual bool Next();
	virtual IValueRecordManagerObject* GetRecordManager(const valueArray_t& keyValues) const;

	//get metaData from object 
	virtual IValueMetaObjectRegisterData* GetMetaObject() const {
		return m_metaObject;
	}

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}
	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);//method call

	//attribute
	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

protected:

	virtual void Reset();
	virtual bool Read();

protected:

	IValueMetaObjectRegisterData* m_metaObject;

	valueArray_t m_keyValues;

	std::vector <valueArray_t> m_currentValues;
	std::map<
		valueArray_t,
		valueArray_t
	> m_listObjectValue;
};

#endif