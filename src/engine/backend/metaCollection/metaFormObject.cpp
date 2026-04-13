////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : metaform object
////////////////////////////////////////////////////////////////////////////

#include "metaFormObject.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/metaData.h"
#include "backend/metaCollection/partial/commonObject.h"
#include "backend/appData.h"

// -----------------------------------------------------------------------
// IBackendCommandItem
// -----------------------------------------------------------------------

#include "backend/system/systemManager.h"

bool IBackendCommandItem::ShowFormByCommandType(EInterfaceCommandType cmdType)
{
	IBackendValueForm* valueForm = nullptr;

	try {

		valueForm = GetFormByCommandType(cmdType);

		if (valueForm == nullptr)
			return false;

		valueForm->ShowForm();
	}
	catch (const CBackendAccessException* err) {
		wxDELETE(valueForm);
		CSystemFunction::Alert(err->GetErrorDescription());
		return false;
	}
	catch (const CBackendException*) {
		wxDELETE(valueForm);
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------
// IValueMetaObjectForm
// -----------------------------------------------------------------------

wxIMPLEMENT_ABSTRACT_CLASS(IValueMetaObjectForm, CValueMetaObjectModule);

//***********************************************************************
//*                          common value object                        *
//***********************************************************************

bool IValueMetaObjectForm::LoadFormData(IBackendValueForm* valueForm) const {
	return valueForm->LoadForm(GetFormData());
}

bool IValueMetaObjectForm::SaveFormData(IBackendValueForm* valueForm) {
	const wxMemoryBuffer& memoryBuffer = valueForm->SaveForm();
	SetFormData(memoryBuffer);
	return !memoryBuffer.IsEmpty();
}

//***********************************************************************

#pragma region _form_creator_h_

IBackendValueForm* IValueMetaObjectForm::CreateAndBuildForm(const IValueMetaObjectForm* creator,
	IBackendControlFrame* ownerControl, ISourceDataObject* srcObject, const CUniqueKey& formGuid)
{
	return CreateAndBuildForm(creator,
		creator != nullptr ? creator->GetTypeForm() : defaultFormType, ownerControl, srcObject, formGuid);
}

IBackendValueForm* IValueMetaObjectForm::CreateAndBuildForm(const IValueMetaObjectForm* creator, const form_identifier_t& form_id,
	IBackendControlFrame* ownerControl, ISourceDataObject* srcObject, const CUniqueKey& formGuid)
{
	IBackendValueForm* result = nullptr;

	if (creator != nullptr) {
		const IMetaData* metaData = creator->GetMetaData();
		wxASSERT(metaData);
		const IValueModuleManager* moduleManager = metaData->GetModuleManager();
		wxASSERT(moduleManager);
		if (!moduleManager->FindCompileModule(creator, result)) {
			result = IBackendValueForm::CreateNewForm(creator, ownerControl, srcObject, formGuid);
			if (!creator->GetFormData().IsEmpty() && !creator->LoadFormData(result)) {
				wxDELETE(result);
				return nullptr;
			}
			else if (creator->GetFormData().IsEmpty()) {
				result->BuildForm(form_id);
			}
		}
	}
	else {
		result = IBackendValueForm::CreateNewForm(nullptr, ownerControl, srcObject, formGuid);
		result->BuildForm(form_id);
	}

	if (result != nullptr) {

		bool success = true;

		try {
			success = result->InitializeFormModule();
		}
		catch (...) {
			success = false;
		}

		if (!success) {
			wxDELETE(result);
			return nullptr;
		}

		if (srcObject != nullptr) result->Modify(srcObject->IsModified());
	}

	return result;
}

#pragma endregion

///////////////////////////////////////////////////////////////////////////

wxMemoryBuffer IValueMetaObjectForm::CopyFormData() const
{
	IBackendValueForm* valueForm = nullptr;
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);
	if (moduleManager->FindCompileModule(this, valueForm))
		return valueForm->SaveForm();
	return wxMemoryBuffer();
}

bool IValueMetaObjectForm::PasteFormData()
{
	return true;
}

///////////////////////////////////////////////////////////////////////////

IValueMetaObjectForm::IValueMetaObjectForm(const wxString& name, const wxString& synonym, const wxString& comment) :
	IValueMetaObjectModule(name, synonym, comment)
{
	//set default proc
	SetDefaultProcedure(wxT("BeforeOpen"), eContentHelper::eProcedureHelper, { wxT("Cancel") });
	SetDefaultProcedure(wxT("OnOpen"), eContentHelper::eProcedureHelper);
	SetDefaultProcedure(wxT("BeforeClose"), eContentHelper::eProcedureHelper, { wxT("Cancel") });
	SetDefaultProcedure(wxT("OnClose"), eContentHelper::eProcedureHelper);

	SetDefaultProcedure(wxT("OnReOpen"), eContentHelper::eProcedureHelper);
	SetDefaultProcedure(wxT("ChoiceProcessing"), eContentHelper::eProcedureHelper, { { wxT("SelectedValue") }, { wxT("ChoiceSource") } });
	SetDefaultProcedure(wxT("RefreshDisplay"), eContentHelper::eProcedureHelper);
}

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectForm, IValueMetaObjectForm)

