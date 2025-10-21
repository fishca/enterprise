#ifndef __DEBUGGER_CLIENT_H__
#define __DEBUGGER_CLIENT_H__

#include <wx/thread.h>

#define debugClient           (CDebuggerClient::Get())
#define debugClientInit()     (CDebuggerClient::Initialize())
#define debugClientDestroy()  (CDebuggerClient::Destroy())

#include "debugClientBridge.h"

class BACKEND_API CDebuggerClient {

	class BACKEND_API CDebuggerClientAdapter : public wxEvtHandler {
	public:

		CDebuggerClientAdapter() : m_debugBridge(nullptr) {}
		void SetBridge(IDebuggerClientBridge* bridge) {
			if (m_debugBridge != nullptr)
				wxDELETE(m_debugBridge);
			m_debugBridge = bridge;
		}
		virtual ~CDebuggerClientAdapter() { wxDELETE(m_debugBridge); }

		//commands 
		void OnSessionStart(wxSocketClient* sock);
		void OnSessionEnd(wxSocketClient* sock);

		void OnEnterLoop(wxSocketClient* sock, const CDebugLineData& data);
		void OnLeaveLoop(wxSocketClient* sock, const CDebugLineData& data);

		void OnAutoComplete(const CDebugAutoCompleteData& data);
		void OnMessageFromServer(const CDebugLineData& data, const wxString& message);
		void OnSetToolTip(const CDebugExpressionData& data, const wxString& resultStr);
		void OnSetStack(const CStackData& data);

		void OnSetLocalVariable(const CLocalWindowData& data);

		void OnSetVariable(const CWatchWindowData& data);
		void OnSetExpanded(const CWatchWindowData& data);

	private:
		IDebuggerClientBridge* m_debugBridge;
	};

	class BACKEND_API CDebuggerClientConnection : public wxThread {
	public:

		bool IsConnected() const {
			if (m_socketClient == nullptr)
				return false;
			if (!m_socketClient->IsConnected())
				return false;
			if (!m_socketClient->IsOk())
				return false;
			wxSocketError error =
				m_socketClient->LastError();
			return error == wxSOCKET_NOERROR ||
				error == wxSOCKET_WOULDBLOCK;
		}

		wxString GetHostName() const { return m_hostName; }
		unsigned short GetPort() const { return m_port; }
		wxString GetComputerName() const { return m_compName; }
		wxString GetUserName() const { return m_userName; }

		bool AttachConnection();
		bool DetachConnection(bool kill = false);

		CDebuggerClient::CDebuggerClientConnection::CDebuggerClientConnection(CDebuggerClient* client, const wxString& hostName, unsigned short port) :
			wxThread(wxTHREAD_DETACHED),
			m_hostName(hostName),
			m_port(port),
			m_verifiedConnection(false),
			m_connectionType(ConnectionType::ConnectionType_Scanner),
			m_socketClient(nullptr) {
			if (debugClient != nullptr) debugClient->AppendConnection(this);
		}

		CDebuggerClient::CDebuggerClientConnection::~CDebuggerClientConnection() {
			if (debugClient != nullptr) debugClient->DeleteConnection(this);
			if (m_socketClient != nullptr) m_socketClient->Destroy();
		}

		// This one is called by Kill() before killing the thread and is executed
		// in the context of the thread that called Kill().
		virtual void OnKill() override;

		// entry point for the thread - called by Run() and executes in the context
		// of this thread.
		virtual ExitCode Entry();

		virtual ConnectionType GetConnectionType() const { return m_connectionType; }

	protected:

		void EntryClient();

		void RecvCommand(void* pointer, unsigned int length);
		void SendCommand(void* pointer, unsigned int length);

	private:

		bool			m_verifiedConnection;

		wxString		m_hostName;
		unsigned short	m_port;

		wxSocketClient* m_socketClient;

		wxString		m_confGuid;
		wxString		m_md5Hash;
		wxString		m_userName;
		wxString		m_compName;

		ConnectionType	m_connectionType;

		friend class CDebuggerClient;
	};

	CDebuggerClient() : m_activeSocket(nullptr), m_adapter(new CDebuggerClientAdapter), m_enterLoop(false) {}

public:

	void SetBridge(IDebuggerClientBridge* bridge) { m_adapter->SetBridge(bridge); }

	virtual ~CDebuggerClient() {
		while (m_listConnection.size()) {
			m_listConnection[m_listConnection.size() - 1]->Delete();
		}
		wxDELETE(m_adapter);
	}

	static CDebuggerClient* Get() { return ms_debugClient; }

	// Force the static appData instance to Init()
	static bool Initialize();
	static void Destroy();

public:

	void ConnectToServer(const wxString& hostName, unsigned short port);
	CDebuggerClientConnection* FindConnection(const wxString& hostName, unsigned short port);
	CDebuggerClientConnection* FindDebugger(const wxString& hostName, unsigned short port);
	void SearchServer(const wxString& hostName = defaultHost, unsigned short startPort = defaultDebuggerPort);
	std::vector<CDebuggerClientConnection*>& GetListConnection() { return m_listConnection; }

