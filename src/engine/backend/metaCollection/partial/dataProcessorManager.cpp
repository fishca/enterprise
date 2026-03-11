////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : dataProcessor - manager
////////////////////////////////////////////////////////////////////////////

#include "dataProcessorManager.h"
#include "backend/metaData.h"
#include "commonObject.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueManagerDataObjectDataProcessor, CValue);

CValueMetaObjectCommonModule* CValueManagerDataObjectDataProcessor::GetModuleManager() const { return m_metaObject->GetModuleManager(); }

//////////////////////////////////////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(CValueManagerDataObjectExternalDataProcessor, CValue);

//////////////////////////////////////////////////////////////////////////////////////////////////////

enum Func {
	eCreate = 0,
	eGetForm,
	eGetTemplate,
};

void CValueManagerDataObjectDataProcessor::PrepareNames() const
{
	IValueManagerDataObject::PrepareNames();

	m_methodHelper->AppendFunc(wxT("Create"), wxT("Create()"));
	m_methodHelper->AppendFunc(wxT("GetForm"), wxT("GetForm(name : string, owner : any, id : guid)"));
	m_methodHelper->AppendFunc(wxT("GetTemplate"), 1, wxT("GetTemplate(name : string)"));
}

bool CValueManagerDataObjectDataProcessor::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
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
	case eGetTemplate:
		pvarRetValue = m_metaObject->GetTemplate(paParams[0]->GetString());
		return true;
	}

	return IValueManagerDataObject::CallAsFunc(lMethodNum, pvarRetValue, paParams, lSizeArray);
}

CValue::CMethodHelper CValueManagerDataObjectExternalDataProcessor::m_methodHelper;

void CValueManagerDataObjectExternalDataProcessor::PrepareNames() const
{
	m_methodHelper.ClearHelper();
	m_methodHelper.AppendFunc(wxT("Create"), 1, wxT("Create(fullPath : string)"));
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
		CBackendCoreException::Error(_("Failed to load data processor '%s'"), paParams[0]->GetString());
		return false;
	}
	}

	return false;
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

SYSTEM_TYPE_REGISTER(CValueManagerDataObjectExternalDataProcessor, "externalManagerDataProcessor", string_to_clsid("MG_EXTD"));