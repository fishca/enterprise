////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : dataProcessor - object
////////////////////////////////////////////////////////////////////////////

#include "dataProcessor.h"

//*********************************************************************************************
//*                                  ObjectCatalogValue                                       *
//*********************************************************************************************

ibValueRecordDataObjectDataProcessor::ibValueRecordDataObjectDataProcessor(ibValueMetaObjectDataProcessor* metaObject) :
	ibValueRecordDataObjectExt(metaObject)
{
}

ibValueRecordDataObjectDataProcessor::ibValueRecordDataObjectDataProcessor(const ibValueRecordDataObjectDataProcessor& source) :
	ibValueRecordDataObjectExt(source)
{
}

#pragma region _form_builder_h_
void ibValueRecordDataObjectDataProcessor::ShowFormValue(const wxString& strFormName, ibBackendControlFrame* ownerControl)
{
	ibBackendValueForm* foundedForm = GetForm();

	if (foundedForm && foundedForm->IsShown()) {
		foundedForm->ActivateForm();
		return;
	}

	//if form is not initialized then generate  
	ibBackendValueForm* const valueForm =
		GetFormValue(strFormName, ownerControl);

	if (valueForm != nullptr) {
		valueForm->Modify(false);
		valueForm->ShowForm();
	}
}

ibBackendValueForm* ibValueRecordDataObjectDataProcessor::GetFormValue(const wxString& strFormName, ibBackendControlFrame* ownerControl)
{
	ibBackendValueForm* const foundedForm = GetForm();

	if (foundedForm == nullptr) {

		ibBackendValueForm* createdForm = m_metaObject->CreateAndBuildForm(
			strFormName,
			ibValueMetaObjectDataProcessor::eFormDataProcessor,
			ownerControl,
			this,
			m_objGuid
		);

		return createdForm;
	}

	return foundedForm;
}
#pragma endregion