#include "sourceVariant.h"
#include "backend/metaData.h"

////////////////////////////////////////////////////////////////////////////

void wxVariantDataAttributeSource::DoSetFromMetaId(const meta_identifier_t& id)
{
	if (m_ownerSrcProperty != nullptr && id != wxNOT_FOUND) {
		const ISourceObject* srcData = m_ownerSrcProperty->GetSourceObject();
		if (srcData != nullptr) {
			const IMetaObjectSourceData* metaObject = srcData->GetSourceMetaObject();
			if (metaObject != nullptr && metaObject->IsAllowed() && id == metaObject->GetMetaID()) {
				m_typeDesc.SetDefaultMetaType(srcData->GetSourceClassType());
				return;
			}
		}
	}

	wxVariantDataAttribute::DoSetFromMetaId(id);
}

#include "backend/objCtor.h"

void wxVariantDataAttributeSource::DoRefreshTypeDesc()
{
	if (m_ownerSrcProperty != nullptr) {

		std::set<class_identifier_t> clear_list;

		const IMetaData* metaData = m_ownerSrcProperty->GetMetaData();
		wxASSERT(metaData);
		const ISourceObject* srcObject = m_ownerSrcProperty->GetSourceObject();

		for (auto clsid : m_typeDesc.GetClsidList()) {
			const IMetaValueTypeCtor* typeCtor = metaData->GetTypeCtor(clsid);
			if (typeCtor != nullptr && typeCtor->GetMetaTypeCtor() == eCtorMetaType_TabularSection) {
				if (srcObject != nullptr) {
					const IMetaObject* metaTable = typeCtor->GetMetaObject();
					if (metaTable == nullptr) clear_list.insert(clsid);
					else if (metaTable->GetParent() != srcObject->GetSourceMetaObject()) clear_list.insert(clsid);
				}
				else if (srcObject == nullptr)clear_list.insert(clsid);
			}
		}

		for (auto clsid : clear_list) { m_typeDesc.ClearMetaType(clsid); }
		if (!m_typeDesc.IsOk()) SetDefaultMetaType();
	}

	wxVariantDataAttribute::DoRefreshTypeDesc();
}

////////////////////////////////////////////////////////////////////////////

wxString wxVariantDataSource::MakeString() const
{
	if (!m_dataSource.isValid()) return _("<not selected>");
	if (m_ownerProperty != nullptr) {
		const ISourceObject* sourceObject = m_ownerProperty->GetSourceObject();
		if (sourceObject != nullptr) {
			const IMetaObjectSourceData* genericObject = sourceObject->GetSourceMetaObject();
			//wxASSERT(genericObject);
			const IMetaObject* metaObject = genericObject != nullptr && genericObject->IsAllowed() ?
				genericObject->FindMetaObjectByID(m_dataSource) : nullptr;
			if (metaObject != nullptr && !metaObject->IsAllowed()) return _("<not selected>");
			else if (metaObject == nullptr) return _("<not selected>");
			return metaObject->GetName();
		}
	}
	return _("<not selected>");
}

////////////////////////////////////////////////////////////////////////////

meta_identifier_t wxVariantDataSource::GetIdByGuid(const CGuid& guid) const
{
	const ISourceObject* sourceObject = m_ownerProperty->GetSourceObject();
	if (guid.isValid() && sourceObject != nullptr) {
		const IMetaObjectSourceData* genericObject = sourceObject->GetSourceMetaObject();
		//wxASSERT(genericObject);
		const IMetaObject* metaObject = genericObject != nullptr && genericObject->IsAllowed() ?
			genericObject->FindMetaObjectByID(guid) : nullptr;
		//wxASSERT(metaObject);
		return metaObject != nullptr &&
			metaObject->IsAllowed() ? metaObject->GetMetaID() : wxNOT_FOUND;
	}
	return wxNOT_FOUND;
}

CGuid wxVariantDataSource::GetGuidByID(const meta_identifier_t& id) const
{
	const ISourceObject* sourceObject = m_ownerProperty->GetSourceObject();
	if (id != wxNOT_FOUND && sourceObject != nullptr) {
		const IMetaObjectSourceData* genericObject = sourceObject->GetSourceMetaObject();
		//wxASSERT(objMetaValue);
		const IMetaObject* metaObject = genericObject != nullptr && genericObject->IsAllowed() ?
			genericObject->FindMetaObjectByID(id) : nullptr;
		//wxASSERT(metaObject);
		return metaObject != nullptr && metaObject->IsAllowed() ? metaObject->GetCommonGuid() : wxNullGuid;

	}
	return wxNullGuid;
}

