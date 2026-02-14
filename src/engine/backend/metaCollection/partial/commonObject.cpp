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

wxIMPLEMENT_ABSTRACT_CLASS(IValueMetaObjectGenericData, IValueMetaObjectCompositeData);
wxIMPLEMENT_ABSTRACT_CLASS(IValueMetaObjectRegisterData, IValueMetaObjectGenericData);

wxIMPLEMENT_ABSTRACT_CLASS(IValueMetaObjectRecordData, IValueMetaObjectGenericData);
wxIMPLEMENT_ABSTRACT_CLASS(IValueMetaObjectRecordDataExt, IValueMetaObjectRecordData);
wxIMPLEMENT_ABSTRACT_CLASS(IValueMetaObjectRecordDataRef, IValueMetaObjectRecordData);
wxIMPLEMENT_ABSTRACT_CLASS(IValueMetaObjectRecordDataEnumRef, IValueMetaObjectRecordDataRef);
wxIMPLEMENT_ABSTRACT_CLASS(IValueMetaObjectRecordDataMutableRef, IValueMetaObjectRecordDataRef);
wxIMPLEMENT_ABSTRACT_CLASS(IValueMetaObjectRecordDataHierarchyMutableRef, IValueMetaObjectRecordDataMutableRef);

//***********************************************************************
//*							IValueMetaObjectGenericData				    *
//***********************************************************************

#pragma region _form_builder_h_
IBackendValueForm* IValueMetaObjectGenericData::GetGenericForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return CreateAndBuildForm(strFormName, defaultFormType, ownerControl, nullptr, formGuid);
}
#pragma endregion
#pragma region _form_creator_h_
IBackendValueForm* IValueMetaObjectGenericData::CreateAndBuildForm(const wxString& strFormName, const form_identifier_t& form_id, IBackendControlFrame* ownerControl, ISourceDataObject* srcObject, const CUniqueKey& formGuid)
{
#pragma region _source_guard_
	class CSourceDataObjectGuard {
	public:

		CSourceDataObjectGuard(ISourceDataObject* srcObject) : m_srcObject(srcObject) {
			if (m_srcObject != nullptr) m_srcObject->SourceIncrRef();
		}

		~CSourceDataObjectGuard() {
			if (m_srcObject != nullptr) m_srcObject->SourceDecrRef();
		}

	private:
		ISourceDataObject* m_srcObject;
	};

	CSourceDataObjectGuard sourceGuard(srcObject);
#pragma endregion

	IValueMetaObjectForm* creator = nullptr;

	if (!strFormName.IsEmpty()) {

		creator = FindFormObjectByFilter(strFormName, form_id);

		if (creator == nullptr) {
			CBackendCoreException::Error(_("Form not found '%s'"), strFormName);
			return nullptr;
		}
	}

	if (!AccessRight_Show()) {
		CBackendAccessException::Error();
		return nullptr;
	}

	IBackendValueForm* result = IBackendValueForm::FindFormByUniqueKey(ownerControl, srcObject, formGuid);

	if (result == nullptr) {

		result = IValueMetaObjectForm::CreateAndBuildForm(
			creator != nullptr ? creator : GetDefaultFormByID(form_id),
			form_id,
			ownerControl, srcObject, formGuid
		);
	}

	return result;
}

#pragma endregion
#pragma region _template_builder_h_

CValueSpreadsheetDocument* IValueMetaObjectGenericData::GetTemplate(const wxString& strTemplateName)
{
	IValueMetaObjectSpreadsheet* creator = nullptr;

	if (!strTemplateName.IsEmpty()) {

		creator = FindTemplateObjectByFilter(strTemplateName);

		if (creator == nullptr) {
			CBackendCoreException::Error(_("Template not found '%s'"), strTemplateName);
			return nullptr;
		}

		CValueSpreadsheetDocument* valueSpreadsheetDocument = 
			new CValueSpreadsheetDocument(creator->GetSpreadsheetDesc());
		
		valueSpreadsheetDocument->PrepareNames();
		return valueSpreadsheetDocument;
	}

	CBackendCoreException::Error(_("Template not found!"));
	return nullptr;
}

#pragma endregion

//***********************************************************************
//*                           IValueMetaObjectRecordData				*
//***********************************************************************

#include "backend/fileSystem/fs.h"

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool IValueMetaObjectRecordData::OnLoadMetaObject(IMetaData* metaData)
{
	return IValueMetaObject::OnLoadMetaObject(metaData);
}

bool IValueMetaObjectRecordData::OnSaveMetaObject(int flags)
{
	return IValueMetaObject::OnSaveMetaObject(flags);
}

bool IValueMetaObjectRecordData::OnDeleteMetaObject()
{
	return IValueMetaObject::OnDeleteMetaObject();
}

bool IValueMetaObjectRecordData::OnBeforeRunMetaObject(int flags)
{
	return IValueMetaObject::OnBeforeRunMetaObject(flags);
}

bool IValueMetaObjectRecordData::OnAfterCloseMetaObject()
{
	return IValueMetaObject::OnAfterCloseMetaObject();
}

//***********************************************************************
//*						IValueMetaObjectRecordDataExt					*
//***********************************************************************

IValueMetaObjectRecordDataExt::IValueMetaObjectRecordDataExt() :
	IValueMetaObjectRecordData()
{
}

IValueRecordDataObjectExt* IValueMetaObjectRecordDataExt::CreateObjectValue()
{
	IValueRecordDataObjectExt* createdValue = CreateObjectExtValue();
	if (!IsExternalCreate()) {
		if (createdValue && !createdValue->InitializeObject()) {
			wxDELETE(createdValue);
			return nullptr;
		}
	}
	return createdValue;
}

IValueRecordDataObjectExt* IValueMetaObjectRecordDataExt::CreateObjectValue(IValueRecordDataObjectExt* objSrc)
{
	IValueRecordDataObjectExt* createdValue = CreateObjectExtValue();
	if (!IsExternalCreate()) {
		if (createdValue && !createdValue->InitializeObject(objSrc)) {
			wxDELETE(createdValue);
			return nullptr;
		}
	}
	return createdValue;
}

IValueRecordDataObject* IValueMetaObjectRecordDataExt::CreateRecordDataObjectValue()
{
	return CreateObjectValue();
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool IValueMetaObjectRecordDataExt::OnBeforeRunMetaObject(int flags)
{
	if (IsExternalCreate()) {
		registerExternalObject();
		registerExternalManager();
	}
	else {
		registerObject();
		registerManager();
	}

	return IValueMetaObjectRecordData::OnBeforeRunMetaObject(flags);
}

bool IValueMetaObjectRecordDataExt::OnAfterCloseMetaObject()
{
	unregisterObject();
	unregisterManager();

	return IValueMetaObjectRecordData::OnAfterCloseMetaObject();
}

//***********************************************************************
//*						IValueMetaObjectRecordDataRef					*
//***********************************************************************

IValueMetaObjectRecordDataRef::IValueMetaObjectRecordDataRef() : IValueMetaObjectRecordData()
{
}

IValueMetaObjectRecordDataRef::~IValueMetaObjectRecordDataRef()
{
	//wxDELETE((*m_propertyAttributeReference));
}

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool IValueMetaObjectRecordDataRef::LoadData(CMemoryReader& dataReader)
{
	//get quick choice
	m_propertyQuickChoice->SetValue(dataReader.r_u8());

	//load default attributes:
	(*m_propertyAttributeReference)->LoadMeta(dataReader);

	return IValueMetaObjectRecordData::LoadData(dataReader);
}

bool IValueMetaObjectRecordDataRef::SaveData(CMemoryWriter& dataWritter)
{
	//set quick choice
	dataWritter.w_u8(m_propertyQuickChoice->GetValueAsBoolean());

	//save default attributes:
	(*m_propertyAttributeReference)->SaveMeta(dataWritter);

	//create or update table:
	return IValueMetaObjectRecordData::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool IValueMetaObjectRecordDataRef::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IValueMetaObjectRecordData::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyAttributeReference)->OnCreateMetaObject(metaData, flags);
}

#include "backend/appData.h"
#include "databaseLayer/databaseLayer.h"

bool IValueMetaObjectRecordDataRef::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyAttributeReference)->OnLoadMetaObject(metaData))
		return false;

	return IValueMetaObjectRecordData::OnLoadMetaObject(metaData);
}

bool IValueMetaObjectRecordDataRef::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyAttributeReference)->OnSaveMetaObject(flags))
		return false;

	return IValueMetaObjectRecordData::OnSaveMetaObject(flags);
}

bool IValueMetaObjectRecordDataRef::OnDeleteMetaObject()
{
	if (!(*m_propertyAttributeReference)->OnDeleteMetaObject())
		return false;

	return IValueMetaObjectRecordData::OnDeleteMetaObject();
}

