
#include "moduleManagerExt.h"
#include "backend/metadataConfiguration.h"

#include "backend/appData.h"

//////////////////////////////////////////////////////////////////////////////////
#define thisObject wxT("thisObject")
//////////////////////////////////////////////////////////////////////////////////
//  CModuleManagerExternalDataProcessor
//////////////////////////////////////////////////////////////////////////////////

CCompileModule* CModuleManagerExternalDataProcessor::GetCompileModule() const
{
	IModuleManager* moduleManager = activeMetaData->GetModuleManager();
	wxASSERT(moduleManager);
	return moduleManager->GetCompileModule();
}

CProcUnit* CModuleManagerExternalDataProcessor::GetProcUnit() const
{
	IModuleManager* moduleManager = activeMetaData->GetModuleManager();
	wxASSERT(moduleManager);
	return moduleManager->GetProcUnit();
}

std::map<wxString, CValue*>& CModuleManagerExternalDataProcessor::GetContextVariables()
{
	IModuleManager* moduleManager = activeMetaData->GetModuleManager();
	wxASSERT(moduleManager);
	return moduleManager->GetContextVariables();
}

CModuleManagerExternalDataProcessor::CModuleManagerExternalDataProcessor(IMetaData* metadata, CMetaObjectDataProcessor* metaObject)
	: IModuleManager(CMetaDataConfiguration::Get(), metaObject ? metaObject->GetModuleObject() : nullptr)
{
	m_objectValue = new CRecordDataObjectDataProcessor(metaObject);
	//set complile module 
	//set proc unit 
	m_objectValue->m_compileModule = m_compileModule;
	m_objectValue->m_procUnit = m_procUnit;

	if (appData->DesignerMode()) {
		//incrRef
		m_objectValue->IncrRef();
	}
}

CModuleManagerExternalDataProcessor::~CModuleManagerExternalDataProcessor()
{
	if (appData->DesignerMode()) {
		//decrRef
		m_objectValue->DecrRef();
	}
}

