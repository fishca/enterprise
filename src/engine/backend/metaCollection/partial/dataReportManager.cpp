////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : report - manager
////////////////////////////////////////////////////////////////////////////

#include "dataReportManager.h"
#include "backend/metaData.h"
#include "commonObject.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueManagerDataObjectReport, CValue);

CValueMetaObjectCommonModule* CValueManagerDataObjectReport::GetModuleManager()  const { return m_metaObject->GetModuleManager(); }

//////////////////////////////////////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(CValueManagerDataObjectExternalReport, CValue);

//////////////////////////////////////////////////////////////////////////////////////////////////////

enum Func {
	eCreate = 0,
	eGetForm,
	eGetTemplate,
};

void CValueManagerDataObjectReport::PrepareNames() const
{
	IValueManagerDataObject::PrepareNames();

	m_methodHelper->AppendFunc(wxT("Create"), wxT("Create()"));
	m_methodHelper->AppendFunc(wxT("GetForm"), wxT("GetForm(name : string, owner : any, id : guid)"));
	m_methodHelper->AppendFunc(wxT("GetTemplate"), 1, wxT("GetTemplate(name : string)"));
}

bool CValueManagerDataObjectReport::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
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
	case eGetTemplate:
		pvarRetValue = m_metaObject->GetTemplate(paParams[0]->GetString());
		return true;
	}

	return IValueManagerDataObject::CallAsFunc(lMethodNum, pvarRetValue, paParams, lSizeArray);
}

CValue::CMethodHelper CValueManagerDataObjectExternalReport::m_methodHelper;

void CValueManagerDataObjectExternalReport::PrepareNames() const
{
	m_methodHelper.ClearHelper();
	m_methodHelper.AppendFunc(wxT("Create"), 1, wxT("Create(fullPath : string)"));
}

#include "backend/system/systemManager.h"
#include "backend/metadataReport.h"

bool CValueManagerDataObjectExternalReport::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	CValue ret;
	switch (lMethodNum)
	{
	case eCreate:
	{
		CMetaDataReport* metaReport = new CMetaDataReport();
		if (metaReport->LoadFromFile(paParams[0]->GetString())) {
			CValueModuleManagerExternalReport* moduleManager = metaReport->GetModuleManager();
			pvarRetValue = moduleManager->GetObjectValue();
			return true;
		}
		wxDELETE(metaReport);
		CBackendCoreException::Error(_("Failed to load report '%s'"), paParams[0]->GetString());
	}
	}
	return false;
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

SYSTEM_TYPE_REGISTER(CValueManagerDataObjectExternalReport, "ExternalManagerReport", string_to_clsid("MG_EXTR"));