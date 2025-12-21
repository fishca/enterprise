#ifndef __VALUE_ENUM_H__
#define __VALUE_ENUM_H__

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

#pragma region enumeration
#include "backend/compiler/enumUnit.h"
class CValueEnumOrient : public IEnumeration<wxOrientation> {
	wxDECLARE_DYNAMIC_CLASS(CValueEnumOrient);
public:
	CValueEnumOrient() : IEnumeration() {}
	//CValueEnumOrient(wxOrientation orient) : IEnumeration(orient) {}

	virtual void CreateEnumeration() {
		AddEnumeration(wxHORIZONTAL, wxT("horizontal"), _("Horizontal"));
		AddEnumeration(wxVERTICAL, wxT("vertical"), _("Vertical"));
	}
};

class CValueEnumStretch : public IEnumeration<wxStretch> {
	wxDECLARE_DYNAMIC_CLASS(CValueEnumStretch);
public:
	CValueEnumStretch() : IEnumeration() {}
	//CValueEnumStretch(const wxStretch &v) : IEnumeration(v) {}

	virtual void CreateEnumeration() {
		AddEnumeration(wxStretch::wxSHRINK, wxT("shrink"), _("Shrink"));
		AddEnumeration(wxStretch::wxEXPAND, wxT("expand"), _("Expand"));
	}
};

#include <wx/aui/auibook.h>

class CValueEnumOrientNotebookPage : public IEnumeration<wxAuiNotebookOption> {
	wxDECLARE_DYNAMIC_CLASS(CValueEnumOrientNotebookPage);
public:
	CValueEnumOrientNotebookPage() : IEnumeration() {}
	//CValueEnumOrientNotebookPage(const wxAuiNotebookOption& v) : IEnumeration(v) {}

	virtual void CreateEnumeration() {
		AddEnumeration(wxAuiNotebookOption::wxAUI_NB_TOP, wxT("top"), _("Top"));
		AddEnumeration(wxAuiNotebookOption::wxAUI_NB_BOTTOM, wxT("bottom"), _("Bottom"));
	}
};

class CValueEnumHorizontalAlignment : public IEnumeration<wxAlignment> {
	wxDECLARE_DYNAMIC_CLASS(CValueEnumHorizontalAlignment);
public:
	CValueEnumHorizontalAlignment() : IEnumeration() {}
	//CValueEnumHorizontalAlignment(const wxAlignment& v) : IEnumeration(v) {}

	virtual void CreateEnumeration() {
		AddEnumeration(wxAlignment::wxALIGN_LEFT, wxT("left"), _("Left"));
		AddEnumeration(wxAlignment::wxALIGN_CENTER, wxT("center"), _("Center"));
		AddEnumeration(wxAlignment::wxALIGN_RIGHT, wxT("right"), _("Right"));
	}
};

class CValueEnumVerticalAlignment : public IEnumeration<wxAlignment> {
	wxDECLARE_DYNAMIC_CLASS(CValueEnumVerticalAlignment);
public:
	CValueEnumVerticalAlignment() : IEnumeration() {}
	//CValueEnumVerticalAlignment(const wxAlignment& v) : IEnumeration(v) {}

	virtual void CreateEnumeration() {
		AddEnumeration(wxAlignment::wxALIGN_TOP, wxT("top"), _("Top"));
		AddEnumeration(wxAlignment::wxALIGN_CENTER, wxT("center"), _("Center"));
		AddEnumeration(wxAlignment::wxALIGN_BOTTOM, wxT("bottom"), _("Bottom"));
	}
};

class CValueEnumBorder : public IEnumeration<wxPenStyle> {
	wxDECLARE_DYNAMIC_CLASS(CValueEnumBorder);
public:
	CValueEnumBorder() : IEnumeration() {}
	//CValueEnumBorder(const wxPenStyle& v) : IEnumeration(v) {}

	virtual void CreateEnumeration() {
		AddEnumeration(wxPenStyle::wxPENSTYLE_TRANSPARENT, wxT("none"), _("None"));
		AddEnumeration(wxPenStyle::wxPENSTYLE_SOLID, wxT("solid"), _("Solid"));
		AddEnumeration(wxPenStyle::wxPENSTYLE_DOT, wxT("dotted"), _("Dotted"));
		AddEnumeration(wxPenStyle::wxPENSTYLE_SHORT_DASH, wxT("thin_dashed"), _("Thin dashed"));
		AddEnumeration(wxPenStyle::wxPENSTYLE_DOT_DASH, wxT("thick_dashed"), _("Thick dashed"));
		AddEnumeration(wxPenStyle::wxPENSTYLE_LONG_DASH, wxT("large_dashed"), _("Large dashed"));
	}
};

class CValueEnumTitleLocation : public IEnumeration<enTitleLocation> {
	wxDECLARE_DYNAMIC_CLASS(CValueEnumTitleLocation);
public:
	CValueEnumTitleLocation() : IEnumeration() {}
	//CValueEnumTitleLocation(enTitleLocation v) : IEnumeration(v) {}

	virtual void CreateEnumeration() {
		AddEnumeration(enTitleLocation::eLeft, wxT("left"), _("Left"));
		AddEnumeration(enTitleLocation::eRight, wxT("right"), _("Right"));
	}
};

class CValueEnumRepresentation : public IEnumeration<enRepresentation> {
	wxDECLARE_DYNAMIC_CLASS(CValueEnumRepresentation);
public:
	CValueEnumRepresentation() : IEnumeration() {}
	//CValueEnumTitleLocation(enTitleLocation v) : IEnumeration(v) {}

	virtual void CreateEnumeration() {
		AddEnumeration(enRepresentation::eRepresentation_Auto, wxT("auto"), _("Auto"));
		AddEnumeration(enRepresentation::eRepresentation_Text, wxT("text"), _("Text"));
		AddEnumeration(enRepresentation::eRepresentation_Picture, wxT("picture"), _("Picture"));
		AddEnumeration(enRepresentation::eRepresentation_PictureAndText, wxT("pictureText"), _("Picture and text"));
	}
};

#pragma endregion 
#endif