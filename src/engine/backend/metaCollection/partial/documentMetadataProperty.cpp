////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : meta-attribues
////////////////////////////////////////////////////////////////////////////

#include "document.h"
#include "backend/metadata.h"
#include "backend/objCtor.h"

void CValueMetaObjectDocument::OnPropertyCreated(IProperty* property)
{
	IValueMetaObjectRecordDataMutableRef::OnPropertyCreated(property);
}

bool CValueMetaObjectDocument::OnPropertyChanging(IProperty* property, const wxVariant& newValue)
{
	return IValueMetaObjectRecordDataMutableRef::OnPropertyChanging(property, newValue);
}

void CValueMetaObjectDocument::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (m_propertyRegisterRecord == property) {
		const CMetaDescription& old_metaDesc = m_propertyRegisterRecord->GetValueAsMetaDesc(oldValue);
		const CMetaDescription& new_metaDesc = m_propertyRegisterRecord->GetValueAsMetaDesc(newValue);
		for (unsigned int idx = 0; idx < old_metaDesc.GetTypeCount(); idx++) {
			if (new_metaDesc.ContainMetaType(old_metaDesc.GetByIdx(idx))) continue;
			const IValueMetaObjectRegisterData* registerData = m_metaData->FindAnyObjectByFilter<IValueMetaObjectRegisterData>(old_metaDesc.GetByIdx(idx));
			if (registerData != nullptr) {
				CValueMetaObjectAttributePredefined* infoRecorder = registerData->GetRegisterRecorder();
				wxASSERT(infoRecorder);
				infoRecorder->GetTypeDesc().ClearMetaType((*m_propertyAttributeReference)->GetTypeDesc());
			}
		}
		for (unsigned int idx = 0; idx < new_metaDesc.GetTypeCount(); idx++) {
			if (old_metaDesc.ContainMetaType(new_metaDesc.GetByIdx(idx))) continue;
			IValueMetaObjectRegisterData* registerData = m_metaData->FindAnyObjectByFilter<IValueMetaObjectRegisterData>(new_metaDesc.GetByIdx(idx));
			if (registerData != nullptr) {
				CValueMetaObjectAttributePredefined* infoRecorder = registerData->GetRegisterRecorder();
				wxASSERT(infoRecorder);
				infoRecorder->GetTypeDesc().AppendMetaType((*m_propertyAttributeReference)->GetTypeDesc());
			}
		}
	}

	if (CValueMetaObjectDocument::OnReloadMetaObject()) IValueMetaObject::OnPropertyChanged(property, oldValue, newValue);
}
