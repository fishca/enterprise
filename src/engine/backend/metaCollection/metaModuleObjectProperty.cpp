#include "metaModuleObject.h"

bool CValueMetaObjectCommonModule::OnPropertyChanging(IProperty* property, const wxVariant& newValue)
{
	if (m_propertyGlobalModule == property) {
		return CValueMetaObjectCommonModule::OnBeforeCloseMetaObject() &&
			CValueMetaObjectCommonModule::OnAfterCloseMetaObject();
	}

	return IValueMetaObjectModule::OnPropertyChanging(property, newValue);
}

void CValueMetaObjectCommonModule::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (m_propertyGlobalModule == property) {
		CValueMetaObjectCommonModule::OnBeforeRunMetaObject(newObjectFlag);
		CValueMetaObjectCommonModule::OnAfterRunMetaObject(newObjectFlag);
	}

	IValueMetaObjectModule::OnPropertyChanged(property, oldValue, newValue);
}