bool IValueMetaObjectRecordDataRef::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyAttributeReference)->OnBeforeRunMetaObject(flags))
		return false;

	registerReference();
	registerRefList();
	registerManager();

	const IMetaValueTypeCtor* typeCtor = m_metaData->GetTypeCtor(this, eCtorMetaType::eCtorMetaType_Reference);
	if (typeCtor != nullptr)
		(*m_propertyAttributeReference)->SetDefaultMetaType(typeCtor->GetClassType());

	return IValueMetaObjectRecordData::OnBeforeRunMetaObject(flags);
}

bool IValueMetaObjectRecordDataRef::OnAfterRunMetaObject(int flags)
{
	return IValueMetaObjectRecordData::OnAfterRunMetaObject(flags);
}

bool IValueMetaObjectRecordDataRef::OnBeforeCloseMetaObject()
{
	return IValueMetaObjectRecordData::OnBeforeCloseMetaObject();
}

bool IValueMetaObjectRecordDataRef::OnAfterCloseMetaObject()
{
	if (!(*m_propertyAttributeReference)->OnAfterCloseMetaObject())
		return false;

	if (m_propertyAttributeReference != nullptr)
		(*m_propertyAttributeReference)->SetDefaultMetaType(eValueTypes::TYPE_EMPTY);

	unregisterReference();
	unregisteRefList();
	unregisterManager();

	return IValueMetaObjectRecordData::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                             form events                             *
//***********************************************************************

//process choice 
bool IValueMetaObjectRecordDataRef::ProcessChoice(IBackendControlFrame* ownerValue, const wxString& strFormName, eSelectMode selMode)
{
	IBackendValueForm* const selectChoiceForm = GetSelectForm(strFormName, ownerValue);
	if (selectChoiceForm == nullptr)
		return false;

	selectChoiceForm->ShowForm();
	return true;
}

CValueReferenceDataObject* IValueMetaObjectRecordDataRef::FindObjectValue(const CGuid& objGuid)
{
	if (!objGuid.isValid())
		return nullptr;
	return CValueReferenceDataObject::Create(this, objGuid);
}

//***********************************************************************
//*						IValueMetaObjectRecordDataEnumRef					*
//***********************************************************************

///////////////////////////////////////////////////////////////////////////////////////////////

IValueMetaObjectRecordDataEnumRef::IValueMetaObjectRecordDataEnumRef() : IValueMetaObjectRecordDataRef()
{
}

IValueMetaObjectRecordDataEnumRef::~IValueMetaObjectRecordDataEnumRef()
{
	//wxDELETE((*m_propertyAttributeOrder));
}

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool IValueMetaObjectRecordDataEnumRef::LoadData(CMemoryReader& dataReader)
{
	//load default attributes:
	(*m_propertyAttributeOrder)->LoadMeta(dataReader);
	return IValueMetaObjectRecordDataRef::LoadData(dataReader);
}

bool IValueMetaObjectRecordDataEnumRef::SaveData(CMemoryWriter& dataWritter)
{
	//save default attributes:
	(*m_propertyAttributeOrder)->SaveMeta(dataWritter);
	return IValueMetaObjectRecordDataRef::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool IValueMetaObjectRecordDataEnumRef::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IValueMetaObjectRecordDataRef::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyAttributeOrder)->OnCreateMetaObject(metaData, flags);
}

bool IValueMetaObjectRecordDataEnumRef::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyAttributeOrder)->OnLoadMetaObject(metaData))
		return false;

	return IValueMetaObjectRecordDataRef::OnLoadMetaObject(metaData);
}

bool IValueMetaObjectRecordDataEnumRef::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyAttributeOrder)->OnSaveMetaObject(flags))
		return false;

	return IValueMetaObjectRecordDataRef::OnSaveMetaObject(flags);
}

bool IValueMetaObjectRecordDataEnumRef::OnDeleteMetaObject()
{
	if (!(*m_propertyAttributeOrder)->OnDeleteMetaObject())
		return false;

	return IValueMetaObjectRecordDataRef::OnDeleteMetaObject();
}

bool IValueMetaObjectRecordDataEnumRef::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyAttributeOrder)->OnBeforeRunMetaObject(flags))
		return false;

	return IValueMetaObjectRecordDataRef::OnBeforeRunMetaObject(flags);
}

bool IValueMetaObjectRecordDataEnumRef::OnAfterRunMetaObject(int flags)
{
	return IValueMetaObjectRecordDataRef::OnAfterRunMetaObject(flags);
}

bool IValueMetaObjectRecordDataEnumRef::OnBeforeCloseMetaObject()
{
	return IValueMetaObjectRecordDataRef::OnBeforeCloseMetaObject();
}

bool IValueMetaObjectRecordDataEnumRef::OnAfterCloseMetaObject()
{
	if (!(*m_propertyAttributeOrder)->OnAfterCloseMetaObject())
		return false;

	return IValueMetaObjectRecordDataRef::OnAfterCloseMetaObject();
}

//***********************************************************************
//*						IValueMetaObjectRecordDataMutableRef					*
//***********************************************************************

IValueMetaObjectRecordDataMutableRef::IValueMetaObjectRecordDataMutableRef() : IValueMetaObjectRecordDataRef()
{
}

IValueMetaObjectRecordDataMutableRef::~IValueMetaObjectRecordDataMutableRef()
{
	//wxDELETE((*m_propertyAttributeDataVersion);
	//wxDELETE((*m_propertyAttributeDeletionMark));
}

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool IValueMetaObjectRecordDataMutableRef::LoadData(CMemoryReader& dataReader)
{
	//load default attributes:
	(*m_propertyAttributeDataVersion)->LoadMeta(dataReader);
	(*m_propertyAttributeDeletionMark)->LoadMeta(dataReader);

	if (!IValueMetaObjectRecordDataRef::LoadData(dataReader))
		return false;

	return m_propertyGeneration->LoadData(dataReader);
}

bool IValueMetaObjectRecordDataMutableRef::SaveData(CMemoryWriter& dataWritter)
{
	//save default attributes:
	(*m_propertyAttributeDataVersion)->SaveMeta(dataWritter);
	(*m_propertyAttributeDeletionMark)->SaveMeta(dataWritter);

	//create or update table:
	if (!IValueMetaObjectRecordDataRef::SaveData(dataWritter))
		return false;

	return m_propertyGeneration->SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool IValueMetaObjectRecordDataMutableRef::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IValueMetaObjectRecordDataRef::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyAttributeDataVersion)->OnCreateMetaObject(metaData, flags)
		&& (*m_propertyAttributeDeletionMark)->OnCreateMetaObject(metaData, flags);
}

bool IValueMetaObjectRecordDataMutableRef::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyAttributeDataVersion)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyAttributeDeletionMark)->OnLoadMetaObject(metaData))
		return false;

	return IValueMetaObjectRecordDataRef::OnLoadMetaObject(metaData);
}

bool IValueMetaObjectRecordDataMutableRef::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyAttributeDataVersion)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeDeletionMark)->OnSaveMetaObject(flags))
		return false;

	return IValueMetaObjectRecordDataRef::OnSaveMetaObject(flags);
}

bool IValueMetaObjectRecordDataMutableRef::OnDeleteMetaObject()
{
	if (!(*m_propertyAttributeDataVersion)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyAttributeDeletionMark)->OnDeleteMetaObject())
		return false;

	return IValueMetaObjectRecordDataRef::OnDeleteMetaObject();
}

bool IValueMetaObjectRecordDataMutableRef::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyAttributeDataVersion)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeDeletionMark)->OnBeforeRunMetaObject(flags))
		return false;

	registerObject();
	return IValueMetaObjectRecordDataRef::OnBeforeRunMetaObject(flags);
}

bool IValueMetaObjectRecordDataMutableRef::OnAfterRunMetaObject(int flags)
{
	if (!(*m_propertyAttributeDataVersion)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeDeletionMark)->OnAfterRunMetaObject(flags))
		return false;

	return IValueMetaObjectRecordDataRef::OnAfterRunMetaObject(flags);
}

bool IValueMetaObjectRecordDataMutableRef::OnBeforeCloseMetaObject()
{
	if (!(*m_propertyAttributeDataVersion)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeDeletionMark)->OnBeforeCloseMetaObject())
		return false;

	return IValueMetaObjectRecordDataRef::OnBeforeCloseMetaObject();
}

bool IValueMetaObjectRecordDataMutableRef::OnAfterCloseMetaObject()
{
	if (!(*m_propertyAttributeDataVersion)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeDeletionMark)->OnAfterCloseMetaObject())
		return false;

	unregisterObject();
	return IValueMetaObjectRecordDataRef::OnAfterCloseMetaObject();
}

///////////////////////////////////////////////////////////////////////////////

IValueRecordDataObjectRef* IValueMetaObjectRecordDataMutableRef::CreateObjectValue()
{
	IValueRecordDataObjectRef* createdValue = CreateObjectRefValue();
	if (createdValue && !createdValue->InitializeObject()) {
		wxDELETE(createdValue);
		return nullptr;
	}

	return createdValue;
}

