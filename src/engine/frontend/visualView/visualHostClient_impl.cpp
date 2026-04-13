////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxFormBuilder
//	Description : visual view events 
////////////////////////////////////////////////////////////////////////////

#include "visualHostClient.h"

void CVisualClientHost::ShowForm()
{
	m_valueForm->ShowForm(nullptr); //has no parent
}

void CVisualClientHost::ActivateForm()
{
	m_valueForm->ActivateForm();
}

void CVisualClientHost::UpdateForm()
{
	m_valueForm->UpdateForm();
}

bool CVisualClientHost::CloseForm()
{
	return m_valueForm->CloseForm();
}
