////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : dataProcessor
////////////////////////////////////////////////////////////////////////////

#include "dataProcessor.h"

CValueRecordDataObjectDataProcessor::CActionCollection CValueRecordDataObjectDataProcessor::GetActionCollection(const form_identifier_t &formType)
{
	return CActionCollection(this);
}

void CValueRecordDataObjectDataProcessor::ExecuteAction(const action_identifier_t &action, IBackendValueForm *srcForm)
{
}