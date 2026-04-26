////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxwidgets community
//	Description : main frame window
////////////////////////////////////////////////////////////////////////////

#include "mainFrameEnterprise.h"
#include "docManager/docManager.h"

///////////////////////////////////////////////////////////////////

ibFrontendDocMDIFrameEnterprise* ibFrontendDocMDIFrameEnterprise::GetFrame() {
	ibFrontendDocMDIFrame* instance = ibFrontendDocMDIFrame::GetFrame();
	if (instance != nullptr) {
		ibFrontendDocMDIFrameEnterprise* enterprise_instance =
			dynamic_cast<ibFrontendDocMDIFrameEnterprise*>(instance);
		wxASSERT(enterprise_instance);
		return enterprise_instance;
	}
	return nullptr;
}

///////////////////////////////////////////////////////////////////

ibFrontendDocMDIFrameEnterprise::ibFrontendDocMDIFrameEnterprise(const wxString& title,
	const wxPoint& pos,
	const wxSize& size) :
	ibFrontendDocMDIFrame(title, pos, size),
	m_outputWindow(new ibOutputWindow(this, wxID_ANY))
{
	m_docManager = new ibMetaDocManagerEnterprise;
}

ibFrontendDocMDIFrameEnterprise::~ibFrontendDocMDIFrameEnterprise()
{
	wxDELETE(m_docManager);
}

#include "backend/debugger/debugServer.h"
#include "frontend/win/dlgs/errorDialog.h"

#include "backend/appData.h"

void ibFrontendDocMDIFrameEnterprise::BackendError(const wxString& strFileName, const wxString& strDocPath, const long currLine, const wxString& strErrorMessage) const
{
	//open error dialog
	std::shared_ptr<ibDialogError> errDlg(new ibDialogError(mainFrame, wxID_ANY));
	
	//set message 
	errDlg->SetErrorMessage(strErrorMessage);

	//get error code
	const int retCode = errDlg->ShowModal();

	//send message to enterprise
	if (retCode == 1) {
		outputWindow->OutputError(strErrorMessage);
	}

	//send error to designer
	if (retCode == 2) {
		debugServer->SendErrorToClient(
			strFileName,
			strDocPath,
			currLine,
			strErrorMessage
		);
	}

	//close window
	if (retCode == 3) {
		ibApplicationData::ForceExit();
	}
}

void ibFrontendDocMDIFrameEnterprise::CreateGUI()
{
	CreateWideGui();
}

bool ibFrontendDocMDIFrameEnterprise::Show(bool show)
{
	bool ret = ibFrontendDocMDIFrame::Show(show);
	if (ret) {
		if (!outputWindow->IsEmpty()) {
			outputWindow->SetFocus();
		}
		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////

#include "backend/metadataConfiguration.h"
#include "backend/session/session.h"
#include "backend/moduleManager/moduleManager.h"

bool ibFrontendDocMDIFrameEnterprise::AllowRun() const
{
	// StartMainModule fires BeforeStart / OnStart on the session's
	// root. BeforeStart veto returns false → frame show blocked.
	if (ibSession* s = GetSession()) {
		if (auto* root = s->GetModuleManager())
			return root->StartMainModule();
	}
	return false;
}

bool ibFrontendDocMDIFrameEnterprise::AllowClose() const
{
	// ExitMainModule fires BeforeExit / OnExit. BeforeExit veto blocks
	// close (user sees "cancelled by script" banner / modal).
	if (ibSession* s = GetSession()) {
		if (auto* root = s->GetModuleManager())
			return root->ExitMainModule();
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////