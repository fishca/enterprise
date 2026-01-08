#ifndef _VALUE_GUID_H__
#define _VALUE_GUID_H__

#include "backend/compiler/value.h"

class BACKEND_API CValueGuid : public CValue {
	wxDECLARE_DYNAMIC_CLASS(CValueGuid);
public:

	operator CGuid() const {
		return m_guid;
	}

	CValueGuid();
	CValueGuid(const CGuid &guid);

	virtual bool Init();
	virtual bool Init(CValue **paParams, const long lSizeArray);

	virtual wxString GetString() const { 
		return m_guid; 
	}

	//check is empty
	virtual bool IsEmpty() const { 
		return !m_guid.isValid(); 
	}

	//operator '=='
	virtual bool CompareValueEQ(const CValue &cParam) const {
		return m_guid == cParam.GetString(); 
	}
	
	//operator '!='
	virtual bool CompareValueNE(const CValue &cParam) const { 
		return m_guid != cParam.GetString(); 
	}

private:
	CGuid m_guid;
};

#endif // !_VALUEUUID_H__
