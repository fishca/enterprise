////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : debugger - server part
////////////////////////////////////////////////////////////////////////////

#include "debugServer.h"

#include "backend/compiler/procUnit.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/metadataConfiguration.h"

#include "backend/fileSystem/fs.h"
#if _USE_NET_COMPRESSOR == 1
#include "utils/fs/lz/lzhuf.h"
#endif

#include "backend/appData.h"

///////////////////////////////////////////////////////////////////////
CDebuggerServer* CDebuggerServer::ms_debugServer = nullptr;
///////////////////////////////////////////////////////////////////////

void CDebuggerServer::Initialize(const int flags)
{
	if (ms_debugServer != nullptr)
		ms_debugServer->Destroy();

	ms_debugServer = new CDebuggerServer();
}

void CDebuggerServer::Destroy()
{
	if (ms_debugServer != nullptr)
		ms_debugServer->ShutdownServer();

	wxDELETE(ms_debugServer);
}

CDebuggerServer::CDebuggerServer() :
	m_bUseDebug(false), m_bDoLoop(false), m_bDebugLoop(false), m_bDebugStopLine(false),
	m_numCurrentNumberStopContext(0),
	m_runContext(nullptr), m_socketConnectionThread(nullptr)
{
}

CDebuggerServer::~CDebuggerServer()
{
}

bool CDebuggerServer::CreateServer(const wxString& hostName, unsigned short startPort, bool wait)
{
	ShutdownServer();

	m_socketConnectionThread = new CDebuggerServerConnection(hostName, startPort);

	if (m_socketConnectionThread->Run() != wxTHREAD_NO_ERROR) {
		ShutdownServer();
		return false;
	}

	m_socketConnectionThread->m_waitConnection = wait;

	if (wait) {

		while (m_socketConnectionThread != nullptr) {

			if (m_bUseDebug || m_socketConnectionThread->m_acceptConnection)
				break;

			wxMilliSleep(5);
		}

		wxASSERT_MSG(m_socketConnectionThread != nullptr
			&& m_socketConnectionThread->m_socket != nullptr, _("Client not connected!"));
	}

	return m_socketConnectionThread != nullptr;
}

void CDebuggerServer::ShutdownServer()
{
	if (m_socketConnectionThread != nullptr) {
		m_socketConnectionThread->Delete();
		m_socketConnectionThread = nullptr;
	}
}

#include "backend/system/value/valueOLE.h"
#include "backend/backend_mainFrame.h"

void CDebuggerServer::ClearCollectionBreakpoint()
{
	// anytime we access the m_pThread pointer we must ensure that it won't
	// be modified in the meanwhile; since only a single thread may be
	// inside a given critical section at a given time, the following code
	// is safe:
	wxCriticalSectionLocker enter(m_clearBreakpointsCS);

	m_listBreakpoint.clear();
}

