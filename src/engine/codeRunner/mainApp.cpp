////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : code runner app
////////////////////////////////////////////////////////////////////////////

#include "mainApp.h"

#include "backend/appData.h"

bool CCodeRunnerApp::OnInit()
{
	CApplicationData::CreateAppDataEnv();
	return wxApp::OnInit() && m_codeRunner->Show();
}

int CCodeRunnerApp::OnExit()
{
	CApplicationData::DestroyAppDataEnv();
	return wxApp::OnExit();
}

void CCodeRunnerApp::AppendOutput(const wxString& str)
{
	m_codeRunner->AppendOutput(str);
}

wxIMPLEMENT_APP(CCodeRunnerApp);