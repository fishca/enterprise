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

ibValueForm::ibActionCollection ibValueForm::GetActionCollection(const ibFormID& formType)
{
	ibActionCollection actionData(this);

	ibActionDataObject* srcAction = dynamic_cast<ibActionDataObject*>(ibValueForm::GetSourceObject());
	if (srcAction != nullptr) srcAction->AppendActionCollection(actionData, formType);

	actionData.AddAction(wxT("Close"), _("Close"), g_picCloseFormCLSID, false, enClose);
	actionData.AddAction(wxT("Update"), _("Update"), g_picUpdateFormCLSID, false, enUpdate);
	actionData.AddAction(wxT("Help"), _("Help"), g_picHelpFormCLSID, false, enHelp);

	actionData.AddSeparator();
	actionData.AddAction(wxT("Change"), _("Change form"), g_picChangeFormCLSID, false, enChange);

	return actionData;
}

void ibValueForm::ExecuteAction(const ibActionID& lNumAction, ibBackendValueForm* srcForm)
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
		ibActionDataObject* srcAction = 
			dynamic_cast<ibActionDataObject*>(ibValueForm::GetSourceObject());
		if (srcAction != nullptr)
			srcAction->ExecuteAction(lNumAction, srcForm);	
		break;
	}
	}
}
