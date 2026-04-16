
#include "moduleManagerExt.h"
#include "backend/metadataConfiguration.h"

#include "backend/appData.h"

//////////////////////////////////////////////////////////////////////////////////
#define thisObject wxT("ThisObject")
//////////////////////////////////////////////////////////////////////////////////
//  ibValueModuleManagerExternalDataProcessor
//////////////////////////////////////////////////////////////////////////////////

ibCompileModule* ibValueModuleManagerExternalDataProcessor::GetCompileModule() const
{
	ibValueModuleManager* moduleManager = activeMetaData->GetModuleManager();
	wxASSERT(moduleManager);
	return moduleManager->GetCompileModule();
}

ibProcUnit* ibValueModuleManagerExternalDataProcessor::GetProcUnit() const
{
	ibValueModuleManager* moduleManager = activeMetaData->GetModuleManager();
	wxASSERT(moduleManager);
	return moduleManager->GetProcUnit();
}

std::map<wxString, ibValue*>& ibValueModuleManagerExternalDataProcessor::GetContextVariables()
{
	ibValueModuleManager* moduleManager = activeMetaData->GetModuleManager();
	wxASSERT(moduleManager);
	return moduleManager->GetContextVariables();
}

ibValueModuleManagerExternalDataProcessor::ibValueModuleManagerExternalDataProcessor(ibMetaData* metadata, ibValueMetaObjectDataProcessor* metaObject)
	: ibValueModuleManager(ibMetaDataConfiguration::Get(), metaObject ? metaObject->GetModuleObject() : nullptr)
{
	m_objectValue = new ibValueRecordDataObjectDataProcessor(metaObject);
	//set complile module 
	//set proc unit 
	m_objectValue->m_compileModule = m_compileModule;
	m_objectValue->m_procUnit = m_procUnit;

	if (appData->DesignerMode()) {
		//incrRef
		m_objectValue->IncrRef();
	}
}

ibValueModuleManagerExternalDataProcessor::~ibValueModuleManagerExternalDataProcessor()
{
	if (appData->DesignerMode()) {
		//decrRef
		m_objectValue->DecrRef();
	}
}

bool ibValueModuleManagerExternalDataProcessor::CreateMainModule()
{
	if (m_initialized)
		return true;

	ibValueModuleManager* moduleManager = activeMetaData->GetModuleManager();
	wxASSERT(moduleManager);

	m_compileModule->SetParent(moduleManager->GetCompileModule());
	m_compileModule->AddContextVariable(thisObject, m_objectValue);

	//initialize procUnit
	wxDELETE(m_procUnit);

	//set complile module 
	m_objectValue->m_compileModule = m_compileModule;

	//initialize object 
	m_objectValue->InitializeObject();

	if (!appData->DesignerMode()) {

		m_procUnit = new ibProcUnit();
		m_procUnit->SetParent(moduleManager->GetProcUnit());

		//set proc unit 
		m_objectValue->m_procUnit = m_procUnit;

		try {
			m_compileModule->Compile();
			//and run...
			m_procUnit->Execute(m_compileModule->m_cByteCode);
		}
		catch (const ibBackendException&) {
			return false;
		};
	}

	//Setup common modules
	for (auto& moduleValue : m_listCommonModuleManager) {
		if (!moduleValue->CreateCommonModule()) {
			return false;
		}
	}

	//set initialized true
	m_initialized = true;
	return true;
}

bool ibValueModuleManagerExternalDataProcessor::DestroyMainModule()
{
	if (!m_initialized)
		return true;

	m_compileModule->Reset();
	wxDELETE(m_procUnit);

	//set complile module 
	//set proc unit
	m_objectValue->m_compileModule = nullptr;
	m_objectValue->m_procUnit = nullptr;

	//Setup common modules
	for (auto& moduleValue : m_listCommonModuleManager) {
		if (!moduleValue->DestroyCommonModule()) {
			return false;
		}
	}

	m_initialized = false;
	return true;
}

