////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : reference - db
////////////////////////////////////////////////////////////////////////////

#include "reference.h"
#include "backend/appData.h"
#include "backend/session/session.h"
#include "backend/metaCollection/partial/commonObject.h"
#include "backend/metaCollection/partial/tabularSection/tabularSection.h"
#include "backend/databaseLayer/databaseLayer.h"


bool ibValueReferenceDataObject::ReadData(bool createData)
{
	if (m_metaObject == nullptr || !m_objGuid.isValid())
		return false;

	// Cache the conn locally — every ses_query call Acquires a fresh
	// shared_ptr; multiple ses_query->X(...) in one function would each
	// route through Checkout and pin a different pool entry, leaving rs
	// orphaned on a conn that the next Close runs on the wrong handle.
	const auto db = ses_query;
	const wxString& tableName = m_metaObject->GetTableNameDB();

	if (db->TableExists(tableName)) {
		ibDatabaseResultSet* resultSet = (db->GetDatabaseLayerType() == DATABASELAYER_FIREBIRD)
			? db->RunQueryWithResults("SELECT FIRST 1 * FROM %s WHERE uuid = '%s';", tableName, m_objGuid.str())
			: db->RunQueryWithResults("SELECT * FROM %s WHERE uuid = '%s' LIMIT 1;", tableName, m_objGuid.str());
		if (resultSet == nullptr)
			return false;
		bool readRef = false;
		if (resultSet->Next()) {
			for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
				if (!m_metaObject->IsDataReference(object->GetMetaID())) {
					ibValueMetaObjectAttributeBase::GetValueAttribute(
						object,
						m_listObjectValue[object->GetMetaID()],
						resultSet,
						createData
					);
				}
			}
			readRef = true;
		}
		db->CloseResultSet(resultSet);
		return readRef;
	}
	return false;
}

bool ibValueReferenceDataObject::FindValue(const wxString& findData, std::vector<ibValue>& listValue) const
{
	class ibValueDataObjectComparator : public ibValueDataObject {
		bool ReadValues() {
			if (m_metaObject == nullptr || m_newObject)
				return false;
			const auto db = ses_query;
			const wxString& tableName = m_metaObject->GetTableNameDB();
			if (db->TableExists(tableName)) {
				ibDatabaseResultSet* resultSet = (db->GetDatabaseLayerType() == DATABASELAYER_FIREBIRD)
					? db->RunQueryWithResults("SELECT FIRST 1 * FROM " + tableName + " WHERE uuid = '" + m_objGuid.str() + "';")
					: db->RunQueryWithResults("SELECT * FROM " + tableName + " WHERE uuid = '" + m_objGuid.str() + "' LIMIT 1;");
				if (!resultSet)
					return false;
				if (resultSet->Next()) {
					for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
						if (m_metaObject->IsDataReference(object->GetMetaID()))
							continue;
						ibValueMetaObjectAttributeBase::GetValueAttribute(object, m_listObjectValue[object->GetMetaID()], resultSet);
					}
				}
				db->CloseResultSet(resultSet);
				return true;
			}
			return false;
		}
		const ibValueMetaObjectRecordDataRef* m_metaObject;
	private:
		ibValueDataObjectComparator(const ibValueMetaObjectRecordDataRef* metaObject, const ibGuid& guid) : ibValueDataObject(guid, false), m_metaObject(metaObject) {}
	public:

		static bool CompareValue(const wxString& findData, const ibValueMetaObjectRecordDataRef* metaObject, const ibGuid& guid) {
			bool allow = false;
			ibValueDataObjectComparator* comparator = new ibValueDataObjectComparator(metaObject, guid);
			if (comparator->ReadValues()) {
				for (const auto object : metaObject->GetSearchedAttributeObjectArray()) {
					const wxString& fieldCompare = comparator->m_listObjectValue[object->GetMetaID()].GetString();
					if (fieldCompare.Contains(findData)) {
						allow = true; break;
					}
				}
			}
			if (!allow) {
				const wxString& fieldCompare = metaObject->GetDataPresentation(comparator);
				if (fieldCompare.Contains(findData)) {
					allow = true;
				}
			}
			wxDELETE(comparator);
			return allow;
		}

		//get metaData from object
		virtual const ibValueMetaObjectRecordData* GetMetaObject() const {
			return m_metaObject;
		}
	};
	const auto db = ses_query;
	const wxString& tableName = m_metaObject->GetTableNameDB();
	if (db->TableExists(tableName)) {
		ibDatabaseResultSet* resultSet = db->RunQueryWithResults("SELECT * FROM %s ORDER BY uuid; ", tableName);
		if (resultSet == nullptr)
			return false;
		while (resultSet->Next()) {
			const ibGuid& currentGuid = resultSet->GetResultString(guidName);
			if (ibValueDataObjectComparator::CompareValue(findData, m_metaObject, currentGuid)) {
				listValue.push_back(
					ibValueReferenceDataObject::Create(m_metaObject, currentGuid)
				);
			}
		}
		std::sort(listValue.begin(), listValue.end(), [](const ibValue& a, const ibValue& b) { return a.GetString() < b.GetString(); });
		db->CloseResultSet(resultSet);
		return listValue.size() > 0;
	}

	return false;
}