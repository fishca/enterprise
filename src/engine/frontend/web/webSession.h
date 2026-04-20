#ifndef __WEB_SESSION_H__
#define __WEB_SESSION_H__

// Per-session runtime context for the web frontend.
//
// Owns everything that must be isolated between concurrent web users:
// the module manager's main-module instance, the ibProcUnit bytecode
// interpreter, the logical MDI frame holding "open" documents, and any
// session-scoped variables. The metadata itself (compiled modules,
// form descriptors, catalog schemas, ...) lives on the singleton
// activeMetaData and is shared across sessions.
//
// Lifecycle mirrors the desktop BeforeRun / RunDatabase / Close flow:
//
//   ctor → OnInit(user, password)  [authorises + CreateMainModule]
//        │
//        │   ... HandleRequest(...) on every HTTP hit ...
//        │
//        └─ OnExit()                [DestroyMainModule + cleanup]
//     dtor

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

class ibWebApplication;
class ibSessionTicket;

class ibWebSession {
public:
	ibWebSession(std::string id, std::string user);
	~ibWebSession();

	// Lifecycle:
	//   OnInit()  — lightweight: reads the configuration name, registers
	//               the session in the manager. Runs with no user
	//               identity yet and does NOT start a runtime.
	//   Login()   — called after the user authenticates (POST /login or
	//               auto-login when sys_user is empty). Sets the user
	//               identity, then spins up the ibWebApplication which
	//               builds the MDI frame and calls CreateMainModule.
	//   OnExit()  — tears the runtime down.
	bool OnInit();
	bool Login(const std::string& user, const std::string& password);
	void OnExit();

	bool IsAuthenticated() const { return m_app != nullptr; }

	const std::string& Id()         const { return m_id; }
	const std::string& User()       const { return m_user; }
	const std::string& ConfigName() const { return m_configName; }
	ibWebApplication*  App()        const { return m_app.get(); }
	// Raw ibSession pointer from the ticket — for HTTP handlers that need
	// to pin SessionScope on their worker thread so ibSession::Current()
	// resolves for moduleManager->GetProcUnit() etc. Nullptr before Login.
	class ibSession* Session() const;

	// Milliseconds since epoch at last request — used later for idle
	// timeout eviction in the session manager.
	std::int64_t LastActiveMs() const;
	void         Touch();

private:
	std::string m_id;
	std::string m_user;
	std::string m_configName;

	mutable std::mutex m_mutex;
	std::int64_t       m_lastActiveMs;
	bool               m_initialized = false;

	// Per-session "app" — owns the module manager, future frame, etc.
	// Created lazily in OnInit so a failed authentication does not
	// leave a half-initialised app behind.
	std::unique_ptr<ibWebApplication> m_app;

	// Session registry ticket — RAII owner of the underlying ibSession.
	// Populated by Login() via ibSessionRegistry::Connect; dtor submits
	// Remove@Urgent so the sys_session row is DELETEd + row lock
	// released on the registry thread ahead of any pending Normal Adds.
	std::unique_ptr<ibSessionTicket> m_ticket;
};

#endif
