#include "uniqueKey.h"
#include "metaCollection/partial/commonObject.h"

bool ibUniqueKey::isValid() const
{
	return m_objGuid.isValid();
}

void ibUniqueKey::reset()
{
	m_objGuid.reset();
}

bool ibUniqueKey::operator>(const ibUniqueKey& other) const
{
	return m_objGuid > other.m_objGuid;
}

bool ibUniqueKey::operator>=(const ibUniqueKey& other) const
{
	return m_objGuid >= other.m_objGuid;
}

bool ibUniqueKey::operator<(const ibUniqueKey& other) const
{
	return m_objGuid < other.m_objGuid;
}

bool ibUniqueKey::operator<=(const ibUniqueKey& other) const
{
	return m_objGuid <= other.m_objGuid;
}

bool ibUniqueKey::operator==(const ibUniqueKey& other) const
{
	if (m_uniqueData == ibUniqueData::enUniqueGuid) {
		return m_objGuid == other.m_objGuid;
	}
	else if (m_uniqueData == ibUniqueData::enUniqueKey) {
		if (other.m_uniqueData == ibUniqueData::enUniqueKey) {
			if ((m_metaObject == nullptr && other.m_metaObject == nullptr) &&
				(m_keyValues.size() == 0 && other.m_keyValues.size() == 0))
				return m_objGuid == other.m_objGuid; // new object 
			return m_metaObject == other.m_metaObject &&
				m_keyValues == other.m_keyValues;
		}
		else if (m_uniqueData == ibUniqueData::enUniqueGuid) {
			return m_objGuid == other.m_objGuid;
		}
	}
	return false;
}

bool ibUniqueKey::operator!=(const ibUniqueKey& other) const
{
	return !((*this) == other);
}

bool ibUniqueKey::operator==(const ibGuid& other) const
{
	return m_objGuid == other;
}

bool ibUniqueKey::operator!=(const ibGuid& other) const
{
	return m_objGuid != other;
}

ibUniqueKey::ibUniqueKey() : ibUniqueKey(ibUniqueData::enUniqueGuid)
{
	m_objGuid = wxNullGuid;
	m_metaObject = nullptr;
	m_keyValues = {};
}

ibUniqueKey::ibUniqueKey(const ibGuid& guid) : ibUniqueKey(ibUniqueData::enUniqueGuid)
{
	m_objGuid = guid;
	m_metaObject = nullptr;
	m_keyValues = {};
}

ibUniqueKeyPair::ibUniqueKeyPair(const ibValueMetaObjectRegisterData* metaObject) : ibUniqueKey(ibUniqueData::enUniqueKey)
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

ibUniqueKeyPair::ibUniqueKeyPair(const ibValueMetaObjectRegisterData* metaObject, const ibMetaValueArray& keyValues) : ibUniqueKey(ibUniqueData::enUniqueKey)
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