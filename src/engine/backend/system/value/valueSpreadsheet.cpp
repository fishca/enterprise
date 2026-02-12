#include "valueSpreadsheet.h"
#include "backend/backend_exception.h"
#include "backend/backend_mainFrame.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueSpreadsheetDocument, CValue);

wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumSpreadsheetOrient, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumSpreadsheetHorizontalAlignment, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumSpreadsheetVerticalAlignment, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumSpreadsheetFitMode, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumSpreadsheetBorder, CValue);

CValue::CMethodHelper CValueSpreadsheetDocument::m_methodHelper;

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
	ePut,
	eJoin
};

#include "backend/system/value/valueMap.h"

class CValueSpreadsheetDocumentAreaCollection : public CValueStructure {
public:
	CValueSpreadsheetDocumentAreaCollection() : CValueStructure(true) {}

	wxDECLARE_DYNAMIC_CLASS(CValueSpreadsheetDocumentAreaCollection);
};

class CValueSpreadsheetDocumentParameterCollection : public CValueStructure {
public:
	CValueSpreadsheetDocumentParameterCollection(): CValueStructure(true) {}

	wxDECLARE_DYNAMIC_CLASS(CValueSpreadsheetDocumentParameterCollection);
};

wxIMPLEMENT_DYNAMIC_CLASS(CValueSpreadsheetDocumentAreaCollection, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueSpreadsheetDocumentParameterCollection, CValue);

void CValueSpreadsheetDocument::PrepareNames() const
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

	m_methodHelper.AppendFunc(wxT("area"), 2, wxT("area(string: left, string: top = <empty>)"));
	m_methodHelper.AppendFunc(wxT("range"), 2, wxT("area(number: row start, number: row end, number: col start = -1, number: col end = -1)"));
	m_methodHelper.AppendProc(wxT("putVerticalPageBreak"), wxT("putVerticalPageBreak()"));
	m_methodHelper.AppendProc(wxT("putHorizontalPageBreak"), wxT("putHorizontalPageBreak()"));
	m_methodHelper.AppendFunc(wxT("getArea"), 1, wxT("getArea(string: label)"));
	m_methodHelper.AppendProc(wxT("clear"), wxT("clear()"));
	m_methodHelper.AppendProc(wxT("print"), wxT("print(bool: showPrintDlg = true)"));
	m_methodHelper.AppendProc(wxT("show"), 1, wxT("show(string: title)"));
	m_methodHelper.AppendProc(wxT("put"), 1, wxT("put(spreadsheetDocument: table)"));
	m_methodHelper.AppendProc(wxT("join"), 1, wxT("join(spreadsheetDocument: table)"));
}

bool CValueSpreadsheetDocument::SetPropVal(const long lPropNum, const CValue& varPropVal)
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
		return false;
	case eParameters:
		return false;
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

bool CValueSpreadsheetDocument::GetPropVal(const long lPropNum, CValue& pvarPropVal)
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
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueSpreadsheetDocumentAreaCollection>();
		return true;
	case eParameters:
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueSpreadsheetDocumentParameterCollection>();
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

bool CValueSpreadsheetDocument::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	if (lMethodNum == eArea) {
		const CSpreadsheetCellDescription* cell = m_spreadsheetDoc->GetSpreadsheetDesc().GetCell(
			paParams[0]->GetInteger(), lSizeArray > 1 ? paParams[1]->GetInteger() : 0);	
		if (cell != nullptr) {
			pvarRetValue = CValue::CreateAndPrepareValueRef<CValueSpreadsheetDocumentArea>(
				m_spreadsheetDoc, paParams[0]->GetInteger(), lSizeArray > 1 ? paParams[1]->GetInteger() : 0);
			return true;
		}
	}
	else if (lMethodNum == eRange) {
		pvarRetValue = CValue::CreateAndPrepareValueRef<CValueSpreadsheetDocument>(m_spreadsheetDoc->GetArea(
			paParams[0]->GetInteger(), paParams[1]->GetInteger(), lSizeArray > 2 ? paParams[2]->GetInteger() : -1, lSizeArray > 3 ? paParams[3]->GetInteger() : -1));
		return true;
	}
	else if (lMethodNum == eGetArea) {
		pvarRetValue = CValue::CreateAndPrepareValueRef<CValueSpreadsheetDocument>(m_spreadsheetDoc->GetAreaByName(
			paParams[0]->GetString(), lSizeArray > 1 ? paParams[1]->GetString() : wxT("")));
		return true;
	}

	return false;
}

#include <wx/tokenzr.h>

bool CValueSpreadsheetDocument::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
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
	else if (lMethodNum == ePut) {
		CValuePtr<CValueSpreadsheetDocument> valueSpreadsheet =
			paParams[0]->ConvertToType<CValueSpreadsheetDocument>();
		if (valueSpreadsheet)
			m_spreadsheetDoc->PutArea(valueSpreadsheet->GetSpreadsheetDesc());
		return true;
	}
	else if (lMethodNum == eJoin) {
		CValuePtr<CValueSpreadsheetDocument> valueSpreadsheet =
			paParams[0]->ConvertToType<CValueSpreadsheetDocument>();
		if (valueSpreadsheet)
			m_spreadsheetDoc->JoinArea(valueSpreadsheet->GetSpreadsheetDesc());
		return true;
	}

	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

VALUE_TYPE_REGISTER(CValueSpreadsheetDocument, "spreadsheetDocument", string_to_clsid("VL_SPSTD"));
SYSTEM_TYPE_REGISTER(CValueSpreadsheetDocumentAreaCollection, "spreadsheetAreaCollection", string_to_clsid("SY_SPAEA"));
SYSTEM_TYPE_REGISTER(CValueSpreadsheetDocumentParameterCollection, "spreadsheetParameterCollection", string_to_clsid("SY_SPPRM"));
ENUM_TYPE_REGISTER(CValueEnumSpreadsheetOrient, "spreadsheetOrient", string_to_clsid("EN_SORNT"));
ENUM_TYPE_REGISTER(CValueEnumSpreadsheetHorizontalAlignment, "spreadsheetHorizontalAlignment", string_to_clsid("EN_SHOAL"));
ENUM_TYPE_REGISTER(CValueEnumSpreadsheetVerticalAlignment, "spreadsheetVerticalAlignment", string_to_clsid("EN_SVEAL"));
ENUM_TYPE_REGISTER(CValueEnumSpreadsheetBorder, "spreadsheetBorder", string_to_clsid("EN_SBORD"));
ENUM_TYPE_REGISTER(CValueEnumSpreadsheetFitMode, "spreadsheetFitMode", string_to_clsid("EN_SFTMD"));
