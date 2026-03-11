#ifndef __about__
#define __about__

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/hyperlink.h>

#include "frontend/frontend.h"

/**
 * Class CDialogAbout
 */
class FRONTEND_API CDialogAbout :
	public wxDialog {
public:

	CDialogAbout(wxWindow* parent, int id = wxID_ANY);

private:

	wxStaticText* m_staticTextHeader;
	wxStaticText* m_staticTextFramework;
	wxStaticText* m_staticTextCommunity;
	wxStaticText* m_staticTextThanks;
	wxStaticLine* m_staticlineHeader;
	wxTextCtrl* m_textCtrlContributors;

	wxStaticText* m_staticDataBaseInfo;
	wxTextCtrl* m_textCtrl1;
	wxStaticText* m_staticAppInfo;
	wxTextCtrl* m_textCtrl2;
	wxStaticText* m_staticUserInfo;
	wxTextCtrl* m_textCtrl3;
	wxStaticText* m_staticLocaleInfo;
	wxTextCtrl* m_textCtrl4;
	wxButton* m_buttonOK;
};

#endif //__about__

