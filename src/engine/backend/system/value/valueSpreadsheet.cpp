#include "valueSpreadsheet.h"
#include "backend/backend_exception.h"
#include "backend/backend_mainFrame.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueSpreadsheet, CValue);

wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumSpreadsheetOrient, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumSpreadsheetHorizontalAlignment, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumSpreadsheetVerticalAlignment, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumSpreadsheetFitMode, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumSpreadsheetBorder, CValue);

CValue::CMethodHelper CValueSpreadsheet::m_methodHelper;

enum
{
	eFixedLeft,
	eFixedTop,
	eAreas,
	eParameters,
	eReadOnly,
	ePrinterName,
	eLanguageCode,
};

enum
{
	eArea,
	eRange,
	ePutVerticalPageBreak,
	ePutHorizontalPageBreak,
	eGetArea,
	eClear,
	ePrint,
	eShow,
	eHide,
	ePut,
	eJoin
};

void CValueSpreadsheet::PrepareNames() const
{
	m_methodHelper.ClearHelper();

	//freeze row/col
	m_methodHelper.AppendProp(wxT("fixedLeft"), eFixedLeft);
	m_methodHelper.AppendProp(wxT("fixedTop"), eFixedTop);
	m_methodHelper.AppendProp(wxT("areas"), eAreas);
	m_methodHelper.AppendProp(wxT("parameters"), eParameters);
	m_methodHelper.AppendProp(wxT("readOnly"), eReadOnly);
	m_methodHelper.AppendProp(wxT("printerName"), ePrinterName);
	m_methodHelper.AppendProp(wxT("languageCode"), eLanguageCode);

	m_methodHelper.AppendFunc(wxT("area"), 1, wxT("area(string: left, string: top = <empty>)"));
	m_methodHelper.AppendFunc(wxT("range"), 2, wxT("area(number: row start, number: row end, number: col start = -1, number: col end = -1)"));
	m_methodHelper.AppendProc(wxT("putVerticalPageBreak"), wxT("putVerticalPageBreak()"));
	m_methodHelper.AppendProc(wxT("putHorizontalPageBreak"), wxT("putHorizontalPageBreak()"));
	m_methodHelper.AppendFunc(wxT("getArea"), 1, wxT("getArea(string: label)"));
	m_methodHelper.AppendProc(wxT("clear"), wxT("clear()"));
	m_methodHelper.AppendProc(wxT("print"), wxT("print(bool: showPrintDlg = true)"));
	m_methodHelper.AppendProc(wxT("show"), 1, wxT("show(string: title)"));
	m_methodHelper.AppendProc(wxT("hide"), wxT("hide"));
	m_methodHelper.AppendProc(wxT("put"), 1, wxT("put(spreadsheetDocument: table)"));
	m_methodHelper.AppendProc(wxT("join"), 1, wxT("join(spreadsheetDocument: table)"));
}

bool CValueSpreadsheet::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	switch (lPropNum)
	{
	case eFixedLeft:
		m_spreadsheetDoc->SetRowFreeze(varPropVal.GetInteger());
		return true;
	case eFixedTop:
		m_spreadsheetDoc->SetColFreeze(varPropVal.GetInteger());
		return true;
	case eAreas:
		return true;
	case eReadOnly:
		m_spreadsheetDoc->SetReadOnly(varPropVal.GetBoolean());
		return true;
	case ePrinterName:
		m_spreadsheetDoc->SetPrinterName(varPropVal.GetString());
		return true;
	case eLanguageCode:
		m_spreadsheetDoc->SetLangCode(varPropVal.GetString());
		return true;
	}

	return false;
}

bool CValueSpreadsheet::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	switch (lPropNum)
	{
	case eFixedLeft:
		pvarPropVal = m_spreadsheetDoc->GetRowFreeze();
		return true;
	case eFixedTop:
		pvarPropVal = m_spreadsheetDoc->GetColFreeze();
		return true;
	case eAreas:
		return true;
	case eReadOnly:
		pvarPropVal = m_spreadsheetDoc->GetReadOnly();
		return true;
	case ePrinterName:
		pvarPropVal = m_spreadsheetDoc->GetPrinterName();
		return true;
	case eLanguageCode:
		pvarPropVal = m_spreadsheetDoc->GetLangCode();
		return true;
	}

	return false;
}