void CDebuggerServer::DoDebugLoop(const wxString& strDocPath, const wxString& strModuleName, int numLine, CRunContext* runContext)
{
	m_runContext = runContext;

	if (m_socketConnectionThread == nullptr || (!m_socketConnectionThread->IsConnected() || !m_socketConnectionThread->IsRunning())) {
		CDebuggerServer::ResetDebugger();
		return;
	}

	m_numCurrentNumberStopContext = 0;

	if (m_socketConnectionThread != nullptr && ConnectionType::ConnectionType_Debugger == m_socketConnectionThread->GetConnectionType()) {

		CMemoryWriter commandChannelEnterLoop;

		commandChannelEnterLoop.w_u16(CommandId_EnterLoop);
		commandChannelEnterLoop.w_stringZ(strDocPath);
		commandChannelEnterLoop.w_stringZ(strModuleName);
		commandChannelEnterLoop.w_s32(numLine);

		SendCommand(commandChannelEnterLoop.pointer(), commandChannelEnterLoop.size());
	}

	//send expressions from user 
	SendExpressions();

	//send local variable
	SendLocalVariables();

	//send stack data to designer
	SendStack();

	//start debug loop
	m_bDebugLoop = true;
	m_bDebugStopLine = false;

	//create stream for this loop
	CValueOLE::CreateStreamForDispatch();

	//start debug loop
	while (m_bDebugLoop) {

		// there is no configurator or the connection was somehow lost
		// thread was lost
		if (m_socketConnectionThread == nullptr || (!m_socketConnectionThread->IsConnected() || !m_socketConnectionThread->IsRunning())) {
			m_bUseDebug = m_bDebugLoop = false;
			break;
		}

		wxMilliSleep(5);
	}

	CValueOLE::ReleaseStreamForDispatch();

	if (m_socketConnectionThread != nullptr && ConnectionType::ConnectionType_Debugger == m_socketConnectionThread->GetConnectionType()) {

		CMemoryWriter commandChannelLeaveLoop;

		commandChannelLeaveLoop.w_u16(CommandId_LeaveLoop);
		commandChannelLeaveLoop.w_stringZ(strDocPath);
		commandChannelLeaveLoop.w_stringZ(strModuleName);
		commandChannelLeaveLoop.w_s32(numLine);

		SendCommand(commandChannelLeaveLoop.pointer(), commandChannelLeaveLoop.size());
	}

	//activate main frame 
	if (backend_mainFrame != nullptr)
		backend_mainFrame->RaiseFrame();

	m_runContext = nullptr;
}

void CDebuggerServer::EnterDebugger(CRunContext* runContext, const CByteUnit& byteCode, long& numPrevLine)
{
	if (!m_bUseDebug)
		return;

	if (byteCode.m_numOper != OPER_FUNC && byteCode.m_numOper != OPER_END
		&& byteCode.m_numOper != OPER_SET && byteCode.m_numOper != OPER_SETCONST && byteCode.m_numOper != OPER_SET_TYPE
		&& byteCode.m_numOper != OPER_TRY && byteCode.m_numOper != OPER_ENDTRY) {

		if (byteCode.m_numLine != numPrevLine) {

			m_bDoLoop = false;

			//step into 
			if (m_bDebugStopLine && byteCode.m_numLine >= 0)
			{
				m_bDebugStopLine = false;
				m_bDoLoop = true;
			}
			// step through 
			else if (m_numCurrentNumberStopContext && m_numCurrentNumberStopContext >= CProcUnit::GetCountRunContext() && byteCode.m_numLine >= 0)
			{
				m_numCurrentNumberStopContext = CProcUnit::GetCountRunContext();
				m_bDoLoop = true;
			}
			else
			{
				//arbitrary breakpoint 
				if (byteCode.m_numLine >= 0) {
					auto list_breakpoint_iterator = m_listBreakpoint.find(byteCode.m_strDocPath);
					if (list_breakpoint_iterator != m_listBreakpoint.end()) {

						const auto& list_current_breakpoint = list_breakpoint_iterator->second;
						auto list_current_breakpoint_iterator = std::find(
							list_current_breakpoint.begin(), list_current_breakpoint.end(), byteCode.m_numLine);

						m_bDoLoop = list_current_breakpoint_iterator != list_current_breakpoint.end();
					}
				}
			}

			if (m_bDoLoop)
				DoDebugLoop(byteCode.m_strFileName, byteCode.m_strDocPath, byteCode.m_numLine + 1, runContext);
		}

		numPrevLine = byteCode.m_numLine;
	}
}

void CDebuggerServer::SendErrorToClient(const wxString& strFileName,
	const wxString& strDocPath, unsigned int numLine, const wxString& strErrorMessage)
{
	if (m_socketConnectionThread == nullptr)
		CreateServer(defaultHost, defaultDebuggerPort, true);

	if (m_socketConnectionThread == nullptr || !m_socketConnectionThread->IsConnected() || !m_bUseDebug)
		return;

	CMemoryWriter commandChannel;

	commandChannel.w_u16(CommandId_MessageFromServer);
	commandChannel.w_stringZ(strFileName); // file name
	commandChannel.w_stringZ(strDocPath); // module name
	commandChannel.w_u32(numLine); // line code 
	commandChannel.w_stringZ(strErrorMessage); // error message 

	SendCommand(commandChannel.pointer(), commandChannel.size());
}

