////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : meta-tables
////////////////////////////////////////////////////////////////////////////

#include "metaTableObject.h"
#include "backend/metaData.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectTableData, IValueMetaObjectCompositeData)

//***********************************************************************
//*                         Attributes                                  * 
//***********************************************************************

#include "backend/objCtor.h"
#include "backend/metaCollection/partial/commonObject.h"

CTypeDescription CValueMetaObjectTableData::GetTypeDesc() const
{
	const IMetaValueTypeCtor* typeCtor = m_metaData->GetTypeCtor(this, eCtorMetaType::eCtorMetaType_TabularSection);
	wxASSERT(typeCtor);
	if (typeCtor != nullptr) return CTypeDescription(typeCtor->GetClassType());
	return CTypeDescription();
}

////////////////////////////////////////////////////////////////////////////

CValueMetaObjectTableData::CValueMetaObjectTableData() : IValueMetaObjectCompositeData()
{
}

CValueMetaObjectTableData::~CValueMetaObjectTableData()
{
	//wxDELETE(m_numberLine);
}

bool CValueMetaObjectTableData::LoadData(CMemoryReader& dataReader)
{
	m_propertyUse->SetValue(dataReader.r_u16());

	//load default attributes:
	return (*m_propertyNumberLine)->LoadMeta(dataReader);
}

bool CValueMetaObjectTableData::SaveData(CMemoryWriter& dataWritter)
{
	dataWritter.w_u16(m_propertyUse->GetValueAsInteger());

	//save default attributes:
	return (*m_propertyNumberLine)->SaveMeta(dataWritter);;
}

//***********************************************************************
//*								Events								    *
//***********************************************************************

bool CValueMetaObjectTableData::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IValueMetaObject::OnCreateMetaObject(metaData, flags))
		return false;
	if (!(*m_propertyNumberLine)->OnCreateMetaObject(metaData, flags)) {
		return false;
	}
	return true;
}

bool CValueMetaObjectTableData::OnLoadMetaObject(IMetaData* metaData)
{
	if (!(*m_propertyNumberLine)->OnLoadMetaObject(metaData))
		return false;

	return IValueMetaObject::OnLoadMetaObject(metaData);
}

bool CValueMetaObjectTableData::OnSaveMetaObject(int flags)
{
	if (!(*m_propertyNumberLine)->OnSaveMetaObject(flags))
		return false;

	return IValueMetaObject::OnSaveMetaObject(flags);
}

bool CValueMetaObjectTableData::OnDeleteMetaObject()
{
	if (!(*m_propertyNumberLine)->OnDeleteMetaObject())
		return false;

	return IValueMetaObject::OnDeleteMetaObject();
}

bool CValueMetaObjectTableData::OnReloadMetaObject()
{
	IValueMetaObject* metaObject = GetParent();
	wxASSERT(metaObject);
	if (metaObject->OnReloadMetaObject())
		return IValueMetaObject::OnReloadMetaObject();
	return false;
}

bool CValueMetaObjectTableData::OnBeforeRunMetaObject(int flags)
{
	if (!(*m_propertyNumberLine)->OnBeforeRunMetaObject(flags))
		return false;
	IValueMetaObjectRecordData* metaObject = wxDynamicCast(GetParent(), IValueMetaObjectRecordData);
	wxASSERT(metaObject);
	if (metaObject != nullptr) {
		registerTabularSection();
		registerTabularSection_String();
	}

	return IValueMetaObject::OnBeforeRunMetaObject(flags);
}

bool CValueMetaObjectTableData::OnAfterRunMetaObject(int flags)
{
	if ((flags & newObjectFlag) != 0 || (flags & pasteObjectFlag) != 0) 
		OnReloadMetaObject();
	return IValueMetaObject::OnAfterRunMetaObject(flags);
}

bool CValueMetaObjectTableData::OnAfterCloseMetaObject()
{
	if (!(*m_propertyNumberLine)->OnAfterCloseMetaObject())
		return false;
	IValueMetaObjectRecordData* metaObject = wxDynamicCast(GetParent(), IValueMetaObjectRecordData);
	wxASSERT(metaObject);
	if (metaObject != nullptr) {
		unregisterTabularSection();
		unregisterTabularSection_String();
	}
	return IValueMetaObject::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CValueMetaObjectTableData, "tabularSection", g_metaTableCLSID);