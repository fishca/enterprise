#ifndef __MODULE_INFO_H__
#define __MODULE_INFO_H__

#include "backend/compiler/compileModule.h"
#include "backend/compiler/procUnit.h"

class BACKEND_API IModuleDataObject {
public:

	//method call
	bool ExecuteProc(const wxString& strMethodName,
		CValue** paParams, const long lSizeArray)
	{
		if (m_procUnit != nullptr)
			return m_procUnit->CallAsProc(strMethodName, paParams, lSizeArray);
		return false;
	}

	bool ExecuteFunc(const wxString& strMethodName,
		CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
	{
		if (m_procUnit != nullptr)
			return m_procUnit->CallAsFunc(strMethodName, pvarRetValue, paParams, lSizeArray);
		return false;
	}

	IModuleDataObject();
	IModuleDataObject(CCompileModule* compileCode);
	virtual ~IModuleDataObject();

	const IMetaObjectModule* GetMetaObject() const {
		return GetCompileModule() ? GetCompileModule()->GetModuleObject() : nullptr;
	}

	virtual CCompileModule* GetCompileModule() const { return m_compileModule; }
	virtual CProcUnit* GetProcUnit() const { return m_procUnit; }

protected:
	CCompileModule* m_compileModule;
	CProcUnit* m_procUnit;
};

#endif 