void CDebuggerServer::SendExpressions()
{
	if (!m_listExpression.size())
		return;

	CMemoryWriter commandChannel;

	//header 
	commandChannel.w_u16(CommandId_SetExpressions);
	commandChannel.w_u32(m_listExpression.size());

	CValue vResult;

	for (auto expression : m_listExpression) {
		//header 
#if _USE_64_BIT_POINT_IN_DEBUGGER == 1
		commandChannel.w_u64(expression.first);
#else 
		commandChannel.w_u32(expression.first);
#endif 
		//variable
		commandChannel.w_stringZ(expression.second);

		if (CProcUnit::Evaluate(expression.second, m_runContext, vResult, false)) {
			commandChannel.w_stringZ(vResult.GetString());
			commandChannel.w_stringZ(vResult.GetClassName());
			//array
			commandChannel.w_u32(vResult.GetNProps());
		}
		else {
			commandChannel.w_stringZ(CBackendException::GetLastError());
			commandChannel.w_stringZ(wxT("<error>"));
			//array
			commandChannel.w_u32(0);
		}
	}

	SendCommand(commandChannel.pointer(), commandChannel.size());
}

void CDebuggerServer::SendLocalVariables()
{
	CMemoryWriter commandChannel;
	commandChannel.w_u16(CommandId_SetLocalVariables);

	CCompileContext* compileContext = m_runContext->m_compileContext;
	wxASSERT(compileContext);
	commandChannel.w_u32(compileContext->m_listVariable.size());

	for (auto variable : compileContext->m_listVariable) {

		const auto locRefVariable = variable.second;
		const auto locRefValue = m_runContext->m_pRefLocVars[locRefVariable->m_numVariable];

		//send temp var 
		commandChannel.w_u8(locRefVariable->m_bTempVar);

		//send attribute body
		commandChannel.w_stringZ(locRefVariable->m_strRealName);
		commandChannel.w_stringZ(locRefValue->GetString());
		commandChannel.w_stringZ(locRefValue->GetClassName());

		//send attribute count 
		commandChannel.w_u32(locRefValue->GetNProps());
	}

	SendCommand(commandChannel.pointer(), commandChannel.size());
}

void CDebuggerServer::SendStack()
{
	CMemoryWriter commandChannel;

	commandChannel.w_u16(CommandId_SetStack);
	commandChannel.w_u32(CProcUnit::GetCountRunContext());

	for (unsigned int i = CProcUnit::GetCountRunContext(); i > 0; i--) { //передаем снизу вверх

		CRunContext* runContext = CProcUnit::GetRunContext(i - 1);
		CByteCode* byteCode = runContext->GetByteCode();
		wxASSERT(runContext && byteCode);
		CCompileContext* compileContext = runContext->m_compileContext;
		wxASSERT(compileContext);
		CCompileCode* compileCode = compileContext->m_compileModule;
		wxASSERT(compileCode);
		if (compileCode->m_bExpressionOnly)
			continue;
		if (byteCode != nullptr) {
			const long lCurLine = runContext->m_lCurLine;
			if (lCurLine >= 0 && lCurLine <= (long)byteCode->m_listCode.size()) {
				wxString strFullName = byteCode->m_listCode[lCurLine].m_strModuleName;
				strFullName += wxT(".");
				if (compileContext->m_functionContext) {
					strFullName += compileContext->m_functionContext->m_strRealName;
					strFullName += wxT("(");
					for (unsigned int j = 0; j < compileContext->m_functionContext->m_listParam.size(); j++) {
						const wxString& valStr = runContext->m_pRefLocVars[compileContext->m_listVariable[stringUtils::MakeUpper(compileContext->m_functionContext->m_listParam[j].m_strName)]->m_numVariable]->GetString();
						if (j != compileContext->m_functionContext->m_listParam.size() - 1) {
							strFullName += compileContext->m_functionContext->m_listParam[j].m_strName + wxT(" = ") + valStr + wxT(", ");
						}
						else {
							strFullName += compileContext->m_functionContext->m_listParam[j].m_strName + wxT(" = ") + valStr;
						}
					}
					strFullName += wxT(")");
				}
				else {
					strFullName += wxT("<initializer>");
				}
				commandChannel.w_stringZ(byteCode->m_listCode[lCurLine].m_strDocPath);
				commandChannel.w_stringZ(strFullName);
				commandChannel.w_u32(byteCode->m_listCode[lCurLine].m_numLine + 1);
			}
		}
	}

	SendCommand(commandChannel.pointer(), commandChannel.size());
}

