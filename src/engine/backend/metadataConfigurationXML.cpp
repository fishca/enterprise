////////////////////////////////////////////////////////////////////////////
//	Author		: Tetracode Dev
//	Description : configuration XML export/import
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

#include <wx/xml/xml.h>
#include <wx/wfstream.h>
#include <wx/base64.h>

//***********************************************************************
//*                    XML Serialization Helpers                        *
//***********************************************************************

namespace {

void AddTextNode(wxXmlNode* parent, const wxString& name, const wxString& value)
{
	wxXmlNode* node = new wxXmlNode(wxXML_ELEMENT_NODE, name);
	node->AddChild(new wxXmlNode(wxXML_TEXT_NODE, wxEmptyString, value));
	parent->AddChild(node);
}

void AddCDataNode(wxXmlNode* parent, const wxString& name, const wxString& value)
{
	wxXmlNode* node = new wxXmlNode(wxXML_ELEMENT_NODE, name);
	node->AddChild(new wxXmlNode(wxXML_CDATA_SECTION_NODE, wxEmptyString, value));
	parent->AddChild(node);
}

//***********************************************************************
//*                    Type Description Serialization                  *
//***********************************************************************

void SaveTypeDescriptionToXML(wxXmlNode* parent, const ibTypeDescription& typeDesc)
{
	const auto& clsidList = typeDesc.GetClsidList();
	if (clsidList.empty())
		return;

	wxXmlNode* xmlType = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Type"));
	for (const auto& clsid : clsidList) {
		AddTextNode(xmlType, wxT("TypeId"), wxString::Format(wxT("%llu"), (unsigned long long)clsid));
	}

	// Number qualifiers
	AddTextNode(xmlType, wxT("Precision"), wxString::Format(wxT("%d"), typeDesc.GetPrecision()));
	AddTextNode(xmlType, wxT("Scale"), wxString::Format(wxT("%d"), typeDesc.GetScale()));
	if (typeDesc.IsNonNegative())
		AddTextNode(xmlType, wxT("NonNegative"), wxT("true"));

	// String qualifiers
	AddTextNode(xmlType, wxT("Length"), wxString::Format(wxT("%d"), typeDesc.GetLength()));
	AddTextNode(xmlType, wxT("AllowedLength"), wxString::Format(wxT("%d"), (int)typeDesc.GetAllowedLength()));

	// Date qualifiers
	AddTextNode(xmlType, wxT("DateFractions"), wxString::Format(wxT("%d"), (int)typeDesc.GetDateFraction()));

	parent->AddChild(xmlType);
}

void SaveMetaDescriptionToXML(wxXmlNode* parent, const wxString& name, const ibMetaDescription& metaDesc)
{
	wxXmlNode* xmlDesc = new wxXmlNode(wxXML_ELEMENT_NODE, name);
	for (unsigned int i = 0; i < metaDesc.GetTypeCount(); i++) {
		AddTextNode(xmlDesc, wxT("MetaId"), wxString::Format(wxT("%d"), metaDesc.GetByIdx(i)));
	}
	parent->AddChild(xmlDesc);
}

//***********************************************************************
//*                    Attribute Serialization                         *
//***********************************************************************

void SaveAttributeToXML(wxXmlNode* parent, ibValueMetaObjectAttributeBase* attr, const wxString& elementName = wxT("Attribute"))
{
	wxXmlNode* xmlAttr = new wxXmlNode(wxXML_ELEMENT_NODE, elementName);
	xmlAttr->AddAttribute(wxT("guid"), attr->GetGuid().str());
	xmlAttr->AddAttribute(wxT("id"), wxString::Format(wxT("%d"), attr->GetMetaID()));

	AddTextNode(xmlAttr, wxT("Name"), attr->GetName());

	const wxString synonym = attr->GetSynonym();
	if (!synonym.IsEmpty())
		AddTextNode(xmlAttr, wxT("Synonym"), synonym);

	// Fill check
	AddTextNode(xmlAttr, wxT("FillCheck"), attr->FillCheck() ? wxT("true") : wxT("false"));

	// Item mode
	AddTextNode(xmlAttr, wxT("ItemMode"), wxString::Format(wxT("%d"), (int)attr->GetItemMode()));

	// Select mode
	AddTextNode(xmlAttr, wxT("SelectMode"), wxString::Format(wxT("%d"), (int)attr->GetSelectMode()));

	// Type information with qualifiers
	SaveTypeDescriptionToXML(xmlAttr, attr->GetTypeDesc());

	parent->AddChild(xmlAttr);
}

void SavePredefinedAttributeToXML(wxXmlNode* parent, ibValueMetaObjectAttributePredefined* attr, const wxString& elementName = wxT("PredefinedAttribute"))
{
	SaveAttributeToXML(parent, attr, elementName);
}

//***********************************************************************
//*                    Table/Dimension/Resource Serialization           *
//***********************************************************************

void SaveTableToXML(wxXmlNode* parent, ibValueMetaObjectTableData* table)
{
	wxXmlNode* xmlTable = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("TabularSection"));
	xmlTable->AddAttribute(wxT("guid"), table->GetGuid().str());
	xmlTable->AddAttribute(wxT("id"), wxString::Format(wxT("%d"), table->GetMetaID()));

	AddTextNode(xmlTable, wxT("Name"), table->GetName());

	const wxString synonym = table->GetSynonym();
	if (!synonym.IsEmpty())
		AddTextNode(xmlTable, wxT("Synonym"), synonym);

	// Table attributes (columns)
	wxXmlNode* xmlAttrs = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Attributes"));
	for (auto attr : table->GetAttributeArrayObject()) {
		if (attr->IsDeleted()) continue;
		SaveAttributeToXML(xmlAttrs, attr);
	}
	xmlTable->AddChild(xmlAttrs);

	parent->AddChild(xmlTable);
}

void SaveDimensionToXML(wxXmlNode* parent, ibValueMetaObjectDimension* dim)
{
	SaveAttributeToXML(parent, dim, wxT("Dimension"));
}

void SaveResourceToXML(wxXmlNode* parent, ibValueMetaObjectResource* res)
{
	SaveAttributeToXML(parent, res, wxT("Resource"));
}

//***********************************************************************
//*                    Form Serialization                              *
//***********************************************************************

void SaveFormToXML(wxXmlNode* parent, ibValueMetaObjectFormBase* form)
{
	wxXmlNode* xmlForm = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Form"));
	xmlForm->AddAttribute(wxT("guid"), form->GetGuid().str());
	xmlForm->AddAttribute(wxT("id"), wxString::Format(wxT("%d"), form->GetMetaID()));
	xmlForm->AddAttribute(wxT("type"), wxString::Format(wxT("%d"), form->GetTypeForm()));

	AddTextNode(xmlForm, wxT("Name"), form->GetName());

	const wxString synonym = form->GetSynonym();
	if (!synonym.IsEmpty())
		AddTextNode(xmlForm, wxT("Synonym"), synonym);

	// Form layout as base64 (binary)
	wxMemoryBuffer formData = form->GetFormData();
	if (formData.GetDataLen() > 0) {
		AddTextNode(xmlForm, wxT("FormData"), wxBase64Encode(formData));
	}

	// Form module code
	const wxString moduleText = form->GetModuleText();
	if (!moduleText.IsEmpty())
		AddCDataNode(xmlForm, wxT("Module"), moduleText);

	parent->AddChild(xmlForm);
}

//***********************************************************************
//*                    Module Serialization                            *
//***********************************************************************

void SaveModuleToXML(wxXmlNode* parent, const wxString& elementName, ibValueMetaObjectModuleBase* moduleObj)
{
	if (moduleObj == nullptr)
		return;
	const wxString text = moduleObj->GetModuleText();
	AddCDataNode(parent, elementName, text);
}

//***********************************************************************
//*                    Predefined Values Serialization                 *
//***********************************************************************

void SavePredefinedValuesToXML(wxXmlNode* parent, ibValueMetaObjectRecordDataHierarchyMutableRef* hierarchyObj)
{
	const auto& predefinedValues = hierarchyObj->GetPredefinedValueArray();

	wxXmlNode* xmlPredefined = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("PredefinedValues"));
	xmlPredefined->AddAttribute(wxT("count"), wxString::Format(wxT("%u"), (unsigned int)predefinedValues.size()));

	for (const auto& pv : predefinedValues) {
		wxXmlNode* xmlItem = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("PredefinedValue"));
		xmlItem->AddAttribute(wxT("guid"), pv->GetPredefinedGuid().str());
		xmlItem->AddAttribute(wxT("isFolder"), pv->IsPredefinedFolder() ? wxT("true") : wxT("false"));
		AddTextNode(xmlItem, wxT("Name"), pv->GetPredefinedName());
		AddTextNode(xmlItem, wxT("Code"), pv->GetPredefinedCode());
		AddTextNode(xmlItem, wxT("Description"), pv->GetPredefinedDescription());
		AddTextNode(xmlItem, wxT("Parent"), pv->GetPredefinedParentName());
		xmlPredefined->AddChild(xmlItem);
	}
	parent->AddChild(xmlPredefined);
}

