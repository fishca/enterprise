#ifndef __VALUE_EVENT_H__
#define __VALUE_EVENT_H__

#include "backend/compiler/value.h"

//event support
class BACKEND_API CValueEvent : public CValue {
	wxDECLARE_DYNAMIC_CLASS(CValueEvent);
public:

	CValueEvent();
	CValueEvent(const wxString &eventName);

	virtual bool IsEmpty() const { return m_eventName.IsEmpty(); }

	virtual bool Init(CValue **paParams, const long lSizeArray);
	virtual wxString GetString() const{ return m_eventName; }

protected:
	wxString m_eventName;
};

class BACKEND_API CValueActionEvent : public CValueEvent {
	wxDECLARE_DYNAMIC_CLASS(CValueActionEvent);
public:
	CValueActionEvent();
	CValueActionEvent(const wxString& eventName, action_identifier_t eventId);
private:
	action_identifier_t m_eventId;
};
#endif