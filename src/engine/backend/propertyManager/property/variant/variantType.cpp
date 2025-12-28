#include "variantType.h"
#include "backend/metaData.h"

wxString wxVariantDataAttribute::MakeString() const
{
	wxString strDescr;
	if (m_ownerProperty != nullptr) {
		const IMetaData* metaData = m_ownerProperty->GetMetaData();
		wxASSERT(metaData);
		for (const auto clsid : m_typeDesc.GetClsidList()) {
			if (metaData->IsRegisterCtor(clsid) && strDescr.IsEmpty()) {
				strDescr = metaData->GetNameObjectFromID(clsid);
			}
			else if (metaData->IsRegisterCtor(clsid)) {
				strDescr = strDescr +
					wxT(", ") + metaData->GetNameObjectFromID(clsid);
			}
		}
	}
	return strDescr;
}

#include "backend/system/value/valueTable.h"

void wxVariantDataAttribute::DoSetDefaultMetaType() {
	if (m_ownerProperty != nullptr) {
		if (eSelectorDataType::eSelectorDataType_boolean == m_ownerProperty->GetFilterDataType()) {
			m_typeDesc.SetDefaultMetaType(g_valueBooleanCLSID);
		}
		else if (eSelectorDataType::eSelectorDataType_reference == m_ownerProperty->GetFilterDataType()) {
			m_typeDesc.SetDefaultMetaType(g_valueStringCLSID);
		}
		else if (eSelectorDataType::eSelectorDataType_resource == m_ownerProperty->GetFilterDataType()) {
			m_typeDesc.SetDefaultMetaType(g_valueNumberCLSID);
		}
		else if (eSelectorDataType::eSelectorDataType_table == m_ownerProperty->GetFilterDataType()) {
			m_typeDesc.SetDefaultMetaType(g_valueTableCLSID);
		}
		else {
			m_typeDesc.SetDefaultMetaType(g_valueStringCLSID);
		}
	}
}

void wxVariantDataAttribute::DoSetFromMetaId(const meta_identifier_t& id)
{
	if (m_ownerProperty != nullptr && id != wxNOT_FOUND) {

		const IMetaData* metaData = m_ownerProperty->GetMetaData();
		wxASSERT(metaData);

		IMetaObjectAttribute* attribute = metaData->FindAnyObjectByFilter<IMetaObjectAttribute>(id);
		if (attribute != nullptr && attribute->IsAllowed()) {
			m_typeDesc.SetDefaultMetaType(attribute->GetTypeDesc());
			return;
		}

		CMetaObjectTableData* metaTable = metaData->FindAnyObjectByFilter<CMetaObjectTableData>(id);
		if (metaTable != nullptr && metaTable->IsAllowed()) {
			m_typeDesc.SetDefaultMetaType(metaTable->GetTypeDesc());
			return;
		}

		SetDefaultMetaType();
	}
}

void wxVariantDataAttribute::DoSetFromTypeId(const CTypeDescription& td)
{
	m_typeDesc = td;
	RefreshTypeDesc();
}

void wxVariantDataAttribute::DoRefreshTypeDesc()
{
	if (m_ownerProperty != nullptr) {

		const IMetaData* metaData = m_ownerProperty->GetMetaData();
		wxASSERT(metaData);

		const unsigned int object_version = metaData->GetFactoryCountChanges();
		if (object_version != m_object_version) {

			for (const auto clsid : m_typeDesc.GetClsidList()) {

				if (!metaData->IsRegisterCtor(clsid))
					m_typeDesc.ClearMetaType(clsid);

				if (m_typeDesc.GetClsidCount() == 0)
					break;
			}

			m_object_version = object_version;
		}

		if (!m_typeDesc.IsOk()) SetDefaultMetaType();
	}
}