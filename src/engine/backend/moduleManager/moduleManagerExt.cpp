
#include "moduleManagerExt.h"
#include "backend/metadataConfiguration.h"

#include "backend/appData.h"

//////////////////////////////////////////////////////////////////////////////////
#define thisObject wxT("thisObject")
//////////////////////////////////////////////////////////////////////////////////
//  CValueModuleManagerExternalDataProcessor
//////////////////////////////////////////////////////////////////////////////////

CCompileModule* CValueModuleManagerExternalDataProcessor::GetCompileModule() const
{
	IValueModuleManager* moduleManager = activeMetaData->GetModuleManager();
	wxASSERT(moduleManager);
	return moduleManager->GetCompileModule();
}

CProcUnit* CValueModuleManagerExternalDataProcessor::GetProcUnit() const
{
	IValueModuleManager* moduleManager = activeMetaData->GetModuleManager();
	wxASSERT(moduleManager);
	return moduleManager->GetProcUnit();
}

std::map<wxString, CValue*>& CValueModuleManagerExternalDataProcessor::GetContextVariables()
{
	IValueModuleManager* moduleManager = activeMetaData->GetModuleManager();
	wxASSERT(moduleManager);
	return moduleManager->GetContextVariables();
}

CValueModuleManagerExternalDataProcessor::CValueModuleManagerExternalDataProcessor(IMetaData* metadata, CValueMetaObjectDataProcessor* metaObject)
	: IValueModuleManager(CMetaDataConfiguration::Get(), metaObject ? metaObject->GetModuleObject() : nullptr)
{
	m_objectValue = new CValueRecordDataObjectDataProcessor(metaObject);
	//set complile module 
	//set proc unit 
	m_objectValue->m_compileModule = m_compileModule;
	m_objectValue->m_procUnit = m_procUnit;

	if (appData->DesignerMode()) {
		//incrRef
		m_objectValue->IncrRef();
	}
}

CValueModuleManagerExternalDataProcessor::~CValueModuleManagerExternalDataProcessor()
{
	if (appData->DesignerMode()) {
		//decrRef
		m_objectValue->DecrRef();
	}
}

