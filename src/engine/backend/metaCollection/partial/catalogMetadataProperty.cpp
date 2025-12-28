#include "catalog.h"
#include "backend/metadata.h"
#include "backend/objCtor.h"

void CMetaObjectCatalog::OnPropertyCreated(IProperty* property)
{
	IMetaObjectRecordDataMutableRef::OnPropertyCreated(property);
}

bool CMetaObjectCatalog::OnPropertyChanging(IProperty* property, const wxVariant& newValue)
{
	return IMetaObjectRecordDataMutableRef::OnPropertyChanging(property, newValue);
}

void CMetaObjectCatalog::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (m_propertyOwner == property) {
		const CMetaDescription& metaDesc = m_propertyOwner->GetValueAsMetaDesc(); CTypeDescription typeDesc;
		for (unsigned int idx = 0; idx < metaDesc.GetTypeCount(); idx++) {
			const IMetaObject* catalog = m_metaData->FindAnyObjectByFilter(metaDesc.GetByIdx(idx));
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
	
	IMetaObjectRecordDataMutableRef::OnPropertyChanged(property, oldValue, newValue);
}