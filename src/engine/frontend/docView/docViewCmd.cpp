////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko 
//	Description : document/view implementation for metaObject 
////////////////////////////////////////////////////////////////////////////

#include "docView.h"
#include "frontend/mainFrame/mainFrame.h"
void CMetaView::OnActivateView(bool activate, wxView* activeView, wxView* deactiveView)
{
	mainFrame->OnActivateView(activate, activeView, deactiveView);
}

#define menuPos 2

void CMetaView::Activate(bool activate)
{
	wxDocManager* docManager = m_viewDocument != nullptr ?
		m_viewDocument->GetDocumentManager() : nullptr;

#if wxUSE_MENUS		
	wxFrame* viewFrame = dynamic_cast<wxFrame*>(m_viewFrame);
	if (viewFrame != nullptr) viewFrame->SetMenuBar(activate ? CreateMenuBar() : nullptr);
#endif

	if (docManager != nullptr && CDocMDIFrame::GetFrame()) {
		OnActivateView(activate, this, docManager->GetCurrentView());
		docManager->ActivateView(this, activate);
	}
}

void CMetaView::OnViewMenuClicked(wxCommandEvent& event) {
	OnMenuItemClicked(event.GetId());
	event.Skip();
}