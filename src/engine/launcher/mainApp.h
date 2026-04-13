#ifndef _MAINAPP_H__
#define _MAINAPP_H__

#include <wx/app.h>
#include "launcher.h"

class CLauncherApp :
	public wxApp {
public:
	virtual bool OnInit();
	virtual int OnExit();

private:
	CFrameLauncher* m_launcher = nullptr;
};

wxDECLARE_APP(CLauncherApp);

#endif 