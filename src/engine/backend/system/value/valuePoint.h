#ifndef __VALUE_POINT_H__
#define __VALUE_POINT_H__

#include "backend/compiler/value.h"

class BACKEND_API CValuePoint : public CValue
{
	wxDECLARE_DYNAMIC_CLASS(CValuePoint);

public:

	wxPoint m_point;

public:

	CValuePoint();
	CValuePoint(const wxPoint& point);
	virtual ~CValuePoint() {}

	virtual bool Init(CValue** paParams, const long lSizeArray);
	virtual wxString GetString() const {
		return typeConv::PointToString(m_point); 
	}

	//check is empty
	virtual bool IsEmpty() const {
		return m_point == wxDefaultPosition;
	}

	static CMethodHelper m_methodHelper;

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return &m_methodHelper;
	}
	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.

	operator wxPoint() const { return m_point; }
};

#endif