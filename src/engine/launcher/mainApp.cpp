////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : launcher app
////////////////////////////////////////////////////////////////////////////

#include "mainApp.h"
#include "backend/appData.h"

bool CLauncherApp::OnInit()
{
	if (m_launcher)
		return false;
	
	CApplicationData::CreateAppDataEnv(eRunMode::eLAUNCHER_MODE);
	m_launcher = new CFrameLauncher(nullptr, wxID_ANY);

	return wxApp::OnInit() && m_launcher->Show();
}

int CLauncherApp::OnExit()
{
	CApplicationData::DestroyAppDataEnv();
	return wxApp::OnExit();
}

wxIMPLEMENT_APP(CLauncherApp);