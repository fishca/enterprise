////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : report manager 
////////////////////////////////////////////////////////////////////////////

#include "docManager.h"

wxIMPLEMENT_DYNAMIC_CLASS(CEnterpriseDocManager, CMetaDocManager);

//files
#include "templates/dataProcessorFile.h"
#include "templates/dataReportFile.h"

CEnterpriseDocManager::CEnterpriseDocManager()
	: CMetaDocManager()
{
	AddDocTemplate(wxT("External data processor"), wxT("*.edp"), "", wxT("edp"), _("Data processor Doc"), _("Data processor View"), CLASSINFO(CDataProcessorDocument), CLASSINFO(CDataProcessorView), wxTEMPLATE_ONLY_OPEN);
	AddDocTemplate(wxT("External report"), wxT("*.erp"), "", wxT("erp"), _("Report Doc"), _("Report View"), CLASSINFO(CReportDocument), CLASSINFO(CReportView), wxTEMPLATE_ONLY_OPEN);
}