IValueRecordDataObjectRef* IValueMetaObjectRecordDataMutableRef::CreateObjectValue(const CGuid& guid)
{
	IValueRecordDataObjectRef* createdValue = CreateObjectRefValue(guid);
	if (createdValue && !createdValue->InitializeObject()) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IValueRecordDataObjectRef* IValueMetaObjectRecordDataMutableRef::CreateObjectValue(IValueRecordDataObjectRef* objSrc, bool generate)
{
	if (objSrc == nullptr)
		return nullptr;
	IValueRecordDataObjectRef* createdValue = CreateObjectRefValue();
	if (createdValue && !createdValue->InitializeObject(objSrc, generate)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IValueRecordDataObjectRef* IValueMetaObjectRecordDataMutableRef::CopyObjectValue(const CGuid& srcGuid)
{
	IValueRecordDataObjectRef* createdValue = CreateObjectRefValue();
	if (createdValue && !createdValue->InitializeObject(srcGuid)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IValueRecordDataObject* IValueMetaObjectRecordDataMutableRef::CreateRecordDataObjectValue()
{
	return CreateObjectValue();
}

//***********************************************************************
//*						IValueMetaObjectRecordDataHierarchyMutableRef			*
//***********************************************************************

IValueMetaObjectRecordDataHierarchyMutableRef::IValueMetaObjectRecordDataHierarchyMutableRef()
	: IValueMetaObjectRecordDataMutableRef()
{
}

IValueMetaObjectRecordDataHierarchyMutableRef::~IValueMetaObjectRecordDataHierarchyMutableRef()
{
	//wxDELETE(m_propertyAttributeCode);
	//wxDELETE(m_propertyAttributeDescription);
	//wxDELETE(m_propertyAttributeParent);
	//wxDELETE(m_propertyAttributeIsFolder);
}

////////////////////////////////////////////////////////////////////////////////////////////////

IValueRecordDataObjectFolderRef* IValueMetaObjectRecordDataHierarchyMutableRef::CreateObjectValue(eObjectMode mode)
{
	IValueRecordDataObjectFolderRef* createdValue = CreateObjectRefValue(mode);
	if (createdValue && !createdValue->InitializeObject()) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IValueRecordDataObjectFolderRef* IValueMetaObjectRecordDataHierarchyMutableRef::CreateObjectValue(eObjectMode mode, const CGuid& guid)
{
	IValueRecordDataObjectFolderRef* createdValue = CreateObjectRefValue(mode, guid);
	if (createdValue && !createdValue->InitializeObject()) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IValueRecordDataObjectFolderRef* IValueMetaObjectRecordDataHierarchyMutableRef::CreateObjectValue(eObjectMode mode, IValueRecordDataObjectRef* objSrc, bool generate)
{
	IValueRecordDataObjectFolderRef* createdValue = CreateObjectRefValue(mode);
	if (createdValue && !createdValue->InitializeObject(objSrc, generate)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IValueRecordDataObjectFolderRef* IValueMetaObjectRecordDataHierarchyMutableRef::CopyObjectValue(eObjectMode mode, const CGuid& srcGuid)
{
	IValueRecordDataObjectFolderRef* createdValue = CreateObjectRefValue(mode);
	if (createdValue && !createdValue->InitializeObject(srcGuid)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

#define predefinedBlock 0x1234532

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool IValueMetaObjectRecordDataHierarchyMutableRef::LoadData(CMemoryReader& dataReader)
{
	//load predefined value 
	wxMemoryBuffer predefined_buffer;
	if (!dataReader.r_chunk(predefinedBlock, predefined_buffer))
		return false;

	CMemoryReader predefinedReader(predefined_buffer);

	unsigned int size = predefinedReader.r_u32();
	for (unsigned int i = 0; i < size; i++)
	{
		CPredefinedObjectValue entry;

		entry.m_valueGuid = predefinedReader.r_stringZ();

		entry.m_valueCode = m_metaData->Deserialize(predefinedReader.r_stringZ());
		entry.m_valueDescription = m_metaData->Deserialize(predefinedReader.r_stringZ());
		entry.m_valueIsFolder = m_metaData->Deserialize(predefinedReader.r_stringZ());

		m_predefinedObjectVector.emplace_back(std::move(entry));
	}

	//load default attributes:
	(*m_propertyAttributePredefined)->LoadMeta(dataReader);
	(*m_propertyAttributeCode)->LoadMeta(dataReader);
	(*m_propertyAttributeDescription)->LoadMeta(dataReader);
	(*m_propertyAttributeParent)->LoadMeta(dataReader);
	(*m_propertyAttributeIsFolder)->LoadMeta(dataReader);

	return IValueMetaObjectRecordDataMutableRef::LoadData(dataReader);
}

bool IValueMetaObjectRecordDataHierarchyMutableRef::SaveData(CMemoryWriter& dataWritter)
{
	//save predefined value 
	CMemoryWriter predefinedWritter;
	predefinedWritter.w_u32(m_predefinedObjectVector.size());

	for (const CPredefinedObjectValue& value : m_predefinedObjectVector)
	{
		predefinedWritter.w_stringZ(value.m_valueGuid);

		predefinedWritter.w_stringZ(m_metaData->Serialize(value.m_valueCode));
		predefinedWritter.w_stringZ(m_metaData->Serialize(value.m_valueDescription));
		predefinedWritter.w_stringZ(m_metaData->Serialize(value.m_valueIsFolder));
	}

	dataWritter.w_chunk(predefinedBlock, predefinedWritter.buffer());

	//save default attributes:
	(*m_propertyAttributePredefined)->SaveMeta(dataWritter);
	(*m_propertyAttributeCode)->SaveMeta(dataWritter);
	(*m_propertyAttributeDescription)->SaveMeta(dataWritter);
	(*m_propertyAttributeParent)->SaveMeta(dataWritter);
	(*m_propertyAttributeIsFolder)->SaveMeta(dataWritter);

	//create or update table:
	return IValueMetaObjectRecordDataMutableRef::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool IValueMetaObjectRecordDataHierarchyMutableRef::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IValueMetaObjectRecordDataMutableRef::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyAttributePredefined)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyAttributeCode)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyAttributeDescription)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyAttributeParent)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyAttributeIsFolder)->OnCreateMetaObject(metaData, flags);
}

bool IValueMetaObjectRecordDataHierarchyMutableRef::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyAttributePredefined)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyAttributeCode)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyAttributeDescription)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyAttributeParent)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyAttributeIsFolder)->OnLoadMetaObject(metaData))
		return false;

	return IValueMetaObjectRecordDataMutableRef::OnLoadMetaObject(metaData);
}

bool IValueMetaObjectRecordDataHierarchyMutableRef::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyAttributePredefined)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeCode)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeDescription)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeParent)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeIsFolder)->OnSaveMetaObject(flags))
		return false;

	return IValueMetaObjectRecordDataMutableRef::OnSaveMetaObject(flags);
}

bool IValueMetaObjectRecordDataHierarchyMutableRef::OnDeleteMetaObject()
{
	if (!(*m_propertyAttributePredefined)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyAttributeCode)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyAttributeDescription)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyAttributeParent)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyAttributeIsFolder)->OnDeleteMetaObject())
		return false;

	return IValueMetaObjectRecordDataMutableRef::OnDeleteMetaObject();
}

bool IValueMetaObjectRecordDataHierarchyMutableRef::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyAttributePredefined)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeCode)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeDescription)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeParent)->OnBeforeRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeIsFolder)->OnBeforeRunMetaObject(flags))
		return false;

	if (!IValueMetaObjectRecordDataMutableRef::OnBeforeRunMetaObject(flags))
		return false;

	return true;
}

bool IValueMetaObjectRecordDataHierarchyMutableRef::OnAfterRunMetaObject(int flags)
{
	if (!(*m_propertyAttributePredefined)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeCode)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeDescription)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeParent)->OnAfterRunMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeIsFolder)->OnAfterRunMetaObject(flags))
		return false;

	return IValueMetaObjectRecordDataMutableRef::OnAfterRunMetaObject(flags);
}

bool IValueMetaObjectRecordDataHierarchyMutableRef::OnBeforeCloseMetaObject()
{
	if (!(*m_propertyAttributePredefined)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeCode)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeDescription)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeParent)->OnBeforeCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeIsFolder)->OnBeforeCloseMetaObject())
		return false;

	return IValueMetaObjectRecordDataMutableRef::OnBeforeCloseMetaObject();
}

bool IValueMetaObjectRecordDataHierarchyMutableRef::OnAfterCloseMetaObject()
{
	if (!(*m_propertyAttributePredefined)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeCode)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeDescription)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeParent)->OnAfterCloseMetaObject())
		return false;

	if (!(*m_propertyAttributeIsFolder)->OnAfterCloseMetaObject())
		return false;

	return IValueMetaObjectRecordDataMutableRef::OnAfterCloseMetaObject();
}

