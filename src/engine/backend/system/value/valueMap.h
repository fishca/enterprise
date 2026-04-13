#ifndef __VALUE_MAP_H__
#define __VALUE_MAP_H__

#include "backend/compiler/value.h"

class BACKEND_API CValueContainer : public CValue {
	wxDECLARE_DYNAMIC_CLASS(CValueContainer);
private:
	enum Func  {
		enCount = 0,
		enProperty,
		enClear,
		enDelete,
		enInsert
	};
public:

	//Attribute -> String key
	//working with an array as an aggregate object:

	virtual bool GetAt(const CValue& varKeyValue, CValue& pvarValue);
	virtual bool SetAt(const CValue& varKeyValue, const CValue& cValue);

	//check is empty
	virtual bool IsEmpty() const { 
		return m_containerValues.empty();
	}

public:

	class BACKEND_API CValueReturnContainer : public CValue {
		
		enum Prop {
			enKey,
			enValue
		};
		
		CValue m_key;
		CValue m_value;
	
		static CMethodHelper m_methodHelper;

	public:

		CValueReturnContainer() : CValue(eValueTypes::TYPE_VALUE, true) { 
			PrepareNames(); 
		}
		
		CValueReturnContainer(const CValue& key, CValue& value) : CValue(eValueTypes::TYPE_VALUE, true), m_key(key), m_value(value) { 
			PrepareNames();
		}

		virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
			//PrepareNames(); 
			return &m_methodHelper;
		}
		virtual void PrepareNames() const;

		virtual bool SetPropVal(const long lPropNum, CValue& cValue);        //setting attribute
		virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value
	};

public:

	CValueContainer();
	CValueContainer(const std::map<CValue, CValue>& containerValues);
	CValueContainer(bool readOnly);

	virtual ~CValueContainer();

	virtual bool SetPropVal(const long lPropNum, const CValue& cValue);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}
	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);       //method call

	//Расширенные методы:
	virtual void Insert(const CValue& varKeyValue, const CValue& cValue);
	virtual void Delete(const CValue& varKeyValue);
	virtual bool Property(const CValue& varKeyValue, CValue& cValueFound);
	unsigned int Count() const { return m_containerValues.size(); }
	void Clear() { m_containerValues.clear(); }

	//Работа с итераторами:
	virtual bool HasIterator() const { return true; }
	virtual CValue GetIteratorEmpty();
	virtual CValue GetIteratorAt(unsigned int idx);
	virtual unsigned int GetIteratorCount() const { return m_containerValues.size(); }

protected:

	CMethodHelper* m_methodHelper;

	struct ContainerComparator {
		bool operator()(const CValue& lhs, const CValue& rhs) const;
	};

	std::map<const CValue, CValue, ContainerComparator> m_containerValues;
};

// structure  
class BACKEND_API CValueStructure : public CValueContainer {
	wxDECLARE_DYNAMIC_CLASS(CValueStructure);
public:

	CValueStructure() : CValueContainer(false) {}
	CValueStructure(const std::map<wxString, CValue>& structureValues) : CValueContainer(true) {
		for (auto& strBVal : structureValues) m_containerValues.insert_or_assign(strBVal.first, strBVal.second); 
		PrepareNames();
	}

	CValueStructure(bool readOnly) : CValueContainer(readOnly) {}

	virtual void Delete(const CValue& varKeyValue) override;
	virtual void Insert(const CValue& varKeyValue, const CValue& cValue = CValue()) override;
	virtual bool Property(const CValue& varKeyValue, CValue& cValueFound) override;

	virtual bool GetAt(const CValue& varKeyValue, CValue& pvarValue);
	virtual bool SetAt(const CValue& varKeyValue, const CValue& cValue);
};

#include <locale>

#endif