////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : catalog metaData
////////////////////////////////////////////////////////////////////////////

#include "catalog.h"
#include "list/objectList.h"
#include "backend/metaData.h"
#include "backend/moduleManager/moduleManager.h"

//********************************************************************************************
//*										 metaData											 * 
//********************************************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectCatalog, IMetaObjectRecordDataFolderMutableRef);

//********************************************************************************************
//*                                      metaData                                            *
//********************************************************************************************

CMetaObjectCatalog::CMetaObjectCatalog() : IMetaObjectRecordDataFolderMutableRef()
{
	//set default proc
	(*m_propertyModuleObject)->SetDefaultProcedure("beforeWrite", eContentHelper::eProcedureHelper, {"cancel"});
	(*m_propertyModuleObject)->SetDefaultProcedure("onWrite", eContentHelper::eProcedureHelper, { "cancel" });
	(*m_propertyModuleObject)->SetDefaultProcedure("beforeDelete", eContentHelper::eProcedureHelper, { "cancel" });
	(*m_propertyModuleObject)->SetDefaultProcedure("onDelete", eContentHelper::eProcedureHelper, { "cancel" });

	(*m_propertyModuleObject)->SetDefaultProcedure("filling", eContentHelper::eProcedureHelper, { "source", "standartProcessing" });
	(*m_propertyModuleObject)->SetDefaultProcedure("onCopy", eContentHelper::eProcedureHelper, { "source" });
}

CMetaObjectCatalog::~CMetaObjectCatalog()
{
	//wxDELETE((*m_propertyAttributeOwner));
}

CMetaObjectForm* CMetaObjectCatalog::GetDefaultFormByID(const form_identifier_t& id)
{
	if (id == eFormObject
		&& m_propertyDefFormObject->GetValueAsInteger() != wxNOT_FOUND) {
		for (auto& obj : GetObjectForms()) {
			if (m_propertyDefFormObject->GetValueAsInteger() == obj->GetMetaID()) {
				return obj;
			}
		}
	}
	else if (id == eFormList
		&& m_propertyDefFormList->GetValueAsInteger() != wxNOT_FOUND) {
		for (auto& obj : GetObjectForms()) {
			if (m_propertyDefFormList->GetValueAsInteger() == obj->GetMetaID()) {
				return obj;
			}
		}
	}
	else if (id == eFormSelect
		&& m_propertyDefFormSelect->GetValueAsInteger() != wxNOT_FOUND) {
		for (auto& obj : GetObjectForms()) {
			if (m_propertyDefFormSelect->GetValueAsInteger() == obj->GetMetaID()) {
				return obj;
			}
		}
	}

	return nullptr;
}

ISourceDataObject* CMetaObjectCatalog::CreateSourceObject(IMetaObjectForm* metaObject)
{
	switch (metaObject->GetTypeForm())
	{
	case eFormObject:
		return CreateObjectValue(eObjectMode::OBJECT_ITEM);
	case eFormGroup:
		return CreateObjectValue(eObjectMode::OBJECT_FOLDER);
	case eFormList:
		return m_metaData->CreateAndConvertObjectValueRef<CTreeDataObjectFolderRef>(this, metaObject->GetTypeForm(), CTreeDataObjectFolderRef::LIST_ITEM_FOLDER);
	case eFormSelect:
		return m_metaData->CreateAndConvertObjectValueRef<CTreeDataObjectFolderRef>(this, metaObject->GetTypeForm(), CTreeDataObjectFolderRef::LIST_ITEM_FOLDER, true);
	case eFormFolderSelect:
		return m_metaData->CreateAndConvertObjectValueRef<CTreeDataObjectFolderRef>(this, metaObject->GetTypeForm(), CTreeDataObjectFolderRef::LIST_FOLDER, true);
	}

	return nullptr;
}

#include "backend/appData.h"

