////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : base control
////////////////////////////////////////////////////////////////////////////

#include "control.h"
#include "form.h"

wxIMPLEMENT_ABSTRACT_CLASS(ibValueControl, ibValueFrame)

//*************************************************************************
//*                          ValueControl		                          *
//*************************************************************************

ibValueControl::ibValueControl()
	: ibValueFrame(), m_formOwner(nullptr)
{
}

ibValueControl::~ibValueControl()
{
	SetOwnerForm(nullptr);
}

#include "backend/metaData.h"

void ibValueControl::SetOwnerForm(ibValueForm* ownerForm)
{
	if (ownerForm && m_formOwner == nullptr) {
		if (GetComponentType() != COMPONENT_TYPE_SIZERITEM)
			ownerForm->m_listControl.emplace(this);
	}
	else if (!ownerForm && m_formOwner != nullptr) {
		m_formOwner->m_listControl.erase(this);
	}
	m_formOwner = ownerForm;
}

ibMetaData* ibValueControl::GetMetaData() const
{
	const ibValueMetaObjectFormBase* metaFormObject = m_formOwner ?
		m_formOwner->GetFormMetaObject() : nullptr;

	//for form buider
	if (metaFormObject == nullptr) {
		ibSourceDataObject* srcValue = m_formOwner ?
			m_formOwner->GetSourceObject() :
			nullptr;
		if (srcValue != nullptr) {
			ibValueMetaObjectGenericData* metaValue = srcValue->GetSourceMetaObject();
			wxASSERT(metaValue);
			return metaValue->GetMetaData();
		}
	}

	return metaFormObject ?
		metaFormObject->GetMetaData() :
		nullptr;
}

#include "backend/metaCollection/metaFormObject.h"

ibFormID ibValueControl::GetTypeForm() const
{
	if (m_formOwner == nullptr) {
		wxASSERT(m_formOwner);
		return 0;
	}

	const ibValueMetaObjectFormBase* creator = m_formOwner->GetFormMetaObject();
 	if (creator != nullptr) 
		return creator->GetTypeForm();
	return m_formOwner->GetTypeForm();
}

ibProcUnit* ibValueControl::GetFormProcUnit() const
{
	if (!m_formOwner) {
		wxASSERT(m_formOwner);
		return nullptr;
	}

	// .get() on the shared_ptr — raw pointer out is acceptable here
	// because form event handlers run on the same worker thread that
	// owns the session, so a concurrent session teardown can't race
	// mid-callback (unlike BeforeStart which is re-entered from the
	// registry thread via Init/ExitRuntimeForSession).
	return m_formOwner->GetProcUnit().get();
}