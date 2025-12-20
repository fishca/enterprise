#include "widgets.h"
#include "backend/compiler/procUnit.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueButton, IValueWindow)

//****************************************************************************
//*                              Button                                      *
//****************************************************************************

CValueButton::CValueButton() : IValueWindow()
{
}

wxObject* CValueButton::Create(wxWindow* wxparent, IVisualHost* visualHost)
{
	wxButton* wxbutton = new wxButton(wxparent, wxID_ANY, m_propertyCaption->GetValueAsString());
	//setup event 
	wxbutton->Bind(wxEVT_BUTTON, &CValueButton::OnButtonPressed, this);
	return wxbutton;
}

void CValueButton::OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated)
{
}

void CValueButton::Update(wxObject* wxobject, IVisualHost* visualHost)
{
	wxButton* button = dynamic_cast<wxButton*>(wxobject);

	if (button != nullptr) {

		if (m_propertyRepresentation->GetValueAsEnum() == enRepresentation::eRepresentation_PictureAndText) {
			button->SetLabel(m_propertyCaption->GetValueAsString());
			button->SetBitmap(m_propertyPicture->GetValueAsBitmap());
		}
		else if (m_propertyRepresentation->GetValueAsEnum() == enRepresentation::eRepresentation_Picture) {
			button->SetLabel(wxEmptyString);
			button->SetBitmap(m_propertyPicture->GetValueAsBitmap());
		}
		else if (m_propertyRepresentation->GetValueAsEnum() == enRepresentation::eRepresentation_Text) {
			button->SetLabel(m_propertyCaption->GetValueAsString());
			button->SetBitmap(wxNullBitmap);
		}
	}

	UpdateWindow(button);
}

void CValueButton::Cleanup(wxObject* obj, IVisualHost* visualHost)
{
}

//*******************************************************************
//*                           Data									*
//*******************************************************************

bool CValueButton::LoadData(CMemoryReader& reader)
{
	m_propertyCaption->LoadData(reader);
	m_propertyRepresentation->LoadData(reader);
	m_propertyPicture->LoadData(reader);

	//events
	m_onButtonPressed->LoadData(reader);

	return IValueWindow::LoadData(reader);
}

bool CValueButton::SaveData(CMemoryWriter& writer)
{
	m_propertyCaption->SaveData(writer);
	m_propertyRepresentation->SaveData(writer);
	m_propertyPicture->SaveData(writer);

	//events
	m_onButtonPressed->SaveData(writer);

	return IValueWindow::SaveData(writer);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

CONTROL_TYPE_REGISTER(CValueButton, "button", "widget", string_to_clsid("CT_BUTN"));