//////////////////////////////////////////////////////////////////////

bool IValueMetaObjectRecordDataHierarchyMutableRef::ProcessChoice(IBackendControlFrame* ownerValue, const wxString& strFormName, eSelectMode selMode)
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

IValueRecordDataObjectRef* IValueMetaObjectRecordDataHierarchyMutableRef::CreateObjectRefValue(const CGuid& objGuid)
{
	return CreateObjectRefValue(eObjectMode::OBJECT_ITEM, objGuid);
}

//***********************************************************************
//*                      IValueMetaObjectRegisterData						*
//***********************************************************************

IValueMetaObjectRegisterData::IValueMetaObjectRegisterData() : IValueMetaObjectGenericData()
{
}

IValueMetaObjectRegisterData::~IValueMetaObjectRegisterData()
{
	//wxDELETE((*m_propertyAttributeLineActive));
	//wxDELETE((*m_propertyAttributePeriod));
	//wxDELETE((*m_propertyAttributeRecorder));
	//wxDELETE((*m_propertyAttributeLineNumber));
}

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool IValueMetaObjectRegisterData::LoadData(CMemoryReader& dataReader)
{
	//load default attributes:
	(*m_propertyAttributeLineActive)->LoadMeta(dataReader);
	(*m_propertyAttributePeriod)->LoadMeta(dataReader);
	(*m_propertyAttributeRecorder)->LoadMeta(dataReader);
	(*m_propertyAttributeLineNumber)->LoadMeta(dataReader);

	return IValueMetaObjectGenericData::LoadData(dataReader);
}

bool IValueMetaObjectRegisterData::SaveData(CMemoryWriter& dataWritter)
{
	//save default attributes:
	(*m_propertyAttributeLineActive)->SaveMeta(dataWritter);
	(*m_propertyAttributePeriod)->SaveMeta(dataWritter);
	(*m_propertyAttributeRecorder)->SaveMeta(dataWritter);
	(*m_propertyAttributeLineNumber)->SaveMeta(dataWritter);

	//create or update table:
	return IValueMetaObjectGenericData::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool IValueMetaObjectRegisterData::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IValueMetaObject::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyAttributeLineActive)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyAttributePeriod)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyAttributeRecorder)->OnCreateMetaObject(metaData, flags) &&
		(*m_propertyAttributeLineNumber)->OnCreateMetaObject(metaData, flags);
}

bool IValueMetaObjectRegisterData::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyAttributeLineActive)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyAttributePeriod)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyAttributeRecorder)->OnLoadMetaObject(metaData))
		return false;

	if (!(*m_propertyAttributeLineNumber)->OnLoadMetaObject(metaData))
		return false;

	return IValueMetaObject::OnLoadMetaObject(metaData);
}

bool IValueMetaObjectRegisterData::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyAttributeLineActive)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyAttributePeriod)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeRecorder)->OnSaveMetaObject(flags))
		return false;

	if (!(*m_propertyAttributeLineNumber)->OnSaveMetaObject(flags))
		return false;

	return IValueMetaObject::OnSaveMetaObject(flags);
}

bool IValueMetaObjectRegisterData::OnDeleteMetaObject()
{
	if (!(*m_propertyAttributeLineActive)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyAttributePeriod)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyAttributeRecorder)->OnDeleteMetaObject())
		return false;

	if (!(*m_propertyAttributeLineNumber)->OnDeleteMetaObject())
		return false;

	return IValueMetaObject::OnDeleteMetaObject();
}

bool IValueMetaObjectRegisterData::OnBeforeRunMetaObject(int flags)
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

	return IValueMetaObject::OnBeforeRunMetaObject(flags);
}

bool IValueMetaObjectRegisterData::OnAfterCloseMetaObject()
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

	return IValueMetaObject::OnAfterCloseMetaObject();
}

//***********************************************************************
//*								ARRAY									*
//***********************************************************************

CValueRecordKeyObject* IValueMetaObjectRegisterData::CreateRecordKeyObjectValue()
{
	return CValue::CreateAndPrepareValueRef<CValueRecordKeyObject>(this);
}

