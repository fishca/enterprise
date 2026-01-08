#ifndef __VALUE_SIZE_H__
#define __VALUE_SIZE_H__

#include "backend/compiler/value.h"

class BACKEND_API CValueSize : public CValue {
	wxDECLARE_DYNAMIC_CLASS(CValueSize);
public:

	wxSize m_size;

public:

	CValueSize();
	CValueSize(const wxSize& size);
	virtual ~CValueSize() {}

	virtual bool Init(CValue** paParams, const long lSizeArray);
	virtual wxString GetString() const {
		return typeConv::SizeToString(m_size);
	}

	virtual bool IsEmpty() const {
		return m_size == wxDefaultSize;
	}

	static CMethodHelper m_methodHelper;

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return &m_methodHelper;
	}
	virtual void PrepareNames() const; // this method is automatically called to initialize attribute and method names.

	operator wxSize() const {
		return m_size;
	}


};

#endif