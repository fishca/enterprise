////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : debugger - client part
////////////////////////////////////////////////////////////////////////////

#include "debugClient.h"
#include "backend/metadataConfiguration.h"

#include "backend/fileSystem/fs.h"
#if _USE_NET_COMPRESSOR == 1
#include "utils/fs/lz/lzhuf.h"
#endif 

///////////////////////////////////////////////////////////////////////
CDebuggerClient* CDebuggerClient::ms_debugClient = nullptr;
///////////////////////////////////////////////////////////////////////

bool CDebuggerClient::Initialize()
{
	if (!CDebuggerClient::TableAlreadyCreated()) {
		CDebuggerClient::CreateBreakpointDatabase();
	}

	if (ms_debugClient != nullptr)
		ms_debugClient->Destroy();

	ms_debugClient = new CDebuggerClient();

	unsigned int debugOffsetPort = 0;
	while (debugOffsetPort < diapasonDebuggerPort) {
		ms_debugClient->CreateConnection(
			defaultHost,
			defaultDebuggerPort + debugOffsetPort
		);
		debugOffsetPort++;
	}

	return true;
}

void CDebuggerClient::Destroy()
{
	wxDELETE(ms_debugClient);
}

void CDebuggerClient::ConnectToServer(const wxString& hostName, unsigned short port)
{
	CDebuggerClientConnection* foundedConnection = FindConnection(hostName, port);
	if (foundedConnection != nullptr) {
		foundedConnection->AttachConnection();
	}
}

CDebuggerClient::CDebuggerClientConnection* CDebuggerClient::FindConnection(const wxString& hostName, unsigned short port)
{
	auto foundedConnection = std::find_if(m_listConnection.begin(), m_listConnection.end(), [hostName, port](CDebuggerClientConnection* client) {
		return client->m_hostName == hostName && client->m_port == port;
		}
	);

	if (foundedConnection != m_listConnection.end()) {
		return *foundedConnection;
	}

	return nullptr;
}

CDebuggerClient::CDebuggerClientConnection* CDebuggerClient::FindDebugger(const wxString& hostName, unsigned short port)
{
	auto foundedConnection = std::find_if(m_listConnection.begin(), m_listConnection.end(), [hostName, port](CDebuggerClientConnection* client) {
		return client->m_hostName == hostName && client->m_port == port && client->GetConnectionType() == ConnectionType::ConnectionType_Debugger;
		});

	if (foundedConnection != m_listConnection.end()) {
		return *foundedConnection;
	}

	return nullptr;
}

void CDebuggerClient::SearchServer(const wxString& hostName, unsigned short startPort)
{
	for (unsigned short currentPort = startPort; currentPort < startPort + diapasonDebuggerPort; currentPort++) {
		CDebuggerClientConnection* sockDebugger = FindConnection(hostName, currentPort);
		if (sockDebugger && !sockDebugger->m_verifiedConnection) { sockDebugger->AttachConnection(); break; }
	}
}

//special functions:
void CDebuggerClient::Continue()
{
	CMemoryWriter commandChannel;
	commandChannel.w_u16(CommandId_Continue);
	SendCommand(commandChannel.pointer(), commandChannel.size());
}

void CDebuggerClient::StepOver()
{
	CMemoryWriter commandChannel;
	commandChannel.w_u16(CommandId_StepOver);
	SendCommand(commandChannel.pointer(), commandChannel.size());
}

void CDebuggerClient::StepInto()
{
	CMemoryWriter commandChannel;
	commandChannel.w_u16(CommandId_StepInto);
	SendCommand(commandChannel.pointer(), commandChannel.size());
}

void CDebuggerClient::Pause()
{
	CMemoryWriter commandChannel;
	commandChannel.w_u16(CommandId_Pause);
	SendCommand(commandChannel.pointer(), commandChannel.size());
}

void CDebuggerClient::Stop(bool kill)
{
	for (auto connection : m_listConnection) {
		if (connection->IsConnected()) connection->DetachConnection(kill);
	}
}

void CDebuggerClient::InitializeModule(const wxString& strModuleName, unsigned int line_count)
{
	m_listOffsetBreakpoint[strModuleName].clear();
	for (unsigned int i = 0; i < line_count; i++)
		m_listOffsetBreakpoint[strModuleName].emplace(i, 0);

	LoadBreakpointCollection(strModuleName);
}

