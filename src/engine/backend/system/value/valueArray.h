#ifndef __VALUE_ARRAY_H__
#define __VALUE_ARRAY_H__

#include "backend/compiler/value.h"

//Array support
class BACKEND_API ibValueArray : public ibValue {
	wxDECLARE_DYNAMIC_CLASS(ibValueArray);
private:
	std::vector <ibValue> m_listValue;
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

	ibValueArray() :
		ibValue(ibValueTypes::TYPE_VALUE) {
	}

	ibValueArray(const std::vector <ibValue>& arr) :
		ibValue(ibValueTypes::TYPE_VALUE, true), m_listValue(arr) {
	}

	virtual ~ibValueArray() {
		Clear();
	}

	virtual bool Init();
	virtual bool Init(ibValue** paParams, const long lSizeArray);

	//check is empty
	virtual bool IsEmpty() const {
		return m_listValue.empty();
	}

public:

	//Attribute -> String key
	//working with array as an aggregate object
	static ibValueMethodHelper m_methodHelper;

	virtual ibValueMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		return &m_methodHelper;
	}
	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray);       //method call

	//����������� ������
	void Add(const ibValue& varValue) {
		m_listValue.push_back(varValue);
	}

	void Insert(unsigned int index, const ibValue& varValue) {
		CheckIndex(index);
		m_listValue.insert(m_listValue.begin() + index, varValue);
	}

	unsigned int Count() const {
		return m_listValue.size();
	}

	ibValue Find(const ibValue& varValue) {
		auto it = std::find(m_listValue.begin(), m_listValue.end(), varValue);
		if (it != m_listValue.end())
			return ibValue(static_cast<signed int>(std::distance(m_listValue.begin(), it)));
		return ibValue();
	}

	void Remove(unsigned int index) {
		CheckIndex(index);
		auto it = std::find(m_listValue.begin(), m_listValue.end(), index);
		if (it != m_listValue.end())
			m_listValue.erase(it);
	}

	void Clear() {
		m_listValue.clear();
	}

	//array support
	virtual bool SetAt(const ibValue& varKeyValue, const ibValue& varValue);
	virtual bool GetAt(const ibValue& varKeyValue, ibValue& pvarValue);

	//Working with iterators
	virtual std::shared_ptr<ibValueIteratorState> CreateIterator() override {
		class ArrayIteratorState : public ibValueIteratorState {
		public:
			explicit ArrayIteratorState(const std::vector<ibValue>& list) : m_list(list) {}
			bool MoveNext(ibValue& current) override {
				if (m_started) ++m_pos; else m_started = true;
				if (m_pos >= m_list.size()) return false;
				current = m_list[m_pos];
				return true;
			}
			void Reset() override { m_pos = 0; m_started = false; }
		private:
			const std::vector<ibValue>& m_list;
			size_t m_pos = 0;
			bool m_started = false;
		};
		return std::make_shared<ArrayIteratorState>(m_listValue);
	}
};

#endif