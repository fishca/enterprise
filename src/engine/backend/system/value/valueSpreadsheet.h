#ifndef __VALUE_SPREADSHEET_DOC_H__
#define __VALUE_SPREADSHEET_DOC_H__

#include "backend/compiler/value.h"
#include "backend/backend_spreadsheet.h"

class BACKEND_API CValueSpreadsheet : public CValue {
	wxDECLARE_DYNAMIC_CLASS(CValueSpreadsheet);
public:

	const CSpreadsheetDescription& GetSpreadsheetDesc() const { return m_spreadsheetDesc; }

	CValueSpreadsheet(const CSpreadsheetDescription& spreadsheetDesc = CSpreadsheetDescription()) : CValue(eValueTypes::TYPE_VALUE), m_spreadsheetDesc(spreadsheetDesc) {}

	virtual bool IsEmpty() const { return false; }

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);       //function call
	virtual bool CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray);       //procudre call

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return &m_methodHelper;
	}

	virtual void PrepareNames() const; // this method is automatically called to initialize attribute and method names.

private:
	CSpreadsheetDescription m_spreadsheetDesc;
	static CMethodHelper m_methodHelper;
};
#endif 