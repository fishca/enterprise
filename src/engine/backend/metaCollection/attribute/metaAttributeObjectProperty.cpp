////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : meta-attribues
////////////////////////////////////////////////////////////////////////////

#include "metaAttributeObject.h"

#include "backend/metaData.h"
#include "backend/objCtor.h"

void CValueMetaObjectAttribute::OnPropertyCreated(IProperty* property)
{
	//if (m_propertyType == property) {
	/////	CValueMetaObjectAttribute::SaveToVariant(m_propertyType->GetValue(), m_metaData);
	//}
}

#include <wx/propgrid/manager.h>

void CValueMetaObjectAttribute::OnPropertyRefresh(wxPropertyGridManager* pg, wxPGProperty* pgProperty, IProperty* property)
{
	if (m_propertySelectMode == property) {
		if (GetClsidCount() > 1) {
			pg->HideProperty(pgProperty, true);
		}
		else {
			const IMetaValueTypeCtor* so = GetMetaData()->GetTypeCtor(GetFirstClsid());
			if (so != nullptr) {
				IValueMetaObjectRecordDataHierarchyMutableRef* metaObject = dynamic_cast<IValueMetaObjectRecordDataHierarchyMutableRef*>(so->GetMetaObject());
				if (metaObject == nullptr)
					pg->HideProperty(pgProperty, true);
				else if (so->GetMetaTypeCtor() != eCtorMetaType::eCtorMetaType_Reference)
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
		IValueMetaObjectRecordDataHierarchyMutableRef* metaObject = dynamic_cast<IValueMetaObjectRecordDataHierarchyMutableRef*>(m_parent);
		pg->HideProperty(pgProperty, metaObject == nullptr);
	}
}

bool CValueMetaObjectAttribute::OnPropertyChanging(IProperty* property, const wxVariant& newValue)
{
	//if (m_propertyType == property && !CValueMetaObjectAttribute::LoadFromVariant(newValue))
	//	return false;
	return IValueMetaObjectAttribute::OnPropertyChanging(property, newValue);
}

void CValueMetaObjectAttribute::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	IValueMetaObject* metaObject = GetParent();
	wxASSERT(metaObject);

	if (metaObject->OnReloadMetaObject()) {
		IValueMetaObject::OnPropertyChanged(property, oldValue, newValue);
	}
}