void CDebuggerClient::PatchModule(const wxString& strModuleName, unsigned int line, int line_offset)
{
	auto breakpoint_iterator = std::find_if(m_listBreakpoint.begin(), m_listBreakpoint.end(),
		[strModuleName](const auto pair) { return stringUtils::CompareString(pair.first, strModuleName); });

	if (breakpoint_iterator != m_listBreakpoint.end()) {

		auto& list_breakpoint = breakpoint_iterator->second;
		{
			auto list_breakpoint_iterator = list_breakpoint.begin();
			while (list_breakpoint_iterator != list_breakpoint.end()) {

				const unsigned int calc_line = list_breakpoint_iterator->first + list_breakpoint_iterator->second;

				if (calc_line > line)
					list_breakpoint_iterator->second += line_offset;

				list_breakpoint_iterator = std::next(list_breakpoint_iterator);

				while (line_offset > 0 && list_breakpoint_iterator != list_breakpoint.end()) {
					const unsigned int next_calc_line = list_breakpoint_iterator->first + list_breakpoint_iterator->second;
					if (calc_line != next_calc_line)
						break;
					list_breakpoint_iterator = list_breakpoint.erase(list_breakpoint_iterator);
				}
			}
		}
	}

	auto module_offset_iterator = std::find_if(m_listOffsetBreakpoint.begin(), m_listOffsetBreakpoint.end(),
		[strModuleName](const auto pair) { return stringUtils::CompareString(pair.first, strModuleName); });

	if (module_offset_iterator != m_listOffsetBreakpoint.end()) {

		auto& list_module_offset = module_offset_iterator->second;
		{
			auto list_module_offset_iterator = list_module_offset.begin(); bool set_offset_value = false;
			while (list_module_offset_iterator != list_module_offset.end()) {

				const unsigned int calc_line = list_module_offset_iterator->first + list_module_offset_iterator->second;

				if (calc_line > line || list_module_offset.size() == 1) {
					list_module_offset_iterator->second += line_offset;
					set_offset_value = true;
				}

				auto list_module_offset_iterator_start = list_module_offset_iterator;
				auto list_module_offset_iterator_end = list_module_offset_iterator;

				list_module_offset_iterator = std::next(list_module_offset_iterator);

				while (line_offset > 0 && list_module_offset_iterator != list_module_offset.end()) {

					const unsigned int next_calc_line = list_module_offset_iterator->first + list_module_offset_iterator->second;
					if (calc_line != next_calc_line)
						break;

					list_module_offset_iterator->second += line_offset;

					list_module_offset_iterator_end = list_module_offset_iterator;
					list_module_offset_iterator = std::next(list_module_offset_iterator);
				}
			}

			if (!set_offset_value && list_module_offset.size() > 0) {
				list_module_offset_iterator = std::prev(list_module_offset.end());
				list_module_offset_iterator->second += line_offset;
			}
		}
	}

	CMemoryWriter commandChannel;

	commandChannel.w_u16(line_offset > 0 ? CommandId_PatchInsertLine : CommandId_PatchDeleteLine);
	commandChannel.w_stringZ(strModuleName);
	commandChannel.w_u32(line);
	commandChannel.w_s32(line_offset);

	SendCommand(commandChannel.pointer(), commandChannel.size());
}

bool CDebuggerClient::SaveModule(const wxString& strModuleName, unsigned int line_count)
{
	//initialize breakpoint 
	auto breakpoint_iterator = std::find_if(m_listBreakpoint.begin(), m_listBreakpoint.end(),
		[strModuleName](const auto pair) { return stringUtils::CompareString(pair.first, strModuleName); });

	if (breakpoint_iterator != m_listBreakpoint.end()) {

		std::map<unsigned int, int>& list_breakpoint = breakpoint_iterator->second, moduleBreakpointsNew;
		for (auto it = list_breakpoint.begin(); it != list_breakpoint.end(); it++) {
			if (!OffsetBreakpointInDB(breakpoint_iterator->first, it->first, it->second))
				return false;
			moduleBreakpointsNew.emplace(it->first + it->second, 0);
		}

		list_breakpoint.clear();

		for (auto it = moduleBreakpointsNew.begin(); it != moduleBreakpointsNew.end(); it++) {
			list_breakpoint.emplace(it->first, it->second);
		}
	}

	//initialize offsets 
	auto module_offset_iterator = std::find_if(m_listOffsetBreakpoint.begin(), m_listOffsetBreakpoint.end(),
		[strModuleName](const auto pair) { return stringUtils::CompareString(pair.first, strModuleName); });

	if (module_offset_iterator != m_listOffsetBreakpoint.end()) {

		std::map<unsigned int, int>& list_module_offset = module_offset_iterator->second;
		std::map<unsigned int, int> ::iterator it = list_module_offset.begin();

		if (it != list_module_offset.end()) {
			std::advance(it, list_module_offset.size() - 1);
			if (it != list_module_offset.end()) {
				InitializeModule(module_offset_iterator->first, it->first + it->second + 1);
			}
			else {
				InitializeModule(module_offset_iterator->first, line_count);
			}
		}
		else {
			InitializeModule(module_offset_iterator->first, line_count);
		}
	}
	else {
		InitializeModule(strModuleName, line_count);
	}

	CMemoryWriter commandChannel;
	commandChannel.w_u16(CommandId_PatchComplete);
	commandChannel.w_stringZ(strModuleName);

	SendCommand(commandChannel.pointer(), commandChannel.size());
	return true;
}

