////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : metaform object
////////////////////////////////////////////////////////////////////////////

#include "metaFormObject.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/metaData.h"
#include "backend/metaCollection/partial/commonObject.h"
#include "backend/appData.h"

//***********************************************************************
//*                             IMetaObjectForm metaData                *
//***********************************************************************

wxIMPLEMENT_ABSTRACT_CLASS(IMetaObjectForm, CMetaObjectModule);

//***********************************************************************
//*                          common value object                        *
//***********************************************************************

bool IMetaObjectForm::LoadFormData(IBackendValueForm* valueForm) const {
	return valueForm->LoadForm(GetFormData());
}

bool IMetaObjectForm::SaveFormData(IBackendValueForm* valueForm) {
	const wxMemoryBuffer& memoryBuffer = valueForm->SaveForm();
	SetFormData(memoryBuffer);
	return !memoryBuffer.IsEmpty();
}

//***********************************************************************

#pragma region _form_creator_h_

IBackendValueForm* IMetaObjectForm::CreateAndBuildForm(const IMetaObjectForm* creator,
	IBackendControlFrame* ownerControl, ISourceDataObject* srcObject, const CUniqueKey& formGuid)
{
	return CreateAndBuildForm(creator,
		creator != nullptr ? creator->GetTypeForm() : defaultFormType, ownerControl, srcObject, formGuid);
}

IBackendValueForm* IMetaObjectForm::CreateAndBuildForm(const IMetaObjectForm* creator, const form_identifier_t& form_id,
	IBackendControlFrame* ownerControl, ISourceDataObject* srcObject, const CUniqueKey& formGuid)
{
	IBackendValueForm* result = nullptr;

	if (creator != nullptr) {
		const IMetaData* metaData = creator->GetMetaData();
		wxASSERT(metaData);
		const IModuleManager* moduleManager = metaData->GetModuleManager();
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

wxMemoryBuffer IMetaObjectForm::CopyFormData() const
{
	IBackendValueForm* valueForm = nullptr;
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);
	if (moduleManager->FindCompileModule(this, valueForm))
		return valueForm->SaveForm();
	return wxMemoryBuffer();
}

bool IMetaObjectForm::PasteFormData()
{
	return true;
}

///////////////////////////////////////////////////////////////////////////

IMetaObjectForm::IMetaObjectForm(const wxString& name, const wxString& synonym, const wxString& comment) :
	IMetaObjectModule(name, synonym, comment)
{
	//set default proc
	SetDefaultProcedure("beforeOpen", eContentHelper::eProcedureHelper, { "cancel" });
	SetDefaultProcedure("onOpen", eContentHelper::eProcedureHelper);
	SetDefaultProcedure("beforeClose", eContentHelper::eProcedureHelper, { "cancel" });
	SetDefaultProcedure("onClose", eContentHelper::eProcedureHelper);

	SetDefaultProcedure("onReOpen", eContentHelper::eProcedureHelper);
	SetDefaultProcedure("choiceProcessing", eContentHelper::eProcedureHelper, { { "selectedValue" }, { "choiceSource" } });
	SetDefaultProcedure("refreshDisplay", eContentHelper::eProcedureHelper);
}

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectForm, IMetaObjectForm)

//***********************************************************************
//*                            Metaform                                 *
//***********************************************************************

CMetaObjectForm::CMetaObjectForm(const wxString& name, const wxString& synonym, const wxString& comment) : IMetaObjectForm(name, synonym, comment)
{
}

bool CMetaObjectForm::LoadData(CMemoryReader& reader)
{
	m_properyFormType->SetValue(reader.r_s32());
	return m_propertyForm->LoadData(reader);
}

bool CMetaObjectForm::SaveData(CMemoryWriter& writer)
{
	writer.w_s32(m_properyFormType->GetValueAsInteger());
	return m_propertyForm->SaveData(writer);
}

//***********************************************************************

