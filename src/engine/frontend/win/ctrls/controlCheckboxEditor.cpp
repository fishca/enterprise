#include "controlCheckboxEditor.h"

wxBEGIN_EVENT_TABLE(wxControlCheckbox, wxWindow)
EVT_SIZE(wxControlCheckbox::OnSize)
EVT_DPI_CHANGED(wxControlCheckbox::OnDPIChanged)
wxEND_EVENT_TABLE()

wxIMPLEMENT_DYNAMIC_CLASS(wxControlCheckbox, wxWindow);

// ----------------------------------------------------------------------------
// geometry
// ----------------------------------------------------------------------------

wxSize wxControlCheckbox::DoGetBestClientSize() const
{
	wxSize size = m_checkBox->GetBestSize();
	wxSize labelSize = m_label->GetBestSize();

	if (size.y < labelSize.y)
		size.y = labelSize.y;

	size.x += labelSize.x;
	return size;
}

wxWindowList wxControlCheckbox::GetCompositeWindowParts() const
{
	wxWindowList parts;
	parts.push_back(m_label);
	parts.push_back(m_checkBox);
	return parts;
}

// ----------------------------------------------------------------------------
// label
// ----------------------------------------------------------------------------

void wxControlCheckbox::CalculateLabelSize(wxCoord* w, wxCoord* h) const
{
	m_label->SetMinSize(wxDefaultSize);
	m_label->GetBestSize(w, h);
}

void wxControlCheckbox::ApplyLabelSize(const wxSize& s)
{
	m_label->SetMinSize(s);
}