void CDebuggerServer::RecvCommand(void* pointer, unsigned int length)
{
	if (m_socketConnectionThread != nullptr)
		m_socketConnectionThread->RecvCommand(pointer, length);
}

void CDebuggerServer::SendCommand(void* pointer, unsigned int length)
{
	if (m_socketConnectionThread != nullptr)
		m_socketConnectionThread->SendCommand(pointer, length);
}

//////////////////////////////////////////////////////////////////////

void CDebuggerServer::CDebuggerServerConnection::WaitConnection()
{
	m_waitConnection = true;
}

void CDebuggerServer::CDebuggerServerConnection::Disconnect()
{
	m_connectionType = m_waitConnection ?
		ConnectionType::ConnectionType_Waiter : ConnectionType::ConnectionType_Scanner;

	if (m_socket != nullptr)
		m_socket->Destroy();

	m_socket = nullptr;
}

CDebuggerServer::CDebuggerServerConnection::CDebuggerServerConnection(const wxString& strHostName, unsigned short numHostPort) :
	wxThread(wxTHREAD_DETACHED), m_socket(nullptr), m_socketServer(nullptr),
	m_connectionType(ConnectionType::ConnectionType_Unknown),
	m_strHostName(strHostName), m_numHostPort(numHostPort),
	m_waitConnection(false), m_acceptConnection(false)
{
}

CDebuggerServer::CDebuggerServerConnection::~CDebuggerServerConnection()
{
	if (m_socket != nullptr)
		m_socket->Destroy();

	m_socket = nullptr;

	if (m_socketServer != nullptr)
		m_socketServer->Destroy();

	m_socketServer = nullptr;

	// the thread is being destroyed; make sure not to leave dangling pointers around
	if (ms_debugServer != nullptr)
		ms_debugServer->SetConnectSocket(nullptr);
}

wxThread::ExitCode CDebuggerServer::CDebuggerServerConnection::Entry()
{
#ifdef __WXMSW__
	HRESULT hr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		wxLogSysError(hr, _("Failed to create an instance in thread!"));
	}
#endif // !_WXMSW

	ExitCode retCode = 0;

	try {
		EntryClient();
	}
	catch (...) {
		retCode = (ExitCode)1;
	}

#ifdef __WXMSW__
	if (SUCCEEDED(hr)) {
		::CoUninitialize();
	}
#endif // !_WXMSW

	if (ms_debugServer != nullptr)
		ms_debugServer->ResetDebugger();

	return retCode;
}

void CDebuggerServer::CDebuggerServerConnection::OnKill()
{
	if (m_socket != nullptr)
		m_socket->Destroy();

	m_socket = nullptr;

	if (m_socketServer != nullptr)
		m_socketServer->Destroy();

	m_socketServer = nullptr;

	// the thread is being destroyed; make sure not to leave dangling pointers around
	if (ms_debugServer != nullptr)
		ms_debugServer->SetConnectSocket(nullptr);
}

