#include "debugClient.h"

void CDebuggerClient::CDebuggerClientAdapter::OnSessionStart(wxSocketClient* sock)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnSessionStart(sock);
	}
}

void CDebuggerClient::CDebuggerClientAdapter::OnSessionEnd(wxSocketClient* sock)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnSessionEnd(sock);
	}
}

void CDebuggerClient::CDebuggerClientAdapter::OnEnterLoop(wxSocketClient* sock, const CDebugLineData& data)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnEnterLoop(sock, data);
	}
}

void CDebuggerClient::CDebuggerClientAdapter::OnLeaveLoop(wxSocketClient* sock, const CDebugLineData& data)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnLeaveLoop(sock, data);
	}
}

void CDebuggerClient::CDebuggerClientAdapter::OnAutoComplete(const CDebugAutoCompleteData& data)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnAutoComplete(data);
	}
}

void CDebuggerClient::CDebuggerClientAdapter::OnMessageFromServer(const CDebugLineData& data, const wxString& message)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnMessageFromServer(data, message);
	}
}

void CDebuggerClient::CDebuggerClientAdapter::OnSetToolTip(const CDebugExpressionData& data, const wxString& strResult)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnSetToolTip(data, strResult);
	}
}

void CDebuggerClient::CDebuggerClientAdapter::OnSetStack(const CStackData& data)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnSetStack(data);
	}
}

void CDebuggerClient::CDebuggerClientAdapter::OnSetLocalVariable(const CLocalWindowData& data)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnSetLocalVariable(data);
	}
}

void CDebuggerClient::CDebuggerClientAdapter::OnSetVariable(const CWatchWindowData& data)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnSetVariable(data);
	}
}

void CDebuggerClient::CDebuggerClientAdapter::OnSetExpanded(const CWatchWindowData& data)
{
	if (m_debugBridge != nullptr) {
		m_debugBridge->OnSetExpanded(data);
	}
}
