#include "guiSession.h"
#include "frontend/mainFrame/mainFrame.h"
#include "frontend/win/dlgs/authorization.h"

#include "backend/appData.h"
#include "backend/session/sessionRegistry.h"

#include <wx/app.h>
#include <wx/msgdlg.h>
#include <wx/frame.h>

ibGUISession::~ibGUISession()
{
	// Clear the back-link if the frame is still around. We intentionally
	// DO NOT delete the frame here — wx widgets must be torn down on the
	// main thread, and ~ibGUISession can run on the registry worker
	// thread if the last shared_ptr drop happens there (force-exit path).
	// The graceful path runs OnDestroySession first and nulls m_frame
	// before the dtor ever sees it.
	if (m_frame != nullptr) m_frame->SetGUISession(nullptr);
}

ibBackendDocFrame* ibGUISession::GetFrame() const
{
	return m_frame;
}

void ibGUISession::AttachFrame(ibFrontendDocMDIFrame* frame)
{
	m_frame = frame;
	if (m_frame == nullptr) return;

	// Two-way link: frame knows its owning session, session knows its
	// frame. Keeps frame event handlers reaching per-session runtime
	// (module manager, ProcUnit map, ibValueSystemFunction once it goes
	// non-static) without detouring through wxApp singletons.
	m_frame->SetGUISession(this);

	// Bind ibSession* on the frame too, so legacy GetSession() (used by
	// AllowRun/AllowClose and session-aware UI) resolves immediately —
	// Initialize() is otherwise a separate explicit step in mainApp.
	m_frame->Initialize(this);

	// Register the singleton ibFrontendDocMDIFrame::s_instance so legacy
	// `backend_mainFrame` macro / ibFrontendDocMDIFrame::GetFrame() keep
	// resolving to the same window that session now owns.
	ibFrontendDocMDIFrame::InitFrame(m_frame);

	// Reload listener — admin's "reload" signal on this session's
	// sys_session row fires NotifyReload from the registry worker.
	// Hop to the main thread, notify the user, close the frame.
	// Process exits through wxApp's normal shutdown.
	ibSession* sessSelf = this;
	ibSessionRegistry::Instance().OnReload([sessSelf](ibSession* target) {
		if (target != sessSelf) return;
		if (wxTheApp == nullptr) return;
		wxTheApp->CallAfter([]() {
			wxMessageBox(_("Session reloaded by an administrator. The application will close — please re-open it from the launcher."),
				wxTheApp->GetAppDisplayName(), wxOK | wxICON_INFORMATION);
			if (auto* frame = ibFrontendDocMDIFrame::GetFrame())
				frame->Close(true);
		});
	});
}

bool ibGUISession::OnShowAuthenticate(const wxString& user, const wxString& password)
{
	// Desktop dialog lives as a standalone free function — no frame
	// dependency on the call path. Dialog's OK handler runs under the
	// main-thread ibSessionScope bound to this session, so the InstallUser
	// it triggers writes m_userInfo / m_sessionRawPassword on this session
	// directly. Authenticate then re-submits Attach with those validated creds.
	return ibPromptAuthenticationDialog(user, password);
}

bool ibGUISession::OnDestroySession(bool force)
{
	(void)force;  // GUI sessions don't yet veto — frame->AllowClose hook lives on wx frame's Close cycle.
	if (m_frame == nullptr) return true;

	// Clear back-link first so any wx event that fires during Destroy()
	// (close vetoes, pane teardown) sees a detached session rather than
	// a half-torn-down one.
	m_frame->SetGUISession(nullptr);

	// wx-idiomatic shutdown — Destroy() schedules deletion through the
	// event loop, then the frame dtor runs on the main thread. After
	// that s_instance is null-cleared by ~ibFrontendDocMDIFrame.
	m_frame->Destroy();
	m_frame = nullptr;
	return true;
}

bool ibGUISession::ShowFrame()
{
	// Frameless variant of a GUI session shouldn't happen in practice
	// (designer / enterprise both AttachFrame in OnCreateSession), but
	// keep it as a tolerant no-op success rather than UAF.
	if (m_frame == nullptr || ibApplicationData::IsForceExit())
		return true;
	if (m_frame->IsShown())
		return true;
	m_frame->CreateGUI();
	m_frame->SetClientSize(m_frame->FromDIP(wxSize(800, 600)));
	m_frame->SetFocus();
	m_frame->Center();
	if (!m_frame->Show())
		return false;
	m_frame->Raise();
	return true;
}