IRecordDataObjectFolderRef* CMetaObjectCatalog::CreateObjectRefValue(eObjectMode mode, const Guid& guid)
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);
	CRecordDataObjectCatalog* pDataRef = nullptr;
	if (appData->DesignerMode()) {
		if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
			return m_metaData->CreateAndConvertObjectValueRef<CRecordDataObjectCatalog>(this, guid, mode);
		}
	}
	else {
		pDataRef = m_metaData->CreateAndConvertObjectValueRef<CRecordDataObjectCatalog>(this, guid, mode);
	}

	return pDataRef;
}

#pragma region _form_builder_h_
IBackendValueForm* CMetaObjectCatalog::GetObjectForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IMetaObjectGenericData::CreateAndBuildForm(
		strFormName, 
		CMetaObjectCatalog::eFormObject, 
		ownerControl, CreateObjectValue(eObjectMode::OBJECT_ITEM), 
		formGuid
	);
}

IBackendValueForm* CMetaObjectCatalog::GetFolderForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CMetaObjectCatalog::eFormGroup,
		ownerControl, CreateObjectValue(eObjectMode::OBJECT_FOLDER),
		formGuid
	);
}

IBackendValueForm* CMetaObjectCatalog::GetListForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CMetaObjectCatalog::eFormList,
		ownerControl, m_metaData->CreateAndConvertObjectValueRef<CTreeDataObjectFolderRef>(this, CMetaObjectCatalog::eFormList, CTreeDataObjectFolderRef::LIST_ITEM_FOLDER),
		formGuid
	);
}

IBackendValueForm* CMetaObjectCatalog::GetSelectForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CMetaObjectCatalog::eFormSelect,
		ownerControl, m_metaData->CreateAndConvertObjectValueRef<CTreeDataObjectFolderRef>(this, CMetaObjectCatalog::eFormSelect, CTreeDataObjectFolderRef::LIST_ITEM, true),
		formGuid
	);
}

IBackendValueForm* CMetaObjectCatalog::GetFolderSelectForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CMetaObjectCatalog::eFormSelect,
		ownerControl, m_metaData->CreateAndConvertObjectValueRef<CTreeDataObjectFolderRef>(this, CMetaObjectCatalog::eFormSelect, CTreeDataObjectFolderRef::LIST_FOLDER, true),
		formGuid
	);
}
#pragma endregion

bool CMetaObjectCatalog::GetFormObject(CPropertyList*prop)
{
	prop->AppendItem(wxT("notSelected"), _("<not selected>"), wxNOT_FOUND);
	for (auto formObject : GetObjectForms()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormObject == formObject->GetTypeForm()) {
			prop->AppendItem(formObject->GetName(), formObject->GetMetaID(), formObject);
		}
	}
	return true;
}

bool CMetaObjectCatalog::GetFormFolder(CPropertyList* prop)
{
	prop->AppendItem(wxT("notSelected"), _("<not selected>"), wxNOT_FOUND);
	for (auto formObject : GetObjectForms()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormGroup == formObject->GetTypeForm()) {
			prop->AppendItem(formObject->GetName(), formObject->GetMetaID(), formObject);
		}
	}
	return true;
}

bool CMetaObjectCatalog::GetFormList(CPropertyList* prop)
{
	prop->AppendItem(wxT("notSelected"), _("<not selected>"), wxNOT_FOUND);
	for (auto formObject : GetObjectForms()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormList == formObject->GetTypeForm()) {
			prop->AppendItem(formObject->GetName(), formObject->GetMetaID(), formObject);
		}
	}
	return true;
}

bool CMetaObjectCatalog::GetFormSelect(CPropertyList* prop)
{
	prop->AppendItem(wxT("notSelected"), _("<not selected>"), wxNOT_FOUND);
	for (auto formObject : GetObjectForms()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormSelect == formObject->GetTypeForm()) {
			prop->AppendItem(formObject->GetName(), formObject->GetMetaID(), formObject);
		}
	}
	return true;
}

