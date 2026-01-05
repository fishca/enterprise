////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : informationRegister action
////////////////////////////////////////////////////////////////////////////

#include "informationRegister.h"

enum
{
	eDefActionAndClose = 1,
	eSave,
	eCopy,
};

CRecordManagerObjectInformationRegister::CActionCollection CRecordManagerObjectInformationRegister::GetActionCollection(const form_identifier_t &formType)
{
	CActionCollection registerActions(this);

	registerActions.AddAction(wxT("saveAndClose"), _("Save and close"), g_picSaveCLSID, true, eDefActionAndClose);
	registerActions.AddAction(wxT("save"), _("Save"), g_picSaveCLSID, true, eSave);
	registerActions.AddAction(wxT("clone"), _("Clone"), g_picCloneCLSID, true, eCopy);

	return registerActions;
}

void CRecordManagerObjectInformationRegister::ExecuteAction(const action_identifier_t &action, IBackendValueForm* srcForm)
{
	switch (action)
	{
	case eDefActionAndClose:
		if (WriteRegister())
			srcForm->CloseForm();
		break;
	case eSave: WriteRegister(); 
		break;
	case eCopy: CopyRegister(true); 
		break;
	}
}