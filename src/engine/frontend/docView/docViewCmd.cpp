////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko 
//	Description : document/view implementation for metaObject 
////////////////////////////////////////////////////////////////////////////

#include "docView.h"
#include "frontend/mainFrame/mainFrame.h"
#include "backend/moduleManager/moduleManager.h"

void ibMetaView::OnActivateView(bool activate, wxView* activeView, wxView* deactiveView)
{
	if (activate)
		objectInspector->SelectObject(GetDocument() ? GetDocument()->GetMetaObject() : nullptr);
}

void ibMetaView::Activate(bool activate)
{
	wxDocManager* docManager = m_viewDocument != nullptr ?
		m_viewDocument->GetDocumentManager() : wxDocManager::GetDocumentManager();

	if (docManager != nullptr && ibFrontendDocMDIFrame::GetFrame()) {
		mainFrame->ActivateView(this, activate);
		OnActivateView(activate, this, docManager->GetCurrentView());
		docManager->ActivateView(this, activate);
	}

	if (activate) wxLogDebug("! <debug> activate view %s", ibMetaView::GetViewName());
	else wxLogDebug("! <debug> deactivate view %s", ibMetaView::GetViewName());
}