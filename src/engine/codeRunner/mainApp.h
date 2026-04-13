#ifndef _MAINAPP_H__
#define _MAINAPP_H__

#include <wx/app.h>
#include "codeRunner.h"

class CCodeRunnerApp :
	public wxApp {
public:

	virtual bool OnInit();
	virtual int OnExit();

	void AppendOutput(const wxString& str);

private:
	CFrameCodeRunner* m_codeRunner = nullptr;
};

wxDECLARE_APP(CCodeRunnerApp);

#endif 