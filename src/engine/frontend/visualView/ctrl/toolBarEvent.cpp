#include "toolbar.h"
#include "backend/metaCollection/partial/commonObject.h"
#include "frontend/visualView/visualHostClient.h"

//**********************************************************************************
//*                              Events                                            *
//**********************************************************************************

void ibValueToolbar::OnToolBarLeftDown(wxMouseEvent& event)
{
	ibAuiToolBar* toolBar = wxDynamicCast(
		GetWxObject(), ibAuiToolBar
	);
	wxASSERT(toolBar);
	if (g_visualHostContext != nullptr) {
		const ibFormVisualDocument* visualDoc = ibValueToolbar::GetVisualDocument();
		if (visualDoc == nullptr || (visualDoc != nullptr && !visualDoc->IsVisualDemonstrationDoc())) {
			wxAuiToolBarItem* foundedItem = toolBar->FindToolByPosition(event.GetX(), event.GetY());
			if (foundedItem == nullptr) g_visualHostContext->SelectControl(this);
		}
	}

	event.Skip();
}

#include "backend/system/systemManager.h"

void ibValueToolbar::OnTool(wxCommandEvent& event)
{
	const ibFormVisualDocument* visualDoc = ibValueToolbar::GetVisualDocument();
	if (visualDoc == nullptr || (visualDoc != nullptr && !visualDoc->IsVisualDemonstrationDoc())) {

		ibValueFrame* parentToolControl = FindControlByID(event.GetId());
		if (parentToolControl != nullptr) {

			ibAuiToolBar* toolBar = wxDynamicCast(GetWxObject(), ibAuiToolBar);
			wxASSERT(toolBar);

			if (toolBar != nullptr)
				toolBar->SetFocus();

			ibValueToolBarItem* foundedToolControl = dynamic_cast<ibValueToolBarItem*>(parentToolControl);
			if (foundedToolControl != nullptr) {

				const ibActionDescription& actionDesc = foundedToolControl->GetAction();
				const wxString& strAction = actionDesc.GetCustomAction();

				ibValueFrame* sourceElement = GetActionSrc() != wxNOT_FOUND ? FindControlByID(GetActionSrc()) : nullptr;
				if (sourceElement != nullptr && actionDesc.GetSystemAction() != wxNOT_FOUND) {
					try {
						sourceElement->ExecuteAction(
							actionDesc.GetSystemAction(),
							GetOwnerForm()
						);
					}
					catch (const ibBackendAccessException* err) {
						ibValueSystemFunction::Alert(err->GetErrorDescription());
					}
					catch (const ibBackendException*) {
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

void ibValueToolbar::OnToolDropDown(wxAuiToolBarEvent& event)
{
	ibAuiToolBar* toolBar = wxDynamicCast(
		GetWxObject(), ibAuiToolBar
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

void ibValueToolbar::OnRightDown(wxMouseEvent& event)
{
	event.Skip();
}