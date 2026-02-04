#include "catalog.h"
#include "backend/metadata.h"
#include "backend/objCtor.h"

void CValueMetaObjectCatalog::OnPropertyCreated(IProperty* property)
{
	IValueMetaObjectRecordDataMutableRef::OnPropertyCreated(property);
}

bool CValueMetaObjectCatalog::OnPropertyChanging(IProperty* property, const wxVariant& newValue)
{
	return IValueMetaObjectRecordDataMutableRef::OnPropertyChanging(property, newValue);
}

void CValueMetaObjectCatalog::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (m_propertyOwner == property) {
		const CMetaDescription& metaDesc = m_propertyOwner->GetValueAsMetaDesc(); CTypeDescription typeDesc;
		for (unsigned int idx = 0; idx < metaDesc.GetTypeCount(); idx++) {
			const IValueMetaObject* catalog = m_metaData->FindAnyObjectByFilter(metaDesc.GetByIdx(idx));
			if (catalog != nullptr) {
				const IMetaValueTypeCtor* so = m_metaData->GetTypeCtor(catalog, eCtorMetaType::eCtorMetaType_Reference);
				wxASSERT(so);
				typeDesc.AppendMetaType(so->GetClassType());
			}
		}
		(*m_propertyAttributeOwner)->SetDefaultMetaType(typeDesc);
	}

	if ((*m_propertyAttributeOwner)->GetClsidCount() > 0) (*m_propertyAttributeOwner)->ClearFlag(metaDisableFlag);
	else (*m_propertyAttributeOwner)->SetFlag(metaDisableFlag);
	
	IValueMetaObjectRecordDataMutableRef::OnPropertyChanged(property, oldValue, newValue);
}