////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : common classes for catalogs, docs etc..  
////////////////////////////////////////////////////////////////////////////

#include "commonObject.h"
#include "backend/metaData.h"
#include "backend/srcExplorer.h"
#include "backend/system/systemManager.h"
#include "backend/objCtor.h"


#include "backend/metaCollection/partial/reference/reference.h"

//***********************************************************************
//*								 metaData                               * 
//***********************************************************************

wxIMPLEMENT_ABSTRACT_CLASS(IMetaObjectGenericData, IMetaObjectCompositeData);
wxIMPLEMENT_ABSTRACT_CLASS(IMetaObjectRegisterData, IMetaObjectGenericData);

wxIMPLEMENT_ABSTRACT_CLASS(IMetaObjectRecordData, IMetaObjectGenericData);
wxIMPLEMENT_ABSTRACT_CLASS(IMetaObjectRecordDataExt, IMetaObjectRecordData);
wxIMPLEMENT_ABSTRACT_CLASS(IMetaObjectRecordDataRef, IMetaObjectRecordData);
wxIMPLEMENT_ABSTRACT_CLASS(IMetaObjectRecordDataEnumRef, IMetaObjectRecordDataRef);
wxIMPLEMENT_ABSTRACT_CLASS(IMetaObjectRecordDataMutableRef, IMetaObjectRecordDataRef);
wxIMPLEMENT_ABSTRACT_CLASS(IMetaObjectRecordDataFolderMutableRef, IMetaObjectRecordDataMutableRef);

//***********************************************************************
//*							IMetaObjectGenericData				        *
//***********************************************************************

#pragma region _form_builder_h_
IBackendValueForm* IMetaObjectGenericData::GetGenericForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return CreateAndBuildForm(strFormName, defaultFormType, ownerControl, nullptr, formGuid);
}
#pragma endregion
#pragma region _form_creator_h_
IBackendValueForm* IMetaObjectGenericData::CreateAndBuildForm(const wxString& strFormName, const form_identifier_t& form_id, IBackendControlFrame* ownerControl, ISourceDataObject* srcObject, const CUniqueKey& formGuid)
{
	IMetaObjectForm* creator = nullptr;

	if (srcObject != nullptr) srcObject->SourceIncrRef();

	if (!strFormName.IsEmpty()) {

		creator = FindFormObjectByFilter(strFormName, form_id);

		if (creator == nullptr) {
			if (srcObject != nullptr) srcObject->SourceDecrRef();
			CSystemFunction::Raise(_("Form not found '") + strFormName + "'");
			return nullptr;
		}
	}

	IBackendValueForm* result = IBackendValueForm::FindFormByUniqueKey(ownerControl, srcObject, formGuid);

	if (result == nullptr) {

		result = IMetaObjectForm::CreateAndBuildForm(
			creator != nullptr ? creator : GetDefaultFormByID(form_id),
			form_id,
			ownerControl, srcObject, formGuid
		);
	}

	if (srcObject != nullptr) srcObject->SourceDecrRef();
	return result;
}
#pragma endregion

//***********************************************************************
//*                           IMetaObjectRecordData					    *
//***********************************************************************

#include "backend/fileSystem/fs.h"

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool IMetaObjectRecordData::OnLoadMetaObject(IMetaData* metaData)
{
	return IMetaObject::OnLoadMetaObject(metaData);
}

bool IMetaObjectRecordData::OnSaveMetaObject(int flags)
{
	return IMetaObject::OnSaveMetaObject(flags);
}

bool IMetaObjectRecordData::OnDeleteMetaObject()
{
	return IMetaObject::OnDeleteMetaObject();
}

bool IMetaObjectRecordData::OnBeforeRunMetaObject(int flags)
{
	return IMetaObject::OnBeforeRunMetaObject(flags);
}

bool IMetaObjectRecordData::OnAfterCloseMetaObject()
{
	return IMetaObject::OnAfterCloseMetaObject();
}

//***********************************************************************
//*						IMetaObjectRecordDataExt					    *
//***********************************************************************

IMetaObjectRecordDataExt::IMetaObjectRecordDataExt() :
	IMetaObjectRecordData()
{
}

IRecordDataObjectExt* IMetaObjectRecordDataExt::CreateObjectValue()
{
	IRecordDataObjectExt* createdValue = CreateObjectExtValue();
	if (!IsExternalCreate()) {
		if (createdValue && !createdValue->InitializeObject()) {
			wxDELETE(createdValue);
			return nullptr;
		}
	}
	return createdValue;
}

IRecordDataObjectExt* IMetaObjectRecordDataExt::CreateObjectValue(IRecordDataObjectExt* objSrc)
{
	IRecordDataObjectExt* createdValue = CreateObjectExtValue();
	if (!IsExternalCreate()) {
		if (createdValue && !createdValue->InitializeObject(objSrc)) {
			wxDELETE(createdValue);
			return nullptr;
		}
	}
	return createdValue;
}

IRecordDataObject* IMetaObjectRecordDataExt::CreateRecordDataObject()
{
	return CreateObjectValue();
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool IMetaObjectRecordDataExt::OnBeforeRunMetaObject(int flags)
{
	if (IsExternalCreate()) {
		registerExternalObject();
		registerExternalManager();
	}
	else {
		registerObject();
		registerManager();
	}

	return IMetaObjectRecordData::OnBeforeRunMetaObject(flags);
}

bool IMetaObjectRecordDataExt::OnAfterCloseMetaObject()
{
	unregisterObject();
	unregisterManager();

	return IMetaObjectRecordData::OnAfterCloseMetaObject();
}

//***********************************************************************
//*						IMetaObjectRecordDataRef					    *
//***********************************************************************

IMetaObjectRecordDataRef::IMetaObjectRecordDataRef() : IMetaObjectRecordData()
{
}

IMetaObjectRecordDataRef::~IMetaObjectRecordDataRef()
{
	//wxDELETE((*m_propertyAttributeReference));
}

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool IMetaObjectRecordDataRef::LoadData(CMemoryReader& dataReader)
{
	//get quick choice
	m_propertyQuickChoice->SetValue(dataReader.r_u8());

	//load default attributes:
	(*m_propertyAttributeReference)->LoadMeta(dataReader);

	return IMetaObjectRecordData::LoadData(dataReader);
}

bool IMetaObjectRecordDataRef::SaveData(CMemoryWriter& dataWritter)
{
	//set quick choice
	dataWritter.w_u8(m_propertyQuickChoice->GetValueAsBoolean());

	//save default attributes:
	(*m_propertyAttributeReference)->SaveMeta(dataWritter);

	//create or update table:
	return IMetaObjectRecordData::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool IMetaObjectRecordDataRef::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IMetaObjectRecordData::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyAttributeReference)->OnCreateMetaObject(metaData, flags);
}

#include "backend/appData.h"
#include "databaseLayer/databaseLayer.h"

bool IMetaObjectRecordDataRef::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyAttributeReference)->OnLoadMetaObject(metaData))
		return false;

	return IMetaObjectRecordData::OnLoadMetaObject(metaData);
}

bool IMetaObjectRecordDataRef::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyAttributeReference)->OnSaveMetaObject(flags))
		return false;

	return IMetaObjectRecordData::OnSaveMetaObject(flags);
}

bool IMetaObjectRecordDataRef::OnDeleteMetaObject()
{
	if (!(*m_propertyAttributeReference)->OnDeleteMetaObject())
		return false;

	return IMetaObjectRecordData::OnDeleteMetaObject();
}

bool IMetaObjectRecordDataRef::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyAttributeReference)->OnBeforeRunMetaObject(flags))
		return false;

	registerReference();
	registerRefList();
	registerManager();

	const IMetaValueTypeCtor* typeCtor = m_metaData->GetTypeCtor(this, eCtorMetaType::eCtorMetaType_Reference);
	if (typeCtor != nullptr)
		(*m_propertyAttributeReference)->SetDefaultMetaType(typeCtor->GetClassType());

	return IMetaObjectRecordData::OnBeforeRunMetaObject(flags);
}

bool IMetaObjectRecordDataRef::OnAfterRunMetaObject(int flags)
{
	return IMetaObjectRecordData::OnAfterRunMetaObject(flags);
}

bool IMetaObjectRecordDataRef::OnBeforeCloseMetaObject()
{
	return IMetaObjectRecordData::OnBeforeCloseMetaObject();
}

bool IMetaObjectRecordDataRef::OnAfterCloseMetaObject()
{
	if (!(*m_propertyAttributeReference)->OnAfterCloseMetaObject())
		return false;

	if (m_propertyAttributeReference != nullptr)
		(*m_propertyAttributeReference)->SetDefaultMetaType(eValueTypes::TYPE_EMPTY);

	unregisterReference();
	unregisteRefList();
	unregisterManager();

	return IMetaObjectRecordData::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                             form events                             *
//***********************************************************************

//process choice 
bool IMetaObjectRecordDataRef::ProcessChoice(IBackendControlFrame* ownerValue, const wxString& strFormName, eSelectMode selMode)
{
	IBackendValueForm* const selectChoiceForm = GetSelectForm(strFormName, ownerValue);
	if (selectChoiceForm == nullptr)
		return false;

	selectChoiceForm->ShowForm();
	return true;
}

CReferenceDataObject* IMetaObjectRecordDataRef::FindObjectValue(const CGuid& objGuid)
{
	if (!objGuid.isValid())
		return nullptr;
	return CReferenceDataObject::Create(this, objGuid);
}

//***********************************************************************
//*						IMetaObjectRecordDataEnumRef					*
//***********************************************************************

///////////////////////////////////////////////////////////////////////////////////////////////

IMetaObjectRecordDataEnumRef::IMetaObjectRecordDataEnumRef() : IMetaObjectRecordDataRef()
{
}

IMetaObjectRecordDataEnumRef::~IMetaObjectRecordDataEnumRef()
{
	//wxDELETE((*m_propertyAttributeOrder));
}

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool IMetaObjectRecordDataEnumRef::LoadData(CMemoryReader& dataReader)
{
	//load default attributes:
	(*m_propertyAttributeOrder)->LoadMeta(dataReader);
	return IMetaObjectRecordDataRef::LoadData(dataReader);
}

bool IMetaObjectRecordDataEnumRef::SaveData(CMemoryWriter& dataWritter)
{
	//save default attributes:
	(*m_propertyAttributeOrder)->SaveMeta(dataWritter);
	return IMetaObjectRecordDataRef::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool IMetaObjectRecordDataEnumRef::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IMetaObjectRecordDataRef::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyAttributeOrder)->OnCreateMetaObject(metaData, flags);
}

bool IMetaObjectRecordDataEnumRef::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyAttributeOrder)->OnLoadMetaObject(metaData))
		return false;

	return IMetaObjectRecordDataRef::OnLoadMetaObject(metaData);
}

bool IMetaObjectRecordDataEnumRef::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyAttributeOrder)->OnSaveMetaObject(flags))
		return false;

	return IMetaObjectRecordDataRef::OnSaveMetaObject(flags);
}