void CDebuggerServer::CDebuggerServerConnection::EntryClient()
{
	unsigned short numHostPort = m_numHostPort;

	while (true) {

		if (m_socketServer != nullptr) {
			numHostPort++;
			m_socketServer->Destroy();
			m_socketServer = nullptr;
		}

		if (numHostPort > m_numHostPort + diapasonDebuggerPort)
			break;

		wxIPV4address addr;

		addr.Hostname(m_strHostName);
		addr.Service(numHostPort);

		m_socketServer = new wxSocketServer(addr, wxSOCKET_BLOCK | wxSOCKET_WAITALL);
		m_socketServer->SetTimeout(10);

		if (m_socketServer->IsOk())
			break;
	}

	while (!TestDestroy()) {

		if (m_socketServer == nullptr)
			break;

		while (!TestDestroy()) {

			if (m_socketServer != nullptr && m_waitConnection) {
				m_socket = m_socketServer->Accept(true);
			}
			else if (m_socketServer != nullptr && m_socketServer->WaitForAccept(0, waitDebuggerTimeout)) {
				m_socket = m_socketServer->Accept(false);
			}

			if (m_socket != nullptr || m_waitConnection)
				break;
		}

		m_acceptConnection = true;

		while (!TestDestroy() && CDebuggerServerConnection::IsConnected()) {
			if (m_socket != nullptr && m_socket->WaitForRead(0, waitDebuggerTimeout)) {
				unsigned int length = 0;
				m_socket->ReadMsg(&length, sizeof(unsigned int));
				if (m_socket && m_socket->WaitForRead(0, waitDebuggerTimeout)) {
					wxMemoryBuffer bufferData(length);
					m_socket->ReadMsg(bufferData.GetData(), length);
					if (length > 0) {
						CValueOLE::GetInterfaceAndReleaseStream();
#if _USE_NET_COMPRESSOR == 1
						BYTE* dest = nullptr; unsigned int dest_sz = 0;
						_decompressLZ(&dest, &dest_sz, bufferData.GetData(), bufferData.GetBufSize());
						RecvCommand(dest, dest_sz); free(dest);
#else
						RecvCommand(bufferData.GetData(), bufferData.GetBufSize());
#endif 
						length = 0;
					}
				}
			}
		}

		if (m_socket != nullptr)
			m_socket->Destroy();

		m_waitConnection = false;

		m_socket = nullptr;
		m_connectionType = ConnectionType::ConnectionType_Unknown;
	}

	if (m_socket != nullptr)
		m_socket->Destroy();

	m_waitConnection = false;
	m_acceptConnection = false;

	m_socket = nullptr;
	m_connectionType = ConnectionType::ConnectionType_Unknown;
}

