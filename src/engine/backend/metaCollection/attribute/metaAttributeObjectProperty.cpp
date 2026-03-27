////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : meta-attribues
////////////////////////////////////////////////////////////////////////////

#include "metaAttributeObject.h"

#include "backend/metaData.h"
#include "backend/objCtor.h"

void ibValueMetaObjectAttribute::OnPropertyCreated(ibProperty* property)
{
	//if (m_propertyType == property) {
	/////	ibValueMetaObjectAttribute::SaveToVariant(m_propertyType->GetValue(), m_metaData);
	//}
}

#include <wx/propgrid/manager.h>

void ibValueMetaObjectAttribute::OnPropertyRefresh(wxPropertyGridManager* pg, wxPGProperty* pgProperty, ibProperty* property)
{
	if (m_propertySelectMode == property) {
		if (GetClsidCount() > 1) {
			pg->HideProperty(pgProperty, true);
		}
		else {
			const ibCtorMetaValueType* so = GetMetaData()->GetTypeCtor(GetFirstClsid());
			if (so != nullptr) {
				ibValueMetaObjectRecordDataHierarchyMutableRef* metaObject = dynamic_cast<ibValueMetaObjectRecordDataHierarchyMutableRef*>(so->GetMetaObject());
				if (metaObject == nullptr)
					pg->HideProperty(pgProperty, true);
				else if (so->GetMetaTypeCtor() != ibCtorMetaType::ibCtorMetaType_Reference)
					pg->HideProperty(pgProperty, true);
				else 
					pg->HideProperty(pgProperty, false);
			}
			else {
				pg->HideProperty(pgProperty, true);
			}
		}
	}
	else if (m_propertyItemMode == property) {
		ibValueMetaObjectRecordDataHierarchyMutableRef* metaObject = dynamic_cast<ibValueMetaObjectRecordDataHierarchyMutableRef*>(m_parent);
		pg->HideProperty(pgProperty, metaObject == nullptr);
	}
}

bool ibValueMetaObjectAttribute::OnPropertyChanging(ibProperty* property, const wxVariant& newValue)
{
	//if (m_propertyType == property && !ibValueMetaObjectAttribute::LoadFromVariant(newValue))
	//	return false;
	return ibValueMetaObjectAttributeBase::OnPropertyChanging(property, newValue);
}

void ibValueMetaObjectAttribute::OnPropertyChanged(ibProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	ibValueMetaObject* metaObject = GetParent();
	wxASSERT(metaObject);

	if (metaObject->OnReloadMetaObject()) {
		ibValueMetaObject::OnPropertyChanged(property, oldValue, newValue);
	}
}