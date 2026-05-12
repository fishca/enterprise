#include "backend_mainFrame.h"

// ibBackendDocFrame no longer owns any process-level / thread-local
// state — the frame is a member of the ibSession that created it.
// ms_mainFrame / t_mainFrame / GetDocMDIFrame / InstallOnThread and the
// transitional ibCurrentFrame() helper are all gone. Callers reach
// their frame through the session pointer that's already in scope.
// Default ctor/dtor live in the header.

int ibBackendDocFrame::ShowModalMessage(const wxString& /*message*/,
                                           const wxString& /*caption*/,
                                           int /*style*/)
{
	// Default base body — no UI, so nothing to show. Returns 0 which
	// maps to wxCANCEL/"aborted" in Question-style callers.
	return 0;
}
