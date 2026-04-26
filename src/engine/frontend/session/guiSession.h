#ifndef __IB_GUI_SESSION_H__
#define __IB_GUI_SESSION_H__

// ibGUISession — desktop GUI session base. Owns the main frame instance
// for the duration of the session. Concrete derived classes live in the
// exe'шки that carry their specific frame class (ibFrontendDocMDIFrameEnterprise
// in enterprise.exe, ibFrontendDocMDIFrameDesigner in designer.exe) and
// override OnCreateSession() to `new` that concrete frame + call AttachFrame.
//
// ibGUISession itself is concrete-able but useless without an override —
// base OnCreateSession returns true, leaving m_frame null, so GetFrame()
// yields nullptr and Authenticate()'s dialog-prompt fallback no-ops. Any
// exe that wires CreateSession<ibGUISession>() without overriding
// will see an empty main window — that's a bug, not a feature.

#include "frontend/frontend.h"
#include "backend/session/session.h"

class ibFrontendDocMDIFrame;

class FRONTEND_API ibGUISession : public ibSession {
public:
	// Inherit (std::string, ibSessionKind) ctor so CreateSession<T>'s
	// factory lambda (make_shared<T>(id, kind)) works across every
	// derived session class without boilerplate.
	using ibSession::ibSession;

	~ibGUISession() override;

	// Returns the owned frame or nullptr when OnCreateSession never ran
	// (session in error / still anonymous phase). Downcasters interested
	// in ibFrontendDocMDIFrame specifically use GetFrontendFrame().
	ibBackendDocFrame* GetFrame() const override;

	// Typed accessor — returns the concrete frontend frame, not the
	// backend abstraction. Non-const for wx widget mutators (menus,
	// toolbars, Show). Still nullptr when unattached.
	ibFrontendDocMDIFrame* GetFrontendFrame() const { return m_frame; }

	// Derived OnCreateSession calls this right after `new ibFrontendDocMDIFrameXxx`
	// to take ownership + wire the back-link on the frame + register the
	// singleton (ibFrontendDocMDIFrame::InitFrame) so legacy callers of
	// backend_mainFrame / GetFrame() keep resolving the same instance.
	void AttachFrame(ibFrontendDocMDIFrame* frame);

	// Main-thread teardown — called from ibApplicationData::CloseSession.
	// Clears the back-link first so any closure-handlers that fire during
	// ~Frame don't see a dangling session. Base impl covers the common
	// path; derived classes only override when they need to intercept
	// (e.g. stop a debugger session before the frame dies).
	bool OnDestroySession(bool force = false) override;

	// Show the desktop main frame (CreateGUI + Center + Show + Raise).
	// Called from the host app's OnRun after Open() succeeds. Returns
	// false only when the frame exists but Show() is rejected — base
	// frameless sessions just return true (no-op success).
	bool ShowFrame() override;

	// Reverse of AttachFrame: called from ~ibFrontendDocMDIFrame when wx
	// destroys the frame before the session (graceful wxApp exit — top-level
	// frames die before OnExit runs). Without this, OnDestroySession would
	// dereference a freed wxFrame.
	void DetachFrame(ibFrontendDocMDIFrame* frame) { if (m_frame == frame) m_frame = nullptr; }

	// Shared interactive-auth implementation for designer + enterprise:
	// shows the standalone ibDialogAuthentication (via ibPromptAuthenticationDialog),
	// which calls appData->Login on submit (verify + install on this
	// session). Returns true on user confirm, false on cancel. Web-client
	// session uses its own HTTP login-form flow instead.
	bool OnShowAuthenticate(const wxString& user, const wxString& password) override;

protected:
	// Non-owning at rest; OnDestroySession deletes. Dtor clears the back-link
	// but doesn't delete (wx widgets must be destroyed on the main thread,
	// and ~ibGUISession can run on the registry worker thread via the last
	// shared_ptr drop in force-exit paths).
	ibFrontendDocMDIFrame* m_frame = nullptr;
};

#endif
