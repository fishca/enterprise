#include "objectSelector.h"
#include "backend/metaCollection/partial/reference/reference.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/appData.h"

IValueSelectorDataObject::IValueSelectorDataObject() : CValue(eValueTypes::TYPE_VALUE, true),
m_methodHelper(new CMethodHelper())
{
}

IValueSelectorDataObject::~IValueSelectorDataObject()
{
	wxDELETE(m_methodHelper);
}

#include "backend/objCtor.h"

class_identifier_t IValueSelectorDataObject::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		GetMetaObject()->GetTypeCtor(eCtorMetaType::eCtorMetaType_Selection);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString IValueSelectorDataObject::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		GetMetaObject()->GetTypeCtor(eCtorMetaType::eCtorMetaType_Selection);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString IValueSelectorDataObject::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		GetMetaObject()->GetTypeCtor(eCtorMetaType::eCtorMetaType_Selection);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

/////////////////////////////////////////////////////////////////////////

CValueSelectorRecordDataObject::CValueSelectorRecordDataObject(IValueMetaObjectRecordDataMutableRef* metaObject) :
	IValueSelectorDataObject(),
	IValueDataObject(CGuid(), false),
	m_metaObject(metaObject)
{
	Reset();
}

bool CValueSelectorRecordDataObject::Next()
{
	if (appData->DesignerMode()) {
		return false;
	}

	if (!m_objGuid.isValid()) {
		if (m_currentValues.size() > 0) {
			auto itStart = m_currentValues.begin();
			m_objGuid = *itStart;
			return Read();
		}
	}
	else {
		auto it = std::find(m_currentValues.begin(), m_currentValues.end(), m_objGuid);
		ptrdiff_t pos =
			std::distance(m_currentValues.begin(), it);
		if (pos == m_currentValues.size() - 1) {
			return false;
		}
		std::advance(it, 1);
		m_objGuid = *it;
		return Read();
	}

	return false;
}

IValueRecordDataObjectRef* CValueSelectorRecordDataObject::GetObject(const CGuid& guid) const
{
	if (appData->DesignerMode()) {
		return m_metaObject->CreateObjectValue();
	}

	if (!guid.isValid()) {
		return nullptr;
	}

	return m_metaObject->CreateObjectValue(guid);
}

//////////////////////////////////////////////////////////////////////////

CValueSelectorRegisterDataObject::CValueSelectorRegisterDataObject(IValueMetaObjectRegisterData* metaObject) :
	IValueSelectorDataObject(),
	m_metaObject(metaObject)
{
	Reset();
}

bool CValueSelectorRegisterDataObject::Next()
{
	if (appData->DesignerMode()) {
		return false;
	}

	if (m_keyValues.empty()) {
		if (m_currentValues.size() > 0) {
			auto itStart = m_currentValues.begin();
			m_keyValues = *itStart;
			return Read();
		}
	}
	else {
		auto it = std::find(m_currentValues.begin(), m_currentValues.end(), m_keyValues);
		ptrdiff_t pos =
			std::distance(m_currentValues.begin(), it);
		if (pos == m_currentValues.size() - 1) {
			return false;
		}
		std::advance(it, 1);
		m_keyValues = *it;
		return Read();
	}

	return false;
}

IValueRecordManagerObject* CValueSelectorRegisterDataObject::GetRecordManager(const valueArray_t& keyValues) const
{
	if (appData->DesignerMode()) {
		return m_metaObject->CreateRecordManagerObjectValue();
	}

	if (keyValues.empty()) {
		return nullptr;
	}

	return m_metaObject->CreateRecordManagerObjectValue(
		CUniquePairKey(m_metaObject, keyValues)
	);
}

enum Func {
	enNext,
	enReset,
	enGetObjectRecord
};

void CValueSelectorRecordDataObject::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	m_methodHelper->AppendFunc(wxT("Next"), wxT("Next()"));
	m_methodHelper->AppendFunc(wxT("Reset"), wxT("Reset()"));
	m_methodHelper->AppendFunc(wxT("GetObject"), wxT("GetObject()"));

	//set object name 
	wxString objectName;

	for (const auto object : m_metaObject->GetAttributeArrayObject()) {
		if (object->IsDeleted())
			continue;
		if (!object->GetObjectNameAsString(objectName))
			continue;
		m_methodHelper->AppendProp(
			objectName,
			true,
			false,
			object->GetMetaID()
		);
	}

	for (const auto object : m_metaObject->GetTableArrayObject()) {
		if (object->IsDeleted())
			continue;
		if (!object->GetObjectNameAsString(objectName))
			continue;
		m_methodHelper->AppendProp(
			objectName,
			true,
			false,
			object->GetMetaID()
		);
	}

	m_methodHelper->AppendProp(wxT("Reference"), m_metaObject->GetMetaID());
}

bool CValueSelectorRecordDataObject::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enNext:
		pvarRetValue = Next();
		return true;
	case enReset:
		Reset();
		return true;
	case enGetObjectRecord:
		pvarRetValue = GetObject(m_objGuid);
		return true;
	}

	return false;
}

bool CValueSelectorRecordDataObject::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	return false;
}

bool CValueSelectorRecordDataObject::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const meta_identifier_t& id = m_methodHelper->GetPropData(lPropNum);
	if (!m_objGuid.isValid()) {
		if (!appData->DesignerMode()) {
			pvarPropVal = CValue(eValueTypes::TYPE_NULL);
			return true;
		}
	}
	if (id != m_metaObject->GetMetaID()) {
		pvarPropVal = m_listObjectValue.at(id);
		return true;
	}
	pvarPropVal = CValueReferenceDataObject::Create(m_metaObject, m_objGuid);
	return true;
}

void CValueSelectorRegisterDataObject::PrepareNames() const
{
	m_methodHelper->AppendFunc(wxT("Next"), wxT("Next()"));
	m_methodHelper->AppendFunc(wxT("Reset"), wxT("Reset()"));

	if (m_metaObject->HasRecordManager()) {
		m_methodHelper->AppendFunc(wxT("GetRecordManager"), wxT("GetRecordManager()"));
	}

	//set object name 
	wxString objectName;

	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		if (object->IsDeleted())
			continue;
		if (!object->GetObjectNameAsString(objectName))
			continue;
		m_methodHelper->AppendProp(
			objectName,
			true,
			false,
			object->GetMetaID()
		);
	}
}

bool CValueSelectorRegisterDataObject::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enNext:
		pvarRetValue = Next();
		return true;
	case enReset:
		Reset();
		return true;
	case enGetObjectRecord:
		pvarRetValue = GetRecordManager(m_keyValues);
		return true;
	}

	return false;
}

bool CValueSelectorRegisterDataObject::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	return false;
}

bool CValueSelectorRegisterDataObject::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const meta_identifier_t& id = m_methodHelper->GetPropData(lPropNum);
	if (m_keyValues.empty()) {
		if (!appData->DesignerMode()) {
			pvarPropVal = CValue(eValueTypes::TYPE_NULL);
			return true;
		}
	}
	pvarPropVal = m_listObjectValue[m_keyValues][id];
	return true;
}
