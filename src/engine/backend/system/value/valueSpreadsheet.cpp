#include "valueSpreadsheet.h"
#include "backend/backend_mainFrame.h"

#pragma region __collection__h__

#include "backend/system/value/valueMap.h"

class CValueSpreadsheetDocumentRange :
	public CValue {

	enum
	{
		enPropLabel,
		enPropStart,
		enPropEnd
	};

public:

	CValueSpreadsheetDocumentRange() : CValue(eValueTypes::TYPE_VALUE), m_label(), m_start(-1), m_end(-1) {}
	CValueSpreadsheetDocumentRange(const wxString& label, int start, int end) : CValue(eValueTypes::TYPE_VALUE), m_label(label), m_start(start), m_end(end) {}

	virtual bool IsEmpty() const { return false; }

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal) { return false; }
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal) {

		switch (lPropNum)
		{
		case enPropLabel:
			pvarPropVal = m_label;
			return true;
		case enPropStart:
			pvarPropVal = m_start;
			return true;
		case enPropEnd:
			pvarPropVal = m_end;
			return true;
		}

		return false;
	}

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return &m_methodHelper;
	}

	virtual void PrepareNames() const // this method is automatically called to initialize attribute and method names.
	{
		m_methodHelper.ClearHelper();

		m_methodHelper.AppendProp(wxT("label"));
		m_methodHelper.AppendProp(wxT("start"));
		m_methodHelper.AppendProp(wxT("end"));
	}

private:

	const wxString m_label;
	const int m_start, m_end;

	static CMethodHelper m_methodHelper;

	wxDECLARE_DYNAMIC_CLASS(CValueSpreadsheetDocumentRange);
};

class CValueSpreadsheetDocumentAreaCollection :
	public CValueStructure {

public:

	CValueSpreadsheetDocumentAreaCollection() :
		CValueStructure(true)
	{
	}

	CValueSpreadsheetDocumentAreaCollection(const wxObjectDataPtr<CBackendSpreadsheetObject>& spreadsheetDoc) :
		CValueStructure(true), m_spreadsheetDoc(spreadsheetDoc)
	{
		for (int idx = 0; idx < spreadsheetDoc->GetSpreadsheetDesc().GetAreaNumberRows(); idx++) {
			const CSpreadsheetAreaDescription* area = spreadsheetDoc->GetSpreadsheetDesc().GetRowAreaByIdx(idx);
			if (area == nullptr)
				continue;
			CValueStructure::Insert(area->m_label,
				CValue::CreateAndPrepareValueRef<CValueSpreadsheetDocumentRange>(area->m_label, area->m_start, area->m_end));
		}
	}

private:

	wxObjectDataPtr<CBackendSpreadsheetObject> m_spreadsheetDoc;
	wxDECLARE_DYNAMIC_CLASS(CValueSpreadsheetDocumentAreaCollection);
};

class CValueSpreadsheetDocumentParameterCollection : public CValue {

	enum
	{
		enCount,
		enFill,
		enGet,
		enSet,
	};

	static wxVector<wxString> ParseBrackets(const wxString& str) {

		wxVector<wxString> tokens;

		size_t start_pos = 0;
		size_t end_pos = 0;

		const wxString delimiters = wxT("[]");

		// Find the first opening or closing bracket
		start_pos = str.find_first_of(delimiters, start_pos);

		while (start_pos != wxString::npos) {

			// Find the next bracket of any type
			end_pos = str.find_first_of(delimiters, start_pos + 1);

			if (end_pos != wxString::npos) {
				// Extract the substring between the brackets
				// +1 to start after the opening bracket
				wxString token = str.substr(start_pos + 1, end_pos - start_pos - 1);
				if (!token.empty()) {
					tokens.push_back(token);
				}
				// Move start_pos to the character after the closing bracket for the next iteration
				start_pos = end_pos + 1;
			}
			else {
				// No matching end bracket found, stop
				break;
			}

			// Find the next opening bracket for the next iteration
			start_pos = str.find_first_of(delimiters, start_pos);
		}

		return tokens;
	}

public:

	CValueSpreadsheetDocumentParameterCollection() :
		CValue(eValueTypes::TYPE_VALUE), m_methodHelper(nullptr)
	{
	}

	CValueSpreadsheetDocumentParameterCollection(const wxObjectDataPtr<CBackendSpreadsheetObject>& spreadsheetDoc) :
		CValue(eValueTypes::TYPE_VALUE), m_spreadsheetDoc(spreadsheetDoc), m_methodHelper(new CMethodHelper)
	{
	}

