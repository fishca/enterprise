#include "tableBox.h"

void ibValueModelTableBoxColumn::OnPropertyCreated(ibProperty* property)
{
}

#include <wx/propgrid/manager.h>

#include "backend/metaData.h"
#include "backend/objCtor.h"

void ibValueModelTableBoxColumn::OnPropertyRefresh(wxPropertyGridManager* pg, wxPGProperty* pgProperty, ibProperty* property)
{
	if (m_propertyChoiceForm == property) {
		if (GetClsidCount() > 1) {
			pg->HideProperty(pgProperty, true);
		}
		else {
			const ibCtorMetaValueType* so = GetMetaData()->GetTypeCtor(GetFirstClsid());
			if (so != nullptr) {
				if (so->GetMetaTypeCtor() != ibCtorMetaType::ibCtorMetaType_Reference)
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

bool ibValueModelTableBoxColumn::OnPropertyChanging(ibProperty* property, const wxVariant& newValue)
{
	return ibValueControl::OnPropertyChanging(property, newValue);
}