bool IMetaObjectRecordDataEnumRef::OnDeleteMetaObject()
{
	if (!(*m_propertyAttributeOrder)->OnDeleteMetaObject())
		return false;

	return IMetaObjectRecordDataRef::OnDeleteMetaObject();
}

bool IMetaObjectRecordDataEnumRef::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyAttributeOrder)->OnBeforeRunMetaObject(flags))
		return false;

	return IMetaObjectRecordDataRef::OnBeforeRunMetaObject(flags);
}

bool IMetaObjectRecordDataEnumRef::OnAfterRunMetaObject(int flags)
{
	return IMetaObjectRecordDataRef::OnAfterRunMetaObject(flags);
}

bool IMetaObjectRecordDataEnumRef::OnBeforeCloseMetaObject()
{
	return IMetaObjectRecordDataRef::OnBeforeCloseMetaObject();
}

bool IMetaObjectRecordDataEnumRef::OnAfterCloseMetaObject()
{
	if (!(*m_propertyAttributeOrder)->OnAfterCloseMetaObject())
		return false;

	return IMetaObjectRecordDataRef::OnAfterCloseMetaObject();
}

//***********************************************************************
//*						IMetaObjectRecordDataMutableRef					*
//***********************************************************************

IMetaObjectRecordDataMutableRef::IMetaObjectRecordDataMutableRef() : IMetaObjectRecordDataRef()
{
}

IMetaObjectRecordDataMutableRef::~IMetaObjectRecordDataMutableRef()
{
	//wxDELETE((*m_propertyAttributeDataVersion);
	//wxDELETE((*m_propertyAttributeDeletionMark));
}

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool IMetaObjectRecordDataMutableRef::LoadData(CMemoryReader& dataReader)
{
	//load default attributes:
	(*m_propertyAttributeDataVersion)->LoadMeta(dataReader);
	(*m_propertyAttributeDeletionMark)->LoadMeta(dataReader);

	if (!IMetaObjectRecordDataRef::LoadData(dataReader))
		return false;

	return m_propertyGeneration->LoadData(dataReader);
}

bool IMetaObjectRecordDataMutableRef::SaveData(CMemoryWriter& dataWritter)
{
	//save default attributes:
	(*m_propertyAttributeDataVersion)->SaveMeta(dataWritter);
	(*m_propertyAttributeDeletionMark)->SaveMeta(dataWritter);

	//create or update table:
	if (!IMetaObjectRecordDataRef::SaveData(dataWritter))
		return false;

	return m_propertyGeneration->SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool IMetaObjectRecordDataMutableRef::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IMetaObjectRecordDataRef::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyAttributeDataVersion)->OnCreateMetaObject(metaData, flags)
		&& (*m_propertyAttributeDeletionMark)->OnCreateMetaObject(metaData, flags);
}

bool IMetaObjectRecordDataMutableRef::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyAttributeDataVersion)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyAttributeDeletionMark)->OnLoadMetaObject(metaData))
		return false;

	return IMetaObjectRecordDataRef::OnLoadMetaObject(metaData);
}

bool IMetaObjectRecordDataMutableRef::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyAttributeDataVersion)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeDeletionMark)->OnSaveMetaObject(flags))
		return false;

	return IMetaObjectRecordDataRef::OnSaveMetaObject(flags);
}

bool IMetaObjectRecordDataMutableRef::OnDeleteMetaObject()
{
	if (!(*m_propertyAttributeDataVersion)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyAttributeDeletionMark)->OnDeleteMetaObject())
		return false;

	return IMetaObjectRecordDataRef::OnDeleteMetaObject();
}

bool IMetaObjectRecordDataMutableRef::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyAttributeDataVersion)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeDeletionMark)->OnBeforeRunMetaObject(flags))
		return false;

	registerObject();
	return IMetaObjectRecordDataRef::OnBeforeRunMetaObject(flags);
}

bool IMetaObjectRecordDataMutableRef::OnAfterRunMetaObject(int flags)
{
	if (!(*m_propertyAttributeDataVersion)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeDeletionMark)->OnAfterRunMetaObject(flags))
		return false;

	return IMetaObjectRecordDataRef::OnAfterRunMetaObject(flags);
}

bool IMetaObjectRecordDataMutableRef::OnBeforeCloseMetaObject()
{
	if (!(*m_propertyAttributeDataVersion)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeDeletionMark)->OnBeforeCloseMetaObject())
		return false;

	return IMetaObjectRecordDataRef::OnBeforeCloseMetaObject();
}

bool IMetaObjectRecordDataMutableRef::OnAfterCloseMetaObject()
{
	if (!(*m_propertyAttributeDataVersion)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeDeletionMark)->OnAfterCloseMetaObject())
		return false;

	unregisterObject();
	return IMetaObjectRecordDataRef::OnAfterCloseMetaObject();
}

///////////////////////////////////////////////////////////////////////////////

IRecordDataObjectRef* IMetaObjectRecordDataMutableRef::CreateObjectValue()
{
	IRecordDataObjectRef* createdValue = CreateObjectRefValue();
	if (createdValue && !createdValue->InitializeObject()) {
		wxDELETE(createdValue);
		return nullptr;
	}

	return createdValue;
}

IRecordDataObjectRef* IMetaObjectRecordDataMutableRef::CreateObjectValue(const CGuid& guid)
{
	IRecordDataObjectRef* createdValue = CreateObjectRefValue(guid);
	if (createdValue && !createdValue->InitializeObject()) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IRecordDataObjectRef* IMetaObjectRecordDataMutableRef::CreateObjectValue(IRecordDataObjectRef* objSrc, bool generate)
{
	if (objSrc == nullptr)
		return nullptr;
	IRecordDataObjectRef* createdValue = CreateObjectRefValue();
	if (createdValue && !createdValue->InitializeObject(objSrc, generate)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IRecordDataObjectRef* IMetaObjectRecordDataMutableRef::CopyObjectValue(const CGuid& srcGuid)
{
	IRecordDataObjectRef* createdValue = CreateObjectRefValue();
	if (createdValue && !createdValue->InitializeObject(srcGuid)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IRecordDataObject* IMetaObjectRecordDataMutableRef::CreateRecordDataObject()
{
	return CreateObjectValue();
}

//***********************************************************************
//*						IMetaObjectRecordDataFolderMutableRef			*
//***********************************************************************

IMetaObjectRecordDataFolderMutableRef::IMetaObjectRecordDataFolderMutableRef()
	: IMetaObjectRecordDataMutableRef()
{
}

IMetaObjectRecordDataFolderMutableRef::~IMetaObjectRecordDataFolderMutableRef()
{
	//wxDELETE(m_propertyAttributeCode);
	//wxDELETE(m_propertyAttributeDescription);
	//wxDELETE(m_propertyAttributeParent);
	//wxDELETE(m_propertyAttributeIsFolder);
}

////////////////////////////////////////////////////////////////////////////////////////////////

IRecordDataObjectFolderRef* IMetaObjectRecordDataFolderMutableRef::CreateObjectValue(eObjectMode mode)
{
	IRecordDataObjectFolderRef* createdValue = CreateObjectRefValue(mode);
	if (createdValue && !createdValue->InitializeObject()) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IRecordDataObjectFolderRef* IMetaObjectRecordDataFolderMutableRef::CreateObjectValue(eObjectMode mode, const CGuid& guid)
{
	IRecordDataObjectFolderRef* createdValue = CreateObjectRefValue(mode, guid);
	if (createdValue && !createdValue->InitializeObject()) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IRecordDataObjectFolderRef* IMetaObjectRecordDataFolderMutableRef::CreateObjectValue(eObjectMode mode, IRecordDataObjectRef* objSrc, bool generate)
{
	IRecordDataObjectFolderRef* createdValue = CreateObjectRefValue(mode);
	if (createdValue && !createdValue->InitializeObject(objSrc, generate)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IRecordDataObjectFolderRef* IMetaObjectRecordDataFolderMutableRef::CopyObjectValue(eObjectMode mode, const CGuid& srcGuid)
{
	IRecordDataObjectFolderRef* createdValue = CreateObjectRefValue(mode);
	if (createdValue && !createdValue->InitializeObject(srcGuid)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool IMetaObjectRecordDataFolderMutableRef::LoadData(CMemoryReader& dataReader)
{
	//load default attributes:
	(*m_propertyAttributeCode)->LoadMeta(dataReader);
	(*m_propertyAttributeDescription)->LoadMeta(dataReader);
	(*m_propertyAttributeParent)->LoadMeta(dataReader);
	(*m_propertyAttributeIsFolder)->LoadMeta(dataReader);

	return IMetaObjectRecordDataMutableRef::LoadData(dataReader);
}

bool IMetaObjectRecordDataFolderMutableRef::SaveData(CMemoryWriter& dataWritter)
{
	//save default attributes:
	(*m_propertyAttributeCode)->SaveMeta(dataWritter);
	(*m_propertyAttributeDescription)->SaveMeta(dataWritter);
	(*m_propertyAttributeParent)->SaveMeta(dataWritter);
	(*m_propertyAttributeIsFolder)->SaveMeta(dataWritter);

	//create or update table:
	return IMetaObjectRecordDataMutableRef::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool IMetaObjectRecordDataFolderMutableRef::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IMetaObjectRecordDataMutableRef::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyAttributeCode)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyAttributeDescription)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyAttributeParent)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyAttributeIsFolder)->OnCreateMetaObject(metaData, flags);
}

bool IMetaObjectRecordDataFolderMutableRef::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyAttributeCode)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyAttributeDescription)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyAttributeParent)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyAttributeIsFolder)->OnLoadMetaObject(metaData))
		return false;

	return IMetaObjectRecordDataMutableRef::OnLoadMetaObject(metaData);
}

bool IMetaObjectRecordDataFolderMutableRef::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyAttributeCode)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeDescription)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeParent)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeIsFolder)->OnSaveMetaObject(flags))
		return false;

	return IMetaObjectRecordDataMutableRef::OnSaveMetaObject(flags);
}

bool IMetaObjectRecordDataFolderMutableRef::OnDeleteMetaObject()
{
	if (!(*m_propertyAttributeCode)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyAttributeDescription)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyAttributeParent)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyAttributeIsFolder)->OnDeleteMetaObject())
		return false;

	return IMetaObjectRecordDataMutableRef::OnDeleteMetaObject();
}

bool IMetaObjectRecordDataFolderMutableRef::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyAttributeCode)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeDescription)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeParent)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeIsFolder)->OnBeforeRunMetaObject(flags))
		return false;

	if (!IMetaObjectRecordDataMutableRef::OnBeforeRunMetaObject(flags))
		return false;

	return true;
}

bool IMetaObjectRecordDataFolderMutableRef::OnAfterRunMetaObject(int flags)
{
	if (!(*m_propertyAttributeCode)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeDescription)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeParent)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeIsFolder)->OnAfterRunMetaObject(flags))
		return false;

	return IMetaObjectRecordDataMutableRef::OnAfterRunMetaObject(flags);
}

