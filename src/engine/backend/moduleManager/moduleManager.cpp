////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : module manager for common modules and compile value (in designer mode)
////////////////////////////////////////////////////////////////////////////

#include "moduleManager.h"
#include "backend/metaCollection/partial/contextManager.h"

#include "backend/appData.h"

#define objectManager wxT("manager")
#define objectMetadataManager wxT("metadata")

//*********************************************************************************************************
//*                                   Singleton class "moduleManager"                                     *
//*********************************************************************************************************

#include "backend/metadataConfiguration.h"

IModuleManager::IModuleManager(IMetaData* metadata, CMetaObjectModule* obj) :
	CValue(eValueTypes::TYPE_VALUE), IModuleDataObject(new CCompileModule(obj)),
	m_objectManager(new CContextSystemManager(metadata)),
	m_metaManager(new CMetadataUnit(metadata)),
	m_methodHelper(new CMethodHelper()),
	m_initialized(false)
{
	//add global variables 
	m_listGlConstValue.insert_or_assign(objectMetadataManager, m_metaManager);
}

void IModuleManager::Clear()
{
	//clear compile table 
	m_listCommonModuleValue.clear();
	m_listCommonModuleManager.clear();
}

IModuleManager::~IModuleManager()
{
	Clear();
	wxDELETE(m_methodHelper);
}

//*************************************************************************************************************************
//************************************************  support compile module ************************************************
//*************************************************************************************************************************

bool IModuleManager::AddCompileModule(const IMetaObject* mobj, CValue* object)
{
	if (!appData->DesignerMode() || !object)
		return true;
	auto iterator = m_listCommonModuleValue.find(mobj);
	if (iterator == m_listCommonModuleValue.end()) {
		m_listCommonModuleValue.emplace(mobj, object);
		return true;
	}
	return false;
}

bool IModuleManager::RemoveCompileModule(const IMetaObject* obj)
{
	if (!appData->DesignerMode())
		return true;
	auto iterator = m_listCommonModuleValue.find(obj);
	if (iterator != m_listCommonModuleValue.end()) {
		m_listCommonModuleValue.erase(iterator);
		return true;
	}
	return false;
}

bool IModuleManager::AddCommonModule(CMetaObjectCommonModule* commonModule, bool managerModule, bool runModule)
{
	CValuePtr<CModuleUnit> moduleValue(new CModuleUnit(this, commonModule, managerModule));

	if (!IModuleManager::AddCompileModule(commonModule, moduleValue))
		return false;

	m_listCommonModuleManager.emplace_back(moduleValue);

	if (!commonModule->IsGlobalModule()) {
		const wxString& strModuleName = commonModule->GetName();
		m_listGlConstValue.insert_or_assign(strModuleName, moduleValue);
		m_compileModule->AddVariable(strModuleName, moduleValue);
	}
	else {
		const wxString& strModuleName = commonModule->GetName();
		m_compileModule->RemoveVariable(strModuleName);
		m_compileModule->AppendModule(moduleValue->GetCompileModule());
	}

	if (runModule) {
		if (!commonModule->IsGlobalModule()) {
			try {
				m_compileModule->Compile();
			}
			catch (const CBackendException*) {
			};
		}
		return moduleValue->CreateCommonModule();
	}

	return true;
}

IModuleManager::CModuleUnit* IModuleManager::FindCommonModule(CMetaObjectCommonModule* commonModule) const
{
	auto moduleObjectIt = std::find_if(m_listCommonModuleManager.begin(), m_listCommonModuleManager.end(),
		[commonModule](CModuleUnit* valueModule) {
			return commonModule == valueModule->GetModuleObject();
		}
	);

	if (moduleObjectIt != m_listCommonModuleManager.end())
		return *moduleObjectIt;

	return nullptr;
}

bool IModuleManager::RenameCommonModule(CMetaObjectCommonModule* commonModule, const wxString& newName)
{
	CValue* moduleValue = FindCommonModule(commonModule);
	wxASSERT(moduleValue);

	if (!commonModule->IsGlobalModule()) {
		try {
			m_compileModule->AddVariable(newName, moduleValue);
			m_compileModule->RemoveVariable(commonModule->GetName());
			m_compileModule->Compile();
		}
		catch (const CBackendException*) {
		};

		m_listGlConstValue.insert_or_assign(newName, moduleValue);
		m_listGlConstValue.erase(commonModule->GetName());
	}

	return true;
}

bool IModuleManager::RemoveCommonModule(CMetaObjectCommonModule* commonModule)
{
	CValuePtr<IModuleManager::CModuleUnit> moduleValue(FindCommonModule(commonModule));
	wxASSERT(moduleValue);

	if (!IModuleManager::RemoveCompileModule(commonModule))
		return false;

	auto iterator = std::find(m_listCommonModuleManager.begin(), m_listCommonModuleManager.end(), moduleValue);

	if (iterator == m_listCommonModuleManager.end())
		return false;

	if (!commonModule->IsGlobalModule()) {
		m_listGlConstValue.erase(commonModule->GetName());
	}

	m_listCommonModuleManager.erase(iterator);

	if (commonModule->IsGlobalModule()) {
		m_compileModule->RemoveModule(moduleValue->GetCompileModule());
	}

	return true;
}

