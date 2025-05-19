////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : reference - db
////////////////////////////////////////////////////////////////////////////

#include "reference.h"
#include "backend/appData.h"
#include "backend/metaCollection/partial/commonObject.h"
#include "backend/metaCollection/partial/tabularSection/tabularSection.h"
#include "backend/databaseLayer/databaseLayer.h"


bool CReferenceDataObject::ReadData(bool createData)
{
	if (m_metaObject == nullptr || !m_objGuid.isValid())
		return false;

	const wxString& tableName = m_metaObject->GetTableNameDB();

	if (db_query->TableExists(tableName)) {
		IDatabaseResultSet* resultSet = nullptr;	
		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
			resultSet = db_query->RunQueryWithResults("SELECT * FROM %s WHERE uuid = '%s' LIMIT 1;", tableName, m_objGuid.str());
		else
			resultSet = db_query->RunQueryWithResults("SELECT FIRST 1 * FROM %s WHERE uuid = '%s';", tableName, m_objGuid.str());
		if (resultSet == nullptr)
			return false;
		bool readRef = false;
		if (resultSet->Next()) {			
			//load attributes 
			for (auto& obj : m_metaObject->GetGenericAttributes()) {
				if (!m_metaObject->IsDataReference(obj->GetMetaID())) {
					IMetaObjectAttribute::GetValueAttribute(
						obj,
						m_listObjectValue[obj->GetMetaID()],
						resultSet,
						createData
					);
				}
			}
			// table is collection values 
			//for (auto& obj : m_metaObject->GetObjectTables()) {
			//	m_listObjectValue.insert_or_assign(obj->GetMetaID(), 
			//		m_metaObject->GetMetaData()->CreateObjectValue<CTabularSectionDataObjectRef>(this, obj, true));
			//}

			readRef = true;
		}	
		resultSet->Close();
		return readRef;
	}
	return false;
}

bool CReferenceDataObject::FindValue(const wxString& findData, std::vector<CValue>& listValue) const
{
	class CObjectComparatorValue : public IValueDataObject {
		bool ReadValues() {
			if (m_metaObject == nullptr || m_newObject)
				return false;
			const wxString& tableName = m_metaObject->GetTableNameDB();
			if (db_query->TableExists(tableName)) {

				IDatabaseResultSet* resultSet = nullptr;
				if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL)
					resultSet = db_query->RunQueryWithResults("SELECT * FROM " + tableName + " WHERE uuid = '" + m_objGuid.str() + "' LIMIT 1;");
				else
					resultSet = db_query->RunQueryWithResults("SELECT FIRST 1 * FROM " + tableName + " WHERE uuid = '" + m_objGuid.str() + "';");

				if (!resultSet)
					return false;
				if (resultSet->Next()) {
					//load other attributes 
					for (auto& obj : m_metaObject->GetGenericAttributes()) {
						if (m_metaObject->IsDataReference(obj->GetMetaID()))
							continue;
						IMetaObjectAttribute::GetValueAttribute(obj, m_listObjectValue[obj->GetMetaID()], resultSet);
					}
				}
				resultSet->Close();
				return true;
			}
			return false;
		}
		IMetaObjectRecordDataRef* m_metaObject;
	private:
		CObjectComparatorValue(IMetaObjectRecordDataRef* metaObject, const Guid& guid) : IValueDataObject(guid, false), m_metaObject(metaObject) {}
	public:

		static bool CompareValue(const wxString& findData, IMetaObjectRecordDataRef* metaObject, const Guid& guid) {
			bool allow = false;
			CObjectComparatorValue* comparator = new CObjectComparatorValue(metaObject, guid);
			if (comparator->ReadValues()) {
				for (auto& obj : metaObject->GetSearchedAttributes()) {
					const wxString& fieldCompare = comparator->m_listObjectValue[obj->GetMetaID()].GetString();
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
		virtual IMetaObjectRecordData* GetMetaObject() const {
			return m_metaObject;
		}
	};
	const wxString& tableName = m_metaObject->GetTableNameDB();
	if (db_query->TableExists(tableName)) {
		IDatabaseResultSet* resultSet = db_query->RunQueryWithResults("SELECT * FROM %s ORDER BY uuid; ", tableName);
		if (resultSet == nullptr)
			return false;
		while (resultSet->Next()) {
			const Guid& currentGuid = resultSet->GetResultString(guidName);
			if (CObjectComparatorValue::CompareValue(findData, m_metaObject, currentGuid)) {
				listValue.push_back(
					CReferenceDataObject::Create(m_metaObject, currentGuid)
				);
			}
		}
		std::sort(listValue.begin(), listValue.end(), [](const CValue& a, const CValue& b) { return a.GetString() < b.GetString(); });
		resultSet->Close();
		return listValue.size() > 0;
	}

	return false;
}