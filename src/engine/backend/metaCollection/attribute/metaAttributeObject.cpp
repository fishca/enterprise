////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : meta-attribues
////////////////////////////////////////////////////////////////////////////

#include "metaAttributeObject.h"
#include "backend/metadata.h"

////////////////////////////////////////////////////////////////////////////

wxIMPLEMENT_ABSTRACT_CLASS(IValueMetaObjectAttribute, IValueMetaObject);

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectAttribute, IValueMetaObjectAttribute);
wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectAttributePredefined, IValueMetaObjectAttribute);

//***********************************************************************
//*                         Attributes                                  * 
//***********************************************************************

#include "backend/objCtor.h"

bool IValueMetaObjectAttribute::ContainType(const eValueTypes& valType) const
{
	return GetTypeDesc().ContainType(valType);
}

bool IValueMetaObjectAttribute::ContainType(const class_identifier_t& clsid) const
{
	return GetTypeDesc().ContainType(clsid);
}

bool IValueMetaObjectAttribute::EqualType(const class_identifier_t& clsid, const CTypeDescription& rhs) const
{
	return GetTypeDesc().EqualType(clsid, rhs);
}

bool IValueMetaObjectAttribute::ContainMetaType(eCtorMetaType type) const
{
	for (auto& clsid : GetTypeDesc().GetClsidList()) {
		const IMetaValueTypeCtor* typeCtor = m_metaData->GetTypeCtor(clsid);
		if (typeCtor != nullptr && typeCtor->GetMetaTypeCtor() == type)
			return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////

eItemMode CValueMetaObjectAttribute::GetItemMode() const {
	IValueMetaObjectRecordDataHierarchyMutableRef* metaObject =
		dynamic_cast<IValueMetaObjectRecordDataHierarchyMutableRef*>(m_parent);
	if (metaObject != nullptr)
		return m_propertyItemMode->GetValueAsEnum();
	return eItemMode::eItemMode_Item;
}

eSelectMode CValueMetaObjectAttribute::GetSelectMode() const
{
	if (GetTypeDesc().GetClsidCount() > 1)
		return eSelectMode::eSelectMode_Items;
	const IMetaValueTypeCtor* so = m_metaData->GetTypeCtor(GetTypeDesc().GetFirstClsid());
	if (so != nullptr) {
		IValueMetaObjectRecordDataHierarchyMutableRef* metaObject = dynamic_cast<IValueMetaObjectRecordDataHierarchyMutableRef*>(so->GetMetaObject());
		if (so->GetMetaTypeCtor() == eCtorMetaType::eCtorMetaType_Reference && metaObject != nullptr)
			return (eSelectMode)m_propertySelectMode->GetValueAsInteger();
		return eSelectMode::eSelectMode_Items;
	}
	return eSelectMode::eSelectMode_Items;
}

/////////////////////////////////////////////////////////////////////////

eSelectorDataType IValueMetaObjectAttribute::GetFilterDataType() const
{
	IValueMetaObjectGenericData* metaObject = dynamic_cast<IValueMetaObjectGenericData*>(m_parent);
	if (metaObject != nullptr) return metaObject->GetFilterDataType();
	return eSelectorDataType::eSelectorDataType_reference;
}

/////////////////////////////////////////////////////////////////////////

CValue IValueMetaObjectAttribute::CreateValue() const
{
	CValue* refData = CreateValueRef();
	if (refData == nullptr)
		return CValue();
	return refData;
}

CValue* IValueMetaObjectAttribute::CreateValueRef() const
{
	if (m_defValue.IsEmpty()) 
		return IBackendTypeConfigFactory::CreateValueRef();
	return new CValue(m_defValue);
}

//***********************************************************************
//*								Events								    *
//***********************************************************************

bool IValueMetaObjectAttribute::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	return IValueMetaObject::OnCreateMetaObject(metaData, flags);
}

bool IValueMetaObjectAttribute::OnDeleteMetaObject()
{
	return IValueMetaObject::OnDeleteMetaObject();
}

bool IValueMetaObjectAttribute::OnReloadMetaObject()
{
	IValueMetaObject* metaObject = GetParent();
	wxASSERT(metaObject);
	if (metaObject->OnReloadMetaObject())
		return IValueMetaObject::OnReloadMetaObject();
	return false;
}

///////////////////////////////////////////////////////////////////////////

bool IValueMetaObjectAttribute::OnBeforeRunMetaObject(int flags)
{
	return IValueMetaObject::OnBeforeRunMetaObject(flags);
}

bool IValueMetaObjectAttribute::OnAfterRunMetaObject(int flags)
{
	if ((flags & newObjectFlag) != 0 || (flags & pasteObjectFlag) != 0) OnReloadMetaObject();
	return IValueMetaObject::OnAfterRunMetaObject(flags);
}

//***********************************************************************
//*                               Data				                    *
//***********************************************************************

bool CValueMetaObjectAttribute::LoadData(CMemoryReader& reader)
{
	if (!m_propertyType->LoadData(reader))
		return false;

	m_propertyFillCheck->LoadData(reader);
	m_propertyItemMode->LoadData(reader);
	m_propertySelectMode->LoadData(reader);

	return true;
}

bool CValueMetaObjectAttribute::SaveData(CMemoryWriter& writer)
{
	if (!m_propertyType->SaveData(writer))
		return false;

	m_propertyFillCheck->SaveData(writer);
	m_propertyItemMode->SaveData(writer);
	m_propertySelectMode->SaveData(writer);
	return true;
}

bool CValueMetaObjectAttributePredefined::LoadData(CMemoryReader& reader)
{
	if (!CTypeDescriptionMemory::LoadData(reader, m_typeDesc))
		return false;

	m_fillCheck = reader.r_u8();
	return true;
}

bool CValueMetaObjectAttributePredefined::SaveData(CMemoryWriter& writer)
{
	if (!CTypeDescriptionMemory::SaveData(writer, m_typeDesc))
		return false;

	writer.w_u8(m_fillCheck);
	return true;
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CValueMetaObjectAttribute, "attribute", g_metaAttributeCLSID);
METADATA_TYPE_REGISTER(CValueMetaObjectAttributePredefined, "predefinedAttribute", g_metaPredefinedAttributeCLSID);