	virtual ~CValueSpreadsheetDocumentParameterCollection() { wxDELETE(m_methodHelper); }

	virtual bool IsEmpty() const { return false; }

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal) {
		m_spreadsheetDoc->SetParameter(m_methodHelper->GetPropName(lPropNum), varPropVal);
		return true;
	}

	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal) {
		m_spreadsheetDoc->GetParameter(m_methodHelper->GetPropName(lPropNum), pvarPropVal);
		return true;
	}

	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray) {

		switch (lMethodNum)
		{
		case enCount:
			pvarRetValue = m_methodHelper ? m_methodHelper->GetNProps() : 0;
			return true;
		case enGet:
			m_spreadsheetDoc->GetParameter(paParams[0]->GetString(), pvarRetValue);
			return true;
		}

		return false;
	}

	virtual bool CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray) {

		switch (lMethodNum)
		{
		case enFill:
		{
			CMethodHelper* methodHelper = paParams[0]->GetPMethods();
			if (methodHelper != nullptr) {
				for (int lPropPos = 0; lPropPos <= m_methodHelper->GetNProps(); lPropPos++) {
					wxString strPropName = methodHelper->GetPropName(lPropPos);
					long lPropNum = methodHelper->FindProp(strPropName);
					if (lPropNum == wxNOT_FOUND)
						continue;
					CValue pvarRetValue;
					paParams[0]->GetPropVal(lPropNum, pvarRetValue);
					m_spreadsheetDoc->SetParameter(strPropName, pvarRetValue);
				}
				return true;
			}
			return false;
		}
		case enSet:
			m_spreadsheetDoc->SetParameter(paParams[0]->GetString(), *paParams[1]);
			return true;
		}

		return false;
	}

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}

	virtual void PrepareNames() const { // this method is automatically called to initialize attribute and method names.

		m_methodHelper->ClearHelper();

		m_methodHelper->AppendFunc(wxT("count"), wxT("count"));
		m_methodHelper->AppendProc(wxT("fill"), 1, wxT("fill(any : value)"));
		m_methodHelper->AppendFunc(wxT("get"), 1, wxT("get(parameter: string)"));
		m_methodHelper->AppendProc(wxT("set"), 2, wxT("set(parameter: string, any: value)"));

		for (int idx = 0; idx < m_spreadsheetDoc->GetSpreadsheetDesc().GetCellCount(); idx++) {

			const CSpreadsheetCellDescription* cell = m_spreadsheetDoc->GetSpreadsheetDesc().GetCellByIdx(idx);
			if (cell == nullptr)
				continue;

			if (cell->m_fillSetType == enSpreadsheetFillType::enSpreadsheetFillType_StrParameter) {
				if (!cell->IsEmptyValue()) m_methodHelper->AppendProp(cell->m_value);
			}
			else if (cell->m_fillSetType == enSpreadsheetFillType::enSpreadsheetFillType_StrTemplate) {
				if (!cell->IsEmptyValue()) for (auto s : ParseBrackets(cell->m_value)) if (!s.IsEmpty()) m_methodHelper->AppendProp(s);
			}
		}
	}

private:

	wxObjectDataPtr<CBackendSpreadsheetObject> m_spreadsheetDoc;
	CMethodHelper* m_methodHelper;
	wxDECLARE_DYNAMIC_CLASS(CValueSpreadsheetDocumentParameterCollection);
};

wxIMPLEMENT_DYNAMIC_CLASS(CValueSpreadsheetDocumentRange, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueSpreadsheetDocumentAreaCollection, CValueStructure);
wxIMPLEMENT_DYNAMIC_CLASS(CValueSpreadsheetDocumentParameterCollection, CValue);

#pragma endregion

wxIMPLEMENT_DYNAMIC_CLASS(CValueSpreadsheetDocument, CValue);

wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumSpreadsheetOrient, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumSpreadsheetHorizontalAlignment, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumSpreadsheetVerticalAlignment, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumSpreadsheetFitMode, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumSpreadsheetBorder, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumSpreadsheetFillType, CValue);

CValue::CMethodHelper CValueSpreadsheetDocument::m_methodHelper;
CValue::CMethodHelper CValueSpreadsheetDocumentRange::m_methodHelper;

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
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueSpreadsheetDocumentAreaCollection>(m_spreadsheetDoc);
		return true;
	case eParameters:
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueSpreadsheetDocumentParameterCollection>(m_spreadsheetDoc);
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

