
#include "widgets.h"
#include "backend/compiler/procUnit.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueStaticText, IValueWindow)

//****************************************************************************
//*                              StaticText                                  *
//****************************************************************************

CValueStaticText::CValueStaticText() : IValueWindow()
{
}

wxObject* CValueStaticText::Create(wxWindow* wxparent, IVisualHost* visualHost)
{
	wxStaticText* staticText = new wxStaticText(wxparent, wxID_ANY,
		m_propertyTitle->GetValueAsTranslateString(),
		wxDefaultPosition,
		wxDefaultSize);

	return staticText;
}

void CValueStaticText::OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated)
{
}

void CValueStaticText::Update(wxObject* wxobject, IVisualHost* visualHost)
{
	wxStaticText* staticText = dynamic_cast<wxStaticText*>(wxobject);

	if (staticText != nullptr) {
			
		staticText->SetLabel(m_propertyTitle->GetValueAsTranslateString());
		staticText->Wrap(m_propertyWrap->GetValueAsUInteger());
		
		if (m_propertyMarkup->GetValueAsBoolean() != false) {
			staticText->SetLabelMarkup(m_propertyTitle->GetValueAsTranslateString());
		}
	}

	UpdateWindow(staticText);
}

void CValueStaticText::Cleanup(wxObject* obj, IVisualHost* visualHost)
{
}

//*******************************************************************
//*                              Data	                            *
//*******************************************************************

bool CValueStaticText::LoadData(CMemoryReader& reader)
{
	m_propertyMarkup->SetValue(reader.r_u8());
	m_propertyWrap->SetValue(reader.r_u32());
	wxString label; reader.r_stringZ(label);
	m_propertyTitle->SetValue(label);
	return IValueWindow::LoadData(reader);
}

bool CValueStaticText::SaveData(CMemoryWriter& writer)
{
	writer.w_u8(m_propertyMarkup->GetValueAsBoolean());
	writer.w_u32(m_propertyWrap->GetValueAsUInteger());
	writer.w_stringZ(m_propertyTitle->GetValueAsString());

	return IValueWindow::SaveData(writer);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

CONTROL_TYPE_REGISTER(CValueStaticText, "Statictext", "Widget", string_to_clsid("CT_STTX"));