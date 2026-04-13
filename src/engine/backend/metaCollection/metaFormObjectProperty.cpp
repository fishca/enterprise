////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : metaform property
////////////////////////////////////////////////////////////////////////////

#include "metaFormObject.h"
#include "backend/metaData.h"
#include "backend/metaCollection/partial/commonObject.h"
#include "backend/appData.h"

void CValueMetaObjectForm::OnPropertyCreated(IProperty* property)
{
	IValueMetaObjectForm::OnPropertyCreated(property);
}

void CValueMetaObjectForm::OnPropertySelected(IProperty* property)
{
	IValueMetaObjectForm::OnPropertySelected(property);
}

void CValueMetaObjectForm::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (property == m_properyFormType) {
		if (appData->DesignerMode()) {
			IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
			wxASSERT(moduleManager);
			IValueMetaObjectGenericData* metaObjectValue = wxDynamicCast(m_parent, IValueMetaObjectGenericData);
			wxASSERT(metaObjectValue);
			IBackendMetadataTree* metaTree = m_metaData->GetMetaTree();
			if (metaTree != nullptr) {
				metaTree->CloseMetaObject(this);
			}
			if (moduleManager->RemoveCompileModule(this)) {
				moduleManager->AddCompileModule(this, formWrapper::inl::cast_value(metaObjectValue->CreateObjectForm(this)));
			}
			if (metaTree != nullptr) {
				metaTree->UpdateChoiceSelection();
			}
		}
	}

	IValueMetaObjectForm::OnPropertyChanged(property, oldValue, newValue);
}