////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : frame action
////////////////////////////////////////////////////////////////////////////

#include "form.h"
#include "backend/appData.h"
#include "backend/metaCollection/partial/commonObject.h"

enum
{
	enClose = 10000,
	enUpdate,
	enHelp,

	enChange,
};

//****************************************************************************
//*                              actionData                                     *
//****************************************************************************

CValueForm::CActionCollection CValueForm::GetActionCollection(const form_identifier_t& formType)
{
	CActionCollection actionData(this);

	IActionDataObject* srcAction = dynamic_cast<IActionDataObject*>(CValueForm::GetSourceObject());
	if (srcAction != nullptr) srcAction->AppendActionCollection(actionData, formType);

	actionData.AddAction(wxT("close"), _("Close"), g_picCloseFormCLSID, true, enClose);
	actionData.AddAction(wxT("update"), _("Update"), g_picUpdateFormCLSID, true, enUpdate);
	actionData.AddAction(wxT("help"), _("Help"), g_picHelpFormCLSID, true, enHelp);

	actionData.AddSeparator();
	actionData.AddAction(wxT("change"), _("Change form"), g_picChangeFormCLSID, false, enChange);

	return actionData;
}

void CValueForm::ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm)
{
	if (appData->DesignerMode()) {
		return;
	}

	switch (lNumAction)
	{
	case enClose:
		CloseForm();
		break;
	case enUpdate:
		UpdateForm();
		break;
	case enHelp:
		HelpForm();
		break;
	case enChange:
		ChangeForm();
		break;
	default:
	{
		IActionDataObject* srcAction = 
			dynamic_cast<IActionDataObject*>(CValueForm::GetSourceObject());
		if (srcAction != nullptr)
			srcAction->ExecuteAction(lNumAction, srcForm);	
		break;
	}
	}
}
