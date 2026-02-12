////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : report manager 
////////////////////////////////////////////////////////////////////////////

#include "docManager.h"

wxIMPLEMENT_DYNAMIC_CLASS(CEnterpriseDocManager, CMetaDocManager);

//files
#include "templates/docViewDataProcessorFile.h"
#include "templates/docViewDataReportFile.h"

CEnterpriseDocManager::CEnterpriseDocManager()
	: CMetaDocManager()
{
	AddDocTemplate(g_metaExternalDataProcessorCLSID, wxT("External data processor"), wxT("*.edp"), wxT("edp"), _("Data processor Doc"), _("Data processor View"), CLASSINFO(CDataProcessorFileDocument), CLASSINFO(CDataProcessorEditView), wxTEMPLATE_ONLY_OPEN);
	AddDocTemplate(g_metaExternalReportCLSID, wxT("External report"), wxT("*.erp"), wxT("erp"), _("Report Doc"), _("Report View"), CLASSINFO(CReportFileDocument), CLASSINFO(CReportEditView), wxTEMPLATE_ONLY_OPEN);
}
