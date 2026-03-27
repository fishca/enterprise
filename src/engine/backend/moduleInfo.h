#ifndef __MODULE_INFO_H__
#define __MODULE_INFO_H__

#include "backend/compiler/compileModule.h"
#include "backend/compiler/procUnit.h"

class BACKEND_API ibModuleDataObject {
public:

	//method call
	bool ExecuteProc(const wxString& strMethodName,
		ibValue** paParams, const long lSizeArray)
	{
		if (m_procUnit != nullptr)
			return m_procUnit->CallAsProc(strMethodName, paParams, lSizeArray);
		return false;
	}

	bool ExecuteFunc(const wxString& strMethodName,
		ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray)
	{
		if (m_procUnit != nullptr)
			return m_procUnit->CallAsFunc(strMethodName, pvarRetValue, paParams, lSizeArray);
		return false;
	}

	ibModuleDataObject();
	ibModuleDataObject(ibCompileModule* compileCode);
	virtual ~ibModuleDataObject();

	const ibValueMetaObjectModuleBase* GetMetaObject() const {
		return GetCompileModule() ? GetCompileModule()->GetModuleObject() : nullptr;
	}

	virtual ibCompileModule* GetCompileModule() const { return m_compileModule; }
	virtual ibProcUnit* GetProcUnit() const { return m_procUnit; }

protected:
	ibCompileModule* m_compileModule;
	ibProcUnit* m_procUnit;
};

#endif 