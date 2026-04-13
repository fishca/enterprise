////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : base - events 
////////////////////////////////////////////////////////////////////////////

#include "control.h"

//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(IValueFrame::CValueEventContainer, CValue);


//////////////////////////////////////////////////////////////////////

IValueFrame::CValueEventContainer::CValueEventContainer() : CValue(eValueTypes::TYPE_VALUE, true),
m_controlEvent(nullptr), m_methodHelper(nullptr)
{
}

IValueFrame::CValueEventContainer::CValueEventContainer(IValueFrame* controlEvent) : CValue(eValueTypes::TYPE_VALUE, true),
m_controlEvent(controlEvent), m_methodHelper(new CMethodHelper())
{
}

#include "backend/system/value/valueMap.h"
//#include "backend/system/value/valueEvent.h"

IValueFrame::CValueEventContainer::~CValueEventContainer()
{
	if (m_methodHelper)
		delete m_methodHelper;
}

CValue IValueFrame::CValueEventContainer::GetIteratorEmpty()
{
	return CValue();
}

CValue IValueFrame::CValueEventContainer::GetIteratorAt(unsigned int idx)
{
	if (m_controlEvent->GetEventCount() < idx) {
		return CValue();
	}

	IEvent* event = m_controlEvent->GetEvent(idx);
	if (event == nullptr) return CValue();
	//return CValue::CreateAndPrepareValueRef<CValueEvent>(event->GetValue());

	CValue retEvent;
	event->GetDataValue(retEvent);
	return retEvent;
}

#include "backend/appData.h"

bool IValueFrame::CValueEventContainer::SetAt(const CValue& varKeyValue, const CValue& varValue)
{
	const number_t number = varKeyValue.GetNumber();
	if (m_controlEvent->GetEventCount() < number.ToUInt())
		return false;
	IEvent* event = m_controlEvent->GetEvent(number.ToUInt());
	if (event == nullptr) return false;
	//CValueEvent* eventValue = nullptr;
	//if (varValue.ConvertToValue(eventValue)) {
	//	event->SetValue(eventValue->GetString());
	//}
	//else {
	//	event->SetValue(wxEmptyString);
	//}
	//return true;
	return event->SetDataValue(varValue);
}

bool IValueFrame::CValueEventContainer::GetAt(const CValue& varKeyValue, CValue& pvarValue)
{
	number_t number = varKeyValue.GetNumber();
	if (m_controlEvent->GetEventCount() < number.ToUInt())
		return false;
	IEvent* event = m_controlEvent->GetEvent(number.ToUInt());
	if (event == nullptr) return false;
	//wxString eventValue = event->GetValue();
	//if (eventValue.IsEmpty())
	//	return false;
	//pvarValue = CValue::CreateAndPrepareValueRef<CValueEvent>(eventValue);
	//return true;
	return event->GetDataValue(pvarValue);
}

bool IValueFrame::CValueEventContainer::Property(const CValue& varKeyValue, CValue& cValueFound)
{
	const wxString& key = varKeyValue.GetString();
	for (unsigned int idx = 0; idx < m_controlEvent->GetEventCount(); idx++) {
		IEvent* event = m_controlEvent->GetEvent(idx);
		if (event == nullptr) continue;
		if (stringUtils::CompareString(key, event->GetName())) {
			//cValueFound = CValue::CreateAndPrepareValueRef<CValueEvent>(event->GetName());
			//return true;
			return event->GetDataValue(cValueFound);
		}
	}
	return false;
}

enum
{
	enControlProperty,
	enControlCount
};

void IValueFrame::CValueEventContainer::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	m_methodHelper->AppendFunc(wxT("Property"), 2, wxT("Property(key : string, valueFound : event)"));
	m_methodHelper->AppendFunc(wxT("Count"), wxT("Count()"));

	for (unsigned int idx = 0; idx < m_controlEvent->GetEventCount(); idx++) {
		IEvent* event = m_controlEvent->GetEvent(idx);
		if (event == nullptr)
			continue;
		m_methodHelper->AppendProp(event->GetName());
	}
}

bool IValueFrame::CValueEventContainer::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	//CValueEvent* eventValue = nullptr;
	if (m_controlEvent->GetEventCount() < (unsigned int)lPropNum)
		return false;
	IEvent* event = m_controlEvent->GetEvent(lPropNum);
	if (event == nullptr) return false;
	//if (varPropVal.ConvertToValue(eventValue)) {
	//	event->SetValue(eventValue->GetString());
	//}
	//else {
	//	event->SetValue(wxEmptyString);
	//}
	//return true;
	return event->SetDataValue(varPropVal);
}

bool IValueFrame::CValueEventContainer::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	if (m_controlEvent->GetEventCount() < (unsigned int)lPropNum)
		return false;

	IEvent* event = m_controlEvent->GetEvent(lPropNum);
	if (event == nullptr) return false;

	const wxString& eventValue = event->GetValue();
	if (eventValue.IsEmpty()) return true;
	//pvarPropVal = CValue::CreateAndPrepareValueRef<CValueEvent>(eventValue);
	return event->GetDataValue(pvarPropVal);
}

#include "backend/system/value/valueType.h"

bool IValueFrame::CValueEventContainer::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enControlProperty:
		pvarRetValue = Property(*paParams[0], lSizeArray > 1 ? *paParams[1] : CValue());
		return true;
	case enControlCount:
		pvarRetValue = Count();
		return true;
	}

	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

SYSTEM_TYPE_REGISTER(IValueFrame::CValueEventContainer, "EventContainer", string_to_clsid("VL_EVCT"));