//***********************************************************************
//*                    Enum Values Serialization                       *
//***********************************************************************

void SaveEnumValuesToXML(wxXmlNode* parent, ibValueMetaObjectRecordDataEnumRef* enumRef)
{
	auto enumValues = enumRef->GetEnumObjectArray();
	if (enumValues.empty())
		return;

	wxXmlNode* xmlEnums = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("EnumValues"));
	for (auto enumVal : enumValues) {
		if (enumVal->IsDeleted()) continue;
		wxXmlNode* xmlItem = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("EnumValue"));
		xmlItem->AddAttribute(wxT("guid"), enumVal->GetGuid().str());
		xmlItem->AddAttribute(wxT("id"), wxString::Format(wxT("%d"), enumVal->GetMetaID()));
		AddTextNode(xmlItem, wxT("Name"), enumVal->GetName());
		const wxString synonym = enumVal->GetSynonym();
		if (!synonym.IsEmpty())
			AddTextNode(xmlItem, wxT("Synonym"), synonym);
		xmlEnums->AddChild(xmlItem);
	}
	parent->AddChild(xmlEnums);
}

//***********************************************************************
//*                   Common elements shared by record types           *
//***********************************************************************

void SaveUserAttributesToXML(wxXmlNode* xmlObj, ibValueMetaObjectRecordData* recordData)
{
	auto attrs = recordData->GetAttributeArrayObject();
	if (!attrs.empty()) {
		wxXmlNode* xmlAttrs = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Attributes"));
		for (auto attr : attrs) {
			if (attr->IsDeleted()) continue;
			SaveAttributeToXML(xmlAttrs, attr);
		}
		xmlObj->AddChild(xmlAttrs);
	}
}

void SaveUserTablesToXML(wxXmlNode* xmlObj, ibValueMetaObjectRecordData* recordData)
{
	auto tables = recordData->GetTableArrayObject();
	if (!tables.empty()) {
		wxXmlNode* xmlTables = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("TabularSections"));
		for (auto table : tables) {
			if (table->IsDeleted()) continue;
			SaveTableToXML(xmlTables, table);
		}
		xmlObj->AddChild(xmlTables);
	}
}

void SaveFormsToXML(wxXmlNode* xmlObj, ibValueMetaObjectGenericData* genericData)
{
	auto forms = genericData->GetFormArrayObject();
	if (!forms.empty()) {
		wxXmlNode* xmlForms = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Forms"));
		for (auto form : forms) {
			if (form->IsDeleted()) continue;
			SaveFormToXML(xmlForms, form);
		}
		xmlObj->AddChild(xmlForms);
	}
}

void SaveRegisterDimensionsToXML(wxXmlNode* xmlObj, ibValueMetaObjectRegisterData* regData)
{
	auto dims = regData->GetDimentionArrayObject();
	if (!dims.empty()) {
		wxXmlNode* xmlDims = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Dimensions"));
		for (auto dim : dims) {
			if (dim->IsDeleted()) continue;
			SaveDimensionToXML(xmlDims, dim);
		}
		xmlObj->AddChild(xmlDims);
	}
}

void SaveRegisterResourcesToXML(wxXmlNode* xmlObj, ibValueMetaObjectRegisterData* regData)
{
	auto resources = regData->GetResourceArrayObject();
	if (!resources.empty()) {
		wxXmlNode* xmlRes = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Resources"));
		for (auto res : resources) {
			if (res->IsDeleted()) continue;
			SaveResourceToXML(xmlRes, res);
		}
		xmlObj->AddChild(xmlRes);
	}
}

void SaveRegisterAttributesToXML(wxXmlNode* xmlObj, ibValueMetaObjectRegisterData* regData)
{
	auto attrs = regData->GetAttributeArrayObject();
	if (!attrs.empty()) {
		wxXmlNode* xmlAttrs = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Attributes"));
		for (auto attr : attrs) {
			if (attr->IsDeleted()) continue;
			SaveAttributeToXML(xmlAttrs, attr);
		}
		xmlObj->AddChild(xmlAttrs);
	}
}

//***********************************************************************
//*                   Per-Type SaveData XML Serialization               *
//***********************************************************************

// ---- Catalog ----
void SaveCatalogToXML(wxXmlNode* xmlObj, ibValueMetaObjectCatalog* catalog)
{
	// Owner attribute (predefined)
	wxXmlNode* xmlPredAttrs = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("PredefinedAttributes"));
	SavePredefinedAttributeToXML(xmlPredAttrs, catalog->GetCatalogOwner(), wxT("Owner"));
	SavePredefinedAttributeToXML(xmlPredAttrs, catalog->GetDataPredefinedName(), wxT("PredefinedName"));
	SavePredefinedAttributeToXML(xmlPredAttrs, catalog->GetDataCode(), wxT("Code"));
	SavePredefinedAttributeToXML(xmlPredAttrs, catalog->GetDataDescription(), wxT("Description"));
	SavePredefinedAttributeToXML(xmlPredAttrs, catalog->GetDataParent(), wxT("Parent"));
	SavePredefinedAttributeToXML(xmlPredAttrs, catalog->GetDataIsFolder(), wxT("IsFolder"));
	SavePredefinedAttributeToXML(xmlPredAttrs, catalog->GetDataVersion(), wxT("DataVersion"));
	SavePredefinedAttributeToXML(xmlPredAttrs, catalog->GetDataDeletionMark(), wxT("DeletionMark"));
	SavePredefinedAttributeToXML(xmlPredAttrs, catalog->GetDataReference(), wxT("Reference"));
	xmlObj->AddChild(xmlPredAttrs);

	// QuickChoice
	AddTextNode(xmlObj, wxT("QuickChoice"), catalog->HasQuickChoice() ? wxT("true") : wxT("false"));

	// Modules
	wxXmlNode* xmlModules = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Modules"));
	SaveModuleToXML(xmlModules, wxT("ObjectModule"), catalog->GetModuleObject());
	SaveModuleToXML(xmlModules, wxT("ManagerModule"), catalog->GetModuleManager());
	xmlObj->AddChild(xmlModules);

	// Default forms (as GUIDs)
	ibValueMetaObjectFormBase* defFormObj = catalog->GetDefaultFormByID(1); // eFormObject
	ibValueMetaObjectFormBase* defFormFolder = catalog->GetDefaultFormByID(4); // eFormFolder
	ibValueMetaObjectFormBase* defFormList = catalog->GetDefaultFormByID(2); // eFormList
	ibValueMetaObjectFormBase* defFormSelect = catalog->GetDefaultFormByID(3); // eFormSelect

	wxXmlNode* xmlDefForms = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("DefaultForms"));
	AddTextNode(xmlDefForms, wxT("DefaultFormObject"), defFormObj ? defFormObj->GetGuid().str() : wxString());
	AddTextNode(xmlDefForms, wxT("DefaultFormFolder"), defFormFolder ? defFormFolder->GetGuid().str() : wxString());
	AddTextNode(xmlDefForms, wxT("DefaultFormList"), defFormList ? defFormList->GetGuid().str() : wxString());
	AddTextNode(xmlDefForms, wxT("DefaultFormSelect"), defFormSelect ? defFormSelect->GetGuid().str() : wxString());
	xmlObj->AddChild(xmlDefForms);

	// Generation property
	SaveMetaDescriptionToXML(xmlObj, wxT("Generation"), catalog->GetGenerationDescription());

	// Predefined values
	SavePredefinedValuesToXML(xmlObj, catalog);

	// User attributes, tables, forms
	SaveUserAttributesToXML(xmlObj, catalog);
	SaveUserTablesToXML(xmlObj, catalog);
	SaveFormsToXML(xmlObj, catalog);
}

