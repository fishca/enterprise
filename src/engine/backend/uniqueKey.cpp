#include "uniqueKey.h"
#include "metaCollection/partial/commonObject.h"

bool CUniqueKey::isValid() const
{
	return m_objGuid.isValid();
}

void CUniqueKey::reset()
{
	m_objGuid.reset();
}

bool CUniqueKey::operator>(const CUniqueKey& other) const
{
	return m_objGuid > other.m_objGuid;
}

bool CUniqueKey::operator>=(const CUniqueKey& other) const
{
	return m_objGuid >= other.m_objGuid;
}

bool CUniqueKey::operator<(const CUniqueKey& other) const
{
	return m_objGuid < other.m_objGuid;
}

bool CUniqueKey::operator<=(const CUniqueKey& other) const
{
	return m_objGuid <= other.m_objGuid;
}

bool CUniqueKey::operator==(const CUniqueKey& other) const
{
	if (m_uniqueData == enUniqueData::enUniqueGuid) {
		return m_objGuid == other.m_objGuid;
	}
	else if (m_uniqueData == enUniqueData::enUniqueKey) {
		if (other.m_uniqueData == enUniqueData::enUniqueKey) {
			if ((m_metaObject == nullptr && other.m_metaObject == nullptr) &&
				(m_keyValues.size() == 0 && other.m_keyValues.size() == 0))
				return m_objGuid == other.m_objGuid; // new object 
			return m_metaObject == other.m_metaObject &&
				m_keyValues == other.m_keyValues;
		}
		else if (m_uniqueData == enUniqueData::enUniqueGuid) {
			return m_objGuid == other.m_objGuid;
		}
	}
	return false;
}

bool CUniqueKey::operator!=(const CUniqueKey& other) const
{
	return !((*this) == other);
}

bool CUniqueKey::operator==(const CGuid& other) const
{
	return m_objGuid == other;
}

bool CUniqueKey::operator!=(const CGuid& other) const
{
	return m_objGuid != other;
}

CUniqueKey::CUniqueKey() : CUniqueKey(enUniqueData::enUniqueGuid)
{
	m_objGuid = wxNullGuid;
	m_metaObject = nullptr;
	m_keyValues = {};
}

CUniqueKey::CUniqueKey(const CGuid& guid) : CUniqueKey(enUniqueData::enUniqueGuid)
{
	m_objGuid = guid;
	m_metaObject = nullptr;
	m_keyValues = {};
}

CUniquePairKey::CUniquePairKey(const IValueMetaObjectRegisterData* metaObject) : CUniqueKey(enUniqueData::enUniqueKey)
{
	m_objGuid = wxNewUniqueGuid;
	m_metaObject = metaObject;
	m_keyValues = {};

	if (metaObject == nullptr) 
		return;

	for (const auto object : metaObject->GetGenericDimentionArrayObject()) {
		m_keyValues.insert_or_assign(
			object->GetMetaID(), object->CreateValue()
		);
	}
}

CUniquePairKey::CUniquePairKey(const IValueMetaObjectRegisterData* metaObject, const valueArray_t& keyValues) : CUniqueKey(enUniqueData::enUniqueKey)
{
	m_objGuid = wxNewUniqueGuid;
	m_metaObject = metaObject;
	m_keyValues = {};

	if (metaObject == nullptr) 
		return;

	for (const auto object : metaObject->GetGenericDimentionArrayObject()) {
		auto iterator = keyValues.find(object->GetMetaID());
		if (iterator != keyValues.end()) {
			m_keyValues.insert_or_assign(
				object->GetMetaID(), keyValues.at(object->GetMetaID())
			);
		}
	}
}