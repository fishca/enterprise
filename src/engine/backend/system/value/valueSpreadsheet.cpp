#include "valueSpreadsheet.h"
#include "backend/backend_exception.h"
#include "backend/backend_mainFrame.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueSpreadsheet, CValue);

CValue::CMethodHelper CValueSpreadsheet::m_methodHelper;

enum
{
	ePrint, 
	eShow
};

void CValueSpreadsheet::PrepareNames() const
{
	m_methodHelper.ClearHelper();

	m_methodHelper.AppendProc(wxT("print"), wxT("print()"));
	m_methodHelper.AppendProc(wxT("show"), 1, wxT("show(string: title)"));
}

bool CValueSpreadsheet::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	return false;
}

bool CValueSpreadsheet::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	return false;
}

bool CValueSpreadsheet::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	return false;
}

bool CValueSpreadsheet::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
{

	if (lMethodNum == ePrint)
	{
		if (backend_mainFrame != nullptr)
			return backend_mainFrame->PrintSpreadSheetDocument(m_spreadsheetDesc);

		CBackendCoreException::Error(_("Context functions are not available!"));
		return false;
	}
	else if (lMethodNum == eShow)
	{
		if (backend_mainFrame != nullptr)
			return backend_mainFrame->ShowSpreadSheetDocument(paParams[0]->GetString(), m_spreadsheetDesc);
		
		CBackendCoreException::Error(_("Context functions are not available!"));
		return false;
	}

	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

VALUE_TYPE_REGISTER(CValueSpreadsheet, "spreadsheetDocument", string_to_clsid("VL_SPSTD"));