////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : base manager for catalogs, docs etc..  
////////////////////////////////////////////////////////////////////////////

#include "globalContextManager.h"
#include "backend/system/value/valueMap.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueGlobalContextManager, CValue);

#include "backend/objCtor.h"

class CValueGlobalContextStructureManager : public CValueStructure {
public:

	CValueGlobalContextStructureManager() : m_clsid(0), m_metaData(nullptr) {}
	CValueGlobalContextStructureManager(const class_identifier_t& clsid, IMetaData* metaData)
		: CValueStructure(true), m_clsid(clsid), m_metaData(metaData) {

		for (const auto object : m_metaData->GetAnyArrayObject(clsid)) {
			IMetaValueTypeCtor* so = m_metaData->GetTypeCtor(object, eCtorMetaType::eCtorMetaType_Manager);
			if (so == nullptr)
				continue;
			CValuePtr<CValue> createdValue = so->CreateObject();
			if (createdValue != nullptr)
				createdValue->PrepareNames();
			CValueStructure::Insert(object->GetName(), createdValue);
		}
	}

	virtual wxString GetClassName() const {
		IAbstractTypeCtor* so = m_metaData->GetAvailableCtor(m_clsid);
		if (so != nullptr)
			return so->GetClassName() + wxT("Manager");
		return CValueStructure::GetClassName();
	}

	virtual wxString GetString() const {
		IAbstractTypeCtor* so = m_metaData->GetAvailableCtor(m_clsid);
		if (so != nullptr)
			return so->GetClassName() + wxT("Manager");
		return CValueStructure::GetString();
	}

private:

	IMetaData* m_metaData;

	class_identifier_t m_clsid;

	wxDECLARE_DYNAMIC_CLASS(CValueGlobalContextStructureManager);
};

wxIMPLEMENT_DYNAMIC_CLASS(CValueGlobalContextStructureManager, CValue);

enum
{
	enConstants = 0,
	enCatalogs,
	enDocuments,
	enEnumerations,
	enDataProcessors,
	enExternalDataProcessors,
	enReports,
	enExternalReports,
	enInformationRegisters,
	enAccumulationRegisters
};

void CValueGlobalContextManager::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	m_methodHelper->AppendProp(wxT("constants"));
	m_methodHelper->AppendProp(wxT("catalogs"));
	m_methodHelper->AppendProp(wxT("documents"));
	m_methodHelper->AppendProp(wxT("enumerations"));
	m_methodHelper->AppendProp(wxT("dataProcessors"));
	m_methodHelper->AppendProp(wxT("externalDataProcessors"));
	m_methodHelper->AppendProp(wxT("reports"));
	m_methodHelper->AppendProp(wxT("externalReports"));
	m_methodHelper->AppendProp(wxT("informationRegisters"));
	m_methodHelper->AppendProp(wxT("accumulationRegisters"));
}

#include "backend/metaCollection/partial/dataProcessorManager.h"
#include "backend/metaCollection/partial/dataReportManager.h"

bool CValueGlobalContextManager::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	switch (lPropNum)
	{
	case enConstants:
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueGlobalContextStructureManager>(g_metaConstantCLSID, m_metaData);
		return true;
	case enCatalogs:
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueGlobalContextStructureManager>(g_metaCatalogCLSID, m_metaData);
		return true;
	case enDocuments:
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueGlobalContextStructureManager>(g_metaDocumentCLSID, m_metaData);
		return true;
	case enEnumerations:
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueGlobalContextStructureManager>(g_metaEnumerationCLSID, m_metaData);
		return true;
	case enDataProcessors:
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueGlobalContextStructureManager>(g_metaDataProcessorCLSID, m_metaData);
		return true;
	case enExternalDataProcessors:
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueManagerDataObjectExternalDataProcessor>();
		return true;
	case enReports:
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueGlobalContextStructureManager>(g_metaReportCLSID, m_metaData);
		return true;
	case enExternalReports:
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueManagerDataObjectExternalReport>();
		return true;
	case enInformationRegisters:
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueGlobalContextStructureManager>(g_metaInformationRegisterCLSID, m_metaData);
		return true;
	case enAccumulationRegisters:
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueGlobalContextStructureManager>(g_metaAccumulationRegisterCLSID, m_metaData);
		return true;
	}

	return false;
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

SYSTEM_TYPE_REGISTER(CValueGlobalContextManager, "globalContextManager", string_to_clsid("MG_SYSM"));
SYSTEM_TYPE_REGISTER(CValueGlobalContextStructureManager, "globalContextStructureManager", string_to_clsid("MG_SYAM"));