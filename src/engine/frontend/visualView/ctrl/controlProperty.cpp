#include "control.h"

bool IValueControl::OnPropertyChanging(IProperty* property, const wxVariant& newValue)
{
	if (m_propertyName == property && FindControlByName(newValue.GetString()) != nullptr)
		return false;

	return true;
}

void IValueControl::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (g_visualHostContext != nullptr)
		g_visualHostContext->ModifyProperty(property, oldValue, newValue);
}