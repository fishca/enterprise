
#include "sizer.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueGridSizer, IValueSizer)

//****************************************************************************
//*                             GridSizer                                    *
//****************************************************************************

CValueGridSizer::CValueGridSizer() : IValueSizer()
{
}

wxObject* CValueGridSizer::Create(wxWindow* /*parent*/, IVisualHost* /*visualHost*/)
{
	return new wxGridSizer(m_propertyRows->GetValueAsUInteger(), m_propertyCols->GetValueAsUInteger(), 0, 0);
}

void CValueGridSizer::OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost *visualHost, bool first—reated)
{
}

void CValueGridSizer::Update(wxObject* wxobject, IVisualHost *visualHost)
{
	wxGridSizer *gridsizer = dynamic_cast<wxGridSizer *>(wxobject);

	if (gridsizer != nullptr) {
		gridsizer->SetRows(m_propertyRows->GetValueAsUInteger());
		gridsizer->SetCols(m_propertyCols->GetValueAsUInteger());

		gridsizer->SetMinSize(m_propertyMinSize->GetValueAsSize());
	}

	UpdateSizer(gridsizer);
}

void CValueGridSizer::Cleanup(wxObject* obj, IVisualHost *visualHost)
{
}

//**********************************************************************************
//*                           Property                                             *
//**********************************************************************************

bool CValueGridSizer::LoadData(CMemoryReader &reader)
{
	m_propertyRows->SetValue(reader.r_s32());
	m_propertyCols->SetValue(reader.r_s32());

	return IValueSizer::LoadData(reader);
}

bool CValueGridSizer::SaveData(CMemoryWriter &writer)
{
	writer.w_s32(m_propertyRows->GetValueAsUInteger());
	writer.w_s32(m_propertyCols->GetValueAsUInteger());

	return IValueSizer::SaveData(writer);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

CONTROL_TYPE_REGISTER(CValueGridSizer, "gridsizer", "sizer", string_to_clsid("CT_GSZR"));