#include "tableBox.h"
#include "frontend/visualView/ctrl/form.h"
#include "frontend/visualView/visualHost.h"

#include "backend/metaData.h"
#include "backend/objCtor.h"

void CValueTableBox::OnPropertyCreated(IProperty* property)
{
	//if (m_propertySource == property) {
	//	CValueTableBox::SaveToVariant(m_propertySource->GetValue(), GetMetaData());
	//}
}

bool CValueTableBox::OnPropertyChanging(IProperty* property, const wxVariant& newValue)
{
	return IValueWindow::OnPropertyChanging(property, newValue);
}

void CValueTableBox::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (m_propertySource == property) {

		CValueTableBox::RefreshModel(true);

		const int answer = wxMessageBox(
			_("The data source has been changed. Refill columns?"),
			_("TableBox"), wxYES_NO
		);

		if (answer == wxYES) {

			const IMetaData* metaData = GetMetaData();

			while (GetChildCount() != 0) {
				g_visualHostContext->CutControl(GetChild(0), true);
			}

			const class_identifier_t& clsid = CValueTableBox::GetFirstClsid();
			const IMetaValueTypeCtor* typeCtor =
				metaData->GetTypeCtor(clsid);
			if (typeCtor != nullptr) {
				IMetaObjectCompositeData* metaObject =
					dynamic_cast<IMetaObjectCompositeData*>(typeCtor->GetMetaObject());
				if (metaObject != nullptr) {
					for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
						CValueTableBoxColumn* tableBoxColumn =
							wxDynamicCast(
								m_formOwner->CreateControl(wxT("tableboxColumn"), this), CValueTableBoxColumn
							);
						wxASSERT(tableBoxColumn);
						tableBoxColumn->SetControlName(GetControlName() + wxT("_") + object->GetName());
						tableBoxColumn->SetCaption(object->GetSynonym());
						tableBoxColumn->SetSource(object->GetMetaID());
						tableBoxColumn->SetVisibleColumn(true);
						g_visualHostContext->InsertControl(tableBoxColumn, this);
					}
				}
			}

			if (GetChildCount() == 0) {
				CValueTableBox::AddColumn();
			}

			g_visualHostContext->RefreshEditor();
		}
	}

	IValueWindow::OnPropertyChanged(property, oldValue, newValue);
}