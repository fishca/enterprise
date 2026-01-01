#include "metaObjectMetadata.h"

void CMetaObjectConfiguration::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (m_propertyDefLanguage == property)
		CBackendLocalization::SetUserLanguage(GetLangCode());

	IMetaObject::OnPropertyChanged(property, oldValue, newValue);
}