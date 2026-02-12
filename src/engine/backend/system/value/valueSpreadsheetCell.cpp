#include "valueSpreadsheet.h"

CValue::CMethodHelper CValueSpreadsheetCell::m_methodHelper;

enum
{
	eBackgroundColour,
	eTextColour,
	eTextOrient,
	eFont,

	eAlignmentHorz,
	eAlignmentVert,

	eBorderLeft,
	eBorderRight,
	eBorderTop,
	eBorderBottom,

	eSize,
	eReadOnly,

	eValue
};

void CValueSpreadsheetCell::PrepareNames() const
{
	m_methodHelper.ClearHelper();
	m_methodHelper.AppendProc(wxT("backgroundColour"));
	m_methodHelper.AppendProc(wxT("textColour"));
	m_methodHelper.AppendProc(wxT("textOrient"));
	m_methodHelper.AppendProc(wxT("font"));
	m_methodHelper.AppendProc(wxT("alignHorizontal"));
	m_methodHelper.AppendProc(wxT("alignVertical"));

	m_methodHelper.AppendProc(wxT("borderLeft"));
	m_methodHelper.AppendProc(wxT("borderRight"));
	m_methodHelper.AppendProc(wxT("borderTop"));
	m_methodHelper.AppendProc(wxT("borderBottom"));

	m_methodHelper.AppendProc(wxT("size"));
	m_methodHelper.AppendProc(wxT("readOnly"));

	m_methodHelper.AppendProc(wxT("value"));
}

#include "valueFont.h"
#include "valueColour.h"
#include "valueSize.h"

bool CValueSpreadsheetCell::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	switch (lPropNum)
	{
	case eBackgroundColour:
	{
		m_spreadsheetDoc->SetCellBackgroundColour(m_row, m_col, *varPropVal.ConvertToType<CValueColour>());
		return true;
	}
	case eTextColour:
	{
		m_spreadsheetDoc->SetCellTextColour(m_row, m_col, *varPropVal.ConvertToType<CValueColour>());
		return true;
	}
	case eTextOrient:
	{
		m_spreadsheetDoc->SetCellTextOrient(m_row, m_col, varPropVal.ConvertToEnumType<wxOrientation>());
		return true;
	}
	case eFont:
	{
		m_spreadsheetDoc->SetCellFont(m_row, m_col, *varPropVal.ConvertToType<CValueFont>());
		return true;
	}
	case eAlignmentHorz:
	{
		int vertical;
		m_spreadsheetDoc->GetCellAlignment(m_row, m_col, nullptr, &vertical);
		m_spreadsheetDoc->SetCellAlignment(m_row, m_col,
			varPropVal.ConvertToEnumType<wxAlignment>(), vertical);
		return true;
	}
	case eAlignmentVert:
	{
		int horizontal;
		m_spreadsheetDoc->GetCellAlignment(m_row, m_col, &horizontal, nullptr);
		//m_spreadsheetDoc->SetCellAlignment(m_row, m_col,
		//	horizontal, varPropVal.ConvertToEnumType<wxAlignment>());
		return true;
	}
	case eBorderLeft:
	{
		CValuePtr<CValueSpreadsheetBorder> valueBorder = varPropVal.ConvertToType<CValueSpreadsheetBorder>();
		m_spreadsheetDoc->SetCellBorderLeft(m_row, m_col, { valueBorder->GetStyle(), valueBorder->GetColour(), valueBorder->GetWidth() });
		return true;
	}
	case eBorderRight:
	{
		CValuePtr<CValueSpreadsheetBorder> valueBorder = varPropVal.ConvertToType<CValueSpreadsheetBorder>();
		m_spreadsheetDoc->SetCellBorderRight(m_row, m_col, { valueBorder->GetStyle(), valueBorder->GetColour(), valueBorder->GetWidth() });
		return true;
	}
	case eBorderTop:
	{
		CValuePtr<CValueSpreadsheetBorder> valueBorder = varPropVal.ConvertToType<CValueSpreadsheetBorder>();
		m_spreadsheetDoc->SetCellBorderTop(m_row, m_col, { valueBorder->GetStyle(), valueBorder->GetColour(), valueBorder->GetWidth() });
		return true;
	}
	case eBorderBottom:
	{
		CValuePtr<CValueSpreadsheetBorder> valueBorder = varPropVal.ConvertToType<CValueSpreadsheetBorder>();
		m_spreadsheetDoc->SetCellBorderBottom(m_row, m_col, { valueBorder->GetStyle(), valueBorder->GetColour(), valueBorder->GetWidth() });
		return true;
	}
	case eSize:
	{
		CValuePtr<CValueSize> valueSize = varPropVal.ConvertToType<CValueSize>();
		m_spreadsheetDoc->SetCellSize(m_row, m_col, valueSize->m_size.x, valueSize->m_size.y);
		return true;
	}
	case eReadOnly:
	{
		m_spreadsheetDoc->SetCellReadOnly(m_row, m_col, varPropVal.GetBoolean());
		return true;
	}
	case eValue:
	{
		m_spreadsheetDoc->SetCellValue(m_row, m_col, varPropVal.GetString());
		return true;
	}
	}

	return false;
}

