////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : launcher app
////////////////////////////////////////////////////////////////////////////

#include "mainApp.h"
#include "classCheckerWnd.h"

bool ibAppClassChecker::OnInit()
{
	wxDateTime::SetCountry(wxDateTime::Country::USA);

	m_locale.AddCatalogLookupPathPrefix(_T("lang"));
	m_locale.AddCatalog(m_locale.GetCanonicalName());

	ibFrameClassChecker *launcherWnd = 
		new ibFrameClassChecker(nullptr, wxID_ANY);
	launcherWnd->Show();

	return m_locale.Init(wxLANGUAGE_ENGLISH);
}


wxIMPLEMENT_APP(ibAppClassChecker);   