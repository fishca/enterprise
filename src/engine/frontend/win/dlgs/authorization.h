#ifndef _authorization_h__
#define _authorization_h__

#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/bmpcbox.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>

#include "frontend/frontend.h"

class FRONTEND_API CDialogAuthentication : public wxDialog {
public:

	CDialogAuthentication(wxWindow* parent = nullptr, wxWindowID id = wxID_ANY, const wxString& title = _("Authentication to information base"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(420, 130), long style = wxDEFAULT_DIALOG_STYLE);

	wxString GetLogin() const { return m_comboBoxLogin->GetValue(); }
	void SetLogin(const wxString& login) { m_comboBoxLogin->SetValue(login); }
	wxString GetPassword() const { return m_textCtrlPassword->GetValue(); }
	void SetPassword(const wxString& password) { m_textCtrlPassword->SetValue(password); }

private:

	wxStaticText* m_staticTextLogin;
	wxStaticText* m_staticTextPassword;
	wxBitmapComboBox* m_comboBoxLogin;
	wxTextCtrl* m_textCtrlPassword;
	wxButton* m_buttonOK;
	wxButton* m_buttonCancel;
};

#endif 