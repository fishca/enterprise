////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : meta-attribues
////////////////////////////////////////////////////////////////////////////

#include "metaTableObject.h"
#include "backend/metaData.h"

#include <wx/propgrid/manager.h>

void ibValueMetaObjectTableData::OnPropertyRefresh(wxPropertyGridManager* pg, wxPGProperty* pgProperty, ibProperty* property)
{
	if (m_propertyUse == property) {
		ibValueMetaObjectRecordDataHierarchyMutableRef* metaObject = dynamic_cast<ibValueMetaObjectRecordDataHierarchyMutableRef*>(m_parent);
		pg->HideProperty(pgProperty, metaObject == nullptr);
	}
}