#include "toolbar.h"
#include "form.h"

#include "backend/system/value/valueEvent.h"

bool CValueToolBarItem::GetToolAction(CEventAction* evtList)
{
	CValueToolbar* toolbar = dynamic_cast<CValueToolbar*> (m_parent);
	if (toolbar == nullptr) return false;
	IValueFrame* sourceElement = toolbar->GetActionSrc() != wxNOT_FOUND ? FindControlByID(toolbar->GetActionSrc()) : nullptr;
	if (sourceElement != nullptr) {
		const CActionCollection& data = sourceElement->GetActionCollection(sourceElement->GetTypeForm());
		for (unsigned int i = 0; i < data.GetCount(); i++) {
			const action_identifier_t& id = data.GetID(i);
			if (id == wxNOT_FOUND) continue; 
			evtList->AppendItem(
				data.GetNameByID(id), 
				data.GetCaptionByID(id), 
				id, 
				CValue::CreateObjectValue<CValueActionEvent>(data.GetNameByID(id), id)
			);
		}
	}

	return true;
}