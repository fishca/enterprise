#ifndef _DEBUG_CLIENT_BRIDGE_H__
#define _DEBUG_CLIENT_BRIDGE_H__

#include <wx/wx.h>
#include <wx/socket.h>

#include "backend/backend_core.h"
#include "debugDefs.h"

class BACKEND_API IDebuggerClientBridge {
public:

	virtual ~IDebuggerClientBridge() {}

	//commands 
	virtual void OnSessionStart(wxSocketClient* sock) = 0;
	virtual void OnSessionEnd(wxSocketClient* sock) = 0;

	virtual void OnEnterLoop(wxSocketClient* sock, const CDebugLineData& data) = 0;
	virtual void OnLeaveLoop(wxSocketClient* sock, const CDebugLineData& data) = 0;

	virtual void OnAutoComplete(const CDebugAutoCompleteData& data) = 0;
	virtual void OnMessageFromServer(const CDebugLineData& data, const wxString& message) = 0;
	virtual void OnSetToolTip(const CDebugExpressionData& data, const wxString& resultStr) = 0;

	virtual void OnSetStack(const CStackData& stackData) = 0;

	virtual void OnSetLocalVariable(const CLocalWindowData& watchData) = 0;

	virtual void OnSetVariable(const CWatchWindowData& watchData) = 0;
	virtual void OnSetExpanded(const CWatchWindowData& watchData) = 0;
};

extern BACKEND_API void SetDebuggerClientBridge(IDebuggerClientBridge *bridge);
#endif