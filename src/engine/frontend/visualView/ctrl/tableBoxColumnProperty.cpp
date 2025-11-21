#include "tableBox.h"

void CValueTableBoxColumn::OnPropertyCreated(IProperty* property)
{
}

#include <wx/propgrid/manager.h>

#include "backend/metaData.h"
#include "backend/objCtor.h"

void CValueTableBoxColumn::OnPropertyRefresh(wxPropertyGridManager* pg, wxPGProperty* pgProperty, IProperty* property)
{
	if (m_propertyChoiceForm == property) {
		if (GetClsidCount() > 1) {
			pg->HideProperty(pgProperty, true);
		}
		else {
			const IMetaValueTypeCtor* so = GetMetaData()->GetTypeCtor(GetFirstClsid());
			if (so != nullptr) {
				if (so->GetMetaTypeCtor() != eCtorMetaType::eCtorMetaType_Reference)
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

bool CValueTableBoxColumn::OnPropertyChanging(IProperty* property, const wxVariant& newValue)
{
	return IValueControl::OnPropertyChanging(property, newValue);
}