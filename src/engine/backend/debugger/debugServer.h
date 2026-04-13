#ifndef _DEBUGGER_SERVER_H__
#define _DEBUGGER_SERVER_H__

#include <map>
#include <queue>
#include <wx/thread.h>
#include <wx/socket.h>

struct ibRunContext;

#define debugServer           (ibDebuggerServer::Get())
#define debugServerInit(f)    (ibDebuggerServer::Initialize(f))
#define debugServerDestroy()  (ibDebuggerServer::Destroy())

#include "backend/backend_core.h"
#include "debugDefs.h"

class BACKEND_API ibDebuggerServer {

	class BACKEND_API ibDebuggerServerConnection : public wxThread {
	public:

		bool IsConnected() const {
			if (m_socket == nullptr)
				return false;
			if (!m_socket->IsConnected())
				return false;
			if (!m_socket->IsOk())
				return false;
			wxSocketError error =
				m_socket->LastError();
			return error == wxSOCKET_NOERROR ||
				error == wxSOCKET_WOULDBLOCK;
		}

		void WaitConnection();
		void Disconnect();

		ConnectionType GetConnectionType() const { return m_connectionType; }

		ibDebuggerServerConnection(const wxString& hostName, unsigned short startPort);
		virtual ~ibDebuggerServerConnection();

		// entry point for the thread - called by Run() and executes in the context
		// of this thread.
		virtual ExitCode Entry() override;

		// This one is called by Kill() before killing the thread and is executed
		// in the context of the thread that called Kill().
		virtual void OnKill() override;

	protected:

		void EntryClient();

		void RecvCommand(void* pointer, unsigned int length);
		void SendCommand(void* pointer, unsigned int length);

	private:

		bool		m_waitConnection;

		ConnectionType m_connectionType;

		wxString m_strHostName;
		unsigned short m_numHostPort;

		bool m_acceptConnection;

		wxSocketServer* m_socketServer;
		wxSocketBase* m_socket;

		friend class ibDebuggerServer;
	};

	ibDebuggerServer();

public:

	virtual ~ibDebuggerServer();
	static ibDebuggerServer* Get() { return ms_debugServer; }

	// Force the static appData instance to Init()
	static void Initialize(const int flags);
	static void Destroy();

	bool EnableDebugging() const { return m_socketConnectionThread != nullptr; }

	bool AllowDebugging() const {
		return m_socketConnectionThread != nullptr ?
			!m_socketConnectionThread->m_waitConnection : true;
	}

	bool CreateServer(const wxString& hostName = defaultHost, unsigned short startPort = defaultDebuggerPort, bool wait = false);
	void ShutdownServer();

	void EnterDebugger(ibRunContext* runContext, const struct ibByteUnit& byteCode, long& numPrevLine);
	void SendErrorToClient(const wxString& strFileName, const wxString& strDocPath, unsigned int numLine, const wxString& strErrorMessage);

	bool IsDebugLooped() const { return m_bDebugLoop; }

protected:

	void SetConnectSocket(ibDebuggerServerConnection* socketConnectionThread) {
		m_socketConnectionThread = socketConnectionThread;
	}

	void ResetDebugger() {

		m_runContext = nullptr;
		m_bUseDebug = m_bDebugLoop = false;

		ClearCollectionBreakpoint();
	}

	void ClearCollectionBreakpoint();

	//main loop
	inline void DoDebugLoop(const wxString& strDocPath, const wxString& strModuleName, int numLine, ibRunContext* runContext);

	//special functions:
	inline void SendExpressions();
	inline void SendLocalVariables();
	inline void SendStack();

	//commands:
	void RecvCommand(void* pointer, unsigned int length);
	void SendCommand(void* pointer, unsigned int length);

private:

	static ibDebuggerServer* ms_debugServer;

	bool		m_bUseDebug;
	bool		m_bDoLoop;
	bool		m_bDebugLoop;
	bool		m_bDebugStopLine;

	unsigned int m_numCurrentNumberStopContext;

	std::map<wxString, std::vector<unsigned int>> m_listBreakpoint; //list of points 

#if _USE_64_BIT_POINT_IN_DEBUGGER == 1
	std::map <unsigned long long, wxString> m_listExpression;
#else 
	std::map <unsigned int, wxString> m_listExpression;
#endif 

	ibDebuggerServerConnection* m_socketConnectionThread;
	ibRunContext* m_runContext;

	wxCriticalSection m_clearBreakpointsCS;

	friend class ibBackendException;
};

#endif // __DEBUGGER_CLIENT_H__
