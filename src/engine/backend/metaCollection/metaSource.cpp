#include "metaSource.h"
#include "backend/metadata.h"

wxIMPLEMENT_ABSTRACT_CLASS(IMetaObjectSourceData, IMetaObject);

//***********************************************************************
//*							IMetaObjectSourceData				        *
//***********************************************************************

IMetaValueTypeCtor* IMetaObjectSourceData::GetTypeCtor(const eCtorMetaType &refType) const
{
	return m_metaData->GetTypeCtor(this, refType);
}

meta_identifier_t IMetaObjectSourceData::GetIdByGuid(const CGuid& guid) const
{
	IMetaObject* metaObject = FindMetaObjectByID(guid);
	if (metaObject != nullptr)
		return metaObject->GetMetaID();
	return wxNOT_FOUND;
}

CGuid IMetaObjectSourceData::GetGuidByID(const meta_identifier_t& id) const
{
	IMetaObject* metaObject = FindMetaObjectByID(id);
	if (metaObject != nullptr)
		return metaObject->GetGuid();
	return wxNullGuid;
}

//////////////////////////////////////////////////////////////////////////////

IMetaObject* IMetaObjectSourceData::FindMetaObjectByID(const meta_identifier_t& id) const
{
	return m_metaData->GetMetaObject(id,
		const_cast<IMetaObjectSourceData*>(this));
}

IMetaObject* IMetaObjectSourceData::FindMetaObjectByID(const CGuid& guid) const
{
	return m_metaData->GetMetaObject(guid,
		const_cast<IMetaObjectSourceData*>(this));
}
