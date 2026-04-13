#ifndef _MAINAPP_H__
#define _MAINAPP_H__

#include <wx/app.h>

class ibAppClassChecker : public wxApp
{
	wxLocale m_locale;
public:
	virtual bool OnInit();
};

wxDECLARE_APP(ibAppClassChecker);

#endif 