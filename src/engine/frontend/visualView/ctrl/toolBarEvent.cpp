#include "toolbar.h"
#include "backend/metaCollection/partial/commonObject.h"

//**********************************************************************************
//*                              Events                                            *
//**********************************************************************************

void CValueToolbar::OnToolBarLeftDown(wxMouseEvent& event)
{
	CAuiToolBar* toolBar = wxDynamicCast(
		GetWxObject(), CAuiToolBar
	);
	wxASSERT(toolBar);
	if (g_visualHostContext != nullptr) {
		wxAuiToolBarItem* foundedItem = toolBar->FindToolByPosition(event.GetX(), event.GetY());
		if (foundedItem == nullptr) g_visualHostContext->SelectControl(this);
	}

	event.Skip();
}

#include "backend/appData.h"
#include "frontend/visualView/visualHost.h"

void CValueToolbar::OnTool(wxCommandEvent& event)
{
	CVisualDocument* visualDoc = CValueToolbar::GetVisualDocument();
	if (visualDoc != nullptr) {
		IVisualHost* visualHost = visualDoc->GetVisualView();
		bool isDemonstation = visualHost ? visualHost->IsDemonstration() : false;
		if (isDemonstation) {
			event.Skip();
			return;
		}
	}
	IValueFrame* foundedControl = FindControlByID(event.GetId());
	if (foundedControl != nullptr) {
		CAuiToolBar* toolBar = wxDynamicCast(GetWxObject(), CAuiToolBar);
		wxASSERT(toolBar);
		if (toolBar != nullptr)
			toolBar->SetFocus();
		CValueToolBarItem* toolItem = dynamic_cast<CValueToolBarItem*>(foundedControl);
		if (toolItem != nullptr) {
			action_identifier_t actionId = wxNOT_FOUND;
			IValueFrame* sourceElement = GetActionSrc() != wxNOT_FOUND ? FindControlByID(GetActionSrc()) : nullptr;
			const wxString& actionStr = toolItem->GetAction();
			if (sourceElement != nullptr && actionStr.ToInt(&actionId)) {
				try {
					sourceElement->ExecuteAction(actionId, toolItem->GetOwnerForm());
				}
				catch (const CBackendException* err) {
					wxMessageBox(err->what(), m_formOwner->GetCaption());
				}
			}
			else if (actionStr.Length() > 0) {
				CallAsEvent(actionStr, toolItem->GetValue());
			}
		}

		if (g_visualHostContext != nullptr) {
			g_visualHostContext->SelectControl(foundedControl);
		}
	}

	event.Skip();
}

void CValueToolbar::OnToolDropDown(wxAuiToolBarEvent& event)
{
	CAuiToolBar* toolBar = wxDynamicCast(
		GetWxObject(), CAuiToolBar
	);

	if (event.IsDropDownClicked()) {

		wxPoint pos;
		wxRect itemRect = event.GetItemRect();
		pos.y = itemRect.height + 5;

		for (size_t idx = 0; idx < toolBar->GetToolCount(); idx++) {
			wxAuiToolBarItem* item = toolBar->FindToolByIndex(idx);
			wxASSERT(item);
			int toolid = item->GetId();
			if (toolid == event.GetId())
				break;
			wxRect rect = toolBar->GetToolRect(item->GetId());
			pos.x += rect.width;
		}

		wxMenu menuPopup;
		menuPopup.Append(wxID_OPEN, _("Test"));
		if (toolBar->PopupMenu(&menuPopup, pos)) {
			event.Skip();
		}
		else {
			event.Veto();
		}
	}
	else {
		event.Skip();
	}
}

void CValueToolbar::OnRightDown(wxMouseEvent& event)
{
	event.Skip();
}