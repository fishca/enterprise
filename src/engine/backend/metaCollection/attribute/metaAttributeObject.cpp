////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : meta-attribues
////////////////////////////////////////////////////////////////////////////

#include "metaAttributeObject.h"
#include "backend/metadata.h"

////////////////////////////////////////////////////////////////////////////

wxIMPLEMENT_ABSTRACT_CLASS(IMetaObjectAttribute, IMetaObject);

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectAttribute, IMetaObjectAttribute);
wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectAttributeDefault, IMetaObjectAttribute);

//***********************************************************************
//*                         Attributes                                  * 
//***********************************************************************

#include "backend/objCtor.h"

bool IMetaObjectAttribute::ContainType(const eValueTypes& valType) const
{
	return GetTypeDesc().ContainType(valType);
}

bool IMetaObjectAttribute::ContainType(const class_identifier_t& clsid) const
{
	return GetTypeDesc().ContainType(clsid);
}

bool IMetaObjectAttribute::EqualType(const class_identifier_t& clsid, const CTypeDescription& rhs) const
{
	return GetTypeDesc().EqualType(clsid, rhs);
}

bool IMetaObjectAttribute::ContainMetaType(eCtorMetaType type) const
{
	for (auto& clsid : GetTypeDesc().GetClsidList()) {
		IMetaValueTypeCtor* typeCtor = m_metaData->GetTypeCtor(clsid);
		if (typeCtor != nullptr && typeCtor->GetMetaTypeCtor() == type)
			return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////

eItemMode CMetaObjectAttribute::GetItemMode() const {
	IMetaObjectRecordDataFolderMutableRef* metaObject =
		dynamic_cast<IMetaObjectRecordDataFolderMutableRef*>(m_parent);
	if (metaObject != nullptr)
		return m_propertyItemMode->GetValueAsEnum();
	return eItemMode::eItemMode_Item;
}

eSelectMode CMetaObjectAttribute::GetSelectMode() const
{
	if (GetTypeDesc().GetClsidCount() > 1)
		return eSelectMode::eSelectMode_Items;
	IMetaValueTypeCtor* so = m_metaData->GetTypeCtor(GetTypeDesc().GetFirstClsid());
	if (so != nullptr) {
		IMetaObjectRecordDataFolderMutableRef* metaObject = dynamic_cast<IMetaObjectRecordDataFolderMutableRef*>(so->GetMetaObject());
		if (so->GetMetaTypeCtor() == eCtorMetaType::eCtorMetaType_Reference && metaObject != nullptr)
			return (eSelectMode)m_propertySelectMode->GetValueAsInteger();
		return eSelectMode::eSelectMode_Items;
	}
	return eSelectMode::eSelectMode_Items;
}

/////////////////////////////////////////////////////////////////////////

eSelectorDataType IMetaObjectAttribute::GetFilterDataType() const
{
	IMetaObjectGenericData* metaObject = dynamic_cast<IMetaObjectGenericData*>(m_parent);
	if (metaObject != nullptr) return metaObject->GetFilterDataType();
	return eSelectorDataType::eSelectorDataType_reference;
}

/////////////////////////////////////////////////////////////////////////

CValue IMetaObjectAttribute::CreateValue() const
{
	CValue* refData = CreateValueRef();
	if (refData == nullptr)
		return CValue();
	return refData;
}

CValue* IMetaObjectAttribute::CreateValueRef() const
{
	if (m_defValue.IsEmpty()) 
		return IBackendTypeConfigFactory::CreateValueRef();
	return new CValue(m_defValue);
}

//***********************************************************************
//*								Events								    *
//***********************************************************************

bool IMetaObjectAttribute::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	return IMetaObject::OnCreateMetaObject(metaData, flags);
}

bool IMetaObjectAttribute::OnDeleteMetaObject()
{
	return IMetaObject::OnDeleteMetaObject();
}

bool IMetaObjectAttribute::OnReloadMetaObject()
{
	IMetaObject* metaObject = GetParent();
	wxASSERT(metaObject);
	if (metaObject->OnReloadMetaObject())
		return IMetaObject::OnReloadMetaObject();
	return false;
}

///////////////////////////////////////////////////////////////////////////

bool IMetaObjectAttribute::OnBeforeRunMetaObject(int flags)
{
	return IMetaObject::OnBeforeRunMetaObject(flags);
}

bool IMetaObjectAttribute::OnAfterRunMetaObject(int flags)
{
	if ((flags & newObjectFlag) != 0 || (flags & pasteObjectFlag) != 0) OnReloadMetaObject();
	return IMetaObject::OnAfterRunMetaObject(flags);
}

//***********************************************************************
//*                               Data				                    *
//***********************************************************************

bool CMetaObjectAttribute::LoadData(CMemoryReader& reader)
{
	if (!m_propertyType->LoadData(reader))
		return false;

	m_propertyFillCheck->LoadData(reader);
	m_propertyItemMode->LoadData(reader);
	m_propertySelectMode->LoadData(reader);

	return true;
}

bool CMetaObjectAttribute::SaveData(CMemoryWriter& writer)
{
	if (!m_propertyType->SaveData(writer))
		return false;

	m_propertyFillCheck->SaveData(writer);
	m_propertyItemMode->SaveData(writer);
	m_propertySelectMode->SaveData(writer);
	return true;
}

bool CMetaObjectAttributeDefault::LoadData(CMemoryReader& reader)
{
	if (!CTypeDescriptionMemory::LoadData(reader, m_typeDesc))
		return false;

	m_fillCheck = reader.r_u8();
	return true;
}

bool CMetaObjectAttributeDefault::SaveData(CMemoryWriter& writer)
{
	if (!CTypeDescriptionMemory::SaveData(writer, m_typeDesc))
		return false;

	writer.w_u8(m_fillCheck);
	return true;
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CMetaObjectAttribute, "attribute", g_metaAttributeCLSID);
METADATA_TYPE_REGISTER(CMetaObjectAttributeDefault, "defaultAttribute", g_metaDefaultAttributeCLSID);