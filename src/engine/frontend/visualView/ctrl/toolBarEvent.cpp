#include "toolbar.h"
#include "backend/metaCollection/partial/commonObject.h"
#include "frontend/visualView/visualHost.h"

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
		const CVisualDocument* visualDoc = CValueToolbar::GetVisualDocument();
		if (visualDoc == nullptr || (visualDoc != nullptr && !visualDoc->IsVisualDemonstrationDoc())) {
			wxAuiToolBarItem* foundedItem = toolBar->FindToolByPosition(event.GetX(), event.GetY());
			if (foundedItem == nullptr) g_visualHostContext->SelectControl(this);
		}
	}

	event.Skip();
}

#include "backend/system/systemManager.h"

void CValueToolbar::OnTool(wxCommandEvent& event)
{
	const CVisualDocument* visualDoc = CValueToolbar::GetVisualDocument();
	if (visualDoc == nullptr || (visualDoc != nullptr && !visualDoc->IsVisualDemonstrationDoc())) {

		IValueFrame* parentToolControl = FindControlByID(event.GetId());
		if (parentToolControl != nullptr) {

			CAuiToolBar* toolBar = wxDynamicCast(GetWxObject(), CAuiToolBar);
			wxASSERT(toolBar);

			if (toolBar != nullptr)
				toolBar->SetFocus();

			CValueToolBarItem* foundedToolControl = dynamic_cast<CValueToolBarItem*>(parentToolControl);
			if (foundedToolControl != nullptr) {

				const CActionDescription& actionDesc = foundedToolControl->GetAction();
				const wxString& strAction = actionDesc.GetCustomAction();

				IValueFrame* sourceElement = GetActionSrc() != wxNOT_FOUND ? FindControlByID(GetActionSrc()) : nullptr;
				if (sourceElement != nullptr && actionDesc.GetSystemAction() != wxNOT_FOUND) {
					try {
						sourceElement->ExecuteAction(
							actionDesc.GetSystemAction(),
							GetOwnerForm()
						);
					}
					catch (const CBackendException* err) {
						CSystemFunction::Message(err->what(), eStatusMessage_Error);
					}
				}
				else if (strAction.Length() > 0) {
					CallAsEvent(strAction, parentToolControl->GetValue());
				}

			}

			if (g_visualHostContext != nullptr) {
				g_visualHostContext->SelectControl(parentToolControl);
			}
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