#include <wx/tokenzr.h>

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

		return false;
	}
	else if (lMethodNum == eRange) {

		if (paParams[0]->GetInteger() > m_spreadsheetDoc->GetNumberRows())
			return false;

		else if (paParams[1]->GetInteger() > m_spreadsheetDoc->GetNumberCols())
			return false;

		pvarRetValue = CValue::CreateAndPrepareValueRef<CValueSpreadsheetDocument>(m_spreadsheetDoc->GetArea(
			paParams[0]->GetInteger(), paParams[1]->GetInteger(), lSizeArray > 2 ? paParams[2]->GetInteger() : -1, lSizeArray > 3 ? paParams[3]->GetInteger() : -1));

		return true;
	}
	else if (lMethodNum == eGetArea) {

		if (lSizeArray == 1) {

			wxStringTokenizer tkn(paParams[0]->GetString(), wxT("|"));

			const CSpreadsheetAreaDescription* r = m_spreadsheetDoc->GetSpreadsheetDesc().GetRowAreaByName(tkn.GetNextToken());
			const CSpreadsheetAreaDescription* c = m_spreadsheetDoc->GetSpreadsheetDesc().GetRowAreaByName(tkn.GetNextToken());

			if (!r)
				return false;

			pvarRetValue = CValue::CreateAndPrepareValueRef<CValueSpreadsheetDocument>(m_spreadsheetDoc->GetAreaByName(r->m_label, c ? c->m_label : wxT("")));
			return true;
		}

		const CSpreadsheetAreaDescription* r = m_spreadsheetDoc->GetSpreadsheetDesc().GetRowAreaByName(paParams[0]->GetString());
		const CSpreadsheetAreaDescription* c = m_spreadsheetDoc->GetSpreadsheetDesc().GetRowAreaByName(lSizeArray > 1 ? paParams[1]->GetString() : wxT(""));

		if (!r)
			return false;

		pvarRetValue = CValue::CreateAndPrepareValueRef<CValueSpreadsheetDocument>(m_spreadsheetDoc->GetAreaByName(
			r->m_label, c ? c->m_label : wxT("")));

		return true;
	}

	return false;
}

#include "backend/backend_exception.h"

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
			m_spreadsheetDoc->PutArea(valueSpreadsheet->GetSpreadsheetDocument());
		return true;
	}
	else if (lMethodNum == eJoin) {
		CValuePtr<CValueSpreadsheetDocument> valueSpreadsheet =
			paParams[0]->ConvertToType<CValueSpreadsheetDocument>();
		if (valueSpreadsheet)
			m_spreadsheetDoc->JoinArea(valueSpreadsheet->GetSpreadsheetDocument());
		return true;
	}

	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

VALUE_TYPE_REGISTER(CValueSpreadsheetDocument, "spreadsheetDocument", string_to_clsid("VL_SPSTD"));
SYSTEM_TYPE_REGISTER(CValueSpreadsheetDocumentRange, "spreadsheetAreaRange", string_to_clsid("SY_SPPRA"));
SYSTEM_TYPE_REGISTER(CValueSpreadsheetDocumentAreaCollection, "spreadsheetAreaCollection", string_to_clsid("SY_SPAEA"));
SYSTEM_TYPE_REGISTER(CValueSpreadsheetDocumentParameterCollection, "spreadsheetParameterCollection", string_to_clsid("SY_SPPRM"));
ENUM_TYPE_REGISTER(CValueEnumSpreadsheetOrient, "spreadsheetOrient", string_to_clsid("EN_SORNT"));
ENUM_TYPE_REGISTER(CValueEnumSpreadsheetHorizontalAlignment, "spreadsheetHorizontalAlignment", string_to_clsid("EN_SHOAL"));
ENUM_TYPE_REGISTER(CValueEnumSpreadsheetVerticalAlignment, "spreadsheetVerticalAlignment", string_to_clsid("EN_SVEAL"));
ENUM_TYPE_REGISTER(CValueEnumSpreadsheetBorder, "spreadsheetBorder", string_to_clsid("EN_SBORD"));
ENUM_TYPE_REGISTER(CValueEnumSpreadsheetFitMode, "spreadsheetFitMode", string_to_clsid("EN_SFTMD"));
ENUM_TYPE_REGISTER(CValueEnumSpreadsheetFillType, "spreadsheetTemplate", string_to_clsid("EN_SFTMP"));