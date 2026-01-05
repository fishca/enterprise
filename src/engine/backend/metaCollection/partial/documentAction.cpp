////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : document action
////////////////////////////////////////////////////////////////////////////

#include "document.h"

enum action {
	eDefActionAndClose = 1,
	ePost,
	eClearPosting,
	eGenerate,
	eCopy,
	eMarkAsDelete,
};

CRecordDataObjectDocument::CActionCollection CRecordDataObjectDocument::GetActionCollection(const form_identifier_t &formType)
{
	CActionCollection documentActions(this);
	documentActions.AddAction(wxT("postAndClose"), _("Post and close"), g_picPostCLSID, true, eDefActionAndClose);
	documentActions.AddAction(wxT("post"), _("Post"), g_picPostCLSID, true, ePost);
	documentActions.AddAction(wxT("clearPosting"), _("Clear posting"), g_picSaveCLSID, true, eClearPosting);
	documentActions.AddSeparator();
	documentActions.AddAction(wxT("generate"), _("Generate"), g_picGenerateCLSID, true, eGenerate);
	documentActions.AddSeparator();
	documentActions.AddAction(wxT("clone"), _("Clone"), g_picCloneCLSID, true, eCopy);

	return documentActions;
}

void CRecordDataObjectDocument::ExecuteAction(const action_identifier_t &action, IBackendValueForm* srcForm)
{
	switch (action)
	{
	case eDefActionAndClose:
		if (WriteObject(eDocumentWriteMode::eDocumentWriteMode_Posting, eDocumentPostingMode::eDocumentPostingMode_Regular)) {
			srcForm->CloseForm();
		}
		break;
	case ePost: WriteObject(eDocumentWriteMode::eDocumentWriteMode_Posting, eDocumentPostingMode::eDocumentPostingMode_Regular); break;
	case eClearPosting: WriteObject(eDocumentWriteMode::eDocumentWriteMode_UndoPosting, eDocumentPostingMode::eDocumentPostingMode_Regular); break;
	case eGenerate: Generate(); break;
	case eCopy: CopyObject(true); break;
	}
}