bool CMetaObjectCatalog::GetFormFolderSelect(CPropertyList* prop)
{
	prop->AppendItem(wxT("notSelected"), _("<not selected>"), wxNOT_FOUND);
	for (auto formObject : GetObjectForms()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormFolderSelect == formObject->GetTypeForm()) {
			prop->AppendItem(formObject->GetName(), formObject->GetMetaID(), formObject);
		}
	}
	return true;
}

wxString CMetaObjectCatalog::GetDataPresentation(const IValueDataObject* objValue) const
{
	CValue vDescription;
	if (objValue->GetValueByMetaID((*m_propertyAttributeDescription)->GetMetaID(), vDescription))
		return vDescription.GetString();
	return wxEmptyString;
}

std::vector<IMetaObjectAttribute*> CMetaObjectCatalog::GetDefaultAttributes() const
{
	std::vector<IMetaObjectAttribute*> attributes;

	attributes.push_back(m_propertyAttributeCode->GetMetaObject());
	attributes.push_back(m_propertyAttributeDescription->GetMetaObject());
	attributes.push_back(m_propertyAttributeOwner->GetMetaObject());
	attributes.push_back(m_propertyAttributeParent->GetMetaObject());
	attributes.push_back(m_propertyAttributeIsFolder->GetMetaObject());
	
	attributes.push_back(m_propertyAttributeReference->GetMetaObject());
	attributes.push_back(m_propertyAttributeDeletionMark->GetMetaObject());

	return attributes;
}

std::vector<IMetaObjectAttribute*> CMetaObjectCatalog::GetSearchedAttributes() const
{
	std::vector<IMetaObjectAttribute*> attributes;

	attributes.push_back(m_propertyAttributeCode->GetMetaObject());
	attributes.push_back(m_propertyAttributeDescription->GetMetaObject());

	return attributes;
}

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool CMetaObjectCatalog::LoadData(CMemoryReader& dataReader)
{
	//load default attributes:
	(*m_propertyAttributeOwner)->LoadMeta(dataReader);

	//Load object module
	(*m_propertyModuleObject)->LoadMeta(dataReader);
	(*m_propertyModuleManager)->LoadMeta(dataReader);

	//load default form 
	m_propertyDefFormObject->SetValue(GetIdByGuid(dataReader.r_stringZ()));
	m_propertyDefFormFolder->SetValue(GetIdByGuid(dataReader.r_stringZ()));
	m_propertyDefFormList->SetValue(GetIdByGuid(dataReader.r_stringZ()));
	m_propertyDefFormSelect->SetValue(GetIdByGuid(dataReader.r_stringZ()));

	if (!m_propertyOwner->LoadData(dataReader))
		return false;

	return IMetaObjectRecordDataFolderMutableRef::LoadData(dataReader);
}

bool CMetaObjectCatalog::SaveData(CMemoryWriter& dataWritter)
{
	//save default attributes:
	(*m_propertyAttributeOwner)->SaveMeta(dataWritter);

	//Save object module
	(*m_propertyModuleObject)->SaveMeta(dataWritter);
	(*m_propertyModuleManager)->SaveMeta(dataWritter);

	//save default form 
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormObject->GetValueAsInteger()));
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormFolder->GetValueAsInteger()));
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormList->GetValueAsInteger()));
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormSelect->GetValueAsInteger()));

	if (!m_propertyOwner->SaveData(dataWritter))
		return false;

	//create or update table:
	return IMetaObjectRecordDataFolderMutableRef::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool CMetaObjectCatalog::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IMetaObjectRecordDataFolderMutableRef::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyAttributeOwner)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyModuleObject)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyModuleManager)->OnCreateMetaObject(metaData, flags);
}

bool CMetaObjectCatalog::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyAttributeOwner)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyModuleObject)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyModuleManager)->OnLoadMetaObject(metaData))
		return false;

	return IMetaObjectRecordDataFolderMutableRef::OnLoadMetaObject(metaData);
}

