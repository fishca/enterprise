////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : common factory module 
////////////////////////////////////////////////////////////////////////////

#include "value.h"
#include "backend/backend_exception.h"

static std::vector<IAbstractTypeCtor*>* s_factoryCtors = nullptr;
static std::atomic<unsigned int> s_factoryCtorCountChanges = 0;

//*******************************************************************************
//*                      Support dynamic object                                 *
//*******************************************************************************

CValue* CValue::CreateObjectRef(const class_identifier_t& clsid, CValue** paParams, const long lSizeArray)
{
	const IAbstractTypeCtor* typeCtor = GetAvailableCtor(clsid);

	if (typeCtor != nullptr) {
		CValue* created_value = typeCtor->CreateObject();
		wxASSERT(created_value);
		if (typeCtor->GetObjectTypeCtor() != eCtorObjectType::eCtorObjectType_object_system) {
			bool succes = true;
			if (lSizeArray > 0)
				succes = created_value->Init(paParams, lSizeArray);
			else
				succes = created_value->Init();
			if (!succes) {
				wxDELETE(created_value);
				CBackendCoreException::Error(_("Error initializing object '%s'"), typeCtor->GetClassName());
			}
			created_value->PrepareNames();
		}
		return created_value;
	}
	else {
		CBackendCoreException::Error(_("Error creating object '%llu'"), clsid);
	}

	return nullptr;
}

void CValue::RegisterCtor(IAbstractTypeCtor* typeCtor)
{
	if (s_factoryCtors == nullptr) s_factoryCtors = new std::vector<IAbstractTypeCtor*>;

	if (typeCtor != nullptr) {

		if (CValue::IsRegisterCtor(typeCtor->GetClassType())) {
			CBackendCoreException::Error(_("Object '%s' is exist"), typeCtor->GetClassName());
		}
		else if (CValue::IsRegisterCtor(typeCtor->GetClassName())) {
			CBackendCoreException::Error(_("Object '%s' is exist"), typeCtor->GetClassName());
		}

#ifdef DEBUG
		wxLogDebug(wxT("* Register class '%s' with clsid '%s:%llu' "), typeCtor->GetClassName(), clsid_to_string(typeCtor->GetClassType()), typeCtor->GetClassType());
#endif

		s_factoryCtorCountChanges++;

		typeCtor->CallEvent(eCtorObjectTypeEvent::eCtorObjectTypeEvent_Register);
		s_factoryCtors->emplace_back(typeCtor);
	}
}

void CValue::UnRegisterCtor(IAbstractTypeCtor*& typeCtor)
{
	if (typeCtor != nullptr && IsRegisterCtor(typeCtor->GetClassType())) {

		typeCtor->CallEvent(eCtorObjectTypeEvent::eCtorObjectTypeEvent_UnRegister);

#ifdef DEBUG
		wxLogDebug(wxT("* Unregister class '%s' with clsid '%s:%llu' "), typeCtor->GetClassName(), clsid_to_string(typeCtor->GetClassType()), typeCtor->GetClassType());
#endif
		s_factoryCtors->erase(
			std::remove(s_factoryCtors->begin(), s_factoryCtors->end(), typeCtor)
		);

		wxDELETE(typeCtor);
		s_factoryCtorCountChanges++;
	}
	else if (typeCtor != nullptr) {
		CBackendCoreException::Error(_("Object '%s' is not register"), typeCtor->GetClassName());
	}

	if (s_factoryCtors->size() == 0) wxDELETE(s_factoryCtors);
}

void CValue::UnRegisterCtor(const wxString& className)
{
	IAbstractTypeCtor* typeCtor = GetAvailableCtor(className);

	if (typeCtor == nullptr) {
		CBackendCoreException::Error(_("Object '%s' is not exist"), className);
		return;
	}

	UnRegisterCtor(typeCtor);
}

bool CValue::IsRegisterCtor(const wxString& className)
{
	if (s_factoryCtors == nullptr || className.IsEmpty())
		return false;
	for (auto& typeCtor : *s_factoryCtors)
		if (stringUtils::CompareString(className, typeCtor->GetClassName()))
			return true;
	return false;
}

bool CValue::IsRegisterCtor(const wxString& className, eCtorObjectType objectType)
{
	if (s_factoryCtors == nullptr)
		return false;
	for (auto& typeCtor : *s_factoryCtors)
		if (stringUtils::CompareString(className, typeCtor->GetClassName()) && (objectType == typeCtor->GetObjectTypeCtor()))
			return true;
	return false;
}

bool CValue::IsRegisterCtor(const class_identifier_t& clsid)
{
	if (s_factoryCtors == nullptr)
		return false;
	for (auto& typeCtor : *s_factoryCtors)
		if (clsid == typeCtor->GetClassType())
			return true;
	return false;
}

class_identifier_t CValue::GetTypeIDByRef(const wxClassInfo* classInfo)
{
	const IAbstractTypeCtor* typeCtor = GetAvailableCtor(classInfo);
	return typeCtor != nullptr ?
		typeCtor->GetClassType() : 0;
}

class_identifier_t CValue::GetTypeIDByRef(const CValue* objectRef)
{
	if (objectRef->m_typeClass != eValueTypes::TYPE_VALUE &&
		objectRef->m_typeClass != eValueTypes::TYPE_OLE &&
		objectRef->m_typeClass != eValueTypes::TYPE_ENUM) {
		return objectRef->GetClassType();
	}
	const wxClassInfo* classInfo = objectRef->GetClassInfo();
	wxASSERT(classInfo);
	if (classInfo != nullptr)
		return GetTypeIDByRef(classInfo);
	return 0;
}

