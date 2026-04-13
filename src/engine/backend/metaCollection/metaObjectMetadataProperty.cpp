#include "metaObjectMetadata.h"

void CValueMetaObjectConfiguration::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (m_propertyDefLanguage == property)
		CBackendLocalization::SetUserLanguage(GetLangCode());

	IValueMetaObject::OnPropertyChanged(property, oldValue, newValue);
}