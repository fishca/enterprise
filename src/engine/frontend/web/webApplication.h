#ifndef __WEB_APPLICATION_H__
#define __WEB_APPLICATION_H__

// Per-session web "application" — analogue of wxTheApp for one web user.
//
// Owns everything that must be isolated between concurrent sessions:
// the session-scoped module manager (with its own compiled bytecode,
// ibProcUnit, common modules and context variables) and — in future
// steps — the logical MDI frame with open documents.
//
// Lifecycle mirrors wxApp: OnInit() stands the per-session runtime up
// (think BeforeRun + RunDatabase in desktop mode), OnExit() tears it
// down (Close + DestroyMainModule). One ibWebApplication lives inside
// each ibWebSession and is destroyed with it.
//
// Inherits wxEvtHandler so scripts, timers (AttachIdleHandler) and any
// future per-session dispatch can Bind/Unbind/ProcessEvent through the
// app object the same way wxTheApp is used on desktop — wfrontend.dll
// has no real wxApp loop, but ibWebSession::Tick (HTTP-tick from the
// handler thread) will route events into this handler.

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <wx/event.h>

// value_ptr.h only forward-declares ibValue (via backend_core.h); we need
// the complete ibValue base for ibValuePtr<T> instantiation below.
#include "backend/compiler/value.h"
#include "backend/value_ptr.h"

class ibValueForm;
class ibValueModuleManagerConfiguration;
class ibWebFrame;
class ibWebTimer;
class ibSession;

class ibWebApplication : public wxEvtHandler {
public:
	ibWebApplication();
	virtual ~ibWebApplication();

	// Startup. Runs AFTER user authentication succeeded: build the
	// web frame first (scripts fired from CreateMainModule — OnStart
	// handlers, load-page code, early OpenForm calls — already need a
	// frame to attach to), then compile + execute the session's main
	// module.
	virtual bool OnInit();
	virtual void OnExit();

	// Out-of-line: the ibValuePtr<T> → T* conversion static_casts via
	// ibValue*, which requires T complete. Keeping the body in
	// webApplication.cpp (with moduleManager.h already included there)
	// lets TUs that only need the class identity (formObject.cpp,
	// webTimer.cpp) stop at a forward decl.
	ibValueModuleManagerConfiguration* GetModuleManager() const;
	ibWebFrame*                        GetFrame()         const { return m_frame; }

	// Session context for this application. Set by the owning
	// ibWebSession right after construction; worker loop installs it
	// onto its thread via ibSessionScope so any descriptor-level
	// GetProcUnit call delegates through the session. nullptr until
	// the session wires it up.
	ibSession*                  GetSessionContext() const { return m_sessionContext; }
	void                               SetSessionContext(ibSession* ctx) { m_sessionContext = ctx; }

	// Active tab's visual host, or nullptr if no tab is open. The HTTP
	// layer uses this to serialise the current tree into the response
	// body after a Dispatch*.
	class ibVisualHostClient* GetActiveHost() const;

	// Live-update sequence counter. Every state-changing action
	// (Dispatch, timer tick, tab switch/close, CreateAndUpdateVisualHost
	// from script) calls MarkDirty() which bumps m_seq and wakes every
	// waiter on m_seqCv. SSE subscribers call WaitForChange(lastSeen,
	// timeoutMs) — block until seq advances past lastSeen OR timeout,
	// return the current seq. Multiple subscribers on one session (e.g.
	// multiple browser tabs of the same user) each track their own
	// lastSeen, so notify_all wakes them all without any waiter "eating"
	// an update for the others. Sequence starts at 1 so first WaitForChange
	// with lastSeen=0 returns immediately — new SSE clients get current
	// state without waiting for the next event.
	void     MarkDirty();
	uint64_t CurrentSeq() const { return m_seq.load(); }
	uint64_t WaitForChange(uint64_t lastSeen, int timeoutMs);

