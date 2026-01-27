#include "commonObject.h"

void IMetaObjectRecordDataMutableRef::OnPropertyCreated(IProperty* property)
{
	IMetaObjectRecordDataRef::OnPropertyCreated(property);
}

#include <wx/propgrid/manager.h>

void IMetaObjectRecordDataMutableRef::OnPropertyRefresh(wxPropertyGridManager* pg, wxPGProperty* pgProperty, IProperty* property)
{
	if (m_propertyQuickChoice == property) { pg->HideProperty(pgProperty, true); }
}

bool IMetaObjectRecordDataMutableRef::OnPropertyChanging(IProperty* property, const wxVariant& newValue)
{
	return IMetaObjectRecordDataRef::OnPropertyChanging(property, newValue);
}

void IMetaObjectRecordDataMutableRef::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	IMetaObjectRecordDataRef::OnPropertyChanged(property, oldValue, newValue);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void IMetaObjectRecordDataHierarchyMutableRef::OnPropertyRefresh(wxPropertyGridManager* pg, wxPGProperty* pgProperty, IProperty* property)
{
}