bool CValueSpreadsheetCell::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	switch (lPropNum)
	{
	case eBackgroundColour:
	{
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueColour>(m_spreadsheetDoc->GetCellBackgroundColour(m_row, m_col));
		return true;
	}
	case eTextColour:
	{
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueColour>(m_spreadsheetDoc->GetCellTextColour(m_row, m_col));
		return true;
	}
	case eTextOrient:
	{
		pvarPropVal = CValue::CreateAndConvertEnumObjectRef<CValueEnumSpreadsheetOrient>(m_spreadsheetDoc->GetCellTextOrient(m_row, m_col));
		return true;
	}
	case eFont:
	{
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueFont>(m_spreadsheetDoc->GetCellFont(m_row, m_col));
		return true;
	}
	case eAlignmentHorz:
	{
		int horizontal;
		m_spreadsheetDoc->GetCellAlignment(m_row, m_col, &horizontal, nullptr);
		pvarPropVal = CValue::CreateAndConvertEnumObjectRef<CValueEnumSpreadsheetHorizontalAlignment>(static_cast<wxAlignment>(horizontal));
		return true;
	}
	case eAlignmentVert:
	{
		int vertical;
		m_spreadsheetDoc->GetCellAlignment(m_row, m_col, nullptr, &vertical);
		pvarPropVal = CValue::CreateAndConvertEnumObjectRef<CValueEnumSpreadsheetVerticalAlignment>(static_cast<wxAlignment>(vertical));
		return true;
	}
	case eBorderLeft:
	{
		const CSpreadsheetBorderDescription& borderDesc = m_spreadsheetDoc->GetCellBorderLeft(m_row, m_col);
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueSpreadsheetBorder>(borderDesc.m_style, borderDesc.m_colour, borderDesc.m_width);
		return true;
	}
	case eBorderRight:
	{
		const CSpreadsheetBorderDescription& borderDesc = m_spreadsheetDoc->GetCellBorderRight(m_row, m_col);
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueSpreadsheetBorder>(borderDesc.m_style, borderDesc.m_colour, borderDesc.m_width);
		return true;
	}
	case eBorderTop:
	{
		const CSpreadsheetBorderDescription& borderDesc = m_spreadsheetDoc->GetCellBorderTop(m_row, m_col);
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueSpreadsheetBorder>(borderDesc.m_style, borderDesc.m_colour, borderDesc.m_width);
		return true;
	}
	case eBorderBottom:
	{
		const CSpreadsheetBorderDescription& borderDesc = m_spreadsheetDoc->GetCellBorderBottom(m_row, m_col);
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueSpreadsheetBorder>(borderDesc.m_style, borderDesc.m_colour, borderDesc.m_width);
		return true;
	}
	case eSize:
	{
		CValuePtr<CValueSize> valueSize = new CValueSize();
		m_spreadsheetDoc->GetCellSize(m_row, m_col, &valueSize->m_size.x, &valueSize->m_size.y);
		pvarPropVal = valueSize;
		return true;
	}
	case eReadOnly:
	{
		pvarPropVal = m_spreadsheetDoc->IsCellReadOnly(m_row, m_col);
		return true;
	}
	case eValue:
	{
		pvarPropVal = m_spreadsheetDoc->GetCellValue(m_row, m_col);
		return true;
	}
	}

	return false;
}

bool CValueSpreadsheetCell::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	return false;
}

bool CValueSpreadsheetCell::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
{
	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

SYSTEM_TYPE_REGISTER(CValueSpreadsheetCell, "spreadsheetCell", string_to_clsid("VL_SPSTA"));
VALUE_TYPE_REGISTER(CValueSpreadsheetBorder, "spreadsheetBorder", string_to_clsid("VL_SPSTA"));