void CDebuggerClient::RemoveModule(const wxString& strModuleName)
{
	auto breakpoint_iterator = std::find_if(m_listBreakpoint.begin(), m_listBreakpoint.end(),
		[strModuleName](const auto pair) { return stringUtils::CompareString(pair.first, strModuleName); });
	if (breakpoint_iterator != m_listBreakpoint.end())
		m_listBreakpoint[breakpoint_iterator->first].clear();

	auto module_offset_iterator = std::find_if(m_listOffsetBreakpoint.begin(), m_listOffsetBreakpoint.end(),
		[strModuleName](const auto pair) { return stringUtils::CompareString(pair.first, strModuleName); });
	if (module_offset_iterator != m_listOffsetBreakpoint.end())
		m_listOffsetBreakpoint[module_offset_iterator->first].clear();
}

bool CDebuggerClient::SaveAllBreakpoints()
{
	//initialize breakpoint 
	for (auto breakpoint_iterator = m_listBreakpoint.begin(); breakpoint_iterator != m_listBreakpoint.end(); breakpoint_iterator++) {
		std::map<unsigned int, int>& list_breakpoint = breakpoint_iterator->second, moduleBreakpointsNew;
		for (auto it = list_breakpoint.begin(); it != list_breakpoint.end(); it++) {
			if (!OffsetBreakpointInDB(breakpoint_iterator->first, it->first, it->second))
				return false;
			moduleBreakpointsNew.emplace(it->first + it->second, 0);
		}
		list_breakpoint.clear();
		for (auto it = moduleBreakpointsNew.begin(); it != moduleBreakpointsNew.end(); it++) {
			list_breakpoint.emplace(it->first, it->second);
		}
	}

	//initialize offsets 
	for (auto module_offset_iterator = m_listOffsetBreakpoint.begin(); module_offset_iterator != m_listOffsetBreakpoint.end(); module_offset_iterator++) {
		auto& list_module_offset = module_offset_iterator->second;
		std::map<unsigned int, int> ::iterator it = list_module_offset.begin(); std::advance(it, list_module_offset.size() - 1);
		if (it != list_module_offset.end()) {
			InitializeModule(module_offset_iterator->first, it->first + it->second + 1);
		}
		else {
			InitializeModule(module_offset_iterator->first, 1);
		}
	}

	for (auto breakpoint_iterator = m_listBreakpoint.begin();
		breakpoint_iterator != m_listBreakpoint.end(); breakpoint_iterator++) {
		CMemoryWriter commandChannel;
		commandChannel.w_u16(CommandId_PatchComplete);
		commandChannel.w_stringZ(breakpoint_iterator->first);
		SendCommand(commandChannel.pointer(), commandChannel.size());
	}

	return true;
}

bool CDebuggerClient::ToggleBreakpoint(const wxString& strModuleName, unsigned int line)
{
	unsigned int startLine = line; int locOffsetPrev = 0, locOffsetCurr = 0;
	std::map<unsigned int, int>& list_module_offset = m_listOffsetBreakpoint[strModuleName];

	for (auto it = list_module_offset.begin(); it != list_module_offset.end(); it++) {
		if (it->second < 0 && (int)it->first < -it->second) { locOffsetPrev = it->second; continue; }
		locOffsetCurr = it->second;
		if ((it->first + locOffsetPrev) <= line && (it->first + locOffsetCurr) >= line) { startLine = it->first; break; }
		locOffsetPrev = it->second;
	}
	std::map<unsigned int, int>::iterator itOffset = list_module_offset.find(startLine);
	if (itOffset != list_module_offset.end()) {
		if (line != (itOffset->first + itOffset->second)) {
			wxMessageBox(_("Cannot set breakpoint in unsaved copy!"));
			return false;
		}
	}
	else {
		wxMessageBox(_("Cannot set breakpoint in unsaved copy!"));
		return false;
	}
	std::map<unsigned int, int>& list_breakpoint = m_listBreakpoint[strModuleName];
	std::map<unsigned int, int>::iterator breakpoint_iterator = list_breakpoint.find(itOffset->first);
	unsigned int currLine = itOffset->first; int offset = itOffset->second;
	if (breakpoint_iterator == list_breakpoint.end()) {
		if (ToggleBreakpointInDB(strModuleName, currLine)) {
			list_breakpoint.emplace(currLine, offset);
			CMemoryWriter commandChannel;
			commandChannel.w_u16(CommandId_ToggleBreakpoint);
			commandChannel.w_stringZ(strModuleName);
			commandChannel.w_u32(currLine);
			commandChannel.w_s32(offset);
			SendCommand(commandChannel.pointer(), commandChannel.size());
		}
		else {
			return false;
		}
	}

	return true;
}

