#include "widgets.h"
#include "form.h"

//*******************************************************************
//*                             Events                              *
//*******************************************************************
void CValueButton::OnButtonPressed(wxCommandEvent &event)
{
	event.Skip(
		CallAsEvent(m_onButtonPressed, CValueButton::GetValue())
	);

	m_formOwner->RefreshForm();
}
