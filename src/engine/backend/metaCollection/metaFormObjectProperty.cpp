////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : metaform property
////////////////////////////////////////////////////////////////////////////

#include "metaFormObject.h"
#include "backend/metaData.h"
#include "backend/metaCollection/partial/commonObject.h"
#include "backend/appData.h"

void ibValueMetaObjectForm::OnPropertyCreated(ibProperty* property)
{
	ibValueMetaObjectFormBase::OnPropertyCreated(property);
}

void ibValueMetaObjectForm::OnPropertySelected(ibProperty* property)
{
	ibValueMetaObjectFormBase::OnPropertySelected(property);
}

void ibValueMetaObjectForm::OnPropertyChanged(ibProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (property == m_properyFormType) {
		if (appData->DesignerMode()) {
			ibValueModuleManager* moduleManager = m_metaData->GetModuleManager();
			wxASSERT(moduleManager);
			ibValueMetaObjectGenericData* metaObjectValue = wxDynamicCast(m_parent, ibValueMetaObjectGenericData);
			wxASSERT(metaObjectValue);
			ibBackendMetadataTree* metaTree = m_metaData->GetMetaTree();
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

	ibValueMetaObjectFormBase::OnPropertyChanged(property, oldValue, newValue);
}