#include "textBox.h"

//***********************************************************************************
//*                           IMPLEMENT_DYNAMIC_CLASS                               *
//***********************************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CValueTextBox, IValueWindow);

//***********************************************************************************
//*                                 Value Notebook                                  *
//***********************************************************************************

CValueTextBox::CValueTextBox() : IValueWindow()
{
	//set default params
	m_propertyMinSize->SetValue(wxSize(150, 50));
}

#include "frontend/visualView/ctrl/form.h"

wxObject* CValueTextBox::Create(wxWindow* wxparent, IVisualHost* visualHost)
{
	CTextEditor* textWindow = new CTextEditor(nullptr, wxparent, wxID_ANY, wxDefaultPosition, wxDefaultSize);
	return textWindow;
}

void CValueTextBox::OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated)
{
	CTextEditor* textWindow = dynamic_cast<CTextEditor*>(wxobject);
}

void CValueTextBox::OnSelected(wxObject* wxobject)
{
}

void CValueTextBox::Update(wxObject* wxobject, IVisualHost* visualHost)
{
	CTextEditor* textWindow = dynamic_cast<CTextEditor*>(wxobject);

	if (textWindow) {
	}

	UpdateWindow(textWindow);
}

void CValueTextBox::Cleanup(wxObject* wxobject, IVisualHost* visualHost)
{
}

//**********************************************************************************

#include "frontend/win/editor/textEditor/textEditorPrintOut.h"

wxPrintout* CValueTextBox::CreatePrintout() const
{
	CTextEditor* gridWindow = dynamic_cast<CTextEditor*>(GetWxObject());
	if (gridWindow != nullptr)
		return new CTextEditorPrintout(gridWindow);

	return false;
}

//**********************************************************************************
//*                                   Data										   *
//**********************************************************************************

bool CValueTextBox::LoadData(CMemoryReader& reader)
{
	return IValueWindow::LoadData(reader);
}

bool CValueTextBox::SaveData(CMemoryWriter& writer)
{
	return IValueWindow::SaveData(writer);
}

//***********************************************************************************

void CValueTextBox::PrepareNames() const
{
	IValueFrame::PrepareNames();
}

bool CValueTextBox::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	return IValueFrame::SetPropVal(lPropNum, varPropVal);
}

bool CValueTextBox::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	return IValueFrame::GetPropVal(lPropNum, pvarPropVal);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

CONTROL_TYPE_REGISTER(CValueTextBox, "Textbox", "Container", string_to_clsid("CT_TEXT"));