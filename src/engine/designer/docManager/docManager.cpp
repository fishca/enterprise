////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : report manager 
////////////////////////////////////////////////////////////////////////////

#include "docManager.h"

//common templates 
#include "frontend/docView/templates/docViewSpreadsheet.h"

#include "templates/docViewModuleEditor.h"
#include "templates/docViewFormEditor.h"
#include "templates/docViewInterface.h"
#include "templates/docViewRole.h"

//files
#include "templates/docViewdataProcessorFile.h"
#include "templates/docViewDataReportFile.h"
#include "templates/docViewMetaFile.h"

wxBEGIN_EVENT_TABLE(CDesignerDocManager, CMetaDocManager)
EVT_UPDATE_UI(wxID_DESIGNER_CONFIGURATION_ROLLBACK_DATABASE, CDesignerDocManager::OnUpdateSaveMetadata)
EVT_UPDATE_UI(wxID_DESIGNER_CONFIGURATION_UPDATE_DATABASE, CDesignerDocManager::OnUpdateSaveMetadata)
wxEND_EVENT_TABLE()

wxIMPLEMENT_DYNAMIC_CLASS(CDesignerDocManager, CMetaDocManager);

CDesignerDocManager::CDesignerDocManager()
	: CMetaDocManager()
{
	AddDocTemplate(wxT("External data processor"), wxT("*.edp"), wxEmptyString, wxT("edp"), _("Data processor Doc"), _("Data processor View"), CLASSINFO(CDataProcessorFileDocument), CLASSINFO(CDataProcessorEditView), wxTEMPLATE_VISIBLE);
	AddDocTemplate(wxT("External report"), wxT("*.erp"), wxEmptyString, wxT("erp"), _("Report Doc"), _("Report View"), CLASSINFO(CReportFileDocument), CLASSINFO(CReportEditView), wxTEMPLATE_VISIBLE);
	AddDocTemplate(wxT("Configuration"), wxT("*.mcf"), wxEmptyString, wxT("mcf"), _("Configuration Doc"), _("Configuration View"), CLASSINFO(CMetadataFileDocument), CLASSINFO(CMetadataEditView), wxTEMPLATE_ONLY_OPEN);

	//common objects 
	AddDocTemplate(g_metaCommonModuleCLSID, CLASSINFO(CModuleEditDocument), CLASSINFO(CModuleEditView));
	AddDocTemplate(g_metaCommonFormCLSID, CLASSINFO(CFormEditDocument), CLASSINFO(CFormEditView));
	AddDocTemplate(g_metaCommonTemplateCLSID, wxT("*.oxl"), wxT("oxl"), CLASSINFO(CSpreadsheetEditDocument), CLASSINFO(CSpreadsheetEditView));

	AddDocTemplate(g_metaInterfaceCLSID, CLASSINFO(CInterfaceEditDocument), CLASSINFO(CInterfaceEditView));
	AddDocTemplate(g_metaRoleCLSID, CLASSINFO(CRoleEditDocument), CLASSINFO(CRoleEditView));

	//advanced object
	AddDocTemplate(g_metaModuleCLSID, CLASSINFO(CModuleEditDocument), CLASSINFO(CModuleEditView));
	AddDocTemplate(g_metaManagerCLSID, CLASSINFO(CModuleEditDocument), CLASSINFO(CModuleEditView));
	AddDocTemplate(g_metaFormCLSID, CLASSINFO(CFormEditDocument), CLASSINFO(CFormEditView));
	AddDocTemplate(g_metaTemplateCLSID, CLASSINFO(CSpreadsheetEditDocument), CLASSINFO(CSpreadsheetEditView));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CDesignerDocManager::OnUpdateSaveMetadata(wxUpdateUIEvent& event)
{
	event.Enable(activeMetaData != nullptr && activeMetaData->IsModified());
}
