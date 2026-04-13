#ifndef __VALUE_FILE_H__
#define __VALUE_FILE_H__

#include "backend/compiler/value.h"

class BACKEND_API CValueFile : public CValue {
	wxDECLARE_DYNAMIC_CLASS(CValueFile);
private:

	enum Prop {
		enBaseName,
		enExtension,
		enFullName,
		enName,
		enPath,
	};

	enum Func {
		enExist,
		//enGetHidden,
		enGetModificationTime,
		enGetReadOnly,
		enIsDirectory,
		enIsFile,
		//enSetHidden,
		//enSetModificationTime,
		//enSetReadOnly,
		enSize,
	};

public:

	// these methods need to be overridden in your aggregate objects:
	virtual CMethodHelper* GetPMethods() const { 
		//PrepareNames();
		return &m_methodHelper; 
	}

	virtual void PrepareNames() const;// this method is automatically called to initialize attribute and method names.
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);//method call

	virtual bool SetPropVal(const long lPropNum, const CValue &varValue);//setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);//attribute value

	CValueFile();
	virtual ~CValueFile();

	virtual bool IsEmpty() const { return false; }

	virtual bool Init() { return false; }
	virtual bool Init(CValue **paParams, const long lSizeArray);

private:
	wxString m_fileName; 
	static CMethodHelper m_methodHelper;
};

#endif 