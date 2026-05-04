////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : report - object
////////////////////////////////////////////////////////////////////////////

#include "dataReport.h"

//*********************************************************************************************
//*                                  ObjectCatalogValue                                       *
//*********************************************************************************************

ibValueRecordDataObjectReport::ibValueRecordDataObjectReport(const ibValueMetaObjectReport* metaObject) : ibValueRecordDataObjectExt(metaObject)
{
}

ibValueRecordDataObjectReport::ibValueRecordDataObjectReport(const ibValueRecordDataObjectReport& source) : ibValueRecordDataObjectExt(source)
{
}

#pragma region _form_builder_h_
void ibValueRecordDataObjectReport::ShowFormValue(const wxString& strFormName, ibBackendControlFrame* ownerControl)
{
	ibBackendValueForm* const foundedForm = GetForm();

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

ibBackendValueForm* ibValueRecordDataObjectReport::GetFormValue(const wxString& strFormName, ibBackendControlFrame* ownerControl)
{
	ibBackendValueForm* const foundedForm = GetForm();

	if (foundedForm == nullptr) {

		ibBackendValueForm* createdForm = m_metaObject->CreateAndBuildForm(
			strFormName,
			ibValueMetaObjectReport::eFormReport,
			ownerControl,
			this,
			m_objGuid
		);

		return createdForm;
	}

	return foundedForm;
}
#pragma endregion