bool CDebuggerClient::RemoveBreakpoint(const wxString& strModuleName, unsigned int line)
{
	unsigned int startLine = line; int locOffsetPrev = 0, locOffsetCurr = 0;
	std::map<unsigned int, int>& list_module_offset = m_listOffsetBreakpoint[strModuleName];
	for (auto it = list_module_offset.begin(); it != list_module_offset.end(); it++) {
		if (it->second < 0 && (int)it->first < -it->second) {
			locOffsetPrev = it->second; continue;
		}
		locOffsetCurr = it->second;
		if ((it->first + locOffsetPrev) <= line && (it->first + locOffsetCurr) >= line) {
			startLine = it->first; break;
		}
		locOffsetPrev = it->second;
	}
	std::map<unsigned int, int>::iterator itOffset = list_module_offset.find(startLine);
	std::map<unsigned int, int>& list_breakpoint = m_listBreakpoint[strModuleName];
	std::map<unsigned int, int>::iterator breakpoint_iterator = list_breakpoint.find(itOffset->first);
	unsigned int currLine = itOffset->first;
	if (breakpoint_iterator != list_breakpoint.end()) {
		if (RemoveBreakpointInDB(strModuleName, currLine)) {
			list_breakpoint.erase(breakpoint_iterator);
			CMemoryWriter commandChannel;
			commandChannel.w_u16(CommandId_RemoveBreakpoint);
			commandChannel.w_stringZ(strModuleName);
			commandChannel.w_u32(currLine);
			SendCommand(commandChannel.pointer(), commandChannel.size());
		}
		else {
			return false;
		}
	}
	return true;
}

#include "backend/backend_mainFrame.h"

void CDebuggerClient::RemoveAllBreakpoint()
{
	CMemoryWriter commandChannel;
	commandChannel.w_u16(CommandId_DeleteAllBreakpoints);
	SendCommand(commandChannel.pointer(), commandChannel.size());
	if (RemoveAllBreakpointInDB()) {
		m_listBreakpoint.clear();
		if (backend_mainFrame != nullptr) {
			backend_mainFrame->RefreshFrame();
		}
	}
	else {
		wxMessageBox("Error in : void CDebuggerClient::RemoveAllBreakpoint()");
	}
}

#if _USE_64_BIT_POINT_IN_DEBUGGER == 1
void CDebuggerClient::AddExpression(const wxString& strExpression, unsigned long long id)
#else 
void CDebuggerClient::AddExpression(const wxString& strExpression, unsigned int id)
#endif 
{
	CMemoryWriter commandChannel;

	commandChannel.w_u16(CommandId_AddExpression);
	commandChannel.w_stringZ(strExpression);
#if _USE_64_BIT_POINT_IN_DEBUGGER == 1
	commandChannel.w_u64(id);
#else 
	commandChannel.w_u32(id);
#endif 

	SendCommand(commandChannel.pointer(), commandChannel.size());

	//set expression in map 
	m_listExpression.insert_or_assign(id, strExpression);
}

#if _USE_64_BIT_POINT_IN_DEBUGGER == 1
void CDebuggerClient::ExpandExpression(const wxString& strExpression, unsigned long long id)
#else
void CDebuggerClient::ExpandExpression(const wxString& strExpression, unsigned int id)
#endif 
{
	CMemoryWriter commandChannel;

	commandChannel.w_u16(CommandId_ExpandExpression);
	commandChannel.w_stringZ(strExpression);
#if _USE_64_BIT_POINT_IN_DEBUGGER == 1
	commandChannel.w_u64(id);
#else 
	commandChannel.w_u32(id);
#endif 

	SendCommand(commandChannel.pointer(), commandChannel.size());
}

#if _USE_64_BIT_POINT_IN_DEBUGGER == 1
void CDebuggerClient::RemoveExpression(unsigned long long id)
#else
void CDebuggerClient::RemoveExpression(unsigned int id)
#endif 
{
	CMemoryWriter commandChannel;

	commandChannel.w_u16(CommandId_RemoveExpression);
#if _USE_64_BIT_POINT_IN_DEBUGGER == 1
	commandChannel.w_u64(id);
#else 
	commandChannel.w_u32(id);
#endif 

	SendCommand(commandChannel.pointer(), commandChannel.size());

	//delete expression from map
	m_listExpression.erase(id);
}

