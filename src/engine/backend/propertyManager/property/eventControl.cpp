#include "eventControl.h"
#include "backend/system/value/valueEvent.h"

//base property for "event"
bool CEventControl::SetDataValue(const ibValue& varPropVal)
{
	ibValueEvent* event = varPropVal.ConvertToType<ibValueEvent>();
	if (event == nullptr) return false;
	m_propValue = event->GetString();
	return true;
}

bool CEventControl::GetDataValue(ibValue& pvarPropVal) const
{
	pvarPropVal = ibValue::CreateObjectValue<ibValueEvent>(m_propValue);
	return true;
}

bool CEventControl::LoadData(ibReaderMemory& reader)
{
	m_propName = reader.r_stringZ();
	m_propValue = reader.r_stringZ();
	return true;
}

bool CEventControl::SaveData(ibWriterMemory& writer)
{
	writer.w_stringZ(m_propName);
	writer.w_stringZ(m_propValue.GetString());
	return true;
}