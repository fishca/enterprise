#ifndef __VALUE_FONT_H__
#define __VALUE_FONT_H__

#include "backend/compiler/value.h"

//Array support
class BACKEND_API CValueFont : public CValue
{
	wxDECLARE_DYNAMIC_CLASS(CValueFont);
public:
	wxFont m_font;
public:

	CValueFont();
	CValueFont(const wxFont& font);
	virtual ~CValueFont() {}

	virtual bool Init(CValue** paParams, const long lSizeArray);

	virtual wxString GetString() const {
		return typeConv::FontToString(m_font);
	}

	//check is empty
	virtual bool IsEmpty() const {
		return !m_font.IsOk();
	}

	static CMethodHelper m_methodHelper;

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

	virtual CMethodHelper* GetPMethods() const {
		//PrepareNames();
		return &m_methodHelper;
	}
	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.

	operator wxFont() {
		return m_font;
	}
};

#endif