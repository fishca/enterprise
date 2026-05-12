#ifndef _MAINAPP_H__
#define _MAINAPP_H__

#include <wx/app.h>
#include "launcher.h"

class ibAppLauncher :
	public wxApp {
public:
	virtual bool OnInit();
	virtual int OnExit();

private:
	ibFrameLauncher* m_launcher = nullptr;
};

wxDECLARE_APP(ibAppLauncher);

#endif 