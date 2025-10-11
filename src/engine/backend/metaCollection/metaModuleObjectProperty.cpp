#include "metaModuleObject.h"

bool CMetaObjectCommonModule::OnPropertyChanging(IProperty* property, const wxVariant& newValue)
{
	if (m_propertyGlobalModule == property) {
		return CMetaObjectCommonModule::OnBeforeCloseMetaObject() &&
			CMetaObjectCommonModule::OnAfterCloseMetaObject();
	}

	return IMetaObjectModule::OnPropertyChanging(property, newValue);
}

void CMetaObjectCommonModule::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (m_propertyGlobalModule == property) {
		CMetaObjectCommonModule::OnBeforeRunMetaObject(newObjectFlag);
		CMetaObjectCommonModule::OnAfterRunMetaObject(newObjectFlag);
	}

	IMetaObjectModule::OnPropertyChanged(property, oldValue, newValue);
}