#include "metadataReport.h"

#include "backend/objCtor.h"
#include "backend/metadataConfiguration.h"

CValue* CMetaDataReport::CreateObjectRef(const class_identifier_t& clsid, CValue** paParams, const long lSizeArray) const
{
	auto it = std::find_if(m_factoryCtors.begin(), m_factoryCtors.end(), [clsid](IAbstractTypeCtor* typeCtor) {
		return clsid == typeCtor->GetClassType();
		}
	);

	if (it != m_factoryCtors.end()) {
		IAbstractTypeCtor* typeCtor(*it);
		wxASSERT(typeCtor);
		CValue* newObject = typeCtor->CreateObject();
		wxASSERT(newObject);

		bool succes = true;
		if (lSizeArray > 0)
			succes = newObject->Init(paParams, lSizeArray);
		else
			succes = newObject->Init();

		if (!succes) {
			wxDELETE(newObject);
			CBackendException::Error(_("Error initializing object '%s'"), typeCtor->GetClassName());
		}
		newObject->PrepareNames();
		return newObject;
	}
	return activeMetaData->CreateObjectRef(clsid, paParams, lSizeArray);
}

bool CMetaDataReport::IsRegisterCtor(const wxString& className) const
{
	if (!IMetaData::IsRegisterCtor(className))
		return activeMetaData->IsRegisterCtor(className);
	return true;
}

bool CMetaDataReport::IsRegisterCtor(const wxString& className, eCtorObjectType objectType) const
{
	if (!IMetaData::IsRegisterCtor(className, objectType))
		return activeMetaData->IsRegisterCtor(className);
	return true;
}

bool CMetaDataReport::IsRegisterCtor(const wxString& className, eCtorObjectType objectType, eCtorMetaType refType) const
{
	if (!IMetaData::IsRegisterCtor(className, objectType, refType))
		return activeMetaData->IsRegisterCtor(className, objectType, refType);
	return true;
}

bool CMetaDataReport::IsRegisterCtor(const class_identifier_t& clsid) const
{
	if (!IMetaData::IsRegisterCtor(clsid))
		return activeMetaData->IsRegisterCtor(clsid);
	return true;
}

class_identifier_t CMetaDataReport::GetIDObjectFromString(const wxString& className) const
{
	auto it = std::find_if(m_factoryCtors.begin(), m_factoryCtors.end(), [className](IAbstractTypeCtor* typeCtor) {
		return stringUtils::CompareString(className, typeCtor->GetClassName());
		});

	if (it != m_factoryCtors.end()) {
		IAbstractTypeCtor* typeCtor = *it;
		wxASSERT(typeCtor);
		return typeCtor->GetClassType();
	}

	return activeMetaData->GetIDObjectFromString(className);
}

wxString CMetaDataReport::GetNameObjectFromID(const class_identifier_t& clsid, bool upper) const
{
	auto it = std::find_if(m_factoryCtors.begin(), m_factoryCtors.end(), [clsid](IAbstractTypeCtor* typeCtor) {
		return clsid == typeCtor->GetClassType();
		});

	if (it != m_factoryCtors.end()) {
		IAbstractTypeCtor* typeCtor = *it;
		wxASSERT(typeCtor);
		return upper ? typeCtor->GetClassName().Upper() : typeCtor->GetClassName();
	}

	return activeMetaData->GetNameObjectFromID(clsid, upper);
}

IMetaValueTypeCtor* CMetaDataReport::GetTypeCtor(const class_identifier_t& clsid) const
{
	auto it = std::find_if(m_factoryCtors.begin(), m_factoryCtors.end(), [clsid](IMetaValueTypeCtor* typeCtor) {
		return clsid == typeCtor->GetClassType(); }
	);
	if (it != m_factoryCtors.end()) return *it;
	return activeMetaData->GetTypeCtor(clsid);
}

IMetaValueTypeCtor* CMetaDataReport::GetTypeCtor(const IMetaObject* metaValue, eCtorMetaType refType) const
{
	auto it = std::find_if(m_factoryCtors.begin(), m_factoryCtors.end(), [metaValue, refType](IMetaValueTypeCtor* typeCtor) {
		return refType == typeCtor->GetMetaTypeCtor() &&
			metaValue == typeCtor->GetMetaObject();
		}
	);
	if (it != m_factoryCtors.end()) return *it;
	return activeMetaData->GetTypeCtor(metaValue, refType);
}

IAbstractTypeCtor* CMetaDataReport::GetAvailableCtor(const wxString& className) const
{
	auto it = std::find_if(m_factoryCtors.begin(), m_factoryCtors.end(), [className](IAbstractTypeCtor* typeCtor) {
		return stringUtils::CompareString(className, typeCtor->GetClassName());
		}
	);
	if (it != m_factoryCtors.end()) return *it;
	return activeMetaData->GetAvailableCtor(className);
}

IAbstractTypeCtor* CMetaDataReport::GetAvailableCtor(const class_identifier_t& clsid) const
{
	auto it = std::find_if(m_factoryCtors.begin(), m_factoryCtors.end(), [clsid](IMetaValueTypeCtor* typeCtor) {
		return clsid == typeCtor->GetClassType(); }
	);
	if (it != m_factoryCtors.end()) return *it;
	return activeMetaData->GetAvailableCtor(clsid);
}

std::vector<IMetaValueTypeCtor*> CMetaDataReport::GetListCtorsByType() const
{
	return activeMetaData->GetListCtorsByType();
}

std::vector<IMetaValueTypeCtor*> CMetaDataReport::GetListCtorsByType(const class_identifier_t& clsid, eCtorMetaType refType) const
{
	return activeMetaData->GetListCtorsByType(clsid, refType);
}

bool CMetaDataReport::GetOwner(IMetaData*& metaData) const
{
	metaData = activeMetaData;
	return true;
}

std::vector<IMetaValueTypeCtor*> CMetaDataReport::GetListCtorsByType(eCtorMetaType refType) const
{
	return activeMetaData->GetListCtorsByType(refType);
}