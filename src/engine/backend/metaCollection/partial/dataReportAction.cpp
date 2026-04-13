////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : report
////////////////////////////////////////////////////////////////////////////

#include "dataReport.h"

CValueRecordDataObjectReport::CActionCollection CValueRecordDataObjectReport::GetActionCollection(const form_identifier_t &formType)
{
	return CActionCollection(this);
}

void CValueRecordDataObjectReport::ExecuteAction(const action_identifier_t &action, IBackendValueForm *srcForm)
{
}