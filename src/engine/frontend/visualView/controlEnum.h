#ifndef __CONTROL_ENUM_H__
#define __CONTROL_ENUM_H__

enum enTitleLocation {
	eLeft = 1,
	eRight
};

enum enRepresentation {
	eRepresentation_Auto,
	eRepresentation_Text,
	eRepresentation_Picture,
	eRepresentation_PictureAndText
};

#include "frontend/frontend.h"

#pragma region enumeration
#include "backend/compiler/enumUnit.h"
class FRONTEND_API CValueEnumOrient :
	public IEnumeration<wxOrientation> {
public:
	CValueEnumOrient() : IEnumeration() {}
	virtual void CreateEnumeration() {
		AddEnumeration(wxHORIZONTAL, wxT("Horizontal"), _("Horizontal"));
		AddEnumeration(wxVERTICAL, wxT("Vertical"), _("Vertical"));
	}
private:
	wxDECLARE_DYNAMIC_CLASS(CValueEnumOrient);
};

class FRONTEND_API CValueEnumStretch :
	public IEnumeration<wxStretch> {
public:
	CValueEnumStretch() : IEnumeration() {}
	virtual void CreateEnumeration() {
		AddEnumeration(wxStretch::wxSHRINK, wxT("Shrink"), _("Shrink"));
		AddEnumeration(wxStretch::wxEXPAND, wxT("Expand"), _("Expand"));
	}
private:
	wxDECLARE_DYNAMIC_CLASS(CValueEnumStretch);
};

#include <wx/aui/auibook.h>

class FRONTEND_API CValueEnumOrientNotebookPage :
	public IEnumeration<wxAuiNotebookOption> {
public:
	CValueEnumOrientNotebookPage() : IEnumeration() {}
	virtual void CreateEnumeration() {
		AddEnumeration(wxAuiNotebookOption::wxAUI_NB_TOP, wxT("Top"), _("Top"));
		AddEnumeration(wxAuiNotebookOption::wxAUI_NB_BOTTOM, wxT("Bottom"), _("Bottom"));
	}
private:
	wxDECLARE_DYNAMIC_CLASS(CValueEnumOrientNotebookPage);
};

class FRONTEND_API CValueEnumHorizontalAlignment :
	public IEnumeration<wxAlignment> {
public:
	CValueEnumHorizontalAlignment() : IEnumeration() {}
	virtual void CreateEnumeration() {
		AddEnumeration(wxAlignment::wxALIGN_LEFT, wxT("Left"), _("Left"));
		AddEnumeration(wxAlignment::wxALIGN_CENTER, wxT("Center"), _("Center"));
		AddEnumeration(wxAlignment::wxALIGN_RIGHT, wxT("Right"), _("Right"));
	}
private:
	wxDECLARE_DYNAMIC_CLASS(CValueEnumHorizontalAlignment);
};

class FRONTEND_API CValueEnumVerticalAlignment :
	public IEnumeration<wxAlignment> {
public:
	CValueEnumVerticalAlignment() : IEnumeration() {}
	virtual void CreateEnumeration() {
		AddEnumeration(wxAlignment::wxALIGN_TOP, wxT("Top"), _("Top"));
		AddEnumeration(wxAlignment::wxALIGN_CENTER, wxT("Center"), _("Center"));
		AddEnumeration(wxAlignment::wxALIGN_BOTTOM, wxT("Bottom"), _("Bottom"));
	}
private:
	wxDECLARE_DYNAMIC_CLASS(CValueEnumVerticalAlignment);
};

class FRONTEND_API CValueEnumTitleLocation :
	public IEnumeration<enTitleLocation> {
public:
	CValueEnumTitleLocation() : IEnumeration() {}
	virtual void CreateEnumeration() {
		AddEnumeration(enTitleLocation::eLeft, wxT("Left"), _("Left"));
		AddEnumeration(enTitleLocation::eRight, wxT("Right"), _("Right"));
	}
private:
	wxDECLARE_DYNAMIC_CLASS(CValueEnumTitleLocation);
};

class FRONTEND_API CValueEnumRepresentation :
	public IEnumeration<enRepresentation> {
public:
	CValueEnumRepresentation() : IEnumeration() {}
	virtual void CreateEnumeration() {
		AddEnumeration(enRepresentation::eRepresentation_Auto, wxT("Auto"), _("Auto"));
		AddEnumeration(enRepresentation::eRepresentation_Text, wxT("Text"), _("Text"));
		AddEnumeration(enRepresentation::eRepresentation_Picture, wxT("Picture"), _("Picture"));
		AddEnumeration(enRepresentation::eRepresentation_PictureAndText, wxT("PictureAndText"), _("Picture and text"));
	}
private:
	wxDECLARE_DYNAMIC_CLASS(CValueEnumRepresentation);
};

#pragma endregion 
#endif // !__CONTROL_ENUM_H__