bool IMetaObjectRecordDataFolderMutableRef::OnBeforeCloseMetaObject()
{
	if (!(*m_propertyAttributeCode)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeDescription)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeParent)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeIsFolder)->OnBeforeCloseMetaObject())
		return false;

	return IMetaObjectRecordDataMutableRef::OnBeforeCloseMetaObject();
}

bool IMetaObjectRecordDataFolderMutableRef::OnAfterCloseMetaObject()
{
	if (!(*m_propertyAttributeCode)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeDescription)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeParent)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeIsFolder)->OnAfterCloseMetaObject())
		return false;

	return IMetaObjectRecordDataMutableRef::OnAfterCloseMetaObject();
}

//////////////////////////////////////////////////////////////////////

bool IMetaObjectRecordDataFolderMutableRef::ProcessChoice(IBackendControlFrame* ownerValue, const wxString& strFormName, eSelectMode selMode)
{
	if (ownerValue == nullptr)
		return false;

	IBackendValueForm* selectChoiceForm = nullptr;

	if (selectChoiceForm == nullptr && selMode == eSelectMode::eSelectMode_Items || selMode == eSelectMode::eSelectMode_FoldersAndItems) {
		selectChoiceForm = GetSelectForm(strFormName, ownerValue);
	}
	else if (selectChoiceForm == nullptr && selMode == eSelectMode::eSelectMode_Folders) {
		selectChoiceForm = GetFolderSelectForm(strFormName, ownerValue);
	}

	if (selectChoiceForm == nullptr)
		return false;

	selectChoiceForm->ShowForm();
	return true;
}

//////////////////////////////////////////////////////////////////////

IRecordDataObjectRef* IMetaObjectRecordDataFolderMutableRef::CreateObjectRefValue(const CGuid& objGuid)
{
	return CreateObjectRefValue(eObjectMode::OBJECT_ITEM, objGuid);
}

//***********************************************************************
//*                      IMetaObjectRegisterData						*
//***********************************************************************

IMetaObjectRegisterData::IMetaObjectRegisterData() : IMetaObjectGenericData()
{
}

IMetaObjectRegisterData::~IMetaObjectRegisterData()
{
	//wxDELETE((*m_propertyAttributeLineActive));
	//wxDELETE((*m_propertyAttributePeriod));
	//wxDELETE((*m_propertyAttributeRecorder));
	//wxDELETE((*m_propertyAttributeLineNumber));
}

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool IMetaObjectRegisterData::LoadData(CMemoryReader& dataReader)
{
	//load default attributes:
	(*m_propertyAttributeLineActive)->LoadMeta(dataReader);
	(*m_propertyAttributePeriod)->LoadMeta(dataReader);
	(*m_propertyAttributeRecorder)->LoadMeta(dataReader);
	(*m_propertyAttributeLineNumber)->LoadMeta(dataReader);

	return IMetaObjectGenericData::LoadData(dataReader);
}

bool IMetaObjectRegisterData::SaveData(CMemoryWriter& dataWritter)
{
	//save default attributes:
	(*m_propertyAttributeLineActive)->SaveMeta(dataWritter);
	(*m_propertyAttributePeriod)->SaveMeta(dataWritter);
	(*m_propertyAttributeRecorder)->SaveMeta(dataWritter);
	(*m_propertyAttributeLineNumber)->SaveMeta(dataWritter);

	//create or update table:
	return IMetaObjectGenericData::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool IMetaObjectRegisterData::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IMetaObject::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyAttributeLineActive)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyAttributePeriod)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyAttributeRecorder)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyAttributeLineNumber)->OnCreateMetaObject(metaData, flags);
}

bool IMetaObjectRegisterData::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyAttributeLineActive)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyAttributePeriod)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyAttributeRecorder)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyAttributeLineNumber)->OnLoadMetaObject(metaData))
		return false;

	return IMetaObject::OnLoadMetaObject(metaData);
}

bool IMetaObjectRegisterData::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyAttributeLineActive)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyAttributePeriod)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeRecorder)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeLineNumber)->OnSaveMetaObject(flags))
		return false;

	return IMetaObject::OnSaveMetaObject(flags);
}

bool IMetaObjectRegisterData::OnDeleteMetaObject()
{
	if (!(*m_propertyAttributeLineActive)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyAttributePeriod)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyAttributeRecorder)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyAttributeLineNumber)->OnDeleteMetaObject())
		return false;

	return IMetaObject::OnDeleteMetaObject();
}

bool IMetaObjectRegisterData::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyAttributeLineActive)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributePeriod)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeRecorder)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeLineNumber)->OnBeforeRunMetaObject(flags))
		return false;

	registerManager();
	registerRegList();
	registerRecordKey();
	registerRecordSet();
	registerRecordSet_String();

	registerRecordManager();

	return IMetaObject::OnBeforeRunMetaObject(flags);
}

bool IMetaObjectRegisterData::OnAfterCloseMetaObject()
{
	if (!(*m_propertyAttributeLineActive)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyAttributePeriod)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeRecorder)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeLineNumber)->OnAfterCloseMetaObject())
		return false;

	unregisterManager();
	unregisterRegList();
	unregisterRecordKey();
	unregisterRecordSet();
	unregisterRecordSet_String();

	unregisterRecordManager();

	return IMetaObject::OnAfterCloseMetaObject();
}

//***********************************************************************
//*								ARRAY									*
//***********************************************************************

CRecordKeyObject* IMetaObjectRegisterData::CreateRecordKeyObjectValue()
{
	return CValue::CreateAndPrepareValueRef<CRecordKeyObject>(this);
}

