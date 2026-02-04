////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : dataProcessor - manager
////////////////////////////////////////////////////////////////////////////

#include "dataProcessorManager.h"
#include "backend/metaData.h"
#include "commonObject.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueManagerDataObjectDataProcessor, CValue);

CValueManagerDataObjectDataProcessor::CValueManagerDataObjectDataProcessor(CValueMetaObjectDataProcessor* metaObject) :
	m_methodHelper(new CMethodHelper()), m_metaObject(metaObject)
{
}

CValueManagerDataObjectDataProcessor::~CValueManagerDataObjectDataProcessor()
{
	wxDELETE(m_methodHelper);
}

CValueMetaObjectCommonModule* CValueManagerDataObjectDataProcessor::GetModuleManager() const { return m_metaObject->GetModuleManager(); }

#include "backend/objCtor.h"

class_identifier_t CValueManagerDataObjectDataProcessor::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CValueManagerDataObjectDataProcessor::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CValueManagerDataObjectDataProcessor::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(CValueManagerDataObjectExternalDataProcessor, CValue);

CValueManagerDataObjectExternalDataProcessor::CValueManagerDataObjectExternalDataProcessor() :
	m_methodHelper(new CMethodHelper())
{
}

CValueManagerDataObjectExternalDataProcessor::~CValueManagerDataObjectExternalDataProcessor()
{
	wxDELETE(m_methodHelper);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

enum Func {
	eCreate = 0,
	eGetForm
};

void CValueManagerDataObjectDataProcessor::PrepareNames() const
{
	IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);
	IValueModuleManager* moduleManager = metaData->GetModuleManager();
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

bool CValueManagerDataObjectDataProcessor::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);

	switch (lMethodNum)
	{
	case eCreate:
		pvarRetValue = m_metaObject->CreateObjectValue();
		return true;
	case eGetForm:
	{
		CValueGuid* guidVal = lSizeArray > 2 ? paParams[2]->ConvertToType<CValueGuid>() : nullptr;
		pvarRetValue = m_metaObject->GetGenericForm(paParams[0]->GetString(),
			lSizeArray > 1 ? paParams[1]->ConvertToType<IBackendControlFrame>() : nullptr,
			guidVal ? ((CGuid)*guidVal) : CGuid());
		return true;
	}
	}

	IValueModuleManager* moduleManager = metaData->GetModuleManager();
	wxASSERT(moduleManager);

	CValue* pRefData =
		moduleManager->FindCommonModule(m_metaObject->GetModuleManager());
	if (pRefData != nullptr)
		return pRefData->CallAsFunc(lMethodNum, pvarRetValue, paParams, lSizeArray);
	return false;
}

void CValueManagerDataObjectExternalDataProcessor::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendFunc("create", 1, "create(fullPath)");
}

#include "backend/system/systemManager.h"
#include "backend/metadataDataProcessor.h"

bool CValueManagerDataObjectExternalDataProcessor::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case eCreate:
	{
		CMetaDataDataProcessor* metaDataProcessor = new CMetaDataDataProcessor();
		if (metaDataProcessor->LoadFromFile(paParams[0]->GetString())) {
			CValueModuleManagerExternalDataProcessor* moduleManager = metaDataProcessor->GetModuleManager();
			pvarRetValue = moduleManager->GetObjectValue();
			return true;
		}
		wxDELETE(metaDataProcessor);
		CBackendCoreException::Error("Failed to load data processor '%s'", paParams[0]->GetString());
		return false;
	}
	}

	return false;
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

SYSTEM_TYPE_REGISTER(CValueManagerDataObjectExternalDataProcessor, "externalManagerDataProcessor", string_to_clsid("MG_EXTD"));