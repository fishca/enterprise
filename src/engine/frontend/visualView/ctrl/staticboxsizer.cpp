
#include "sizer.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueStaticBoxSizer, IValueSizer)

//****************************************************************************
//*                             StaticBoxSizer                               *
//****************************************************************************

CValueStaticBoxSizer::CValueStaticBoxSizer() : IValueSizer()
{
}

wxObject* CValueStaticBoxSizer::Create(wxWindow* wxparent, IVisualHost* visualHost)
{
	wxStaticBox* staticBox = new wxStaticBox(wxparent, wxID_ANY, m_propertyCaption->GetValueAsTranslateString());
	return new wxStaticBoxSizer(staticBox, m_propertyOrient->GetValueAsInteger());
}

void CValueStaticBoxSizer::OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated)
{
	wxStaticBoxSizer* staticboxsizer = dynamic_cast<wxStaticBoxSizer*>(wxobject);
	wxStaticBox* staticBox = staticboxsizer->GetStaticBox();
	wxASSERT(staticBox);
	
	if (visualHost->IsDesignerHost()) {
		staticBox->PushEventHandler(g_visualHostContext->GetHighlightPaintHandler(staticBox));
	}
}

void CValueStaticBoxSizer::Update(wxObject* wxobject, IVisualHost* visualHost)
{
	wxStaticBoxSizer* staticboxsizer = dynamic_cast<wxStaticBoxSizer*>(wxobject);
	wxStaticBox* staticBox = staticboxsizer->GetStaticBox();
	wxASSERT(staticBox);
	if (staticboxsizer != nullptr) {
		staticBox->SetLabel(m_propertyCaption->GetValueAsTranslateString());
		staticBox->SetMinSize(m_propertyMinSize->GetValueAsSize());

		staticBox->SetFont(m_propertyFont->GetValueAsFont());
		staticBox->SetForegroundColour(m_propertyFG->GetValueAsColour());
		staticBox->SetBackgroundColour(m_propertyBG->GetValueAsColour());
		staticBox->Enable(m_propertyEnabled->GetValueAsBoolean());
		staticBox->Show(m_propertyVisible->GetValueAsBoolean());
		staticBox->SetToolTip(m_propertyTooltip->GetValueAsString());

		staticboxsizer->SetOrientation(m_propertyOrient->GetValueAsInteger());
		staticboxsizer->SetMinSize(m_propertyMinSize->GetValueAsSize());

		//after lay out 
		if (m_propertyMinSize->GetValueAsSize() != wxDefaultSize) {
			staticBox->Layout();
		}
	}

	UpdateSizer(staticboxsizer);
}

void CValueStaticBoxSizer::Cleanup(wxObject* wxobject, IVisualHost* visualHost)
{
	wxStaticBoxSizer* staticboxsizer = dynamic_cast<wxStaticBoxSizer*>(wxobject);
	wxStaticBox* staticBox = staticboxsizer->GetStaticBox();
	wxASSERT(staticBox);
	if (visualHost->IsDesignerHost()) {
		staticBox->PopEventHandler(true);
	}
}

//**********************************************************************************
//*                                    Data										   *
//**********************************************************************************



bool CValueStaticBoxSizer::LoadData(CMemoryReader& reader)
{
	m_propertyOrient->SetValue(reader.r_u16());	
	wxString propValue = wxEmptyString;
	reader.r_stringZ(propValue);
	m_propertyCaption->SetValue(propValue);
	reader.r_stringZ(propValue);
	m_propertyFont->SetValue(typeConv::StringToFont(propValue));
	reader.r_stringZ(propValue);
	m_propertyFG->SetValue(typeConv::StringToColour(propValue));
	reader.r_stringZ(propValue);
	m_propertyBG->SetValue(typeConv::StringToColour(propValue));

	reader.r_stringZ(propValue);
	m_propertyTooltip->SetValue(propValue);
	reader.r_stringZ(propValue);
	m_propertyContextHelp->SetValue(propValue);

	m_propertyContextMenu->SetValue(reader.r_u8());
	m_propertyEnabled->SetValue(reader.r_u8());
	m_propertyVisible->SetValue(reader.r_u8());

	return IValueSizer::LoadData(reader);
}

bool CValueStaticBoxSizer::SaveData(CMemoryWriter& writer)
{
	writer.w_u16(m_propertyOrient->GetValueAsInteger());
	writer.w_stringZ(m_propertyCaption->GetValueAsString());
	writer.w_stringZ(m_propertyFont->GetValueAsString());
	writer.w_stringZ(m_propertyFG->GetValueAsString());
	writer.w_stringZ(m_propertyBG->GetValueAsString());
	writer.w_stringZ(m_propertyTooltip->GetValueAsString());
	writer.w_stringZ(m_propertyContextHelp->GetValueAsString());
	writer.w_u8(m_propertyContextMenu->GetValueAsBoolean());
	writer.w_u8(m_propertyEnabled->GetValueAsBoolean());
	writer.w_u8(m_propertyVisible->GetValueAsBoolean());

	return IValueSizer::SaveData(writer);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

CONTROL_TYPE_REGISTER(CValueStaticBoxSizer, "staticboxsizer", "sizer", string_to_clsid("CT_SSZER"));