IRecordSetObject* IMetaObjectRegisterData::CreateRecordSetObjectValue(bool needInitialize)
{
	IRecordSetObject* createdValue = CreateRecordSetObjectRegValue();
	if (!needInitialize)
		return createdValue;
	if (createdValue && !createdValue->InitializeObject(nullptr, true)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IRecordSetObject* IMetaObjectRegisterData::CreateRecordSetObjectValue(const CUniquePairKey& uniqueKey, bool needInitialize)
{
	IRecordSetObject* createdValue = CreateRecordSetObjectRegValue(uniqueKey);
	if (!needInitialize)
		return createdValue;
	if (createdValue && !createdValue->InitializeObject(nullptr, false)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IRecordSetObject* IMetaObjectRegisterData::CreateRecordSetObjectValue(IRecordSetObject* source, bool needInitialize)
{
	IRecordSetObject* createdValue = CreateRecordSetObjectRegValue();
	if (!needInitialize)
		return createdValue;
	if (createdValue && !createdValue->InitializeObject(source, true)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IRecordSetObject* IMetaObjectRegisterData::CopyRecordSetObjectValue(const CUniquePairKey& uniqueKey)
{
	IRecordSetObject* createdValue = CreateRecordSetObjectRegValue(uniqueKey);
	if (createdValue && !createdValue->InitializeObject(nullptr, true)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IRecordManagerObject* IMetaObjectRegisterData::CreateRecordManagerObjectValue()
{
	IRecordManagerObject* createdValue = CreateRecordManagerObjectRegValue();
	if (createdValue && !createdValue->InitializeObject(nullptr, true)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IRecordManagerObject* IMetaObjectRegisterData::CreateRecordManagerObjectValue(const CUniquePairKey& uniqueKey)
{
	IRecordManagerObject* createdValue = CreateRecordManagerObjectRegValue(uniqueKey);
	if (createdValue && !createdValue->InitializeObject(nullptr, false)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IRecordManagerObject* IMetaObjectRegisterData::CreateRecordManagerObjectValue(IRecordManagerObject* source)
{
	IRecordManagerObject* createdValue = CreateRecordManagerObjectRegValue();
	if (createdValue && !createdValue->InitializeObject(source, true)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IRecordManagerObject* IMetaObjectRegisterData::CopyRecordManagerObjectValue(const CUniquePairKey& uniqueKey)
{
	IRecordManagerObject* createdValue = CreateRecordManagerObjectRegValue();
	if (createdValue && !createdValue->InitializeObject(uniqueKey)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

//***********************************************************************
//*                        ISourceDataObject							*
//***********************************************************************

//***********************************************************************
//*                        IRecordDataObject							*
//***********************************************************************

wxIMPLEMENT_ABSTRACT_CLASS(IRecordDataObject, CValue);

IRecordDataObject::IRecordDataObject(const CGuid& objGuid, bool newObject) :
	CValue(eValueTypes::TYPE_VALUE), IValueDataObject(objGuid, newObject),
	m_methodHelper(new CMethodHelper())
{
}

IRecordDataObject::IRecordDataObject(const IRecordDataObject& source) :
	CValue(eValueTypes::TYPE_VALUE), IValueDataObject(wxNewUniqueGuid, true),
	m_methodHelper(new CMethodHelper())
{
}

IRecordDataObject::~IRecordDataObject()
{
	wxDELETE(m_methodHelper);
}

IBackendValueForm* IRecordDataObject::GetForm() const
{
	if (!m_objGuid.isValid())
		return nullptr;
	return IBackendValueForm::FindFormByUniqueKey(m_objGuid);
}

class_identifier_t IRecordDataObject::GetClassType() const
{
	IMetaObjectRecordData* metaObject = GetMetaObject();
	wxASSERT(metaObject);
	const IMetaValueTypeCtor* clsFactory =
		metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Object);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString IRecordDataObject::GetClassName() const
{
	IMetaObjectRecordData* metaObject = GetMetaObject();
	wxASSERT(metaObject);
	const IMetaValueTypeCtor* clsFactory =
		metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Object);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString IRecordDataObject::GetString() const
{
	IMetaObjectRecordData* metaObject = GetMetaObject();
	wxASSERT(metaObject);
	const IMetaValueTypeCtor* clsFactory =
		metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Object);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

CSourceExplorer IRecordDataObject::GetSourceExplorer() const
{
	IMetaObjectRecordData* metaObject = GetMetaObject();

	CSourceExplorer srcHelper(
		metaObject, GetClassType(),
		false
	);

	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		srcHelper.AppendSource(object);
	}

	for (const auto object : metaObject->GetTableArrayObject()) {
		srcHelper.AppendSource(object);
	}

	return srcHelper;
}

#include "backend/metaCollection/partial/tabularSection/tabularSection.h"

bool IRecordDataObject::GetModel(IValueModel*& tableValue, const meta_identifier_t& id)
{
	auto& it = m_listObjectValue.find(id);
	if (it != m_listObjectValue.end()) {
		const CValue& cTabularSection = it->second;
		return cTabularSection.ConvertToValue(tableValue);
	};

	tableValue = nullptr;
	return false;
}

bool IRecordDataObject::SetValueByMetaID(const meta_identifier_t& id, const CValue& varMetaVal)
{
	auto& it = m_listObjectValue.find(id);
	wxASSERT(it != m_listObjectValue.end());
	if (it != m_listObjectValue.end()) {

		IMetaObjectRecordData* metaObjectValue = GetMetaObject();
		wxASSERT(metaObjectValue);

		const IMetaObjectAttribute* attribute = metaObjectValue->FindAnyAttributeObjectByFilter(id);
		wxASSERT(attribute);
		it->second = attribute->AdjustValue(varMetaVal);
		return true;
	}
	return false;
}

bool IRecordDataObject::GetValueByMetaID(const meta_identifier_t& id, CValue& pvarMetaVal) const
{
	auto& it = m_listObjectValue.find(id);
	wxASSERT(it != m_listObjectValue.end());
	if (it != m_listObjectValue.end()) {
		pvarMetaVal = it->second;
		return true;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

IValueTable* IRecordDataObject::GetTableByMetaID(const meta_identifier_t& id) const
{
	const CValue& cTable = GetValueByMetaID(id); IValueTable* retTable = nullptr;
	if (cTable.ConvertToValue(retTable))
		return retTable;
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

#define thisObject wxT("thisObject")

void IRecordDataObject::PrepareEmptyObject()
{
	IMetaObjectRecordData* metaObject = GetMetaObject();
	wxASSERT(metaObject);

	m_listObjectValue.clear();

	//attrbutes can refValue 
	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		if (object->IsDeleted())
			continue;
		m_listObjectValue.insert_or_assign(object->GetMetaID(), object->CreateValue());
	}

	// table is collection values 
	for (const auto object : metaObject->GetTableArrayObject()) {
		if (object->IsDeleted())
			continue;
		m_listObjectValue.insert_or_assign(object->GetMetaID(), CValue::CreateAndPrepareValueRef<CTabularSectionDataObject>(this, object));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void IRecordDataObject::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	m_methodHelper->AppendFunc("getFormObject", 2, "getFormObject(string, owner)");
	m_methodHelper->AppendFunc("getMetadata", "getMetadata()");

	m_methodHelper->AppendProp(thisObject,
		true, false, eThisObject, eSystem
	);

	IMetaObjectRecordData* metaObject = GetMetaObject();
	wxASSERT(metaObject);

	if (metaObject != nullptr) {

		wxString objectName;

		//fill custom attributes 
		for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
			if (object->IsDeleted())
				continue;
			if (!object->GetObjectNameAsString(objectName))
				continue;
			m_methodHelper->AppendProp(
				objectName,
				object->GetMetaID(),
				eProperty
			);
		}

		//fill custom tables 
		for (const auto object : metaObject->GetTableArrayObject()) {
			if (object->IsDeleted())
				continue;
			if (!object->GetObjectNameAsString(objectName))
				continue;
			m_methodHelper->AppendProp(
				objectName,
				true,
				false,
				object->GetMetaID(),
				eTable
			);
		}
	}

	if (m_procUnit != nullptr) {
		CByteCode* byteCode = m_procUnit->GetByteCode();
		if (byteCode != nullptr) {
			for (auto exportFunction : byteCode->m_listExportFunc) {
				m_methodHelper->AppendMethod(
					exportFunction.first,
					byteCode->GetNParams(exportFunction.second),
					byteCode->HasRetVal(exportFunction.second),
					exportFunction.second,
					eProcUnit
				);
			}
			for (auto exportVariable : byteCode->m_listExportVar) {
				m_methodHelper->AppendProp(
					exportVariable.first,
					exportVariable.second,
					eProcUnit
				);
			}
		}
	}
}

bool IRecordDataObject::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	const long lPropAlias = m_methodHelper->GetPropAlias(lPropNum);
	if (lPropAlias == eProcUnit) {
		if (m_procUnit != nullptr) {
			return m_procUnit->SetPropVal(
				GetPropName(lPropNum), varPropVal
			);
		}
	}
	else if (lPropAlias == eProperty) {
		return SetValueByMetaID(
			m_methodHelper->GetPropData(lPropNum),
			varPropVal
		);
	}
	return false;
}

bool IRecordDataObject::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const long lPropAlias = m_methodHelper->GetPropAlias(lPropNum);
	if (lPropAlias == eProcUnit) {
		if (m_procUnit != nullptr) {
			return m_procUnit->GetPropVal(
				GetPropName(lPropNum), pvarPropVal
			);
		}
	}
	else if (lPropAlias == eProperty || lPropAlias == eTable) {
		return GetValueByMetaID(
			m_methodHelper->GetPropData(lPropNum), pvarPropVal
		);
	}
	else if (lPropAlias == eSystem) {
		switch (m_methodHelper->GetPropData(lPropNum))
		{
		case eThisObject:
			pvarPropVal = GetValue();
			return true;
		}
	}
	return false;
}

bool IRecordDataObject::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
{
	const long lMethodAlias = m_methodHelper->GetPropAlias(lMethodNum);
	if (lMethodAlias == eProcUnit) {
		return IModuleDataObject::ExecuteProc(
			GetMethodName(lMethodNum), paParams, lSizeArray
		);
	}

	return false;
}

bool IRecordDataObject::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	const long lMethodAlias = m_methodHelper->GetPropAlias(lMethodNum);
	if (lMethodAlias == eProcUnit) {
		return IModuleDataObject::ExecuteFunc(
			GetMethodName(lMethodNum), pvarRetValue, paParams, lSizeArray
		);
	}

	switch (lMethodNum)
	{
	case eGetFormObject:
		pvarRetValue = GetFormValue(
			lSizeArray > 0 ? paParams[0]->GetString() : wxEmptyString,
			lSizeArray > 1 ? paParams[1]->ConvertToType<IBackendControlFrame>() : nullptr
		);
		return true;
	case eGetMetadata:
		pvarRetValue = GetMetaObject();
		return true;
	}

	return false;
}

//***********************************************************************
//*                        IRecordDataObjectExt							*           
//***********************************************************************

wxIMPLEMENT_ABSTRACT_CLASS(IRecordDataObjectExt, IRecordDataObject);

IRecordDataObjectExt::IRecordDataObjectExt(IMetaObjectRecordDataExt* metaObject) :
	IRecordDataObject(wxNewUniqueGuid, true), m_metaObject(metaObject)
{
}

IRecordDataObjectExt::IRecordDataObjectExt(const IRecordDataObjectExt& source) :
	IRecordDataObject(source), m_metaObject(source.m_metaObject)
{
}

IRecordDataObjectExt::~IRecordDataObjectExt()
{
	if (m_metaObject->IsExternalCreate()) {
		if (!appData->DesignerMode()) {
			IMetaData* metaData = m_metaObject->GetMetaData();
			if (!metaData->CloseDatabase(forceCloseFlag)) {
				wxASSERT_MSG(false, "m_moduleManager->CloseDatabase() == false");
			}
			wxDELETE(metaData);
		}
	}
}

bool IRecordDataObjectExt::InitializeObject()
{
	if (!m_metaObject->IsExternalCreate()) {
		IMetaData* metaData = m_metaObject->GetMetaData();
		wxASSERT(metaData);
		IModuleManager* moduleManager = metaData->GetModuleManager();
		wxASSERT(moduleManager);

		if (!m_compileModule) {
			m_compileModule = new CCompileModule(m_metaObject->GetModuleObject());
			m_compileModule->SetParent(moduleManager->GetCompileModule());
			m_compileModule->AddContextVariable(thisObject, this);
		}

		if (!appData->DesignerMode()) {
			try {
				m_compileModule->Compile();
			}
			catch (const CBackendException* err)
			{
				if (!appData->DesignerMode()) {
					CSystemFunction::Raise(err->what());
				}

				return false;
			};

			m_procUnit = new CProcUnit();
			m_procUnit->SetParent(moduleManager->GetProcUnit());
		}
	}

	PrepareEmptyObject();

	if (!m_metaObject->IsExternalCreate()) {
		if (!appData->DesignerMode()) {
			m_procUnit->Execute(m_compileModule->m_cByteCode);
		}
	}

	PrepareNames();

	//is Ok
	return true;
}

bool IRecordDataObjectExt::InitializeObject(IRecordDataObjectExt* source)
{
	if (!m_metaObject->IsExternalCreate()) {
		IMetaData* metaData = m_metaObject->GetMetaData();
		wxASSERT(metaData);
		IModuleManager* moduleManager = metaData->GetModuleManager();
		wxASSERT(moduleManager);

		if (m_compileModule == nullptr) {
			m_compileModule = new CCompileModule(m_metaObject->GetModuleObject());
			m_compileModule->SetParent(moduleManager->GetCompileModule());
			m_compileModule->AddContextVariable(thisObject, this);
		}

		if (!appData->DesignerMode()) {
			try {
				m_compileModule->Compile();
			}
			catch (const CBackendException* err) {
				if (!appData->DesignerMode()) {
					CSystemFunction::Raise(err->what());
				}

				return false;
			};

			m_procUnit = new CProcUnit();
			m_procUnit->SetParent(moduleManager->GetProcUnit());
		}
	}

	PrepareEmptyObject();

	if (!m_metaObject->IsExternalCreate()) {
		if (!appData->DesignerMode()) {
			m_procUnit->Execute(m_compileModule->m_cByteCode);
		}
	}

	PrepareNames();

	//is Ok
	return true;
}

IRecordDataObjectExt* IRecordDataObjectExt::CopyObjectValue()
{
	return m_metaObject->CreateObjectValue(this);
}

//***********************************************************************
//*                        IRecordDataObjectRef							*           
//***********************************************************************

wxIMPLEMENT_ABSTRACT_CLASS(IRecordDataObjectRef, IRecordDataObject);

IRecordDataObjectRef::IRecordDataObjectRef(IMetaObjectRecordDataMutableRef* metaObject, const CGuid& objGuid) :
	IRecordDataObject(objGuid.isValid() ? objGuid : CGuid::newGuid(GUID_TIME_BASED), !objGuid.isValid()),
	m_metaObject(metaObject),
	m_reference_impl(nullptr),
	m_objModified(false)
{
	if (m_metaObject != nullptr)
		m_reference_impl = new reference_t(m_metaObject->GetMetaID(), m_objGuid);
}

IRecordDataObjectRef::IRecordDataObjectRef(const IRecordDataObjectRef& src) :
	IRecordDataObject(src),
	m_metaObject(src.m_metaObject),
	m_reference_impl(nullptr),
	m_objModified(false)
{
	if (m_metaObject != nullptr)
		m_reference_impl = new reference_t(m_metaObject->GetMetaID(), m_objGuid);
}

IRecordDataObjectRef::~IRecordDataObjectRef()
{
	wxDELETE(m_reference_impl);
}

bool IRecordDataObjectRef::InitializeObject(const CGuid& copyGuid)
{
	IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);
	IModuleManager* moduleManager = metaData->GetModuleManager();
	wxASSERT(moduleManager);
	if (m_compileModule == nullptr) {
		m_compileModule = new CCompileModule(m_metaObject->GetModuleObject());
		m_compileModule->SetParent(moduleManager->GetCompileModule());
		m_compileModule->AddContextVariable(thisObject, this);
	}
	if (!appData->DesignerMode()) {
		try {
			m_compileModule->Compile();
		}
		catch (const CBackendException* err) {
			if (!appData->DesignerMode()) {
				CSystemFunction::Raise(err->what());
			}
			return false;
		};
	}
	bool succes = true;
	if (!appData->DesignerMode()) {
		if (m_newObject && !copyGuid.isValid()) {
			PrepareEmptyObject();
		}
		else if (m_newObject && copyGuid.isValid()) {
			succes = ReadData(copyGuid);
			if (succes) {
				IMetaObjectAttribute* codeAttribute = m_metaObject->GetAttributeForCode();
				wxASSERT(codeAttribute);
				m_listObjectValue[codeAttribute->GetMetaID()] = codeAttribute->CreateValue();
			}
			m_objModified = true;
		}
		else {
			if (!ReadData()) PrepareEmptyObject();
		}
		if (!succes) return succes;
	}
	else {
		PrepareEmptyObject();
	}
	if (!appData->DesignerMode()) {
		wxASSERT(m_procUnit == nullptr);
		m_procUnit = new CProcUnit();
		m_procUnit->SetParent(moduleManager->GetProcUnit());
		m_procUnit->Execute(m_compileModule->m_cByteCode);
		if (m_newObject) {
			succes = Filling();
		}
	}

	PrepareNames();

	//is Ok
	return succes;
}

bool IRecordDataObjectRef::InitializeObject(IRecordDataObjectRef* source, bool generate)
{
	IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);
	IModuleManager* moduleManager = metaData->GetModuleManager();
	wxASSERT(moduleManager);
	if (m_compileModule == nullptr) {
		m_compileModule = new CCompileModule(m_metaObject->GetModuleObject());
		m_compileModule->SetParent(moduleManager->GetCompileModule());
		m_compileModule->AddContextVariable(thisObject, this);
	}
	if (!appData->DesignerMode()) {
		try {
			m_compileModule->Compile();
		}
		catch (const CBackendException* err) {
			if (!appData->DesignerMode()) {
				CSystemFunction::Raise(err->what());
			}

			return false;
		};
	}

	CValuePtr<CReferenceDataObject> reference(
		source != nullptr ? source->GetReference() : nullptr
	);

	if (!generate && source != nullptr)
		PrepareEmptyObject(source);
	else
		PrepareEmptyObject();

	bool succes = true;
	if (!appData->DesignerMode()) {
		wxASSERT(m_procUnit == nullptr);
		m_procUnit = new CProcUnit();
		m_procUnit->SetParent(moduleManager->GetProcUnit());
		m_procUnit->Execute(m_compileModule->m_cByteCode);
		if (m_newObject && source != nullptr && !generate) {
			m_procUnit->CallAsProc(wxT("OnCopy"), source->GetValue());
		}
		else if (m_newObject && source == nullptr) {
			succes = Filling();
		}
		else if (generate) {
			succes = Filling(reference->GetValue());
		}
	}

	PrepareNames();

	//is Ok
	return succes;
}

class_identifier_t IRecordDataObjectRef::GetClassType() const
{
	return IRecordDataObject::GetClassType();
}

wxString IRecordDataObjectRef::GetClassName() const
{
	return IRecordDataObject::GetClassName();
}

wxString IRecordDataObjectRef::GetString() const
{
	return m_metaObject->GetDataPresentation(this);
}

CSourceExplorer IRecordDataObjectRef::GetSourceExplorer() const
{
	CSourceExplorer srcHelper(
		m_metaObject, GetClassType(),
		false
	);

	IMetaObjectAttribute* attribute = m_metaObject->GetAttributeForCode();

	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		if (!m_metaObject->IsDataReference(object->GetMetaID())) {
			srcHelper.AppendSource(object, object != attribute);
		}
	}

	for (const auto object : m_metaObject->GetTableArrayObject()) {
		srcHelper.AppendSource(object);
	}

	return srcHelper;
}

bool IRecordDataObjectRef::GetModel(IValueModel*& tableValue, const meta_identifier_t& id)
{
	auto& it = m_listObjectValue.find(id);
	if (it != m_listObjectValue.end()) {
		const CValue& cTabularSection = it->second;
		return cTabularSection.ConvertToValue(tableValue);
	};
	tableValue = nullptr;
	return false;
}

void IRecordDataObjectRef::Modify(bool mod)
{
	IBackendValueForm* const foundedForm = IBackendValueForm::FindFormByUniqueKey(m_objGuid);

	if (foundedForm != nullptr)
		foundedForm->Modify(mod);

	m_objModified = mod;
}

bool IRecordDataObjectRef::Generate()
{
	if (m_newObject)
		return false;

	IBackendValueForm* const foundedForm = IBackendValueForm::FindFormByUniqueKey(m_objGuid);
	if (foundedForm != nullptr)
		return foundedForm->GenerateForm(this);

	return false;
}

bool IRecordDataObjectRef::Filling(CValue& cValue) const
{
	CValue standartProcessing = true;
	if (m_procUnit != nullptr) {
		m_procUnit->CallAsProc(wxT("Filling"), cValue, standartProcessing);
	}
	return standartProcessing.GetBoolean();
}

bool IRecordDataObjectRef::SetValueByMetaID(const meta_identifier_t& id, const CValue& varMetaVal)
{
	if (varMetaVal != IRecordDataObject::GetValueByMetaID(id)) {
		if (IRecordDataObject::SetValueByMetaID(id, varMetaVal)) {
			IRecordDataObjectRef::Modify(true);
			return true;
		}
		return false;
	}
	return true;
}

bool IRecordDataObjectRef::GetValueByMetaID(const meta_identifier_t& id, CValue& pvarMetaVal) const
{
	return IRecordDataObject::GetValueByMetaID(id, pvarMetaVal);
}

IRecordDataObjectRef* IRecordDataObjectRef::CopyObjectValue()
{
	return m_metaObject->CreateObjectValue(this);
}

void IRecordDataObjectRef::PrepareEmptyObject()
{
	m_listObjectValue.clear();
	//attrbutes can refValue 
	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		if (object->IsDeleted())
			continue;
		if (!m_metaObject->IsDataReference(object->GetMetaID())) {
			m_listObjectValue.insert_or_assign(object->GetMetaID(), object->CreateValue());
		}
	}
	// table is collection values 
	for (const auto object : m_metaObject->GetTableArrayObject()) {
		if (object->IsDeleted())
			continue;
		m_listObjectValue.insert_or_assign(object->GetMetaID(), CValue::CreateAndPrepareValueRef<CTabularSectionDataObjectRef>(this, object));
	}
	m_objModified = true;
}

void IRecordDataObjectRef::PrepareEmptyObject(const IRecordDataObjectRef* source)
{
	m_listObjectValue.clear();

	IMetaObjectAttribute* codeAttribute = m_metaObject->GetAttributeForCode();
	wxASSERT(codeAttribute);

	//attributes can refValue 
	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		if (object->IsDeleted())
			continue;
		if (object != codeAttribute && !m_metaObject->IsDataReference(object->GetMetaID())) {
			source->GetValueByMetaID(object->GetMetaID(), m_listObjectValue[object->GetMetaID()]);
		}
	}

	m_listObjectValue[codeAttribute->GetMetaID()] = codeAttribute->CreateValue();

	// table is collection values 
	for (const auto object : m_metaObject->GetTableArrayObject()) {
		if (object->IsDeleted())
			continue;
		CTabularSectionDataObjectRef* tableSection = CValue::CreateAndPrepareValueRef<CTabularSectionDataObjectRef>(this, object);
		if (tableSection->LoadDataFromTable(source->GetTableByMetaID(object->GetMetaID())))
			m_listObjectValue.insert_or_assign(object->GetMetaID(), tableSection);
		else
			wxDELETE(tableSection);
	}
	m_objModified = true;
}

CReferenceDataObject* IRecordDataObjectRef::GetReference() const
{
	if (m_newObject) {
		return CReferenceDataObject::Create(m_metaObject);
	}

	return CReferenceDataObject::Create(m_metaObject, m_objGuid);
}

//***********************************************************************
//*                        IRecordDataObjectFolderRef					*           
//***********************************************************************

wxIMPLEMENT_ABSTRACT_CLASS(IRecordDataObjectFolderRef, IRecordDataObjectRef);

IRecordDataObjectFolderRef::IRecordDataObjectFolderRef(IMetaObjectRecordDataFolderMutableRef* metaObject, const CGuid& objGuid, eObjectMode objMode)
	: IRecordDataObjectRef(metaObject, objGuid), m_objMode(objMode)
{
}

IRecordDataObjectFolderRef::IRecordDataObjectFolderRef(const IRecordDataObjectFolderRef& source)
	: IRecordDataObjectRef(source), m_objMode(source.m_objMode)
{
}

IRecordDataObjectFolderRef::~IRecordDataObjectFolderRef()
{
}

CSourceExplorer IRecordDataObjectFolderRef::GetSourceExplorer() const
{
	CSourceExplorer srcHelper(
		m_metaObject, GetClassType(),
		false
	);
	IMetaObjectAttribute* attribute = m_metaObject->GetAttributeForCode();
	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		eItemMode attrUse = object->GetItemMode();
		if (m_objMode == eObjectMode::OBJECT_ITEM) {
			if (attrUse == eItemMode::eItemMode_Item
				|| attrUse == eItemMode::eItemMode_Folder_Item) {
				if (!m_metaObject->IsDataReference(object->GetMetaID())) {
					srcHelper.AppendSource(object, object != attribute);
				}
			}
		}
		else {
			if (attrUse == eItemMode::eItemMode_Folder ||
				attrUse == eItemMode::eItemMode_Folder_Item) {
				if (!m_metaObject->IsDataReference(object->GetMetaID())) {
					srcHelper.AppendSource(object, object != attribute);
				}
			}
		}
	}

	for (const auto object : m_metaObject->GetTableArrayObject()) {
		eItemMode tableUse = object->GetTableUse();
		if (m_objMode == eObjectMode::OBJECT_ITEM) {
			if (tableUse == eItemMode::eItemMode_Item
				|| tableUse == eItemMode::eItemMode_Folder_Item) {
				srcHelper.AppendSource(object);
			}
		}
		else {
			if (tableUse == eItemMode::eItemMode_Folder ||
				tableUse == eItemMode::eItemMode_Folder_Item) {
				srcHelper.AppendSource(object);
			}
		}
	}

	return srcHelper;
}

bool IRecordDataObjectFolderRef::GetModel(IValueModel*& tableValue, const meta_identifier_t& id)
{
	auto& it = m_listObjectValue.find(id);
	if (it != m_listObjectValue.end()) {
		const CValue& cTabularSection = it->second;
		return cTabularSection.ConvertToValue(tableValue);
	};

	tableValue = nullptr;
	return false;
}

IRecordDataObjectRef* IRecordDataObjectFolderRef::CopyObjectValue()
{
	return ((IMetaObjectRecordDataFolderMutableRef*)m_metaObject)->CreateObjectValue(m_objMode, this);
}

bool IRecordDataObjectFolderRef::SetValueByMetaID(const meta_identifier_t& id, const CValue& varMetaVal)
{
	const CValue& cOldValue = IRecordDataObjectRef::GetValueByMetaID(id);
	if (cOldValue.GetType() == TYPE_NULL)
		return false;
	if (GetMetaObject()->IsDataParent(id) &&
		varMetaVal == GetReference()) {
		return false;
	}

	return IRecordDataObjectRef::SetValueByMetaID(id, varMetaVal);
}

bool IRecordDataObjectFolderRef::GetValueByMetaID(const meta_identifier_t& id, CValue& pvarMetaVal) const
{
	return IRecordDataObjectRef::GetValueByMetaID(id, pvarMetaVal);
}

void IRecordDataObjectFolderRef::PrepareEmptyObject()
{
	m_listObjectValue.clear();
	//attrbutes can refValue 
	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		if (object->IsDeleted())
			continue;
		eItemMode attrUse = object->GetItemMode();
		if (m_objMode == eObjectMode::OBJECT_ITEM) {
			if (attrUse == eItemMode::eItemMode_Item ||
				attrUse == eItemMode::eItemMode_Folder_Item) {
				m_listObjectValue.insert_or_assign(object->GetMetaID(), object->CreateValue());
			}
			else {
				m_listObjectValue.insert_or_assign(object->GetMetaID(), eValueTypes::TYPE_NULL);
			}
		}
		else {
			if (attrUse == eItemMode::eItemMode_Folder ||
				attrUse == eItemMode::eItemMode_Folder_Item) {
				m_listObjectValue.insert_or_assign(object->GetMetaID(), object->CreateValue());
			}
			else {
				m_listObjectValue.insert_or_assign(object->GetMetaID(), eValueTypes::TYPE_NULL);
			}
		}
	}
	IMetaObjectRecordDataFolderMutableRef* metaFolder = GetMetaObject();
	wxASSERT(metaFolder);
	if (m_objMode == eObjectMode::OBJECT_ITEM) {
		m_listObjectValue.insert_or_assign(*metaFolder->GetDataIsFolder(), false);
	}
	else if (m_objMode == eObjectMode::OBJECT_FOLDER) {
		m_listObjectValue.insert_or_assign(*metaFolder->GetDataIsFolder(), true);
	}
	// table is collection values 
	for (const auto object : m_metaObject->GetTableArrayObject()) {
		if (object->IsDeleted())
			continue;
		eItemMode tableUse = object->GetTableUse();
		if (m_objMode == eObjectMode::OBJECT_ITEM) {
			if (tableUse == eItemMode::eItemMode_Item ||
				tableUse == eItemMode::eItemMode_Folder_Item) {
				m_listObjectValue.insert_or_assign(object->GetMetaID(), CValue::CreateAndPrepareValueRef<CTabularSectionDataObjectRef>(this, object));
			}
			else {
				m_listObjectValue.insert_or_assign(object->GetMetaID(), eValueTypes::TYPE_NULL);
			}
		}
		else {
			if (tableUse == eItemMode::eItemMode_Folder ||
				tableUse == eItemMode::eItemMode_Folder_Item) {
				m_listObjectValue.insert_or_assign(object->GetMetaID(), CValue::CreateAndPrepareValueRef<CTabularSectionDataObjectRef>(this, object));
			}
			else {
				m_listObjectValue.insert_or_assign(object->GetMetaID(), eValueTypes::TYPE_NULL);
			}
		}
	}
	m_objModified = true;
}

void IRecordDataObjectFolderRef::PrepareEmptyObject(const IRecordDataObjectRef* source)
{
	m_listObjectValue.clear();
	IMetaObjectAttribute* codeAttribute = m_metaObject->GetAttributeForCode();
	wxASSERT(codeAttribute);
	m_listObjectValue[codeAttribute->GetMetaID()] = codeAttribute->CreateValue();
	//attributes can refValue 
	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		if (object->IsDeleted())
			continue;
		CMetaObjectAttribute* metaAttr = nullptr; eItemMode attrUse = eItemMode::eItemMode_Folder_Item;
		if (object->ConvertToValue(metaAttr)) {
			attrUse = metaAttr->GetItemMode();
		}
		if (object != codeAttribute && !m_metaObject->IsDataReference(object->GetMetaID())) {
			source->GetValueByMetaID(object->GetMetaID(), m_listObjectValue[object->GetMetaID()]);
		}
	}
	IMetaObjectRecordDataFolderMutableRef* metaFolder = GetMetaObject();
	wxASSERT(metaFolder);
	if (m_objMode == eObjectMode::OBJECT_ITEM) {
		m_listObjectValue.insert_or_assign(*metaFolder->GetDataIsFolder(), false);
	}
	else if (m_objMode == eObjectMode::OBJECT_FOLDER) {
		m_listObjectValue.insert_or_assign(*metaFolder->GetDataIsFolder(), true);
	}
	// table is collection values 
	for (const auto object : m_metaObject->GetTableArrayObject()) {
		if (object->IsDeleted())
			continue;
		CMetaObjectTableData* metaTable = nullptr; eItemMode tableUse = eItemMode::eItemMode_Folder_Item;
		if (object->ConvertToValue(metaTable))
			tableUse = metaTable->GetTableUse();
		CTabularSectionDataObjectRef* tableSection = CValue::CreateAndPrepareValueRef <CTabularSectionDataObjectRef>(this, object);
		if (tableSection->LoadDataFromTable(source->GetTableByMetaID(object->GetMetaID())))
			m_listObjectValue.insert_or_assign(object->GetMetaID(), tableSection);
		else
			wxDELETE(tableSection);
	}
	m_objModified = true;
}

//***********************************************************************
//*						     metaData									* 
//***********************************************************************

wxIMPLEMENT_ABSTRACT_CLASS(IRecordSetObject, IValueTable);
wxIMPLEMENT_ABSTRACT_CLASS(IRecordManagerObject, CValue);

wxIMPLEMENT_ABSTRACT_CLASS(CRecordKeyObject, CValue);

wxIMPLEMENT_DYNAMIC_CLASS(IRecordSetObject::CRecordSetObjectRegisterKeyValue, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(IRecordSetObject::CRecordSetObjectRegisterKeyValue::CRecordSetObjectRegisterKeyDescriptionValue, CValue);

//***********************************************************************
//*                      Record key & set								*
//***********************************************************************

//////////////////////////////////////////////////////////////////////
//						  CRecordKeyObject							//
//////////////////////////////////////////////////////////////////////

CRecordKeyObject::CRecordKeyObject(IMetaObjectRegisterData* metaObject) : CValue(eValueTypes::TYPE_VALUE),
m_metaObject(metaObject), m_methodHelper(new CMethodHelper())
{
}

CRecordKeyObject::~CRecordKeyObject()
{
	wxDELETE(m_methodHelper);
}

bool CRecordKeyObject::IsEmpty() const
{
	for (auto value : m_keyValues) {
		const CValue& cValue = value.second;
		if (!cValue.IsEmpty())
			return false;
	}

	return true;
}

class_identifier_t CRecordKeyObject::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_RecordKey);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CRecordKeyObject::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_RecordKey);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CRecordKeyObject::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_RecordKey);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

//////////////////////////////////////////////////////////////////////
//						  IRecordManagerObject						//
//////////////////////////////////////////////////////////////////////

void IRecordManagerObject::CreateEmptyKey()
{
	m_recordSet->CreateEmptyKey();
}

bool IRecordManagerObject::InitializeObject(const IRecordManagerObject* source, bool newRecord)
{
	if (!m_recordSet->InitializeObject(source ? source->GetRecordSet() : nullptr, newRecord))
		return false;

	if (!appData->DesignerMode()) {
		if (!newRecord && !ReadData(m_objGuid)) {
			PrepareEmptyObject(source);
		}
		else if (newRecord) {
			PrepareEmptyObject(source);
		}
	}
	else {
		PrepareEmptyObject(source);
	}

	PrepareNames();

	//is Ok
	return true;
}

bool IRecordManagerObject::InitializeObject(const CUniquePairKey& key)
{
	if (!m_recordSet->InitializeObject(nullptr, true))
		return false;

	if (!appData->DesignerMode()) {
		if (ReadData(key)) {
			m_recordSet->m_selected = false; // is new 
			m_recordSet->Modify(true); // and modify
		}
		else {
			PrepareEmptyObject(nullptr);
		}
	}
	else {
		PrepareEmptyObject(nullptr);
	}

	PrepareNames();

	//is Ok
	return true;
}

IRecordManagerObject* IRecordManagerObject::CopyRegisterValue()
{
	return m_metaObject->CreateRecordManagerObjectValue(this);
}

IRecordManagerObject::IRecordManagerObject(IMetaObjectRegisterData* metaObject, const CUniquePairKey& uniqueKey) : CValue(eValueTypes::TYPE_VALUE),
m_metaObject(metaObject), m_methodHelper(new CMethodHelper()),
m_recordSet(m_metaObject->CreateRecordSetObjectValue(uniqueKey, false)), m_recordLine(nullptr),
m_objGuid(uniqueKey)
{
}

IRecordManagerObject::IRecordManagerObject(const IRecordManagerObject& source) : CValue(eValueTypes::TYPE_VALUE),
m_metaObject(source.m_metaObject), m_methodHelper(new CMethodHelper()),
m_recordSet(m_metaObject->CreateRecordSetObjectValue(source.m_recordSet, false)), m_recordLine(nullptr),
m_objGuid(source.m_metaObject)
{
}

IRecordManagerObject::~IRecordManagerObject()
{
	wxDELETE(m_methodHelper);
}

IBackendValueForm* IRecordManagerObject::GetForm() const
{
	if (!m_objGuid.isValid())
		return nullptr;
	if (m_recordSet->m_selected)
		return IBackendValueForm::FindFormByUniqueKey(m_objGuid);
	return nullptr;
}

bool IRecordManagerObject::IsEmpty() const
{
	return m_recordSet->IsEmpty();
}

CSourceExplorer IRecordManagerObject::GetSourceExplorer() const
{
	CSourceExplorer srcHelper(
		m_metaObject, GetClassType(), false
	);

	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		srcHelper.AppendSource(object);
	}

	return srcHelper;
}

bool IRecordManagerObject::GetModel(IValueModel*& tableValue, const meta_identifier_t& id)
{
	return false;
}

void IRecordManagerObject::Modify(bool mod)
{
	IBackendValueForm* const foundedForm = IBackendValueForm::FindFormByUniqueKey(m_objGuid);
	if (foundedForm != nullptr)
		foundedForm->Modify(mod);
	m_recordSet->Modify(mod);
}

bool IRecordManagerObject::IsModified() const
{
	return m_recordSet->IsModified();
}

bool IRecordManagerObject::SetValueByMetaID(const meta_identifier_t& id, const CValue& varMetaVal)
{
	if (varMetaVal != IRecordManagerObject::GetValueByMetaID(id)) {
		bool result = m_recordLine->SetValueByMetaID(id, varMetaVal);
		IRecordManagerObject::Modify(true);
		return result;
	}
	return true;
}

bool IRecordManagerObject::GetValueByMetaID(const meta_identifier_t& id, CValue& pvarMetaVal) const
{
	return m_recordLine->GetValueByMetaID(id, pvarMetaVal);
}

class_identifier_t IRecordManagerObject::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_RecordManager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString IRecordManagerObject::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_RecordManager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString IRecordManagerObject::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_RecordManager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

/////////////////////////////////////////////////////////////////////

void IRecordManagerObject::PrepareEmptyObject(const IRecordManagerObject* source)
{
	m_recordLine = nullptr;

	if (source == nullptr) {
		m_recordLine = CValue::CreateAndPrepareValueRef<IRecordSetObject::CRecordSetObjectRegisterReturnLine>(
			m_recordSet,
			m_recordSet->GetItem(
				m_recordSet->AppendRow()
			)
		);
	}
	else if (source != nullptr) {
		m_recordLine = m_recordSet->GetRowAt(
			m_recordSet->GetItem(0)
		);
	}

	m_recordSet->Modify(true);
}

//////////////////////////////////////////////////////////////////////
//						  IRecordSetObject							//
//////////////////////////////////////////////////////////////////////

void IRecordSetObject::CreateEmptyKey()
{
	m_keyValues.clear();
	for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
		if (object->IsDeleted())
			continue;
		m_keyValues.insert_or_assign(
			object->GetMetaID(), object->CreateValue()
		);
	}
}

bool IRecordSetObject::InitializeObject(const IRecordSetObject* source, bool newRecord)
{
	IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);

	IModuleManager* moduleManager = metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (m_compileModule == nullptr) {
		m_compileModule = new CCompileModule(m_metaObject->GetModuleObject());
		m_compileModule->SetParent(moduleManager->GetCompileModule());
		m_compileModule->AddContextVariable(thisObject, this);
	}

	if (!appData->DesignerMode()) {
		try {
			m_compileModule->Compile();
		}
		catch (const CBackendException* err) {
			if (!appData->DesignerMode()) {
				CSystemFunction::Raise(err->what());
			}
			return false;
		};
	}

	if (source != nullptr) {
		for (long row = 0; row < source->GetRowCount(); row++) {
			wxValueTableRow* node = source->GetViewData<wxValueTableRow>(source->GetItem(row));
			wxASSERT(node);
			IValueTable::Append(new wxValueTableRow(*node), false);
		}
	}

	if (!appData->DesignerMode()) {
		if (!newRecord) ReadData();
	}

	if (!appData->DesignerMode()) {
		wxASSERT(m_procUnit == nullptr);
		m_procUnit = new CProcUnit();
		m_procUnit->SetParent(moduleManager->GetProcUnit());
		m_procUnit->Execute(m_compileModule->m_cByteCode);
	}

	PrepareNames();

	//is Ok
	return true;
}

///////////////////////////////////////////////////////////////////////////////////

IRecordSetObject* IRecordSetObject::CopyRegisterValue()
{
	return m_metaObject->CreateRecordSetObjectValue(this);
}

///////////////////////////////////////////////////////////////////////////////////

IRecordSetObject::IRecordSetObject(IMetaObjectRegisterData* metaObject, const CUniquePairKey& uniqueKey) : IValueTable(),
m_recordColumnCollection(CValue::CreateAndPrepareValueRef<CRecordSetObjectRegisterColumnCollection>(this)), m_recordSetKeyValue(CValue::CreateAndPrepareValueRef<CRecordSetObjectRegisterKeyValue>(this)),
m_metaObject(metaObject), m_keyValues(uniqueKey.IsOk() ? uniqueKey : metaObject), m_objModified(false), m_selected(false),
m_methodHelper(new CMethodHelper())
{
}

IRecordSetObject::IRecordSetObject(const IRecordSetObject& source) : IValueTable(),
m_recordColumnCollection(CValue::CreateAndPrepareValueRef<CRecordSetObjectRegisterColumnCollection>(this)), m_recordSetKeyValue(CValue::CreateAndPrepareValueRef<CRecordSetObjectRegisterKeyValue>(this)),
m_metaObject(source.m_metaObject), m_keyValues(source.m_keyValues), m_objModified(true), m_selected(false),
m_methodHelper(new CMethodHelper())
{
	for (long row = 0; row < source.GetRowCount(); row++) {
		wxValueTableRow* node = source.GetViewData<wxValueTableRow>(source.GetItem(row));
		wxASSERT(node);
		IValueTable::Append(new wxValueTableRow(*node), false);
	}
}

IRecordSetObject::~IRecordSetObject()
{
	wxDELETE(m_methodHelper);
}

bool IRecordSetObject::GetAt(const CValue& varKeyValue, CValue& pvarValue)
{
	long index = varKeyValue.GetUInteger();
	if (index >= GetRowCount() && !appData->DesignerMode()) {
		CBackendException::Error(_("Array index out of bounds"));
		return false;
	}
	pvarValue = CValue::CreateAndPrepareValueRef<CRecordSetObjectRegisterReturnLine>(this, GetItem(index));
	return true;
}

class_identifier_t IRecordSetObject::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_RecordSet);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString IRecordSetObject::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_RecordSet);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString IRecordSetObject::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_RecordSet);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

