#include "metadataReport.h"

#include "backend/objCtor.h"
#include "backend/metadataConfiguration.h"

ibValue* ibMetaDataReport::CreateObjectRef(const ibClassID& clsid, ibValue** paParams, const long lSizeArray) const
{
	auto it = std::find_if(m_factoryCtors.begin(), m_factoryCtors.end(), [clsid](ibCtorAbstractType* typeCtor) {
		return clsid == typeCtor->GetClassType();
		}
	);

	if (it != m_factoryCtors.end()) {
		ibCtorAbstractType* typeCtor(*it);
		wxASSERT(typeCtor);
		ibValue* newObject = typeCtor->CreateObject();
		wxASSERT(newObject);

		bool succes = true;
		if (lSizeArray > 0)
			succes = newObject->Init(paParams, lSizeArray);
		else
			succes = newObject->Init();

		if (!succes) {
			wxDELETE(newObject);
			ibBackendCoreException::Error(_("Error initializing object '%s'"), typeCtor->GetClassName());
		}
		newObject->PrepareNames();
		return newObject;
	}
	return activeMetaData->CreateObjectRef(clsid, paParams, lSizeArray);
}

bool ibMetaDataReport::IsRegisterCtor(const wxString& className) const
{
	if (!ibMetaData::IsRegisterCtor(className))
		return activeMetaData->IsRegisterCtor(className);
	return true;
}

bool ibMetaDataReport::IsRegisterCtor(const wxString& className, ibCtorObjectType objectType) const
{
	if (!ibMetaData::IsRegisterCtor(className, objectType))
		return activeMetaData->IsRegisterCtor(className);
	return true;
}

bool ibMetaDataReport::IsRegisterCtor(const wxString& className, ibCtorObjectType objectType, ibCtorObjectMetaType refType) const
{
	if (!ibMetaData::IsRegisterCtor(className, objectType, refType))
		return activeMetaData->IsRegisterCtor(className, objectType, refType);
	return true;
}

bool ibMetaDataReport::IsRegisterCtor(const ibClassID& clsid) const
{
	if (!ibMetaData::IsRegisterCtor(clsid))
		return activeMetaData->IsRegisterCtor(clsid);
	return true;
}

ibClassID ibMetaDataReport::GetIDObjectFromString(const wxString& className) const
{
	auto it = std::find_if(m_factoryCtors.begin(), m_factoryCtors.end(), [className](ibCtorAbstractType* typeCtor) {
		return stringUtils::CompareString(className, typeCtor->GetClassName());
		});

	if (it != m_factoryCtors.end()) {
		ibCtorAbstractType* typeCtor = *it;
		wxASSERT(typeCtor);
		return typeCtor->GetClassType();
	}

	return activeMetaData->GetIDObjectFromString(className);
}

wxString ibMetaDataReport::GetNameObjectFromID(const ibClassID& clsid, bool upper) const
{
	auto it = std::find_if(m_factoryCtors.begin(), m_factoryCtors.end(), [clsid](ibCtorAbstractType* typeCtor) {
		return clsid == typeCtor->GetClassType();
		});

	if (it != m_factoryCtors.end()) {
		ibCtorAbstractType* typeCtor = *it;
		wxASSERT(typeCtor);
		return upper ? typeCtor->GetClassName().Upper() : typeCtor->GetClassName();
	}

	return activeMetaData->GetNameObjectFromID(clsid, upper);
}

ibCtorMetaValueType* ibMetaDataReport::GetTypeCtor(const ibClassID& clsid) const
{
	auto it = std::find_if(m_factoryCtors.begin(), m_factoryCtors.end(), [clsid](ibCtorMetaValueType* typeCtor) {
		return clsid == typeCtor->GetClassType(); }
	);
	if (it != m_factoryCtors.end()) return *it;
	return activeMetaData->GetTypeCtor(clsid);
}

ibCtorMetaValueType* ibMetaDataReport::GetTypeCtor(const ibValueMetaObject* metaValue, ibCtorObjectMetaType refType) const
{
	auto it = std::find_if(m_factoryCtors.begin(), m_factoryCtors.end(), [metaValue, refType](ibCtorMetaValueType* typeCtor) {
		return refType == typeCtor->GetMetaTypeCtor() &&
			metaValue == typeCtor->GetMetaObject();
		}
	);
	if (it != m_factoryCtors.end()) return *it;
	return activeMetaData->GetTypeCtor(metaValue, refType);
}

ibCtorAbstractType* ibMetaDataReport::GetAvailableCtor(const wxString& className) const
{
	auto it = std::find_if(m_factoryCtors.begin(), m_factoryCtors.end(), [className](ibCtorAbstractType* typeCtor) {
		return stringUtils::CompareString(className, typeCtor->GetClassName());
		}
	);
	if (it != m_factoryCtors.end()) return *it;
	return activeMetaData->GetAvailableCtor(className);
}

ibCtorAbstractType* ibMetaDataReport::GetAvailableCtor(const ibClassID& clsid) const
{
	auto it = std::find_if(m_factoryCtors.begin(), m_factoryCtors.end(), [clsid](ibCtorMetaValueType* typeCtor) {
		return clsid == typeCtor->GetClassType(); }
	);
	if (it != m_factoryCtors.end()) return *it;
	return activeMetaData->GetAvailableCtor(clsid);
}

std::vector<ibCtorMetaValueType*> ibMetaDataReport::GetListCtorsByType() const
{
	return activeMetaData->GetListCtorsByType();
}

std::vector<ibCtorMetaValueType*> ibMetaDataReport::GetListCtorsByType(const ibClassID& clsid, ibCtorObjectMetaType refType) const
{
	return activeMetaData->GetListCtorsByType(clsid, refType);
}

bool ibMetaDataReport::GetOwner(ibMetaData*& metaData) const
{
	metaData = activeMetaData;
	return true;
}

std::vector<ibCtorMetaValueType*> ibMetaDataReport::GetListCtorsByType(ibCtorObjectMetaType refType) const
{
	return activeMetaData->GetListCtorsByType(refType);
}