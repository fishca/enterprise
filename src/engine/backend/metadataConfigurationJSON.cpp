////////////////////////////////////////////////////////////////////////////
//	Author		: Tetracode Dev
//	Description : configuration JSON export/import
////////////////////////////////////////////////////////////////////////////

#include "metadataConfiguration.h"
#include "metaCollection/metaObject.h"
#include "metaCollection/metaObjectMetadata.h"
#include "metaCollection/metaObjectComposite.h"
#include "metaCollection/metaModuleObject.h"
#include "metaCollection/metaFormObject.h"
#include "metaCollection/attribute/metaAttributeObject.h"
#include "metaCollection/enumeration/metaEnumObject.h"
#include "metaCollection/dimension/metaDimensionObject.h"
#include "metaCollection/resource/metaResourceObject.h"
#include "metaCollection/table/metaTableObject.h"

#include "metaCollection/partial/catalog.h"
#include "metaCollection/partial/document.h"
#include "metaCollection/partial/enumeration.h"
#include "metaCollection/partial/constant.h"
#include "metaCollection/partial/informationRegister.h"
#include "metaCollection/partial/accumulationRegister.h"
#include "metaCollection/partial/dataProcessor.h"
#include "metaCollection/partial/dataReport.h"
#include "metaCollection/partial/chartOfCharacteristicTypes.h"
#include "metaCollection/partial/chartOfAccounts.h"
#include "metaCollection/partial/accountingRegister.h"

#include "3rdparty/nlohmann/json.hpp"

#include <wx/wfstream.h>
#include <wx/base64.h>

#include "propertyManager/property/propertyGeneration.h"
#include "propertyManager/property/propertyRecord.h"
#include "propertyManager/property/propertyOwner.h"
#include "propertyManager/property/propertyChartOfCharacteristicTypes.h"
#include "propertyManager/property/propertyChartOfAccounts.h"

#include <fstream>

using json = nlohmann::json;

//***********************************************************************
//*                    JSON Serialization Helpers                       *
//***********************************************************************

