#include "checkBoxEditor.h"

wxBEGIN_EVENT_TABLE(wxControlCheckboxCtrl, wxWindow)
EVT_SIZE(wxControlCheckboxCtrl::OnSize)
EVT_DPI_CHANGED(wxControlCheckboxCtrl::OnDPIChanged)
wxEND_EVENT_TABLE()

wxIMPLEMENT_DYNAMIC_CLASS(wxControlCheckboxCtrl, wxWindow);

// ----------------------------------------------------------------------------
// geometry
// ----------------------------------------------------------------------------

wxSize wxControlCheckboxCtrl::DoGetBestClientSize() const
{
	wxSize size = m_checkBox->GetBestSize();
	wxSize labelSize = m_label->GetBestSize();

	if (size.y < labelSize.y)
		size.y = labelSize.y;

	size.x += labelSize.x;
	return size;
}

wxWindowList wxControlCheckboxCtrl::GetCompositeWindowParts() const
{
	wxWindowList parts;
	parts.push_back(m_label);
	parts.push_back(m_checkBox);
	return parts;
}