////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxwidgets community
//	Description : main frame window
////////////////////////////////////////////////////////////////////////////

#include "mainFrameEnterprise.h"
#include "docManager/docManager.h"

///////////////////////////////////////////////////////////////////

CDocEnterpriseMDIFrame* CDocEnterpriseMDIFrame::GetFrame() {
	CDocMDIFrame* instance = CDocMDIFrame::GetFrame();
	if (instance != nullptr) {
		CDocEnterpriseMDIFrame* enterprise_instance =
			dynamic_cast<CDocEnterpriseMDIFrame*>(instance);
		wxASSERT(enterprise_instance);
		return enterprise_instance;
	}
	return nullptr;
}

///////////////////////////////////////////////////////////////////

CDocEnterpriseMDIFrame::CDocEnterpriseMDIFrame(const wxString& title,
	const wxPoint& pos,
	const wxSize& size) :
	CDocMDIFrame(title, pos, size),
	m_outputWindow(new COutputWindow(this, wxID_ANY))
{
	m_docManager = new CEnterpriseDocManager;
}

CDocEnterpriseMDIFrame::~CDocEnterpriseMDIFrame()
{
	wxDELETE(m_docManager);
}

#include "backend/debugger/debugServer.h"
#include "frontend/win/dlgs/errorDialog.h"

#include "backend/appData.h"

void CDocEnterpriseMDIFrame::BackendError(const wxString& strFileName, const wxString& strDocPath, const long currLine, const wxString& strErrorMessage) const
{
	//open error dialog
	std::shared_ptr<CDialogError> errDlg(new CDialogError(mainFrame, wxID_ANY));
	
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
		CApplicationData::ForceExit();
	}
}

void CDocEnterpriseMDIFrame::CreateGUI()
{
	CreateWideGui();
}

bool CDocEnterpriseMDIFrame::Show(bool show)
{
	bool ret = CDocMDIFrame::Show(show);
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

bool CDocEnterpriseMDIFrame::AllowRun() const
{
	if (activeMetaData != nullptr && activeMetaData->StartMainModule())
		return true;

	return false;
}

bool CDocEnterpriseMDIFrame::AllowClose() const
{
	if (activeMetaData != nullptr && activeMetaData->ExitMainModule())
		return true;

	return false;
}

///////////////////////////////////////////////////////////////////////////