namespace {

inline std::string WxToStd(const wxString& s) {
	return std::string(s.ToUTF8());
}

inline wxString StdToWx(const std::string& s) {
	return wxString::FromUTF8(s.c_str());
}

//***********************************************************************
//*                    Type Description Serialization                  *
//***********************************************************************

json SaveTypeDescriptionToJSON(const ibTypeDescription& typeDesc)
{
	json jType = json::object();

	const auto& clsidList = typeDesc.GetClsidList();
	if (clsidList.empty())
		return jType;

	json jTypeIds = json::array();
	for (const auto& clsid : clsidList) {
		jTypeIds.push_back((unsigned long long)clsid);
	}
	jType["typeIds"] = jTypeIds;

	// Number qualifiers
	jType["precision"] = typeDesc.GetPrecision();
	jType["scale"] = typeDesc.GetScale();
	jType["nonNegative"] = typeDesc.IsNonNegative();

	// String qualifiers
	jType["length"] = typeDesc.GetLength();
	jType["allowedLength"] = (int)typeDesc.GetAllowedLength();

	// Date qualifiers
	jType["dateFractions"] = (int)typeDesc.GetDateFraction();

	return jType;
}

json SaveMetaDescriptionToJSON(const ibMetaDescription& metaDesc)
{
	json jDesc = json::array();
	for (unsigned int i = 0; i < metaDesc.GetTypeCount(); i++) {
		jDesc.push_back(metaDesc.GetByIdx(i));
	}
	return jDesc;
}

//***********************************************************************
//*                    Attribute Serialization                         *
//***********************************************************************

json SaveAttributeToJSON(ibValueMetaObjectAttributeBase* attr)
{
	json jAttr = json::object();
	jAttr["guid"] = WxToStd(attr->GetGuid().str());
	jAttr["id"] = attr->GetMetaID();
	jAttr["name"] = WxToStd(attr->GetName());

	const wxString synonym = attr->GetSynonym();
	if (!synonym.IsEmpty())
		jAttr["synonym"] = WxToStd(synonym);

	jAttr["fillCheck"] = attr->FillCheck();
	jAttr["itemMode"] = (int)attr->GetItemMode();
	jAttr["selectMode"] = (int)attr->GetSelectMode();

	// Type information with qualifiers
	json jType = SaveTypeDescriptionToJSON(attr->GetTypeDesc());
	if (!jType.empty())
		jAttr["type"] = jType;

	return jAttr;
}

json SavePredefinedAttributeToJSON(ibValueMetaObjectAttributePredefined* attr)
{
	return SaveAttributeToJSON(attr);
}

//***********************************************************************
//*                    Table/Dimension/Resource Serialization           *
//***********************************************************************

json SaveTableToJSON(ibValueMetaObjectTableData* table)
{
	json jTable = json::object();
	jTable["guid"] = WxToStd(table->GetGuid().str());
	jTable["id"] = table->GetMetaID();
	jTable["name"] = WxToStd(table->GetName());

	const wxString synonym = table->GetSynonym();
	if (!synonym.IsEmpty())
		jTable["synonym"] = WxToStd(synonym);

	// Table attributes (columns)
	json jAttrs = json::array();
	for (auto attr : table->GetAttributeArrayObject()) {
		if (attr->IsDeleted()) continue;
		jAttrs.push_back(SaveAttributeToJSON(attr));
	}
	jTable["attributes"] = jAttrs;

	return jTable;
}

json SaveDimensionToJSON(ibValueMetaObjectDimension* dim)
{
	return SaveAttributeToJSON(dim);
}

json SaveResourceToJSON(ibValueMetaObjectResource* res)
{
	return SaveAttributeToJSON(res);
}

//***********************************************************************
//*                    Form Serialization                              *
//***********************************************************************

json SaveFormToJSON(ibValueMetaObjectFormBase* form)
{
	json jForm = json::object();
	jForm["guid"] = WxToStd(form->GetGuid().str());
	jForm["id"] = form->GetMetaID();
	jForm["type"] = form->GetTypeForm();
	jForm["name"] = WxToStd(form->GetName());

	const wxString synonym = form->GetSynonym();
	if (!synonym.IsEmpty())
		jForm["synonym"] = WxToStd(synonym);

	// Form layout as base64 (binary)
	wxMemoryBuffer formData = form->GetFormData();
	if (formData.GetDataLen() > 0) {
		jForm["formData"] = WxToStd(wxBase64Encode(formData));
	}

	// Form module code
	const wxString moduleText = form->GetModuleText();
	if (!moduleText.IsEmpty())
		jForm["module"] = WxToStd(moduleText);

	return jForm;
}

//***********************************************************************
//*                    Module Serialization                            *
//***********************************************************************

std::string SaveModuleToJSON(ibValueMetaObjectModuleBase* moduleObj)
{
	if (moduleObj == nullptr)
		return std::string();
	return WxToStd(moduleObj->GetModuleText());
}

//***********************************************************************
//*                    Predefined Values Serialization                 *
//***********************************************************************

json SavePredefinedValuesToJSON(ibValueMetaObjectRecordDataHierarchyMutableRef* hierarchyObj)
{
	const auto& predefinedValues = hierarchyObj->GetPredefinedValueArray();

	json jPredefined = json::array();
	for (const auto& pv : predefinedValues) {
		json jItem = json::object();
		jItem["guid"] = WxToStd(pv->GetPredefinedGuid().str());
		jItem["isFolder"] = pv->IsPredefinedFolder();
		jItem["name"] = WxToStd(pv->GetPredefinedName());
		jItem["code"] = WxToStd(pv->GetPredefinedCode());
		jItem["description"] = WxToStd(pv->GetPredefinedDescription());
		jItem["parent"] = WxToStd(pv->GetPredefinedParentName());
		jPredefined.push_back(jItem);
	}
	return jPredefined;
}

//***********************************************************************
//*                    Enum Values Serialization                       *
//***********************************************************************

json SaveEnumValuesToJSON(ibValueMetaObjectRecordDataEnumRef* enumRef)
{
	json jEnums = json::array();
	auto enumValues = enumRef->GetEnumObjectArray();
	for (auto enumVal : enumValues) {
		if (enumVal->IsDeleted()) continue;
		json jItem = json::object();
		jItem["guid"] = WxToStd(enumVal->GetGuid().str());
		jItem["id"] = enumVal->GetMetaID();
		jItem["name"] = WxToStd(enumVal->GetName());
		const wxString synonym = enumVal->GetSynonym();
		if (!synonym.IsEmpty())
			jItem["synonym"] = WxToStd(synonym);
		jEnums.push_back(jItem);
	}
	return jEnums;
}

//***********************************************************************
//*                   Common elements shared by record types           *
//***********************************************************************

json SaveUserAttributesToJSON(ibValueMetaObjectRecordData* recordData)
{
	json jAttrs = json::array();
	auto attrs = recordData->GetAttributeArrayObject();
	for (auto attr : attrs) {
		if (attr->IsDeleted()) continue;
		jAttrs.push_back(SaveAttributeToJSON(attr));
	}
	return jAttrs;
}

json SaveUserTablesToJSON(ibValueMetaObjectRecordData* recordData)
{
	json jTables = json::array();
	auto tables = recordData->GetTableArrayObject();
	for (auto table : tables) {
		if (table->IsDeleted()) continue;
		jTables.push_back(SaveTableToJSON(table));
	}
	return jTables;
}

json SaveFormsToJSON(ibValueMetaObjectGenericData* genericData)
{
	json jForms = json::array();
	auto forms = genericData->GetFormArrayObject();
	for (auto form : forms) {
		if (form->IsDeleted()) continue;
		jForms.push_back(SaveFormToJSON(form));
	}
	return jForms;
}

json SaveRegisterDimensionsToJSON(ibValueMetaObjectRegisterData* regData)
{
	json jDims = json::array();
	auto dims = regData->GetDimentionArrayObject();
	for (auto dim : dims) {
		if (dim->IsDeleted()) continue;
		jDims.push_back(SaveDimensionToJSON(dim));
	}
	return jDims;
}

json SaveRegisterResourcesToJSON(ibValueMetaObjectRegisterData* regData)
{
	json jRes = json::array();
	auto resources = regData->GetResourceArrayObject();
	for (auto res : resources) {
		if (res->IsDeleted()) continue;
		jRes.push_back(SaveResourceToJSON(res));
	}
	return jRes;
}

json SaveRegisterAttributesToJSON(ibValueMetaObjectRegisterData* regData)
{
	json jAttrs = json::array();
	auto attrs = regData->GetAttributeArrayObject();
	for (auto attr : attrs) {
		if (attr->IsDeleted()) continue;
		jAttrs.push_back(SaveAttributeToJSON(attr));
	}
	return jAttrs;
}

//***********************************************************************
//*                   Per-Type SaveData JSON Serialization              *
//***********************************************************************

// ---- Catalog ----
void SaveCatalogToJSON(json& jObj, ibValueMetaObjectCatalog* catalog)
{
	// Predefined attributes
	json jPredAttrs = json::object();
	jPredAttrs["owner"] = SavePredefinedAttributeToJSON(catalog->GetCatalogOwner());
	jPredAttrs["predefinedName"] = SavePredefinedAttributeToJSON(catalog->GetDataPredefinedName());
	jPredAttrs["code"] = SavePredefinedAttributeToJSON(catalog->GetDataCode());
	jPredAttrs["description"] = SavePredefinedAttributeToJSON(catalog->GetDataDescription());
	jPredAttrs["parent"] = SavePredefinedAttributeToJSON(catalog->GetDataParent());
	jPredAttrs["isFolder"] = SavePredefinedAttributeToJSON(catalog->GetDataIsFolder());
	jPredAttrs["dataVersion"] = SavePredefinedAttributeToJSON(catalog->GetDataVersion());
	jPredAttrs["deletionMark"] = SavePredefinedAttributeToJSON(catalog->GetDataDeletionMark());
	jPredAttrs["reference"] = SavePredefinedAttributeToJSON(catalog->GetDataReference());
	jObj["predefinedAttributes"] = jPredAttrs;

	// QuickChoice
	jObj["quickChoice"] = catalog->HasQuickChoice();

	// Modules
	json jModules = json::object();
	jModules["objectModule"] = SaveModuleToJSON(catalog->GetModuleObject());
	jModules["managerModule"] = SaveModuleToJSON(catalog->GetModuleManager());
	jObj["modules"] = jModules;

	// Default forms (as GUIDs)
	ibValueMetaObjectFormBase* defFormObj = catalog->GetDefaultFormByID(1);
	ibValueMetaObjectFormBase* defFormFolder = catalog->GetDefaultFormByID(4);
	ibValueMetaObjectFormBase* defFormList = catalog->GetDefaultFormByID(2);
	ibValueMetaObjectFormBase* defFormSelect = catalog->GetDefaultFormByID(3);

	json jDefForms = json::object();
	jDefForms["object"] = WxToStd(defFormObj ? defFormObj->GetGuid().str() : wxString());
	jDefForms["folder"] = WxToStd(defFormFolder ? defFormFolder->GetGuid().str() : wxString());
	jDefForms["list"] = WxToStd(defFormList ? defFormList->GetGuid().str() : wxString());
	jDefForms["select"] = WxToStd(defFormSelect ? defFormSelect->GetGuid().str() : wxString());
	jObj["defaultForms"] = jDefForms;

	// Generation property
	jObj["generation"] = SaveMetaDescriptionToJSON(catalog->GetGenerationDescription());

	// Predefined values
	jObj["predefinedValues"] = SavePredefinedValuesToJSON(catalog);

	// User attributes, tables, forms
	jObj["attributes"] = SaveUserAttributesToJSON(catalog);
	jObj["tabularSections"] = SaveUserTablesToJSON(catalog);
	jObj["forms"] = SaveFormsToJSON(catalog);
}

// ---- Document ----
void SaveDocumentToJSON(json& jObj, ibValueMetaObjectDocument* document)
{
	// Predefined attributes
	json jPredAttrs = json::object();
	jPredAttrs["number"] = SavePredefinedAttributeToJSON(document->GetDocumentNumber());
	jPredAttrs["date"] = SavePredefinedAttributeToJSON(document->GetDocumentDate());
	jPredAttrs["posted"] = SavePredefinedAttributeToJSON(document->GetDocumentPosted());
	jPredAttrs["dataVersion"] = SavePredefinedAttributeToJSON(document->GetDataVersion());
	jPredAttrs["deletionMark"] = SavePredefinedAttributeToJSON(document->GetDataDeletionMark());
	jPredAttrs["reference"] = SavePredefinedAttributeToJSON(document->GetDataReference());
	jObj["predefinedAttributes"] = jPredAttrs;

	// QuickChoice
	jObj["quickChoice"] = document->HasQuickChoice();

	// Modules
	json jModules = json::object();
	jModules["objectModule"] = SaveModuleToJSON(document->GetModuleObject());
	jModules["managerModule"] = SaveModuleToJSON(document->GetModuleManager());
	jObj["modules"] = jModules;

	// Default forms
	ibValueMetaObjectFormBase* defFormObj = document->GetDefaultFormByID(1);
	ibValueMetaObjectFormBase* defFormList = document->GetDefaultFormByID(2);
	ibValueMetaObjectFormBase* defFormSelect = document->GetDefaultFormByID(3);

	json jDefForms = json::object();
	jDefForms["object"] = WxToStd(defFormObj ? defFormObj->GetGuid().str() : wxString());
	jDefForms["list"] = WxToStd(defFormList ? defFormList->GetGuid().str() : wxString());
	jDefForms["select"] = WxToStd(defFormSelect ? defFormSelect->GetGuid().str() : wxString());
	jObj["defaultForms"] = jDefForms;

	// RegisterRecord property
	jObj["registerRecord"] = SaveMetaDescriptionToJSON(document->GetRecordDescription());

	// Generation property
	jObj["generation"] = SaveMetaDescriptionToJSON(document->GetGenerationDescription());

	// User attributes, tables, forms
	jObj["attributes"] = SaveUserAttributesToJSON(document);
	jObj["tabularSections"] = SaveUserTablesToJSON(document);
	jObj["forms"] = SaveFormsToJSON(document);
}

// ---- Enumeration ----
void SaveEnumerationToJSON(json& jObj, ibValueMetaObjectEnumeration* enumeration)
{
	// Predefined attributes
	json jPredAttrs = json::object();
	jPredAttrs["order"] = SavePredefinedAttributeToJSON(enumeration->GetDataOrder());
	jPredAttrs["reference"] = SavePredefinedAttributeToJSON(enumeration->GetDataReference());
	jObj["predefinedAttributes"] = jPredAttrs;

	// QuickChoice
	jObj["quickChoice"] = enumeration->HasQuickChoice();

	// Module
	json jModules = json::object();
	jModules["managerModule"] = SaveModuleToJSON(enumeration->GetModuleManager());
	jObj["modules"] = jModules;

	// Default forms
	ibValueMetaObjectFormBase* defFormList = enumeration->GetDefaultFormByID(2);
	ibValueMetaObjectFormBase* defFormSelect = enumeration->GetDefaultFormByID(3);

	json jDefForms = json::object();
	jDefForms["list"] = WxToStd(defFormList ? defFormList->GetGuid().str() : wxString());
	jDefForms["select"] = WxToStd(defFormSelect ? defFormSelect->GetGuid().str() : wxString());
	jObj["defaultForms"] = jDefForms;

	// Enum values
	jObj["enumValues"] = SaveEnumValuesToJSON(enumeration);

	// Forms
	jObj["forms"] = SaveFormsToJSON(enumeration);
}

// ---- Constant ----
void SaveConstantToJSON(json& jObj, ibValueMetaObjectConstant* constant)
{
	// Module
	json jModules = json::object();
	jModules["recordModule"] = SaveModuleToJSON(constant->GetModuleObject());
	jObj["modules"] = jModules;

	// The constant itself is an attribute - save its type description
	json jType = SaveTypeDescriptionToJSON(constant->GetTypeDesc());
	if (!jType.empty())
		jObj["type"] = jType;
}

// ---- InformationRegister ----
void SaveInformationRegisterToJSON(json& jObj, ibValueMetaObjectInformationRegister* infoReg)
{
	// Predefined attributes
	json jPredAttrs = json::object();
	jPredAttrs["lineActive"] = SavePredefinedAttributeToJSON(infoReg->GetRegisterActive());
	jPredAttrs["period"] = SavePredefinedAttributeToJSON(infoReg->GetRegisterPeriod());
	jPredAttrs["recorder"] = SavePredefinedAttributeToJSON(infoReg->GetRegisterRecorder());
	jPredAttrs["lineNumber"] = SavePredefinedAttributeToJSON(infoReg->GetRegisterLineNumber());
	jObj["predefinedAttributes"] = jPredAttrs;

	// WriteMode and Periodicity
	jObj["writeMode"] = (int)infoReg->GetWriteRegisterMode();
	jObj["periodicity"] = (int)infoReg->GetPeriodicity();

	// Modules
	json jModules = json::object();
	jModules["recordSetModule"] = SaveModuleToJSON(infoReg->GetModuleObject());
	jModules["managerModule"] = SaveModuleToJSON(infoReg->GetModuleManager());
	jObj["modules"] = jModules;

	// Default forms
	ibValueMetaObjectFormBase* defFormRecord = infoReg->GetDefaultFormByID(1);
	ibValueMetaObjectFormBase* defFormList = infoReg->GetDefaultFormByID(2);

	json jDefForms = json::object();
	jDefForms["record"] = WxToStd(defFormRecord ? defFormRecord->GetGuid().str() : wxString());
	jDefForms["list"] = WxToStd(defFormList ? defFormList->GetGuid().str() : wxString());
	jObj["defaultForms"] = jDefForms;

	// Dimensions, Resources, Attributes
	jObj["dimensions"] = SaveRegisterDimensionsToJSON(infoReg);
	jObj["resources"] = SaveRegisterResourcesToJSON(infoReg);
	jObj["attributes"] = SaveRegisterAttributesToJSON(infoReg);

	// Forms
	jObj["forms"] = SaveFormsToJSON(infoReg);
}

// ---- AccumulationRegister ----
void SaveAccumulationRegisterToJSON(json& jObj, ibValueMetaObjectAccumulationRegister* accReg)
{
	// Predefined attributes
	json jPredAttrs = json::object();
	jPredAttrs["recordType"] = SavePredefinedAttributeToJSON(accReg->GetRegisterRecordType());
	jPredAttrs["lineActive"] = SavePredefinedAttributeToJSON(accReg->GetRegisterActive());
	jPredAttrs["period"] = SavePredefinedAttributeToJSON(accReg->GetRegisterPeriod());
	jPredAttrs["recorder"] = SavePredefinedAttributeToJSON(accReg->GetRegisterRecorder());
	jPredAttrs["lineNumber"] = SavePredefinedAttributeToJSON(accReg->GetRegisterLineNumber());
	jObj["predefinedAttributes"] = jPredAttrs;

	// RegisterType
	jObj["registerType"] = (int)accReg->GetRegisterType();

	// Modules
	json jModules = json::object();
	jModules["recordSetModule"] = SaveModuleToJSON(accReg->GetModuleObject());
	jModules["managerModule"] = SaveModuleToJSON(accReg->GetModuleManager());
	jObj["modules"] = jModules;

	// Default forms
	ibValueMetaObjectFormBase* defFormList = accReg->GetDefaultFormByID(2);

	json jDefForms = json::object();
	jDefForms["list"] = WxToStd(defFormList ? defFormList->GetGuid().str() : wxString());
	jObj["defaultForms"] = jDefForms;

	// Dimensions, Resources, Attributes
	jObj["dimensions"] = SaveRegisterDimensionsToJSON(accReg);
	jObj["resources"] = SaveRegisterResourcesToJSON(accReg);
	jObj["attributes"] = SaveRegisterAttributesToJSON(accReg);

	// Forms
	jObj["forms"] = SaveFormsToJSON(accReg);
}

// ---- DataProcessor ----
void SaveDataProcessorToJSON(json& jObj, ibValueMetaObjectDataProcessor* dataProcessor)
{
	// Modules
	json jModules = json::object();
	jModules["objectModule"] = SaveModuleToJSON(dataProcessor->GetModuleObject());
	jModules["managerModule"] = SaveModuleToJSON(dataProcessor->GetModuleManager());
	jObj["modules"] = jModules;

	// Default form
	ibValueMetaObjectFormBase* defFormObj = dataProcessor->GetDefaultFormByID(1);

	json jDefForms = json::object();
	jDefForms["object"] = WxToStd(defFormObj ? defFormObj->GetGuid().str() : wxString());
	jObj["defaultForms"] = jDefForms;

	// User attributes, tables, forms
	jObj["attributes"] = SaveUserAttributesToJSON(dataProcessor);
	jObj["tabularSections"] = SaveUserTablesToJSON(dataProcessor);
	jObj["forms"] = SaveFormsToJSON(dataProcessor);
}

// ---- Report ----
void SaveReportToJSON(json& jObj, ibValueMetaObjectReport* report)
{
	// Modules
	json jModules = json::object();
	jModules["objectModule"] = SaveModuleToJSON(report->GetModuleObject());
	jModules["managerModule"] = SaveModuleToJSON(report->GetModuleManager());
	jObj["modules"] = jModules;

	// Default form
	ibValueMetaObjectFormBase* defFormObj = report->GetDefaultFormByID(1);

	json jDefForms = json::object();
	jDefForms["object"] = WxToStd(defFormObj ? defFormObj->GetGuid().str() : wxString());
	jObj["defaultForms"] = jDefForms;

	// User attributes, tables, forms
	jObj["attributes"] = SaveUserAttributesToJSON(report);
	jObj["tabularSections"] = SaveUserTablesToJSON(report);
	jObj["forms"] = SaveFormsToJSON(report);
}

// ---- ChartOfCharacteristicTypes ----
void SaveChartOfCharacteristicTypesToJSON(json& jObj, ibValueMetaObjectChartOfCharacteristicTypes* chrt)
{
	// Predefined attributes
	json jPredAttrs = json::object();
	jPredAttrs["predefinedName"] = SavePredefinedAttributeToJSON(chrt->GetDataPredefinedName());
	jPredAttrs["code"] = SavePredefinedAttributeToJSON(chrt->GetDataCode());
	jPredAttrs["description"] = SavePredefinedAttributeToJSON(chrt->GetDataDescription());
	jPredAttrs["parent"] = SavePredefinedAttributeToJSON(chrt->GetDataParent());
	jPredAttrs["isFolder"] = SavePredefinedAttributeToJSON(chrt->GetDataIsFolder());
	jPredAttrs["dataVersion"] = SavePredefinedAttributeToJSON(chrt->GetDataVersion());
	jPredAttrs["deletionMark"] = SavePredefinedAttributeToJSON(chrt->GetDataDeletionMark());
	jPredAttrs["reference"] = SavePredefinedAttributeToJSON(chrt->GetDataReference());
	jObj["predefinedAttributes"] = jPredAttrs;

	// QuickChoice
	jObj["quickChoice"] = chrt->HasQuickChoice();

	// Modules
	json jModules = json::object();
	jModules["objectModule"] = SaveModuleToJSON(chrt->GetModuleObject());
	jModules["managerModule"] = SaveModuleToJSON(chrt->GetModuleManager());
	jObj["modules"] = jModules;

	// Default forms
	ibValueMetaObjectFormBase* defFormObj = chrt->GetDefaultFormByID(1);
	ibValueMetaObjectFormBase* defFormFolder = chrt->GetDefaultFormByID(4);
	ibValueMetaObjectFormBase* defFormList = chrt->GetDefaultFormByID(2);
	ibValueMetaObjectFormBase* defFormSelect = chrt->GetDefaultFormByID(3);

	json jDefForms = json::object();
	jDefForms["object"] = WxToStd(defFormObj ? defFormObj->GetGuid().str() : wxString());
	jDefForms["folder"] = WxToStd(defFormFolder ? defFormFolder->GetGuid().str() : wxString());
	jDefForms["list"] = WxToStd(defFormList ? defFormList->GetGuid().str() : wxString());
	jDefForms["select"] = WxToStd(defFormSelect ? defFormSelect->GetGuid().str() : wxString());
	jObj["defaultForms"] = jDefForms;

	// Generation property
	jObj["generation"] = SaveMetaDescriptionToJSON(chrt->GetGenerationDescription());

	// Predefined values
	jObj["predefinedValues"] = SavePredefinedValuesToJSON(chrt);

	// User attributes, tables, forms
	jObj["attributes"] = SaveUserAttributesToJSON(chrt);
	jObj["tabularSections"] = SaveUserTablesToJSON(chrt);
	jObj["forms"] = SaveFormsToJSON(chrt);
}

// ---- ChartOfAccounts ----
void SaveChartOfAccountsToJSON(json& jObj, ibValueMetaObjectChartOfAccounts* chart)
{
	// Predefined attributes
	json jPredAttrs = json::object();
	jPredAttrs["predefinedName"] = SavePredefinedAttributeToJSON(chart->GetDataPredefinedName());
	jPredAttrs["code"] = SavePredefinedAttributeToJSON(chart->GetDataCode());
	jPredAttrs["description"] = SavePredefinedAttributeToJSON(chart->GetDataDescription());
	jPredAttrs["parent"] = SavePredefinedAttributeToJSON(chart->GetDataParent());
	jPredAttrs["isFolder"] = SavePredefinedAttributeToJSON(chart->GetDataIsFolder());
	jPredAttrs["accountType"] = SavePredefinedAttributeToJSON(chart->GetAccountType());
	jPredAttrs["offBalance"] = SavePredefinedAttributeToJSON(chart->GetOffBalance());
	jPredAttrs["quantitative"] = SavePredefinedAttributeToJSON(chart->GetQuantitative());
	jPredAttrs["currency"] = SavePredefinedAttributeToJSON(chart->GetCurrency());
	jPredAttrs["maxSubcontoCount"] = SavePredefinedAttributeToJSON(chart->GetMaxSubcontoCount());
	jPredAttrs["dataVersion"] = SavePredefinedAttributeToJSON(chart->GetDataVersion());
	jPredAttrs["deletionMark"] = SavePredefinedAttributeToJSON(chart->GetDataDeletionMark());
	jPredAttrs["reference"] = SavePredefinedAttributeToJSON(chart->GetDataReference());
	jObj["predefinedAttributes"] = jPredAttrs;

	// QuickChoice
	jObj["quickChoice"] = chart->HasQuickChoice();

	// Modules
	json jModules = json::object();
	jModules["objectModule"] = SaveModuleToJSON(chart->GetModuleObject());
	jModules["managerModule"] = SaveModuleToJSON(chart->GetModuleManager());
	jObj["modules"] = jModules;

	// Default forms
	ibValueMetaObjectFormBase* defFormObj = chart->GetDefaultFormByID(1);
	ibValueMetaObjectFormBase* defFormFolder = chart->GetDefaultFormByID(4);
	ibValueMetaObjectFormBase* defFormList = chart->GetDefaultFormByID(2);
	ibValueMetaObjectFormBase* defFormSelect = chart->GetDefaultFormByID(3);

	json jDefForms = json::object();
	jDefForms["object"] = WxToStd(defFormObj ? defFormObj->GetGuid().str() : wxString());
	jDefForms["folder"] = WxToStd(defFormFolder ? defFormFolder->GetGuid().str() : wxString());
	jDefForms["list"] = WxToStd(defFormList ? defFormList->GetGuid().str() : wxString());
	jDefForms["select"] = WxToStd(defFormSelect ? defFormSelect->GetGuid().str() : wxString());
	jObj["defaultForms"] = jDefForms;

	// ChartOfCharacteristicTypes owner property
	ibPropertyChartOfCharacteristicTypes* chrtProp = chart->GetChartOfCharacteristicTypes();
	if (chrtProp != nullptr) {
		jObj["chartOfCharacteristicTypes"] = SaveMetaDescriptionToJSON(chrtProp->GetValueAsMetaDesc());
	}

	// Generation property
	jObj["generation"] = SaveMetaDescriptionToJSON(chart->GetGenerationDescription());

	// Predefined values
	jObj["predefinedValues"] = SavePredefinedValuesToJSON(chart);

	// User attributes, tables, forms
	jObj["attributes"] = SaveUserAttributesToJSON(chart);
	jObj["tabularSections"] = SaveUserTablesToJSON(chart);
	jObj["forms"] = SaveFormsToJSON(chart);
}

// ---- AccountingRegister ----
void SaveAccountingRegisterToJSON(json& jObj, ibValueMetaObjectAccountingRegister* accReg)
{
	// Predefined attributes
	json jPredAttrs = json::object();
	jPredAttrs["recordType"] = SavePredefinedAttributeToJSON(accReg->GetRegisterRecordType());
	jPredAttrs["account"] = SavePredefinedAttributeToJSON(accReg->GetRegisterAccount());
	jPredAttrs["subconto1"] = SavePredefinedAttributeToJSON(accReg->GetRegisterSubconto1());
	jPredAttrs["subconto2"] = SavePredefinedAttributeToJSON(accReg->GetRegisterSubconto2());
	jPredAttrs["subconto3"] = SavePredefinedAttributeToJSON(accReg->GetRegisterSubconto3());
	jPredAttrs["lineActive"] = SavePredefinedAttributeToJSON(accReg->GetRegisterActive());
	jPredAttrs["period"] = SavePredefinedAttributeToJSON(accReg->GetRegisterPeriod());
	jPredAttrs["recorder"] = SavePredefinedAttributeToJSON(accReg->GetRegisterRecorder());
	jPredAttrs["lineNumber"] = SavePredefinedAttributeToJSON(accReg->GetRegisterLineNumber());
	jObj["predefinedAttributes"] = jPredAttrs;

	// Modules
	json jModules = json::object();
	jModules["recordSetModule"] = SaveModuleToJSON(accReg->GetModuleObject());
	jModules["managerModule"] = SaveModuleToJSON(accReg->GetModuleManager());
	jObj["modules"] = jModules;

	// Default forms
	ibValueMetaObjectFormBase* defFormList = accReg->GetDefaultFormByID(2);

	json jDefForms = json::object();
	jDefForms["list"] = WxToStd(defFormList ? defFormList->GetGuid().str() : wxString());
	jObj["defaultForms"] = jDefForms;

	// Dimensions, Resources, Attributes
	jObj["dimensions"] = SaveRegisterDimensionsToJSON(accReg);
	jObj["resources"] = SaveRegisterResourcesToJSON(accReg);
	jObj["attributes"] = SaveRegisterAttributesToJSON(accReg);

	// Forms
	jObj["forms"] = SaveFormsToJSON(accReg);
}

//***********************************************************************
//*                    Main Per-Object Dispatch                        *
//***********************************************************************

json SaveMetaObjectToJSON(ibValueMetaObject* metaObject)
{
	json jObj = json::object();
	jObj["guid"] = WxToStd(metaObject->GetGuid().str());
	jObj["id"] = metaObject->GetMetaID();
	jObj["clsid"] = (unsigned long long)metaObject->GetClassType();
	jObj["name"] = WxToStd(metaObject->GetName());

	const wxString synonym = metaObject->GetSynonym();
	if (!synonym.IsEmpty() && synonym != metaObject->GetName())
		jObj["synonym"] = WxToStd(synonym);

	if (!metaObject->GetComment().IsEmpty())
		jObj["comment"] = WxToStd(metaObject->GetComment());

	// Dispatch to type-specific serialization
	ibClassID clsid = metaObject->GetClassType();

	if (clsid == g_metaCatalogCLSID) {
		ibValueMetaObjectCatalog* catalog = dynamic_cast<ibValueMetaObjectCatalog*>(metaObject);
		if (catalog != nullptr)
			SaveCatalogToJSON(jObj, catalog);
	}
	else if (clsid == g_metaDocumentCLSID) {
		ibValueMetaObjectDocument* document = dynamic_cast<ibValueMetaObjectDocument*>(metaObject);
		if (document != nullptr)
			SaveDocumentToJSON(jObj, document);
	}
	else if (clsid == g_metaEnumerationCLSID) {
		ibValueMetaObjectEnumeration* enumeration = dynamic_cast<ibValueMetaObjectEnumeration*>(metaObject);
		if (enumeration != nullptr)
			SaveEnumerationToJSON(jObj, enumeration);
	}
	else if (clsid == g_metaConstantCLSID) {
		ibValueMetaObjectConstant* constant = dynamic_cast<ibValueMetaObjectConstant*>(metaObject);
		if (constant != nullptr)
			SaveConstantToJSON(jObj, constant);
	}
	else if (clsid == g_metaInformationRegisterCLSID) {
		ibValueMetaObjectInformationRegister* infoReg = dynamic_cast<ibValueMetaObjectInformationRegister*>(metaObject);
		if (infoReg != nullptr)
			SaveInformationRegisterToJSON(jObj, infoReg);
	}
	else if (clsid == g_metaAccumulationRegisterCLSID) {
		ibValueMetaObjectAccumulationRegister* accReg = dynamic_cast<ibValueMetaObjectAccumulationRegister*>(metaObject);
		if (accReg != nullptr)
			SaveAccumulationRegisterToJSON(jObj, accReg);
	}
	else if (clsid == g_metaDataProcessorCLSID) {
		ibValueMetaObjectDataProcessor* dataProcessor = dynamic_cast<ibValueMetaObjectDataProcessor*>(metaObject);
		if (dataProcessor != nullptr)
			SaveDataProcessorToJSON(jObj, dataProcessor);
	}
	else if (clsid == g_metaReportCLSID) {
		ibValueMetaObjectReport* report = dynamic_cast<ibValueMetaObjectReport*>(metaObject);
		if (report != nullptr)
			SaveReportToJSON(jObj, report);
	}
	else if (clsid == g_metaChartOfCharacteristicTypesCLSID) {
		ibValueMetaObjectChartOfCharacteristicTypes* chrt = dynamic_cast<ibValueMetaObjectChartOfCharacteristicTypes*>(metaObject);
		if (chrt != nullptr)
			SaveChartOfCharacteristicTypesToJSON(jObj, chrt);
	}
	else if (clsid == g_metaChartOfAccountsCLSID) {
		ibValueMetaObjectChartOfAccounts* chart = dynamic_cast<ibValueMetaObjectChartOfAccounts*>(metaObject);
		if (chart != nullptr)
			SaveChartOfAccountsToJSON(jObj, chart);
	}
	else if (clsid == g_metaAccountingRegisterCLSID) {
		ibValueMetaObjectAccountingRegister* accReg = dynamic_cast<ibValueMetaObjectAccountingRegister*>(metaObject);
		if (accReg != nullptr)
			SaveAccountingRegisterToJSON(jObj, accReg);
	}
	else if (clsid == g_metaCommonModuleCLSID) {
		// Common module
		ibValueMetaObjectCommonModule* commonModule = dynamic_cast<ibValueMetaObjectCommonModule*>(metaObject);
		if (commonModule != nullptr) {
			jObj["module"] = WxToStd(commonModule->GetModuleText());
			jObj["globalModule"] = commonModule->IsGlobalModule();
		}
	}
	else if (clsid == g_metaCommonFormCLSID) {
		// Common form
		ibValueMetaObjectCommonForm* commonForm = dynamic_cast<ibValueMetaObjectCommonForm*>(metaObject);
		if (commonForm != nullptr) {
			wxMemoryBuffer formData = commonForm->GetFormData();
			if (formData.GetDataLen() > 0)
				jObj["formData"] = WxToStd(wxBase64Encode(formData));
			const wxString moduleText = commonForm->GetModuleText();
			if (!moduleText.IsEmpty())
				jObj["module"] = WxToStd(moduleText);
		}
	}
	else {
		// Generic fallback: try to save any record-based data
		ibValueMetaObjectRecordData* recordData = dynamic_cast<ibValueMetaObjectRecordData*>(metaObject);
		if (recordData != nullptr) {
			jObj["attributes"] = SaveUserAttributesToJSON(recordData);
			jObj["tabularSections"] = SaveUserTablesToJSON(recordData);

			ibValueMetaObjectGenericData* genericData = dynamic_cast<ibValueMetaObjectGenericData*>(metaObject);
			if (genericData != nullptr)
				jObj["forms"] = SaveFormsToJSON(genericData);
		}

		// Try predefined values
		ibValueMetaObjectRecordDataHierarchyMutableRef* hierarchyObj = dynamic_cast<ibValueMetaObjectRecordDataHierarchyMutableRef*>(metaObject);
		if (hierarchyObj != nullptr)
			jObj["predefinedValues"] = SavePredefinedValuesToJSON(hierarchyObj);

		// Try module code
		ibValueMetaObjectCommonModule* commonModule = dynamic_cast<ibValueMetaObjectCommonModule*>(metaObject);
		if (commonModule != nullptr) {
			const wxString text = commonModule->GetModuleText();
			if (!text.IsEmpty())
				jObj["module"] = WxToStd(text);
		}
	}

	return jObj;
}

} // anonymous namespace

