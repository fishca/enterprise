#include "controlCheckboxEditor.h"

wxBEGIN_EVENT_TABLE(ibControlCheckbox, wxWindow)
EVT_SIZE(ibControlCheckbox::OnSize)
EVT_DPI_CHANGED(ibControlCheckbox::OnDPIChanged)
wxEND_EVENT_TABLE()

wxIMPLEMENT_DYNAMIC_CLASS(ibControlCheckbox, wxWindow);

// ----------------------------------------------------------------------------
// geometry
// ----------------------------------------------------------------------------

wxSize ibControlCheckbox::DoGetBestClientSize() const
{
	wxSize size = m_checkBox->GetBestSize();
	wxSize labelSize = m_label->GetBestSize();

	if (size.y < labelSize.y)
		size.y = labelSize.y;

	size.x += labelSize.x;
	return size;
}

wxWindowList ibControlCheckbox::GetCompositeWindowParts() const
{
	wxWindowList parts;
	parts.push_back(m_label);
	parts.push_back(m_checkBox);
	return parts;
}

// ----------------------------------------------------------------------------
// label
// ----------------------------------------------------------------------------

void ibControlCheckbox::CalculateLabelSize(wxCoord* w, wxCoord* h) const
{
	m_label->SetMinSize(wxDefaultSize);
	m_label->GetBestSize(w, h);
}

void ibControlCheckbox::ApplyLabelSize(const wxSize& s)
{
	m_label->SetMinSize(s);
}
