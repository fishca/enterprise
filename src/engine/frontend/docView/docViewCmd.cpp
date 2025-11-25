////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko 
//	Description : document/view implementation for metaObject 
////////////////////////////////////////////////////////////////////////////

#include "docView.h"
#include "frontend/mainFrame/mainFrame.h"
#include "backend/moduleManager/moduleManager.h"

void CMetaView::OnActivateView(bool activate, wxView* activeView, wxView* deactiveView)
{
	if (activate)
		objectInspector->SelectObject(GetDocument() ? GetDocument()->GetMetaObject() : nullptr);
}

void CMetaView::Activate(bool activate)
{
	wxDocManager* docManager = m_viewDocument != nullptr ?
		m_viewDocument->GetDocumentManager() : wxDocManager::GetDocumentManager();

	if (docManager != nullptr && CDocMDIFrame::GetFrame()) {
		mainFrame->ActivateView(this, activate);
		OnActivateView(activate, this, docManager->GetCurrentView());
		docManager->ActivateView(this, activate);
	}

	if (activate) wxLogDebug("! <debug> activate view %s", CMetaView::GetViewName());
	else wxLogDebug("! <debug> deactivate view %s", CMetaView::GetViewName());
}