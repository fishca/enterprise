#ifndef _MAINAPP_H__
#define _MAINAPP_H__

#include <wx/app.h>
#include "codeRunner.h"

class CCodeRunnerApp : public wxApp {
	CFrameCodeRunner* m_codeRunner = new CFrameCodeRunner(nullptr, wxID_ANY);
public:
	virtual bool OnInit();
	virtual int OnExit();

	void AppendOutput(const wxString& str);
};

wxDECLARE_APP(CCodeRunnerApp);

#endif 