//***********************************************************************
//*                    Save Configuration to JSON                      *
//***********************************************************************

bool ibMetaDataConfigurationBase::SaveConfigToJSON(const wxString& strFileName)
{
	ibValueMetaObjectConfiguration* commonMetaObject = GetCommonMetaObject();
	if (commonMetaObject == nullptr)
		return false;

	json root = json::object();
	root["format"] = "OES-JSON-1.0";
	root["guid"] = WxToStd(GetConfigGuid().str());
	root["version"] = 2;
	root["name"] = WxToStd(commonMetaObject->GetName());

	const wxString synonym = commonMetaObject->GetSynonym();
	if (!synonym.IsEmpty())
		root["synonym"] = WxToStd(synonym);

	// Configuration module
	ibValueMetaObjectModule* configModule = commonMetaObject->GetModuleObject();
	if (configModule != nullptr) {
		const wxString moduleText = configModule->GetModuleText();
		if (!moduleText.IsEmpty())
			root["configurationModule"] = WxToStd(moduleText);
	}

	// Common modules
	auto commonModules = GetAnyArrayObject(g_metaCommonModuleCLSID);
	if (!commonModules.empty()) {
		json jGroup = json::array();
		for (auto obj : commonModules) {
			if (obj->IsDeleted()) continue;
			jGroup.push_back(SaveMetaObjectToJSON(obj));
		}
		root["commonModules"] = jGroup;
	}

	// Common forms
	auto commonForms = GetAnyArrayObject(g_metaCommonFormCLSID);
	if (!commonForms.empty()) {
		json jGroup = json::array();
		for (auto obj : commonForms) {
			if (obj->IsDeleted()) continue;
			jGroup.push_back(SaveMetaObjectToJSON(obj));
		}
		root["commonForms"] = jGroup;
	}

	// Business object groups
	struct TypeGroup {
		ibClassID clsid;
		std::string jsonKey;
	};

	const TypeGroup groups[] = {
		{ g_metaConstantCLSID, "constants" },
		{ g_metaCatalogCLSID, "catalogs" },
		{ g_metaDocumentCLSID, "documents" },
		{ g_metaEnumerationCLSID, "enumerations" },
		{ g_metaDataProcessorCLSID, "dataProcessors" },
		{ g_metaReportCLSID, "reports" },
		{ g_metaInformationRegisterCLSID, "informationRegisters" },
		{ g_metaAccumulationRegisterCLSID, "accumulationRegisters" },
		{ g_metaChartOfCharacteristicTypesCLSID, "chartsOfCharacteristicTypes" },
		{ g_metaChartOfAccountsCLSID, "chartsOfAccounts" },
		{ g_metaAccountingRegisterCLSID, "accountingRegisters" },
	};

	for (const auto& group : groups) {
		auto objects = GetAnyArrayObject(group.clsid);
		if (objects.empty())
			continue;

		json jGroup = json::array();
		for (auto obj : objects) {
			if (obj->IsDeleted()) continue;
			jGroup.push_back(SaveMetaObjectToJSON(obj));
		}
		root[group.jsonKey] = jGroup;
	}

	// Write to file with pretty-printing (indent=2)
	std::string jsonStr = root.dump(2);

	wxFileOutputStream stream(strFileName);
	if (!stream.IsOk())
		return false;

	stream.Write(jsonStr.c_str(), jsonStr.size());
	return stream.IsOk();
}