bool CMetaObjectForm::FillGenericFormType(CPropertyList* prop)
{
	const IMetaObjectGenericData* geneticObject = m_parent != nullptr ?
		m_parent->ConvertToType<IMetaObjectGenericData>() : nullptr;

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

bool CMetaObjectForm::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IMetaObjectForm::OnCreateMetaObject(metaData, flags))
		return false;

	if ((flags & newObjectFlag) != 0) {

		IMetaObjectGenericData* metaObject = wxDynamicCast(
			m_parent, IMetaObjectGenericData
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

bool CMetaObjectForm::OnLoadMetaObject(IMetaData* metaData)
{
	return IMetaObjectForm::OnLoadMetaObject(metaData);
}

bool CMetaObjectForm::OnSaveMetaObject(int flags)
{
	return IMetaObjectForm::OnSaveMetaObject(flags);
}

bool CMetaObjectForm::OnDeleteMetaObject()
{
	IMetaObjectGenericData* metaObject = wxDynamicCast(m_parent, IMetaObjectGenericData);
	wxASSERT(metaObject);
	metaObject->OnRemoveMetaForm(this);

	return IMetaObjectForm::OnDeleteMetaObject();
}

bool CMetaObjectForm::OnBeforeRunMetaObject(int flags)
{
	return IMetaObjectForm::OnBeforeRunMetaObject(flags);
}

bool CMetaObjectForm::OnAfterRunMetaObject(int flags)
{
	if (appData->DesignerMode()) {

		IModuleManager* moduleManager = m_metaData->GetModuleManager();
		wxASSERT(moduleManager);
		IMetaObjectGenericData* metaObject = wxDynamicCast(m_parent, IMetaObjectGenericData);
		wxASSERT(metaObject);

		return moduleManager->AddCompileModule(this, formWrapper::inl::cast_value(metaObject->CreateObjectForm(this)));
	}

	return IMetaObjectForm::OnAfterRunMetaObject(flags);
}

bool CMetaObjectForm::OnBeforeCloseMetaObject()
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!moduleManager->RemoveCompileModule(this))
		return false;

	return IMetaObjectForm::OnBeforeCloseMetaObject();
}

bool CMetaObjectForm::OnAfterCloseMetaObject()
{
	return IMetaObjectForm::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                           CommonFormObject metaData                 *
//***********************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectCommonForm, IMetaObjectForm)

CMetaObjectCommonForm::CMetaObjectCommonForm(const wxString& name, const wxString& synonym, const wxString& comment) : IMetaObjectForm(name, synonym, comment) {}

bool CMetaObjectCommonForm::LoadData(CMemoryReader& reader)
{
	return m_propertyForm->LoadData(reader);
}

bool CMetaObjectCommonForm::SaveData(CMemoryWriter& writer)
{
	return m_propertyForm->SaveData(writer);
}

//***********************************************************************
//*                             event object                            *
//***********************************************************************

bool CMetaObjectCommonForm::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	return IMetaObjectModule::OnCreateMetaObject(metaData, flags);
}

bool CMetaObjectCommonForm::OnBeforeRunMetaObject(int flags)
{
	return IMetaObjectModule::OnBeforeRunMetaObject(flags);
}

bool CMetaObjectCommonForm::OnAfterRunMetaObject(int flags)
{
	if (appData->DesignerMode()) {
		IModuleManager* moduleManager = m_metaData->GetModuleManager();
		wxASSERT(moduleManager);
		if (moduleManager->AddCompileModule(this, formWrapper::inl::cast_value(IMetaObjectForm::CreateAndBuildForm(this, defaultFormType)))) {
			return IMetaObjectModule::OnAfterRunMetaObject(flags);
		}
		return false;
	}
	return IMetaObjectModule::OnAfterRunMetaObject(flags);
}

bool CMetaObjectCommonForm::OnBeforeCloseMetaObject()
{
	if (appData->DesignerMode()) {
		IModuleManager* moduleManager = m_metaData->GetModuleManager();
		wxASSERT(moduleManager);
		if (moduleManager->RemoveCompileModule(this)) {
			return IMetaObjectModule::OnBeforeCloseMetaObject();
		}
		return false;
	}

	return IMetaObjectModule::OnBeforeCloseMetaObject();
}

bool CMetaObjectCommonForm::OnAfterCloseMetaObject()
{
	return IMetaObjectModule::OnAfterCloseMetaObject();
}


//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CMetaObjectForm, "form", g_metaFormCLSID);
METADATA_TYPE_REGISTER(CMetaObjectCommonForm, "commonForm", g_metaCommonFormCLSID);