////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : report - manager
////////////////////////////////////////////////////////////////////////////

#include "dataReportManager.h"
#include "backend/metaData.h"
#include "commonObject.h"

wxIMPLEMENT_DYNAMIC_CLASS(CManagerDataObjectReport, CValue);

CManagerDataObjectReport::CManagerDataObjectReport(CMetaObjectReport* metaObject) :
	m_methodHelper(new CMethodHelper()), m_metaObject(metaObject)
{
}

CManagerDataObjectReport::~CManagerDataObjectReport()
{
	wxDELETE(m_methodHelper);
}

CMetaObjectCommonModule* CManagerDataObjectReport::GetModuleManager()  const { return m_metaObject->GetModuleManager(); }

#include "backend/objCtor.h"

class_identifier_t CManagerDataObjectReport::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CManagerDataObjectReport::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CManagerDataObjectReport::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(CManagerDataObjectExternalReport, CValue);

CManagerDataObjectExternalReport::CManagerDataObjectExternalReport() :
	m_methodHelper(new CMethodHelper())
{
}

CManagerDataObjectExternalReport::~CManagerDataObjectExternalReport()
{
	wxDELETE(m_methodHelper);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

enum Func {
	eCreate = 0,
	eGetForm
};

void CManagerDataObjectReport::PrepareNames() const
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

bool CManagerDataObjectReport::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
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

void CManagerDataObjectExternalReport::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendFunc("create", 1, "create(fullPath)");
}

#include "backend/system/systemManager.h"
#include "backend/metadataReport.h"

bool CManagerDataObjectExternalReport::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
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
		CBackendCoreException::Error("Failed to load report '%s'", paParams[0]->GetString());
	}
	}
	return false;
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

SYSTEM_TYPE_REGISTER(CManagerDataObjectExternalReport, "externalManagerReport", string_to_clsid("MG_EXTR"));