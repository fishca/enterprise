#include "metaData.h"
#include "backend/objCtor.h"

CValue* IMetaData::CreateObjectRef(const class_identifier_t& clsid, CValue** paParams, const long lSizeArray) const
{
	const IMetaValueTypeCtor* typeCtor = GetTypeCtor(clsid);

	if (typeCtor != nullptr) {

		CValue* newObject = typeCtor->CreateObject();
		wxASSERT(newObject);

		if (newObject == nullptr) return nullptr;

		bool succes = true;
		if (lSizeArray > 0)
			succes = newObject->Init(paParams, lSizeArray);
		else
			succes = newObject->Init();

		if (!succes) {
			wxDELETE(newObject);
			CBackendException::Error("Error initializing object '%s'", typeCtor->GetClassName());
			return nullptr;
		}

		newObject->PrepareNames();
		return newObject;
	}

	return CValue::CreateObjectRef(clsid, paParams, lSizeArray);
}

void IMetaData::RegisterCtor(IMetaValueTypeCtor* typeCtor)
{
	wxASSERT(typeCtor->GetClassType() > 0);

	if (typeCtor != nullptr) {

		if (IMetaData::IsRegisterCtor(typeCtor->GetClassType())) {
			CBackendException::Error("Object '%s' is exist", typeCtor->GetClassName());
		}

#ifdef DEBUG
		wxLogDebug("* Register class '%s' with clsid '%s:%llu' ", typeCtor->GetClassName(), clsid_to_string(typeCtor->GetClassType()), typeCtor->GetClassType());
#endif

		typeCtor->CallEvent(eCtorObjectTypeEvent::eCtorObjectTypeEvent_Register);
		m_factoryCtors.emplace(typeCtor);

		m_factoryCtorCountChanges++;
	}
}

void IMetaData::UnRegisterCtor(IMetaValueTypeCtor*& typeCtor)
{
	if (typeCtor != nullptr && IMetaData::IsRegisterCtor(typeCtor->GetClassType())) {

		typeCtor->CallEvent(eCtorObjectTypeEvent::eCtorObjectTypeEvent_UnRegister);

#ifdef DEBUG
		wxLogDebug("* Unregister class '%s' with clsid '%s:%llu' ", typeCtor->GetClassName(), clsid_to_string(typeCtor->GetClassType()), typeCtor->GetClassType());
#endif
		m_factoryCtors.erase(typeCtor);
		wxDELETE(typeCtor);

		m_factoryCtorCountChanges++;
	}
	else {
		CBackendException::Error("Object '%s' is not exist", typeCtor->GetClassName());
	}
}

void IMetaData::UnRegisterCtor(const wxString& className)
{
	IMetaValueTypeCtor* typeCtor = GetTypeCtor(className);
	if (typeCtor != nullptr) {

		typeCtor->CallEvent(eCtorObjectTypeEvent::eCtorObjectTypeEvent_UnRegister);

#ifdef DEBUG
		wxLogDebug("* Unregister class '%s' with clsid '%s:%llu' ", typeCtor->GetClassName(), clsid_to_string(typeCtor->GetClassType()), typeCtor->GetClassType());
#endif
		m_factoryCtors.erase(typeCtor);
		wxDELETE(typeCtor);

		m_factoryCtorCountChanges++;
	}
	else {
		CBackendException::Error("Object '%s' is not exist", className);
	}
}

bool IMetaData::IsRegisterCtor(const wxString& className) const
{
	for (auto& typeCtor : m_factoryCtors)
		if (stringUtils::CompareString(className, typeCtor->GetClassName()))
			return true;
	return CValue::IsRegisterCtor(className);
}

bool IMetaData::IsRegisterCtor(const wxString& className, eCtorObjectType objectType) const
{
	for (auto& typeCtor : m_factoryCtors)
		if (stringUtils::CompareString(className, typeCtor->GetClassName()) && (objectType == typeCtor->GetObjectTypeCtor()))
			return true;
	return CValue::IsRegisterCtor(className, objectType);
}

bool IMetaData::IsRegisterCtor(const wxString& className, eCtorObjectType objectType, eCtorMetaType refType) const
{
	for (auto& typeCtor : m_factoryCtors)
		if (stringUtils::CompareString(className, typeCtor->GetClassName())
			&& (eCtorObjectType::eCtorObjectType_object_meta_value == typeCtor->GetObjectTypeCtor()
				&& refType == typeCtor->GetMetaTypeCtor()))
			return true;
	return CValue::IsRegisterCtor(className, objectType);
}

bool IMetaData::IsRegisterCtor(const class_identifier_t& clsid) const
{
	for (auto& typeCtor : m_factoryCtors)
		if (clsid == typeCtor->GetClassType())
			return true;
	return CValue::IsRegisterCtor(clsid);
}

class_identifier_t IMetaData::GetIDObjectFromString(const wxString& className) const
{
	const IMetaValueTypeCtor* typeCtor = GetTypeCtor(className);
	if (typeCtor != nullptr)
		return typeCtor->GetClassType();
	return CValue::GetIDObjectFromString(className);
}

