////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : value structure and containers
////////////////////////////////////////////////////////////////////////////

#include "valueMap.h"
#include "backend/backend_exception.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueContainer, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueStructure, CValue);

CValue::CMethodHelper CValueContainer::CValueReturnContainer::m_methodHelper;

bool CValueContainer::ContainerComparator::operator() (const CValue& lhs, const CValue& rhs) const {
	if (lhs.GetType() == eValueTypes::TYPE_STRING
		&& rhs.GetType() == eValueTypes::TYPE_STRING) {
		return stringUtils::MakeUpper(lhs.GetString()) < stringUtils::MakeUpper(rhs.GetString());
	}
	else {
		return lhs < rhs;
	}
}

//**********************************************************************
//*                          CValueReturnMap                           *
//**********************************************************************

void CValueContainer::CValueReturnContainer::PrepareNames() const
{
	m_methodHelper.ClearHelper();
	m_methodHelper.AppendProp(wxT("Key"));
	m_methodHelper.AppendProp(wxT("Value"));
}

bool CValueContainer::CValueReturnContainer::SetPropVal(const long lPropNum, CValue& cValue)
{
	return false;
}

bool CValueContainer::CValueReturnContainer::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	switch (lPropNum)
	{
	case enKey:
		pvarPropVal = m_key;
		return true;
	case enValue:
		pvarPropVal = m_value;
		return true;
	}

	return false;
}

//**********************************************************************
//*                            CValueContainer                         *
//**********************************************************************

CValueContainer::CValueContainer() : CValue(eValueTypes::TYPE_VALUE), m_methodHelper(new CMethodHelper) {}

CValueContainer::CValueContainer(const std::map<CValue, CValue>& containerValues) : CValue(eValueTypes::TYPE_VALUE, true), m_methodHelper(new CMethodHelper) {
	for (auto& cntVal : containerValues) {
		m_containerValues.insert_or_assign(cntVal.first, cntVal.second);
	}
	PrepareNames();
}

CValueContainer::CValueContainer(bool readOnly) : CValue(eValueTypes::TYPE_VALUE, readOnly), m_methodHelper(new CMethodHelper) {
}

CValueContainer::~CValueContainer() {
	m_containerValues.clear();
	wxDELETE(m_methodHelper);
}

//работа с массивом как с агрегатным объектом
//перечисление строковых ключей
void CValueContainer::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendFunc(wxT("Count"), wxT("Count()"));
	m_methodHelper->AppendFunc(wxT("Property"), 2, wxT("Property(key : any, valueFound : any)"));

	if (!m_bReadOnly) {
		m_methodHelper->AppendFunc(wxT("Clear"), wxT("Clear()"));
		m_methodHelper->AppendFunc(wxT("Delete"), 1, wxT("Delete(key : any)"));
		m_methodHelper->AppendFunc(wxT("Insert"), 2, wxT("Insert(key : any, value : any)"));
	}

	for (auto keyValue : m_containerValues) {
		const CValue& cValKey = keyValue.first;
		if (!cValKey.IsEmpty()) {
			m_methodHelper->AppendProp(cValKey.GetString());
		}
	}
}

bool CValueContainer::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	return SetAt(
		GetPropName(lPropNum), varPropVal
	);
}

bool CValueContainer::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	return GetAt(
		GetPropName(lPropNum), pvarPropVal
	);
}

bool CValueContainer::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enClear:
		Clear();
		return true;
	case enCount:
		pvarRetValue = Count();
		return true;
	case enDelete:
		Delete(*paParams[0]);
		return true;
	case enInsert:
		Insert(*paParams[0], *paParams[1]);
		return true;
	case enProperty:
		pvarRetValue = Property(*paParams[0], lSizeArray > 1 ? *paParams[1] : CValue());
		return true;
	}

	return false;
}

void CValueContainer::Delete(const CValue& varKeyValue)
{
	if (m_methodHelper != nullptr) m_methodHelper->RemoveProp(varKeyValue.GetString());
	m_containerValues.erase(varKeyValue);
}

#include "backend/appData.h"

