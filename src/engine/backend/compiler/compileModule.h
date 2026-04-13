#ifndef _COMPILE_MODULE_H__
#define _COMPILE_MODULE_H__

#include "backend/backend.h"
#include "backend/compiler/compileCode.h"

//////////////////////////////////////////////////////////////////////
class BACKEND_API ibValueMetaObjectModuleBase;
//////////////////////////////////////////////////////////////////////

class BACKEND_API ibCompileModule : public ibCompileCode {
public:
	virtual bool Recompile(); // recompile current module from meta-object
	virtual bool Compile(); // compile current module from meta-object
public:

	ibCompileModule(const ibValueMetaObjectModuleBase* moduleObject, bool onlyFunction = false);
	virtual ~ibCompileModule() {}

	virtual ibCompileModule* GetParent() const { return dynamic_cast<ibCompileModule*>(m_parent); }
	virtual const ibValueMetaObjectModuleBase* GetModuleObject() const { return m_moduleObject; }

protected:
	const ibValueMetaObjectModuleBase* m_moduleObject;
};

class BACKEND_API ibCompileCommonModule : public ibCompileModule {
public:
	ibCompileCommonModule(const ibValueMetaObjectModuleBase* moduleObject) :
		ibCompileModule(moduleObject, true) {
	}
};

class BACKEND_API ibCompileGlobalModule : public ibCompileCommonModule {
public:
	ibCompileGlobalModule(const ibValueMetaObjectModuleBase* moduleObject) :
		ibCompileCommonModule(moduleObject) {
	}
};

#endif