//main module - initialize
bool ibValueModuleManagerExternalDataProcessor::StartMainModule(bool force)
{
	if (!m_initialized)
		return false;

	//incrRef - for control delete 
	m_objectValue->IncrRef();

	ibValueMetaObjectRecordData* commonObject = m_objectValue->GetMetaObject();
	wxASSERT(commonObject);
	ibValueMetaObjectFormBase* defFormObject = commonObject->GetDefaultFormByID
	(
		ibValueMetaObjectDataProcessor::eFormDataProcessor
	);

	if (defFormObject != nullptr) {

		ibBackendValueForm* result = nullptr;

		if (!ibValueModuleManager::FindCompileModule(defFormObject, result)) {

			//valueForm = defFormObject->CreateForm(
			//	nullptr, m_objectValue
			//);
			//try {
			//	if (valueForm->InitializeFormModule()) {
			//		valueForm->ShowForm();
			//	}
			//}
			//catch (...) {
			//	wxDELETE(valueForm);
			//	if (!appData->DesignerMode()) {
			//		//decrRef - for control delete 
			//		m_objectValue->DecrRef();
			//		return false;
			//	}
			//}

			result = ibValueMetaObjectFormBase::CreateAndBuildForm(defFormObject, nullptr, m_objectValue);

			if (result != nullptr) {
				result->ShowForm();
			}
			else if (!appData->DesignerMode()) {
				//decrRef - for control delete 
				m_objectValue->DecrRef();
				return false;
			}

		}
	}
	//else {
	//	ibBackendValueForm* valueForm = ibBackendValueForm::CreateNewForm(nullptr, nullptr, m_objectValue, ibGuid::newGuid());
	//	valueForm->BuildForm(ibValueMetaObjectDataProcessor::eFormDataProcessor);
	//	try {
	//		valueForm->ShowForm();
	//	}
	//	catch (...) {
	//		wxDELETE(valueForm);
	//		if (appData->EnterpriseMode() ||
	//			appData->ServiceMode()) {
	//			//decrRef - for control delete 
	//			m_objectValue->DecrRef();
	//			return false;
	//		}
	//	}
	//}

	//decrRef - for control delete 
	m_objectValue->DecrRef();

	return true;
}

