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
	m_methodHelper.AppendProc(wxT("print"), wxT("print()"));
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
		m_spreadsheetDoc.SetFreezeRow(varPropVal.GetInteger());
		return true;
	case eFixedTop:
		m_spreadsheetDoc.SetFreezeCol(varPropVal.GetInteger());
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
	switch (lPropNum)
	{
	case eFixedLeft:
		pvarPropVal = m_spreadsheetDoc.GetFreezeRow();
		return true;
	case eFixedTop:
		pvarPropVal = m_spreadsheetDoc.GetFreezeCol();
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
	if (lMethodNum == eGetArea)
	{
		wxStringTokenizer token(paParams[0]->GetString(), wxT('|'));
		if (token.HasMoreTokens()) {

			const wxString& strRowArea = token.GetNextToken();
			const CSpreadsheetAreaChunk* areaRow = m_spreadsheetDoc.GetAreaRow(strRowArea);

			if (token.HasMoreTokens()) {
				const wxString& strColArea = token.GetNextToken();
				const CSpreadsheetAreaChunk* areaCol = m_spreadsheetDoc.GetAreaCol(strColArea);
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
	if (lMethodNum == ePutHorizontalPageBreak)
	{
		m_spreadsheetDoc.AppendBrakeRow(m_spreadsheetDoc.GetNumberRows());
		return true;
	}
	else if (lMethodNum == ePutVerticalPageBreak)
	{
		m_spreadsheetDoc.AppendBrakeCol(m_spreadsheetDoc.GetNumberCols());
		return true;
	}
	else if (lMethodNum == eClear)
	{
		m_spreadsheetDoc.ResetSpreadsheet();
		return true;
	}
	else if (lMethodNum == ePrint)
	{
		if (backend_mainFrame != nullptr)
			return backend_mainFrame->PrintSpreadSheetDocument(m_spreadsheetDoc);

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