IValueRecordSetObject* IValueMetaObjectRegisterData::CreateRecordSetObjectValue(bool needInitialize)
{
	IValueRecordSetObject* createdValue = CreateRecordSetObjectRegValue();
	if (!needInitialize)
		return createdValue;
	if (createdValue && !createdValue->InitializeObject(nullptr, true)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IValueRecordSetObject* IValueMetaObjectRegisterData::CreateRecordSetObjectValue(const CUniquePairKey& uniqueKey, bool needInitialize)
{
	IValueRecordSetObject* createdValue = CreateRecordSetObjectRegValue(uniqueKey);
	if (!needInitialize)
		return createdValue;
	if (createdValue && !createdValue->InitializeObject(nullptr, false)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IValueRecordSetObject* IValueMetaObjectRegisterData::CreateRecordSetObjectValue(IValueRecordSetObject* source, bool needInitialize)
{
	IValueRecordSetObject* createdValue = CreateRecordSetObjectRegValue();
	if (!needInitialize)
		return createdValue;
	if (createdValue && !createdValue->InitializeObject(source, true)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IValueRecordSetObject* IValueMetaObjectRegisterData::CopyRecordSetObjectValue(const CUniquePairKey& uniqueKey)
{
	IValueRecordSetObject* createdValue = CreateRecordSetObjectRegValue(uniqueKey);
	if (createdValue && !createdValue->InitializeObject(nullptr, true)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IValueRecordManagerObject* IValueMetaObjectRegisterData::CreateRecordManagerObjectValue()
{
	IValueRecordManagerObject* createdValue = CreateRecordManagerObjectRegValue();
	if (createdValue && !createdValue->InitializeObject(nullptr, true)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IValueRecordManagerObject* IValueMetaObjectRegisterData::CreateRecordManagerObjectValue(const CUniquePairKey& uniqueKey)
{
	IValueRecordManagerObject* createdValue = CreateRecordManagerObjectRegValue(uniqueKey);
	if (createdValue && !createdValue->InitializeObject(nullptr, false)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IValueRecordManagerObject* IValueMetaObjectRegisterData::CreateRecordManagerObjectValue(IValueRecordManagerObject* source)
{
	IValueRecordManagerObject* createdValue = CreateRecordManagerObjectRegValue();
	if (createdValue && !createdValue->InitializeObject(source, true)) {
		wxDELETE(createdValue);
		return nullptr;
	}
	return createdValue;
}

IValueRecordManagerObject* IValueMetaObjectRegisterData::CopyRecordManagerObjectValue(const CUniquePairKey& uniqueKey)
{
	IValueRecordManagerObject* createdValue = CreateRecordManagerObjectRegValue();
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
//*                        IValueRecordDataObject							*
//***********************************************************************

wxIMPLEMENT_ABSTRACT_CLASS(IValueRecordDataObject, CValue);

IValueRecordDataObject::IValueRecordDataObject(const CGuid& objGuid, bool newObject) :
	CValue(eValueTypes::TYPE_VALUE), IValueDataObject(objGuid, newObject),
	m_methodHelper(new CMethodHelper())
{
}

IValueRecordDataObject::IValueRecordDataObject(const IValueRecordDataObject& source) :
	CValue(eValueTypes::TYPE_VALUE), IValueDataObject(wxNewUniqueGuid, true),
	m_methodHelper(new CMethodHelper())
{
}

IValueRecordDataObject::~IValueRecordDataObject()
{
	wxDELETE(m_methodHelper);
}

IBackendValueForm* IValueRecordDataObject::GetForm() const
{
	if (!m_objGuid.isValid())
		return nullptr;
	return IBackendValueForm::FindFormByUniqueKey(m_objGuid);
}

class_identifier_t IValueRecordDataObject::GetClassType() const
{
	IValueMetaObjectRecordData* metaObject = GetMetaObject();
	wxASSERT(metaObject);
	const IMetaValueTypeCtor* clsFactory =
		metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Object);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString IValueRecordDataObject::GetClassName() const
{
	IValueMetaObjectRecordData* metaObject = GetMetaObject();
	wxASSERT(metaObject);
	const IMetaValueTypeCtor* clsFactory =
		metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Object);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString IValueRecordDataObject::GetString() const
{
	IValueMetaObjectRecordData* metaObject = GetMetaObject();
	wxASSERT(metaObject);
	const IMetaValueTypeCtor* clsFactory =
		metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Object);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

CSourceExplorer IValueRecordDataObject::GetSourceExplorer() const
{
	IValueMetaObjectRecordData* metaObject = GetMetaObject();

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

bool IValueRecordDataObject::GetModel(IValueModel*& tableValue, const meta_identifier_t& id)
{
	auto& it = m_listObjectValue.find(id);
	if (it != m_listObjectValue.end()) {
		const CValue& cTabularSection = it->second;
		return cTabularSection.ConvertToValue(tableValue);
	};

	tableValue = nullptr;
	return false;
}

bool IValueRecordDataObject::SetValueByMetaID(const meta_identifier_t& id, const CValue& varMetaVal)
{
	auto& it = m_listObjectValue.find(id);
	wxASSERT(it != m_listObjectValue.end());
	if (it != m_listObjectValue.end()) {

		IValueMetaObjectRecordData* metaObjectValue = GetMetaObject();
		wxASSERT(metaObjectValue);

		const IValueMetaObjectAttribute* attribute = metaObjectValue->FindAnyAttributeObjectByFilter(id);
		wxASSERT(attribute);
		it->second = attribute->AdjustValue(varMetaVal);
		return true;
	}
	return false;
}

bool IValueRecordDataObject::GetValueByMetaID(const meta_identifier_t& id, CValue& pvarMetaVal) const
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

IValueTable* IValueRecordDataObject::GetTableByMetaID(const meta_identifier_t& id) const
{
	const CValue& cTable = GetValueByMetaID(id); IValueTable* retTable = nullptr;
	if (cTable.ConvertToValue(retTable))
		return retTable;
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

#define thisObject wxT("thisObject")

void IValueRecordDataObject::PrepareEmptyObject()
{
	IValueMetaObjectRecordData* metaObject = GetMetaObject();
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
		m_listObjectValue.insert_or_assign(object->GetMetaID(), CValue::CreateAndPrepareValueRef<CValueTabularSectionDataObject>(this, object));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void IValueRecordDataObject::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	m_methodHelper->AppendFunc("getFormObject", 2, "getFormObject(string, owner)");
	m_methodHelper->AppendFunc("getMetadata", "getMetadata()");

	m_methodHelper->AppendProp(thisObject,
		true, false, eThisObject, eSystem
	);

	IValueMetaObjectRecordData* metaObject = GetMetaObject();
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

bool IValueRecordDataObject::SetPropVal(const long lPropNum, const CValue& varPropVal)
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

bool IValueRecordDataObject::GetPropVal(const long lPropNum, CValue& pvarPropVal)
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

bool IValueRecordDataObject::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
{
	const long lMethodAlias = m_methodHelper->GetPropAlias(lMethodNum);
	if (lMethodAlias == eProcUnit) {
		return IModuleDataObject::ExecuteProc(
			GetMethodName(lMethodNum), paParams, lSizeArray
		);
	}

	return false;
}

bool IValueRecordDataObject::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
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
//*                        IValueRecordDataObjectExt							*           
//***********************************************************************

wxIMPLEMENT_ABSTRACT_CLASS(IValueRecordDataObjectExt, IValueRecordDataObject);

IValueRecordDataObjectExt::IValueRecordDataObjectExt(IValueMetaObjectRecordDataExt* metaObject) :
	IValueRecordDataObject(wxNewUniqueGuid, true), m_metaObject(metaObject)
{
}

IValueRecordDataObjectExt::IValueRecordDataObjectExt(const IValueRecordDataObjectExt& source) :
	IValueRecordDataObject(source), m_metaObject(source.m_metaObject)
{
}

IValueRecordDataObjectExt::~IValueRecordDataObjectExt()
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

bool IValueRecordDataObjectExt::InitializeObject()
{
	if (!m_metaObject->IsExternalCreate()) {

		if (!m_metaObject->AccessRight_Use()) {
			CBackendAccessException::Error();
			return false;
		}

		const IMetaData* metaData = m_metaObject->GetMetaData();
		wxASSERT(metaData);
		const IValueModuleManager* moduleManager = metaData->GetModuleManager();
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
			catch (const CBackendException* err) {
				if (!appData->DesignerMode())
					throw(err);
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

bool IValueRecordDataObjectExt::InitializeObject(IValueRecordDataObjectExt* source)
{
	//if (m_metaObject->AccessRight("use", appData->GetUserName()))
	//{

	//}

	if (!m_metaObject->IsExternalCreate()) {
		const IMetaData* metaData = m_metaObject->GetMetaData();
		wxASSERT(metaData);
		const IValueModuleManager* moduleManager = metaData->GetModuleManager();
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
				if (!appData->DesignerMode())
					throw(err);
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

IValueRecordDataObjectExt* IValueRecordDataObjectExt::CopyObjectValue()
{
	return m_metaObject->CreateObjectValue(this);
}

//***********************************************************************
//*                        IValueRecordDataObjectRef							*           
//***********************************************************************

wxIMPLEMENT_ABSTRACT_CLASS(IValueRecordDataObjectRef, IValueRecordDataObject);

IValueRecordDataObjectRef::IValueRecordDataObjectRef(IValueMetaObjectRecordDataMutableRef* metaObject, const CGuid& objGuid) :
	IValueRecordDataObject(objGuid.isValid() ? objGuid : CGuid::newGuid(GUID_TIME_BASED), !objGuid.isValid()),
	m_metaObject(metaObject),
	m_reference_impl(nullptr),
	m_objModified(false)
{
	if (m_metaObject != nullptr)
		m_reference_impl = new reference_t(m_metaObject->GetMetaID(), m_objGuid);
}

IValueRecordDataObjectRef::IValueRecordDataObjectRef(const IValueRecordDataObjectRef& src) :
	IValueRecordDataObject(src),
	m_metaObject(src.m_metaObject),
	m_reference_impl(nullptr),
	m_objModified(false)
{
	if (m_metaObject != nullptr)
		m_reference_impl = new reference_t(m_metaObject->GetMetaID(), m_objGuid);
}

IValueRecordDataObjectRef::~IValueRecordDataObjectRef()
{
	wxDELETE(m_reference_impl);
}

bool IValueRecordDataObjectRef::InitializeObject(const CGuid& copyGuid)
{
	if (!m_metaObject->AccessRight_Read()) {
		CBackendAccessException::Error();
		return false;
	}

	IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);
	IValueModuleManager* moduleManager = metaData->GetModuleManager();
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
			if (!appData->DesignerMode())
				throw(err);
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
				IValueMetaObjectAttribute* codeAttribute = m_metaObject->GetAttributeForCode();
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

bool IValueRecordDataObjectRef::InitializeObject(IValueRecordDataObjectRef* source, bool generate)
{
	const IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);
	const IValueModuleManager* moduleManager = metaData->GetModuleManager();
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
			if (!appData->DesignerMode())
				throw(err);
			return false;
		};
	}

	CValuePtr<CValueReferenceDataObject> reference(
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

class_identifier_t IValueRecordDataObjectRef::GetClassType() const
{
	return IValueRecordDataObject::GetClassType();
}

wxString IValueRecordDataObjectRef::GetClassName() const
{
	return IValueRecordDataObject::GetClassName();
}

wxString IValueRecordDataObjectRef::GetString() const
{
	return m_metaObject->GetDataPresentation(this);
}

CSourceExplorer IValueRecordDataObjectRef::GetSourceExplorer() const
{
	CSourceExplorer srcHelper(
		m_metaObject, GetClassType(),
		false
	);

	IValueMetaObjectAttribute* attribute = m_metaObject->GetAttributeForCode();

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

bool IValueRecordDataObjectRef::GetModel(IValueModel*& tableValue, const meta_identifier_t& id)
{
	auto& it = m_listObjectValue.find(id);
	if (it != m_listObjectValue.end()) {
		const CValue& cTabularSection = it->second;
		return cTabularSection.ConvertToValue(tableValue);
	};
	tableValue = nullptr;
	return false;
}

void IValueRecordDataObjectRef::Modify(bool mod)
{
	IBackendValueForm* const foundedForm = IBackendValueForm::FindFormByUniqueKey(m_objGuid);

	if (foundedForm != nullptr)
		foundedForm->Modify(mod);

	m_objModified = mod;
}

bool IValueRecordDataObjectRef::Generate()
{
	if (m_newObject)
		return false;

	IBackendValueForm* const foundedForm = IBackendValueForm::FindFormByUniqueKey(m_objGuid);
	if (foundedForm != nullptr)
		return foundedForm->GenerateForm(this);

	return false;
}

bool IValueRecordDataObjectRef::Filling(CValue& cValue) const
{
	CValue standartProcessing = true;
	if (m_procUnit != nullptr) {
		m_procUnit->CallAsProc(wxT("Filling"), cValue, standartProcessing);
	}
	return standartProcessing.GetBoolean();
}

bool IValueRecordDataObjectRef::SetValueByMetaID(const meta_identifier_t& id, const CValue& varMetaVal)
{
	if (varMetaVal != IValueRecordDataObject::GetValueByMetaID(id)) {
		if (IValueRecordDataObject::SetValueByMetaID(id, varMetaVal)) {
			IValueRecordDataObjectRef::Modify(true);
			return true;
		}
		return false;
	}
	return true;
}

bool IValueRecordDataObjectRef::GetValueByMetaID(const meta_identifier_t& id, CValue& pvarMetaVal) const
{
	return IValueRecordDataObject::GetValueByMetaID(id, pvarMetaVal);
}

IValueRecordDataObjectRef* IValueRecordDataObjectRef::CopyObjectValue()
{
	return m_metaObject->CreateObjectValue(this);
}

void IValueRecordDataObjectRef::PrepareEmptyObject()
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
		m_listObjectValue.insert_or_assign(object->GetMetaID(), CValue::CreateAndPrepareValueRef<CValueTabularSectionDataObjectRef>(this, object));
	}
	m_objModified = true;
}

void IValueRecordDataObjectRef::PrepareEmptyObject(const IValueRecordDataObjectRef* source)
{
	m_listObjectValue.clear();

	IValueMetaObjectAttribute* codeAttribute = m_metaObject->GetAttributeForCode();
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
		CValueTabularSectionDataObjectRef* tableSection = CValue::CreateAndPrepareValueRef<CValueTabularSectionDataObjectRef>(this, object);
		if (tableSection->LoadDataFromTable(source->GetTableByMetaID(object->GetMetaID())))
			m_listObjectValue.insert_or_assign(object->GetMetaID(), tableSection);
		else
			wxDELETE(tableSection);
	}
	m_objModified = true;
}

CValueReferenceDataObject* IValueRecordDataObjectRef::GetReference() const
{
	if (m_newObject) {
		return CValueReferenceDataObject::Create(m_metaObject);
	}

	return CValueReferenceDataObject::Create(m_metaObject, m_objGuid);
}

//***********************************************************************
//*                        IValueRecordDataObjectFolderRef					*           
//***********************************************************************

wxIMPLEMENT_ABSTRACT_CLASS(IValueRecordDataObjectFolderRef, IValueRecordDataObjectRef);

IValueRecordDataObjectFolderRef::IValueRecordDataObjectFolderRef(IValueMetaObjectRecordDataHierarchyMutableRef* metaObject, const CGuid& objGuid, eObjectMode objMode)
	: IValueRecordDataObjectRef(metaObject, objGuid), m_objMode(objMode)
{
}

IValueRecordDataObjectFolderRef::IValueRecordDataObjectFolderRef(const IValueRecordDataObjectFolderRef& source)
	: IValueRecordDataObjectRef(source), m_objMode(source.m_objMode)
{
}

IValueRecordDataObjectFolderRef::~IValueRecordDataObjectFolderRef()
{
}

CSourceExplorer IValueRecordDataObjectFolderRef::GetSourceExplorer() const
{
	CSourceExplorer srcHelper(
		m_metaObject, GetClassType(),
		false
	);
	IValueMetaObjectAttribute* attribute = m_metaObject->GetAttributeForCode();
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

bool IValueRecordDataObjectFolderRef::GetModel(IValueModel*& tableValue, const meta_identifier_t& id)
{
	auto& it = m_listObjectValue.find(id);
	if (it != m_listObjectValue.end()) {
		const CValue& cTabularSection = it->second;
		return cTabularSection.ConvertToValue(tableValue);
	};

	tableValue = nullptr;
	return false;
}

IValueRecordDataObjectRef* IValueRecordDataObjectFolderRef::CopyObjectValue()
{
	return ((IValueMetaObjectRecordDataHierarchyMutableRef*)m_metaObject)->CreateObjectValue(m_objMode, this);
}

bool IValueRecordDataObjectFolderRef::SetValueByMetaID(const meta_identifier_t& id, const CValue& varMetaVal)
{
	const CValue& cOldValue = IValueRecordDataObjectRef::GetValueByMetaID(id);
	if (cOldValue.GetType() == TYPE_NULL)
		return false;
	if (GetMetaObject()->IsDataParent(id) &&
		varMetaVal == GetReference()) {
		return false;
	}

	return IValueRecordDataObjectRef::SetValueByMetaID(id, varMetaVal);
}

bool IValueRecordDataObjectFolderRef::GetValueByMetaID(const meta_identifier_t& id, CValue& pvarMetaVal) const
{
	return IValueRecordDataObjectRef::GetValueByMetaID(id, pvarMetaVal);
}

void IValueRecordDataObjectFolderRef::PrepareEmptyObject()
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
	IValueMetaObjectRecordDataHierarchyMutableRef* metaFolder = GetMetaObject();
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
				m_listObjectValue.insert_or_assign(object->GetMetaID(), CValue::CreateAndPrepareValueRef<CValueTabularSectionDataObjectRef>(this, object));
			}
			else {
				m_listObjectValue.insert_or_assign(object->GetMetaID(), eValueTypes::TYPE_NULL);
			}
		}
		else {
			if (tableUse == eItemMode::eItemMode_Folder ||
				tableUse == eItemMode::eItemMode_Folder_Item) {
				m_listObjectValue.insert_or_assign(object->GetMetaID(), CValue::CreateAndPrepareValueRef<CValueTabularSectionDataObjectRef>(this, object));
			}
			else {
				m_listObjectValue.insert_or_assign(object->GetMetaID(), eValueTypes::TYPE_NULL);
			}
		}
	}
	m_objModified = true;
}

void IValueRecordDataObjectFolderRef::PrepareEmptyObject(const IValueRecordDataObjectRef* source)
{
	m_listObjectValue.clear();
	IValueMetaObjectAttribute* codeAttribute = m_metaObject->GetAttributeForCode();
	wxASSERT(codeAttribute);
	m_listObjectValue[codeAttribute->GetMetaID()] = codeAttribute->CreateValue();
	//attributes can refValue 
	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		if (object->IsDeleted())
			continue;
		CValueMetaObjectAttribute* metaAttr = nullptr; eItemMode attrUse = eItemMode::eItemMode_Folder_Item;
		if (object->ConvertToValue(metaAttr)) {
			attrUse = metaAttr->GetItemMode();
		}
		if (object != codeAttribute && !m_metaObject->IsDataReference(object->GetMetaID())) {
			source->GetValueByMetaID(object->GetMetaID(), m_listObjectValue[object->GetMetaID()]);
		}
	}
	IValueMetaObjectRecordDataHierarchyMutableRef* metaFolder = GetMetaObject();
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
		CValueMetaObjectTableData* metaTable = nullptr; eItemMode tableUse = eItemMode::eItemMode_Folder_Item;
		if (object->ConvertToValue(metaTable))
			tableUse = metaTable->GetTableUse();
		CValueTabularSectionDataObjectRef* tableSection = CValue::CreateAndPrepareValueRef <CValueTabularSectionDataObjectRef>(this, object);
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

wxIMPLEMENT_ABSTRACT_CLASS(IValueRecordSetObject, IValueTable);
wxIMPLEMENT_ABSTRACT_CLASS(IValueRecordManagerObject, CValue);

wxIMPLEMENT_ABSTRACT_CLASS(CValueRecordKeyObject, CValue);

wxIMPLEMENT_DYNAMIC_CLASS(IValueRecordSetObject::CValueRecordSetObjectRegisterKeyValue, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(IValueRecordSetObject::CValueRecordSetObjectRegisterKeyValue::CValueRecordSetObjectRegisterKeyDescriptionValue, CValue);

//***********************************************************************
//*                      Record key & set								*
//***********************************************************************

//////////////////////////////////////////////////////////////////////
//						  CValueRecordKeyObject							//
//////////////////////////////////////////////////////////////////////

CValueRecordKeyObject::CValueRecordKeyObject(IValueMetaObjectRegisterData* metaObject) : CValue(eValueTypes::TYPE_VALUE),
m_metaObject(metaObject), m_methodHelper(new CMethodHelper())
{
}

CValueRecordKeyObject::~CValueRecordKeyObject()
{
	wxDELETE(m_methodHelper);
}

bool CValueRecordKeyObject::IsEmpty() const
{
	for (auto value : m_keyValues) {
		const CValue& cValue = value.second;
		if (!cValue.IsEmpty())
			return false;
	}

	return true;
}

class_identifier_t CValueRecordKeyObject::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_RecordKey);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CValueRecordKeyObject::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_RecordKey);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CValueRecordKeyObject::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_RecordKey);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

//////////////////////////////////////////////////////////////////////
//						  IValueRecordManagerObject						//
//////////////////////////////////////////////////////////////////////

void IValueRecordManagerObject::CreateEmptyKey()
{
	m_recordSet->CreateEmptyKey();
}

bool IValueRecordManagerObject::InitializeObject(const IValueRecordManagerObject* source, bool newRecord)
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

bool IValueRecordManagerObject::InitializeObject(const CUniquePairKey& key)
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

IValueRecordManagerObject* IValueRecordManagerObject::CopyRegisterValue()
{
	return m_metaObject->CreateRecordManagerObjectValue(this);
}

IValueRecordManagerObject::IValueRecordManagerObject(IValueMetaObjectRegisterData* metaObject, const CUniquePairKey& uniqueKey) : CValue(eValueTypes::TYPE_VALUE),
m_metaObject(metaObject), m_methodHelper(new CMethodHelper()),
m_recordSet(m_metaObject->CreateRecordSetObjectValue(uniqueKey, false)), m_recordLine(nullptr),
m_objGuid(uniqueKey)
{
}

IValueRecordManagerObject::IValueRecordManagerObject(const IValueRecordManagerObject& source) : CValue(eValueTypes::TYPE_VALUE),
m_metaObject(source.m_metaObject), m_methodHelper(new CMethodHelper()),
m_recordSet(m_metaObject->CreateRecordSetObjectValue(source.m_recordSet, false)), m_recordLine(nullptr),
m_objGuid(source.m_metaObject)
{
}

IValueRecordManagerObject::~IValueRecordManagerObject()
{
	wxDELETE(m_methodHelper);
}

IBackendValueForm* IValueRecordManagerObject::GetForm() const
{
	if (!m_objGuid.isValid())
		return nullptr;
	if (m_recordSet->m_selected)
		return IBackendValueForm::FindFormByUniqueKey(m_objGuid);
	return nullptr;
}

bool IValueRecordManagerObject::IsEmpty() const
{
	return m_recordSet->IsEmpty();
}

CSourceExplorer IValueRecordManagerObject::GetSourceExplorer() const
{
	CSourceExplorer srcHelper(
		m_metaObject, GetClassType(), false
	);

	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		srcHelper.AppendSource(object);
	}

	return srcHelper;
}

bool IValueRecordManagerObject::GetModel(IValueModel*& tableValue, const meta_identifier_t& id)
{
	return false;
}

void IValueRecordManagerObject::Modify(bool mod)
{
	IBackendValueForm* const foundedForm = IBackendValueForm::FindFormByUniqueKey(m_objGuid);
	if (foundedForm != nullptr)
		foundedForm->Modify(mod);
	m_recordSet->Modify(mod);
}

bool IValueRecordManagerObject::IsModified() const
{
	return m_recordSet->IsModified();
}

bool IValueRecordManagerObject::SetValueByMetaID(const meta_identifier_t& id, const CValue& varMetaVal)
{
	if (varMetaVal != IValueRecordManagerObject::GetValueByMetaID(id)) {
		bool result = m_recordLine->SetValueByMetaID(id, varMetaVal);
		IValueRecordManagerObject::Modify(true);
		return result;
	}
	return true;
}

bool IValueRecordManagerObject::GetValueByMetaID(const meta_identifier_t& id, CValue& pvarMetaVal) const
{
	return m_recordLine->GetValueByMetaID(id, pvarMetaVal);
}

class_identifier_t IValueRecordManagerObject::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_RecordManager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString IValueRecordManagerObject::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_RecordManager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString IValueRecordManagerObject::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_RecordManager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

/////////////////////////////////////////////////////////////////////

void IValueRecordManagerObject::PrepareEmptyObject(const IValueRecordManagerObject* source)
{
	m_recordLine = nullptr;

	if (source == nullptr) {
		m_recordLine = CValue::CreateAndPrepareValueRef<IValueRecordSetObject::CValueRecordSetObjectRegisterReturnLine>(
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
//						  IValueRecordSetObject							//
//////////////////////////////////////////////////////////////////////

void IValueRecordSetObject::CreateEmptyKey()
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

bool IValueRecordSetObject::InitializeObject(const IValueRecordSetObject* source, bool newRecord)
{
	if (!m_metaObject->AccessRight_Read()) {
		CBackendAccessException::Error();
		return false;
	}

	const IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);

	const IValueModuleManager* moduleManager = metaData->GetModuleManager();
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
			if (!appData->DesignerMode())
				throw(err);
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

IValueRecordSetObject* IValueRecordSetObject::CopyRegisterValue()
{
	return m_metaObject->CreateRecordSetObjectValue(this);
}

///////////////////////////////////////////////////////////////////////////////////

IValueRecordSetObject::IValueRecordSetObject(IValueMetaObjectRegisterData* metaObject, const CUniquePairKey& uniqueKey) : IValueTable(),
m_recordColumnCollection(CValue::CreateAndPrepareValueRef<CValueRecordSetObjectRegisterColumnCollection>(this)), m_recordSetKeyValue(CValue::CreateAndPrepareValueRef<CValueRecordSetObjectRegisterKeyValue>(this)),
m_metaObject(metaObject), m_keyValues(uniqueKey.IsOk() ? uniqueKey : metaObject), m_objModified(false), m_selected(false),
m_methodHelper(new CMethodHelper())
{
}

IValueRecordSetObject::IValueRecordSetObject(const IValueRecordSetObject& source) : IValueTable(),
m_recordColumnCollection(CValue::CreateAndPrepareValueRef<CValueRecordSetObjectRegisterColumnCollection>(this)), m_recordSetKeyValue(CValue::CreateAndPrepareValueRef<CValueRecordSetObjectRegisterKeyValue>(this)),
m_metaObject(source.m_metaObject), m_keyValues(source.m_keyValues), m_objModified(true), m_selected(false),
m_methodHelper(new CMethodHelper())
{
	for (long row = 0; row < source.GetRowCount(); row++) {
		wxValueTableRow* node = source.GetViewData<wxValueTableRow>(source.GetItem(row));
		wxASSERT(node);
		IValueTable::Append(new wxValueTableRow(*node), false);
	}
}

IValueRecordSetObject::~IValueRecordSetObject()
{
	wxDELETE(m_methodHelper);
}

bool IValueRecordSetObject::GetAt(const CValue& varKeyValue, CValue& pvarValue)
{
	long index = varKeyValue.GetUInteger();
	if (index >= GetRowCount() && !appData->DesignerMode()) {
		CBackendCoreException::Error(_("Array index out of bounds"));
		return false;
	}
	pvarValue = CValue::CreateAndPrepareValueRef<CValueRecordSetObjectRegisterReturnLine>(this, GetItem(index));
	return true;
}

class_identifier_t IValueRecordSetObject::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_RecordSet);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString IValueRecordSetObject::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_RecordSet);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString IValueRecordSetObject::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_RecordSet);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

#include "backend/system/value/valueTable.h"

bool IValueRecordSetObject::LoadDataFromTable(IValueTable* srcTable)
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

IValueTable* IValueRecordSetObject::SaveDataToTable() const
{
	CValueTableMemory* valueTable = CValue::CreateAndConvertObjectRef<CValueTableMemory>();

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

bool IValueRecordSetObject::SetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id, const CValue& varMetaVal)
{
	if (!appData->DesignerMode()) {
		wxValueTableRow* node = GetViewData<wxValueTableRow>(item);
		if (node != nullptr) {
			const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(id);
			if (attribute != nullptr) {
				return node->SetValue(
					id, attribute->AdjustValue(varMetaVal), true
				);
			}
		}
	}

	return false;
}

bool IValueRecordSetObject::GetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id, CValue& pvarMetaVal) const
{
	if (appData->DesignerMode()) {
		const IValueMetaObjectAttribute* attribute = m_metaObject->FindAnyAttributeObjectByFilter(id);
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
//					CValueRecordSetObjectRegisterColumnCollection				//
//////////////////////////////////////////////////////////////////////



wxIMPLEMENT_DYNAMIC_CLASS(IValueRecordSetObject::CValueRecordSetObjectRegisterColumnCollection, IValueTable::IValueModelColumnCollection);

IValueRecordSetObject::CValueRecordSetObjectRegisterColumnCollection::CValueRecordSetObjectRegisterColumnCollection() :
	IValueModelColumnCollection(),
	m_ownerTable(nullptr),
	m_methodHelper(nullptr)
{
}

IValueRecordSetObject::CValueRecordSetObjectRegisterColumnCollection::CValueRecordSetObjectRegisterColumnCollection(IValueRecordSetObject* ownerTable) :
	IValueModelColumnCollection(),
	m_ownerTable(ownerTable),
	m_methodHelper(new CMethodHelper())
{
	IValueMetaObjectGenericData* metaObject = m_ownerTable->GetMetaObject();
	wxASSERT(metaObject);

	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		m_listColumnInfo.insert_or_assign(object->GetMetaID(),
			CValue::CreateAndPrepareValueRef<CValueRecordSetRegisterColumnInfo>(object));
	}
}

IValueRecordSetObject::CValueRecordSetObjectRegisterColumnCollection::~CValueRecordSetObjectRegisterColumnCollection()
{
	wxDELETE(m_methodHelper);
}

bool IValueRecordSetObject::CValueRecordSetObjectRegisterColumnCollection::SetAt(const CValue& varKeyValue, const CValue& varValue)//индекс массива должен начинаться с 0
{
	return false;
}

bool IValueRecordSetObject::CValueRecordSetObjectRegisterColumnCollection::GetAt(const CValue& varKeyValue, CValue& pvarValue) //индекс массива должен начинаться с 0
{
	unsigned int index = varKeyValue.GetUInteger();
	if ((index < 0 || index >= m_listColumnInfo.size() && !appData->DesignerMode())) {
		CBackendCoreException::Error(_("Index goes beyond array"));
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

wxIMPLEMENT_DYNAMIC_CLASS(IValueRecordSetObject::CValueRecordSetObjectRegisterColumnCollection::CValueRecordSetRegisterColumnInfo, IValueTable::IValueModelColumnCollection::IValueModelColumnInfo);

IValueRecordSetObject::CValueRecordSetObjectRegisterColumnCollection::CValueRecordSetRegisterColumnInfo::CValueRecordSetRegisterColumnInfo() :
	IValueModelColumnInfo(), m_metaAttribute(nullptr)
{
}

IValueRecordSetObject::CValueRecordSetObjectRegisterColumnCollection::CValueRecordSetRegisterColumnInfo::CValueRecordSetRegisterColumnInfo(IValueMetaObjectAttribute* attribute) :
	IValueModelColumnInfo(), m_metaAttribute(attribute)
{
}

IValueRecordSetObject::CValueRecordSetObjectRegisterColumnCollection::CValueRecordSetRegisterColumnInfo::~CValueRecordSetRegisterColumnInfo()
{
}

//////////////////////////////////////////////////////////////////////
//					 CValueRecordSetObjectRegisterReturnLine					//
//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(IValueRecordSetObject::CValueRecordSetObjectRegisterReturnLine, IValueTable::IValueModelReturnLine);

IValueRecordSetObject::CValueRecordSetObjectRegisterReturnLine::CValueRecordSetObjectRegisterReturnLine(IValueRecordSetObject* ownerTable, const wxDataViewItem& line)
	: IValueModelReturnLine(line), m_ownerTable(ownerTable), m_methodHelper(new CMethodHelper())
{
}

IValueRecordSetObject::CValueRecordSetObjectRegisterReturnLine::~CValueRecordSetObjectRegisterReturnLine()
{
	wxDELETE(m_methodHelper);
}

void IValueRecordSetObject::CValueRecordSetObjectRegisterReturnLine::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	const IValueMetaObjectGenericData* metaObject = m_ownerTable->GetMetaObject();
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
//				       CValueRecordSetObjectRegisterKeyValue					//
//////////////////////////////////////////////////////////////////////

IValueRecordSetObject::CValueRecordSetObjectRegisterKeyValue::CValueRecordSetObjectRegisterKeyValue(IValueRecordSetObject* recordSet) : CValue(eValueTypes::TYPE_VALUE, true),
m_methodHelper(new CMethodHelper()), m_recordSet(recordSet)
{
}

IValueRecordSetObject::CValueRecordSetObjectRegisterKeyValue::~CValueRecordSetObjectRegisterKeyValue()
{
	wxDELETE(m_methodHelper);
}

//////////////////////////////////////////////////////////////////////
//						CValueRecordSetObjectRegisterKeyDescriptionValue		//
//////////////////////////////////////////////////////////////////////

IValueRecordSetObject::CValueRecordSetObjectRegisterKeyValue::CValueRecordSetObjectRegisterKeyDescriptionValue::CValueRecordSetObjectRegisterKeyDescriptionValue(IValueRecordSetObject* recordSet, const meta_identifier_t& id) : CValue(eValueTypes::TYPE_VALUE),
m_methodHelper(new CMethodHelper()), m_recordSet(recordSet), m_metaId(id)
{
}

IValueRecordSetObject::CValueRecordSetObjectRegisterKeyValue::CValueRecordSetObjectRegisterKeyDescriptionValue::~CValueRecordSetObjectRegisterKeyDescriptionValue()
{
	wxDELETE(m_methodHelper);
}

//////////////////////////////////////////////////////////////////////////////////////

long IValueRecordSetObject::AppendRow(unsigned int before)
{
	wxValueTableRow* rowData = new wxValueTableRow();

	IValueMetaObjectRegisterData* metaObject = GetMetaObject();
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

bool CValueRecordKeyObject::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	return false;
}

bool CValueRecordKeyObject::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////

bool IValueRecordSetObject::CValueRecordSetObjectRegisterReturnLine::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	const meta_identifier_t& id = m_methodHelper->GetPropData(lPropNum);
	if (id != wxNOT_FOUND)
		return SetValueByMetaID(id, varPropVal);
	return false;
}

bool IValueRecordSetObject::CValueRecordSetObjectRegisterReturnLine::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const meta_identifier_t& id = m_methodHelper->GetPropData(lPropNum);
	if (id != wxNOT_FOUND) {
		return GetValueByMetaID(id, pvarPropVal);
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////

class_identifier_t IValueRecordSetObject::CValueRecordSetObjectRegisterReturnLine::GetClassType() const
{
	const IValueMetaObject* metaTable = m_ownerTable->GetMetaObject();
	const IMetaData* metaData = metaTable->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(metaTable, eCtorMetaType::eCtorMetaType_RecordSet_String);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString IValueRecordSetObject::CValueRecordSetObjectRegisterReturnLine::GetClassName() const
{
	const IValueMetaObject* metaTable = m_ownerTable->GetMetaObject();
	const IMetaData* metaData = metaTable->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(metaTable, eCtorMetaType::eCtorMetaType_RecordSet_String);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString IValueRecordSetObject::CValueRecordSetObjectRegisterReturnLine::GetString() const
{
	const IValueMetaObject* metaTable = m_ownerTable->GetMetaObject();
	const IMetaData* metaData = metaTable->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(metaTable, eCtorMetaType::eCtorMetaType_RecordSet_String);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

////////////////////////////////////////////////////////////////////////////

bool IValueRecordSetObject::CValueRecordSetObjectRegisterKeyValue::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	return false;
}

bool IValueRecordSetObject::CValueRecordSetObjectRegisterKeyValue::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const meta_identifier_t& id = m_methodHelper->GetPropData(lPropNum);
	if (id != wxNOT_FOUND) {
		pvarPropVal = CValue::CreateAndPrepareValueRef<CValueRecordSetObjectRegisterKeyDescriptionValue>(m_recordSet, id);
		return true;
	}
	return false;
}

//****************************************************************************
//*                              Support methods                             *
//****************************************************************************

void CValueRecordKeyObject::PrepareNames() const
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

void IValueRecordSetObject::CValueRecordSetObjectRegisterKeyValue::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	IValueMetaObjectRegisterData* metaObject = m_recordSet->GetMetaObject();
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

void IValueRecordSetObject::CValueRecordSetObjectRegisterKeyValue::CValueRecordSetObjectRegisterKeyDescriptionValue::PrepareNames() const
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

bool IValueRecordSetObject::CValueRecordSetObjectRegisterKeyValue::CValueRecordSetObjectRegisterKeyDescriptionValue::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	const IValueMetaObjectRegisterData* metaObject = m_recordSet->GetMetaObject();
	wxASSERT(metaObject);

	const IValueMetaObjectAttribute* attribute = metaObject->FindAnyAttributeObjectByFilter(m_metaId);
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

bool IValueRecordSetObject::CValueRecordSetObjectRegisterKeyValue::CValueRecordSetObjectRegisterKeyDescriptionValue::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const IValueMetaObjectRegisterData* metaObject = m_recordSet->GetMetaObject();
	wxASSERT(metaObject);

	const IValueMetaObjectAttribute* attribute = metaObject->FindAnyAttributeObjectByFilter(m_metaId);
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

bool CValueRecordKeyObject::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
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

bool IValueRecordSetObject::CValueRecordSetObjectRegisterKeyValue::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	return false;
}

//////////////////////////////////////////////////////////////

bool IValueRecordSetObject::CValueRecordSetObjectRegisterKeyValue::CValueRecordSetObjectRegisterKeyDescriptionValue::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
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

SYSTEM_TYPE_REGISTER(IValueRecordSetObject::CValueRecordSetObjectRegisterColumnCollection, "recordSetRegisterColumn", string_to_clsid("VL_RSCL"));
SYSTEM_TYPE_REGISTER(IValueRecordSetObject::CValueRecordSetObjectRegisterColumnCollection::CValueRecordSetRegisterColumnInfo, "recordSetRegisterColumnInfo", string_to_clsid("VL_RSCI"));

SYSTEM_TYPE_REGISTER(IValueRecordSetObject::CValueRecordSetObjectRegisterKeyValue, "recordSetRegisterKey", string_to_clsid("VL_RSCK"));
SYSTEM_TYPE_REGISTER(IValueRecordSetObject::CValueRecordSetObjectRegisterKeyValue::CValueRecordSetObjectRegisterKeyDescriptionValue, "recordSetRegisterKeyDescription", string_to_clsid("VL_RDVL"));