#include "backend/system/value/valueTable.h"

bool IRecordSetObject::LoadDataFromTable(IValueTable* srcTable)
{
	IValueModelColumnCollection* colData = srcTable->GetColumnCollection();

	if (colData == nullptr)
		return false;
	wxArrayString columnName;
	for (unsigned int idx = 0; idx < colData->GetColumnCount() - 1; idx++) {
		IValueModelColumnCollection::IValueModelColumnInfo* colInfo = colData->GetColumnInfo(idx);
		wxASSERT(colInfo);
		if (m_recordColumnCollection->GetColumnByName(colInfo->GetColumnName()) != nullptr) {
			columnName.push_back(colInfo->GetColumnName());
		}
	}
	unsigned int rowCount = srcTable->GetRowCount();
	for (unsigned int row = 0; row < rowCount; row++) {
		const wxDataViewItem& srcItem = srcTable->GetItem(row);
		const wxDataViewItem& dstItem = GetItem(AppendRow());
		for (auto colName : columnName) {
			CValue cRetValue;
			if (srcTable->GetValueByMetaID(srcItem, srcTable->GetColumnIDByName(colName), cRetValue)) {
				const meta_identifier_t& id = GetColumnIDByName(colName);
				if (id != wxNOT_FOUND) SetValueByMetaID(dstItem, id, cRetValue);
			}
		}
	}

	return true;
}