bool CMetaObjectCatalog::OnSaveMetaObject()
{
	if (!(*m_propertyAttributeOwner)->OnSaveMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnSaveMetaObject())
		return false;

	if (!(*m_propertyModuleManager)->OnSaveMetaObject())
		return false;

	return IMetaObjectRecordDataFolderMutableRef::OnSaveMetaObject();
}

bool CMetaObjectCatalog::OnDeleteMetaObject()
{
	if (!(*m_propertyAttributeOwner)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyModuleManager)->OnDeleteMetaObject())
		return false;

	return IMetaObjectRecordDataFolderMutableRef::OnDeleteMetaObject();
}

bool CMetaObjectCatalog::OnReloadMetaObject()
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		CRecordDataObjectCatalog* pDataRef = nullptr;
		if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
			return true;
		}
		return pDataRef->InitializeObject();
	}

	return true;
}

#include "backend/objCtor.h"

bool CMetaObjectCatalog::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyAttributeOwner)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleManager)->OnBeforeRunMetaObject(flags))
		return false;

	registerSelection();

	if (!IMetaObjectRecordDataFolderMutableRef::OnBeforeRunMetaObject(flags))
		return false;

	IMetaValueTypeCtor* typeCtor =
		m_metaData->GetTypeCtor(this, eCtorMetaType::eCtorMetaType_Reference);

	if (typeCtor != nullptr && !(*m_propertyAttributeParent)->ContainType(typeCtor->GetClassType())) {
		(*m_propertyAttributeParent)->SetDefaultMetaType(typeCtor->GetClassType());
	}

	return true;
}

bool CMetaObjectCatalog::OnAfterRunMetaObject(int flags)
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		if (IMetaObjectRecordDataFolderMutableRef::OnAfterRunMetaObject(flags))
			return moduleManager->AddCompileModule(m_propertyModuleObject->GetMetaObject(), CreateObjectValue(eObjectMode::OBJECT_ITEM));

		return false;
	}

	return IMetaObjectRecordDataFolderMutableRef::OnAfterRunMetaObject(flags);
}

bool CMetaObjectCatalog::OnBeforeCloseMetaObject()
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		if (IMetaObjectRecordDataFolderMutableRef::OnBeforeCloseMetaObject())
			return moduleManager->RemoveCompileModule(m_propertyModuleObject->GetMetaObject());

		return false;
	}

	return IMetaObjectRecordDataFolderMutableRef::OnBeforeCloseMetaObject();
}

bool CMetaObjectCatalog::OnAfterCloseMetaObject()
{
	if (!(*m_propertyAttributeOwner)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyModuleManager)->OnAfterCloseMetaObject())
		return false;

	unregisterSelection();

	return IMetaObjectRecordDataFolderMutableRef::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                             form events                             *
//***********************************************************************

void CMetaObjectCatalog::OnCreateFormObject(IMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CMetaObjectCatalog::eFormObject
		&& m_propertyDefFormObject->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormObject->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CMetaObjectCatalog::eFormGroup
		&& m_propertyDefFormFolder->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormFolder->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CMetaObjectCatalog::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CMetaObjectCatalog::eFormSelect
		&& m_propertyDefFormSelect->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormSelect->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CMetaObjectCatalog::eFormFolderSelect
		&& m_propertyDefFormFolderSelect->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormFolderSelect->SetValue(metaForm->GetMetaID());
	}
}

void CMetaObjectCatalog::OnRemoveMetaForm(IMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CMetaObjectCatalog::eFormObject
		&& m_propertyDefFormObject->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormObject->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CMetaObjectCatalog::eFormGroup
		&& m_propertyDefFormFolder->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormFolder->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CMetaObjectCatalog::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CMetaObjectCatalog::eFormSelect
		&& m_propertyDefFormSelect->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormSelect->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CMetaObjectCatalog::eFormFolderSelect
		&& m_propertyDefFormFolderSelect->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormFolderSelect->SetValue(metaForm->GetMetaID());
	}
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CMetaObjectCatalog, "catalog", g_metaCatalogCLSID);