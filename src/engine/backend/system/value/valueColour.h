#ifndef __VALUE_COLOUR_H__
#define __VALUE_COLOUR_H__

#include "backend/compiler/value.h"

//Array support
class BACKEND_API CValueColour : public CValue
{
	wxDECLARE_DYNAMIC_CLASS(CValueColour);

public:

	wxColour m_colour;

public:

	CValueColour();
	CValueColour(const wxColour& colour);
	virtual ~CValueColour() {}

	virtual bool Init(CValue** paParams, const long lSizeArray);
	virtual wxString GetString() const {
		return typeConv::ColourToString(m_colour);
	}

	//check is empty
	virtual bool IsEmpty() const {
		return !m_colour.IsOk();
	}

	static CMethodHelper m_methodHelper;

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

	virtual CMethodHelper* GetPMethods() const {
		//PrepareNames();
		return &m_methodHelper;
	}
	
	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.

	operator wxColour() const { return m_colour; }
};

#endif