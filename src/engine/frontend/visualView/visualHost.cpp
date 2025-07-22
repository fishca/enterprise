////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxFormBuilder
//	Description : visual host  
////////////////////////////////////////////////////////////////////////////

#include "visualHost.h"
#include <wx/collpane.h>

CVisualHost::CVisualHost(CMetaDocument* document, CValueForm* valueForm, wxWindow* parent) : IVisualHost(parent, wxID_ANY, wxDefaultPosition, parent->GetSize()),
	m_document(document), m_valueForm(valueForm), m_dataViewSizeChanged(false) {
	
	CVisualHost::Bind(wxEVT_SIZE, &CVisualHost::OnSize, this);
	CVisualHost::Bind(wxEVT_IDLE, &CVisualHost::OnIdle, this);
}

CVisualHost::~CVisualHost()
{
	CVisualHost::Unbind(wxEVT_SIZE, &CVisualHost::OnSize, this);
	CVisualHost::Unbind(wxEVT_IDLE, &CVisualHost::OnIdle, this);

	for (unsigned int i = 0; i < m_valueForm->GetChildCount(); i++) {
		IValueFrame* objChild = m_valueForm->GetChild(i);
		DeleteRecursive(objChild, true);
	}
}

/////////////////////////////////////////////////////////////////////////////////

void CVisualHost::OnSize(wxSizeEvent& event)
{
	m_dataViewSizeChanged = (m_dataViewSize != GetSize());
	event.Skip();
}

void CVisualHost::OnIdle(wxIdleEvent& event)
{
	if (m_dataViewSizeChanged)
		m_valueForm->RefreshForm();
	m_dataViewSize = GetSize();
	m_dataViewSizeChanged = false;
	event.Skip();
}

/////////////////////////////////////////////////////////////////////////////////

void CVisualHost::SetCaption(const wxString& strCaption)
{
	if (m_document != nullptr && !IsDemonstration()) {
		m_document->SetFilename(strCaption, true);
		m_document->SetTitle(strCaption);
	}
}

void CVisualHost::SetOrientation(int orient)
{
	if (m_mainBoxSizer == nullptr) {
		m_mainBoxSizer = new wxBoxSizer(orient);
		wxScrolledWindow::SetSizer(m_mainBoxSizer);
	}
	else {
		m_mainBoxSizer->SetOrientation(orient);
	}
}