void CDebuggerClient::SetLevelStack(unsigned int level)
{
	if (CDebuggerClient::IsEnterLoop()) {
		CMemoryWriter commandChannel;
		commandChannel.w_u16(CommandId_SetStack);
		commandChannel.w_u32(level);
		SendCommand(commandChannel.pointer(), commandChannel.size());
	}
}

void CDebuggerClient::EvaluateToolTip(const wxString& strFileName, const wxString& strModuleName, const wxString& strExpression)
{
	if (CDebuggerClient::IsEnterLoop()) {
		CMemoryWriter commandChannel;
		commandChannel.w_u16(CommandId_EvalToolTip);
		commandChannel.w_stringZ(strFileName);
		commandChannel.w_stringZ(strModuleName);
		commandChannel.w_stringZ(strExpression);
		SendCommand(commandChannel.pointer(), commandChannel.size());
	}
}

void CDebuggerClient::EvaluateAutocomplete(const wxString& strFileName, const wxString& strModuleName, const wxString& strExpression, const wxString& keyWord, int currline)
{
	if (CDebuggerClient::IsEnterLoop()) {
		CMemoryWriter commandChannel;
		commandChannel.w_u16(CommandId_EvalAutocomplete);
		commandChannel.w_stringZ(strFileName);
		commandChannel.w_stringZ(strModuleName);
		commandChannel.w_stringZ(strExpression);
		commandChannel.w_stringZ(keyWord);
		commandChannel.w_s32(currline);
		SendCommand(commandChannel.pointer(), commandChannel.size());
	}
}

