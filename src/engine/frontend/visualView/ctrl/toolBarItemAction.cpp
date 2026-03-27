#include "toolbar.h"
#include "form.h"

#include "backend/system/value/valueEvent.h"

bool ibValueToolBarItem::GetToolAction(CEventAction* evtList)
{
	ibValueToolbar* toolbar = dynamic_cast<ibValueToolbar*> (m_parent);
	if (toolbar == nullptr) return false;
	ibValueFrame* sourceElement = toolbar->GetActionSrc() != wxNOT_FOUND ? FindControlByID(toolbar->GetActionSrc()) : nullptr;
	if (sourceElement != nullptr) {
		const ibActionCollection& data = sourceElement->GetActionCollection(sourceElement->GetTypeForm());
		for (unsigned int i = 0; i < data.GetCount(); i++) {
			const ibActionID& id = data.GetID(i);
			if (id == wxNOT_FOUND) continue;
			const ibPictureDescription& pictureDesc = data.GetPictureByID(id);
			evtList->AppendItem(
				data.GetNameByID(id),
				data.GetCaptionByID(id),
				id,
				pictureDesc.IsEmptyPicture() ? wxNullBitmap : ibBackendPicture::CreatePicture(pictureDesc, GetMetaData()),
				ibValue::CreateObjectValue<ibValueActionEvent>(data.GetNameByID(id), id)
			);
		}
	}

	return true;
}