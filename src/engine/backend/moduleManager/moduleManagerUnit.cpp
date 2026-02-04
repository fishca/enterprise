////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : module manager - common modules
////////////////////////////////////////////////////////////////////////////

#include "moduleManager.h"

#include "backend/system/systemManager.h"
#include "backend/appData.h"

wxIMPLEMENT_DYNAMIC_CLASS(IValueModuleManager::CValueModuleUnit, CValue);

IValueModuleManager::CValueModuleUnit::CValueModuleUnit(IValueModuleManager *moduleManager, IValueMetaObjectModule *moduleObject, bool managerModule) :
	CValue(eValueTypes::TYPE_VALUE, true), IModuleDataObject(new CCompileCommonModule(moduleObject)),
	m_methodHelper(new CMethodHelper()),
	m_moduleManager(moduleManager),
	m_moduleObject(moduleObject)
{
}

IValueModuleManager::CValueModuleUnit::~CValueModuleUnit()
{
	wxDELETE(m_methodHelper);
}

#define objectManager wxT("manager")

//common module 
bool IValueModuleManager::CValueModuleUnit::CreateCommonModule()
{
	wxASSERT(m_moduleManager != nullptr);

	m_compileModule->SetParent(m_moduleManager->GetCompileModule());
	//create singleton "manager"
	m_compileModule->AddContextVariable(objectManager, m_moduleManager->GetObjectManager());

	wxDELETE(m_procUnit);

	if (!appData->DesignerMode()) {
		try {
			m_compileModule->Compile();
			// у глобального модуля код исполняется в главном модуле! 
			if (!CValueModuleUnit::IsGlobalModule()) {
				m_procUnit = new CProcUnit();
				m_procUnit->SetParent(m_moduleManager->GetProcUnit());
				m_procUnit->Execute(m_compileModule->m_cByteCode, false);
			}
		}
		catch (const CBackendException *){
			return false;
		};
	}

	CValueModuleUnit::PrepareNames();
	return true;
}

bool IValueModuleManager::CValueModuleUnit::DestroyCommonModule()
{
	wxASSERT(m_moduleManager != nullptr);

	m_compileModule->RemoveVariable(objectManager);
	m_compileModule->Reset();

	wxDELETE(m_procUnit);
	return true;
}

void IValueModuleManager::CValueModuleUnit::PrepareNames() const
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
		}
	}
}

bool IValueModuleManager::CValueModuleUnit::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
{
	return IModuleDataObject::ExecuteProc(
		GetMethodName(lMethodNum), paParams, lSizeArray
	);
}

bool IValueModuleManager::CValueModuleUnit::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	return IModuleDataObject::ExecuteFunc(
		GetMethodName(lMethodNum), pvarRetValue, paParams, lSizeArray
	);
}