////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : base control
////////////////////////////////////////////////////////////////////////////

#include "control.h"
#include "form.h"

wxIMPLEMENT_ABSTRACT_CLASS(IValueControl, IValueFrame)

//*************************************************************************
//*                          ValueControl		                          *
//*************************************************************************

IValueControl::IValueControl()
	: IValueFrame(), m_formOwner(nullptr)
{
}

IValueControl::~IValueControl()
{
	SetOwnerForm(nullptr);
}

#include "backend/metaData.h"

void IValueControl::SetOwnerForm(CValueForm* ownerForm)
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

IMetaData* IValueControl::GetMetaData() const
{
	const IValueMetaObjectForm* metaFormObject = m_formOwner ?
		m_formOwner->GetFormMetaObject() : nullptr;

	//for form buider
	if (metaFormObject == nullptr) {
		ISourceDataObject* srcValue = m_formOwner ?
			m_formOwner->GetSourceObject() :
			nullptr;
		if (srcValue != nullptr) {
			IValueMetaObjectGenericData* metaValue = srcValue->GetSourceMetaObject();
			wxASSERT(metaValue);
			return metaValue->GetMetaData();
		}
	}

	return metaFormObject ?
		metaFormObject->GetMetaData() :
		nullptr;
}

#include "backend/metaCollection/metaFormObject.h"

form_identifier_t IValueControl::GetTypeForm() const
{
	if (m_formOwner == nullptr) {
		wxASSERT(m_formOwner);
		return 0;
	}

	const IValueMetaObjectForm* creator = m_formOwner->GetFormMetaObject();
 	if (creator != nullptr) 
		return creator->GetTypeForm();
	return m_formOwner->GetTypeForm();
}

CProcUnit* IValueControl::GetFormProcUnit() const
{
	if (!m_formOwner) {
		wxASSERT(m_formOwner);
		return nullptr;
	}

	return m_formOwner->GetProcUnit();
}