//***********************************************************************
//*                    Load Configuration from JSON                    *
//***********************************************************************

namespace {

bool LoadMetaDescriptionFromJSON(const json& jDesc, ibMetaDescription& metaDesc)
{
	if (!jDesc.is_array())
		return false;

	metaDesc.ClearMetaType();

	for (const auto& jId : jDesc) {
		if (jId.is_number())
			metaDesc.AppendMetaType((ibMetaID)jId.get<int>());
	}

	return true;
}

bool LoadTypeDescriptionFromJSON(const json& jType, ibTypeDescription& typeDesc)
{
	if (jType.empty() || !jType.is_object())
		return false;

	typeDesc.ClearMetaType();

	if (jType.contains("typeIds") && jType["typeIds"].is_array()) {
		for (const auto& jClsid : jType["typeIds"]) {
			if (jClsid.is_number_unsigned()) {
				typeDesc.AppendMetaType((ibClassID)jClsid.get<unsigned long long>());
			}
			else if (jClsid.is_number()) {
				typeDesc.AppendMetaType((ibClassID)jClsid.get<long long>());
			}
			else if (jClsid.is_string()) {
				// Fallback: try parsing as numeric string
				wxString clsidStr = StdToWx(jClsid.get<std::string>());
				if (!clsidStr.IsEmpty()) {
					unsigned long long clsidVal = 0;
					clsidStr.ToULongLong(&clsidVal);
					typeDesc.AppendMetaType((ibClassID)clsidVal);
				}
			}
		}
	}

	// Number qualifiers
	if (jType.contains("precision") || jType.contains("scale")) {
		int prec = jType.value("precision", 10);
		int scale = jType.value("scale", 0);
		bool nonNeg = jType.value("nonNegative", false);
		typeDesc.m_typeData.SetNumber((unsigned char)prec, (unsigned char)scale, nonNeg);
	}

	// String qualifiers
	if (jType.contains("length")) {
		int length = jType.value("length", 10);
		int alVal = jType.value("allowedLength", 0);
		ibAllowedLength al = (ibAllowedLength)alVal;
		typeDesc.m_typeData.SetString((unsigned short)length, al);
	}

	// Date qualifiers
	if (jType.contains("dateFractions")) {
		int dfVal = jType.value("dateFractions", 1);
		typeDesc.m_typeData.SetDate((ibDateFractions)dfVal);
	}

	return true;
}

void LoadAttributeFromJSON(const json& jAttr, ibMetaData* metaData, ibValueMetaObject* parent)
{
	std::string name = jAttr.value("name", "");
	if (name.empty()) return;

	ibValueMetaObject* attrObj = metaData->CreateMetaObject(g_metaAttributeCLSID, parent, false);
	if (attrObj == nullptr) return;

	attrObj->SetName(StdToWx(name));

	if (jAttr.contains("synonym"))
		attrObj->SetSynonym(StdToWx(jAttr["synonym"].get<std::string>()));

	if (jAttr.contains("guid")) {
		std::string guid = jAttr["guid"].get<std::string>();
		if (!guid.empty())
			attrObj->SetCommonGuid(ibGuid(StdToWx(guid)));
	}

	// Load type description
	if (jAttr.contains("type")) {
		ibValueMetaObjectAttribute* typedAttr = dynamic_cast<ibValueMetaObjectAttribute*>(attrObj);
		if (typedAttr != nullptr) {
			ibTypeDescription typeDesc;
			if (LoadTypeDescriptionFromJSON(jAttr["type"], typeDesc))
				typedAttr->GetTypeDesc().LoadMetaType(typeDesc);
		}
	}

	// FillCheck
	if (jAttr.contains("fillCheck")) {
		ibProperty* prop = attrObj->GetProperty(wxT("FillCheck"));
		if (prop != nullptr)
			prop->SetValue(wxVariant(jAttr["fillCheck"].get<bool>()));
	}

	// ItemMode
	if (jAttr.contains("itemMode")) {
		ibProperty* prop = attrObj->GetProperty(wxT("ItemMode"));
		if (prop != nullptr)
			prop->SetValue(wxVariant((long)jAttr["itemMode"].get<int>()));
	}

	// SelectMode
	if (jAttr.contains("selectMode")) {
		ibProperty* prop = attrObj->GetProperty(wxT("Select"));
		if (prop != nullptr)
			prop->SetValue(wxVariant((long)jAttr["selectMode"].get<int>()));
	}
}

void LoadDimensionFromJSON(const json& jDim, ibMetaData* metaData, ibValueMetaObject* parent)
{
	std::string name = jDim.value("name", "");
	if (name.empty()) return;

	ibValueMetaObject* dimObj = metaData->CreateMetaObject(g_metaDimensionCLSID, parent, false);
	if (dimObj == nullptr) return;

	dimObj->SetName(StdToWx(name));

	if (jDim.contains("synonym"))
		dimObj->SetSynonym(StdToWx(jDim["synonym"].get<std::string>()));

	if (jDim.contains("guid")) {
		std::string guid = jDim["guid"].get<std::string>();
		if (!guid.empty())
			dimObj->SetCommonGuid(ibGuid(StdToWx(guid)));
	}

	if (jDim.contains("type")) {
		ibValueMetaObjectDimension* typedDim = dynamic_cast<ibValueMetaObjectDimension*>(dimObj);
		if (typedDim != nullptr) {
			ibTypeDescription typeDesc;
			if (LoadTypeDescriptionFromJSON(jDim["type"], typeDesc))
				typedDim->GetTypeDesc().LoadMetaType(typeDesc);
		}
	}

	// FillCheck
	if (jDim.contains("fillCheck")) {
		ibProperty* prop = dimObj->GetProperty(wxT("FillCheck"));
		if (prop != nullptr)
			prop->SetValue(wxVariant(jDim["fillCheck"].get<bool>()));
	}

	// ItemMode
	if (jDim.contains("itemMode")) {
		ibProperty* prop = dimObj->GetProperty(wxT("ItemMode"));
		if (prop != nullptr)
			prop->SetValue(wxVariant((long)jDim["itemMode"].get<int>()));
	}

	// SelectMode
	if (jDim.contains("selectMode")) {
		ibProperty* prop = dimObj->GetProperty(wxT("Select"));
		if (prop != nullptr)
			prop->SetValue(wxVariant((long)jDim["selectMode"].get<int>()));
	}
}

void LoadResourceFromJSON(const json& jRes, ibMetaData* metaData, ibValueMetaObject* parent)
{
	std::string name = jRes.value("name", "");
	if (name.empty()) return;

	ibValueMetaObject* resObj = metaData->CreateMetaObject(g_metaResourceCLSID, parent, false);
	if (resObj == nullptr) return;

	resObj->SetName(StdToWx(name));

	if (jRes.contains("synonym"))
		resObj->SetSynonym(StdToWx(jRes["synonym"].get<std::string>()));

	if (jRes.contains("guid")) {
		std::string guid = jRes["guid"].get<std::string>();
		if (!guid.empty())
			resObj->SetCommonGuid(ibGuid(StdToWx(guid)));
	}

	if (jRes.contains("type")) {
		ibValueMetaObjectResource* typedRes = dynamic_cast<ibValueMetaObjectResource*>(resObj);
		if (typedRes != nullptr) {
			ibTypeDescription typeDesc;
			if (LoadTypeDescriptionFromJSON(jRes["type"], typeDesc))
				typedRes->GetTypeDesc().LoadMetaType(typeDesc);
		}
	}

	// FillCheck
	if (jRes.contains("fillCheck")) {
		ibProperty* prop = resObj->GetProperty(wxT("FillCheck"));
		if (prop != nullptr)
			prop->SetValue(wxVariant(jRes["fillCheck"].get<bool>()));
	}

	// ItemMode
	if (jRes.contains("itemMode")) {
		ibProperty* prop = resObj->GetProperty(wxT("ItemMode"));
		if (prop != nullptr)
			prop->SetValue(wxVariant((long)jRes["itemMode"].get<int>()));
	}

	// SelectMode
	if (jRes.contains("selectMode")) {
		ibProperty* prop = resObj->GetProperty(wxT("Select"));
		if (prop != nullptr)
			prop->SetValue(wxVariant((long)jRes["selectMode"].get<int>()));
	}
}

void LoadTableFromJSON(const json& jTable, ibMetaData* metaData, ibValueMetaObject* parent)
{
	std::string name = jTable.value("name", "");
	if (name.empty()) return;

	ibValueMetaObject* tableObj = metaData->CreateMetaObject(g_metaTableCLSID, parent, false);
	if (tableObj == nullptr) return;

	tableObj->SetName(StdToWx(name));

	if (jTable.contains("synonym"))
		tableObj->SetSynonym(StdToWx(jTable["synonym"].get<std::string>()));

	if (jTable.contains("guid")) {
		std::string guid = jTable["guid"].get<std::string>();
		if (!guid.empty())
			tableObj->SetCommonGuid(ibGuid(StdToWx(guid)));
	}

	// Load table columns
	if (jTable.contains("attributes") && jTable["attributes"].is_array()) {
		for (const auto& jAttr : jTable["attributes"]) {
			LoadAttributeFromJSON(jAttr, metaData, tableObj);
		}
	}
}

void LoadEnumValueFromJSON(const json& jEnum, ibMetaData* metaData, ibValueMetaObject* parent)
{
	std::string name = jEnum.value("name", "");
	if (name.empty()) return;

	ibValueMetaObject* enumObj = metaData->CreateMetaObject(g_metaEnumCLSID, parent, false);
	if (enumObj == nullptr) return;

	enumObj->SetName(StdToWx(name));

	if (jEnum.contains("synonym"))
		enumObj->SetSynonym(StdToWx(jEnum["synonym"].get<std::string>()));

	if (jEnum.contains("guid")) {
		std::string guid = jEnum["guid"].get<std::string>();
		if (!guid.empty())
			enumObj->SetCommonGuid(ibGuid(StdToWx(guid)));
	}
}

bool LoadMetaObjectFromJSON(const json& jObj, ibMetaData* metaData, const ibClassID& clsid, ibValueMetaObjectConfiguration* configObj)
{
	std::string name = jObj.value("name", "");
	if (name.empty()) return false;

	ibValueMetaObject* newObj = metaData->CreateMetaObject(clsid, configObj, false);
	if (newObj == nullptr) return false;

	newObj->SetName(StdToWx(name));

	if (jObj.contains("synonym"))
		newObj->SetSynonym(StdToWx(jObj["synonym"].get<std::string>()));

	if (jObj.contains("comment"))
		newObj->SetComment(StdToWx(jObj["comment"].get<std::string>()));

	if (jObj.contains("guid")) {
		std::string guid = jObj["guid"].get<std::string>();
		if (!guid.empty())
			newObj->SetCommonGuid(ibGuid(StdToWx(guid)));
	}

	// Load user-defined attributes
	if (jObj.contains("attributes") && jObj["attributes"].is_array()) {
		for (const auto& jAttr : jObj["attributes"]) {
			LoadAttributeFromJSON(jAttr, metaData, newObj);
		}
	}

	// Load dimensions (for registers)
	if (jObj.contains("dimensions") && jObj["dimensions"].is_array()) {
		for (const auto& jDim : jObj["dimensions"]) {
			LoadDimensionFromJSON(jDim, metaData, newObj);
		}
	}

	// Load resources (for registers)
	if (jObj.contains("resources") && jObj["resources"].is_array()) {
		for (const auto& jRes : jObj["resources"]) {
			LoadResourceFromJSON(jRes, metaData, newObj);
		}
	}

	// Load tabular sections
	if (jObj.contains("tabularSections") && jObj["tabularSections"].is_array()) {
		for (const auto& jTable : jObj["tabularSections"]) {
			LoadTableFromJSON(jTable, metaData, newObj);
		}
	}

	// Load enum values (for Enumeration)
	if (jObj.contains("enumValues") && jObj["enumValues"].is_array()) {
		for (const auto& jEnum : jObj["enumValues"]) {
			LoadEnumValueFromJSON(jEnum, metaData, newObj);
		}
	}

	// Load forms
	if (jObj.contains("forms") && jObj["forms"].is_array()) {
		for (const auto& jForm : jObj["forms"]) {
			std::string formName = jForm.value("name", "");
			if (!formName.empty()) {
				ibValueMetaObject* formObj = metaData->CreateMetaObject(g_metaFormCLSID, newObj, false);
				if (formObj != nullptr) {
					formObj->SetName(StdToWx(formName));

					if (jForm.contains("guid")) {
						std::string formGuid = jForm["guid"].get<std::string>();
						if (!formGuid.empty())
							formObj->SetCommonGuid(ibGuid(StdToWx(formGuid)));
					}

					if (jForm.contains("synonym"))
						formObj->SetSynonym(StdToWx(jForm["synonym"].get<std::string>()));

					// Form type
					if (jForm.contains("type")) {
						ibProperty* prop = formObj->GetProperty(wxT("FormType"));
						if (prop != nullptr)
							prop->SetValue(wxVariant((long)jForm["type"].get<int>()));
					}

					// Form data (base64 binary layout)
					if (jForm.contains("formData")) {
						ibValueMetaObjectFormBase* formBase = dynamic_cast<ibValueMetaObjectFormBase*>(formObj);
						if (formBase != nullptr)
							formBase->SetFormData(wxBase64Decode(StdToWx(jForm["formData"].get<std::string>())));
					}

					// Form module
					if (jForm.contains("module")) {
						ibValueMetaObjectFormBase* formBase = dynamic_cast<ibValueMetaObjectFormBase*>(formObj);
						if (formBase != nullptr)
							formBase->SetModuleText(StdToWx(jForm["module"].get<std::string>()));
					}
				}
			}
		}
	}

	// Load module code (for CommonModule)
	if (jObj.contains("module")) {
		ibValueMetaObjectCommonModule* commonModule = dynamic_cast<ibValueMetaObjectCommonModule*>(newObj);
		if (commonModule != nullptr) {
			commonModule->SetModuleText(StdToWx(jObj["module"].get<std::string>()));
		}
	}

	// Load globalModule flag (for CommonModule)
	if (jObj.contains("globalModule")) {
		ibProperty* prop = newObj->GetProperty(wxT("GlobalModule"));
		if (prop != nullptr)
			prop->SetValue(wxVariant(jObj["globalModule"].get<bool>()));
	}

	// Load form data (for CommonForm)
	if (clsid == g_metaCommonFormCLSID) {
		ibValueMetaObjectCommonForm* commonForm = dynamic_cast<ibValueMetaObjectCommonForm*>(newObj);
		if (commonForm != nullptr) {
			if (jObj.contains("formData")) {
				commonForm->SetFormData(wxBase64Decode(StdToWx(jObj["formData"].get<std::string>())));
			}
			if (jObj.contains("module")) {
				commonForm->SetModuleText(StdToWx(jObj["module"].get<std::string>()));
			}
		}
	}

	// Load modules section (for business objects)
	if (jObj.contains("modules") && jObj["modules"].is_object()) {
		const json& jModules = jObj["modules"];

		// Object module / RecordSet module / Record module
		std::string objModText;
		if (jModules.contains("objectModule"))
			objModText = jModules["objectModule"].get<std::string>();
		else if (jModules.contains("recordSetModule"))
			objModText = jModules["recordSetModule"].get<std::string>();
		else if (jModules.contains("recordModule"))
			objModText = jModules["recordModule"].get<std::string>();

		if (!objModText.empty()) {
			ibValueMetaObjectRecordData* recordData = dynamic_cast<ibValueMetaObjectRecordData*>(newObj);
			if (recordData != nullptr) {
				ibValueMetaObjectModule* moduleObj = recordData->GetModuleObject();
				if (moduleObj != nullptr)
					moduleObj->SetModuleText(StdToWx(objModText));
			}
			else {
				// Constant module
				ibValueMetaObjectConstant* constant = dynamic_cast<ibValueMetaObjectConstant*>(newObj);
				if (constant != nullptr) {
					ibValueMetaObjectModule* moduleObj = constant->GetModuleObject();
					if (moduleObj != nullptr)
						moduleObj->SetModuleText(StdToWx(objModText));
				}
			}
		}

		// Manager module
		if (jModules.contains("managerModule")) {
			std::string mgrModText = jModules["managerModule"].get<std::string>();
			if (!mgrModText.empty()) {
				ibValueMetaObjectRecordData* recordData = dynamic_cast<ibValueMetaObjectRecordData*>(newObj);
				if (recordData != nullptr) {
					ibValueMetaObjectCommonModule* managerObj = recordData->GetModuleManager();
					if (managerObj != nullptr)
						managerObj->SetModuleText(StdToWx(mgrModText));
				}
			}
		}
	}

	// Load predefined values (for hierarchical objects)
	if (jObj.contains("predefinedValues") && jObj["predefinedValues"].is_array()) {
		ibValueMetaObjectRecordDataHierarchyMutableRef* hierarchyObj =
			dynamic_cast<ibValueMetaObjectRecordDataHierarchyMutableRef*>(newObj);
		if (hierarchyObj != nullptr) {
			for (const auto& jPv : jObj["predefinedValues"]) {
				std::string pvName = jPv.value("name", "");
				std::string pvCode = jPv.value("code", "");
				std::string pvDescription = jPv.value("description", "");
				std::string pvParent = jPv.value("parent", "");
				bool isFolder = jPv.value("isFolder", false);

				// Find parent by name if specified
				wxObjectDataPtr<ibValueMetaObjectRecordDataHierarchyMutableRef::ibPredefinedValueObject> parentObj;
				if (!pvParent.empty()) {
					parentObj = hierarchyObj->FindPredefinedValue(StdToWx(pvParent));
				}

				if (jPv.contains("guid")) {
					std::string pvGuid = jPv["guid"].get<std::string>();
					if (!pvGuid.empty()) {
						hierarchyObj->SetPredefinedValue(
							ibGuid(StdToWx(pvGuid)),
							StdToWx(pvName), StdToWx(pvCode), StdToWx(pvDescription),
							isFolder, parentObj
						);
					}
					else {
						hierarchyObj->AppendPredefinedValue(
							StdToWx(pvName), StdToWx(pvCode), StdToWx(pvDescription),
							isFolder, parentObj
						);
					}
				}
				else {
					hierarchyObj->AppendPredefinedValue(
						StdToWx(pvName), StdToWx(pvCode), StdToWx(pvDescription),
						isFolder, parentObj
					);
				}
			}
		}
	}

	// Load constant type (for Constants)
	if (clsid == g_metaConstantCLSID && jObj.contains("type")) {
		ibValueMetaObjectConstant* constant = dynamic_cast<ibValueMetaObjectConstant*>(newObj);
		if (constant != nullptr) {
			ibTypeDescription typeDesc;
			if (LoadTypeDescriptionFromJSON(jObj["type"], typeDesc))
				constant->GetTypeDesc().LoadMetaType(typeDesc);
		}
	}

	// QuickChoice (for Catalog, Document, Enumeration, ChartOfCharacteristicTypes, ChartOfAccounts)
	if (jObj.contains("quickChoice")) {
		ibProperty* prop = newObj->GetProperty(wxT("QuickChoice"));
		if (prop != nullptr)
			prop->SetValue(wxVariant(jObj["quickChoice"].get<bool>()));
	}

	// WriteMode (for InformationRegister)
	if (jObj.contains("writeMode")) {
		ibProperty* prop = newObj->GetProperty(wxT("WriteMode"));
		if (prop != nullptr)
			prop->SetValue(wxVariant((long)jObj["writeMode"].get<int>()));
	}

	// Periodicity (for InformationRegister)
	if (jObj.contains("periodicity")) {
		ibProperty* prop = newObj->GetProperty(wxT("Periodicity"));
		if (prop != nullptr)
			prop->SetValue(wxVariant((long)jObj["periodicity"].get<int>()));
	}

	// RegisterType (for AccumulationRegister)
	if (jObj.contains("registerType")) {
		ibProperty* prop = newObj->GetProperty(wxT("RegisterType"));
		if (prop != nullptr)
			prop->SetValue(wxVariant((long)jObj["registerType"].get<int>()));
	}

	// Generation MetaDescription binding
	if (jObj.contains("generation") && jObj["generation"].is_array()) {
		ibMetaDescription metaDesc;
		if (LoadMetaDescriptionFromJSON(jObj["generation"], metaDesc)) {
			ibPropertyGeneration* genProp = dynamic_cast<ibPropertyGeneration*>(
				newObj->GetProperty(wxT("ListGeneration"))
			);
			if (genProp != nullptr)
				genProp->SetValue(metaDesc);
		}
	}

	// RegisterRecord MetaDescription binding (for Document)
	if (jObj.contains("registerRecord") && jObj["registerRecord"].is_array()) {
		ibMetaDescription metaDesc;
		if (LoadMetaDescriptionFromJSON(jObj["registerRecord"], metaDesc)) {
			ibPropertyRecord* recProp = dynamic_cast<ibPropertyRecord*>(
				newObj->GetProperty(wxT("ListRegisterRecord"))
			);
			if (recProp != nullptr)
				recProp->SetValue(metaDesc);
		}
	}

	// ChartOfCharacteristicTypes MetaDescription binding (for ChartOfAccounts)
	if (jObj.contains("chartOfCharacteristicTypes") && jObj["chartOfCharacteristicTypes"].is_array()) {
		ibMetaDescription metaDesc;
		if (LoadMetaDescriptionFromJSON(jObj["chartOfCharacteristicTypes"], metaDesc)) {
			ibPropertyChartOfCharacteristicTypes* chrtProp = dynamic_cast<ibPropertyChartOfCharacteristicTypes*>(
				newObj->GetProperty(wxT("ChartOfCharacteristicTypes"))
			);
			if (chrtProp != nullptr)
				chrtProp->SetValue(metaDesc);
		}
	}

	// Default forms - resolve GUID strings to metaIDs after all forms are created
	if (jObj.contains("defaultForms") && jObj["defaultForms"].is_object()) {
		const json& jDefForms = jObj["defaultForms"];
		ibValueMetaObjectCompositeData* compositeObj = dynamic_cast<ibValueMetaObjectCompositeData*>(newObj);
		if (compositeObj != nullptr) {
			// DefaultFormObject
			if (jDefForms.contains("object")) {
				std::string guidStr = jDefForms["object"].get<std::string>();
				if (!guidStr.empty()) {
					ibMetaID formId = compositeObj->GetIdByGuid(ibGuid(StdToWx(guidStr)));
					if (formId != wxNOT_FOUND) {
						ibProperty* prop = newObj->GetProperty(wxT("DefaultFormObject"));
						if (prop != nullptr)
							prop->SetValue(wxVariant((long)formId));
					}
				}
			}

			// DefaultFormFolder
			if (jDefForms.contains("folder")) {
				std::string guidStr = jDefForms["folder"].get<std::string>();
				if (!guidStr.empty()) {
					ibMetaID formId = compositeObj->GetIdByGuid(ibGuid(StdToWx(guidStr)));
					if (formId != wxNOT_FOUND) {
						ibProperty* prop = newObj->GetProperty(wxT("DefaultFormFolder"));
						if (prop != nullptr)
							prop->SetValue(wxVariant((long)formId));
					}
				}
			}

			// DefaultFormList
			if (jDefForms.contains("list")) {
				std::string guidStr = jDefForms["list"].get<std::string>();
				if (!guidStr.empty()) {
					ibMetaID formId = compositeObj->GetIdByGuid(ibGuid(StdToWx(guidStr)));
					if (formId != wxNOT_FOUND) {
						ibProperty* prop = newObj->GetProperty(wxT("DefaultFormList"));
						if (prop != nullptr)
							prop->SetValue(wxVariant((long)formId));
					}
				}
			}

			// DefaultFormSelect
			if (jDefForms.contains("select")) {
				std::string guidStr = jDefForms["select"].get<std::string>();
				if (!guidStr.empty()) {
					ibMetaID formId = compositeObj->GetIdByGuid(ibGuid(StdToWx(guidStr)));
					if (formId != wxNOT_FOUND) {
						ibProperty* prop = newObj->GetProperty(wxT("DefaultFormSelect"));
						if (prop != nullptr)
							prop->SetValue(wxVariant((long)formId));
					}
				}
			}

			// DefaultFormRecord (for InformationRegister)
			if (jDefForms.contains("record")) {
				std::string guidStr = jDefForms["record"].get<std::string>();
				if (!guidStr.empty()) {
					ibMetaID formId = compositeObj->GetIdByGuid(ibGuid(StdToWx(guidStr)));
					if (formId != wxNOT_FOUND) {
						ibProperty* prop = newObj->GetProperty(wxT("DefaultFormRecord"));
						if (prop != nullptr)
							prop->SetValue(wxVariant((long)formId));
					}
				}
			}
		}
	}

	return true;
}

} // anonymous namespace (import helpers)