//***********************************************************************
//*                            Metaform                                 *
//***********************************************************************

CValueMetaObjectForm::CValueMetaObjectForm(const wxString& name, const wxString& synonym, const wxString& comment) : IValueMetaObjectForm(name, synonym, comment)
{
}

bool CValueMetaObjectForm::LoadData(CMemoryReader& reader)
{
	m_properyFormType->SetValue(reader.r_s32());
	return m_propertyForm->LoadData(reader);
}

bool CValueMetaObjectForm::SaveData(CMemoryWriter& writer)
{
	writer.w_s32(m_properyFormType->GetValueAsInteger());
	return m_propertyForm->SaveData(writer);
}

//***********************************************************************

bool CValueMetaObjectForm::FillGenericFormType(CPropertyList* prop)
{
	const IValueMetaObjectGenericData* geneticObject = m_parent != nullptr ?
		m_parent->ConvertToType<IValueMetaObjectGenericData>() : nullptr;

	if (geneticObject != nullptr) {

		CFormTypeList formList = geneticObject->GetFormType();
		for (unsigned int idx = 0; idx < formList.GetItemCount(); idx++) {
			prop->AppendItem(
				formList.GetItemName(idx),
				formList.GetItemLabel(idx),
				formList.GetItemHelp(idx),
				formList.GetItemId(idx),
				GetIcon(),
				formList.GetItemName(idx)
			);
		}

		return true;
	}

	return false;
}

//***********************************************************************
//*                             event object                            *
//***********************************************************************

bool CValueMetaObjectForm::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IValueMetaObjectForm::OnCreateMetaObject(metaData, flags))
		return false;

	if ((flags & newObjectFlag) != 0) {

		IValueMetaObjectGenericData* metaObject = wxDynamicCast(
			m_parent, IValueMetaObjectGenericData
		);

		wxASSERT(metaObject);

		form_identifier_t res = wxNOT_FOUND;
		if (metaData != nullptr) {
			IBackendMetadataTree* metaTree = metaData->GetMetaTree();
			if (metaTree != nullptr) {
				res = metaTree->SelectFormType(this);
			}
		}
		if (res != wxNOT_FOUND) {
			m_properyFormType->SetValue(res);
		}
		else {
			return false;
		}
		metaObject->OnCreateFormObject(this);
		if (metaData != nullptr) {
			IBackendMetadataTree* metaTree = metaData->GetMetaTree();
			if (metaTree != nullptr) {
				metaTree->UpdateChoiceSelection();
			}
		}
	}

	return true;
}

bool CValueMetaObjectForm::OnLoadMetaObject(IMetaData* metaData)
{
	return IValueMetaObjectForm::OnLoadMetaObject(metaData);
}

bool CValueMetaObjectForm::OnSaveMetaObject(int flags)
{
	return IValueMetaObjectForm::OnSaveMetaObject(flags);
}

