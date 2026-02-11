#include "gridbox.h"

//***********************************************************************************
//*                           IMPLEMENT_DYNAMIC_CLASS                               *
//***********************************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CValueGridBox, IValueWindow);

//***********************************************************************************
//*                                 Value Notebook                                  *
//***********************************************************************************

CValueGridBox::CValueGridBox() : IValueWindow(), m_valueSpreadsheet(new CValueSpreadsheet)
{
	//set default params
	m_propertyMinSize->SetValue(wxSize(300, 100));
}

#include "frontend/visualView/ctrl/form.h"

wxObject* CValueGridBox::Create(wxWindow* wxparent, IVisualHost* visualHost)
{
	CGridEditor* gridWindow = new CGridEditor(nullptr, wxparent, wxID_ANY, wxDefaultPosition, wxDefaultSize);

	gridWindow->EnableProperty(!visualHost->IsDesignerHost());
	gridWindow->EnableGridArea(false);
	gridWindow->EnableGridLines(false);

	gridWindow->LoadDocument(m_valueSpreadsheet->GetSpreadsheetDocument());

	return gridWindow;
}

void CValueGridBox::OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated)
{
	CGridEditor* gridWindow = dynamic_cast<CGridEditor*>(wxobject);
}

void CValueGridBox::OnSelected(wxObject* wxobject)
{
}

void CValueGridBox::Update(wxObject* wxobject, IVisualHost* visualHost)
{
	CGridEditor* gridWindow = dynamic_cast<CGridEditor*>(wxobject);

	if (gridWindow) {
	}

	UpdateWindow(gridWindow);
}

void CValueGridBox::Cleanup(wxObject* wxobject, IVisualHost* visualHost)
{
}

//**********************************************************************************

#include "frontend/win/editor/gridEditor/gridPrintout.h"

wxPrintout* CValueGridBox::CreatePrintout() const
{
	CGridEditor* gridWindow = dynamic_cast<CGridEditor*>(GetWxObject());
	if (gridWindow != nullptr)
		return gridWindow->CreatePrintout();

	return nullptr;
}

//**********************************************************************************
//*                                   Data										   *
//**********************************************************************************

bool CValueGridBox::LoadData(CMemoryReader& reader)
{
	CSpreadsheetDescriptionMemory::LoadData(reader, m_valueSpreadsheet->GetSpreadsheetDesc());
	return IValueWindow::LoadData(reader);
}

bool CValueGridBox::SaveData(CMemoryWriter& writer)
{
	CSpreadsheetDescriptionMemory::SaveData(writer, m_valueSpreadsheet->GetSpreadsheetDesc());
	return IValueWindow::SaveData(writer);
}

//***********************************************************************************

enum prop {
	eGridValue,
};

void CValueGridBox::PrepareNames() const
{
	IValueFrame::PrepareNames();

	m_methodHelper->AppendProp(wxT("value"), eGridValue, eControl);
}

bool CValueGridBox::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	const long lPropAlias = m_methodHelper->GetPropAlias(lPropNum); bool refreshColumn = false;
	if (lPropAlias == eControl) {
		const long lPropData = m_methodHelper->GetPropData(lPropNum);
		if (lPropData == eGridValue) {
			CGridEditor* gridWindow = dynamic_cast<CGridEditor*>(GetWxObject());
			m_valueSpreadsheet = varPropVal.ConvertToType<CValueSpreadsheet>();
			if (gridWindow != nullptr) gridWindow->LoadDocument(m_valueSpreadsheet->GetSpreadsheetDesc());
		}
	}

	return IValueFrame::SetPropVal(lPropNum, varPropVal);
}

bool CValueGridBox::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const long lPropAlias = m_methodHelper->GetPropAlias(lPropNum);
	if (lPropAlias == eControl) {
		const long lPropData = m_methodHelper->GetPropData(lPropNum);
		if (lPropData == eGridValue) {
			pvarPropVal = m_valueSpreadsheet;
			return true;
		}
	}
	return IValueFrame::GetPropVal(lPropNum, pvarPropVal);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

CONTROL_TYPE_REGISTER(CValueGridBox, "gridbox", "container", string_to_clsid("CT_GRID"));