IValueTable* IRecordSetObject::SaveDataToTable() const
{
	CValueTable* valueTable = CValue::CreateAndConvertObjectRef<CValueTable>();

	IValueModelColumnCollection* colData = valueTable->GetColumnCollection();
	for (unsigned int idx = 0; idx < m_recordColumnCollection->GetColumnCount() - 1; idx++) {
		IValueModelColumnCollection::IValueModelColumnInfo* colInfo = m_recordColumnCollection->GetColumnInfo(idx);
		wxASSERT(colInfo);
		IValueModelColumnCollection::IValueModelColumnInfo* newColInfo = colData->AddColumn(
			colInfo->GetColumnName(), colInfo->GetColumnType(), colInfo->GetColumnCaption(), colInfo->GetColumnWidth()
		);
		newColInfo->SetColumnID(colInfo->GetColumnID());
	}
	valueTable->PrepareNames();
	for (long row = 0; row < GetRowCount(); row++) {
		const wxDataViewItem& srcItem = GetItem(row);
		const wxDataViewItem& dstItem = valueTable->GetItem(valueTable->AppendRow());
		for (unsigned int col = 0; col < colData->GetColumnCount(); col++) {
			CValue cRetValue;
			IValueModelColumnCollection::IValueModelColumnInfo* colInfo = colData->GetColumnInfo(col);
			wxASSERT(colInfo);
			if (GetValueByMetaID(srcItem, colInfo->GetColumnID(), cRetValue)) {
				const meta_identifier_t& id = GetColumnIDByName(colInfo->GetColumnName());
				if (id != wxNOT_FOUND) valueTable->SetValueByMetaID(dstItem, id, cRetValue);
			}
		}
	}

	return valueTable;
}

