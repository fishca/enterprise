////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : launcher app
////////////////////////////////////////////////////////////////////////////

#include "mainApp.h"
#include "backend/appData.h"

bool ibAppLauncher::OnInit()
{
	if (m_launcher)
		return false;
	
	ibApplicationData::CreateAppDataEnv(ibRunMode::eLAUNCHER_MODE);
	m_launcher = new ibFrameLauncher(nullptr, wxID_ANY);

	return wxApp::OnInit() && m_launcher->Show();
}

int ibAppLauncher::OnExit()
{
	ibApplicationData::DestroyAppDataEnv();
	return wxApp::OnExit();
}

wxIMPLEMENT_APP(ibAppLauncher);