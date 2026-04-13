#include "valueSpreadsheet.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueSpreadsheetDocumentArea, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueSpreadsheetDocumentBorder, CValue);

CValue::CMethodHelper CValueSpreadsheetDocumentArea::m_methodHelper;
CValue::CMethodHelper CValueSpreadsheetDocumentBorder::m_methodHelper;

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

void CValueSpreadsheetDocumentArea::PrepareNames() const
{
	m_methodHelper.ClearHelper();
	m_methodHelper.AppendProp(wxT("BackgroundColour"));
	m_methodHelper.AppendProp(wxT("TextColour"));
	m_methodHelper.AppendProp(wxT("TextOrient"));
	m_methodHelper.AppendProp(wxT("Font"));
	m_methodHelper.AppendProp(wxT("AlignHorizontal"));
	m_methodHelper.AppendProp(wxT("AlignVertical"));

	m_methodHelper.AppendProp(wxT("BorderLeft"));
	m_methodHelper.AppendProp(wxT("BorderRight"));
	m_methodHelper.AppendProp(wxT("BorderTop"));
	m_methodHelper.AppendProp(wxT("BorderBottom"));

	m_methodHelper.AppendProp(wxT("Size"));
	m_methodHelper.AppendProp(wxT("ReadOnly"));

	m_methodHelper.AppendProp(wxT("Value"));
}

#include "valueFont.h"
#include "valueColour.h"
#include "valueSize.h"

bool CValueSpreadsheetDocumentArea::SetPropVal(const long lPropNum, const CValue& varPropVal)
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
			varPropVal.ConvertToEnumType<enSpreadsheetAlignmentHorz>(), vertical);
		return true;
	}
	case eAlignmentVert:
	{
		int horizontal;
		m_spreadsheetDoc->GetCellAlignment(m_row, m_col, &horizontal, nullptr);
		m_spreadsheetDoc->SetCellAlignment(m_row, m_col,
			horizontal, varPropVal.ConvertToEnumType<enSpreadsheetAlignmentVert>());
		return true;
	}
	case eBorderLeft:
	{
		CValuePtr<CValueSpreadsheetDocumentBorder> valueBorder = varPropVal.ConvertToType<CValueSpreadsheetDocumentBorder>();
		m_spreadsheetDoc->SetCellBorderLeft(m_row, m_col, { valueBorder->GetStyle(), valueBorder->GetColour(), valueBorder->GetWidth() });
		return true;
	}
	case eBorderRight:
	{
		CValuePtr<CValueSpreadsheetDocumentBorder> valueBorder = varPropVal.ConvertToType<CValueSpreadsheetDocumentBorder>();
		m_spreadsheetDoc->SetCellBorderRight(m_row, m_col, { valueBorder->GetStyle(), valueBorder->GetColour(), valueBorder->GetWidth() });
		return true;
	}
	case eBorderTop:
	{
		CValuePtr<CValueSpreadsheetDocumentBorder> valueBorder = varPropVal.ConvertToType<CValueSpreadsheetDocumentBorder>();
		m_spreadsheetDoc->SetCellBorderTop(m_row, m_col, { valueBorder->GetStyle(), valueBorder->GetColour(), valueBorder->GetWidth() });
		return true;
	}
	case eBorderBottom:
	{
		CValuePtr<CValueSpreadsheetDocumentBorder> valueBorder = varPropVal.ConvertToType<CValueSpreadsheetDocumentBorder>();
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

bool CValueSpreadsheetDocumentArea::GetPropVal(const long lPropNum, CValue& pvarPropVal)
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
		pvarPropVal = CValue::CreateAndConvertEnumObjectRef<CValueEnumSpreadsheetOrient>(
			static_cast<enSpreadsheetOrientation>(m_spreadsheetDoc->GetCellTextOrient(m_row, m_col)));
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
		pvarPropVal = CValue::CreateAndConvertEnumObjectRef<CValueEnumSpreadsheetHorizontalAlignment>(
			static_cast<enSpreadsheetAlignmentHorz>(horizontal));
		return true;
	}
	case eAlignmentVert:
	{
		int vertical;
		m_spreadsheetDoc->GetCellAlignment(m_row, m_col, nullptr, &vertical);
		pvarPropVal = CValue::CreateAndConvertEnumObjectRef<CValueEnumSpreadsheetVerticalAlignment>(
			static_cast<enSpreadsheetAlignmentVert>(vertical));
		return true;
	}
	case eBorderLeft:
	{
		const CSpreadsheetBorderDescription& borderDesc = m_spreadsheetDoc->GetCellBorderLeft(m_row, m_col);
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueSpreadsheetDocumentBorder>(borderDesc.m_style, borderDesc.m_colour, borderDesc.m_width);
		return true;
	}
	case eBorderRight:
	{
		const CSpreadsheetBorderDescription& borderDesc = m_spreadsheetDoc->GetCellBorderRight(m_row, m_col);
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueSpreadsheetDocumentBorder>(borderDesc.m_style, borderDesc.m_colour, borderDesc.m_width);
		return true;
	}
	case eBorderTop:
	{
		const CSpreadsheetBorderDescription& borderDesc = m_spreadsheetDoc->GetCellBorderTop(m_row, m_col);
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueSpreadsheetDocumentBorder>(borderDesc.m_style, borderDesc.m_colour, borderDesc.m_width);
		return true;
	}
	case eBorderBottom:
	{
		const CSpreadsheetBorderDescription& borderDesc = m_spreadsheetDoc->GetCellBorderBottom(m_row, m_col);
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueSpreadsheetDocumentBorder>(borderDesc.m_style, borderDesc.m_colour, borderDesc.m_width);
		return true;
	}
	case eSize:
	{
		CValuePtr<CValueSize> valueSize(CValue::CreateAndPrepareValueRef<CValueSize>());
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

bool CValueSpreadsheetDocumentArea::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	return false;
}

bool CValueSpreadsheetDocumentArea::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
{
	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

SYSTEM_TYPE_REGISTER(CValueSpreadsheetDocumentArea, "SpreadsheetArea", string_to_clsid("VL_SPSTA"));
VALUE_TYPE_REGISTER(CValueSpreadsheetDocumentBorder, "SpreadsheetBorderRow", string_to_clsid("VL_SPSBO"));