void CValueContainer::Insert(const CValue& varKeyValue, const CValue& cValue)
{
	std::map<const CValue, CValue>::iterator it = m_containerValues.find(varKeyValue);
	if (it != m_containerValues.end()) {
		if (!appData->DesignerMode())
			CBackendCoreException::Error(_("Key '%s' is already using!"), varKeyValue.GetString());
		return;
	}
	if (m_methodHelper != nullptr) m_methodHelper->AppendProp(varKeyValue.GetString());
	m_containerValues.insert_or_assign(varKeyValue, cValue);
}

bool CValueContainer::Property(const CValue& varKeyValue, CValue& cValueFound)
{
	std::map<const CValue, CValue>::iterator itFound = m_containerValues.find(varKeyValue);
	if (itFound != m_containerValues.end()) {
		cValueFound = itFound->second;
		return true;
	}
	return false;
}

CValue CValueContainer::GetIteratorEmpty()
{
	return CValue::CreateAndPrepareValueRef<CValueReturnContainer>();
}

CValue CValueContainer::GetIteratorAt(unsigned int idx)
{
	if (m_containerValues.size() < idx)
		return CValue();
	auto structurePos = m_containerValues.begin();
	std::advance(structurePos, idx);
	return CValue::CreateAndPrepareValueRef<CValueReturnContainer>(structurePos->first, structurePos->second);
}

bool CValueContainer::SetAt(const CValue& varKeyValue, const CValue& varValue)
{
	CValueContainer::Insert(varKeyValue, varValue);
	return true;
}

bool CValueContainer::GetAt(const CValue& varKeyValue, CValue& pvarValue)
{
	std::map<const CValue, CValue>::const_iterator itFound = m_containerValues.find(varKeyValue);
	if (itFound != m_containerValues.end()) {
		pvarValue = itFound->second; return true;
	}
	if (!appData->DesignerMode())
		CBackendCoreException::Error(_("Key '%s' not found!"), varKeyValue.GetString());
	return false;
}

//**********************************************************************
//*                            CValueStructure                         *
//**********************************************************************

#define st_error_conversion _("Error conversion value. Must be string!")

bool CValueStructure::GetAt(const CValue& varKeyValue, CValue& pvarValue)
{
	if (varKeyValue.GetType() != eValueTypes::TYPE_STRING) {
		if (!appData->DesignerMode())
			CBackendCoreException::Error(st_error_conversion);
		return false;
	}
	return CValueContainer::GetAt(varKeyValue, pvarValue);
}

bool CValueStructure::SetAt(const CValue& varKeyValue, const CValue& cValue)
{
	if (varKeyValue.GetType() != eValueTypes::TYPE_STRING) {
		if (!appData->DesignerMode()) {
			CBackendCoreException::Error(st_error_conversion);
		} return false;
	}

	return CValueContainer::SetAt(varKeyValue, cValue);
}

void CValueStructure::Delete(const CValue& varKeyValue)
{
	if (varKeyValue.GetType() != eValueTypes::TYPE_STRING) {
		if (!appData->DesignerMode()) {
			CBackendCoreException::Error(st_error_conversion);
		} return;
	}

	CValueContainer::Delete(varKeyValue);
}

void CValueStructure::Insert(const CValue& varKeyValue, const CValue& cValue)
{
	if (varKeyValue.GetType() != eValueTypes::TYPE_STRING) {
		if (!appData->DesignerMode()) {
			CBackendCoreException::Error(st_error_conversion);
		} return;
	}

	CValueContainer::Insert(varKeyValue, cValue);
}

bool CValueStructure::Property(const CValue& varKeyValue, CValue& cValueFound)
{
	if (varKeyValue.GetType() != eValueTypes::TYPE_STRING) {
		if (!appData->DesignerMode()) {
			CBackendCoreException::Error(st_error_conversion);
		}
		return false;
	}

	return CValueContainer::Property(varKeyValue, cValueFound);
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

VALUE_TYPE_REGISTER(CValueContainer, "Container", string_to_clsid("VL_CONTR"));
VALUE_TYPE_REGISTER(CValueStructure, "Structure", string_to_clsid("VL_STRUT"));

SYSTEM_TYPE_REGISTER(CValueContainer::CValueReturnContainer, "KeyValue", string_to_clsid("VL_KEVAL"));