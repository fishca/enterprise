////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : catalog action
////////////////////////////////////////////////////////////////////////////

#include "catalog.h"

enum
{
	eDefActionAndClose = 1,
	eSave,
	eCopy,
	eGenerate, 
	eMarkAsDelete,
};

CRecordDataObjectCatalog::CActionCollection CRecordDataObjectCatalog::GetActionCollection(const form_identifier_t &formType)
{
	CActionCollection catalogActions(this);
	catalogActions.AddAction(wxT("saveAndClose"), _("Save and close"), g_picSaveCLSID, true, eDefActionAndClose);
	catalogActions.AddAction(wxT("save"), _("Save"), g_picSaveCLSID, true, eSave);
	catalogActions.AddSeparator();
	catalogActions.AddAction(wxT("generate"), _("Generate"), g_picGenerateCLSID, true, eGenerate);
	catalogActions.AddSeparator();
	catalogActions.AddAction(wxT("clone"), _("Clone"), g_picCloneCLSID, true, eCopy);
	return catalogActions;
}

void CRecordDataObjectCatalog::ExecuteAction(const action_identifier_t &action, IBackendValueForm* srcForm)
{
	switch (action)
	{
	case eDefActionAndClose:
		if (WriteObject())
			srcForm->CloseForm();
		break;
	case eSave: WriteObject(); break;
	case eGenerate: Generate(); break;
	case eCopy: CopyObject(true); break;
	}
}
