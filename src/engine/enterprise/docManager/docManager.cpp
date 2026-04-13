////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : report manager 
////////////////////////////////////////////////////////////////////////////

#include "docManager.h"

wxIMPLEMENT_DYNAMIC_CLASS(ibMetaDocManagerEnterprise, ibMetaDocManager);

//files
#include "templates/docViewDataProcessorFile.h"
#include "templates/docViewDataReportFile.h"

ibMetaDocManagerEnterprise::ibMetaDocManagerEnterprise()
	: ibMetaDocManager()
{
	AddDocTemplate(g_metaExternalDataProcessorCLSID, _("External data processor"), wxT("*.edp"), wxT("edp"), _("Data processor Doc"), _("Data processor View"), CLASSINFO(ibDataProcessorFilibDocument), CLASSINFO(ibDataProcessorEditView), wxTEMPLATE_VISIBLE | wxTEMPLATE_ONLY_OPEN);
	AddDocTemplate(g_metaExternalReportCLSID, _("External report"), wxT("*.erp"), wxT("erp"), _("Report Doc"), _("Report View"), CLASSINFO(ibReportFilibDocument), CLASSINFO(ibReportEditView), wxTEMPLATE_VISIBLE | wxTEMPLATE_ONLY_OPEN);
}
