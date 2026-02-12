#ifndef __VALUE_SPREADSHEET_DOC_H__
#define __VALUE_SPREADSHEET_DOC_H__

#include "backend/compiler/value.h"
#include "backend/backend_spreadsheet.h"

class BACKEND_API CValueSpreadsheet :
	public CValue {
public:

	wxObjectDataPtr<CBackendSpreadsheetObject> GetSpreadsheetDocument() const { return m_spreadsheetDoc; }

	CSpreadsheetDescription& GetSpreadsheetDesc() { return m_spreadsheetDoc->GetSpreadsheetDesc(); }
	const CSpreadsheetDescription& GetSpreadsheetDesc() const { return m_spreadsheetDoc->GetSpreadsheetDesc(); }

	CValueSpreadsheet(const CSpreadsheetDescription& spreadsheetDesc = CSpreadsheetDescription()) :
		CValue(eValueTypes::TYPE_VALUE), m_spreadsheetDoc(new CBackendSpreadsheetObject(spreadsheetDesc)) {
	}

	virtual bool IsEmpty() const { return m_spreadsheetDoc->IsEmptyDocument(); }

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);       //function call
	virtual bool CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray);       //procudre call

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return &m_methodHelper;
	}

	virtual void PrepareNames() const; // this method is automatically called to initialize attribute and method names.

private:
	wxObjectDataPtr<CBackendSpreadsheetObject> m_spreadsheetDoc;
	static CMethodHelper m_methodHelper;
	wxDECLARE_DYNAMIC_CLASS(CValueSpreadsheet);
};

#pragma region enumeration 
#include "backend/compiler/enumUnit.h"
class BACKEND_API CValueEnumSpreadsheetOrient :
	public IEnumeration<enSpreadsheetOrientation> {
public:

	CValueEnumSpreadsheetOrient() : IEnumeration() {}

	virtual void CreateEnumeration() {
		AddEnumeration(enSpreadsheetOrientation::enOrient_Horizontal, wxT("horizontal"), _("Horizontal"));
		AddEnumeration(enSpreadsheetOrientation::enOrient_Vertical, wxT("vertical"), _("Vertical"));
	}

private:
	wxDECLARE_DYNAMIC_CLASS(CValueEnumSpreadsheetOrient);
};

class BACKEND_API CValueEnumSpreadsheetHorizontalAlignment :
	public IEnumeration<enSpreadsheetAlignmentHorz> {
public:

	CValueEnumSpreadsheetHorizontalAlignment() : IEnumeration() {}

	virtual void CreateEnumeration() {
		AddEnumeration(enSpreadsheetAlignmentHorz::enAlignmentHorz_Left, wxT("left"), _("Left"));
		AddEnumeration(enSpreadsheetAlignmentHorz::enAlignmentHorz_Center, wxT("center"), _("Center"));
		AddEnumeration(enSpreadsheetAlignmentHorz::enAlignmentHorz_Right, wxT("right"), _("Right"));
	}

private:
	wxDECLARE_DYNAMIC_CLASS(CValueEnumSpreadsheetHorizontalAlignment);
};

class BACKEND_API CValueEnumSpreadsheetVerticalAlignment :
	public IEnumeration<enSpreadsheetAlignmentVert> {
public:

	CValueEnumSpreadsheetVerticalAlignment() : IEnumeration() {}

	virtual void CreateEnumeration() {
		AddEnumeration(enSpreadsheetAlignmentVert::enAlignmentVert_Top, wxT("top"), _("Top"));
		AddEnumeration(enSpreadsheetAlignmentVert::enAlignmentVert_Center, wxT("center"), _("Center"));
		AddEnumeration(enSpreadsheetAlignmentVert::enAlignmentVert_Bottom, wxT("bottom"), _("Bottom"));
	}

private:
	wxDECLARE_DYNAMIC_CLASS(CValueEnumSpreadsheetVerticalAlignment);
};

class BACKEND_API CValueEnumSpreadsheetFitMode :
	public IEnumeration<enSpreadsheetFitMode> {
public:

	CValueEnumSpreadsheetFitMode() : IEnumeration() {}

	virtual void CreateEnumeration() {
		AddEnumeration(enSpreadsheetFitMode::enFitMode_Overflow, wxT("overflow"), _("Overflow"));
		AddEnumeration(enSpreadsheetFitMode::enFitMode_Clip, wxT("clip"), _("Clip"));
	}

private:
	wxDECLARE_DYNAMIC_CLASS(CValueEnumSpreadsheetFitMode);
};

class BACKEND_API CValueEnumSpreadsheetBorder :
	public IEnumeration<enSpreadsheetPenStyle> {
public:

	CValueEnumSpreadsheetBorder() : IEnumeration() {}

	virtual void CreateEnumeration() {
		AddEnumeration(enSpreadsheetPenStyle::enPenStyle_Transparent, wxT("none"), _("None"));
		AddEnumeration(enSpreadsheetPenStyle::enPenStyle_Solid, wxT("solid"), _("Solid"));
		AddEnumeration(enSpreadsheetPenStyle::enPenStyle_Dot, wxT("dotted"), _("Dotted"));
		AddEnumeration(enSpreadsheetPenStyle::enPenStyle_ShortDash, wxT("thin_dashed"), _("Thin dashed"));
		AddEnumeration(enSpreadsheetPenStyle::enPenStyle_DotDash, wxT("thick_dashed"), _("Thick dashed"));
		AddEnumeration(enSpreadsheetPenStyle::enPenStyle_LongDash, wxT("large_dashed"), _("Large dashed"));
	}

private:

	wxDECLARE_DYNAMIC_CLASS(CValueEnumSpreadsheetBorder);
};
#pragma endregion 

class BACKEND_API CValueSpreadsheetCell :
	public CValue {
public:

	CValueSpreadsheetCell(wxObjectDataPtr<CBackendSpreadsheetObject>& spreadsheetDoc, int row, int col) :
		m_spreadsheetDoc(spreadsheetDoc), m_row(row), m_col(col) {
	}

	virtual bool IsEmpty() const { return m_spreadsheetDoc->IsEmptyCell(m_row, m_col); }

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);       //function call
	virtual bool CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray);       //procudre call

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return &m_methodHelper;
	}

	virtual void PrepareNames() const; // this method is automatically called to initialize attribute and method names.

private:
	int m_row, m_col;
	wxObjectDataPtr<CBackendSpreadsheetObject> m_spreadsheetDoc;
	static CMethodHelper m_methodHelper;
};

class BACKEND_API CValueSpreadsheetBorder :
	public CValue {
public:

	wxPenStyle GetStyle() const { return m_style; }
	wxColour GetColour() const { return m_colour; }
	int GetWidth() const { return m_width; }

	CValueSpreadsheetBorder(wxPenStyle style = wxPENSTYLE_TRANSPARENT, const wxColour& colour = *wxBLACK, int width = 1) :
		m_style(style), m_colour(colour), m_width(width) {
	}

	virtual bool IsEmpty() const { return false; }

private:
	wxPenStyle m_style = wxPENSTYLE_TRANSPARENT;
	wxColour m_colour = *wxBLACK;
	int m_width = 1;
};

#endif 