////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : meta-attribues
////////////////////////////////////////////////////////////////////////////

#include "metaTableObject.h"
#include "backend/metaData.h"

#include <wx/propgrid/manager.h>

void CValueMetaObjectTableData::OnPropertyRefresh(wxPropertyGridManager* pg, wxPGProperty* pgProperty, IProperty* property)
{
	if (m_propertyUse == property) {
		IValueMetaObjectRecordDataHierarchyMutableRef* metaObject = dynamic_cast<IValueMetaObjectRecordDataHierarchyMutableRef*>(m_parent);
		pg->HideProperty(pgProperty, metaObject == nullptr);
	}
}