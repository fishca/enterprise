#include "metaLanguageObject.h"

bool CMetaObjectLanguage::OnPropertyChanging(IProperty* property, const wxVariant& newValue)
{
	//if (m_propertyCode == property && m_propertyCode->IsEmptyProperty())
	//	return false;

	if (m_propertyCode == property && !IsValidCode(newValue.GetString()))
		return false;

	return IMetaObject::OnPropertyChanging(property, newValue);
}