	//special public function:
#if _USE_64_BIT_POINT_IN_DEBUGGER == 1
	void AddExpression(const wxString& strExpression, unsigned long long id);
	void ExpandExpression(const wxString& strExpression, unsigned long long id);
	void RemoveExpression(unsigned long long id);
#else 
	void AddExpression(wxString strExpression, unsigned int id);
	void ExpandExpression(wxString strExpression, unsigned int id);
	void RemoveExpression(unsigned int id);
#endif 

	void SetLevelStack(unsigned int level);

	//evaluate for tooltip
	void EvaluateToolTip(const wxString& strFileName, const wxString& strModuleName, const wxString& strExpression);

	//support calc strExpression in debugloop
	void EvaluateAutocomplete(const wxString& strFileName, const wxString& strModuleName, const wxString& strExpression, const wxString& keyWord, int currline);

	//get debug list
	std::vector<unsigned int> GetDebugList(const wxString& strModuleName);

	//special functions:
	void Continue();
	void StepOver();
	void StepInto();
	void Pause();
	void Stop(bool kill);

	//for breakpoints and offsets 
	void InitializeBreakpoints(const wxString& strModuleName, unsigned int from, unsigned int to);
	void PatchBreakpointCollection(const wxString& strModuleName, unsigned int line, int offsetLine);

	bool SaveBreakpoints(const wxString& strModuleName);
	bool SaveAllBreakpoints();

	bool ToggleBreakpoint(const wxString& strModuleName, unsigned int line);
	bool RemoveBreakpoint(const wxString& strModuleName, unsigned int line);
	void RemoveAllBreakpoint();

	bool HasConnections() const {
		for (auto connection : m_listConnection) {
			if (connection->GetConnectionType() == ConnectionType::ConnectionType_Debugger) return connection->IsConnected();
		}
		return false;
	}

	bool IsEnterLoop() const { return m_enterLoop; }

public:

	template <typename T>
	void CallAfter(void (T::* method)()) {
		if (m_adapter != nullptr) {
			wxQueueEvent(m_adapter, new wxAsyncMethodCallEvent0<T>(static_cast<T*>(m_adapter), method));
		}
	}

	template <typename T, typename T1, typename P1>
	void CallAfter(void (T::* method)(T1 x1), P1 x1) {
		if (m_adapter != nullptr) {
			wxQueueEvent(m_adapter, new wxAsyncMethodCallEvent1<T, T1>(static_cast<T*>(m_adapter), method, x1));
		}
	}

	template <typename T, typename T1, typename T2, typename P1, typename P2>
	void CallAfter(void (T::* method)(T1 x1, T2 x2), P1 x1, P2 x2) {
		if (m_adapter != nullptr) {
			wxQueueEvent(m_adapter, new wxAsyncMethodCallEvent2<T, T1, T2>(static_cast<T*>(m_adapter), method, x1, x2));
		}
	}

	template <typename T>
	void CallAfter(const T& fn) {
		if (m_adapter != nullptr) {
			wxQueueEvent(m_adapter, new wxAsyncMethodCallEventFunctor<T>(m_adapter, fn));
		}
	}

protected:

	static bool TableAlreadyCreated();
	static bool CreateBreakpointDatabase();

	//db support 
	void LoadBreakpointCollection();

	bool ToggleBreakpointInDB(const wxString& strModuleName, unsigned int line);
	bool RemoveBreakpointInDB(const wxString& strModuleName, unsigned int line);
	bool OffsetBreakpointInDB(const wxString& strModuleName, unsigned int line, int offset);
	bool RemoveAllBreakpointInDB();

	//commands:
	bool CreateConnection(const wxString& hostName, unsigned short port) {
		CDebuggerClientConnection* createdConnection = FindConnection(hostName, port);
		if (createdConnection == nullptr) {
			createdConnection = new CDebuggerClientConnection(this, hostName, port);

			createdConnection->SetPriority(wxPRIORITY_MIN);
			return createdConnection->Run() != wxTHREAD_NO_ERROR;
		}
		return true;
	}

	void AppendConnection(CDebuggerClientConnection* client) { m_listConnection.push_back(client); }

	void DeleteConnection(CDebuggerClientConnection* client) {

		if (m_activeSocket == client) m_activeSocket = nullptr;
		m_listConnection.erase(
			std::remove(m_listConnection.begin(), m_listConnection.end(), client), m_listConnection.end()
		);
		if (m_listConnection.size() == 0) m_enterLoop = false;
	}

	void RecvCommand(void* pointer, unsigned int length) {}
	void SendCommand(void* pointer, unsigned int length) {
		if (m_activeSocket != nullptr) {
			m_activeSocket->SendCommand(pointer, length);
		}
		else {
			for (auto connection : m_listConnection) {
				connection->SendCommand(pointer, length);
			}
		}
	}

private:

	static CDebuggerClient* ms_debugClient;

	CDebuggerClientConnection* m_activeSocket = nullptr;
	CDebuggerClientAdapter* m_adapter = nullptr;

	std::vector<CDebuggerClientConnection*>	m_listConnection;

	std::map <wxString, std::map<unsigned int, int>> m_listBreakpoint; //list of points 
	std::map <wxString, std::map<unsigned int, int>> m_listOffsetBreakpoint; //list of changed transitions

#if _USE_64_BIT_POINT_IN_DEBUGGER == 1
	std::map <unsigned long long, wxString> m_listExpression;
#else 
	std::map <unsigned int, wxString> m_listExpression;
#endif  

	bool	m_enterLoop;
};

#endif