#include "compileModule.h"
#include "backend/metaCollection/metaModuleObject.h"

#pragma warning(push)
#pragma warning(disable : 4018)

CCompileModule::CCompileModule(const IMetaObjectModule* moduleObject, bool onlyFunction) :
	CCompileCode(moduleObject->GetFullName(), moduleObject->GetDocPath(), onlyFunction),
	m_moduleObject(moduleObject)
{
	InitializeCompileModule();

	m_cByteCode.m_strModuleName = m_moduleObject->GetFullName();

	m_strModuleName = m_moduleObject->GetFullName();
	m_strDocPath = m_moduleObject->GetDocPath();
	m_strFileName = m_moduleObject->GetFileName();

	Load(m_moduleObject->GetModuleText());

	//We don’t look for local variables in parent contexts!
	m_rootContext->m_numFindLocalInParent = 0;
}

/**
 * Compile
 * Purpose:
 * Translation and compilation of source code into bytecode (object code)
 * Return value:
 * true,false
 */

bool CCompileModule::Compile()
{
	//clear functions & variables 
	Reset();

	if (m_parent != nullptr) {
		
		if (m_moduleObject != nullptr &&
			m_moduleObject->IsGlobalModule()) {

			m_strModuleName = m_moduleObject->GetFullName();
			m_strDocPath = m_moduleObject->GetDocPath();
			m_strFileName = m_moduleObject->GetFileName();

			m_changedCode = false;

			Load(m_moduleObject->GetModuleText());

			return m_parent != nullptr ?
				m_parent->Compile() : true;
		}
	}

	//recursively compile modules in case of any changes
	if (m_parent != nullptr) {

		std::stack<CCompileModule*> compileModule; bool callRecompile = false;

		CCompileModule* parentModule = GetParent();

		while (parentModule != nullptr) {
			if (parentModule->m_changedCode) callRecompile = true;
			if (callRecompile) compileModule.push(parentModule);
			parentModule = parentModule->GetParent();
		}

		while (!compileModule.empty()) {
			CCompileModule* compileCode = compileModule.top();
			if (!compileCode->Recompile()) return false;
			compileModule.pop();
		}
	}

	if (m_moduleObject != nullptr) {

		m_cByteCode.m_strModuleName = m_moduleObject->GetFullName();

		if (m_parent != nullptr) {
			m_cByteCode.m_parent = &m_parent->m_cByteCode;
			m_rootContext->m_parentContext = m_parent->m_rootContext;
		}

		m_strModuleName = m_moduleObject->GetFullName();
		m_strDocPath = m_moduleObject->GetDocPath();
		m_strFileName = m_moduleObject->GetFileName();

		Load(m_moduleObject->GetModuleText());
	}

	//prepare lexem 
	if (!PrepareLexem()) {
		return false;
	}

	//prepare context variables
	PrepareModuleData();

	// compilation 
	if (CompileModule()) {
		m_changedCode = false;
		return true;
	}

	return false;
}

/**
 * Recompile
 * Purpose:
 * Translation and recompilation of the current source code into bytecode (object code)
 * Return value:
 * true,false
 */

bool CCompileModule::Recompile()
{
	//clear functions & variables 
	Reset();

	if (m_parent != nullptr) {
		
		if (m_moduleObject != nullptr &&
			m_moduleObject->IsGlobalModule()) {

			m_strModuleName = m_moduleObject->GetFullName();
			m_strDocPath = m_moduleObject->GetDocPath();
			m_strFileName = m_moduleObject->GetFileName();

			m_changedCode = false;

			Load(m_moduleObject->GetModuleText());

			return m_parent != nullptr ?
				m_parent->Compile() : true;
		}
	}

	if (m_moduleObject != nullptr) {

		m_cByteCode.m_strModuleName = m_moduleObject->GetFullName();

		if (m_parent) {
			m_cByteCode.m_parent = &m_parent->m_cByteCode;
			m_rootContext->m_parentContext = m_parent->m_rootContext;
		}

		m_strModuleName = m_moduleObject->GetFullName();
		m_strDocPath = m_moduleObject->GetDocPath();
		m_strFileName = m_moduleObject->GetFileName();

		Load(m_moduleObject->GetModuleText());
	}

	//prepare lexem 
	if (!PrepareLexem()) {
		return false;
	}

	//prepare context variables
	PrepareModuleData();

	// compilation 
	if (CompileModule()) {
		m_changedCode = false;
		return true;
	}

	m_changedCode = true;
	return false;
}

#pragma warning(pop)