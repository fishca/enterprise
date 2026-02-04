#include "metaLanguageObject.h"

bool CValueMetaObjectLanguage::OnPropertyChanging(IProperty* property, const wxVariant& newValue)
{
	//if (m_propertyCode == property && m_propertyCode->IsEmptyProperty())
	//	return false;

	if (m_propertyCode == property && !IsValidCode(newValue.GetString()))
		return false;

	return IValueMetaObject::OnPropertyChanging(property, newValue);
}