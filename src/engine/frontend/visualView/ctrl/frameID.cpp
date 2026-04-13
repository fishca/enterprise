////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : base control
////////////////////////////////////////////////////////////////////////////

#include "control.h"
#include "form.h"


ibValueFrame* ibValueFrame::DoFindControlByName(const wxString& controlName, ibValueFrame* top) const
{
	for (unsigned int idx = 0; idx < top->GetChildCount(); idx++) {
		ibValueFrame* child = top->GetChild(idx);
		wxASSERT(child);
		ibValueFrame* foundedMeta = DoFindControlByName(controlName, child);
		if (foundedMeta) {
			return foundedMeta;
		}
	}

	if (stringUtils::CompareString(controlName, top->GetControlName())) {
		return top;
	}

	return nullptr;
}

ibValueFrame* ibValueFrame::FindControlByName(const wxString& controlName) const
{
	return DoFindControlByName(controlName, GetOwnerForm());
}

void ibValueFrame::DoGenerateNewID(ibFormID& id, ibValueFrame* top) const
{
	for (unsigned int idx = 0; idx < top->GetChildCount(); idx++) {
		ibValueFrame* child = top->GetChild(idx);
		wxASSERT(child);
		ibFormID newID = child->GetControlID() + 1;
		if (newID > id) {
			id = newID;
		}
		DoGenerateNewID(id, child);
	}
}

ibFormID ibValueFrame::GenerateNewID()
{
	wxASSERT(m_controlId == 0);
	ibValueForm* ownerForm = GetOwnerForm();
	wxASSERT(ownerForm);
	ibFormID id = ownerForm->GetControlID() + 1; // 1 is valueForm  
	DoGenerateNewID(id, ownerForm);
	m_controlId = id;
	return id;
}

ibValueFrame* ibValueFrame::DoFindControlByID(const ibFormID& id, ibValueFrame* top) const
{
	for (unsigned int idx = 0; idx < top->GetChildCount(); idx++) {
		ibValueFrame* child = top->GetChild(idx);
		wxASSERT(child);
		ibValueFrame* foundedMeta = DoFindControlByID(id, child);
		if (foundedMeta) {
			return foundedMeta;
		}
	}

	if (id == top->GetControlID()) {
		return top;
	}

	return nullptr;
}

ibValueFrame* ibValueFrame::FindControlByID(const ibFormID& id) const
{
	return DoFindControlByID(id, GetOwnerForm());
}