//main module - destroy
bool ibValueModuleManagerExternalDataProcessor::ExitMainModule(bool force)
{
	if (force)
		return true;

	if (!m_initialized)
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////
//  ibValueModuleManagerExternalReport
//////////////////////////////////////////////////////////////////////////////////

ibCompileModule* ibValueModuleManagerExternalReport::GetCompileModule() const
{
	ibValueModuleManager* moduleManager = activeMetaData->GetModuleManager();
	wxASSERT(moduleManager);
	return moduleManager->GetCompileModule();
}

ibProcUnit* ibValueModuleManagerExternalReport::GetProcUnit() const
{
	ibValueModuleManager* moduleManager = activeMetaData->GetModuleManager();
	wxASSERT(moduleManager);
	return moduleManager->GetProcUnit();
}

std::map<wxString, ibValue*>& ibValueModuleManagerExternalReport::GetContextVariables()
{
	ibValueModuleManager* moduleManager = activeMetaData->GetModuleManager();
	wxASSERT(moduleManager);
	return moduleManager->GetContextVariables();
}

ibValueModuleManagerExternalReport::ibValueModuleManagerExternalReport(ibMetaData* metadata, ibValueMetaObjectReport* metaObject)
	: ibValueModuleManager(ibMetaDataConfiguration::Get(), metaObject ? metaObject->GetModuleObject() : nullptr)
{
	m_objectValue = new ibValueRecordDataObjectReport(metaObject);
	//set complile module 
	//set proc unit 
	m_objectValue->m_compileModule = m_compileModule;
	m_objectValue->m_procUnit = m_procUnit;

	if (appData->DesignerMode()) {
		//incrRef
		m_objectValue->IncrRef();
	}
}

ibValueModuleManagerExternalReport::~ibValueModuleManagerExternalReport()
{
	if (appData->DesignerMode()) {
		//decrRef
		m_objectValue->DecrRef();
	}
}

bool ibValueModuleManagerExternalReport::CreateMainModule()
{
	if (m_initialized)
		return true;

	ibValueModuleManager* moduleManager = activeMetaData->GetModuleManager();
	wxASSERT(moduleManager);

	m_compileModule->SetParent(moduleManager->GetCompileModule());
	m_compileModule->AddContextVariable(thisObject, m_objectValue);

	//initialize procUnit
	wxDELETE(m_procUnit);

	//set complile module 
	m_objectValue->m_compileModule = m_compileModule;

	//initialize object 
	m_objectValue->InitializeObject();

	if (!appData->DesignerMode()) {

		m_procUnit = new ibProcUnit();
		m_procUnit->SetParent(moduleManager->GetProcUnit());
		//set proc unit 
		m_objectValue->m_procUnit = m_procUnit;

		try {
			m_compileModule->Compile();
			m_procUnit->Execute(m_compileModule->m_cByteCode);
		}
		catch (const ibBackendException&) {
			return false;
		};
	}

	//Setup common modules
	for (auto& moduleValue : m_listCommonModuleManager) {
		if (!moduleValue->CreateCommonModule()) {
			return false;
		}
	}

	//set initialized true
	m_initialized = true;
	return true;
}

bool ibValueModuleManagerExternalReport::DestroyMainModule()
{
	if (!m_initialized)
		return true;

	m_compileModule->Reset();
	wxDELETE(m_procUnit);

	//set complile module 
	//set proc unit 
	m_objectValue->m_compileModule = nullptr;
	m_objectValue->m_procUnit = nullptr;

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
bool ibValueModuleManagerExternalReport::StartMainModule(bool force)
{
	if (!m_initialized)
		return false;

	//incrRef - for control delete 
	m_objectValue->IncrRef();

	ibValueMetaObjectRecordData* commonObject = m_objectValue->GetMetaObject();
	wxASSERT(commonObject);
	ibValueMetaObjectFormBase* defFormObject = commonObject->GetDefaultFormByID
	(
		ibValueMetaObjectReport::eFormReport
	);

	if (defFormObject != nullptr) {
		ibBackendValueForm* result = nullptr;
		if (!ibValueModuleManager::FindCompileModule(defFormObject, result)) {

			//valueForm = defFormObject->CreateForm(
			//	nullptr, m_objectValue
			//);
			//try {
			//	if (valueForm->InitializeFormModule()) {
			//		valueForm->ShowForm();
			//	}
			//}
			//catch (...) {
			//	wxDELETE(valueForm);
			//	if (!appData->DesignerMode()) {
			//		//decrRef - for control delete 
			//		m_objectValue->DecrRef();
			//		return false;
			//	}
			//}

			result = ibValueMetaObjectFormBase::CreateAndBuildForm(defFormObject, nullptr, m_objectValue);

			if (result != nullptr) {
				result->ShowForm();
			}
			else if (!appData->DesignerMode()) {
				//decrRef - for control delete 
				m_objectValue->DecrRef();
				return false;
			}
		}
	}
	//else {
	//	ibBackendValueForm* valueForm = ibBackendValueForm::CreateNewForm(nullptr, nullptr, m_objectValue, ibGuid::newGuid());
	//	valueForm->BuildForm(ibValueMetaObjectReport::eFormReport);
	//	try {
	//		valueForm->ShowForm();
	//	}
	//	catch (...) {
	//		wxDELETE(valueForm);
	//		if (appData->EnterpriseMode() ||
	//			appData->ServiceMode()) {
	//			//decrRef - for control delete 
	//			m_objectValue->DecrRef();
	//			return false;
	//		}
	//	}
	//}

	//decrRef - for control delete 
	m_objectValue->DecrRef();

	return true;
}

//main module - destroy
bool ibValueModuleManagerExternalReport::ExitMainModule(bool force)
{
	if (force)
		return true;

	if (!m_initialized)
		return false;

	return true;
}

//****************************************************************************
//*                      ibValueModuleManagerExternalDataProcessor                 *
//****************************************************************************

void ibValueModuleManagerExternalDataProcessor::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	if (m_objectValue != nullptr) {
		m_objectValue->PrepareNames();
		ibValueMethodHelper* methodHelper = m_objectValue->GetPMethods();
		wxASSERT(methodHelper);
		for (long idx = 0; idx < methodHelper->GetNMethods(); idx++) {
			m_methodHelper->CopyMethod(methodHelper, idx);
		}
		for (long idx = 0; idx < methodHelper->GetNProps(); idx++) {
			m_methodHelper->CopyProp(methodHelper, idx);
		}
	}

	if (m_procUnit != nullptr) {
		ibByteCode* byteCode = m_procUnit->GetByteCode();
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
}

bool ibValueModuleManagerExternalDataProcessor::CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray)
{
	if (m_objectValue &&
		m_objectValue->FindMethod(GetMethodName(lMethodNum)) != wxNOT_FOUND) {
		return m_objectValue->CallAsFunc(lMethodNum, pvarRetValue, paParams, lSizeArray);
	}

	return ibModuleDataObject::ExecuteFunc(
		GetMethodName(lMethodNum), pvarRetValue, paParams, lSizeArray
	);
}

bool ibValueModuleManagerExternalDataProcessor::SetPropVal(const long lPropNum, const ibValue& varPropVal)
{
	if (m_objectValue &&
		m_objectValue->FindProp(GetPropName(lPropNum)) != wxNOT_FOUND) {
		return m_objectValue->SetPropVal(lPropNum, varPropVal);
	}

	if (m_procUnit != nullptr) {
		return m_procUnit->SetPropVal(lPropNum, varPropVal);
	}

	return false;
}

bool ibValueModuleManagerExternalDataProcessor::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	if (m_objectValue &&
		m_objectValue->FindProp(GetPropName(lPropNum)) != wxNOT_FOUND) {
		return m_objectValue->GetPropVal(lPropNum, pvarPropVal);
	}

	if (m_procUnit != nullptr) {
		return m_procUnit->GetPropVal(lPropNum, pvarPropVal);
	}

	return false;
}

