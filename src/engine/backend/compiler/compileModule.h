#ifndef _COMPILE_MODULE_H__
#define _COMPILE_MODULE_H__

#include "backend/backend.h"
#include "backend/compiler/compileCode.h"

//////////////////////////////////////////////////////////////////////
class BACKEND_API IValueMetaObjectModule;
//////////////////////////////////////////////////////////////////////

class BACKEND_API CCompileModule : public CCompileCode {
public:
	virtual bool Recompile(); // recompile current module from meta-object
	virtual bool Compile(); // compile current module from meta-object
public:

	CCompileModule(const IValueMetaObjectModule* moduleObject, bool onlyFunction = false);
	virtual ~CCompileModule() {}

	virtual CCompileModule* GetParent() const { return dynamic_cast<CCompileModule*>(m_parent); }
	virtual const IValueMetaObjectModule* GetModuleObject() const { return m_moduleObject; }

protected:
	const IValueMetaObjectModule* m_moduleObject;
};

class BACKEND_API CCompileCommonModule : public CCompileModule {
public:
	CCompileCommonModule(const IValueMetaObjectModule* moduleObject) :
		CCompileModule(moduleObject, true) {
	}
};

class BACKEND_API CCompileGlobalModule : public CCompileCommonModule {
public:
	CCompileGlobalModule(const IValueMetaObjectModule* moduleObject) :
		CCompileCommonModule(moduleObject) {
	}
};

#endif