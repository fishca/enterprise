#ifndef __MODULE_INFO_H__
#define __MODULE_INFO_H__

#include "backend/compiler/compileModule.h"
#include "backend/compiler/procUnit.h"

class BACKEND_API IModuleDataObject {
public:

	const IMetaObjectModule* GetMetaObject() const { 
		return GetCompileModule() ? GetCompileModule()->GetModuleObject() : nullptr; 
	}

	//method call
	bool ExecuteProc(const wxString& strMethodName,
		CValue** paParams,
		const long lSizeArray);

	bool ExecuteFunc(const wxString& strMethodName,
		CValue& pvarRetValue,
		CValue** paParams,
		const long lSizeArray);

	IModuleDataObject();
	IModuleDataObject(CCompileModule* compileCode);
	virtual ~IModuleDataObject();

	virtual CCompileModule* GetCompileModule() const { return m_compileModule; }
	virtual CProcUnit* GetProcUnit() const { return m_procUnit; }

protected:
	CCompileModule* m_compileModule;
	CProcUnit* m_procUnit;
};

#endif 