bool CValueModuleManagerExternalDataProcessor::CreateMainModule()
{
	if (m_initialized)
		return true;

	IValueModuleManager* moduleManager = activeMetaData->GetModuleManager();
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

bool CValueModuleManagerExternalDataProcessor::DestroyMainModule()
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
bool CValueModuleManagerExternalDataProcessor::StartMainModule(bool force)
{
	if (!m_initialized)
		return false;

	//incrRef - for control delete 
	m_objectValue->IncrRef();

	IValueMetaObjectRecordData* commonObject = m_objectValue->GetMetaObject();
	wxASSERT(commonObject);
	IValueMetaObjectForm* defFormObject = commonObject->GetDefaultFormByID
	(
		CValueMetaObjectDataProcessor::eFormDataProcessor
	);

	if (defFormObject != nullptr) {

		IBackendValueForm* result = nullptr;

		if (!IValueModuleManager::FindCompileModule(defFormObject, result)) {

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

			result = IValueMetaObjectForm::CreateAndBuildForm(defFormObject, nullptr, m_objectValue);

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
	//	valueForm->BuildForm(CValueMetaObjectDataProcessor::eFormDataProcessor);
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
bool CValueModuleManagerExternalDataProcessor::ExitMainModule(bool force)
{
	if (force)
		return true;

	if (!m_initialized)
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////
//  CValueModuleManagerExternalReport
//////////////////////////////////////////////////////////////////////////////////

CCompileModule* CValueModuleManagerExternalReport::GetCompileModule() const
{
	IValueModuleManager* moduleManager = activeMetaData->GetModuleManager();
	wxASSERT(moduleManager);
	return moduleManager->GetCompileModule();
}

CProcUnit* CValueModuleManagerExternalReport::GetProcUnit() const
{
	IValueModuleManager* moduleManager = activeMetaData->GetModuleManager();
	wxASSERT(moduleManager);
	return moduleManager->GetProcUnit();
}

std::map<wxString, CValue*>& CValueModuleManagerExternalReport::GetContextVariables()
{
	IValueModuleManager* moduleManager = activeMetaData->GetModuleManager();
	wxASSERT(moduleManager);
	return moduleManager->GetContextVariables();
}

CValueModuleManagerExternalReport::CValueModuleManagerExternalReport(IMetaData* metadata, CValueMetaObjectReport* metaObject)
	: IValueModuleManager(CMetaDataConfiguration::Get(), metaObject ? metaObject->GetModuleObject() : nullptr)
{
	m_objectValue = new CValueRecordDataObjectReport(metaObject);
	//set complile module 
	//set proc unit 
	m_objectValue->m_compileModule = m_compileModule;
	m_objectValue->m_procUnit = m_procUnit;

	if (appData->DesignerMode()) {
		//incrRef
		m_objectValue->IncrRef();
	}
}

CValueModuleManagerExternalReport::~CValueModuleManagerExternalReport()
{
	if (appData->DesignerMode()) {
		//decrRef
		m_objectValue->DecrRef();
	}
}

bool CValueModuleManagerExternalReport::CreateMainModule()
{
	if (m_initialized)
		return true;

	IValueModuleManager* moduleManager = activeMetaData->GetModuleManager();
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

bool CValueModuleManagerExternalReport::DestroyMainModule()
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
bool CValueModuleManagerExternalReport::StartMainModule(bool force)
{
	if (!m_initialized)
		return false;

	//incrRef - for control delete 
	m_objectValue->IncrRef();

	IValueMetaObjectRecordData* commonObject = m_objectValue->GetMetaObject();
	wxASSERT(commonObject);
	IValueMetaObjectForm* defFormObject = commonObject->GetDefaultFormByID
	(
		CValueMetaObjectReport::eFormReport
	);

	if (defFormObject != nullptr) {
		IBackendValueForm* result = nullptr;
		if (!IValueModuleManager::FindCompileModule(defFormObject, result)) {

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

			result = IValueMetaObjectForm::CreateAndBuildForm(defFormObject, nullptr, m_objectValue);

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
	//	valueForm->BuildForm(CValueMetaObjectReport::eFormReport);
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
bool CValueModuleManagerExternalReport::ExitMainModule(bool force)
{
	if (force)
		return true;

	if (!m_initialized)
		return false;

	return true;
}

//****************************************************************************
//*                      CValueModuleManagerExternalDataProcessor                 *
//****************************************************************************

void CValueModuleManagerExternalDataProcessor::PrepareNames() const
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

bool CValueModuleManagerExternalDataProcessor::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	if (m_objectValue &&
		m_objectValue->FindMethod(GetMethodName(lMethodNum)) != wxNOT_FOUND) {
		return m_objectValue->CallAsFunc(lMethodNum, pvarRetValue, paParams, lSizeArray);
	}

	return IModuleDataObject::ExecuteFunc(
		GetMethodName(lMethodNum), pvarRetValue, paParams, lSizeArray
	);
}

bool CValueModuleManagerExternalDataProcessor::SetPropVal(const long lPropNum, const CValue& varPropVal)
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

bool CValueModuleManagerExternalDataProcessor::GetPropVal(const long lPropNum, CValue& pvarPropVal)
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

long CValueModuleManagerExternalDataProcessor::FindProp(const wxString& strName) const
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
//*                      CValueModuleManagerExternalReport		                 *
//****************************************************************************

void CValueModuleManagerExternalReport::PrepareNames() const
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

bool CValueModuleManagerExternalReport::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	if (m_objectValue &&
		m_objectValue->FindMethod(GetMethodName(lMethodNum)) != wxNOT_FOUND) {
		return m_objectValue->CallAsFunc(lMethodNum, pvarRetValue, paParams, lSizeArray);
	}

	return IModuleDataObject::ExecuteFunc(
		GetMethodName(lMethodNum), pvarRetValue, paParams, lSizeArray
	);
}

bool CValueModuleManagerExternalReport::SetPropVal(const long lPropNum, const CValue& varPropVal)        //setting attribute
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

bool CValueModuleManagerExternalReport::GetPropVal(const long lPropNum, CValue& pvarPropVal)                   //attribute value
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

long CValueModuleManagerExternalReport::FindProp(const wxString& strName) const
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

SYSTEM_TYPE_REGISTER(CValueModuleManagerExternalDataProcessor, "externalDataProcessorModuleManager", string_to_clsid("SO_EDMM"));
SYSTEM_TYPE_REGISTER(CValueModuleManagerExternalReport, "externalReportModuleManager", string_to_clsid("SO_ERMM"));
