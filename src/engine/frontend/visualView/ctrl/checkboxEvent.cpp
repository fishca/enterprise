#include "widgets.h"
#include "form.h"
#include "backend/metaCollection/partial/commonObject.h"
#include "backend/metaData.h"

//*******************************************************************
//*                             Events                              *
//*******************************************************************

void CValueCheckbox::OnClickedCheckbox(wxCommandEvent& event)
{
	wxCheckBox* checkbox =
		wxDynamicCast(event.GetEventObject(), wxCheckBox);

	m_selValue = checkbox->GetValue();

	if (!m_propertySource->IsEmptyProperty()) {
		ISourceDataObject* srcData = m_formOwner->GetSourceObject();
		wxASSERT(srcData);
		srcData->SetValueByMetaID(m_propertySource->GetValueAsSource(), m_selValue);
	}

	m_formOwner->RefreshForm();

	event.Skip(
		CallAsEvent(m_onCheckboxClicked)
	);
}