bool IRecordSetObject::SetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id, const CValue& varMetaVal)
{
	if (!appData->DesignerMode()) {
		wxValueTableRow* node = GetViewData<wxValueTableRow>(item);
		if (node != nullptr) {
			const IMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(id);
			if (attribute != nullptr) {
				return node->SetValue(
					id, attribute->AdjustValue(varMetaVal), true
				);
			}
		}
	}

	return false;
}

bool IRecordSetObject::GetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id, CValue& pvarMetaVal) const
{
	if (appData->DesignerMode()) {
		const IMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(id);
		if (attribute != nullptr) {
			pvarMetaVal = attribute->CreateValue();
			return true;
		}
		return false;
	}

	wxValueTableRow* node = GetViewData<wxValueTableRow>(item);
	if (node == nullptr)
		return false;
	return node->GetValue(id, pvarMetaVal);
}

//////////////////////////////////////////////////////////////////////
//					CRecordSetObjectRegisterColumnCollection				//
//////////////////////////////////////////////////////////////////////



wxIMPLEMENT_DYNAMIC_CLASS(IRecordSetObject::CRecordSetObjectRegisterColumnCollection, IValueTable::IValueModelColumnCollection);

IRecordSetObject::CRecordSetObjectRegisterColumnCollection::CRecordSetObjectRegisterColumnCollection() :
	IValueModelColumnCollection(),
	m_ownerTable(nullptr),
	m_methodHelper(nullptr)
{
}

IRecordSetObject::CRecordSetObjectRegisterColumnCollection::CRecordSetObjectRegisterColumnCollection(IRecordSetObject* ownerTable) :
	IValueModelColumnCollection(),
	m_ownerTable(ownerTable),
	m_methodHelper(new CMethodHelper())
{
	IMetaObjectGenericData* metaObject = m_ownerTable->GetMetaObject();
	wxASSERT(metaObject);

	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		m_listColumnInfo.insert_or_assign(object->GetMetaID(),
			CValue::CreateAndPrepareValueRef<CValueRecordSetRegisterColumnInfo>(object));
	}
}

IRecordSetObject::CRecordSetObjectRegisterColumnCollection::~CRecordSetObjectRegisterColumnCollection()
{
	wxDELETE(m_methodHelper);
}

bool IRecordSetObject::CRecordSetObjectRegisterColumnCollection::SetAt(const CValue& varKeyValue, const CValue& varValue)//индекс массива должен начинаться с 0
{
	return false;
}

bool IRecordSetObject::CRecordSetObjectRegisterColumnCollection::GetAt(const CValue& varKeyValue, CValue& pvarValue) //индекс массива должен начинаться с 0
{
	unsigned int index = varKeyValue.GetUInteger();
	if ((index < 0 || index >= m_listColumnInfo.size() && !appData->DesignerMode())) {
		CBackendException::Error(_("Index goes beyond array"));
		return false;
	}

	auto it = m_listColumnInfo.begin();
	std::advance(it, index);
	pvarValue = it->second;
	return true;
}

//////////////////////////////////////////////////////////////////////
//					CValueRecordSetRegisterColumnInfo               //
//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(IRecordSetObject::CRecordSetObjectRegisterColumnCollection::CValueRecordSetRegisterColumnInfo, IValueTable::IValueModelColumnCollection::IValueModelColumnInfo);

