////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : report - manager
////////////////////////////////////////////////////////////////////////////

#include "dataReportManager.h"
#include "backend/metaData.h"
#include "commonObject.h"

wxIMPLEMENT_DYNAMIC_CLASS(CReportManager, CValue);

CReportManager::CReportManager(CMetaObjectReport* metaObject) : CValue(eValueTypes::TYPE_VALUE, true),
m_methodHelper(new CMethodHelper()), m_metaObject(metaObject)
{
}

CReportManager::~CReportManager()
{
	wxDELETE(m_methodHelper);
}

CMetaObjectCommonModule* CReportManager::GetModuleManager()  const { return m_metaObject->GetModuleManager(); }

#include "backend/objCtor.h"

class_identifier_t CReportManager::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CReportManager::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CReportManager::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(CReportExternalManager, CValue);

CReportExternalManager::CReportExternalManager() : CValue(eValueTypes::TYPE_VALUE, true),
m_methodHelper(new CMethodHelper())
{
}

CReportExternalManager::~CReportExternalManager()
{
	wxDELETE(m_methodHelper);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

enum Func {
	eCreate = 0,
	eGetForm
};

void CReportManager::PrepareNames() const
{
	IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);
	IModuleManager* moduleManager = metaData->GetModuleManager();
	wxASSERT(moduleManager);

	m_methodHelper->ClearHelper();
	m_methodHelper->AppendFunc("create", "create()");
	m_methodHelper->AppendFunc("getForm", "getForm(string, owner, guid)");

	CValue* pRefData = moduleManager->FindCommonModule(m_metaObject->GetModuleManager());
	if (pRefData != nullptr) {
		// add methods from context
		for (long idx = 0; idx < pRefData->GetNMethods(); idx++) {
			m_methodHelper->CopyMethod(pRefData->GetPMethods(), idx);
		}
	}
}

bool CReportManager::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);

	switch (lMethodNum)
	{
	case eCreate:
		pvarRetValue = m_metaObject->CreateObjectValue();
		return true;
	case eGetForm: {
		CValueGuid* guidVal = lSizeArray > 2 ? paParams[2]->ConvertToType<CValueGuid>() : nullptr;
		pvarRetValue = m_metaObject->GetGenericForm(paParams[0]->GetString(),
			lSizeArray > 1 ? paParams[1]->ConvertToType<IBackendControlFrame>() : nullptr,
			guidVal ? ((CGuid)*guidVal) : CGuid());
		return true;
	}
	}

	IModuleManager* moduleManager = metaData->GetModuleManager();
	wxASSERT(moduleManager);

	CValue* pRefData = moduleManager->FindCommonModule(m_metaObject->GetModuleManager());
	if (pRefData != nullptr)
		return pRefData->CallAsFunc(lMethodNum, pvarRetValue, paParams, lSizeArray);
	return false;
}

void CReportExternalManager::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendFunc("create", "create(fullPath)");
}

#include "backend/system/systemManager.h"
#include "backend/metadataReport.h"

bool CReportExternalManager::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	CValue ret;
	switch (lMethodNum)
	{
	case eCreate:
	{
		CMetaDataReport* metaReport = new CMetaDataReport();
		if (metaReport->LoadFromFile(paParams[0]->GetString())) {
			CModuleManagerExternalReport* moduleManager = metaReport->GetModuleManager();
			pvarRetValue = moduleManager->GetObjectValue();
			return true;
		}
		wxDELETE(metaReport);
		CBackendCoreException::Error(
			wxString::Format("Failed to load report '%s'", paParams[0]->GetString())
		);
	}
	}
	return false;
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

SYSTEM_TYPE_REGISTER(CReportExternalManager, "externalManagerReport", string_to_clsid("MG_EXTR"));