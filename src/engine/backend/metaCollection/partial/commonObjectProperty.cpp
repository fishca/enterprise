#include "commonObject.h"

void ibValueMetaObjectRecordDataMutableRef::OnPropertyCreated(ibProperty* property)
{
	ibValueMetaObjectRecordDataRef::OnPropertyCreated(property);
}

#include <wx/propgrid/manager.h>

void ibValueMetaObjectRecordDataMutableRef::OnPropertyRefresh(wxPropertyGridManager* pg, wxPGProperty* pgProperty, ibProperty* property)
{
	if (m_propertyQuickChoice == property) { pg->HideProperty(pgProperty, true); }
}

bool ibValueMetaObjectRecordDataMutableRef::OnPropertyChanging(ibProperty* property, const wxVariant& newValue)
{
	return ibValueMetaObjectRecordDataRef::OnPropertyChanging(property, newValue);
}

void ibValueMetaObjectRecordDataMutableRef::OnPropertyChanged(ibProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	ibValueMetaObjectRecordDataRef::OnPropertyChanged(property, oldValue, newValue);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void ibValueMetaObjectRecordDataHierarchyMutableRef::OnPropertyRefresh(wxPropertyGridManager* pg, wxPGProperty* pgProperty, ibProperty* property)
{
}