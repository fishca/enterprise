#ifndef _SYSTEMOBJECTS_ENUMS_H__
#define _SYSTEMOBJECTS_ENUMS_H__

#include "systemEnum.h"
#include "backend/compiler/enumUnit.h"

class CValueStatusMessage : public IEnumeration<eStatusMessage> {
	wxDECLARE_DYNAMIC_CLASS(CValueStatusMessage);
public:
	CValueStatusMessage() : IEnumeration() {}
	//CValueStatusMessage(eStatusMessage status) : IEnumeration(status) {}

	virtual void CreateEnumeration() {
		AddEnumeration(eStatusMessage::eStatusMessage_Information, wxT("Information"), _("Information"));
		AddEnumeration(eStatusMessage::eStatusMessage_Warning, wxT("Warning"), _("Warning"));
		AddEnumeration(eStatusMessage::eStatusMessage_Error, wxT("Error"), _("Error"));
	}
};

class CValueQuestionMode : public IEnumeration<eQuestionMode> {
	wxDECLARE_DYNAMIC_CLASS(CValueQuestionMode);
public:
	CValueQuestionMode() : IEnumeration() {}
	//CValueQuestionMode(eQuestionMode mode) : IEnumeration(mode) {}

	virtual void CreateEnumeration() {
		AddEnumeration(eQuestionMode::eQuestionMode_YesNo, wxT("YesNo"), _("Yes or no"));
		AddEnumeration(eQuestionMode::eQuestionMode_YesNoCancel, wxT("YesNoCancel"), _("Yes or no or cancel"));
		AddEnumeration(eQuestionMode::eQuestionMode_OK, wxT("Ok"), _("Ok"));
		AddEnumeration(eQuestionMode::eQuestionMode_OKCancel, wxT("OkCancel"), _("Ok or cancel"));
	}
};

class CValueQuestionReturnCode : public IEnumeration<eQuestionReturnCode> {
	wxDECLARE_DYNAMIC_CLASS(CValueQuestionReturnCode);
public:
	CValueQuestionReturnCode() : IEnumeration() {}
	//CValueQuestionReturnCode(eQuestionReturnCode code) : IEnumeration(code) {}

	virtual void CreateEnumeration() {
		AddEnumeration(eQuestionReturnCode::eQuestionReturnCode_Yes, wxT("Yes"), _("Yes"));
		AddEnumeration(eQuestionReturnCode::eQuestionReturnCode_No, wxT("No"), _("Yes"));
		AddEnumeration(eQuestionReturnCode::eQuestionReturnCode_OK, wxT("Ok"), _("Ok"));
		AddEnumeration(eQuestionReturnCode::eQuestionReturnCode_Cancel, wxT("Cancel"), _("Cancel"));
	}
};

class CValueRoundMode : public IEnumeration<eRoundMode> {
	wxDECLARE_DYNAMIC_CLASS(CValueQuestionReturnCode);
public:
	CValueRoundMode() : IEnumeration() {}
	//CValueRoundMode(eRoundMode mode) : IEnumeration(mode) {}

	virtual void CreateEnumeration() {
		AddEnumeration(eRoundMode::eRoundMode_Round15as10, wxT("Round15as10"), _("Round 15 as 10"));
		AddEnumeration(eRoundMode::eRoundMode_Round15as20, wxT("Round15as20"), _("Round 15 as 20"));
	}
};

class CValueChars : public IEnumeration<enChars> {
	wxDECLARE_DYNAMIC_CLASS(CValueChars);
public:
	CValueChars() : IEnumeration() {}
	//CValueChars(enChars c) : IEnumeration(c) {}

	virtual void CreateEnumeration() {
		AddEnumeration(enChars::eCR, wxT("CR"));
		AddEnumeration(enChars::eFF, wxT("FF"));
		AddEnumeration(enChars::eLF, wxT("LF"));
		AddEnumeration(enChars::eNBSp, wxT("NBSp"));
		AddEnumeration(enChars::eTab, wxT("Tab"));
		AddEnumeration(enChars::eVTab, wxT("VTab"));
	}

	virtual wxString GetDescription(enChars val) const {
		return (char)val;
	}
};

#endif