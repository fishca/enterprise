#include "eventControl.h"
#include "backend/system/value/valueEvent.h"

//base property for "event"
bool CEventControl::SetDataValue(const CValue& varPropVal)
{
	CValueEvent* event = varPropVal.ConvertToType<CValueEvent>();
	if (event == nullptr) return false;
	m_propValue = event->GetString();
	return true;
}

bool CEventControl::GetDataValue(CValue& pvarPropVal) const
{
	pvarPropVal = CValue::CreateObjectValue<CValueEvent>(m_propValue);
	return true;
}

bool CEventControl::LoadData(CMemoryReader& reader)
{
	m_propName = reader.r_stringZ();
	m_propValue = reader.r_stringZ();
	return true;
}

bool CEventControl::SaveData(CMemoryWriter& writer)
{
	writer.w_stringZ(m_propName);
	writer.w_stringZ(m_propValue.GetString());
	return true;
}