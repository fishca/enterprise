#ifndef _ACTIVE_USERS_WND_H__
#define _ACTIVE_USERS_WND_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/listctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/dialog.h>
#include <wx/timer.h>

#include "backend/guid.h"
#include "frontend/frontend.h"

class FRONTEND_API ibDialogActiveUser : public wxDialog {
	wxListCtrl* m_activeTable;
	std::shared_ptr<wxTimer> m_activeTableScanner;
	ibGuid m_sessionArrayHash;
public:

	void RefreshActiveUserTable();

	ibDialogActiveUser(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Active users"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(600, 250), long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
	virtual ~ibDialogActiveUser();

protected:
	void OnIdleHandler(wxTimerEvent& event) {
		RefreshActiveUserTable();
	}
	// Right-click on a row → context menu with Kick / Reload. Both
	// write to sys_session.signal via ibSessionRegistry; the owning
	// process picks the directive up on its next JobCheckSignal tick.
	void OnContextMenu(wxListEvent& event);
	void OnKickSelected(wxCommandEvent& event);
	void OnReloadSelected(wxCommandEvent& event);
};

#endif 