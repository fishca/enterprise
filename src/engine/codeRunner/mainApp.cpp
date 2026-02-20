////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : code runner app
////////////////////////////////////////////////////////////////////////////

#include "mainApp.h"

#include "backend/appData.h"

bool CCodeRunnerApp::OnInit()
{
	if (m_codeRunner)
		return false; 

	CApplicationData::CreateAppDataEnv(eRunMode::eENTERPRISE_MODE);
	m_codeRunner = new CFrameCodeRunner(nullptr, wxID_ANY);

	return wxApp::OnInit() && m_codeRunner->Show();
}

int CCodeRunnerApp::OnExit()
{
	CApplicationData::DestroyAppDataEnv();
	return wxApp::OnExit();
}

void CCodeRunnerApp::AppendOutput(const wxString& str)
{
	if (m_codeRunner)
		m_codeRunner->AppendOutput(str);
}

wxIMPLEMENT_APP(CCodeRunnerApp);