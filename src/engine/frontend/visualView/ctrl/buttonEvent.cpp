#include "widgets.h"
#include "form.h"

//*******************************************************************
//*                             Events                              *
//*******************************************************************
void ibValueButton::OnButtonPressed(wxCommandEvent &event)
{
	event.Skip(
		CallAsEvent(m_onButtonPressed, ibValueButton::GetValue())
	);

	if (m_formOwner != nullptr)
		m_formOwner->RefreshForm();
}