// ---- Document ----
void SaveDocumentToXML(wxXmlNode* xmlObj, ibValueMetaObjectDocument* document)
{
	// Predefined attributes
	wxXmlNode* xmlPredAttrs = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("PredefinedAttributes"));
	SavePredefinedAttributeToXML(xmlPredAttrs, document->GetDocumentNumber(), wxT("Number"));
	SavePredefinedAttributeToXML(xmlPredAttrs, document->GetDocumentDate(), wxT("Date"));
	SavePredefinedAttributeToXML(xmlPredAttrs, document->GetDocumentPosted(), wxT("Posted"));
	SavePredefinedAttributeToXML(xmlPredAttrs, document->GetDataVersion(), wxT("DataVersion"));
	SavePredefinedAttributeToXML(xmlPredAttrs, document->GetDataDeletionMark(), wxT("DeletionMark"));
	SavePredefinedAttributeToXML(xmlPredAttrs, document->GetDataReference(), wxT("Reference"));
	xmlObj->AddChild(xmlPredAttrs);

	// QuickChoice
	AddTextNode(xmlObj, wxT("QuickChoice"), document->HasQuickChoice() ? wxT("true") : wxT("false"));

	// Modules
	wxXmlNode* xmlModules = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Modules"));
	SaveModuleToXML(xmlModules, wxT("ObjectModule"), document->GetModuleObject());
	SaveModuleToXML(xmlModules, wxT("ManagerModule"), document->GetModuleManager());
	xmlObj->AddChild(xmlModules);

	// Default forms
	ibValueMetaObjectFormBase* defFormObj = document->GetDefaultFormByID(1);
	ibValueMetaObjectFormBase* defFormList = document->GetDefaultFormByID(2);
	ibValueMetaObjectFormBase* defFormSelect = document->GetDefaultFormByID(3);

	wxXmlNode* xmlDefForms = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("DefaultForms"));
	AddTextNode(xmlDefForms, wxT("DefaultFormObject"), defFormObj ? defFormObj->GetGuid().str() : wxString());
	AddTextNode(xmlDefForms, wxT("DefaultFormList"), defFormList ? defFormList->GetGuid().str() : wxString());
	AddTextNode(xmlDefForms, wxT("DefaultFormSelect"), defFormSelect ? defFormSelect->GetGuid().str() : wxString());
	xmlObj->AddChild(xmlDefForms);

	// RegisterRecord property
	SaveMetaDescriptionToXML(xmlObj, wxT("RegisterRecord"), document->GetRecordDescription());

	// Generation property
	SaveMetaDescriptionToXML(xmlObj, wxT("Generation"), document->GetGenerationDescription());

	// User attributes, tables, forms
	SaveUserAttributesToXML(xmlObj, document);
	SaveUserTablesToXML(xmlObj, document);
	SaveFormsToXML(xmlObj, document);
}

// ---- Enumeration ----
void SaveEnumerationToXML(wxXmlNode* xmlObj, ibValueMetaObjectEnumeration* enumeration)
{
	// Predefined attributes
	wxXmlNode* xmlPredAttrs = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("PredefinedAttributes"));
	SavePredefinedAttributeToXML(xmlPredAttrs, enumeration->GetDataOrder(), wxT("Order"));
	SavePredefinedAttributeToXML(xmlPredAttrs, enumeration->GetDataReference(), wxT("Reference"));
	xmlObj->AddChild(xmlPredAttrs);

	// QuickChoice
	AddTextNode(xmlObj, wxT("QuickChoice"), enumeration->HasQuickChoice() ? wxT("true") : wxT("false"));

	// Module
	wxXmlNode* xmlModules = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Modules"));
	SaveModuleToXML(xmlModules, wxT("ManagerModule"), enumeration->GetModuleManager());
	xmlObj->AddChild(xmlModules);

	// Default forms
	ibValueMetaObjectFormBase* defFormList = enumeration->GetDefaultFormByID(2);
	ibValueMetaObjectFormBase* defFormSelect = enumeration->GetDefaultFormByID(3);

	wxXmlNode* xmlDefForms = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("DefaultForms"));
	AddTextNode(xmlDefForms, wxT("DefaultFormList"), defFormList ? defFormList->GetGuid().str() : wxString());
	AddTextNode(xmlDefForms, wxT("DefaultFormSelect"), defFormSelect ? defFormSelect->GetGuid().str() : wxString());
	xmlObj->AddChild(xmlDefForms);

	// Enum values
	SaveEnumValuesToXML(xmlObj, enumeration);

	// Forms
	SaveFormsToXML(xmlObj, enumeration);
}

// ---- Constant ----
void SaveConstantToXML(wxXmlNode* xmlObj, ibValueMetaObjectConstant* constant)
{
	// Module
	wxXmlNode* xmlModules = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Modules"));
	SaveModuleToXML(xmlModules, wxT("RecordModule"), constant->GetModuleObject());
	xmlObj->AddChild(xmlModules);

	// The constant itself is an attribute - save its type description
	SaveTypeDescriptionToXML(xmlObj, constant->GetTypeDesc());
}

// ---- InformationRegister ----
void SaveInformationRegisterToXML(wxXmlNode* xmlObj, ibValueMetaObjectInformationRegister* infoReg)
{
	// Predefined attributes
	wxXmlNode* xmlPredAttrs = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("PredefinedAttributes"));
	SavePredefinedAttributeToXML(xmlPredAttrs, infoReg->GetRegisterActive(), wxT("LineActive"));
	SavePredefinedAttributeToXML(xmlPredAttrs, infoReg->GetRegisterPeriod(), wxT("Period"));
	SavePredefinedAttributeToXML(xmlPredAttrs, infoReg->GetRegisterRecorder(), wxT("Recorder"));
	SavePredefinedAttributeToXML(xmlPredAttrs, infoReg->GetRegisterLineNumber(), wxT("LineNumber"));
	xmlObj->AddChild(xmlPredAttrs);

	// WriteMode and Periodicity
	AddTextNode(xmlObj, wxT("WriteMode"), wxString::Format(wxT("%d"), (int)infoReg->GetWriteRegisterMode()));
	AddTextNode(xmlObj, wxT("Periodicity"), wxString::Format(wxT("%d"), (int)infoReg->GetPeriodicity()));

	// Modules
	wxXmlNode* xmlModules = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Modules"));
	SaveModuleToXML(xmlModules, wxT("RecordSetModule"), infoReg->GetModuleObject());
	SaveModuleToXML(xmlModules, wxT("ManagerModule"), infoReg->GetModuleManager());
	xmlObj->AddChild(xmlModules);

	// Default forms
	ibValueMetaObjectFormBase* defFormRecord = infoReg->GetDefaultFormByID(1);
	ibValueMetaObjectFormBase* defFormList = infoReg->GetDefaultFormByID(2);

	wxXmlNode* xmlDefForms = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("DefaultForms"));
	AddTextNode(xmlDefForms, wxT("DefaultFormRecord"), defFormRecord ? defFormRecord->GetGuid().str() : wxString());
	AddTextNode(xmlDefForms, wxT("DefaultFormList"), defFormList ? defFormList->GetGuid().str() : wxString());
	xmlObj->AddChild(xmlDefForms);

	// Dimensions, Resources, Attributes
	SaveRegisterDimensionsToXML(xmlObj, infoReg);
	SaveRegisterResourcesToXML(xmlObj, infoReg);
	SaveRegisterAttributesToXML(xmlObj, infoReg);

	// Forms
	SaveFormsToXML(xmlObj, infoReg);
}

// ---- AccumulationRegister ----
void SaveAccumulationRegisterToXML(wxXmlNode* xmlObj, ibValueMetaObjectAccumulationRegister* accReg)
{
	// Predefined attributes
	wxXmlNode* xmlPredAttrs = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("PredefinedAttributes"));
	SavePredefinedAttributeToXML(xmlPredAttrs, accReg->GetRegisterRecordType(), wxT("RecordType"));
	SavePredefinedAttributeToXML(xmlPredAttrs, accReg->GetRegisterActive(), wxT("LineActive"));
	SavePredefinedAttributeToXML(xmlPredAttrs, accReg->GetRegisterPeriod(), wxT("Period"));
	SavePredefinedAttributeToXML(xmlPredAttrs, accReg->GetRegisterRecorder(), wxT("Recorder"));
	SavePredefinedAttributeToXML(xmlPredAttrs, accReg->GetRegisterLineNumber(), wxT("LineNumber"));
	xmlObj->AddChild(xmlPredAttrs);

	// RegisterType
	AddTextNode(xmlObj, wxT("RegisterType"), wxString::Format(wxT("%d"), (int)accReg->GetRegisterType()));

	// Modules
	wxXmlNode* xmlModules = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Modules"));
	SaveModuleToXML(xmlModules, wxT("RecordSetModule"), accReg->GetModuleObject());
	SaveModuleToXML(xmlModules, wxT("ManagerModule"), accReg->GetModuleManager());
	xmlObj->AddChild(xmlModules);

	// Default forms
	ibValueMetaObjectFormBase* defFormList = accReg->GetDefaultFormByID(2);

	wxXmlNode* xmlDefForms = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("DefaultForms"));
	AddTextNode(xmlDefForms, wxT("DefaultFormList"), defFormList ? defFormList->GetGuid().str() : wxString());
	xmlObj->AddChild(xmlDefForms);

	// Dimensions, Resources, Attributes
	SaveRegisterDimensionsToXML(xmlObj, accReg);
	SaveRegisterResourcesToXML(xmlObj, accReg);
	SaveRegisterAttributesToXML(xmlObj, accReg);

	// Forms
	SaveFormsToXML(xmlObj, accReg);
}

