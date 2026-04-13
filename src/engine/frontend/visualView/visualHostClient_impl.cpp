////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxFormBuilder
//	Description : visual view events 
////////////////////////////////////////////////////////////////////////////

#include "visualHostClient.h"

void ibVisualHostClient::ShowForm()
{
	m_valueForm->ShowForm(nullptr); //has no parent
}

void ibVisualHostClient::ActivateForm()
{
	m_valueForm->ActivateForm();
}

void ibVisualHostClient::UpdateForm()
{
	m_valueForm->UpdateForm();
}

bool ibVisualHostClient::CloseForm()
{
	return m_valueForm->CloseForm();
}
