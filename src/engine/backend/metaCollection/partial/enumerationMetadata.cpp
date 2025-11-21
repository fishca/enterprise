////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : enumeration metaData
////////////////////////////////////////////////////////////////////////////

#include "enumeration.h"
#include "backend/metaData.h"
#include "list/objectList.h"

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectEnumeration, IMetaObjectRecordDataEnumRef)

//********************************************************************************************

#include "databaseLayer/databaseLayer.h"
#include "backend/appData.h"

//********************************************************************************************
//*                                      metaData                                            *
//********************************************************************************************

CMetaObjectEnumeration::CMetaObjectEnumeration() : IMetaObjectRecordDataEnumRef()
{
	m_propertyQuickChoice->SetValue(true);
}

CMetaObjectEnumeration::~CMetaObjectEnumeration()
{
}

IMetaObjectForm* CMetaObjectEnumeration::GetDefaultFormByID(const form_identifier_t& id)
{
	if (id == eFormList && m_propertyDefFormList->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormList->GetValueAsInteger());
	}
	else if (id == eFormSelect && m_propertyDefFormSelect->GetValueAsInteger() != wxNOT_FOUND) {
		return FindFormObjectByFilter(m_propertyDefFormSelect->GetValueAsInteger());
	}

	return nullptr;
}

ISourceDataObject* CMetaObjectEnumeration::CreateSourceObject(IMetaObjectForm* metaObject)
{
	switch (metaObject->GetTypeForm())
	{
	case eFormList: return
		new CListDataObjectEnumRef(this, metaObject->GetTypeForm()); break;
	case eFormSelect: return
		new CListDataObjectEnumRef(this, metaObject->GetTypeForm(), true);
		break;
	}

	return nullptr;
}

#pragma region _form_builder_h_
IBackendValueForm* CMetaObjectEnumeration::GetListForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CMetaObjectEnumeration::eFormList,
		ownerControl, CValue::CreateAndPrepareValueRef<CListDataObjectEnumRef>(this, CMetaObjectEnumeration::eFormList),
		formGuid
	);
}

IBackendValueForm* CMetaObjectEnumeration::GetSelectForm(const wxString& strFormName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	return IMetaObjectGenericData::CreateAndBuildForm(
		strFormName,
		CMetaObjectEnumeration::eFormSelect,
		ownerControl, CValue::CreateAndPrepareValueRef<CListDataObjectEnumRef>(this, CMetaObjectEnumeration::eFormSelect, true),
		formGuid
	);
}
#pragma endregion

bool CMetaObjectEnumeration::GetFormList(CPropertyList* prop)
{
	prop->AppendItem(wxT("notSelected"), _("<not selected>"), wxNOT_FOUND);
	for (auto formObject : GetFormArrayObject()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormList == formObject->GetTypeForm()) {
			prop->AppendItem(formObject->GetName(), formObject->GetMetaID(), formObject);
		}
	}
	return true;
}

bool CMetaObjectEnumeration::GetFormSelect(CPropertyList* prop)
{
	prop->AppendItem(wxT("notSelected"), _("<not selected>"), wxNOT_FOUND);
	for (auto formObject : GetFormArrayObject()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormSelect == formObject->GetTypeForm()) {
			prop->AppendItem(formObject->GetName(), formObject->GetMetaID(), formObject);
		}
	}
	return true;
}

wxString CMetaObjectEnumeration::GetDataPresentation(const IValueDataObject* objValue) const
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

bool CMetaObjectEnumeration::LoadData(CMemoryReader& dataReader)
{
	//Load object module
	(*m_propertyModuleManager)->LoadMeta(dataReader);

	//save default form 
	m_propertyDefFormList->SetValue(GetIdByGuid(dataReader.r_stringZ()));
	m_propertyDefFormSelect->SetValue(GetIdByGuid(dataReader.r_stringZ()));

	return IMetaObjectRecordDataEnumRef::LoadData(dataReader);
}

bool CMetaObjectEnumeration::SaveData(CMemoryWriter& dataWritter)
{
	//Save object module
	(*m_propertyModuleManager)->SaveMeta(dataWritter);

	//save default form 
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormList->GetValueAsInteger()));
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormSelect->GetValueAsInteger()));

	//create or update table:
	return IMetaObjectRecordDataEnumRef::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool CMetaObjectEnumeration::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IMetaObjectRecordDataEnumRef::OnCreateMetaObject(metaData, flags))
		return false;

	return (*m_propertyModuleManager)->OnCreateMetaObject(metaData, flags);
}

bool CMetaObjectEnumeration::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyModuleManager)->OnLoadMetaObject(metaData))
		return false;

	return IMetaObjectRecordDataEnumRef::OnLoadMetaObject(metaData);
}

bool CMetaObjectEnumeration::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyModuleManager)->OnSaveMetaObject(flags))
		return false;

#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
	if (GetEnumObjectArray().size() == 0) {
		s_restructureInfo.AppendError(_("! Doesn't have any enumeration ") + GetFullName());
		return false;
	}
#endif 

	return IMetaObjectRecordDataEnumRef::OnSaveMetaObject(flags);
}

bool CMetaObjectEnumeration::OnDeleteMetaObject()
{
	if (!(*m_propertyModuleManager)->OnDeleteMetaObject())
		return false;

	return IMetaObjectRecordDataEnumRef::OnDeleteMetaObject();
}

bool CMetaObjectEnumeration::OnReloadMetaObject()
{
	return true;
}

bool CMetaObjectEnumeration::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyModuleManager)->OnBeforeRunMetaObject(flags))
		return false;

	return IMetaObjectRecordDataEnumRef::OnBeforeRunMetaObject(flags);
}

bool CMetaObjectEnumeration::OnAfterRunMetaObject(int flags)
{
	if (!(*m_propertyModuleManager)->OnAfterRunMetaObject(flags))
		return false;

	return IMetaObjectRecordDataEnumRef::OnAfterRunMetaObject(flags);
}

bool CMetaObjectEnumeration::OnBeforeCloseMetaObject()
{
	if (!(*m_propertyModuleManager)->OnBeforeCloseMetaObject())
		return false;

	return IMetaObjectRecordDataEnumRef::OnBeforeCloseMetaObject();
}

bool CMetaObjectEnumeration::OnAfterCloseMetaObject()
{
	if (!(*m_propertyModuleManager)->OnAfterCloseMetaObject())
		return false;

	return IMetaObjectRecordDataEnumRef::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                             form events                             *
//***********************************************************************

void CMetaObjectEnumeration::OnCreateFormObject(IMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CMetaObjectEnumeration::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CMetaObjectEnumeration::eFormSelect
		&& m_propertyDefFormSelect->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormSelect->SetValue(metaForm->GetMetaID());
	}
}

void CMetaObjectEnumeration::OnRemoveMetaForm(IMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CMetaObjectEnumeration::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
	else if (metaForm->GetTypeForm() == CMetaObjectEnumeration::eFormSelect
		&& m_propertyDefFormSelect->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormSelect->SetValue(metaForm->GetMetaID());
	}
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CMetaObjectEnumeration, "enumeration", g_metaEnumerationCLSID);