// ---- DataProcessor ----
void SaveDataProcessorToXML(wxXmlNode* xmlObj, ibValueMetaObjectDataProcessor* dataProcessor)
{
	// Modules
	wxXmlNode* xmlModules = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Modules"));
	SaveModuleToXML(xmlModules, wxT("ObjectModule"), dataProcessor->GetModuleObject());
	SaveModuleToXML(xmlModules, wxT("ManagerModule"), dataProcessor->GetModuleManager());
	xmlObj->AddChild(xmlModules);

	// Default form
	ibValueMetaObjectFormBase* defFormObj = dataProcessor->GetDefaultFormByID(1);

	wxXmlNode* xmlDefForms = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("DefaultForms"));
	AddTextNode(xmlDefForms, wxT("DefaultFormObject"), defFormObj ? defFormObj->GetGuid().str() : wxString());
	xmlObj->AddChild(xmlDefForms);

	// User attributes, tables, forms
	SaveUserAttributesToXML(xmlObj, dataProcessor);
	SaveUserTablesToXML(xmlObj, dataProcessor);
	SaveFormsToXML(xmlObj, dataProcessor);
}

// ---- Report ----
void SaveReportToXML(wxXmlNode* xmlObj, ibValueMetaObjectReport* report)
{
	// Modules
	wxXmlNode* xmlModules = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Modules"));
	SaveModuleToXML(xmlModules, wxT("ObjectModule"), report->GetModuleObject());
	SaveModuleToXML(xmlModules, wxT("ManagerModule"), report->GetModuleManager());
	xmlObj->AddChild(xmlModules);

	// Default form
	ibValueMetaObjectFormBase* defFormObj = report->GetDefaultFormByID(1);

	wxXmlNode* xmlDefForms = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("DefaultForms"));
	AddTextNode(xmlDefForms, wxT("DefaultFormObject"), defFormObj ? defFormObj->GetGuid().str() : wxString());
	xmlObj->AddChild(xmlDefForms);

	// User attributes, tables, forms
	SaveUserAttributesToXML(xmlObj, report);
	SaveUserTablesToXML(xmlObj, report);
	SaveFormsToXML(xmlObj, report);
}

