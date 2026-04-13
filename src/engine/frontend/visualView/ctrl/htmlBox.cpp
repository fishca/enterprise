#include "htmlbox.h"

//***********************************************************************************
//*                           IMPLEMENT_DYNAMIC_CLASS                               *
//***********************************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CValueHTMLBox, IValueWindow);

//***********************************************************************************
//*                                 Value Notebook                                  *
//***********************************************************************************

CValueHTMLBox::CValueHTMLBox() : IValueWindow()
{
	m_propertyMinSize->SetValue(wxSize(250, 150));
}

wxObject* CValueHTMLBox::Create(wxWindow* wxparent, IVisualHost* visualHost)
{
	wxHtmlWindow* htmlBox = new wxHtmlWindow(wxparent, wxID_ANY,
		wxDefaultPosition,
		wxDefaultSize);

	wxString dummy_page(
		wxT("<b>wxHtmlWindow</b><br />")
		wxT("This is a dummy page.</body></html>"));

	htmlBox->SetPage(dummy_page);

	return htmlBox;
}

void CValueHTMLBox::OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated)
{
	wxHtmlWindow* htmlBox = dynamic_cast<wxHtmlWindow*>(wxobject);
}

void CValueHTMLBox::OnSelected(wxObject* wxobject)
{
}

void CValueHTMLBox::Update(wxObject* wxobject, IVisualHost* visualHost)
{
	wxHtmlWindow* htmlBox = dynamic_cast<wxHtmlWindow*>(wxobject);

	if (htmlBox)
	{
	}

	UpdateWindow(htmlBox);
}

void CValueHTMLBox::Cleanup(wxObject* obj, IVisualHost* visualHost)
{
}

//**********************************************************************************
//*                                   Data										   *
//**********************************************************************************

bool CValueHTMLBox::LoadData(CMemoryReader& reader)
{
	return IValueWindow::LoadData(reader);
}

bool CValueHTMLBox::SaveData(CMemoryWriter& writer)
{
	return IValueWindow::SaveData(writer);
}

//**********************************************************************************

enum Func {
	enSetPage = 0,
};

void CValueHTMLBox::PrepareNames() const // this method is automatically called to initialize attribute and method names.
{
	IValueFrame::PrepareNames();

	m_methodHelper->AppendFunc(wxT("SetPage"), 1, wxT("SetPage(p: page)"), enSetPage, wxNOT_FOUND);
}

bool CValueHTMLBox::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)       //method call
{
	wxHtmlWindow* htmlBox = dynamic_cast<wxHtmlWindow*>(GetWxObject());
	switch (m_methodHelper->GetMethodData(lMethodNum))
	{
	case enSetPage:
		pvarRetValue = htmlBox ?
			htmlBox->SetPage(paParams[0]->GetString()) : !paParams[0]->IsEmpty();
		return true;
	}

	return IValueFrame::CallAsFunc(lMethodNum, pvarRetValue, paParams, lSizeArray);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

CONTROL_TYPE_REGISTER(CValueHTMLBox, "Htmlbox", "Container", string_to_clsid("CT_HTML"));