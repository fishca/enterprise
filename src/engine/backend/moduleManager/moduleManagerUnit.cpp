////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : module manager - common modules
////////////////////////////////////////////////////////////////////////////

#include "moduleManager.h"

#include "backend/system/systemManager.h"
#include "backend/appData.h"
#include "backend/session/session.h"

wxIMPLEMENT_DYNAMIC_CLASS(ibValueModuleManager::ibValueModuleUnit, ibValue);

ibValueModuleManager::ibValueModuleUnit::ibValueModuleUnit(ibValueModuleManager *moduleManager, ibValueMetaObjectModuleBase *moduleObject, bool managerModule) :
	ibValue(ibValueTypes::TYPE_VALUE, true), ibRuntimeModuleDataObject(new ibCompileCommonModule(moduleObject)),
	m_methodHelper(new ibValueMethodHelper()),
	m_moduleManager(moduleManager),
	m_moduleObject(moduleObject)
{
	// Parent chain in the runtime tree — common module sits directly
	// under its owning root module manager. Enables GetSession() walks
	// from nested scripts without relying on ambient ibSessionScope.
	SetParent(moduleManager);
}

ibValueModuleManager::ibValueModuleUnit::~ibValueModuleUnit()
{
	wxDELETE(m_methodHelper);
}

#define objectManager wxT("Manager")

//common module
bool ibValueModuleManager::ibValueModuleUnit::CreateCommonModule()
{
	wxASSERT(m_moduleManager != nullptr);

	// Parent already wired in ctor (SetParent(moduleManager)). Bind
	// the module-scope "Manager" singleton that scripts reach via
	// Manager.<method>() inside common modules.
	BindContextVariable(objectManager, m_moduleManager->GetObjectManager());

	// Compile only — per-session ProcUnit comes from AttachRuntime.
	try {
		Compile();
	}
	catch (const ibBackendException& err) {
		wxLogWarning(_("Common module init failed: %s"), err.GetErrorDescription());
		return false;
	};

	ibValueModuleUnit::PrepareNames();
	return true;
}

bool ibValueModuleManager::ibValueModuleUnit::DestroyCommonModule()
{
	wxASSERT(m_moduleManager != nullptr);

	m_compileModule->RemoveVariable(objectManager);
	m_compileModule->Reset();

	m_procUnit.reset();
	return true;
}

void ibValueModuleManager::ibValueModuleUnit::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	ExportNamesToHelper(m_methodHelper, eProcUnit);
}

bool ibValueModuleManager::ibValueModuleUnit::CallAsProc(const long lMethodNum, ibValue** paParams, const long lSizeArray)
{
	return ibRuntimeModuleDataObject::ExecAsProc(
		GetMethodName(lMethodNum), paParams, lSizeArray
	);
}

bool ibValueModuleManager::ibValueModuleUnit::CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray)
{
	return ibRuntimeModuleDataObject::ExecAsFunc(
		GetMethodName(lMethodNum), pvarRetValue, paParams, lSizeArray
	);
}