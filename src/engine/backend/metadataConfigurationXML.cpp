////////////////////////////////////////////////////////////////////////////
//	Author		: Tetracode Dev
//	Description : configuration XML export/import
////////////////////////////////////////////////////////////////////////////

#include "metadataConfiguration.h"
#include "metaCollection/metaObject.h"
#include "metaCollection/metaObjectMetadata.h"
#include "metaCollection/metaModuleObject.h"
#include "metaCollection/metaFormObject.h"
#include "metaCollection/attribute/metaAttributeObject.h"
#include "metaCollection/table/metaTableObject.h"

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

void SaveAttributeToXML(wxXmlNode* parent, ibValueMetaObjectAttributeBase* attr)
{
	wxXmlNode* xmlAttr = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Attribute"));
	xmlAttr->AddAttribute(wxT("guid"), attr->GetGuid().str());
	xmlAttr->AddAttribute(wxT("id"), wxString::Format(wxT("%d"), attr->GetMetaID()));

	AddTextNode(xmlAttr, wxT("Name"), attr->GetName());

	const wxString synonym = attr->GetSynonym();
	if (!synonym.IsEmpty())
		AddTextNode(xmlAttr, wxT("Synonym"), synonym);

	// Type information
	const ibTypeDescription& typeDesc = attr->GetTypeDesc();
	const auto& clsidList = typeDesc.GetClsidList();
	if (!clsidList.empty()) {
		wxXmlNode* xmlType = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Type"));
		for (const auto& clsid : clsidList) {
			AddTextNode(xmlType, wxT("TypeId"), clsid_to_string(clsid));
		}
		xmlAttr->AddChild(xmlType);
	}

	parent->AddChild(xmlAttr);
}

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

void SaveFormToXML(wxXmlNode* parent, ibValueMetaObjectFormBase* form)
{
	wxXmlNode* xmlForm = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Form"));
	xmlForm->AddAttribute(wxT("guid"), form->GetGuid().str());
	xmlForm->AddAttribute(wxT("id"), wxString::Format(wxT("%d"), form->GetMetaID()));
	xmlForm->AddAttribute(wxT("type"), wxString::Format(wxT("%d"), form->GetTypeForm()));

	AddTextNode(xmlForm, wxT("Name"), form->GetName());

	// Form layout as base64 (binary)
	wxMemoryBuffer formData = form->GetFormData();
	if (formData.GetDataLen() > 0) {
		AddTextNode(xmlForm, wxT("FormData"), wxBase64Encode(formData));
	}

	// Form module
	ibValueMetaObjectModule* moduleObj = nullptr;
	if (form->ConvertToValue(moduleObj)) {
		const wxString moduleText = moduleObj->GetModuleText();
		if (!moduleText.IsEmpty())
			AddCDataNode(xmlForm, wxT("Module"), moduleText);
	}

	parent->AddChild(xmlForm);
}

void SaveModuleToXML(wxXmlNode* parent, const wxString& moduleName, ibValueMetaObjectModule* moduleObj)
{
	if (moduleObj == nullptr) return;
	const wxString moduleText = moduleObj->GetModuleText();
	if (!moduleText.IsEmpty())
		AddCDataNode(parent, moduleName, moduleText);
}

void SaveMetaObjectToXML(wxXmlNode* parent, ibValueMetaObject* metaObject, const wxString& elementName)
{
	wxXmlNode* xmlObj = new wxXmlNode(wxXML_ELEMENT_NODE, elementName);
	xmlObj->AddAttribute(wxT("guid"), metaObject->GetGuid().str());
	xmlObj->AddAttribute(wxT("id"), wxString::Format(wxT("%d"), metaObject->GetMetaID()));
	xmlObj->AddAttribute(wxT("clsid"), clsid_to_string(metaObject->GetClassType()));

	AddTextNode(xmlObj, wxT("Name"), metaObject->GetName());

	const wxString synonym = metaObject->GetSynonym();
	if (!synonym.IsEmpty() && synonym != metaObject->GetName())
		AddTextNode(xmlObj, wxT("Synonym"), synonym);

	if (!metaObject->GetComment().IsEmpty())
		AddTextNode(xmlObj, wxT("Comment"), metaObject->GetComment());

	// Save child attributes and tables (for record-based objects)
	ibValueMetaObjectRecordData* recordData = nullptr;
	if (metaObject->ConvertToValue(recordData)) {
		// User-defined attributes
		auto attrs = recordData->GetAttributeArrayObject();
		if (!attrs.empty()) {
			wxXmlNode* xmlAttrs = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Attributes"));
			for (auto attr : attrs) {
				if (attr->IsDeleted()) continue;
				SaveAttributeToXML(xmlAttrs, attr);
			}
			xmlObj->AddChild(xmlAttrs);
		}

		// Tabular sections
		auto tables = recordData->GetTableArrayObject();
		if (!tables.empty()) {
			wxXmlNode* xmlTables = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("TabularSections"));
			for (auto table : tables) {
				if (table->IsDeleted()) continue;
				SaveTableToXML(xmlTables, table);
			}
			xmlObj->AddChild(xmlTables);
		}

		// Forms
		auto forms = recordData->GetFormArrayObject();
		if (!forms.empty()) {
			wxXmlNode* xmlForms = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("Forms"));
			for (auto form : forms) {
				if (form->IsDeleted()) continue;
				SaveFormToXML(xmlForms, form);
			}
			xmlObj->AddChild(xmlForms);
		}
	}

	// Save common module code (for CommonModule objects)
	ibValueMetaObjectCommonModule* commonModule = nullptr;
	if (metaObject->ConvertToValue(commonModule)) {
		const wxString text = commonModule->GetModuleText();
		if (!text.IsEmpty())
			AddCDataNode(xmlObj, wxT("Module"), text);
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
	root->AddAttribute(wxT("version"), wxT("1"));
	root->AddAttribute(wxT("format"), wxT("OES-XML-1.0"));

	AddTextNode(root, wxT("Name"), commonMetaObject->GetName());

	const wxString synonym = commonMetaObject->GetSynonym();
	if (!synonym.IsEmpty())
		AddTextNode(root, wxT("Synonym"), synonym);

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

bool ibMetaDataConfigurationBase::LoadConfigFromXML(const wxString& strFileName)
{
	//TODO: Implement XML import — parse XML, create metadata objects
	return false;
}
