#ifndef __VALUE_SQL_H__
#define __VALUE_SQL_H__

#include "backend/compiler/value.h"

class BACKEND_API CValueDatabaseLayer : public CValue {
	wxDECLARE_DYNAMIC_CLASS(CValueDatabaseLayer);
public:

	CValueDatabaseLayer();
	virtual ~CValueDatabaseLayer();

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return &m_methodHelper;
	}

	virtual void PrepareNames() const; // this method is automatically called to initialize attribute and method names.

	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);       //function call
	virtual bool CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray);       //procudre call

private:
	static CMethodHelper m_methodHelper;
};

class BACKEND_API CValuePreparedStatement : public CValue {
	wxDECLARE_DYNAMIC_CLASS(CValuePreparedStatement);
public:

	CValuePreparedStatement(class IPreparedStatement* preparedStatement = nullptr);
	virtual ~CValuePreparedStatement();

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return &m_methodHelper;
	}

	virtual void PrepareNames() const; // this method is automatically called to initialize attribute and method names.

	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);       //function call
	virtual bool CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray);       //procudre call

private:
	class IPreparedStatement* m_preparedStatement;
	static CMethodHelper m_methodHelper;
};

class BACKEND_API CValueResultSet : public CValue {
	wxDECLARE_DYNAMIC_CLASS(CValueResultSet);
public:

	CValueResultSet(class IDatabaseResultSet* resultSet = nullptr);
	virtual ~CValueResultSet();

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}

	virtual void PrepareNames() const; // this method is automatically called to initialize attribute and method names.

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);       //function call
	virtual bool CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray);       //procudre call

private:
	class IDatabaseResultSet* m_resultSet;
	CMethodHelper* m_methodHelper;
};

#endif