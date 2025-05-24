#include "debugClient.h"

void CDebuggerClient::CDebuggerAdapterClient::OnSessionStart(wxSocketClient* sock)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnSessionStart(sock);
	}
}

void CDebuggerClient::CDebuggerAdapterClient::OnSessionEnd(wxSocketClient* sock)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnSessionEnd(sock);
	}
}

void CDebuggerClient::CDebuggerAdapterClient::OnEnterLoop(wxSocketClient* sock, const CDebugLineData& data)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnEnterLoop(sock, data);
	}
}

void CDebuggerClient::CDebuggerAdapterClient::OnLeaveLoop(wxSocketClient* sock, const CDebugLineData& data)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnLeaveLoop(sock, data);
	}
}

void CDebuggerClient::CDebuggerAdapterClient::OnAutoComplete(const CDebugAutoCompleteData& data)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnAutoComplete(data);
	}
}

void CDebuggerClient::CDebuggerAdapterClient::OnMessageFromServer(const CDebugLineData& data, const wxString& message)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnMessageFromServer(data, message);
	}
}

void CDebuggerClient::CDebuggerAdapterClient::OnSetToolTip(const CDebugExpressionData& data, const wxString& strResult)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnSetToolTip(data, strResult);
	}
}

void CDebuggerClient::CDebuggerAdapterClient::OnSetStack(const CStackData& data)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnSetStack(data);
	}
}

void CDebuggerClient::CDebuggerAdapterClient::OnSetLocalVariable(const CLocalWindowData& data)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnSetLocalVariable(data);
	}
}

void CDebuggerClient::CDebuggerAdapterClient::OnSetVariable(const CWatchWindowData& data)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnSetVariable(data);
	}
}

void CDebuggerClient::CDebuggerAdapterClient::OnSetExpanded(const CWatchWindowData& data)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnSetExpanded(data);
	}
}
