#include "toolBar.h"
#include "frontend/visualView/visualHostClient.h"

void ibValueToolbar::OnPropertyCreated(ibProperty* property)
{
	ibValueWindow::OnPropertyCreated(property);
}

void ibValueToolbar::OnPropertySelected(ibProperty* property)
{
	ibValueWindow::OnPropertySelected(property);
}

void ibValueToolbar::OnPropertyChanged(ibProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (m_actSource == property) {

		const int answer = wxMessageBox(
			_("The data source has been changed. Refill Tools?"),
			_("Toolbar"), wxYES_NO
		);

		if (answer == wxYES) {

			while (GetChildCount() != 0) {
				g_visualHostContext->CutControl(GetChild(0), true);
			}

			const ibActionCollection& actionData = GetActionArray();
			for (unsigned int i = 0; i < actionData.GetCount(); i++) {
				const ibActionID& id = actionData.GetID(i);
				if (id != wxNOT_FOUND) {
					ibValueToolBarItem* toolItem = dynamic_cast<ibValueToolBarItem*>(
						m_formOwner->CreateControl(wxT("tool"), this)
						);
					wxASSERT(toolItem);
					toolItem->SetControlName(GetControlName() + actionData.GetNameByID(id));
					//toolItem->SetCaption(actionData.GetCaptionByID(id));
					//toolItem->SetToolTip(actionData.GetCaptionByID(id));
					toolItem->SetAction(id);
					g_visualHostContext->InsertControl(toolItem, this);
				}
				else {
					ibValueToolBarSeparator* toolItemSeparator = dynamic_cast<ibValueToolBarSeparator*>(
						m_formOwner->CreateControl(wxT("toolSeparator"), this)
						);
					g_visualHostContext->InsertControl(toolItemSeparator, this);
				}
			}

			if (GetChildCount() == 0) {
				ibValueToolbar::AddToolItem();
			}

			g_visualHostContext->RefreshEditor();
		}
	}

	ibValueWindow::OnPropertyChanged(property, oldValue, newValue);
}