std::vector<unsigned int> CDebuggerClient::GetDebugList(const wxString& strModuleName)
{
	auto breakpoint_iterator = std::find_if(m_listBreakpoint.begin(), m_listBreakpoint.end(),
		[strModuleName](const auto pair) { return stringUtils::CompareString(pair.first, strModuleName); });

	if (breakpoint_iterator == m_listBreakpoint.end())
		return std::vector<unsigned int>();

	std::vector<unsigned int> listBreakpoint;
	for (auto& breakpoint : breakpoint_iterator->second)
		listBreakpoint.push_back(breakpoint.first + breakpoint.second);
	return listBreakpoint;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

enum eSocketType {
	wxID_SOCKET_CLIENT = 1
};

bool CDebuggerClient::CDebuggerClientConnection::AttachConnection()
{
	if (m_connectionType != ConnectionType::ConnectionType_Scanner) return false;

	if (m_verifiedConnection) {

		m_connectionType = ConnectionType::ConnectionType_Debugger;

		CMemoryWriter commandChannel;
		commandChannel.w_u16(CommandId_StartSession);
		SendCommand(commandChannel.pointer(), commandChannel.size());

		// Send the start event message to the UI.
		ms_debugClient->CallAfter(&CDebuggerClient::CDebuggerClientAdapter::OnSessionStart, m_socketClient);
	}

	return true;
}

bool CDebuggerClient::CDebuggerClientConnection::DetachConnection(bool kill)
{
	if (m_connectionType != ConnectionType::ConnectionType_Debugger) return false;

	if (m_connectionType == ConnectionType::ConnectionType_Debugger) {

		// Send the exit event message to the UI.
		ms_debugClient->CallAfter(&CDebuggerClient::CDebuggerClientAdapter::OnSessionEnd, m_socketClient);

		m_connectionType = ConnectionType::ConnectionType_Scanner;

		CMemoryWriter commandChannel;
		commandChannel.w_u16(kill ? CommandId_Destroy : CommandId_Detach);
		SendCommand(commandChannel.pointer(), commandChannel.size());

		if (m_socketClient != nullptr) m_socketClient->Close();
		m_verifiedConnection = false;

		return true;
	}

	return false;
}

wxThread::ExitCode CDebuggerClient::CDebuggerClientConnection::Entry()
{
	ExitCode retCode = (ExitCode)0;

	try {
		EntryClient();
	}
	catch (...) {
		retCode = (ExitCode)1;
	}

	return retCode;
}

void CDebuggerClient::CDebuggerClientConnection::OnKill()
{
	if (m_connectionType == ConnectionType::ConnectionType_Debugger) {
		// Send the exit event message to the UI.
		ms_debugClient->CallAfter(&CDebuggerClient::CDebuggerClientAdapter::OnSessionEnd, m_socketClient);
	}

	if (m_socketClient != nullptr && m_socketClient->IsConnected()) m_socketClient->Close();
}

void CDebuggerClient::CDebuggerClientConnection::EntryClient()
{
	if (m_socketClient != nullptr) m_socketClient->Close();

	wxIPV4address addr;
	addr.Hostname(m_hostName);
	addr.Service(m_port);

	// set the appropriate flags for the socket
	m_socketClient = new wxSocketClient(wxSOCKET_BLOCK | wxSOCKET_WAITALL);

	while (!TestDestroy()) {

		bool connected = m_socketClient->Connect(addr, false);
		if (!connected && m_socketClient->Wait())
			connected = m_socketClient->IsConnected();

		if (!TestDestroy() && connected) {

			///////////////////////////////////////////////////////////////////////
			CMemoryWriter commandChannel;
			commandChannel.w_u16(CommandId_VerifyConnection);
			SendCommand(commandChannel.pointer(), commandChannel.size());
			///////////////////////////////////////////////////////////////////////

			unsigned int length = 0;

			while (CDebuggerClientConnection::IsConnected()) {

				if (m_verifiedConnection) break;

				if (m_socketClient && m_socketClient->WaitForRead(waitVerifyDebuggerTimeout)) {
					m_socketClient->ReadMsg(&length, sizeof(unsigned int));
					if (m_socketClient && m_socketClient->WaitForRead()) {
						wxMemoryBuffer bufferData(length);
						m_socketClient->ReadMsg(bufferData.GetData(), length);
						if (length > 0) {
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

				if (TestDestroy()) break;
			}

			if (m_verifiedConnection && m_connectionType == ConnectionType::ConnectionType_Debugger) {
				CMemoryWriter commandChannel;
				commandChannel.w_u16(CommandId_StartSession);
				SendCommand(commandChannel.pointer(), commandChannel.size());
			}

			if (m_verifiedConnection) {

				if (m_connectionType == ConnectionType::ConnectionType_Debugger) {
					// Send the start event message to the UI.
					ms_debugClient->CallAfter(&CDebuggerClient::CDebuggerClientAdapter::OnSessionStart, m_socketClient);
				}

				while (CDebuggerClientConnection::IsConnected()) {

					if (m_socketClient && m_socketClient->WaitForRead(0, waitDebuggerTimeout)) {
						m_socketClient->ReadMsg(&length, sizeof(unsigned int));
						if (m_socketClient && m_socketClient->WaitForRead(0, waitDebuggerTimeout)) {
							wxMemoryBuffer bufferData(length);
							m_socketClient->ReadMsg(bufferData.GetData(), length);
							if (m_connectionType == ConnectionType::ConnectionType_Debugger && length > 0) {
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

					if (TestDestroy()) break;
				}

				if (ms_debugClient != nullptr && m_connectionType == ConnectionType::ConnectionType_Debugger) {
					// Send the exit event message to the UI.
					ms_debugClient->CallAfter(&CDebuggerClient::CDebuggerClientAdapter::OnSessionEnd, m_socketClient);
				}
			}

			wxMilliSleep(waitDebuggerTimeout);
		}

		if (m_connectionType != ConnectionType::ConnectionType_Unknown)
			m_connectionType = ConnectionType::ConnectionType_Scanner;

		m_verifiedConnection = false;
	}

	m_socketClient->Close();
}

void CDebuggerClient::CDebuggerClientConnection::RecvCommand(void* pointer, unsigned int length)
{
	CMemoryReader commandReader(pointer, length);
	wxASSERT(ms_debugClient != nullptr);
	u16 commandFromServer = commandReader.r_u16();

	if (commandFromServer == CommandId_VerifyConnection) {

		commandReader.r_stringZ(m_confGuid);
		commandReader.r_stringZ(m_md5Hash);
		commandReader.r_stringZ(m_userName);
		commandReader.r_stringZ(m_compName);

		m_verifiedConnection = commonMetaData->GetConfigGuid() == m_confGuid;

		if (m_verifiedConnection && m_connectionType == ConnectionType::ConnectionType_Waiter)
			m_connectionType = ConnectionType::ConnectionType_Debugger;
		else if (m_verifiedConnection && m_connectionType == ConnectionType::ConnectionType_Scanner)
			m_connectionType = ConnectionType::ConnectionType_Scanner;
		else
			m_connectionType = ConnectionType::ConnectionType_Unknown;

		CMemoryWriter commandChannel;
		commandChannel.w_u16(CommandId_SetConnectionType);
		commandChannel.w_u32(m_connectionType);

		SendCommand(commandChannel.pointer(), commandChannel.size());
	}
	else if (commandFromServer == CommandId_SetConnectionType) {
		m_connectionType = static_cast<ConnectionType>(commandReader.r_u16());
	}
	else if (commandFromServer == CommandId_GetArrayBreakpoint) {
		//send expression 
		for (auto& expression : ms_debugClient->m_listExpression) {
			CMemoryWriter commandChannel;
			commandChannel.w_u16(CommandId_AddExpression);
			commandChannel.w_stringZ(expression.second);
#if _USE_64_BIT_POINT_IN_DEBUGGER == 1
			commandChannel.w_u64(expression.first);
#else 
			commandChannel.w_u32(expression.first);
#endif 
			SendCommand(commandChannel.pointer(), commandChannel.size());
		}

		CMemoryWriter commandChannel;
		commandChannel.w_u16(CommandId_SetArrayBreakpoint);
		commandChannel.w_u32(ms_debugClient->m_listBreakpoint.size());

		//send breakpoints 
		for (auto breakpoint : ms_debugClient->m_listBreakpoint) {

			commandChannel.w_u32(breakpoint.second.size());
			commandChannel.w_stringZ(breakpoint.first);

			for (auto line : breakpoint.second) {
				commandChannel.w_u32(line.first);
			}
		}

		SendCommand(commandChannel.pointer(), commandChannel.size());
	}
	else if (commandFromServer == CommandId_EnterLoop) {

		ms_debugClient->m_enterLoop = true;
		ms_debugClient->m_activeSocket = this;

		wxString strFileName; commandReader.r_stringZ(strFileName);
		wxString strModuleName; commandReader.r_stringZ(strModuleName);

		CDebugLineData data;
		data.m_fileName = strFileName;
		data.m_moduleName = strModuleName;
		data.m_line = ms_debugClient->GetLineOffset(strModuleName, commandReader.r_s32());

		ms_debugClient->CallAfter(
			&CDebuggerClient::CDebuggerClientAdapter::OnEnterLoop, m_socketClient, data
		);
	}
	else if (commandFromServer == CommandId_LeaveLoop) {
		ms_debugClient->m_enterLoop = false;
		ms_debugClient->m_activeSocket = nullptr;

		const wxString& strFileName = commandReader.r_stringZ();
		const wxString& strModuleName = commandReader.r_stringZ();

		CDebugLineData data;

		data.m_fileName = strFileName;
		data.m_moduleName = strModuleName;
		data.m_line = ms_debugClient->GetLineOffset(strModuleName, commandReader.r_s32());

		ms_debugClient->CallAfter(
			&CDebuggerClient::CDebuggerClientAdapter::OnLeaveLoop, m_socketClient, data
		);
	}
	else if (commandFromServer == CommandId_EvalToolTip) {

		wxString strFileName, strModuleName, strExpression, resultStr;
		commandReader.r_stringZ(strFileName);
		commandReader.r_stringZ(strModuleName);
		commandReader.r_stringZ(strExpression);
		commandReader.r_stringZ(resultStr);

		if (ms_debugClient->IsEnterLoop()) {
			CDebugExpressionData data;
			data.m_fileName = strFileName;
			data.m_moduleName = strModuleName;
			data.m_expression = strExpression;
			ms_debugClient->CallAfter(
				&CDebuggerClient::CDebuggerClientAdapter::OnSetToolTip, data, resultStr
			);
		}
	}
	else if (commandFromServer == CommandId_EvalAutocomplete) {

		wxString strFileName, strModuleName, strExpression, strKeyWord;

		commandReader.r_stringZ(strFileName);
		commandReader.r_stringZ(strModuleName);
		commandReader.r_stringZ(strExpression);
		commandReader.r_stringZ(strKeyWord);

		const int currPos = commandReader.r_s32();

		CDebugAutoCompleteData debugAutocompleteData;

		debugAutocompleteData.m_fileName = strFileName;
		debugAutocompleteData.m_moduleName = strModuleName;
		debugAutocompleteData.m_expression = strExpression;
		debugAutocompleteData.m_keyword = strKeyWord;
		debugAutocompleteData.m_currentPos = currPos;

		unsigned int nCountA = commandReader.r_u32();
		for (unsigned int i = 0; i < nCountA; i++) {
			const wxString& strAttributeName = commandReader.r_stringZ();
			debugAutocompleteData.m_arrVar.push_back({ strAttributeName });
		}

		unsigned int nCountM = commandReader.r_u32();
		for (unsigned int i = 0; i < nCountM; i++) {
			const wxString& strMethodName = commandReader.r_stringZ();
			const wxString& strMethodDescription = commandReader.r_stringZ();
			const bool methodRet = commandReader.r_u8();
			debugAutocompleteData.m_arrMeth.push_back({
				strMethodName,
				strMethodDescription,
				methodRet
				}
			);
		}
		ms_debugClient->CallAfter(
			&CDebuggerClient::CDebuggerClientAdapter::OnAutoComplete, debugAutocompleteData
		);
	}
	else if (commandFromServer == CommandId_SetExpressions) {
		unsigned int countExpression = commandReader.r_u32(); CWatchWindowData watchData;
		for (unsigned int i = 0; i < countExpression; i++) {
#if _USE_64_BIT_POINT_IN_DEBUGGER == 1
			const wxTreeItemId& item = reinterpret_cast<void*>(commandReader.r_u64());
#else 
			const wxTreeItemId& item = reinterpret_cast<void*>(commandReader.r_u32());
#endif 
			wxString strExpression, strValue, strType;

			//expressions
			commandReader.r_stringZ(strExpression);
			commandReader.r_stringZ(strValue);
			commandReader.r_stringZ(strType);

			//set space 
			strValue.Replace('\n', ' ');

			//refresh child elements 
			unsigned int attributeCount = commandReader.r_u32();
			watchData.AddWatch(strExpression, strValue, strType, attributeCount > 0, item);
		}
		ms_debugClient->CallAfter(
			&CDebuggerClient::CDebuggerClientAdapter::OnSetVariable, watchData
		);
	}
	else if (commandFromServer == CommandId_ExpandExpression) {
#if _USE_64_BIT_POINT_IN_DEBUGGER == 1
		CWatchWindowData watchData = reinterpret_cast<void*>(commandReader.r_u64());
#else 
		CWatchWindowData watchData = reinterpret_cast<void*>(commandReader.r_u32());
#endif 
		//generate event 
		unsigned int attributeCount = commandReader.r_u32();
		for (unsigned int i = 0; i < attributeCount; i++) {
			wxString strName, strValue, strType;
			commandReader.r_stringZ(strName);
			commandReader.r_stringZ(strValue);
			commandReader.r_stringZ(strType);
			unsigned int attributeChildCount = commandReader.r_u32();
			watchData.AddWatch(strName, strValue, strType, attributeChildCount > 0);
		}
		ms_debugClient->CallAfter(
			&CDebuggerClient::CDebuggerClientAdapter::OnSetExpanded, watchData
		);
	}
	else if (commandFromServer == CommandId_SetStack) {
		unsigned int count = commandReader.r_u32(); CStackData stackData;
		for (unsigned int i = 0; i < count; i++) {
			const wxString& strModuleName = commandReader.r_stringZ();
			const wxString& strFullName = commandReader.r_stringZ();
			stackData.AppendStack(
				strFullName,
				ms_debugClient->GetLineOffset(strModuleName, commandReader.r_u32())
			);
		}
		ms_debugClient->CallAfter(
			&CDebuggerClient::CDebuggerClientAdapter::OnSetStack, stackData
		);
	}
	else if (commandFromServer == CommandId_SetLocalVariables) {

		CLocalWindowData locData;

		//generate event 	
		unsigned int attributeCount = commandReader.r_u32();
		for (unsigned int i = 0; i < attributeCount; i++) {
			bool tempVar = commandReader.r_u8();
			wxString strName, strValue, strType;
			commandReader.r_stringZ(strName);
			commandReader.r_stringZ(strValue);
			commandReader.r_stringZ(strType);
			unsigned int attributeChildCount = commandReader.r_u32();
			if (!tempVar)
				locData.AddLocalVar(strName, strValue, strType, attributeChildCount > 0);
		}

		ms_debugClient->CallAfter(
			&CDebuggerClient::CDebuggerClientAdapter::OnSetLocalVariable, locData
		);
	}
	else if (commandFromServer == CommandId_MessageFromServer) {

		wxString strFileName,
			strModuleName, strErrorMessage; unsigned int currLine;

		commandReader.r_stringZ(strFileName);
		commandReader.r_stringZ(strModuleName);
		currLine = commandReader.r_u32();
		commandReader.r_stringZ(strErrorMessage);

		CDebugLineData debugData;

		debugData.m_fileName = strFileName;
		debugData.m_moduleName = strModuleName;
		debugData.m_line = ms_debugClient->GetLineOffset(strModuleName, currLine);

		ms_debugClient->CallAfter(
			&CDebuggerClient::CDebuggerClientAdapter::OnMessageFromServer, debugData, strErrorMessage
		);
	}

	ms_debugClient->RecvCommand(pointer, length);
}

void CDebuggerClient::CDebuggerClientConnection::SendCommand(void* pointer, unsigned int length)
{
#if _USE_NET_COMPRESSOR == 1
	BYTE* dest = nullptr; unsigned int dest_sz = 0;
	_compressLZ(&dest, &dest_sz, pointer, length);
	if (m_socketClient && m_socketClient->IsOk()) {
		m_socketClient->WriteMsg(&dest_sz, sizeof(unsigned int));
		m_socketClient->WriteMsg(dest, dest_sz);
	}
	free(dest);
#else
	if (m_socketClient && CDebuggerClientConnection::IsConnected()) {
		m_socketClient->WriteMsg(&length, sizeof(unsigned int));
		m_socketClient->WriteMsg(pointer, length);
	}
#endif
}