bool ibMetaDataConfigurationBase::LoadConfigFromJSON(const wxString& strFileName)
{
	wxFileInputStream stream(strFileName);
	if (!stream.IsOk())
		return false;

	// Read entire file into a string
	size_t fileSize = stream.GetSize();
	std::string fileContent(fileSize, '\0');
	stream.Read(&fileContent[0], fileSize);

	json root;
	try {
		root = json::parse(fileContent);
	}
	catch (const json::parse_error&) {
		return false;
	}

	if (!root.is_object())
		return false;

	ibValueMetaObjectConfiguration* configObj = GetCommonMetaObject();
	if (configObj == nullptr)
		return false;

	// Set configuration name
	if (root.contains("name")) {
		wxString configName = StdToWx(root["name"].get<std::string>());
		if (!configName.IsEmpty())
			configObj->SetName(configName);
	}

	if (root.contains("synonym")) {
		wxString configSynonym = StdToWx(root["synonym"].get<std::string>());
		if (!configSynonym.IsEmpty())
			configObj->SetSynonym(configSynonym);
	}

	// Load configuration module
	if (root.contains("configurationModule")) {
		ibValueMetaObjectModule* configModule = configObj->GetModuleObject();
		if (configModule != nullptr)
			configModule->SetModuleText(StdToWx(root["configurationModule"].get<std::string>()));
	}

	// CLSID mapping for JSON keys
	struct TypeMapping {
		std::string jsonKey;
		ibClassID clsid;
	};

	const TypeMapping mappings[] = {
		{ "commonModules", g_metaCommonModuleCLSID },
		{ "commonForms", g_metaCommonFormCLSID },
		{ "constants", g_metaConstantCLSID },
		{ "catalogs", g_metaCatalogCLSID },
		{ "documents", g_metaDocumentCLSID },
		{ "enumerations", g_metaEnumerationCLSID },
		{ "dataProcessors", g_metaDataProcessorCLSID },
		{ "reports", g_metaReportCLSID },
		{ "informationRegisters", g_metaInformationRegisterCLSID },
		{ "accumulationRegisters", g_metaAccumulationRegisterCLSID },
		{ "chartsOfCharacteristicTypes", g_metaChartOfCharacteristicTypesCLSID },
		{ "chartsOfAccounts", g_metaChartOfAccountsCLSID },
		{ "accountingRegisters", g_metaAccountingRegisterCLSID },
	};

	// Parse each group
	for (const auto& mapping : mappings) {
		if (root.contains(mapping.jsonKey) && root[mapping.jsonKey].is_array()) {
			for (const auto& jItem : root[mapping.jsonKey]) {
				LoadMetaObjectFromJSON(jItem, this, mapping.clsid, configObj);
			}
		}
	}

	return true;
}