bool CValueMetaObjectForm::OnDeleteMetaObject()
{
	IValueMetaObjectGenericData* metaObject = wxDynamicCast(m_parent, IValueMetaObjectGenericData);
	wxASSERT(metaObject);
	metaObject->OnRemoveMetaForm(this);

	return IValueMetaObjectForm::OnDeleteMetaObject();
}

bool CValueMetaObjectForm::OnBeforeRunMetaObject(int flags)
{
	return IValueMetaObjectForm::OnBeforeRunMetaObject(flags);
}

bool CValueMetaObjectForm::OnAfterRunMetaObject(int flags)
{
	if (appData->DesignerMode()) {

		IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
		wxASSERT(moduleManager);
		IValueMetaObjectGenericData* metaObject = wxDynamicCast(m_parent, IValueMetaObjectGenericData);
		wxASSERT(metaObject);

		return moduleManager->AddCompileModule(this, formWrapper::inl::cast_value(metaObject->CreateObjectForm(this)));
	}

	return IValueMetaObjectForm::OnAfterRunMetaObject(flags);
}

bool CValueMetaObjectForm::OnBeforeCloseMetaObject()
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!moduleManager->RemoveCompileModule(this))
		return false;

	return IValueMetaObjectForm::OnBeforeCloseMetaObject();
}

bool CValueMetaObjectForm::OnAfterCloseMetaObject()
{
	return IValueMetaObjectForm::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                           CommonFormObject metaData                 *
//***********************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectCommonForm, IValueMetaObjectForm)

CValueMetaObjectCommonForm::CValueMetaObjectCommonForm(const wxString& name, const wxString& synonym, const wxString& comment) : IValueMetaObjectForm(name, synonym, comment) {}

bool CValueMetaObjectCommonForm::LoadData(CMemoryReader& reader)
{
	return m_propertyForm->LoadData(reader);
}

bool CValueMetaObjectCommonForm::SaveData(CMemoryWriter& writer)
{
	return m_propertyForm->SaveData(writer);
}

#include "backend/system/systemManager.h"

IBackendValueForm* CValueMetaObjectCommonForm::GetObjectForm(IBackendControlFrame* ownerControl, const CUniqueKey& formGuid) const
{
	if (!AccessRight_Use()) {
		CBackendAccessException::Error();
		return false;
	}

	return IValueMetaObjectForm::CreateAndBuildForm(
		this,
		ownerControl,
		nullptr,
		formGuid
	);
}

//***********************************************************************
//*                             event object                            *
//***********************************************************************

bool CValueMetaObjectCommonForm::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	return IValueMetaObjectModule::OnCreateMetaObject(metaData, flags);
}

bool CValueMetaObjectCommonForm::OnBeforeRunMetaObject(int flags)
{
	return IValueMetaObjectModule::OnBeforeRunMetaObject(flags);
}

bool CValueMetaObjectCommonForm::OnAfterRunMetaObject(int flags)
{
	if (appData->DesignerMode()) {
		IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
		wxASSERT(moduleManager);
		if (moduleManager->AddCompileModule(this, formWrapper::inl::cast_value(IValueMetaObjectForm::CreateAndBuildForm(this, defaultFormType)))) {
			return IValueMetaObjectModule::OnAfterRunMetaObject(flags);
		}
		return false;
	}
	return IValueMetaObjectModule::OnAfterRunMetaObject(flags);
}

bool CValueMetaObjectCommonForm::OnBeforeCloseMetaObject()
{
	if (appData->DesignerMode()) {
		IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
		wxASSERT(moduleManager);
		if (moduleManager->RemoveCompileModule(this)) {
			return IValueMetaObjectModule::OnBeforeCloseMetaObject();
		}
		return false;
	}

	return IValueMetaObjectModule::OnBeforeCloseMetaObject();
}

bool CValueMetaObjectCommonForm::OnAfterCloseMetaObject()
{
	return IValueMetaObjectModule::OnAfterCloseMetaObject();
}


//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CValueMetaObjectForm, "Form", g_metaFormCLSID);
METADATA_TYPE_REGISTER(CValueMetaObjectCommonForm, "CommonForm", g_metaCommonFormCLSID);