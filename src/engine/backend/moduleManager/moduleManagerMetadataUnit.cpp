////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : meta-access 
////////////////////////////////////////////////////////////////////////////

#include "moduleManager.h"
#include "backend/metaData.h"

#include "backend/system/value/valueMap.h"

wxIMPLEMENT_DYNAMIC_CLASS(IModuleManager::CMetadataUnit, CValue);

IModuleManager::CMetadataUnit::CMetadataUnit(IMetaData* metaData) :
	CValue(eValueTypes::TYPE_VALUE, true),
	m_methodHelper(new CMethodHelper()), m_metaData(metaData)
{
}

IModuleManager::CMetadataUnit::~CMetadataUnit()
{
	wxDELETE(m_methodHelper);
}

enum
{
	enCommonModules = 0,
	enCommonForms,
	enCommonTemplates,
	enConstants,
	enCatalogs,
	enDocuments,
	enEnumerations,
	enDataProcessors,
	enReports,
	enInformationRegisters,
	enAccumulationRegisters,
};

void IModuleManager::CMetadataUnit::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendProp("commonModules", true, false, g_metaCommonModuleCLSID);
	m_methodHelper->AppendProp("commonForms", true, false, g_metaCommonFormCLSID);
	m_methodHelper->AppendProp("commonTemplates", true, false, g_metaCommonTemplateCLSID);
	m_methodHelper->AppendProp("constants", true, false, g_metaConstantCLSID);
	m_methodHelper->AppendProp("catalogs", true, false, g_metaCatalogCLSID);
	m_methodHelper->AppendProp("documents", true, false, g_metaDocumentCLSID);
	m_methodHelper->AppendProp("enumerations", true, false, g_metaEnumerationCLSID);
	m_methodHelper->AppendProp("dataProcessors", true, false, g_metaDataProcessorCLSID);
	m_methodHelper->AppendProp("reports", true, false, g_metaReportCLSID);
	m_methodHelper->AppendProp("informationRegisters", true, false, g_metaInformationRegisterCLSID);
	m_methodHelper->AppendProp("accumulationRegisters", true, false, g_metaAccumulationRegisterCLSID);
}

//****************************************************************************
//*                              Override attribute                          *
//****************************************************************************

bool IModuleManager::CMetadataUnit::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	return false;
}

bool IModuleManager::CMetadataUnit::GetPropVal(const long lPropNum, CValue& pvarPropVal)//attribute value
{
	CValueStructure* valStruct = CValue::CreateAndPrepareValueRef<CValueStructure>();
	switch (lPropNum)
	{
	case enCommonModules: {
		for (const auto object : m_metaData->GetAnyArrayObject(g_metaCommonModuleCLSID)) {
			valStruct->Insert(object->GetName(), object);
		}
	} break;
	case enCommonForms: {
		for (const auto object : m_metaData->GetAnyArrayObject(g_metaCommonFormCLSID)) {
			valStruct->Insert(object->GetName(), object);
		}
	} break;
	case enCommonTemplates: {
		for (const auto object : m_metaData->GetAnyArrayObject(g_metaCommonTemplateCLSID)) {
			valStruct->Insert(object->GetName(), object);
		}
	} break;
	case enConstants: {
		for (const auto object : m_metaData->GetAnyArrayObject(g_metaConstantCLSID)) {
			valStruct->Insert(object->GetName(), object);
		}
	} break;
	case enCatalogs: {
		for (const auto object : m_metaData->GetAnyArrayObject(g_metaCatalogCLSID)) {
			valStruct->Insert(object->GetName(), object);
		}
	} break;
	case enDocuments: {
		for (const auto object : m_metaData->GetAnyArrayObject(g_metaDocumentCLSID)) {
			valStruct->Insert(object->GetName(), object);
		}
	} break;
	case enEnumerations: {
		for (const auto object : m_metaData->GetAnyArrayObject(g_metaEnumerationCLSID)) {
			valStruct->Insert(object->GetName(), object);
		}
	} break;
	case enDataProcessors: {
		for (const auto object : m_metaData->GetAnyArrayObject(g_metaDataProcessorCLSID)) {
			valStruct->Insert(object->GetName(), object);
		}
	} break;
	case enReports: {
		for (const auto object : m_metaData->GetAnyArrayObject(g_metaReportCLSID)) {
			valStruct->Insert(object->GetName(), object);
		}
	} break;
	case enInformationRegisters: {
		for (const auto object : m_metaData->GetAnyArrayObject(g_metaInformationRegisterCLSID)) {
			valStruct->Insert(object->GetName(), object);
		}
	} break;
	case enAccumulationRegisters: {
		for (const auto object : m_metaData->GetAnyArrayObject(g_metaAccumulationRegisterCLSID)) {
			valStruct->Insert(object->GetName(), object);
		}
		break;
	}
	}
	pvarPropVal = valStruct;
	return true;
}