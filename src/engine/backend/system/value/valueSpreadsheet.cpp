#include "valueSpreadsheet.h"
#include "backend/backend_exception.h"
#include "backend/backend_mainFrame.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueSpreadsheet, CValue);

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
	CSpreadsheetDescription& spreadsheetDesc = m_spreadsheetDoc->GetSpreadsheetDesc();

	switch (lPropNum)
	{
	case eFixedLeft:
		m_spreadsheetDoc->SetFreezeRow(varPropVal.GetInteger());
		return true;
	case eFixedTop:
		m_spreadsheetDoc->SetFreezeCol(varPropVal.GetInteger());
		return true;
	case eAreas:
		return true;
	case eReadOnly:
		return true;

	default: break;
	}

	return false;
}

bool CValueSpreadsheet::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const CSpreadsheetDescription& spreadsheetDesc = m_spreadsheetDoc->GetSpreadsheetDesc();

	switch (lPropNum)
	{
	case eFixedLeft:
		pvarPropVal = spreadsheetDesc.GetFreezeRow();
		return true;
	case eFixedTop:
		pvarPropVal = spreadsheetDesc.GetFreezeCol();
		return true;
	case eAreas:
		return true;
	case eReadOnly:
		return true;

	default: break;
	}

	return false;
}

bool CValueSpreadsheet::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	CSpreadsheetDescription& spreadsheetDesc = m_spreadsheetDoc->GetSpreadsheetDesc();

	if (lMethodNum == eGetArea)
	{
		wxStringTokenizer token(paParams[0]->GetString(), wxT('|'));
		if (token.HasMoreTokens()) {

			const wxString& strRowArea = token.GetNextToken();
			const CSpreadsheetAreaDescription* areaRow = spreadsheetDesc.GetRowAreaByName(strRowArea);

			if (token.HasMoreTokens()) {
				const wxString& strColArea = token.GetNextToken();
				const CSpreadsheetAreaDescription* areaCol = spreadsheetDesc.GetColAreaByName(strColArea);
			}

			pvarRetValue = CValue::CreateAndPrepareValueRef<CValueSpreadsheet>();
			return true;
		}
	}

	return false;
}

#include <wx/tokenzr.h>

bool CValueSpreadsheet::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
{
	CSpreadsheetDescription& spreadsheetDesc = m_spreadsheetDoc->GetSpreadsheetDesc();

	if (lMethodNum == ePutHorizontalPageBreak)
	{
		m_spreadsheetDoc->AddRowBrake(spreadsheetDesc.GetNumberRows());
		return true;
	}
	else if (lMethodNum == ePutVerticalPageBreak)
	{
		m_spreadsheetDoc->AddColBrake(spreadsheetDesc.GetNumberCols());
		return true;
	}
	else if (lMethodNum == eClear)
	{
		m_spreadsheetDoc->ResetSpreadsheet();
		return true;
	}
	else if (lMethodNum == ePrint)
	{
		if (backend_mainFrame != nullptr)
			return backend_mainFrame->PrintSpreadSheetDocument(m_spreadsheetDoc, lSizeArray > 0 ? paParams[0]->GetBoolean() : false);

		CBackendCoreException::Error(_("Context functions are not available!"));
		return false;
	}
	else if (lMethodNum == eShow)
	{
		if (backend_mainFrame != nullptr)
			return backend_mainFrame->ShowSpreadSheetDocument(paParams[0]->GetString(), m_spreadsheetDoc);

		CBackendCoreException::Error(_("Context functions are not available!"));
		return false;
	}
	else if (lMethodNum == eHide)
	{
		if (backend_mainFrame != nullptr)
			return backend_mainFrame->ShowSpreadSheetDocument(paParams[0]->GetString(), m_spreadsheetDoc);

		CBackendCoreException::Error(_("Context functions are not available!"));
		return false;
	}
	else if (lMethodNum == ePut)
	{
		CBackendCoreException::Error(_("Context functions are not available!"));
		return false;
	}
	else if (lMethodNum == eJoin)
	{
		CBackendCoreException::Error(_("Context functions are not available!"));
		return false;
	}

	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

VALUE_TYPE_REGISTER(CValueSpreadsheet, "spreadsheetDocument", string_to_clsid("VL_SPSTD"));