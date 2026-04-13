////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : enumeration metaData
////////////////////////////////////////////////////////////////////////////

#include "enumeration.h"
#include "backend/metaData.h"
#include "list/objectList.h"

wxIMPLEMENT_DYNAMIC_CLASS(ibValueMetaObjectEnumeration, ibValueMetaObjectRecordDataEnumRef)

//********************************************************************************************

#include "databaseLayer/databaseLayer.h"
#include "backend/appData.h"

//********************************************************************************************
//*                                      metaData                                            *
//********************************************************************************************

ibValueMetaObjectEnumeration::ibValueMetaObjectEnumeration() : ibValueMetaObjectRecordDataEnumRef()
{
	m_propertyQuickChoice->SetValue(true);
}

ibValueMetaObjectEnumeration::~ibValueMetaObjectEnumeration()
{
}

ibValueMetaObjectFormBase* ibValueMetaObjectEnumeration::GetDefaultFormByID(const ibFormID& id) const
{
	if (id == eFormList && m_propertyDefFormList->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormList->GetValueAsInteger());
	}
	else if (id == eFormSelect && m_propertyDefFormSelect->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormSelect->GetValueAsInteger());
	}

	return nullptr;
}

#include "enumerationManager.h"

ibValueManagerDataObject* ibValueMetaObjectEnumeration::CreateManagerDataObjectValue()
{
	return ibValue::CreateAndPrepareValueRef<ibValueManagerDataObjectEnumeration>(this);
}

ibSourceDataObject* ibValueMetaObjectEnumeration::CreateSourceObject(ibValueMetaObjectFormBase* metaObject)
{
	switch (metaObject->GetTypeForm())
	{
	case eFormList: 
		return ibValue::CreateAndPrepareValueRef<ibValueListDataObjectEnumRef>(this, metaObject->GetTypeForm());
	case eFormSelect: 
		return ibValue::CreateAndPrepareValueRef<ibValueListDataObjectEnumRef>(this, metaObject->GetTypeForm(), true);
	}

	return nullptr;
}

#pragma region _form_builder_h_
ibBackendValueForm* ibValueMetaObjectEnumeration::GetListForm(const wxString& strFormName, ibBackendControlFrame* ownerControl, const ibUniqueKey& formGuid)
{
	return ibValueMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		ibValueMetaObjectEnumeration::eFormList,
		ownerControl, ibValue::CreateAndPrepareValueRef<ibValueListDataObjectEnumRef>(this, ibValueMetaObjectEnumeration::eFormList),
		formGuid
	);
}

ibBackendValueForm* ibValueMetaObjectEnumeration::GetSelectForm(const wxString& strFormName, ibBackendControlFrame* ownerControl, const ibUniqueKey& formGuid)
{
	return ibValueMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		ibValueMetaObjectEnumeration::eFormSelect,
		ownerControl, ibValue::CreateAndPrepareValueRef<ibValueListDataObjectEnumRef>(this, ibValueMetaObjectEnumeration::eFormSelect, true),
		formGuid
	);
}
#pragma endregion

wxString ibValueMetaObjectEnumeration::GetDataPresentation(const ibValueDataObject* objValue) const
{
	for (auto obj : GetEnumObjectArray()) {
		if (objValue->GetGuid() == obj->GetGuid()) {
			return obj->GetSynonym();
		}
	}
	return wxEmptyString;
}

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool ibValueMetaObjectEnumeration::LoadData(ibReaderMemory& dataReader)
{
	//Load object module
	(*m_propertyModuleManager)->LoadMeta(dataReader);

	//save default form 
	m_propertyDefFormList->SetValue(GetIdByGuid(dataReader.r_stringZ()));
	m_propertyDefFormSelect->SetValue(GetIdByGuid(dataReader.r_stringZ()));

	return ibValueMetaObjectRecordDataEnumRef::LoadData(dataReader);
}

bool ibValueMetaObjectEnumeration::SaveData(ibWriterMemory& dataWritter)
{
	//Save object module
	(*m_propertyModuleManager)->SaveMeta(dataWritter);

	//save default form 
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormList->GetValueAsInteger()));
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormSelect->GetValueAsInteger()));

	//create or update table:
	return ibValueMetaObjectRecordDataEnumRef::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool ibValueMetaObjectEnumeration::OnCreateMetaObject(ibMetaData* metaData, int flags)
{
	if (!ibValueMetaObjectRecordDataEnumRef::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyModuleManager)->OnCreateMetaObject(metaData, flags);
}

bool ibValueMetaObjectEnumeration::OnLoadMetaObject(ibMetaData* metaData)
{
	if (!(*m_propertyModuleManager)->OnLoadMetaObject(metaData))
		return false;

	return ibValueMetaObjectRecordDataEnumRef::OnLoadMetaObject(metaData);
}

bool ibValueMetaObjectEnumeration::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyModuleManager)->OnSaveMetaObject(flags))
		return false;

#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
	if (GetEnumObjectArray().size() == 0) {
		s_restructureInfo.AppendError(_("! Doesn't have any enumeration ") + GetFullName());
		return false;
	}
#endif 

	return ibValueMetaObjectRecordDataEnumRef::OnSaveMetaObject(flags);
}

bool ibValueMetaObjectEnumeration::OnDeleteMetaObject()
{
	if (!(*m_propertyModuleManager)->OnDeleteMetaObject())
		return false;

	return ibValueMetaObjectRecordDataEnumRef::OnDeleteMetaObject();
}

bool ibValueMetaObjectEnumeration::OnReloadMetaObject()
{
	return true;
}

bool ibValueMetaObjectEnumeration::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyModuleManager)->OnBeforeRunMetaObject(flags))
		return false;

	return ibValueMetaObjectRecordDataEnumRef::OnBeforeRunMetaObject(flags);
}

bool ibValueMetaObjectEnumeration::OnAfterRunMetaObject(int flags)
{
	if (!(*m_propertyModuleManager)->OnAfterRunMetaObject(flags))
		return false;

	return ibValueMetaObjectRecordDataEnumRef::OnAfterRunMetaObject(flags);
}

bool ibValueMetaObjectEnumeration::OnBeforeCloseMetaObject()
{
	if (!(*m_propertyModuleManager)->OnBeforeCloseMetaObject())
		return false;

	return ibValueMetaObjectRecordDataEnumRef::OnBeforeCloseMetaObject();
}

bool ibValueMetaObjectEnumeration::OnAfterCloseMetaObject()
{
	if (!(*m_propertyModuleManager)->OnAfterCloseMetaObject())
		return false;

	return ibValueMetaObjectRecordDataEnumRef::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                             form events                             *
//***********************************************************************

void ibValueMetaObjectEnumeration::OnCreateFormObject(ibValueMetaObjectFormBase* metaForm)
{
	if (metaForm->GetTypeForm() == ibValueMetaObjectEnumeration::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == ibValueMetaObjectEnumeration::eFormSelect
		&& m_propertyDefFormSelect->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormSelect->SetValue(metaForm->GetMetaID());
	}
}

void ibValueMetaObjectEnumeration::OnRemoveMetaForm(ibValueMetaObjectFormBase* metaForm)
{
	if (metaForm->GetTypeForm() == ibValueMetaObjectEnumeration::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == ibValueMetaObjectEnumeration::eFormSelect
		&& m_propertyDefFormSelect->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormSelect->SetValue(metaForm->GetMetaID());
	}
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(ibValueMetaObjectEnumeration, "Enumeration", g_metaEnumerationCLSID);