wxString IMetaData::GetNameObjectFromID(const class_identifier_t& clsid, bool upper) const
{
	const IMetaValueTypeCtor* typeCtor = GetTypeCtor(clsid);
	if (typeCtor != nullptr)
		return upper ? typeCtor->GetClassName().Upper() : typeCtor->GetClassName();
	return CValue::GetNameObjectFromID(clsid, upper);
}

meta_identifier_t IMetaData::GetVTByID(const class_identifier_t& clsid) const
{
	const IMetaValueTypeCtor* typeCtor = GetTypeCtor(clsid);
	if (typeCtor != nullptr) {
		IMetaObject* metaValue = typeCtor->GetMetaObject();
		wxASSERT(metaValue);
		return metaValue->GetMetaID();
	}
	return CValue::GetVTByID(clsid);
}

class_identifier_t IMetaData::GetIDByVT(const meta_identifier_t& valueType, eCtorMetaType refType) const
{
	for (auto& typeCtor : m_factoryCtors) {
		const IMetaObject* metaValue = typeCtor->GetMetaObject();
		wxASSERT(metaValue);
		if (refType == typeCtor->GetMetaTypeCtor() && valueType == metaValue->GetMetaID())
			return typeCtor->GetClassType();
	}
	return CValue::GetIDByVT(static_cast<eValueTypes>(valueType));
}

IMetaValueTypeCtor* IMetaData::GetTypeCtor(const wxString& className) const
{
	for (auto& typeCtor : m_factoryCtors)
		if (stringUtils::CompareString(className, typeCtor->GetClassName()))
			return typeCtor;
	return nullptr;
}

IMetaValueTypeCtor* IMetaData::GetTypeCtor(const class_identifier_t& clsid) const
{
	for (auto& typeCtor : m_factoryCtors)
		if (clsid == typeCtor->GetClassType())
			return typeCtor;
	return nullptr;
}

IMetaValueTypeCtor* IMetaData::GetTypeCtor(const IMetaObject* metaValue, eCtorMetaType refType) const
{
	for (auto& typeCtor : m_factoryCtors)
		if (refType == typeCtor->GetMetaTypeCtor() && metaValue == typeCtor->GetMetaObject())
			return typeCtor;
	return nullptr;
}

IAbstractTypeCtor* IMetaData::GetAvailableCtor(const wxString& className) const
{
	for (auto& typeCtor : m_factoryCtors)
		if (stringUtils::CompareString(className, typeCtor->GetClassName()))
			return typeCtor;
	return CValue::GetAvailableCtor(className);
}

IAbstractTypeCtor* IMetaData::GetAvailableCtor(const class_identifier_t& clsid) const
{
	for (auto& typeCtor : m_factoryCtors)
		if (clsid == typeCtor->GetClassType())
			return typeCtor;
	
	return CValue::GetAvailableCtor(clsid);
}

std::vector<IMetaValueTypeCtor*> IMetaData::GetListCtorsByType() const
{
	std::vector<IMetaValueTypeCtor*> retVector;
	std::copy(m_factoryCtors.begin(), m_factoryCtors.end(), std::back_inserter(retVector));
	std::sort(retVector.begin(), retVector.end(), [](IMetaValueTypeCtor* a, IMetaValueTypeCtor* b) {
		IMetaObject* ma = a->GetMetaObject(); IMetaObject* mb = b->GetMetaObject();
		return ma->GetName() > mb->GetName() &&
			a->GetMetaTypeCtor() > b->GetMetaTypeCtor();
		}
	);
	
	return retVector;
}

std::vector<IMetaValueTypeCtor*> IMetaData::GetListCtorsByType(const class_identifier_t& clsid, eCtorMetaType refType) const
{
	std::vector<IMetaValueTypeCtor*> retVector;
	std::copy_if(m_factoryCtors.begin(), m_factoryCtors.end(), std::back_inserter(retVector), [clsid, refType](IMetaValueTypeCtor* t) {
		IMetaObject* const m = t->GetMetaObject();
		return refType == t->GetMetaTypeCtor() &&
			clsid == m->GetClassType(); }
	);
	std::sort(retVector.begin(), retVector.end(), [](IMetaValueTypeCtor* a, IMetaValueTypeCtor* b) {
		IMetaObject* ma = a->GetMetaObject(); IMetaObject* mb = b->GetMetaObject();
		return ma->GetName() > mb->GetName() &&
			a->GetMetaTypeCtor() > b->GetMetaTypeCtor();
		}
	);
	return retVector;
}

std::vector<IMetaValueTypeCtor*> IMetaData::GetListCtorsByType(eCtorMetaType refType) const
{
	std::vector<IMetaValueTypeCtor*> retVector;
	std::copy_if(m_factoryCtors.begin(), m_factoryCtors.end(), std::back_inserter(retVector), [refType](IMetaValueTypeCtor* t) { return refType == t->GetMetaTypeCtor(); });
	std::sort(retVector.begin(), retVector.end(), [](IMetaValueTypeCtor* a, IMetaValueTypeCtor* b) {
		IMetaObject* ma = a->GetMetaObject(); IMetaObject* mb = b->GetMetaObject();
		return ma->GetName() > mb->GetName() &&
			a->GetMetaTypeCtor() > b->GetMetaTypeCtor();
		}
	);
	return retVector;
}