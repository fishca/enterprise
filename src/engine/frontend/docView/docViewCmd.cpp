////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko 
//	Description : document/view implementation for metaObject 
////////////////////////////////////////////////////////////////////////////

#include "docView.h"
#include "frontend/mainFrame/mainFrame.h"

void CMetaView::Activate(bool activate)
{
	wxDocManager* docManager = m_viewDocument != nullptr ?
		m_viewDocument->GetDocumentManager() : nullptr;

	if (docManager != nullptr && CDocMDIFrame::GetFrame()) {
		mainFrame->ActivateView(this, activate);
		OnActivateView(activate, this, docManager->GetCurrentView());
		docManager->ActivateView(this, activate);
	}
}