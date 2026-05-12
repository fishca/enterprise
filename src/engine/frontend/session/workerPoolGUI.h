#ifndef __IB_WORKER_POOL_GUI_H__
#define __IB_WORKER_POOL_GUI_H__

// GUI worker pool — wraps wxTheApp::CallAfter so tasks Submitted from
// any thread eventually run on the wx main thread (the only thread
// that may touch wx widgets). One logical "worker" = the wx main
// thread; tasks queue on wx's idle event handler in arrival order.
//
// Use case: a script or background thread on a desktop GUI host wants
// to dispatch heavy work without freezing the UI — Submit lets wx
// drain the task between input events. The headless pool's role
// (N threads serving M sessions) doesn't apply on GUI: there's only
// one session and one UI thread.
//
// Not auto-installed on the registry by default. The current code path
// for GUI session script execution runs inline on the calling thread
// (wx main), which is correct for the synchronous scenarios in use
// today. When async dispatch becomes a real need, host startup
// constructs an ibWorkerPoolGUI and registers it on the registry via
// the planned setter — the class is ready in advance.

#include "frontend/frontend.h"
#include "backend/session/workerPool.h"

#include <atomic>

class FRONTEND_API ibWorkerPoolGUI : public ibWorkerPool {
public:
	ibWorkerPoolGUI()  = default;
	~ibWorkerPoolGUI() override = default;

	std::future<void> Submit(ibSession* session, Task task) override;
	void              DropSession(ibSession* session) override;
	void              CancelSession(ibSession* session) override;
	void              Stop() override;

private:
	std::atomic<bool> m_stop { false };
};

#endif
