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

bool IMetaObjectForm::LoadFormData(IBackendValueForm* valueForm) {
	return valueForm->LoadForm(GetFormData());
}

bool IMetaObjectForm::SaveFormData(IBackendValueForm* valueForm) {
	const wxMemoryBuffer& memoryBuffer = valueForm->SaveForm();
	SetFormData(memoryBuffer);
	return !memoryBuffer.IsEmpty();
}

//***********************************************************************

IBackendValueForm* IMetaObjectForm::GenerateForm(IBackendControlFrame* ownerControl,
	ISourceDataObject* ownerSrc, const CUniqueKey& guidForm)
{
	if (GetFormData().IsEmpty()) {
		IBackendValueForm* valueForm = IBackendValueForm::CreateNewForm(
			ownerControl, this, ownerSrc, guidForm, m_propEnabled
		);
		//build form
		if (m_firstInitialized) {
			m_firstInitialized = false;
			valueForm->BuildForm(GetTypeForm());
			SaveFormData(valueForm);
		}
		return valueForm;
	}

	IBackendValueForm* valueForm = IBackendValueForm::CreateNewForm(
		ownerControl, this, ownerSrc, guidForm, m_propEnabled
	);

	if (!LoadFormData(valueForm)) {
		wxDELETE(valueForm);
	}

	return valueForm;
}

IBackendValueForm* IMetaObjectForm::GenerateFormAndRun(IBackendControlFrame* ownerControl,
	ISourceDataObject* ownerSrc, const CUniqueKey& guidForm)
{
	IBackendValueForm* valueForm = nullptr;
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);
	if (!moduleManager->FindCompileModule(this, valueForm)) {
		valueForm = GenerateForm(ownerControl, ownerSrc, guidForm);
		if (!valueForm->InitializeFormModule()) {
			wxDELETE(valueForm);
			return nullptr;
		}
	}

	return valueForm;
}

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
	//IBackendValueForm* valueForm = nullptr;
	//IModuleManager* moduleManager = m_metaData->GetModuleManager();
	//wxASSERT(moduleManager);
	//
	//if (moduleManager->FindCompileModule(this, valueForm))
	//	return valueForm->LoadForm(GetFormData());

	return true;
}

///////////////////////////////////////////////////////////////////////////

IMetaObjectForm::IMetaObjectForm(const wxString& name, const wxString& synonym, const wxString& comment) :
	IMetaObjectModule(name, synonym, comment), m_firstInitialized(false)
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

bool CMetaObjectForm::GetFormType(CPropertyList* prop)
{
	IMetaObjectGenericData* metaObject = wxDynamicCast(
		GetParent(), IMetaObjectGenericData
	);
	wxASSERT(metaObject);
	prop->AppendItem(formDefaultName, defaultFormType);
	CFormTypeList formList = metaObject->GetFormType();
	for (unsigned int idx = 0; idx < formList.GetItemCount(); idx++) {
		prop->AppendItem(
			formList.GetItemName(idx),
			formList.GetItemLabel(idx),
			formList.GetItemHelp(idx),
			formList.GetItemId(idx),
			formList.GetItemName(idx) 
		);
	}
	return true;
}

//***********************************************************************
//*                             event object                            *
//***********************************************************************

bool CMetaObjectForm::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	m_firstInitialized = true;

	if (!IMetaObjectForm::OnCreateMetaObject(metaData, flags))
		return false;

	if ((flags & newObjectFlag) != 0) {
		
		IMetaObjectGenericData* metaObject = wxDynamicCast(
			m_parent, IMetaObjectGenericData
		);
		
		wxASSERT(metaObject);
		
		form_identifier_t res = wxID_CANCEL;
		if (metaData != nullptr) {
			IBackendMetadataTree* metaTree = metaData->GetMetaTree();
			if (metaTree != nullptr) {
				res = metaTree->SelectFormType(this);
			}
		}
		if (res != wxID_CANCEL) {
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

bool CMetaObjectForm::OnSaveMetaObject()
{
	return IMetaObjectForm::OnSaveMetaObject();
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
	m_firstInitialized = true;
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
		if (moduleManager->AddCompileModule(this, formWrapper::inl::cast_value(GenerateFormAndRun()))) {
			return IMetaObjectModule::OnBeforeRunMetaObject(flags);
		}
		return false;
	}
	return IMetaObjectModule::OnBeforeRunMetaObject(flags);
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