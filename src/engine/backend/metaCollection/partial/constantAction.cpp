#include "constant.h"

enum
{
	eDefActionAndClose = 1,
	eSave,
};


CValueRecordDataObjectConstant::CActionCollection CValueRecordDataObjectConstant::GetActionCollection(const form_identifier_t& formType)
{
	CActionCollection catalogActions(this);
	catalogActions.AddAction(wxT("saveAndClose"), _("Save and close"), g_picSaveCLSID, true, eDefActionAndClose);
	catalogActions.AddAction(wxT("save"), _("Save"), g_picSaveCLSID, true, eSave);
	return catalogActions;
}

void CValueRecordDataObjectConstant::ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm)
{
	switch (lNumAction)
	{
	case eDefActionAndClose:
		if (SetConstValue(m_constValue))
			srcForm->CloseForm();
		break;
	case eSave: 
		SetConstValue(m_constValue); 
		break;
	}
}