////////////////////////////////////////////////////////////////////////////

IMetaObjectAttribute* wxVariantDataSource::GetSourceAttributeObject() const
{
	const ISourceObject* sourceObject = m_ownerProperty->GetSourceObject();
	if (m_dataSource.isValid() && sourceObject != nullptr) {
		const IMetaObjectSourceData* genericObject = sourceObject->GetSourceMetaObject();
		//wxASSERT(genericObject);
		return genericObject != nullptr && genericObject->IsAllowed() ?
			wxDynamicCast(genericObject->FindMetaObjectByID(m_dataSource), IMetaObjectAttribute) : nullptr;
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////

void wxVariantDataSource::SetSource(const meta_identifier_t& id, bool fillTypeDesc)
{
	const ISourceObject* sourceObject = m_ownerProperty->GetSourceObject();
	if (id != wxNOT_FOUND && sourceObject != nullptr) {
		const IMetaObjectSourceData* genericObject = sourceObject->GetSourceMetaObject();
		//wxASSERT(genericObject);
		const IMetaObject* metaObject = genericObject->FindMetaObjectByID(id);
		//wxASSERT(metaObject);
		m_dataSource = metaObject != nullptr && metaObject->IsAllowed() ?
			metaObject->GetGuid() : wxNullGuid;
	}
	else {
		m_dataSource.reset();
	}

	if (fillTypeDesc) m_attributeSource->SetFromMetaDesc(id);
}

meta_identifier_t wxVariantDataSource::GetSource() const
{
	return GetIdByGuid(m_dataSource);
}

////////////////////////////////////////////////////////////////////////////

void wxVariantDataSource::SetSourceGuid(const CGuid& guid, bool fillTypeDesc)
{
	const meta_identifier_t& id = GetIdByGuid(guid);
	m_dataSource = guid;
	if (fillTypeDesc) m_attributeSource->SetFromMetaDesc(id);
}

CGuid wxVariantDataSource::GetSourceGuid() const
{
	const ISourceObject* sourceObject = m_ownerProperty->GetSourceObject();
	if (m_dataSource.isValid() && sourceObject != nullptr) {
		const IMetaObjectSourceData* genericObject = sourceObject->GetSourceMetaObject();
		if (genericObject == nullptr) return wxNullGuid;
		const IMetaObject* metaObject = genericObject ? genericObject->FindMetaObjectByID(m_dataSource) : nullptr;
		//wxASSERT(metaObject);
		return metaObject != nullptr && metaObject->IsAllowed() ?
			metaObject->GetCommonGuid() : wxNullGuid;
	}
	return wxNullGuid;
}

////////////////////////////////////////////////////////////////////////////

void wxVariantDataSource::SetSourceTypeDesc(const CTypeDescription& td)
{
	ResetSource();
	m_attributeSource->SetFromTypeDesc(td);
}

CTypeDescription& wxVariantDataSource::GetSourceTypeDesc(bool fillTypeDesc) const
{
	return m_attributeSource->GetTypeDesc();
}

////////////////////////////////////////////////////////////////////////////

void wxVariantDataSource::ResetSource()
{
	m_attributeSource->SetFromMetaDesc(wxNOT_FOUND);

	if (m_dataSource.isValid()) {
		wxASSERT(m_attributeSource->GetTypeDesc().GetClsidCount() > 0);
		m_dataSource.reset();
	}
}

////////////////////////////////////////////////////////////////////////////

bool wxVariantDataSource::IsPropAllowed() const
{
	const ISourceObject* sourceObject = m_ownerProperty->GetSourceObject();
	if (m_dataSource.isValid() && sourceObject != nullptr) {
		const IMetaObjectSourceData* genericObject = sourceObject->GetSourceMetaObject();
		if (genericObject == nullptr) return true;
		const IMetaObject* metaObject = genericObject ? genericObject->FindMetaObjectByID(m_dataSource) : nullptr;
		//wxASSERT(metaObject);
		if (metaObject != nullptr)
			return !metaObject->IsAllowed();
		return true;
	}
	return true;
}
