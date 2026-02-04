////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : enumeration metaData
////////////////////////////////////////////////////////////////////////////

#include "enumeration.h"
#include "backend/metaData.h"
#include "list/objectList.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectEnumeration, IValueMetaObjectRecordDataEnumRef)

//********************************************************************************************

#include "databaseLayer/databaseLayer.h"
#include "backend/appData.h"

//********************************************************************************************
//*                                      metaData                                            *
//********************************************************************************************

CValueMetaObjectEnumeration::CValueMetaObjectEnumeration() : IValueMetaObjectRecordDataEnumRef()
{
	m_propertyQuickChoice->SetValue(true);
}

CValueMetaObjectEnumeration::~CValueMetaObjectEnumeration()
{
}

IValueMetaObjectForm* CValueMetaObjectEnumeration::GetDefaultFormByID(const form_identifier_t& id) const
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

IValueManagerDataObject* CValueMetaObjectEnumeration::CreateManagerDataObjectValue()
{
	return CValue::CreateAndPrepareValueRef<CValueManagerDataObjectEnumeration>(this);
}

ISourceDataObject* CValueMetaObjectEnumeration::CreateSourceObject(IValueMetaObjectForm* metaObject)
{
	switch (metaObject->GetTypeForm())
	{
	case eFormList: 
		return CValue::CreateAndPrepareValueRef<CValueListDataObjectEnumRef>(this, metaObject->GetTypeForm());
	case eFormSelect: 
		return CValue::CreateAndPrepareValueRef<CValueListDataObjectEnumRef>(this, metaObject->GetTypeForm(), true);
	}

	return nullptr;
}

#pragma region _form_builder_h_
IBackendValueForm* CValueMetaObjectEnumeration::GetListForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IValueMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CValueMetaObjectEnumeration::eFormList,
		ownerControl, CValue::CreateAndPrepareValueRef<CValueListDataObjectEnumRef>(this, CValueMetaObjectEnumeration::eFormList),
		formGuid
	);
}

IBackendValueForm* CValueMetaObjectEnumeration::GetSelectForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IValueMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CValueMetaObjectEnumeration::eFormSelect,
		ownerControl, CValue::CreateAndPrepareValueRef<CValueListDataObjectEnumRef>(this, CValueMetaObjectEnumeration::eFormSelect, true),
		formGuid
	);
}
#pragma endregion

wxString CValueMetaObjectEnumeration::GetDataPresentation(const IValueDataObject* objValue) const
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

bool CValueMetaObjectEnumeration::LoadData(CMemoryReader& dataReader)
{
	//Load object module
	(*m_propertyModuleManager)->LoadMeta(dataReader);

	//save default form 
	m_propertyDefFormList->SetValue(GetIdByGuid(dataReader.r_stringZ()));
	m_propertyDefFormSelect->SetValue(GetIdByGuid(dataReader.r_stringZ()));

	return IValueMetaObjectRecordDataEnumRef::LoadData(dataReader);
}

bool CValueMetaObjectEnumeration::SaveData(CMemoryWriter& dataWritter)
{
	//Save object module
	(*m_propertyModuleManager)->SaveMeta(dataWritter);

	//save default form 
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormList->GetValueAsInteger()));
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormSelect->GetValueAsInteger()));

	//create or update table:
	return IValueMetaObjectRecordDataEnumRef::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool CValueMetaObjectEnumeration::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IValueMetaObjectRecordDataEnumRef::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyModuleManager)->OnCreateMetaObject(metaData, flags);
}

bool CValueMetaObjectEnumeration::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyModuleManager)->OnLoadMetaObject(metaData))
		return false;

	return IValueMetaObjectRecordDataEnumRef::OnLoadMetaObject(metaData);
}

bool CValueMetaObjectEnumeration::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyModuleManager)->OnSaveMetaObject(flags))
		return false;

#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
	if (GetEnumObjectArray().size() == 0) {
		s_restructureInfo.AppendError(_("! Doesn't have any enumeration ") + GetFullName());
		return false;
	}
#endif 

	return IValueMetaObjectRecordDataEnumRef::OnSaveMetaObject(flags);
}

bool CValueMetaObjectEnumeration::OnDeleteMetaObject()
{
	if (!(*m_propertyModuleManager)->OnDeleteMetaObject())
		return false;

	return IValueMetaObjectRecordDataEnumRef::OnDeleteMetaObject();
}

bool CValueMetaObjectEnumeration::OnReloadMetaObject()
{
	return true;
}

bool CValueMetaObjectEnumeration::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyModuleManager)->OnBeforeRunMetaObject(flags))
		return false;

	return IValueMetaObjectRecordDataEnumRef::OnBeforeRunMetaObject(flags);
}

bool CValueMetaObjectEnumeration::OnAfterRunMetaObject(int flags)
{
	if (!(*m_propertyModuleManager)->OnAfterRunMetaObject(flags))
		return false;

	return IValueMetaObjectRecordDataEnumRef::OnAfterRunMetaObject(flags);
}

bool CValueMetaObjectEnumeration::OnBeforeCloseMetaObject()
{
	if (!(*m_propertyModuleManager)->OnBeforeCloseMetaObject())
		return false;

	return IValueMetaObjectRecordDataEnumRef::OnBeforeCloseMetaObject();
}

bool CValueMetaObjectEnumeration::OnAfterCloseMetaObject()
{
	if (!(*m_propertyModuleManager)->OnAfterCloseMetaObject())
		return false;

	return IValueMetaObjectRecordDataEnumRef::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                             form events                             *
//***********************************************************************

void CValueMetaObjectEnumeration::OnCreateFormObject(IValueMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CValueMetaObjectEnumeration::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CValueMetaObjectEnumeration::eFormSelect
		&& m_propertyDefFormSelect->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormSelect->SetValue(metaForm->GetMetaID());
	}
}

void CValueMetaObjectEnumeration::OnRemoveMetaForm(IValueMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CValueMetaObjectEnumeration::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CValueMetaObjectEnumeration::eFormSelect
		&& m_propertyDefFormSelect->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormSelect->SetValue(metaForm->GetMetaID());
	}
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CValueMetaObjectEnumeration, "enumeration", g_metaEnumerationCLSID);