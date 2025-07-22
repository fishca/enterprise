////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : value event 
////////////////////////////////////////////////////////////////////////////

#include "valueEvent.h"

//////////////////////////////////////////////////////////////////////
wxIMPLEMENT_DYNAMIC_CLASS(CValueEvent, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueActionEvent, CValueEvent);

CValueEvent::CValueEvent() :
	CValue(eValueTypes::TYPE_VALUE), m_eventName(wxEmptyString)
{
}

CValueEvent::CValueEvent(const wxString& eventName) :
	CValue(eValueTypes::TYPE_VALUE), m_eventName(eventName)
{
}

bool CValueEvent::Init(CValue** paParams, const long lSizeArray)
{
	if (lSizeArray < 1)
		return false;
	m_eventName = paParams[0]->GetString();
	return true;
}

CValueActionEvent::CValueActionEvent()
	: CValueEvent()
{
}

CValueActionEvent::CValueActionEvent(const wxString& eventName, action_identifier_t eventId)
	: CValueEvent(eventName), m_eventId(eventId)
{
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

VALUE_TYPE_REGISTER(CValueEvent, "event", string_to_clsid("SY_EVENT"));
SYSTEM_TYPE_REGISTER(CValueActionEvent, "actionEvent", string_to_clsid("SY_ATEVT"));
