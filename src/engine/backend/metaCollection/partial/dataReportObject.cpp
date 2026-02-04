////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : report - object
////////////////////////////////////////////////////////////////////////////

#include "dataReport.h"

//*********************************************************************************************
//*                                  ObjectCatalogValue                                       *
//*********************************************************************************************

CValueRecordDataObjectReport::CValueRecordDataObjectReport(CValueMetaObjectReport* metaObject) : IValueRecordDataObjectExt(metaObject)
{
}

CValueRecordDataObjectReport::CValueRecordDataObjectReport(const CValueRecordDataObjectReport& source) : IValueRecordDataObjectExt(source)
{
}

#pragma region _form_builder_h_
void CValueRecordDataObjectReport::ShowFormValue(const wxString& strFormName, IBackendControlFrame* ownerControl)
{
	IBackendValueForm* const foundedForm = GetForm();

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

IBackendValueForm* CValueRecordDataObjectReport::GetFormValue(const wxString& strFormName, IBackendControlFrame* ownerControl)
{
	IBackendValueForm* const foundedForm = GetForm();

	if (foundedForm == nullptr) {

		IBackendValueForm* createdForm = m_metaObject->CreateAndBuildForm(
			strFormName,
			CValueMetaObjectReport::eFormReport,
			ownerControl,
			this,
			m_objGuid
		);

		return createdForm;
	}

	return foundedForm;
}
#pragma endregion