#ifndef __IB_USER_INFO_H__
#define __IB_USER_INFO_H__

// ibApplicationDataUserInfo — currently logged-in user's profile:
// name, GUID, password hash, role memberships, language. Extracted
// from appData.h into its own header so the struct is visible to
// both the legacy singleton (ibApplicationData) and the per-session
// ibSession without either including the other.

#include "backend/backend.h"
#include "backend/backend_core.h"

#include <vector>

#include <wx/string.h>

struct BACKEND_API ibApplicationDataUserInfo {

	struct ibApplicationDataUserRole {
		wxString m_strRoleGuid;
		wxString m_strRoleName;
		ibRoleID m_miRoleId = wxNOT_FOUND;
	};

	bool IsOk() const { return !m_strUserGuid.IsEmpty(); }

	bool IsSetPassword() const { return !m_strUserName.IsEmpty() && !m_strUserPassword.IsEmpty(); }
	bool IsSetRole()     const { return m_roleArray.size() > 0; }
	bool IsSetLanguage() const { return !m_strLanguageGuid.IsEmpty() && !m_strLanguageCode.IsEmpty(); }

	//User info
	wxString m_strUserGuid;
	wxString m_strUserName;
	wxString m_strUserFullName;
	wxString m_strUserPassword;

	//Role info
	std::vector<ibApplicationDataUserRole> m_roleArray;

	//Language info
	wxString m_strLanguageGuid;
	wxString m_strLanguageName;
	wxString m_strLanguageCode;
};

#endif
