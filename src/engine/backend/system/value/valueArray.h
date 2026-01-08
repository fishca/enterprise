#ifndef __VALUE_ARRAY_H__
#define __VALUE_ARRAY_H__

#include "backend/compiler/value.h"

//Array support
class BACKEND_API CValueArray : public CValue {
	wxDECLARE_DYNAMIC_CLASS(CValueArray);
private:
	std::vector <CValue> m_listValue;
private:

	enum Func {
		enAdd = 0,
		enInsert,
		enCount,
		enFind,
		enClear,
		enGet,
		enSet,
		enRemove
	};

	inline void CheckIndex(unsigned int index) const;

public:

	CValueArray() :
		CValue(eValueTypes::TYPE_VALUE) {
	}

	CValueArray(const std::vector <CValue>& arr) :
		CValue(eValueTypes::TYPE_VALUE, true), m_listValue(arr) {
	}

	virtual ~CValueArray() {
		Clear();
	}

	virtual bool Init();
	virtual bool Init(CValue** paParams, const long lSizeArray);

	//check is empty
	virtual bool IsEmpty() const {
		return m_listValue.empty();
	}

public:

	//Attribute -> String key
	//working with array as an aggregate object
	static CMethodHelper m_methodHelper;

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		return &m_methodHelper;
	}
	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);       //method call

	//Расширенные методы
	void Add(const CValue& varValue) {
		m_listValue.push_back(varValue);
	}

	void Insert(unsigned int index, const CValue& varValue) {
		CheckIndex(index);
		m_listValue.insert(m_listValue.begin() + index, varValue);
	}

	unsigned int Count() const {
		return m_listValue.size();
	}

	CValue Find(const CValue& varValue) {
		auto& it = std::find(m_listValue.begin(), m_listValue.end(), varValue);
		if (it != m_listValue.end())
			return std::distance(m_listValue.begin(), it);
		return CValue();
	}

	void Remove(unsigned int index) {
		CheckIndex(index);
		auto& it = std::find(m_listValue.begin(), m_listValue.end(), index);
		if (it != m_listValue.end())
			m_listValue.erase(it);
	}

	void Clear() {
		m_listValue.clear();
	}

	//array support 
	virtual bool SetAt(const CValue& varKeyValue, const CValue& varValue);
	virtual bool GetAt(const CValue& varKeyValue, CValue& pvarValue);

	//Working with iterators
	virtual bool HasIterator() const {
		return true;
	}
	virtual CValue GetIteratorAt(unsigned int idx) {
		return m_listValue[idx];
	}
	virtual unsigned int GetIteratorCount() const {
		return Count();
	}
};

#endif