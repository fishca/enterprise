#include "widgets.h"

void ibValueTextCtrl::OnPropertyCreated(ibProperty* property)
{
	//if (m_propertySource == property) {
	//	ibValueTextCtrl::SaveToVariant(m_propertySource->GetValue(), GetMetaData());
	//}
}

#include <wx/propgrid/manager.h>

#include "backend/metaData.h"
#include "backend/objCtor.h"

void ibValueTextCtrl::OnPropertyRefresh(wxPropertyGridManager* pg, wxPGProperty* pgProperty, ibProperty* property)
{
	if (m_propertyChoiceForm == property) {
		if (GetClsidCount() > 1) {
			pg->HideProperty(pgProperty, true);
		}
		else {
			const ibCtorMetaValueType* so = GetMetaData()->GetTypeCtor(GetFirstClsid());
			if (so != nullptr) {
				if (so->GetMetaTypeCtor() != ibCtorObjectMetaType::ibCtorObjectMetaType_Reference)
					pg->HideProperty(pgProperty, true);
				else
					pg->HideProperty(pgProperty, false);
			}
			else {
				pg->HideProperty(pgProperty, true);
			}
		}
	}
}

bool ibValueTextCtrl::OnPropertyChanging(ibProperty* property, const wxVariant& newValue)
{
	//if (m_propertySource == property && !ibValueTextCtrl::LoadFromVariant(newValue))
	//	return true;
	return ibValueControl::OnPropertyChanging(property, newValue);
}