IRecordSetObject::CRecordSetObjectRegisterColumnCollection::CValueRecordSetRegisterColumnInfo::CValueRecordSetRegisterColumnInfo() :
	IValueModelColumnInfo(), m_metaAttribute(nullptr)
{
}

IRecordSetObject::CRecordSetObjectRegisterColumnCollection::CValueRecordSetRegisterColumnInfo::CValueRecordSetRegisterColumnInfo(IMetaObjectAttribute* attribute) :
	IValueModelColumnInfo(), m_metaAttribute(attribute)
{
}

IRecordSetObject::CRecordSetObjectRegisterColumnCollection::CValueRecordSetRegisterColumnInfo::~CValueRecordSetRegisterColumnInfo()
{
}

//////////////////////////////////////////////////////////////////////
//					 CRecordSetObjectRegisterReturnLine					//
//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(IRecordSetObject::CRecordSetObjectRegisterReturnLine, IValueTable::IValueModelReturnLine);

IRecordSetObject::CRecordSetObjectRegisterReturnLine::CRecordSetObjectRegisterReturnLine(IRecordSetObject* ownerTable, const wxDataViewItem& line)
	: IValueModelReturnLine(line), m_ownerTable(ownerTable), m_methodHelper(new CMethodHelper())
{
}

IRecordSetObject::CRecordSetObjectRegisterReturnLine::~CRecordSetObjectRegisterReturnLine()
{
	wxDELETE(m_methodHelper);
}

void IRecordSetObject::CRecordSetObjectRegisterReturnLine::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	const IMetaObjectGenericData* metaObject = m_ownerTable->GetMetaObject();
	if (metaObject != nullptr) {
		wxString objectName;
		for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
			if (object->IsDeleted())
				continue;
			if (!object->GetObjectNameAsString(objectName))
				continue;
			m_methodHelper->AppendProp(
				objectName,
				object->GetMetaID()
			);
		}
	}
}

//////////////////////////////////////////////////////////////////////
//				       CRecordSetObjectRegisterKeyValue					//
//////////////////////////////////////////////////////////////////////

IRecordSetObject::CRecordSetObjectRegisterKeyValue::CRecordSetObjectRegisterKeyValue(IRecordSetObject* recordSet) : CValue(eValueTypes::TYPE_VALUE, true),
m_methodHelper(new CMethodHelper()), m_recordSet(recordSet)
{
}

IRecordSetObject::CRecordSetObjectRegisterKeyValue::~CRecordSetObjectRegisterKeyValue()
{
	wxDELETE(m_methodHelper);
}

//////////////////////////////////////////////////////////////////////
//						CRecordSetObjectRegisterKeyDescriptionValue		//
//////////////////////////////////////////////////////////////////////

IRecordSetObject::CRecordSetObjectRegisterKeyValue::CRecordSetObjectRegisterKeyDescriptionValue::CRecordSetObjectRegisterKeyDescriptionValue(IRecordSetObject* recordSet, const meta_identifier_t& id) : CValue(eValueTypes::TYPE_VALUE),
m_methodHelper(new CMethodHelper()), m_recordSet(recordSet), m_metaId(id)
{
}

IRecordSetObject::CRecordSetObjectRegisterKeyValue::CRecordSetObjectRegisterKeyDescriptionValue::~CRecordSetObjectRegisterKeyDescriptionValue()
{
	wxDELETE(m_methodHelper);
}

//////////////////////////////////////////////////////////////////////////////////////

long IRecordSetObject::AppendRow(unsigned int before)
{
	wxValueTableRow* rowData = new wxValueTableRow();

	IMetaObjectRegisterData* metaObject = GetMetaObject();
	wxASSERT(metaObject);
	IMetaData* metaData = metaObject->GetMetaData();
	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		rowData->AppendTableValue(object->GetMetaID(), object->CreateValue());
	}

	if (before > 0)
		return IValueTable::Insert(rowData, before, !CBackendException::IsEvalMode());

	return IValueTable::Append(rowData, !CBackendException::IsEvalMode());
}

enum Func
{
	enAdd = 0,
	enCount,
	enClear,
	enLoad,
	enUnload,
	enWrite,
	enModified,
	enRead,
	enSelected,
	enGetMetadata,
};

enum
{
	enEmpty,
	enMetadata,
};

enum
{
	enSet,
};

//****************************************************************************
//*                              Override attribute                          *
//****************************************************************************

bool CRecordKeyObject::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	return false;
}

bool CRecordKeyObject::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////

bool IRecordSetObject::CRecordSetObjectRegisterReturnLine::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	const meta_identifier_t& id = m_methodHelper->GetPropData(lPropNum);
	if (id != wxNOT_FOUND)
		return SetValueByMetaID(id, varPropVal);
	return false;
}

bool IRecordSetObject::CRecordSetObjectRegisterReturnLine::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const meta_identifier_t& id = m_methodHelper->GetPropData(lPropNum);
	if (id != wxNOT_FOUND) {
		return GetValueByMetaID(id, pvarPropVal);
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////

class_identifier_t IRecordSetObject::CRecordSetObjectRegisterReturnLine::GetClassType() const
{
	const IMetaObject* metaTable = m_ownerTable->GetMetaObject();
	const IMetaData* metaData = metaTable->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(metaTable, eCtorMetaType::eCtorMetaType_RecordSet_String);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString IRecordSetObject::CRecordSetObjectRegisterReturnLine::GetClassName() const
{
	const IMetaObject* metaTable = m_ownerTable->GetMetaObject();
	const IMetaData* metaData = metaTable->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(metaTable, eCtorMetaType::eCtorMetaType_RecordSet_String);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString IRecordSetObject::CRecordSetObjectRegisterReturnLine::GetString() const
{
	const IMetaObject* metaTable = m_ownerTable->GetMetaObject();
	const IMetaData* metaData = metaTable->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(metaTable, eCtorMetaType::eCtorMetaType_RecordSet_String);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

////////////////////////////////////////////////////////////////////////////

bool IRecordSetObject::CRecordSetObjectRegisterKeyValue::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	return false;
}

bool IRecordSetObject::CRecordSetObjectRegisterKeyValue::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const meta_identifier_t& id = m_methodHelper->GetPropData(lPropNum);
	if (id != wxNOT_FOUND) {
		pvarPropVal = CValue::CreateAndPrepareValueRef<CRecordSetObjectRegisterKeyDescriptionValue>(m_recordSet, id);
		return true;
	}
	return false;
}

//****************************************************************************
//*                              Support methods                             *
//****************************************************************************

void CRecordKeyObject::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendFunc("isEmpty", "isEmpty()");
	m_methodHelper->AppendFunc("metaData", "metaData()");

	wxString objectName;

	//fill custom attributes 
	for (const auto object : m_metaObject->GetGenericDimentionArrayObject()) {
		if (object->IsDeleted())
			continue;
		if (!object->GetObjectNameAsString(objectName))
			continue;
		m_methodHelper->AppendProp(
			objectName,
			object->GetMetaID()
		);
	}

}

//////////////////////////////////////////////////////////////

void IRecordSetObject::CRecordSetObjectRegisterKeyValue::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	IMetaObjectRegisterData* metaObject = m_recordSet->GetMetaObject();
	if (metaObject != nullptr) {
		wxString objectName;
		for (const auto object : metaObject->GetGenericDimentionArrayObject()) {
			if (object->IsDeleted())
				continue;
			if (!object->GetObjectNameAsString(objectName))
				continue;
			m_methodHelper->AppendProp(
				objectName,
				object->GetMetaID()
			);
		}
	}
}

//////////////////////////////////////////////////////////////

void IRecordSetObject::CRecordSetObjectRegisterKeyValue::CRecordSetObjectRegisterKeyDescriptionValue::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendFunc(wxT("set"), 1, "set(value)");
	m_methodHelper->AppendProp(wxT("value"), m_metaId);
	m_methodHelper->AppendProp(wxT("use"));
}

enum Prop
{
	eValue,
	eUse
};

bool IRecordSetObject::CRecordSetObjectRegisterKeyValue::CRecordSetObjectRegisterKeyDescriptionValue::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	const IMetaObjectRegisterData* metaObject = m_recordSet->GetMetaObject();
	wxASSERT(metaObject);

	const IMetaObjectAttribute* attribute = metaObject->FindAnyAttributeObjectByFilter(m_metaId);
	wxASSERT(attribute);

	switch (lPropNum) {
	case eValue:
		m_recordSet->SetKeyValue(m_metaId, varPropVal);
		return true;
	case eUse:
		if (varPropVal.GetBoolean())
			m_recordSet->SetKeyValue(m_metaId, attribute->CreateValue());
		else
			m_recordSet->EraseKeyValue(m_metaId);
		return true;
	}

	return false;
}

bool IRecordSetObject::CRecordSetObjectRegisterKeyValue::CRecordSetObjectRegisterKeyDescriptionValue::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const IMetaObjectRegisterData* metaObject = m_recordSet->GetMetaObject();
	wxASSERT(metaObject);

	const IMetaObjectAttribute* attribute = metaObject->FindAnyAttributeObjectByFilter(m_metaId);
	wxASSERT(attribute);

	switch (lPropNum) {
	case eValue:
		if (m_recordSet->FindKeyValue(m_metaId))
			pvarPropVal = m_recordSet->GetKeyValue(m_metaId);
		else
			pvarPropVal = attribute->CreateValue();
		return true;
	case eUse:
		pvarPropVal = m_recordSet->FindKeyValue(m_metaId);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////

bool CRecordKeyObject::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enEmpty:
		pvarRetValue = IsEmpty();
		return true;
	case enMetadata:
		pvarRetValue = m_metaObject;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////

bool IRecordSetObject::CRecordSetObjectRegisterKeyValue::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	return false;
}

//////////////////////////////////////////////////////////////

bool IRecordSetObject::CRecordSetObjectRegisterKeyValue::CRecordSetObjectRegisterKeyDescriptionValue::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enSet:
		m_recordSet->SetKeyValue(m_metaId, paParams[0]);
		return true;
	}
	return false;
}


//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

SYSTEM_TYPE_REGISTER(IRecordSetObject::CRecordSetObjectRegisterColumnCollection, "recordSetRegisterColumn", string_to_clsid("VL_RSCL"));
SYSTEM_TYPE_REGISTER(IRecordSetObject::CRecordSetObjectRegisterColumnCollection::CValueRecordSetRegisterColumnInfo, "recordSetRegisterColumnInfo", string_to_clsid("VL_RSCI"));

SYSTEM_TYPE_REGISTER(IRecordSetObject::CRecordSetObjectRegisterKeyValue, "recordSetRegisterKey", string_to_clsid("VL_RSCK"));
SYSTEM_TYPE_REGISTER(IRecordSetObject::CRecordSetObjectRegisterKeyValue::CRecordSetObjectRegisterKeyDescriptionValue, "recordSetRegisterKeyDescription", string_to_clsid("VL_RDVL"));