long ibValueModuleManagerExternalDataProcessor::FindProp(const wxString& strName) const
{
	if (m_objectValue &&
		m_objectValue->FindProp(strName) != wxNOT_FOUND) {
		return m_objectValue->FindProp(strName);
	}

	if (m_procUnit != nullptr) {
		return m_procUnit->FindProp(strName);
	}

	return ibValue::FindProp(strName);
}

//****************************************************************************
//*                      ibValueModuleManagerExternalReport		                 *
//****************************************************************************

void ibValueModuleManagerExternalReport::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	if (m_objectValue != nullptr) {
		m_objectValue->PrepareNames();
		ibValueMethodHelper* methodHelper = m_objectValue->GetPMethods();
		wxASSERT(methodHelper);
		for (long idx = 0; idx < methodHelper->GetNMethods(); idx++) {
			m_methodHelper->CopyMethod(methodHelper, idx);
		}
		for (long idx = 0; idx < methodHelper->GetNProps(); idx++) {
			m_methodHelper->CopyProp(methodHelper, idx);
		}
	}
	if (m_procUnit != nullptr) {
		ibByteCode* byteCode = m_procUnit->GetByteCode();
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
}

bool ibValueModuleManagerExternalReport::CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray)
{
	if (m_objectValue &&
		m_objectValue->FindMethod(GetMethodName(lMethodNum)) != wxNOT_FOUND) {
		return m_objectValue->CallAsFunc(lMethodNum, pvarRetValue, paParams, lSizeArray);
	}

	return ibModuleDataObject::ExecuteFunc(
		GetMethodName(lMethodNum), pvarRetValue, paParams, lSizeArray
	);
}

bool ibValueModuleManagerExternalReport::SetPropVal(const long lPropNum, const ibValue& varPropVal)        //setting attribute
{
	if (m_objectValue &&
		m_objectValue->FindProp(GetPropName(lPropNum)) != wxNOT_FOUND) {
		return m_objectValue->SetPropVal(lPropNum, varPropVal);
	}

	if (m_procUnit != nullptr) {
		return m_procUnit->SetPropVal(lPropNum, varPropVal);
	}

	return false;
}

bool ibValueModuleManagerExternalReport::GetPropVal(const long lPropNum, ibValue& pvarPropVal)                   //attribute value
{
	if (m_objectValue &&
		m_objectValue->FindProp(GetPropName(lPropNum)) != wxNOT_FOUND) {
		return m_objectValue->GetPropVal(lPropNum, pvarPropVal);
	}

	if (m_procUnit != nullptr) {
		return m_procUnit->GetPropVal(lPropNum, pvarPropVal);
	}

	return false;
}

long ibValueModuleManagerExternalReport::FindProp(const wxString& strName) const
{
	if (m_objectValue &&
		m_objectValue->FindProp(strName) != wxNOT_FOUND) {
		return m_objectValue->FindProp(strName);
	}

	if (m_procUnit != nullptr) {
		return m_procUnit->FindProp(strName);
	}

	return ibValue::FindProp(strName);
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

SYSTEM_TYPE_REGISTER(ibValueModuleManagerExternalDataProcessor, "ExternalDataProcessorModuleManager", string_to_clsid("SO_EDMM"));
SYSTEM_TYPE_REGISTER(ibValueModuleManagerExternalReport, "ExternalReportModuleManager", string_to_clsid("SO_ERMM"));
