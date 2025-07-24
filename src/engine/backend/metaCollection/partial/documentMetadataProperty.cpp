////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : meta-attribues
////////////////////////////////////////////////////////////////////////////

#include "document.h"
#include "backend/metadata.h"
#include "backend/objCtor.h"

void CMetaObjectDocument::OnPropertyCreated(IProperty* property)
{
    IMetaObjectRecordDataMutableRef::OnPropertyCreated(property);
}

bool CMetaObjectDocument::OnPropertyChanging(IProperty* property, const wxVariant& newValue)
{
    return IMetaObjectRecordDataMutableRef::OnPropertyChanging(property, newValue);
}

void CMetaObjectDocument::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
    if (m_propertyRegisterRecord == property) {
        const CMetaDescription& old_metaDesc = m_propertyRegisterRecord->GetValueAsMetaDesc(oldValue);
        const CMetaDescription& new_metaDesc = m_propertyRegisterRecord->GetValueAsMetaDesc(newValue);
        for (unsigned int idx = 0; idx < old_metaDesc.GetTypeCount(); idx++) {
            if (new_metaDesc.ContainMetaType(old_metaDesc.GetByIdx(idx))) continue;
            IMetaObjectRegisterData* registerData = nullptr;
            if (m_metaData->GetMetaObject(registerData, old_metaDesc.GetByIdx(idx))) {
                CMetaObjectAttributeDefault* infoRecorder = registerData->GetRegisterRecorder();
                wxASSERT(infoRecorder);
                infoRecorder->GetTypeDesc().ClearMetaType((*m_propertyAttributeReference)->GetTypeDesc());
            }
        }
        for (unsigned int idx = 0; idx < new_metaDesc.GetTypeCount(); idx++) {
            if (old_metaDesc.ContainMetaType(new_metaDesc.GetByIdx(idx))) continue;
            IMetaObjectRegisterData* registerData = nullptr;
            if (m_metaData->GetMetaObject(registerData, new_metaDesc.GetByIdx(idx))) {
                CMetaObjectAttributeDefault* infoRecorder = registerData->GetRegisterRecorder();
                wxASSERT(infoRecorder);
                infoRecorder->GetTypeDesc().AppendMetaType((*m_propertyAttributeReference)->GetTypeDesc());
            }
        }
    }

    if (CMetaObjectDocument::OnReloadMetaObject()) IMetaObject::OnPropertyChanged(property, oldValue, newValue);
}

void CMetaObjectDocument::OnPropertyPasted(IProperty* property)
{
    if (m_propertyRegisterRecord == property) {
        const CMetaDescription& metaDesc = m_propertyRegisterRecord->GetValueAsMetaDesc(property->GetValue());
        for (unsigned int idx = 0; idx < metaDesc.GetTypeCount(); idx++) {
            IMetaObjectRegisterData* registerData = nullptr;
            if (m_metaData->GetMetaObject(registerData, metaDesc.GetByIdx(idx))) {
                CMetaObjectAttributeDefault* infoRecorder = registerData->GetRegisterRecorder();
                wxASSERT(infoRecorder);
                infoRecorder->GetTypeDesc().AppendMetaType((*m_propertyAttributeReference)->GetTypeDesc());
            }
        }
    }

    IMetaObjectRecordDataMutableRef::OnPropertyPasted(property);
}