void IModuleManager::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	if (m_procUnit != nullptr) {
		CByteCode* byteCode = m_procUnit->GetByteCode();
		if (byteCode != nullptr) {
			for (auto exportFunction : byteCode->m_listExportFunc) {
				m_methodHelper->AppendMethod(
					exportFunction.first,
					byteCode->GetNParams(exportFunction.second),
					byteCode->HasRetVal(exportFunction.second),
					exportFunction.second,
					eProcUnit
				);
			}
			for (auto exportVariable : byteCode->m_listExportVar) {
				m_methodHelper->AppendProp(
					exportVariable.first,
					exportVariable.second,
					eProcUnit
				);
			}
		}
	}

	m_objectManager->PrepareNames();
	m_metaManager->PrepareNames();

	for (auto& module : m_listCommonModuleManager) {
		module->PrepareNames();
	}
}

bool IModuleManager::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
{
	return IModuleDataObject::ExecuteProc(
		GetMethodName(lMethodNum), paParams, lSizeArray
	);
}

bool IModuleManager::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	return IModuleDataObject::ExecuteFunc(
		GetMethodName(lMethodNum), pvarRetValue, paParams, lSizeArray
	);
}

bool IModuleManager::SetPropVal(const long lPropNum, const CValue& varPropVal)        //setting attribute
{
	if (m_procUnit != nullptr)
		return m_procUnit->SetPropVal(lPropNum, varPropVal);
	return false;
}

bool IModuleManager::GetPropVal(const long lPropNum, CValue& pvarPropVal)                   //attribute value
{
	if (m_procUnit != nullptr)
		return m_procUnit->GetPropVal(lPropNum, pvarPropVal);
	return false;
}

long IModuleManager::FindProp(const wxString& strName) const
{
	if (m_procUnit != nullptr) {
		return m_procUnit->FindProp(strName);
	}

	return CValue::FindProp(strName);
}

//////////////////////////////////////////////////////////////////////////////////
//  CModuleManagerConfiguration
//////////////////////////////////////////////////////////////////////////////////

CModuleManagerConfiguration::CModuleManagerConfiguration(IMetaData* metadata, CMetaObjectConfiguration* metaObject)
	: IModuleManager(metadata, metaObject ? metaObject->GetModuleObject() : nullptr)
{
}

//main module - initialize
bool CModuleManagerConfiguration::CreateMainModule()
{
	if (m_initialized)
		return true;

	//Добавление глобальных констант
	for (auto variable : m_listGlConstValue) {
		m_compileModule->AddVariable(variable.first, variable.second);
	}

	//create singleton "manager"
	m_compileModule->AddContextVariable(objectManager, m_objectManager);

	for (auto ctor : CValue::GetListCtorsByType(eCtorObjectType_object_context)) {
		m_compileModule->AddContextVariable(ctor->GetClassName(), ctor->CreateObject());
	}

	//initialize procUnit
	wxDELETE(m_procUnit);

	if (!appData->DesignerMode()) {
		try {
			m_compileModule->Compile();

			m_procUnit = new CProcUnit;
			m_procUnit->Execute(m_compileModule->m_cByteCode);
		}
		catch (const CBackendException*) {
			return false;
		};
	}

	//Setup common modules
	for (auto& moduleValue : m_listCommonModuleManager) {
		if (!moduleValue->CreateCommonModule()) {
			return false;
		}
	}

	m_initialized = true;
	return true;
}

bool CModuleManagerConfiguration::DestroyMainModule()
{
	if (!m_initialized)
		return true;

	//Добавление глобальных констант
	for (auto& variable : m_listGlConstValue) {
		m_compileModule->RemoveVariable(variable.first);
	}

	//create singleton "manager"
	m_compileModule->RemoveVariable(objectManager);

	for (auto ctor : CValue::GetListCtorsByType(eCtorObjectType_object_context)) {
		m_compileModule->RemoveVariable(ctor->GetClassName());
	}

	//reset global module
	m_compileModule->Reset();
	wxDELETE(m_procUnit);

	//Setup common modules
	for (auto& moduleValue : m_listCommonModuleManager) {
		if (!moduleValue->DestroyCommonModule()) {
			if (!appData->DesignerMode())
				return false;
		}
	}

	m_initialized = false;
	return true;
}

//main module - initialize
bool CModuleManagerConfiguration::StartMainModule(bool force)
{
	if (force)
		return true;

	if (!m_initialized)
		return false;

	if (BeforeStart()) {
		OnStart(); return true;
	}

	return false;
}

//main module - destroy
bool CModuleManagerConfiguration::ExitMainModule(bool force)
{
	if (force)
		return true;

	if (!m_initialized)
		return false;

	if (BeforeExit()) {
		OnExit(); /*m_initialized = false;*/ return true;
	}

	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

SYSTEM_TYPE_REGISTER(CModuleManagerConfiguration, "configModuleManager", string_to_clsid("SO_COMM"));

SYSTEM_TYPE_REGISTER(IModuleManager::CModuleUnit, "moduleManager", string_to_clsid("SO_MODL"));
SYSTEM_TYPE_REGISTER(IModuleManager::CMetadataUnit, "metadata", string_to_clsid("SO_METD"));