bool CValueSpreadsheet::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	if (lMethodNum == eArea) {
		pvarRetValue = CValue::CreateAndPrepareValueRef<CValueSpreadsheetCell>(
			m_spreadsheetDoc, paParams[0]->GetInteger(), lSizeArray > 1 ? paParams[1]->GetInteger() : 0);
		return true;
	}
	else if (lMethodNum == eRange) {
		pvarRetValue = CValue::CreateAndPrepareValueRef<CValueSpreadsheet>(m_spreadsheetDoc->GetArea(
			paParams[0]->GetInteger(), paParams[1]->GetInteger(), lSizeArray > 2 ? paParams[2]->GetInteger() : -1, lSizeArray > 3 ? paParams[3]->GetInteger() : -1));
		return true;
	}
	else if (lMethodNum == eGetArea) {
		pvarRetValue = CValue::CreateAndPrepareValueRef<CValueSpreadsheet>(m_spreadsheetDoc->GetAreaByName(
			paParams[0]->GetString(), lSizeArray > 1 ? paParams[1]->GetString() : wxT("")));	
		return true;
	}

	return false;
}

#include <wx/tokenzr.h>

bool CValueSpreadsheet::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
{
	if (lMethodNum == ePutHorizontalPageBreak) {
		m_spreadsheetDoc->AddRowBrake(m_spreadsheetDoc->GetNumberRows());
		return true;
	}
	else if (lMethodNum == ePutVerticalPageBreak) {
		m_spreadsheetDoc->AddColBrake(m_spreadsheetDoc->GetNumberCols());
		return true;
	}
	else if (lMethodNum == eClear) {
		m_spreadsheetDoc->ClearSpreadsheet();
		return true;
	}
	else if (lMethodNum == ePrint) {
		if (backend_mainFrame != nullptr)
			return backend_mainFrame->PrintSpreadSheetDocument(m_spreadsheetDoc, lSizeArray > 0 ? paParams[0]->GetBoolean() : false);
		CBackendCoreException::Error(_("Context functions are not available!"));
		return false;
	}
	else if (lMethodNum == eShow) {
		if (backend_mainFrame != nullptr)
			return backend_mainFrame->ShowSpreadSheetDocument(paParams[0]->GetString(), m_spreadsheetDoc);

		CBackendCoreException::Error(_("Context functions are not available!"));
		return false;
	}
	else if (lMethodNum == eHide) {
		if (backend_mainFrame != nullptr)
			return backend_mainFrame->ShowSpreadSheetDocument(paParams[0]->GetString(), m_spreadsheetDoc);
		CBackendCoreException::Error(_("Context functions are not available!"));
		return false;
	}
	else if (lMethodNum == ePut) {
		CValuePtr<CValueSpreadsheet> valueSpreadsheet =
			paParams[0]->ConvertToType<CValueSpreadsheet>();
		if (valueSpreadsheet)
			m_spreadsheetDoc->PutArea(valueSpreadsheet->GetSpreadsheetDesc());
		return true;
	}
	else if (lMethodNum == eJoin) {
		CValuePtr<CValueSpreadsheet> valueSpreadsheet =
			paParams[0]->ConvertToType<CValueSpreadsheet>();
		if (valueSpreadsheet)
			m_spreadsheetDoc->JoinArea(valueSpreadsheet->GetSpreadsheetDesc());
		return true;
	}

	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

VALUE_TYPE_REGISTER(CValueSpreadsheet, "spreadsheetDocument", string_to_clsid("VL_SPSTD"));

ENUM_TYPE_REGISTER(CValueEnumSpreadsheetOrient, "spreadsheetOrient", string_to_clsid("EN_SORNT"));
ENUM_TYPE_REGISTER(CValueEnumSpreadsheetHorizontalAlignment, "spreadsheetHorizontalAlignment", string_to_clsid("EN_SHOAL"));
ENUM_TYPE_REGISTER(CValueEnumSpreadsheetVerticalAlignment, "spreadsheetVerticalAlignment", string_to_clsid("EN_SVEAL"));
ENUM_TYPE_REGISTER(CValueEnumSpreadsheetBorder, "spreadsheetBorder", string_to_clsid("EN_SBORD"));
ENUM_TYPE_REGISTER(CValueEnumSpreadsheetFitMode, "spreadsheetFitMode", string_to_clsid("EN_SFTMD"));