class_identifier_t CValue::GetIDObjectFromString(const wxString& className)
{
	const IAbstractTypeCtor* typeCtor = GetAvailableCtor(className);
	if (typeCtor != nullptr)
		return typeCtor->GetClassType();
	CBackendCoreException::Error(_("Object '%s' is not exist"), className);
	return 0;
}

wxString CValue::GetNameObjectFromID(const class_identifier_t& clsid, bool upper)
{
	const IAbstractTypeCtor* typeCtor = GetAvailableCtor(clsid);
	if (typeCtor != nullptr) {
		return upper ? typeCtor->GetClassName().Upper() :
			typeCtor->GetClassName();
	}
	CBackendCoreException::Error(_("Object with id '%llu' is not exist"), clsid);
	return wxEmptyString;
}

wxString CValue::GetNameObjectFromVT(eValueTypes valueType, bool upper)
{
	if (valueType > eValueTypes::TYPE_REFFER)
		return wxEmptyString;
	for (auto& typeCtor : *s_factoryCtors) {
		const IPrimitiveTypeCtor* simpleSingleObject = dynamic_cast<IPrimitiveTypeCtor*>(typeCtor);
		if (simpleSingleObject != nullptr &&
			valueType == simpleSingleObject->GetValueType()) {
			return upper ? typeCtor->GetClassName().Upper() :
				typeCtor->GetClassName();
		}
	}
	return wxEmptyString;
}

eValueTypes CValue::GetVTByID(const class_identifier_t& clsid)
{
	if (clsid == g_valueUndefinedCLSID)
		return eValueTypes::TYPE_EMPTY;
	else if (clsid == g_valueBooleanCLSID)
		return eValueTypes::TYPE_BOOLEAN;
	else if (clsid == g_valueNumberCLSID)
		return eValueTypes::TYPE_NUMBER;
	else if (clsid == g_valueDateCLSID)
		return eValueTypes::TYPE_DATE;
	else if (clsid == g_valueStringCLSID)
		return eValueTypes::TYPE_STRING;
	else if (clsid == g_valueNullCLSID)
		return eValueTypes::TYPE_NULL;

	const IAbstractTypeCtor* typeCtor = GetAvailableCtor(clsid);

	if (typeCtor != nullptr && typeCtor->GetObjectTypeCtor() == eCtorObjectType_object_enum)
		return eValueTypes::TYPE_ENUM;
	else if (typeCtor != nullptr)
		return eValueTypes::TYPE_VALUE;

	return eValueTypes::TYPE_EMPTY;
}

class_identifier_t CValue::GetIDByVT(const eValueTypes& valueType)
{
	if (valueType == eValueTypes::TYPE_EMPTY)
		return g_valueUndefinedCLSID;
	else if (valueType == eValueTypes::TYPE_BOOLEAN)
		return g_valueBooleanCLSID;
	else if (valueType == eValueTypes::TYPE_NUMBER)
		return g_valueNumberCLSID;
	else if (valueType == eValueTypes::TYPE_DATE)
		return g_valueDateCLSID;
	else if (valueType == eValueTypes::TYPE_STRING)
		return g_valueStringCLSID;
	else if (valueType == eValueTypes::TYPE_NULL)
		return g_valueNullCLSID;

	return 0;
}

IAbstractTypeCtor* CValue::GetAvailableCtor(const wxString& className)
{
	if (s_factoryCtors == nullptr)
		return nullptr;
	for (auto& typeCtor : *s_factoryCtors)
		if (stringUtils::CompareString(className, typeCtor->GetClassName()))
			return typeCtor;
	//CBackendCoreException::Error("Object '%s' is not exist", className);
	return nullptr;
}

IAbstractTypeCtor* CValue::GetAvailableCtor(const class_identifier_t& clsid)
{
	if (s_factoryCtors == nullptr)
		return nullptr;
	for (auto& typeCtor : *s_factoryCtors)
		if (clsid == typeCtor->GetClassType())
			return typeCtor;
	//CBackendCoreException::Error("Object id '%llu' is not exist", clsid);
	return nullptr;
}

IAbstractTypeCtor* CValue::GetAvailableCtor(const wxClassInfo* classInfo)
{
	if (s_factoryCtors == nullptr)
		return nullptr;
	for (auto& typeCtor : *s_factoryCtors)
		if (classInfo == typeCtor->GetClassInfo())
			return typeCtor;
	//CBackendCoreException::Error("Object '%s' is not exist", classInfo->GetClassName());
	return nullptr;
}

std::vector<IAbstractTypeCtor*> CValue::GetListCtorsByType(eCtorObjectType objectType)
{
	std::vector<IAbstractTypeCtor*> retVector;
	std::copy_if(s_factoryCtors->begin(), s_factoryCtors->end(),
		std::back_inserter(retVector), [objectType](IAbstractTypeCtor* t) { return objectType == t->GetObjectTypeCtor(); }
	);
	std::sort(retVector.begin(), retVector.end(),
		[](IAbstractTypeCtor* a, IAbstractTypeCtor* b) { return a->GetClassName() > b->GetClassName(); }
	);
	return retVector;
}

//*******************************************************************************

unsigned int CValue::GetFactoryCountChanges()
{
	return s_factoryCtorCountChanges;
}

//*******************************************************************************