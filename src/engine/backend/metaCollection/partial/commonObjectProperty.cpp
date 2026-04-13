#include "commonObject.h"

void IValueMetaObjectRecordDataMutableRef::OnPropertyCreated(IProperty* property)
{
	IValueMetaObjectRecordDataRef::OnPropertyCreated(property);
}

#include <wx/propgrid/manager.h>

void IValueMetaObjectRecordDataMutableRef::OnPropertyRefresh(wxPropertyGridManager* pg, wxPGProperty* pgProperty, IProperty* property)
{
	if (m_propertyQuickChoice == property) { pg->HideProperty(pgProperty, true); }
}

bool IValueMetaObjectRecordDataMutableRef::OnPropertyChanging(IProperty* property, const wxVariant& newValue)
{
	return IValueMetaObjectRecordDataRef::OnPropertyChanging(property, newValue);
}

void IValueMetaObjectRecordDataMutableRef::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	IValueMetaObjectRecordDataRef::OnPropertyChanged(property, oldValue, newValue);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void IValueMetaObjectRecordDataHierarchyMutableRef::OnPropertyRefresh(wxPropertyGridManager* pg, wxPGProperty* pgProperty, IProperty* property)
{
}