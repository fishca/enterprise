#include "gridbox.h"

//***********************************************************************************
//*                           IMPLEMENT_DYNAMIC_CLASS                               *
//***********************************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CValueGridBox, IValueWindow);

//***********************************************************************************
//*                                 Value Notebook                                  *
//***********************************************************************************

CValueGridBox::CValueGridBox() : IValueWindow()
{
	//set default params
	m_propertyMinSize->SetValue(wxSize(300, 100));
}

wxObject* CValueGridBox::Create(wxWindow* wxparent, IVisualHost* visualHost)
{
	CGridExtCtrl* gridWindow = new CGridExtCtrl(wxparent, wxID_ANY, wxDefaultPosition, wxDefaultSize);
	
	gridWindow->EnableProperty(!visualHost->IsDesignerHost());
	gridWindow->EnableGridArea(false);
	gridWindow->EnableGridLines(false);
	
	return gridWindow;
}

void CValueGridBox::OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated)
{
	CGridExtCtrl* gridWindow = dynamic_cast<CGridExtCtrl*>(wxobject);
}

void CValueGridBox::OnSelected(wxObject* wxobject)
{
}

void CValueGridBox::Update(wxObject* wxobject, IVisualHost* visualHost)
{
	CGridExtCtrl* gridWindow = dynamic_cast<CGridExtCtrl*>(wxobject);

	if (gridWindow) {
	}

	UpdateWindow(gridWindow);
}

void CValueGridBox::Cleanup(wxObject* wxobject, IVisualHost* visualHost)
{
}

//**********************************************************************************
//*                                   Data										   *
//**********************************************************************************

bool CValueGridBox::LoadData(CMemoryReader& reader)
{
	return IValueWindow::LoadData(reader);
}

bool CValueGridBox::SaveData(CMemoryWriter& writer)
{
	return IValueWindow::SaveData(writer);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

CONTROL_TYPE_REGISTER(CValueGridBox, "gridbox", "container", string_to_clsid("CT_GRID"));