// ---- ChartOfCharacteristicTypes ----
void SaveChartOfCharacteristicTypesToXML(wxXmlNode* xmlObj, ibValueMetaObjectChartOfCharacteristicTypes* chrt)
{
	// Predefined attributes
	wxXmlNode* xmlPredAttrs = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("PredefinedAttributes"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chrt->GetDataPredefinedName(), wxT("PredefinedName"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chrt->GetDataCode(), wxT("Code"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chrt->GetDataDescription(), wxT("Description"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chrt->GetDataParent(), wxT("Parent"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chrt->GetDataIsFolder(), wxT("IsFolder"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chrt->GetDataVersion(), wxT("DataVersion"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chrt->GetDataDeletionMark(), wxT("DeletionMark"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chrt->GetDataReference(), wxT("Reference"));
	xmlObj->AddChild(xmlPredAttrs);

	// QuickChoice
	AddTextNode(xmlObj, wxT("QuickChoice"), chrt->HasQuickChoice() ? wxT("true") : wxT("false"));

	// Modules
	wxXmlNode* xmlModules = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Modules"));
	SaveModuleToXML(xmlModules, wxT("ObjectModule"), chrt->GetModuleObject());
	SaveModuleToXML(xmlModules, wxT("ManagerModule"), chrt->GetModuleManager());
	xmlObj->AddChild(xmlModules);

	// Default forms
	ibValueMetaObjectFormBase* defFormObj = chrt->GetDefaultFormByID(1);
	ibValueMetaObjectFormBase* defFormFolder = chrt->GetDefaultFormByID(4);
	ibValueMetaObjectFormBase* defFormList = chrt->GetDefaultFormByID(2);
	ibValueMetaObjectFormBase* defFormSelect = chrt->GetDefaultFormByID(3);

	wxXmlNode* xmlDefForms = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("DefaultForms"));
	AddTextNode(xmlDefForms, wxT("DefaultFormObject"), defFormObj ? defFormObj->GetGuid().str() : wxString());
	AddTextNode(xmlDefForms, wxT("DefaultFormFolder"), defFormFolder ? defFormFolder->GetGuid().str() : wxString());
	AddTextNode(xmlDefForms, wxT("DefaultFormList"), defFormList ? defFormList->GetGuid().str() : wxString());
	AddTextNode(xmlDefForms, wxT("DefaultFormSelect"), defFormSelect ? defFormSelect->GetGuid().str() : wxString());
	xmlObj->AddChild(xmlDefForms);

	// Generation property
	SaveMetaDescriptionToXML(xmlObj, wxT("Generation"), chrt->GetGenerationDescription());

	// Predefined values
	SavePredefinedValuesToXML(xmlObj, chrt);

	// User attributes, tables, forms
	SaveUserAttributesToXML(xmlObj, chrt);
	SaveUserTablesToXML(xmlObj, chrt);
	SaveFormsToXML(xmlObj, chrt);
}

// ---- ChartOfAccounts ----
void SaveChartOfAccountsToXML(wxXmlNode* xmlObj, ibValueMetaObjectChartOfAccounts* chart)
{
	// Predefined attributes (hierarchy + accounting-specific)
	wxXmlNode* xmlPredAttrs = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("PredefinedAttributes"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chart->GetDataPredefinedName(), wxT("PredefinedName"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chart->GetDataCode(), wxT("Code"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chart->GetDataDescription(), wxT("Description"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chart->GetDataParent(), wxT("Parent"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chart->GetDataIsFolder(), wxT("IsFolder"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chart->GetAccountType(), wxT("AccountType"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chart->GetOffBalance(), wxT("OffBalance"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chart->GetQuantitative(), wxT("Quantitative"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chart->GetCurrency(), wxT("Currency"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chart->GetMaxSubcontoCount(), wxT("MaxSubcontoCount"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chart->GetDataVersion(), wxT("DataVersion"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chart->GetDataDeletionMark(), wxT("DeletionMark"));
	SavePredefinedAttributeToXML(xmlPredAttrs, chart->GetDataReference(), wxT("Reference"));
	xmlObj->AddChild(xmlPredAttrs);

	// QuickChoice
	AddTextNode(xmlObj, wxT("QuickChoice"), chart->HasQuickChoice() ? wxT("true") : wxT("false"));

	// Modules
	wxXmlNode* xmlModules = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Modules"));
	SaveModuleToXML(xmlModules, wxT("ObjectModule"), chart->GetModuleObject());
	SaveModuleToXML(xmlModules, wxT("ManagerModule"), chart->GetModuleManager());
	xmlObj->AddChild(xmlModules);

	// Default forms
	ibValueMetaObjectFormBase* defFormObj = chart->GetDefaultFormByID(1);
	ibValueMetaObjectFormBase* defFormFolder = chart->GetDefaultFormByID(4);
	ibValueMetaObjectFormBase* defFormList = chart->GetDefaultFormByID(2);
	ibValueMetaObjectFormBase* defFormSelect = chart->GetDefaultFormByID(3);

	wxXmlNode* xmlDefForms = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("DefaultForms"));
	AddTextNode(xmlDefForms, wxT("DefaultFormObject"), defFormObj ? defFormObj->GetGuid().str() : wxString());
	AddTextNode(xmlDefForms, wxT("DefaultFormFolder"), defFormFolder ? defFormFolder->GetGuid().str() : wxString());
	AddTextNode(xmlDefForms, wxT("DefaultFormList"), defFormList ? defFormList->GetGuid().str() : wxString());
	AddTextNode(xmlDefForms, wxT("DefaultFormSelect"), defFormSelect ? defFormSelect->GetGuid().str() : wxString());
	xmlObj->AddChild(xmlDefForms);

	// ChartOfCharacteristicTypes owner property
	ibPropertyChartOfCharacteristicTypes* chrtProp = chart->GetChartOfCharacteristicTypes();
	if (chrtProp != nullptr) {
		SaveMetaDescriptionToXML(xmlObj, wxT("ChartOfCharacteristicTypes"), chrtProp->GetValueAsMetaDesc());
	}

	// Generation property
	SaveMetaDescriptionToXML(xmlObj, wxT("Generation"), chart->GetGenerationDescription());

	// Predefined values
	SavePredefinedValuesToXML(xmlObj, chart);

	// User attributes, tables, forms
	SaveUserAttributesToXML(xmlObj, chart);
	SaveUserTablesToXML(xmlObj, chart);
	SaveFormsToXML(xmlObj, chart);
}

// ---- AccountingRegister ----
void SaveAccountingRegisterToXML(wxXmlNode* xmlObj, ibValueMetaObjectAccountingRegister* accReg)
{
	// Predefined attributes
	wxXmlNode* xmlPredAttrs = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("PredefinedAttributes"));
	SavePredefinedAttributeToXML(xmlPredAttrs, accReg->GetRegisterRecordType(), wxT("RecordType"));
	SavePredefinedAttributeToXML(xmlPredAttrs, accReg->GetRegisterAccount(), wxT("Account"));
	SavePredefinedAttributeToXML(xmlPredAttrs, accReg->GetRegisterSubconto1(), wxT("Subconto1"));
	SavePredefinedAttributeToXML(xmlPredAttrs, accReg->GetRegisterSubconto2(), wxT("Subconto2"));
	SavePredefinedAttributeToXML(xmlPredAttrs, accReg->GetRegisterSubconto3(), wxT("Subconto3"));
	SavePredefinedAttributeToXML(xmlPredAttrs, accReg->GetRegisterActive(), wxT("LineActive"));
	SavePredefinedAttributeToXML(xmlPredAttrs, accReg->GetRegisterPeriod(), wxT("Period"));
	SavePredefinedAttributeToXML(xmlPredAttrs, accReg->GetRegisterRecorder(), wxT("Recorder"));
	SavePredefinedAttributeToXML(xmlPredAttrs, accReg->GetRegisterLineNumber(), wxT("LineNumber"));
	xmlObj->AddChild(xmlPredAttrs);

	// Modules
	wxXmlNode* xmlModules = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Modules"));
	SaveModuleToXML(xmlModules, wxT("RecordSetModule"), accReg->GetModuleObject());
	SaveModuleToXML(xmlModules, wxT("ManagerModule"), accReg->GetModuleManager());
	xmlObj->AddChild(xmlModules);

	// Default forms
	ibValueMetaObjectFormBase* defFormList = accReg->GetDefaultFormByID(2);

	wxXmlNode* xmlDefForms = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("DefaultForms"));
	AddTextNode(xmlDefForms, wxT("DefaultFormList"), defFormList ? defFormList->GetGuid().str() : wxString());
	xmlObj->AddChild(xmlDefForms);

	// ChartOfAccounts owner property - save as meta description
	// The m_propertyChartOfAccounts is private, but we have no public accessor.
	// We save it through available meta description data.
	// Note: AccountingRegister doesn't expose GetChartOfAccounts() publicly.
	// We serialize any available dimensions/resources/attributes instead.

	// Dimensions, Resources, Attributes
	SaveRegisterDimensionsToXML(xmlObj, accReg);
	SaveRegisterResourcesToXML(xmlObj, accReg);
	SaveRegisterAttributesToXML(xmlObj, accReg);

	// Forms
	SaveFormsToXML(xmlObj, accReg);
}

//***********************************************************************
//*                    Main Per-Object Dispatch                        *
//***********************************************************************

void SaveMetaObjectToXML(wxXmlNode* parent, ibValueMetaObject* metaObject, const wxString& elementName)
{
	wxXmlNode* xmlObj = new wxXmlNode(wxXML_ELEMENT_NODE, elementName);
	xmlObj->AddAttribute(wxT("guid"), metaObject->GetGuid().str());
	xmlObj->AddAttribute(wxT("id"), wxString::Format(wxT("%d"), metaObject->GetMetaID()));
	xmlObj->AddAttribute(wxT("clsid"), wxString::Format(wxT("%llu"), (unsigned long long)metaObject->GetClassType()));

	AddTextNode(xmlObj, wxT("Name"), metaObject->GetName());

	const wxString synonym = metaObject->GetSynonym();
	if (!synonym.IsEmpty() && synonym != metaObject->GetName())
		AddTextNode(xmlObj, wxT("Synonym"), synonym);

	if (!metaObject->GetComment().IsEmpty())
		AddTextNode(xmlObj, wxT("Comment"), metaObject->GetComment());

	// Dispatch to type-specific serialization
	ibClassID clsid = metaObject->GetClassType();

	if (clsid == g_metaCatalogCLSID) {
		ibValueMetaObjectCatalog* catalog = dynamic_cast<ibValueMetaObjectCatalog*>(metaObject);
		if (catalog != nullptr)
			SaveCatalogToXML(xmlObj, catalog);
	}
	else if (clsid == g_metaDocumentCLSID) {
		ibValueMetaObjectDocument* document = dynamic_cast<ibValueMetaObjectDocument*>(metaObject);
		if (document != nullptr)
			SaveDocumentToXML(xmlObj, document);
	}
	else if (clsid == g_metaEnumerationCLSID) {
		ibValueMetaObjectEnumeration* enumeration = dynamic_cast<ibValueMetaObjectEnumeration*>(metaObject);
		if (enumeration != nullptr)
			SaveEnumerationToXML(xmlObj, enumeration);
	}
	else if (clsid == g_metaConstantCLSID) {
		ibValueMetaObjectConstant* constant = dynamic_cast<ibValueMetaObjectConstant*>(metaObject);
		if (constant != nullptr)
			SaveConstantToXML(xmlObj, constant);
	}
	else if (clsid == g_metaInformationRegisterCLSID) {
		ibValueMetaObjectInformationRegister* infoReg = dynamic_cast<ibValueMetaObjectInformationRegister*>(metaObject);
		if (infoReg != nullptr)
			SaveInformationRegisterToXML(xmlObj, infoReg);
	}
	else if (clsid == g_metaAccumulationRegisterCLSID) {
		ibValueMetaObjectAccumulationRegister* accReg = dynamic_cast<ibValueMetaObjectAccumulationRegister*>(metaObject);
		if (accReg != nullptr)
			SaveAccumulationRegisterToXML(xmlObj, accReg);
	}
	else if (clsid == g_metaDataProcessorCLSID) {
		ibValueMetaObjectDataProcessor* dataProcessor = dynamic_cast<ibValueMetaObjectDataProcessor*>(metaObject);
		if (dataProcessor != nullptr)
			SaveDataProcessorToXML(xmlObj, dataProcessor);
	}
	else if (clsid == g_metaReportCLSID) {
		ibValueMetaObjectReport* report = dynamic_cast<ibValueMetaObjectReport*>(metaObject);
		if (report != nullptr)
			SaveReportToXML(xmlObj, report);
	}
	else if (clsid == g_metaChartOfCharacteristicTypesCLSID) {
		ibValueMetaObjectChartOfCharacteristicTypes* chrt = dynamic_cast<ibValueMetaObjectChartOfCharacteristicTypes*>(metaObject);
		if (chrt != nullptr)
			SaveChartOfCharacteristicTypesToXML(xmlObj, chrt);
	}
	else if (clsid == g_metaChartOfAccountsCLSID) {
		ibValueMetaObjectChartOfAccounts* chart = dynamic_cast<ibValueMetaObjectChartOfAccounts*>(metaObject);
		if (chart != nullptr)
			SaveChartOfAccountsToXML(xmlObj, chart);
	}
	else if (clsid == g_metaAccountingRegisterCLSID) {
		ibValueMetaObjectAccountingRegister* accReg = dynamic_cast<ibValueMetaObjectAccountingRegister*>(metaObject);
		if (accReg != nullptr)
			SaveAccountingRegisterToXML(xmlObj, accReg);
	}
	else if (clsid == g_metaCommonModuleCLSID) {
		// Common module
		ibValueMetaObjectCommonModule* commonModule = dynamic_cast<ibValueMetaObjectCommonModule*>(metaObject);
		if (commonModule != nullptr) {
			const wxString text = commonModule->GetModuleText();
			AddCDataNode(xmlObj, wxT("Module"), text);
			AddTextNode(xmlObj, wxT("GlobalModule"), commonModule->IsGlobalModule() ? wxT("true") : wxT("false"));
		}
	}
	else if (clsid == g_metaCommonFormCLSID) {
		// Common form
		ibValueMetaObjectCommonForm* commonForm = dynamic_cast<ibValueMetaObjectCommonForm*>(metaObject);
		if (commonForm != nullptr) {
			wxMemoryBuffer formData = commonForm->GetFormData();
			if (formData.GetDataLen() > 0)
				AddTextNode(xmlObj, wxT("FormData"), wxBase64Encode(formData));
			const wxString moduleText = commonForm->GetModuleText();
			if (!moduleText.IsEmpty())
				AddCDataNode(xmlObj, wxT("Module"), moduleText);
		}
	}
	else {
		// Generic fallback: try to save any record-based data
		ibValueMetaObjectRecordData* recordData = dynamic_cast<ibValueMetaObjectRecordData*>(metaObject);
		if (recordData != nullptr) {
			SaveUserAttributesToXML(xmlObj, recordData);
			SaveUserTablesToXML(xmlObj, recordData);

			ibValueMetaObjectGenericData* genericData = dynamic_cast<ibValueMetaObjectGenericData*>(metaObject);
			if (genericData != nullptr)
				SaveFormsToXML(xmlObj, genericData);
		}

		// Try predefined values
		ibValueMetaObjectRecordDataHierarchyMutableRef* hierarchyObj = dynamic_cast<ibValueMetaObjectRecordDataHierarchyMutableRef*>(metaObject);
		if (hierarchyObj != nullptr)
			SavePredefinedValuesToXML(xmlObj, hierarchyObj);

		// Try module code
		ibValueMetaObjectCommonModule* commonModule = dynamic_cast<ibValueMetaObjectCommonModule*>(metaObject);
		if (commonModule != nullptr) {
			const wxString text = commonModule->GetModuleText();
			if (!text.IsEmpty())
				AddCDataNode(xmlObj, wxT("Module"), text);
		}
	}

	parent->AddChild(xmlObj);
}

} // anonymous namespace

//***********************************************************************
//*                    Save Configuration to XML                       *
//***********************************************************************

bool ibMetaDataConfigurationBase::SaveConfigToXML(const wxString& strFileName)
{
	ibValueMetaObjectConfiguration* commonMetaObject = GetCommonMetaObject();
	if (commonMetaObject == nullptr)
		return false;

	wxXmlDocument doc;
	wxXmlNode* root = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Configuration"));
	root->AddAttribute(wxT("guid"), GetConfigGuid().str());
	root->AddAttribute(wxT("version"), wxT("2"));
	root->AddAttribute(wxT("format"), wxT("OES-XML-2.0"));

	AddTextNode(root, wxT("Name"), commonMetaObject->GetName());

	const wxString synonym = commonMetaObject->GetSynonym();
	if (!synonym.IsEmpty())
		AddTextNode(root, wxT("Synonym"), synonym);

	// Configuration module
	ibValueMetaObjectModule* configModule = commonMetaObject->GetModuleObject();
	if (configModule != nullptr) {
		const wxString moduleText = configModule->GetModuleText();
		if (!moduleText.IsEmpty())
			AddCDataNode(root, wxT("ConfigurationModule"), moduleText);
	}

	// Common modules
	auto commonModules = GetAnyArrayObject(g_metaCommonModuleCLSID);
	if (!commonModules.empty()) {
		wxXmlNode* xmlGroup = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("CommonModules"));
		for (auto obj : commonModules) {
			if (obj->IsDeleted()) continue;
			SaveMetaObjectToXML(xmlGroup, obj, wxT("CommonModule"));
		}
		root->AddChild(xmlGroup);
	}

	// Common forms
	auto commonForms = GetAnyArrayObject(g_metaCommonFormCLSID);
	if (!commonForms.empty()) {
		wxXmlNode* xmlGroup = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("CommonForms"));
		for (auto obj : commonForms) {
			if (obj->IsDeleted()) continue;
			SaveMetaObjectToXML(xmlGroup, obj, wxT("CommonForm"));
		}
		root->AddChild(xmlGroup);
	}

	// Business object groups
	struct TypeGroup {
		ibClassID clsid;
		wxString groupName;
		wxString itemName;
	};

	const TypeGroup groups[] = {
		{ g_metaConstantCLSID, wxT("Constants"), wxT("Constant") },
		{ g_metaCatalogCLSID, wxT("Catalogs"), wxT("Catalog") },
		{ g_metaDocumentCLSID, wxT("Documents"), wxT("Document") },
		{ g_metaEnumerationCLSID, wxT("Enumerations"), wxT("Enumeration") },
		{ g_metaDataProcessorCLSID, wxT("DataProcessors"), wxT("DataProcessor") },
		{ g_metaReportCLSID, wxT("Reports"), wxT("Report") },
		{ g_metaInformationRegisterCLSID, wxT("InformationRegisters"), wxT("InformationRegister") },
		{ g_metaAccumulationRegisterCLSID, wxT("AccumulationRegisters"), wxT("AccumulationRegister") },
		{ g_metaChartOfCharacteristicTypesCLSID, wxT("ChartsOfCharacteristicTypes"), wxT("ChartOfCharacteristicTypes") },
		{ g_metaChartOfAccountsCLSID, wxT("ChartsOfAccounts"), wxT("ChartOfAccounts") },
		{ g_metaAccountingRegisterCLSID, wxT("AccountingRegisters"), wxT("AccountingRegister") },
	};

	for (const auto& group : groups) {
		auto objects = GetAnyArrayObject(group.clsid);
		if (objects.empty())
			continue;

		wxXmlNode* xmlGroup = new wxXmlNode(wxXML_ELEMENT_NODE, group.groupName);
		for (auto obj : objects) {
			if (obj->IsDeleted()) continue;
			SaveMetaObjectToXML(xmlGroup, obj, group.itemName);
		}
		root->AddChild(xmlGroup);
	}

	doc.SetRoot(root);

	wxFileOutputStream stream(strFileName);
	if (!stream.IsOk())
		return false;

	return doc.Save(stream);
}

//***********************************************************************
//*                    Load Configuration from XML                     *
//***********************************************************************

namespace {

wxString GetChildText(wxXmlNode* parent, const wxString& childName)
{
	for (wxXmlNode* child = parent->GetChildren(); child != nullptr; child = child->GetNext()) {
		if (child->GetName() == childName)
			return child->GetNodeContent();
	}
	return wxEmptyString;
}

wxXmlNode* FindChildNode(wxXmlNode* parent, const wxString& childName)
{
	for (wxXmlNode* child = parent->GetChildren(); child != nullptr; child = child->GetNext()) {
		if (child->GetName() == childName)
			return child;
	}
	return nullptr;
}

bool LoadTypeDescriptionFromXML(wxXmlNode* xmlType, ibTypeDescription& typeDesc)
{
	if (xmlType == nullptr)
		return false;

	typeDesc.ClearMetaType();

	for (wxXmlNode* child = xmlType->GetChildren(); child != nullptr; child = child->GetNext()) {
		if (child->GetName() == wxT("TypeId")) {
			wxString clsidStr = child->GetNodeContent();
			if (!clsidStr.IsEmpty()) {
				ibClassID clsid = string_to_clsid(clsidStr);
				typeDesc.AppendMetaType(clsid);
			}
		}
	}

	// Number qualifiers
	wxString precStr = GetChildText(xmlType, wxT("Precision"));
	wxString scaleStr = GetChildText(xmlType, wxT("Scale"));
	wxString nonNegStr = GetChildText(xmlType, wxT("NonNegative"));
	if (!precStr.IsEmpty() || !scaleStr.IsEmpty()) {
		long prec = 10, scale = 0;
		if (!precStr.IsEmpty()) precStr.ToLong(&prec);
		if (!scaleStr.IsEmpty()) scaleStr.ToLong(&scale);
		bool nonNeg = (nonNegStr == wxT("true"));
		typeDesc.m_typeData.SetNumber((unsigned char)prec, (unsigned char)scale, nonNeg);
	}

	// String qualifiers
	wxString lenStr = GetChildText(xmlType, wxT("Length"));
	wxString alStr = GetChildText(xmlType, wxT("AllowedLength"));
	if (!lenStr.IsEmpty()) {
		long length = 10;
		lenStr.ToLong(&length);
		ibAllowedLength al = ibAllowedLength::ibAllowedLength_Variable;
		if (!alStr.IsEmpty()) {
			long alVal = 0;
			alStr.ToLong(&alVal);
			al = (ibAllowedLength)alVal;
		}
		typeDesc.m_typeData.SetString((unsigned short)length, al);
	}

	// Date qualifiers
	wxString dfStr = GetChildText(xmlType, wxT("DateFractions"));
	if (!dfStr.IsEmpty()) {
		long dfVal = 1;
		dfStr.ToLong(&dfVal);
		typeDesc.m_typeData.SetDate((ibDateFractions)dfVal);
	}

	return true;
}

void LoadAttributeFromXML(wxXmlNode* xmlAttr, ibMetaData* metaData, ibValueMetaObject* parent)
{
	wxString name = GetChildText(xmlAttr, wxT("Name"));
	if (name.IsEmpty()) return;

	ibValueMetaObject* attrObj = metaData->CreateMetaObject(g_metaAttributeCLSID, parent, false);
	if (attrObj == nullptr) return;

	attrObj->SetName(name);
	wxString synonym = GetChildText(xmlAttr, wxT("Synonym"));
	if (!synonym.IsEmpty()) attrObj->SetSynonym(synonym);

	wxString guid;
	if (xmlAttr->GetAttribute(wxT("guid"), &guid) && !guid.IsEmpty())
		attrObj->SetCommonGuid(ibGuid(guid));

	// Load type description
	wxXmlNode* xmlType = FindChildNode(xmlAttr, wxT("Type"));
	if (xmlType != nullptr) {
		ibValueMetaObjectAttribute* typedAttr = dynamic_cast<ibValueMetaObjectAttribute*>(attrObj);
		if (typedAttr != nullptr) {
			ibTypeDescription typeDesc;
			if (LoadTypeDescriptionFromXML(xmlType, typeDesc))
				typedAttr->GetTypeDesc().LoadMetaType(typeDesc);
		}
	}
}

void LoadDimensionFromXML(wxXmlNode* xmlDim, ibMetaData* metaData, ibValueMetaObject* parent)
{
	wxString name = GetChildText(xmlDim, wxT("Name"));
	if (name.IsEmpty()) return;

	ibValueMetaObject* dimObj = metaData->CreateMetaObject(g_metaDimensionCLSID, parent, false);
	if (dimObj == nullptr) return;

	dimObj->SetName(name);
	wxString synonym = GetChildText(xmlDim, wxT("Synonym"));
	if (!synonym.IsEmpty()) dimObj->SetSynonym(synonym);

	wxString guid;
	if (xmlDim->GetAttribute(wxT("guid"), &guid) && !guid.IsEmpty())
		dimObj->SetCommonGuid(ibGuid(guid));

	wxXmlNode* xmlType = FindChildNode(xmlDim, wxT("Type"));
	if (xmlType != nullptr) {
		ibValueMetaObjectDimension* typedDim = dynamic_cast<ibValueMetaObjectDimension*>(dimObj);
		if (typedDim != nullptr) {
			ibTypeDescription typeDesc;
			if (LoadTypeDescriptionFromXML(xmlType, typeDesc))
				typedDim->GetTypeDesc().LoadMetaType(typeDesc);
		}
	}
}

void LoadResourceFromXML(wxXmlNode* xmlRes, ibMetaData* metaData, ibValueMetaObject* parent)
{
	wxString name = GetChildText(xmlRes, wxT("Name"));
	if (name.IsEmpty()) return;

	ibValueMetaObject* resObj = metaData->CreateMetaObject(g_metaResourceCLSID, parent, false);
	if (resObj == nullptr) return;

	resObj->SetName(name);
	wxString synonym = GetChildText(xmlRes, wxT("Synonym"));
	if (!synonym.IsEmpty()) resObj->SetSynonym(synonym);

	wxString guid;
	if (xmlRes->GetAttribute(wxT("guid"), &guid) && !guid.IsEmpty())
		resObj->SetCommonGuid(ibGuid(guid));

	wxXmlNode* xmlType = FindChildNode(xmlRes, wxT("Type"));
	if (xmlType != nullptr) {
		ibValueMetaObjectResource* typedRes = dynamic_cast<ibValueMetaObjectResource*>(resObj);
		if (typedRes != nullptr) {
			ibTypeDescription typeDesc;
			if (LoadTypeDescriptionFromXML(xmlType, typeDesc))
				typedRes->GetTypeDesc().LoadMetaType(typeDesc);
		}
	}
}

void LoadTableFromXML(wxXmlNode* xmlTable, ibMetaData* metaData, ibValueMetaObject* parent)
{
	wxString name = GetChildText(xmlTable, wxT("Name"));
	if (name.IsEmpty()) return;

	ibValueMetaObject* tableObj = metaData->CreateMetaObject(g_metaTableCLSID, parent, false);
	if (tableObj == nullptr) return;

	tableObj->SetName(name);
	wxString synonym = GetChildText(xmlTable, wxT("Synonym"));
	if (!synonym.IsEmpty()) tableObj->SetSynonym(synonym);

	wxString guid;
	if (xmlTable->GetAttribute(wxT("guid"), &guid) && !guid.IsEmpty())
		tableObj->SetCommonGuid(ibGuid(guid));

	// Load table columns
	wxXmlNode* xmlAttrs = FindChildNode(xmlTable, wxT("Attributes"));
	if (xmlAttrs != nullptr) {
		for (wxXmlNode* child = xmlAttrs->GetChildren(); child != nullptr; child = child->GetNext()) {
			if (child->GetName() == wxT("Attribute"))
				LoadAttributeFromXML(child, metaData, tableObj);
		}
	}
}

void LoadEnumValueFromXML(wxXmlNode* xmlEnum, ibMetaData* metaData, ibValueMetaObject* parent)
{
	wxString name = GetChildText(xmlEnum, wxT("Name"));
	if (name.IsEmpty()) return;

	ibValueMetaObject* enumObj = metaData->CreateMetaObject(g_metaEnumCLSID, parent, false);
	if (enumObj == nullptr) return;

	enumObj->SetName(name);
	wxString synonym = GetChildText(xmlEnum, wxT("Synonym"));
	if (!synonym.IsEmpty()) enumObj->SetSynonym(synonym);

	wxString guid;
	if (xmlEnum->GetAttribute(wxT("guid"), &guid) && !guid.IsEmpty())
		enumObj->SetCommonGuid(ibGuid(guid));
}

bool LoadMetaObjectFromXML(wxXmlNode* xmlObj, ibMetaData* metaData, const ibClassID& clsid, ibValueMetaObjectConfiguration* configObj)
{
	wxString name = GetChildText(xmlObj, wxT("Name"));
	if (name.IsEmpty()) return false;

	ibValueMetaObject* newObj = metaData->CreateMetaObject(clsid, configObj, false);
	if (newObj == nullptr) return false;

	newObj->SetName(name);

	wxString synonym = GetChildText(xmlObj, wxT("Synonym"));
	if (!synonym.IsEmpty()) newObj->SetSynonym(synonym);

	wxString comment = GetChildText(xmlObj, wxT("Comment"));
	if (!comment.IsEmpty()) newObj->SetComment(comment);

	wxString guid;
	if (xmlObj->GetAttribute(wxT("guid"), &guid) && !guid.IsEmpty())
		newObj->SetCommonGuid(ibGuid(guid));

	// Load user-defined attributes
	wxXmlNode* xmlAttrs = FindChildNode(xmlObj, wxT("Attributes"));
	if (xmlAttrs != nullptr) {
		for (wxXmlNode* child = xmlAttrs->GetChildren(); child != nullptr; child = child->GetNext()) {
			if (child->GetName() == wxT("Attribute"))
				LoadAttributeFromXML(child, metaData, newObj);
		}
	}

	// Load dimensions (for registers)
	wxXmlNode* xmlDims = FindChildNode(xmlObj, wxT("Dimensions"));
	if (xmlDims != nullptr) {
		for (wxXmlNode* child = xmlDims->GetChildren(); child != nullptr; child = child->GetNext()) {
			if (child->GetName() == wxT("Dimension"))
				LoadDimensionFromXML(child, metaData, newObj);
		}
	}

	// Load resources (for registers)
	wxXmlNode* xmlResources = FindChildNode(xmlObj, wxT("Resources"));
	if (xmlResources != nullptr) {
		for (wxXmlNode* child = xmlResources->GetChildren(); child != nullptr; child = child->GetNext()) {
			if (child->GetName() == wxT("Resource"))
				LoadResourceFromXML(child, metaData, newObj);
		}
	}

	// Load tabular sections
	wxXmlNode* xmlTables = FindChildNode(xmlObj, wxT("TabularSections"));
	if (xmlTables != nullptr) {
		for (wxXmlNode* child = xmlTables->GetChildren(); child != nullptr; child = child->GetNext()) {
			if (child->GetName() == wxT("TabularSection"))
				LoadTableFromXML(child, metaData, newObj);
		}
	}

	// Load enum values (for Enumeration)
	wxXmlNode* xmlEnumValues = FindChildNode(xmlObj, wxT("EnumValues"));
	if (xmlEnumValues != nullptr) {
		for (wxXmlNode* child = xmlEnumValues->GetChildren(); child != nullptr; child = child->GetNext()) {
			if (child->GetName() == wxT("EnumValue"))
				LoadEnumValueFromXML(child, metaData, newObj);
		}
	}

	// Load forms
	wxXmlNode* xmlForms = FindChildNode(xmlObj, wxT("Forms"));
	if (xmlForms != nullptr) {
		for (wxXmlNode* child = xmlForms->GetChildren(); child != nullptr; child = child->GetNext()) {
			if (child->GetName() == wxT("Form")) {
				wxString formName = GetChildText(child, wxT("Name"));
				if (!formName.IsEmpty()) {
					ibValueMetaObject* formObj = metaData->CreateMetaObject(g_metaFormCLSID, newObj, false);
					if (formObj != nullptr) {
						formObj->SetName(formName);

						wxString formGuid;
						if (child->GetAttribute(wxT("guid"), &formGuid) && !formGuid.IsEmpty())
							formObj->SetCommonGuid(ibGuid(formGuid));

						wxString formSynonym = GetChildText(child, wxT("Synonym"));
						if (!formSynonym.IsEmpty())
							formObj->SetSynonym(formSynonym);

						// Form data (base64 binary layout)
						wxString formDataB64 = GetChildText(child, wxT("FormData"));
						if (!formDataB64.IsEmpty()) {
							ibValueMetaObjectFormBase* formBase = dynamic_cast<ibValueMetaObjectFormBase*>(formObj);
							if (formBase != nullptr)
								formBase->SetFormData(wxBase64Decode(formDataB64));
						}

						// Form module
						wxXmlNode* formModuleNode = FindChildNode(child, wxT("Module"));
						if (formModuleNode != nullptr) {
							ibValueMetaObjectFormBase* formBase = dynamic_cast<ibValueMetaObjectFormBase*>(formObj);
							if (formBase != nullptr)
								formBase->SetModuleText(formModuleNode->GetNodeContent());
						}
					}
				}
			}
		}
	}

	// Load module code (for CommonModule)
	wxXmlNode* xmlModule = FindChildNode(xmlObj, wxT("Module"));
	if (xmlModule != nullptr) {
		ibValueMetaObjectCommonModule* commonModule = dynamic_cast<ibValueMetaObjectCommonModule*>(newObj);
		if (commonModule != nullptr) {
			commonModule->SetModuleText(xmlModule->GetNodeContent());
		}
	}

	// Load modules section (for business objects)
	wxXmlNode* xmlModules = FindChildNode(xmlObj, wxT("Modules"));
	if (xmlModules != nullptr) {
		// Object module
		wxXmlNode* objModNode = FindChildNode(xmlModules, wxT("ObjectModule"));
		if (objModNode == nullptr)
			objModNode = FindChildNode(xmlModules, wxT("RecordSetModule"));
		if (objModNode == nullptr)
			objModNode = FindChildNode(xmlModules, wxT("RecordModule"));

		if (objModNode != nullptr) {
			ibValueMetaObjectRecordData* recordData = dynamic_cast<ibValueMetaObjectRecordData*>(newObj);
			if (recordData != nullptr) {
				ibValueMetaObjectModule* moduleObj = recordData->GetModuleObject();
				if (moduleObj != nullptr)
					moduleObj->SetModuleText(objModNode->GetNodeContent());
			}
			else {
				// Constant module
				ibValueMetaObjectConstant* constant = dynamic_cast<ibValueMetaObjectConstant*>(newObj);
				if (constant != nullptr) {
					ibValueMetaObjectModule* moduleObj = constant->GetModuleObject();
					if (moduleObj != nullptr)
						moduleObj->SetModuleText(objModNode->GetNodeContent());
				}
			}
		}

		// Manager module
		wxXmlNode* mgrModNode = FindChildNode(xmlModules, wxT("ManagerModule"));
		if (mgrModNode != nullptr) {
			ibValueMetaObjectRecordData* recordData = dynamic_cast<ibValueMetaObjectRecordData*>(newObj);
			if (recordData != nullptr) {
				ibValueMetaObjectCommonModule* managerObj = recordData->GetModuleManager();
				if (managerObj != nullptr)
					managerObj->SetModuleText(mgrModNode->GetNodeContent());
			}
		}
	}

	// Load predefined values (for hierarchical objects)
	wxXmlNode* xmlPredefined = FindChildNode(xmlObj, wxT("PredefinedValues"));
	if (xmlPredefined != nullptr) {
		ibValueMetaObjectRecordDataHierarchyMutableRef* hierarchyObj =
			dynamic_cast<ibValueMetaObjectRecordDataHierarchyMutableRef*>(newObj);
		if (hierarchyObj != nullptr) {
			for (wxXmlNode* pvNode = xmlPredefined->GetChildren(); pvNode != nullptr; pvNode = pvNode->GetNext()) {
				if (pvNode->GetName() == wxT("PredefinedValue")) {
					wxString pvName = GetChildText(pvNode, wxT("Name"));
					wxString pvCode = GetChildText(pvNode, wxT("Code"));
					wxString pvDescription = GetChildText(pvNode, wxT("Description"));
					wxString pvParent = GetChildText(pvNode, wxT("Parent"));

					wxString isFolderStr;
					pvNode->GetAttribute(wxT("isFolder"), &isFolderStr);
					bool isFolder = (isFolderStr == wxT("true"));

					// Find parent by name if specified
					wxObjectDataPtr<ibValueMetaObjectRecordDataHierarchyMutableRef::ibPredefinedValueObject> parentObj;
					if (!pvParent.IsEmpty()) {
						parentObj = hierarchyObj->FindPredefinedValue(pvParent);
					}

					wxString pvGuidStr;
					if (pvNode->GetAttribute(wxT("guid"), &pvGuidStr) && !pvGuidStr.IsEmpty()) {
						hierarchyObj->SetPredefinedValue(
							ibGuid(pvGuidStr),
							pvName, pvCode, pvDescription,
							isFolder, parentObj
						);
					}
					else {
						hierarchyObj->AppendPredefinedValue(
							pvName, pvCode, pvDescription,
							isFolder, parentObj
						);
					}
				}
			}
		}
	}

	// Load constant type (for Constants)
	if (clsid == g_metaConstantCLSID) {
		wxXmlNode* xmlType = FindChildNode(xmlObj, wxT("Type"));
		if (xmlType != nullptr) {
			ibValueMetaObjectConstant* constant = dynamic_cast<ibValueMetaObjectConstant*>(newObj);
			if (constant != nullptr) {
				ibTypeDescription typeDesc;
				if (LoadTypeDescriptionFromXML(xmlType, typeDesc))
					constant->GetTypeDesc().LoadMetaType(typeDesc);
			}
		}
	}

	return true;
}

} // anonymous namespace (import helpers)

bool ibMetaDataConfigurationBase::LoadConfigFromXML(const wxString& strFileName)
{
	wxFileInputStream stream(strFileName);
	if (!stream.IsOk())
		return false;

	wxXmlDocument doc;
	if (!doc.Load(stream))
		return false;

	wxXmlNode* root = doc.GetRoot();
	if (root == nullptr || root->GetName() != wxT("Configuration"))
		return false;

	ibValueMetaObjectConfiguration* configObj = GetCommonMetaObject();
	if (configObj == nullptr)
		return false;

	// Set configuration name
	wxString configName = GetChildText(root, wxT("Name"));
	if (!configName.IsEmpty())
		configObj->SetName(configName);

	wxString configSynonym = GetChildText(root, wxT("Synonym"));
	if (!configSynonym.IsEmpty())
		configObj->SetSynonym(configSynonym);

	// Load configuration module
	wxXmlNode* configModuleNode = FindChildNode(root, wxT("ConfigurationModule"));
	if (configModuleNode != nullptr) {
		ibValueMetaObjectModule* configModule = configObj->GetModuleObject();
		if (configModule != nullptr)
			configModule->SetModuleText(configModuleNode->GetNodeContent());
	}

	// CLSID mapping for element names
	struct TypeMapping {
		wxString groupName;
		wxString itemName;
		ibClassID clsid;
	};

	const TypeMapping mappings[] = {
		{ wxT("CommonModules"), wxT("CommonModule"), g_metaCommonModuleCLSID },
		{ wxT("CommonForms"), wxT("CommonForm"), g_metaCommonFormCLSID },
		{ wxT("Constants"), wxT("Constant"), g_metaConstantCLSID },
		{ wxT("Catalogs"), wxT("Catalog"), g_metaCatalogCLSID },
		{ wxT("Documents"), wxT("Document"), g_metaDocumentCLSID },
		{ wxT("Enumerations"), wxT("Enumeration"), g_metaEnumerationCLSID },
		{ wxT("DataProcessors"), wxT("DataProcessor"), g_metaDataProcessorCLSID },
		{ wxT("Reports"), wxT("Report"), g_metaReportCLSID },
		{ wxT("InformationRegisters"), wxT("InformationRegister"), g_metaInformationRegisterCLSID },
		{ wxT("AccumulationRegisters"), wxT("AccumulationRegister"), g_metaAccumulationRegisterCLSID },
		{ wxT("ChartsOfCharacteristicTypes"), wxT("ChartOfCharacteristicTypes"), g_metaChartOfCharacteristicTypesCLSID },
		{ wxT("ChartsOfAccounts"), wxT("ChartOfAccounts"), g_metaChartOfAccountsCLSID },
		{ wxT("AccountingRegisters"), wxT("AccountingRegister"), g_metaAccountingRegisterCLSID },
	};

	// Parse each group
	for (wxXmlNode* groupNode = root->GetChildren(); groupNode != nullptr; groupNode = groupNode->GetNext()) {
		if (groupNode->GetType() != wxXML_ELEMENT_NODE)
			continue;

		for (const auto& mapping : mappings) {
			if (groupNode->GetName() == mapping.groupName) {
				for (wxXmlNode* itemNode = groupNode->GetChildren(); itemNode != nullptr; itemNode = itemNode->GetNext()) {
					if (itemNode->GetName() == mapping.itemName) {
						LoadMetaObjectFromXML(itemNode, this, mapping.clsid, configObj);
					}
				}
				break;
			}
		}
	}

	return true;
}