void CDebuggerServer::CDebuggerServerConnection::RecvCommand(void* pointer, unsigned int length)
{
	CMemoryReader commandReader(pointer, length);
	wxASSERT(ms_debugServer != nullptr);
	u16 commandFromClient = commandReader.r_u16();

	if (commandFromClient == CommandId_VerifyConnection) {

		m_connectionType = m_waitConnection ?
			ConnectionType::ConnectionType_Waiter : ConnectionType::ConnectionType_Scanner;

		CMemoryWriter commandChannel_SetConnectionType;
		commandChannel_SetConnectionType.w_u16(CommandId_SetConnectionType);
		commandChannel_SetConnectionType.w_u16(m_connectionType);
		SendCommand(commandChannel_SetConnectionType.pointer(), commandChannel_SetConnectionType.size());

		CMemoryWriter commandChannel_VerifyConnection;
		commandChannel_VerifyConnection.w_u16(CommandId_VerifyConnection);
		commandChannel_VerifyConnection.w_stringZ(activeMetaData->GetConfigGuid());
		commandChannel_VerifyConnection.w_stringZ(activeMetaData->GetConfigMD5());
		commandChannel_VerifyConnection.w_stringZ(appData->GetUserName());
		commandChannel_VerifyConnection.w_stringZ(appData->GetComputerName());
		SendCommand(commandChannel_VerifyConnection.pointer(), commandChannel_VerifyConnection.size());
	}
	else if (commandFromClient == CommandId_SetConnectionType) {
		m_connectionType = static_cast<ConnectionType>(commandReader.r_u16());
		if (m_connectionType == ConnectionType::ConnectionType_Unknown)
			CDebuggerServerConnection::Disconnect();
	}
	else if (commandFromClient == CommandId_StartSession) {
		CMemoryWriter commandChannel;
		commandChannel.w_u16(CommandId_GetArrayBreakpoint);
		SendCommand(commandChannel.pointer(), commandChannel.size());
		m_connectionType = ConnectionType::ConnectionType_Debugger;
	}
	else if (commandFromClient == CommandId_SetArrayBreakpoint) {
		unsigned int countBreakpoints = commandReader.r_u32();
		//parse breakpoints 
		for (unsigned int i = 0; i < countBreakpoints; i++) {
			unsigned int countBreakPoints = commandReader.r_u32();
			wxString strModuleName; commandReader.r_stringZ(strModuleName);
			for (unsigned int j = 0; j < countBreakPoints; j++) {
				ms_debugServer->m_listBreakpoint[strModuleName].push_back(commandReader.r_u32());
			}
		}
		ms_debugServer->m_bUseDebug = true;
	}
	else if (commandFromClient == CommandId_ToggleBreakpoint) {
		wxString strModuleName; commandReader.r_stringZ(strModuleName);
		unsigned int line = commandReader.r_u32();
		ms_debugServer->m_listBreakpoint[strModuleName].push_back(line);
	}
	else if (commandFromClient == CommandId_RemoveBreakpoint) {

		wxString strModuleName; commandReader.r_stringZ(strModuleName);
		unsigned int line = commandReader.r_u32();
		{
			auto& module_breakpoint = ms_debugServer->m_listBreakpoint[strModuleName];
			module_breakpoint.erase(
				std::remove(module_breakpoint.begin(), module_breakpoint.end(), line), module_breakpoint.end());
		}

		if (ms_debugServer->m_listBreakpoint[strModuleName].size() == 0)
			ms_debugServer->m_listBreakpoint.erase(strModuleName);
	}
	else if (commandFromClient == CommandId_AddExpression) {
		wxString strExpression; commandReader.r_stringZ(strExpression);
#if _USE_64_BIT_POINT_IN_DEBUGGER == 1
		unsigned long long id = commandReader.r_u64();
#else 
		unsigned int id = commandReader.r_u32();
#endif 
		CMemoryWriter commandChannel;

		commandChannel.w_u16(CommandId_SetExpressions);
		commandChannel.w_u32(1); // first elements 

		if (ms_debugServer->IsDebugLooped()) {
			CValue vResult;
			//header 
#if _USE_64_BIT_POINT_IN_DEBUGGER == 1
			commandChannel.w_u64(id);
#else
			commandChannel.w_u32(id);
#endif 
			//variable
			commandChannel.w_stringZ(strExpression);
			if (CProcUnit::Evaluate(strExpression, ms_debugServer->m_runContext, vResult, false)) {
				commandChannel.w_stringZ(vResult.GetString());
				commandChannel.w_stringZ(vResult.GetClassName());
				//count of elemetns 
				commandChannel.w_u32(vResult.GetNProps());
			}
			else {
				commandChannel.w_stringZ(CBackendException::GetLastError());
				commandChannel.w_stringZ(wxT("<error>"));
				//count of elemetns 
				commandChannel.w_u32(0);
			}
			//send expression 
			SendCommand(commandChannel.pointer(), commandChannel.size());
			//set expression in map 
			ms_debugServer->m_listExpression.insert_or_assign(id, strExpression);
		}
		else {
			//header 
#if _USE_64_BIT_POINT_IN_DEBUGGER == 1
			commandChannel.w_u64(id);
#else
			commandChannel.w_u32(id);
#endif 
			//variable
			commandChannel.w_stringZ(strExpression);
			commandChannel.w_stringZ(wxEmptyString);
			commandChannel.w_stringZ(wxEmptyString);
			//count of elemetns 
			commandChannel.w_u32(0);
			//send expression 
			SendCommand(commandChannel.pointer(), commandChannel.size());
			//set expression in map 
			ms_debugServer->m_listExpression.insert_or_assign(id, strExpression);
		}
	}
	else if (commandFromClient == CommandId_ExpandExpression) {
		wxString strExpression;
		commandReader.r_stringZ(strExpression);
		if (ms_debugServer->IsDebugLooped()) {
			CValue vResult;
#if _USE_64_BIT_POINT_IN_DEBUGGER == 1
			unsigned long long id = commandReader.r_u64();
#else 
			unsigned int id = commandReader.r_u32();
#endif
			if (CProcUnit::Evaluate(strExpression, ms_debugServer->m_runContext, vResult, false)) {
				CMemoryWriter commandChannel;
				commandChannel.w_u16(CommandId_ExpandExpression);
#if _USE_64_BIT_POINT_IN_DEBUGGER == 1
				commandChannel.w_u64(id);
#else 
				commandChannel.w_u32(id);
#endif 
				//count of attribute  
				commandChannel.w_u32(vResult.GetNProps());

				//send varables 
				for (long i = 0; i < vResult.GetNProps(); i++) {
					const wxString& strPropName = vResult.GetPropName(i); const long lPropNum = vResult.FindProp(strPropName);
					if (lPropNum != wxNOT_FOUND) {
						try {
							if (!vResult.IsPropReadable(lPropNum))
								CBackendException::Error(_("Object field not readable (%s)"), strPropName);
							//send attribute body
							commandChannel.w_stringZ(strPropName);
							CValue vAttribute;
							if (vResult.GetPropVal(lPropNum, vAttribute)) {
								commandChannel.w_stringZ(vAttribute.GetString());
								commandChannel.w_stringZ(vAttribute.GetClassName());
							}
							else {
								commandChannel.w_stringZ(CBackendException::GetLastError());
								commandChannel.w_stringZ(wxT("<error>"));
							}
							//count of attribute   
							commandChannel.w_u32(vAttribute.GetNProps());
						}
						catch (const CBackendException* err) {
							wxString strErrorMessage = err->what();
							strErrorMessage.Replace('\n', ' ');
							//send attribute body
							commandChannel.w_stringZ(strPropName);
							commandChannel.w_stringZ(strErrorMessage);
							commandChannel.w_stringZ(wxT("<error>"));
							//count of attribute   
							commandChannel.w_u32(0);
						}
					}
					else {
						//send attribute body
						commandChannel.w_stringZ(strPropName);
						commandChannel.w_stringZ(wxT("<not found>"));
						commandChannel.w_stringZ(wxT("<error>"));
						//count of attribute   
						commandChannel.w_u32(0);
					}
				}
				SendCommand(commandChannel.pointer(), commandChannel.size());
			}
		}
	}
	else if (commandFromClient == CommandId_RemoveExpression) {
#if _USE_64_BIT_POINT_IN_DEBUGGER == 1
		ms_debugServer->m_listExpression.erase(commandReader.r_u64());
#else 
		ms_debugServer->m_listExpression.erase(commandReader.r_u32());
#endif 
	}
	else if (commandFromClient == CommandId_EvalToolTip) {
		wxString strFileName, strModuleName, strExpression;
		commandReader.r_stringZ(strFileName);
		commandReader.r_stringZ(strModuleName);
		commandReader.r_stringZ(strExpression);
		if (ms_debugServer->IsDebugLooped()) {
			CValue vResult;
			if (CProcUnit::Evaluate(strExpression, ms_debugServer->m_runContext, vResult, false)) {
				CMemoryWriter commandChannel;
				commandChannel.w_u16(CommandId_EvalToolTip);
				commandChannel.w_stringZ(strFileName);
				commandChannel.w_stringZ(strModuleName);
				commandChannel.w_stringZ(strExpression);
				commandChannel.w_stringZ(vResult.GetString());
				if (ms_debugServer->IsDebugLooped()) {
					SendCommand(commandChannel.pointer(), commandChannel.size());
				}
			}
		}
	}
	else if (commandFromClient == CommandId_SetStack) {
		unsigned int stackLevel = commandReader.r_u32();
		CRunContext* newRunContext =
			CProcUnit::GetRunContext(stackLevel);
		if (newRunContext) {
			ms_debugServer->m_runContext = newRunContext;
			//send expressions from user 
			ms_debugServer->SendExpressions();
			//send local variable
			ms_debugServer->SendLocalVariables();
		}
	}
	else if (commandFromClient == CommandId_EvalAutocomplete) {

		wxString strFileName, strModuleName, strExpression, strKeyWord;

		commandReader.r_stringZ(strFileName);
		commandReader.r_stringZ(strModuleName);
		commandReader.r_stringZ(strExpression);
		commandReader.r_stringZ(strKeyWord);

		s32 currPos = commandReader.r_s32();
		if (ms_debugServer->IsDebugLooped()) {
			CValue vResult;
			if (CProcUnit::Evaluate(strExpression, ms_debugServer->m_runContext, vResult, false)) {

				CMemoryWriter commandChannel;
				commandChannel.w_u16(CommandId_EvalAutocomplete);
				commandChannel.w_stringZ(strFileName);
				commandChannel.w_stringZ(strModuleName);
				commandChannel.w_stringZ(strExpression);
				commandChannel.w_stringZ(strKeyWord);
				commandChannel.w_s32(currPos);

				commandChannel.w_u32(vResult.GetNProps());
				//send varables 
				for (long i = 0; i < vResult.GetNProps(); i++) {
					const wxString& strAttributeName = vResult.GetPropName(i);
					commandChannel.w_stringZ(strAttributeName);
				}

				commandChannel.w_u32(vResult.GetNMethods());
				//send functions 
				for (long i = 0; i < vResult.GetNMethods(); i++) {
					const wxString& strMethodName = vResult.GetMethodName(i);
					const wxString& strMethodDescription = vResult.GetMethodHelper(i);
					//send attribute body
					commandChannel.w_stringZ(strMethodName);
					commandChannel.w_stringZ(strMethodDescription);
					commandChannel.w_u8(vResult.HasRetVal(i));
				}

				SendCommand(commandChannel.pointer(), commandChannel.size());
			}
		}
	}
	else if (commandFromClient == CommandId_Continue) {
		ms_debugServer->m_bDebugStopLine = false;
		ms_debugServer->m_bDebugLoop = ms_debugServer->m_bDoLoop = false;
	}
	else if (commandFromClient == CommandId_StepInto) {
		if (ms_debugServer->IsDebugLooped()) {
			ms_debugServer->m_bDebugStopLine = true;
			ms_debugServer->m_bDebugLoop = ms_debugServer->m_bDoLoop = false;
		}
	}
	else if (commandFromClient == CommandId_StepOver) {
		if (ms_debugServer->IsDebugLooped()) {
			ms_debugServer->m_numCurrentNumberStopContext = CProcUnit::GetCountRunContext();
			ms_debugServer->m_bDebugLoop = ms_debugServer->m_bDoLoop = false;
		}
	}
	else if (commandFromClient == CommandId_Pause) {
		ms_debugServer->m_bDebugStopLine = true;
	}
	else if (commandFromClient == CommandId_Detach) {

		ms_debugServer->m_bUseDebug =
			ms_debugServer->m_bDebugLoop =
			ms_debugServer->m_bDoLoop = false;

		CDebuggerServerConnection::Disconnect();
	}
	else if (commandFromClient == CommandId_Destroy) {

		ms_debugServer->m_bUseDebug =
			ms_debugServer->m_bDebugLoop =
			ms_debugServer->m_bDoLoop = false;

		CDebuggerServerConnection::Disconnect();

#ifdef __WXMSW__
		::CoUninitialize();
#endif // !_WXMSW		

		CApplicationData::ForceExit();
	}
	else if (commandFromClient == CommandId_DeleteAllBreakpoints) {
		ms_debugServer->m_listBreakpoint.clear();
	}
}

void CDebuggerServer::CDebuggerServerConnection::SendCommand(void* pointer, unsigned int length)
{
#if _USE_NET_COMPRESSOR == 1
	BYTE* dest = nullptr; unsigned int dest_sz = 0;
	_compressLZ(&dest, &dest_sz, pointer, length);
	if (m_socket && m_socket->IsOk()) {
		m_socket->WriteMsg(&dest_sz, sizeof(unsigned int));
		m_socket->WriteMsg(dest, dest_sz);
	}
	free(dest);
#else
	if (m_socket && CDebuggerServerConnection::IsConnected()) {
		m_socket->WriteMsg(&length, sizeof(unsigned int));
		m_socket->WriteMsg(pointer, length);
	}
#endif
}