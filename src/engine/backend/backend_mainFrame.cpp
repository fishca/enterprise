#include "backend_mainFrame.h"

// Per-thread "current main frame" slot — identical to the old process-
// wide singleton on desktop (one UI thread = one frame), but lets the
// web runtime stand up a separate frame per concurrent HTTP handler
// thread without tripping a singleton assert. The storage is defined
// here rather than as the class's ms_mainFrame static so the ODR is
// obvious and no OES_USE_WEB is needed in the implementation. The
// class still exposes ms_mainFrame as a static member (see header) for
// binary-compat with existing callers, mirrored to the thread-local on
// each ctor/dtor.
static thread_local ibBackendDocMDIFrame* t_mainFrame = nullptr;
ibBackendDocMDIFrame* ibBackendDocMDIFrame::ms_mainFrame = nullptr;

///////////////////////////////////////////////////////////////////////////

ibBackendDocMDIFrame::ibBackendDocMDIFrame()
{
	// Swap in — caller owns the dtor pair. Multi-session web: each
	// session's ibWebApplication::OnInit runs on its own HTTP-handler
	// thread, so t_mainFrame stays scoped to that session.
	t_mainFrame = this;
	ms_mainFrame = this;
}

void ibBackendDocMDIFrame::InstallOnThread()
{
	// Pin this frame as the current thread's main frame. Worker threads
	// spawn long after the frame's ctor ran, so they inherit the ctor
	// thread's t_mainFrame (namely: none). Call this from the worker's
	// entry point to make GetDocMDIFrame() resolve to THIS session's
	// frame instead of the ms_mainFrame global-fallback, which would
	// otherwise point at whichever session constructed a frame last.
	t_mainFrame = this;
}

ibBackendDocMDIFrame* ibBackendDocMDIFrame::GetDocMDIFrame()
{
	// Prefer the thread-local (web path where sessions may coexist);
	// fall back to the process-wide static for desktop callers that
	// never leave the UI thread.
	return t_mainFrame != nullptr ? t_mainFrame : ms_mainFrame;
}

///////////////////////////////////////////////////////////////////////////

ibBackendDocMDIFrame::~ibBackendDocMDIFrame() {
	if (t_mainFrame == this)
		t_mainFrame = nullptr;
	if (ms_mainFrame == this)
		ms_mainFrame = nullptr;
}