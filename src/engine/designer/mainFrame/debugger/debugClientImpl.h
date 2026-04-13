#ifndef _DEBUG_CLIENT_IMPL_H__
#define _DEBUG_CLIENT_IMPL_H__

#include "backend/debugger/debugClientBridge.h"

class CDebuggerClientBridge : public IDebuggerClientBridge {
public:

	//commands 
	virtual void OnSessionStart(wxSocketClient* sock);
	virtual void OnSessionEnd(wxSocketClient* sock);

	virtual void OnEnterLoop(wxSocketClient* sock, const CDebugLineData& data);
	virtual void OnLeaveLoop(wxSocketClient* sock, const CDebugLineData& data);

	virtual void OnAutoComplete(const CDebugAutoCompleteData& data);
	virtual void OnMessageFromServer(const CDebugLineData& data, const wxString& message);
	virtual void OnSetToolTip(const CDebugExpressionData& data, const wxString& resultStr);

	virtual void OnSetStack(const CStackData& stackData);

	virtual void OnSetLocalVariable(const CLocalWindowData& data);

	virtual void OnSetVariable(const CWatchWindowData& watchData);
	virtual void OnSetExpanded(const CWatchWindowData& watchData);
};

#endif