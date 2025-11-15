////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : dataProcessor - object
////////////////////////////////////////////////////////////////////////////

#include "dataProcessor.h"

//*********************************************************************************************
//*                                  ObjectCatalogValue                                       *
//*********************************************************************************************

CRecordDataObjectDataProcessor::CRecordDataObjectDataProcessor(CMetaObjectDataProcessor* metaObject) :
	IRecordDataObjectExt(metaObject)
{
}

CRecordDataObjectDataProcessor::CRecordDataObjectDataProcessor(const CRecordDataObjectDataProcessor& source) :
	IRecordDataObjectExt(source)
{
}

#pragma region _form_builder_h_
void CRecordDataObjectDataProcessor::ShowFormValue(const wxString& strFormName, IBackendControlFrame* ownerControl)
{
	IBackendValueForm* foundedForm = GetForm();

	if (foundedForm && foundedForm->IsShown()) {
		foundedForm->ActivateForm();
		return;
	}

	//if form is not initialized then generate  
	IBackendValueForm* valueForm = GetFormValue(strFormName, ownerControl);

	valueForm->Modify(false);
	valueForm->ShowForm();
}

IBackendValueForm* CRecordDataObjectDataProcessor::GetFormValue(const wxString& strFormName, IBackendControlFrame* ownerControl)
{
	IBackendValueForm* const foundedForm = GetForm();

	if (foundedForm == nullptr) {

		IBackendValueForm* createdForm = m_metaObject->CreateAndBuildForm(
			strFormName,
			CMetaObjectDataProcessor::eFormDataProcessor,
			ownerControl,
			this,
			m_objGuid
		);

		return createdForm;
	}

	return foundedForm;
}
#pragma endregion