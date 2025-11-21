////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : meta-tables
////////////////////////////////////////////////////////////////////////////

#include "metaTableObject.h"
#include "backend/metaData.h"

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectTableData, IMetaObjectCompositeData)

//***********************************************************************
//*                         Attributes                                  * 
//***********************************************************************

#include "backend/objCtor.h"
#include "backend/metaCollection/partial/commonObject.h"

CTypeDescription CMetaObjectTableData::GetTypeDesc() const
{
	const IMetaValueTypeCtor* typeCtor = m_metaData->GetTypeCtor(this, eCtorMetaType::eCtorMetaType_TabularSection);
	wxASSERT(typeCtor);
	if (typeCtor != nullptr) return CTypeDescription(typeCtor->GetClassType());
	return CTypeDescription();
}

////////////////////////////////////////////////////////////////////////////

CMetaObjectTableData::CMetaObjectTableData() : IMetaObjectCompositeData()
{
}

CMetaObjectTableData::~CMetaObjectTableData()
{
	//wxDELETE(m_numberLine);
}

bool CMetaObjectTableData::LoadData(CMemoryReader& dataReader)
{
	m_propertyUse->SetValue(dataReader.r_u16());

	//load default attributes:
	return (*m_propertyNumberLine)->LoadMeta(dataReader);
}

bool CMetaObjectTableData::SaveData(CMemoryWriter& dataWritter)
{
	dataWritter.w_u16(m_propertyUse->GetValueAsInteger());

	//save default attributes:
	return (*m_propertyNumberLine)->SaveMeta(dataWritter);;
}

//***********************************************************************
//*								Events								    *
//***********************************************************************

bool CMetaObjectTableData::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IMetaObject::OnCreateMetaObject(metaData, flags))
		return false;
	if (!(*m_propertyNumberLine)->OnCreateMetaObject(metaData, flags)) {
		return false;
	}
	return true;
}

bool CMetaObjectTableData::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyNumberLine)->OnLoadMetaObject(metaData))
		return false;

	return IMetaObject::OnLoadMetaObject(metaData);
}

bool CMetaObjectTableData::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyNumberLine)->OnSaveMetaObject(flags))
		return false;

	return IMetaObject::OnSaveMetaObject(flags);
}

bool CMetaObjectTableData::OnDeleteMetaObject()
{
	if (!(*m_propertyNumberLine)->OnDeleteMetaObject())
		return false;

	return IMetaObject::OnDeleteMetaObject();
}

bool CMetaObjectTableData::OnReloadMetaObject()
{
	IMetaObject* metaObject = GetParent();
	wxASSERT(metaObject);
	if (metaObject->OnReloadMetaObject())
		return IMetaObject::OnReloadMetaObject();
	return false;
}

bool CMetaObjectTableData::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyNumberLine)->OnBeforeRunMetaObject(flags))
		return false;
	IMetaObjectRecordData* metaObject = wxDynamicCast(GetParent(), IMetaObjectRecordData);
	wxASSERT(metaObject);
	if (metaObject != nullptr) {
		registerTabularSection();
		registerTabularSection_String();
	}

	return IMetaObject::OnBeforeRunMetaObject(flags);
}

bool CMetaObjectTableData::OnAfterRunMetaObject(int flags)
{
	if ((flags & newObjectFlag) != 0 || (flags & pasteObjectFlag) != 0) 
		OnReloadMetaObject();
	return IMetaObject::OnAfterRunMetaObject(flags);
}

bool CMetaObjectTableData::OnAfterCloseMetaObject()
{
	if (!(*m_propertyNumberLine)->OnAfterCloseMetaObject())
		return false;
	IMetaObjectRecordData* metaObject = wxDynamicCast(GetParent(), IMetaObjectRecordData);
	wxASSERT(metaObject);
	if (metaObject != nullptr) {
		unregisterTabularSection();
		unregisterTabularSection_String();
	}
	return IMetaObject::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CMetaObjectTableData, "tabularSection", g_metaTableCLSID);