bool CModuleManagerExternalDataProcessor::CreateMainModule()
{
	if (m_initialized)
		return true;

	IModuleManager* moduleManager = activeMetaData->GetModuleManager();
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

		m_procUnit = new CProcUnit();
		m_procUnit->SetParent(moduleManager->GetProcUnit());

		//set proc unit 
		m_objectValue->m_procUnit = m_procUnit;

		try {
			m_compileModule->Compile();
			//and run... 
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

	//set initialized true
	m_initialized = true;
	return true;
}

bool CModuleManagerExternalDataProcessor::DestroyMainModule()
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
bool CModuleManagerExternalDataProcessor::StartMainModule(bool force)
{
	if (!m_initialized)
		return false;

	//incrRef - for control delete 
	m_objectValue->IncrRef();

	IMetaObjectRecordData* commonObject = m_objectValue->GetMetaObject();
	wxASSERT(commonObject);
	IMetaObjectForm* defFormObject = commonObject->GetDefaultFormByID
	(
		CMetaObjectDataProcessor::eFormDataProcessor
	);

	if (defFormObject != nullptr) {

		IBackendValueForm* result = nullptr;

		if (!IModuleManager::FindCompileModule(defFormObject, result)) {

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

			result = IMetaObjectForm::CreateAndBuildForm(defFormObject, nullptr, m_objectValue);

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
	//	IBackendValueForm* valueForm = IBackendValueForm::CreateNewForm(nullptr, nullptr, m_objectValue, CGuid::newGuid());
	//	valueForm->BuildForm(CMetaObjectDataProcessor::eFormDataProcessor);
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
bool CModuleManagerExternalDataProcessor::ExitMainModule(bool force)
{
	if (force)
		return true;

	if (!m_initialized)
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////
//  CModuleManagerExternalReport
//////////////////////////////////////////////////////////////////////////////////

CCompileModule* CModuleManagerExternalReport::GetCompileModule() const
{
	IModuleManager* moduleManager = activeMetaData->GetModuleManager();
	wxASSERT(moduleManager);
	return moduleManager->GetCompileModule();
}

CProcUnit* CModuleManagerExternalReport::GetProcUnit() const
{
	IModuleManager* moduleManager = activeMetaData->GetModuleManager();
	wxASSERT(moduleManager);
	return moduleManager->GetProcUnit();
}

std::map<wxString, CValue*>& CModuleManagerExternalReport::GetContextVariables()
{
	IModuleManager* moduleManager = activeMetaData->GetModuleManager();
	wxASSERT(moduleManager);
	return moduleManager->GetContextVariables();
}

CModuleManagerExternalReport::CModuleManagerExternalReport(IMetaData* metadata, CMetaObjectReport* metaObject)
	: IModuleManager(CMetaDataConfiguration::Get(), metaObject ? metaObject->GetModuleObject() : nullptr)
{
	m_objectValue = new CRecordDataObjectReport(metaObject);
	//set complile module 
	//set proc unit 
	m_objectValue->m_compileModule = m_compileModule;
	m_objectValue->m_procUnit = m_procUnit;

	if (appData->DesignerMode()) {
		//incrRef
		m_objectValue->IncrRef();
	}
}

CModuleManagerExternalReport::~CModuleManagerExternalReport()
{
	if (appData->DesignerMode()) {
		//decrRef
		m_objectValue->DecrRef();
	}
}

bool CModuleManagerExternalReport::CreateMainModule()
{
	if (m_initialized)
		return true;

	IModuleManager* moduleManager = activeMetaData->GetModuleManager();
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

		m_procUnit = new CProcUnit();
		m_procUnit->SetParent(moduleManager->GetProcUnit());
		//set proc unit 
		m_objectValue->m_procUnit = m_procUnit;

		try {
			m_compileModule->Compile();
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

	//set initialized true
	m_initialized = true;
	return true;
}

bool CModuleManagerExternalReport::DestroyMainModule()
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
bool CModuleManagerExternalReport::StartMainModule(bool force)
{
	if (!m_initialized)
		return false;

	//incrRef - for control delete 
	m_objectValue->IncrRef();

	IMetaObjectRecordData* commonObject = m_objectValue->GetMetaObject();
	wxASSERT(commonObject);
	IMetaObjectForm* defFormObject = commonObject->GetDefaultFormByID
	(
		CMetaObjectReport::eFormReport
	);

	if (defFormObject != nullptr) {
		IBackendValueForm* result = nullptr;
		if (!IModuleManager::FindCompileModule(defFormObject, result)) {

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

			result = IMetaObjectForm::CreateAndBuildForm(defFormObject, nullptr, m_objectValue);

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
	//	IBackendValueForm* valueForm = IBackendValueForm::CreateNewForm(nullptr, nullptr, m_objectValue, CGuid::newGuid());
	//	valueForm->BuildForm(CMetaObjectReport::eFormReport);
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
bool CModuleManagerExternalReport::ExitMainModule(bool force)
{
	if (force)
		return true;

	if (!m_initialized)
		return false;

	return true;
}

//****************************************************************************
//*                      CModuleManagerExternalDataProcessor                 *
//****************************************************************************

void CModuleManagerExternalDataProcessor::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	if (m_objectValue != nullptr) {
		m_objectValue->PrepareNames();
		CMethodHelper* methodHelper = m_objectValue->GetPMethods();
		wxASSERT(methodHelper);
		for (long idx = 0; idx < methodHelper->GetNMethods(); idx++) {
			m_methodHelper->CopyMethod(methodHelper, idx);
		}
		for (long idx = 0; idx < methodHelper->GetNProps(); idx++) {
			m_methodHelper->CopyProp(methodHelper, idx);
		}
	}

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
}

bool CModuleManagerExternalDataProcessor::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	if (m_objectValue &&
		m_objectValue->FindMethod(GetMethodName(lMethodNum)) != wxNOT_FOUND) {
		return m_objectValue->CallAsFunc(lMethodNum, pvarRetValue, paParams, lSizeArray);
	}

	return IModuleDataObject::ExecuteFunc(
		GetMethodName(lMethodNum), pvarRetValue, paParams, lSizeArray
	);
}

bool CModuleManagerExternalDataProcessor::SetPropVal(const long lPropNum, const CValue& varPropVal)
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

bool CModuleManagerExternalDataProcessor::GetPropVal(const long lPropNum, CValue& pvarPropVal)
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

long CModuleManagerExternalDataProcessor::FindProp(const wxString& strName) const
{
	if (m_objectValue &&
		m_objectValue->FindProp(strName) != wxNOT_FOUND) {
		return m_objectValue->FindProp(strName);
	}

	if (m_procUnit != nullptr) {
		return m_procUnit->FindProp(strName);
	}

	return CValue::FindProp(strName);
}

//****************************************************************************
//*                      CModuleManagerExternalReport		                 *
//****************************************************************************

void CModuleManagerExternalReport::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	if (m_objectValue != nullptr) {
		m_objectValue->PrepareNames();
		CMethodHelper* methodHelper = m_objectValue->GetPMethods();
		wxASSERT(methodHelper);
		for (long idx = 0; idx < methodHelper->GetNMethods(); idx++) {
			m_methodHelper->CopyMethod(methodHelper, idx);
		}
		for (long idx = 0; idx < methodHelper->GetNProps(); idx++) {
			m_methodHelper->CopyProp(methodHelper, idx);
		}
	}
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
}

bool CModuleManagerExternalReport::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	if (m_objectValue &&
		m_objectValue->FindMethod(GetMethodName(lMethodNum)) != wxNOT_FOUND) {
		return m_objectValue->CallAsFunc(lMethodNum, pvarRetValue, paParams, lSizeArray);
	}

	return IModuleDataObject::ExecuteFunc(
		GetMethodName(lMethodNum), pvarRetValue, paParams, lSizeArray
	);
}

bool CModuleManagerExternalReport::SetPropVal(const long lPropNum, const CValue& varPropVal)        //setting attribute
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

bool CModuleManagerExternalReport::GetPropVal(const long lPropNum, CValue& pvarPropVal)                   //attribute value
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

long CModuleManagerExternalReport::FindProp(const wxString& strName) const
{
	if (m_objectValue &&
		m_objectValue->FindProp(strName) != wxNOT_FOUND) {
		return m_objectValue->FindProp(strName);
	}

	if (m_procUnit != nullptr) {
		return m_procUnit->FindProp(strName);
	}

	return CValue::FindProp(strName);
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

SYSTEM_TYPE_REGISTER(CModuleManagerExternalDataProcessor, "externalDataProcessorModuleManager", string_to_clsid("SO_EDMM"));
SYSTEM_TYPE_REGISTER(CModuleManagerExternalReport, "externalReportModuleManager", string_to_clsid("SO_ERMM"));
