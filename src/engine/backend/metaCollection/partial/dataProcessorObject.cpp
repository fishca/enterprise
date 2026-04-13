////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : dataProcessor - object
////////////////////////////////////////////////////////////////////////////

#include "dataProcessor.h"

//*********************************************************************************************
//*                                  ObjectCatalogValue                                       *
//*********************************************************************************************

CValueRecordDataObjectDataProcessor::CValueRecordDataObjectDataProcessor(CValueMetaObjectDataProcessor* metaObject) :
	IValueRecordDataObjectExt(metaObject)
{
}

CValueRecordDataObjectDataProcessor::CValueRecordDataObjectDataProcessor(const CValueRecordDataObjectDataProcessor& source) :
	IValueRecordDataObjectExt(source)
{
}

#pragma region _form_builder_h_
void CValueRecordDataObjectDataProcessor::ShowFormValue(const wxString& strFormName, IBackendControlFrame* ownerControl)
{
	IBackendValueForm* foundedForm = GetForm();

	if (foundedForm && foundedForm->IsShown()) {
		foundedForm->ActivateForm();
		return;
	}

	//if form is not initialized then generate  
	IBackendValueForm* const valueForm =
		GetFormValue(strFormName, ownerControl);

	if (valueForm != nullptr) {
		valueForm->Modify(false);
		valueForm->ShowForm();
	}
}

IBackendValueForm* CValueRecordDataObjectDataProcessor::GetFormValue(const wxString& strFormName, IBackendControlFrame* ownerControl)
{
	IBackendValueForm* const foundedForm = GetForm();

	if (foundedForm == nullptr) {

		IBackendValueForm* createdForm = m_metaObject->CreateAndBuildForm(
			strFormName,
			CValueMetaObjectDataProcessor::eFormDataProcessor,
			ownerControl,
			this,
			m_objGuid
		);

		return createdForm;
	}

	return foundedForm;
}
#pragma endregion