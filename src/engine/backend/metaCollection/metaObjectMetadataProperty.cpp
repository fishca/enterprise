#include "metaObjectMetadata.h"
#include "backend/compiler/compileCode.h"

void ibValueMetaObjectConfiguration::OnPropertyChanged(ibProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (m_propertyDefLanguage == property)
		ibBackendLocalization::SetActiveLanguage(GetLangCode());
	
	if (m_propertySyntax == property)
		ibCompileCode::SetCodeStyle(m_propertySyntax->GetValueAsEnum());

	ibValueMetaObject::OnPropertyChanged(property, oldValue, newValue);
}