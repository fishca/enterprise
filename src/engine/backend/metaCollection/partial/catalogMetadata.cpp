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
	(*m_propertyModuleObject)->SetDefaultProcedure("beforeWrite", eContentHelper::eProcedureHelper, { "cancel" });
	(*m_propertyModuleObject)->SetDefaultProcedure("onWrite", eContentHelper::eProcedureHelper, { "cancel" });
	(*m_propertyModuleObject)->SetDefaultProcedure("beforeDelete", eContentHelper::eProcedureHelper, { "cancel" });
	(*m_propertyModuleObject)->SetDefaultProcedure("onDelete", eContentHelper::eProcedureHelper, { "cancel" });

	(*m_propertyModuleObject)->SetDefaultProcedure("filling", eContentHelper::eProcedureHelper, { "source", "standartProcessing" });
	(*m_propertyModuleObject)->SetDefaultProcedure("onCopy", eContentHelper::eProcedureHelper, { "source" });

	(*m_propertyModuleObject)->SetDefaultProcedure("setNewCode", eContentHelper::eProcedureHelper, { "prefix", "standartProcessing" });
}

CMetaObjectCatalog::~CMetaObjectCatalog()
{
	//wxDELETE((*m_propertyAttributeOwner));
}

IMetaObjectForm* CMetaObjectCatalog::GetDefaultFormByID(const form_identifier_t& id) const
{
	if (id == eFormObject && m_propertyDefFormObject->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormObject->GetValueAsInteger());
	}
	else if (id == eFormFolder && m_propertyDefFormFolder->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormFolder->GetValueAsInteger());
	}
	else if (id == eFormList && m_propertyDefFormList->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormList->GetValueAsInteger());
	}
	else if (id == eFormSelect && m_propertyDefFormSelect->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormSelect->GetValueAsInteger());
	}
	else if (id == eFormFolderSelect && m_propertyDefFormFolderSelect->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormFolderSelect->GetValueAsInteger());
	}

	return nullptr;
}

ISourceDataObject* CMetaObjectCatalog::CreateSourceObject(IMetaObjectForm* metaObject)
{
	switch (metaObject->GetTypeForm())
	{
	case eFormObject:
		return CreateObjectValue(eObjectMode::OBJECT_ITEM);
	case eFormFolder:
		return CreateObjectValue(eObjectMode::OBJECT_FOLDER);
	case eFormList:
		return CValue::CreateAndPrepareValueRef<CTreeDataObjectFolderRef>(this, metaObject->GetTypeForm(), CTreeDataObjectFolderRef::LIST_ITEM_FOLDER);
	case eFormSelect:
		return CValue::CreateAndPrepareValueRef<CTreeDataObjectFolderRef>(this, metaObject->GetTypeForm(), CTreeDataObjectFolderRef::LIST_ITEM_FOLDER, true);
	case eFormFolderSelect:
		return CValue::CreateAndPrepareValueRef<CTreeDataObjectFolderRef>(this, metaObject->GetTypeForm(), CTreeDataObjectFolderRef::LIST_FOLDER, true);
	}

	return nullptr;
}

#include "backend/appData.h"

IRecordDataObjectFolderRef* CMetaObjectCatalog::CreateObjectRefValue(eObjectMode mode, const CGuid& guid)
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);
	CRecordDataObjectCatalog* pDataRef = nullptr;
	if (appData->DesignerMode()) {
		if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
			return CValue::CreateAndPrepareValueRef<CRecordDataObjectCatalog>(this, guid, mode);
		}
	}
	else {
		pDataRef = CValue::CreateAndPrepareValueRef<CRecordDataObjectCatalog>(this, guid, mode);
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
		CMetaObjectCatalog::eFormFolder,
		ownerControl, CreateObjectValue(eObjectMode::OBJECT_FOLDER),
		formGuid
	);
}

IBackendValueForm* CMetaObjectCatalog::GetListForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CMetaObjectCatalog::eFormList,
		ownerControl, CValue::CreateAndPrepareValueRef<CTreeDataObjectFolderRef>(this, CMetaObjectCatalog::eFormList, CTreeDataObjectFolderRef::LIST_ITEM_FOLDER),
		formGuid
	);
}

IBackendValueForm* CMetaObjectCatalog::GetSelectForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CMetaObjectCatalog::eFormSelect,
		ownerControl, CValue::CreateAndPrepareValueRef<CTreeDataObjectFolderRef>(this, CMetaObjectCatalog::eFormSelect, CTreeDataObjectFolderRef::LIST_ITEM, true),
		formGuid
	);
}

IBackendValueForm* CMetaObjectCatalog::GetFolderSelectForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CMetaObjectCatalog::eFormFolderSelect,
		ownerControl, CValue::CreateAndPrepareValueRef<CTreeDataObjectFolderRef>(this, CMetaObjectCatalog::eFormSelect, CTreeDataObjectFolderRef::LIST_FOLDER, true),
		formGuid
	);
}
#pragma endregion

