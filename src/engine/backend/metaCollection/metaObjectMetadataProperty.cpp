#include "metaObjectMetadata.h"

void ibValueMetaObjectConfiguration::OnPropertyChanged(ibProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (m_propertyDefLanguage == property)
		ibBackendLocalization::SetUserLanguage(GetLangCode());

	ibValueMetaObject::OnPropertyChanged(property, oldValue, newValue);
}