	// Session dispatcher — single generic entry. HTTP handlers hand off
	// here: given a controlId + kind ("click" | "text" | "toggle" | …)
	// + value, the dispatcher resolves the ibValueFrame in the active
	// tab's form, fetches its paired ibWebWindow from the host's
	// (frame → wxObject) map, and calls HandleRequest(kind, value) on
	// it — polymorphic, no subclass-specific branches here. Each
	// ibWebXxx handles the kinds it understands. Drains on this
	// thread, rebuilds the visual tree. See docs/web/event-dispatcher.md.
	bool Dispatch(int controlId, const wxString& kind, const wxString& value);

	// Legacy kind-specific entry points. Thin shims over Dispatch —
	// kept so existing HTTP routes (/action, /change, /toggle) stay
	// wired without the wfrontend shims knowing the new unified
	// endpoint exists. Callers choose whichever fits their body shape.
	bool DispatchControlAction(int controlId)                      { return Dispatch(controlId, wxT("click"), wxString());                      }
	bool DispatchTextChange(int controlId, const wxString& value)  { return Dispatch(controlId, wxT("text"),  value);                           }
	bool DispatchToggle(int controlId, bool checked)               { return Dispatch(controlId, wxT("toggle"), checked ? wxT("1") : wxT("0")); }

	// Session worker thread interface. All script execution runs on
	// the worker — HTTP handlers and timers submit work, worker
	// drains serially. Why: ibProcUnit is not thread-safe across
	// session state. Serialising through one thread keeps it safe
	// without per-access locking and mirrors how a wxApp event loop
	// would naturally serialise.
	//
	//   PostWork          — fire-and-forget; used by timer ticks.
	//   RunOnWorker<T>(f) — submits and returns a future for the
	//                       result; HTTP handlers .get() it to
	//                       synchronously return the JSON response.
	//
	// If the worker hasn't started yet (OnInit still running), work
	// is queued and executed when StartWorker runs the loop. If
	// the worker is stopping, PostWork discards and RunOnWorker's
	// future is set to default T{} — a dying session just drops
	// pending ticks.
	void PostWork(std::function<void()> fn);

	template <class Fn>
	auto RunOnWorker(Fn&& fn) -> std::future<decltype(fn())> {
		using R = decltype(fn());
		auto task = std::make_shared<std::packaged_task<R()>>(std::forward<Fn>(fn));
		auto fut  = task->get_future();
		PostWork([task]{ (*task)(); });
		return fut;
	}

private:
	void StartWorker();
	void StopWorker();
	void WorkerLoop();

	// Worker thread state. m_workerStop flips once on OnExit; drains
	// happen under m_workerMtx + m_workerCv.
	std::thread                        m_worker;
	std::atomic<bool>                  m_workerStop { false };
	std::mutex                         m_workerMtx;
	std::condition_variable            m_workerCv;
	std::deque<std::function<void()>>  m_workerQueue;
	// m_moduleManager owns its refcount via ibValuePtr — assigning a raw
	// pointer IncrRef's implicitly, assigning nullptr (or destruction)
	// DecrRef's. Keeps OnInit's failure branch and OnExit's teardown
	// leak-free without a matching IncrRef/DecrRef pair to track.
	ibWebFrame*                                   m_frame         = nullptr;
	// m_moduleManager field removed — moduleManager is shared process-
	// wide and lives on metadata; GetModuleManager() pulls from there.
	bool                                          m_initialized   = false;
	// Borrowed — owned by ibSessionRegistry keyed by cookie. Survives this
	// app because Destroy is driven by the session teardown that runs
	// after OnExit joins the worker thread.
	ibSession*                             m_sessionContext = nullptr;

	// Live-update state. See MarkDirty/WaitForChange docs above.
	std::atomic<uint64_t>                         m_seq { 1 };
	std::mutex                                    m_seqMtx;
	std::condition_variable                       m_seqCv;
};

// Reach the current session's ibWebApplication from arbitrary web
// script context (ibValueForm, ibValueControl, …). Goes through the
// thread_local main-frame singleton that the backend already uses;
// returns null if we're not inside a session (e.g. module-loading
// before OnInit). Defined in webApplication.cpp.
ibWebApplication* currentWebApp();

#endif