bool CMetaObjectCatalog::GetFormObject(CPropertyList* prop)
{
	for (auto formObject : GetFormArrayObject()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormObject == formObject->GetTypeForm()) {
			prop->AppendItem(
				formObject->GetName(), 
				formObject->GetMetaID(), 
				formObject->GetIcon(),
				formObject);
		}
	}
	return true;
}

bool CMetaObjectCatalog::GetFormFolder(CPropertyList* prop)
{
	for (auto formObject : GetFormArrayObject()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormFolder == formObject->GetTypeForm()) {
			prop->AppendItem(
				formObject->GetName(), 
				formObject->GetMetaID(), 
				formObject->GetIcon(),
				formObject);
		}
	}
	return true;
}

bool CMetaObjectCatalog::GetFormList(CPropertyList* prop)
{
	for (auto formObject : GetFormArrayObject()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormList == formObject->GetTypeForm()) {
			prop->AppendItem(
				formObject->GetName(),
				formObject->GetMetaID(),
				formObject->GetIcon(),
				formObject);
		}
	}
	return true;
}

bool CMetaObjectCatalog::GetFormSelect(CPropertyList* prop)
{
	for (auto formObject : GetFormArrayObject()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormSelect == formObject->GetTypeForm()) {
			prop->AppendItem(
				formObject->GetName(),
				formObject->GetMetaID(),
				formObject->GetIcon(),
				formObject);
		}
	}
	return true;
}

bool CMetaObjectCatalog::GetFormFolderSelect(CPropertyList* prop)
{
	for (auto formObject : GetFormArrayObject()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormFolderSelect == formObject->GetTypeForm()) {
			prop->AppendItem(
				formObject->GetName(), 
				formObject->GetMetaID(), 
				formObject->GetIcon(),
				formObject);
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

bool CMetaObjectCatalog::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyAttributeOwner)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyModuleManager)->OnSaveMetaObject(flags))
		return false;

	return IMetaObjectRecordDataFolderMutableRef::OnSaveMetaObject(flags);
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

	const IMetaValueTypeCtor* typeCtor =
		m_metaData->GetTypeCtor(this, eCtorMetaType::eCtorMetaType_Reference);

	if (typeCtor != nullptr && !(*m_propertyAttributeParent)->ContainType(typeCtor->GetClassType())) {
		(*m_propertyAttributeParent)->SetDefaultMetaType(typeCtor->GetClassType());
	}

	return true;
}

bool CMetaObjectCatalog::OnAfterRunMetaObject(int flags)
{
	if (!(*m_propertyAttributeOwner)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleObject)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyModuleManager)->OnAfterRunMetaObject(flags))
		return false;

	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	const CMetaDescription& metaDesc = m_propertyOwner->GetValueAsMetaDesc(); CTypeDescription typeDesc;
	for (unsigned int idx = 0; idx < metaDesc.GetTypeCount(); idx++) {
		const IMetaObject* catalog = m_metaData->FindAnyObjectByFilter(metaDesc.GetByIdx(idx));
		if (catalog != nullptr) {
			const IMetaValueTypeCtor* so = m_metaData->GetTypeCtor(catalog, eCtorMetaType::eCtorMetaType_Reference);
			wxASSERT(so);
			typeDesc.AppendMetaType(so->GetClassType());
		}
	}

	(*m_propertyAttributeOwner)->SetDefaultMetaType(typeDesc);

	if ((*m_propertyAttributeOwner)->GetClsidCount() > 0)
		(*m_propertyAttributeOwner)->ClearFlag(metaDisableFlag);
	else
		(*m_propertyAttributeOwner)->SetFlag(metaDisableFlag);

	if (appData->DesignerMode()) {

		if (IMetaObjectRecordDataFolderMutableRef::OnAfterRunMetaObject(flags))
			return moduleManager->AddCompileModule(m_propertyModuleObject->GetMetaObject(), CreateObjectValue(eObjectMode::OBJECT_ITEM));

		return false;
	}

	return IMetaObjectRecordDataFolderMutableRef::OnAfterRunMetaObject(flags);
}

bool CMetaObjectCatalog::OnBeforeCloseMetaObject()
{
	if (!(*m_propertyAttributeOwner)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyModuleObject)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyModuleManager)->OnBeforeCloseMetaObject())
		return false;

	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	const CMetaDescription& metaDesc = m_propertyOwner->GetValueAsMetaDesc();
	for (unsigned int idx = 0; idx < metaDesc.GetTypeCount(); idx++) {
		const IMetaObject* catalog = m_metaData->FindAnyObjectByFilter(metaDesc.GetByIdx(idx));
		if (catalog != nullptr) {
			const IMetaValueTypeCtor* so = m_metaData->GetTypeCtor(catalog, eCtorMetaType::eCtorMetaType_Reference);
			wxASSERT(so);
			(*m_propertyAttributeOwner)->GetTypeDesc().ClearMetaType(so->GetClassType());
		}
	}

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
	else if (metaForm->GetTypeForm() == CMetaObjectCatalog::eFormFolder
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
	else if (metaForm->GetTypeForm() == CMetaObjectCatalog::eFormFolder
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