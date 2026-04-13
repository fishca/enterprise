#include "backend_mainFrame.h"

ibBackendDocMDIFrame* ibBackendDocMDIFrame::ms_mainFrame = nullptr;

///////////////////////////////////////////////////////////////////////////

ibBackendDocMDIFrame::ibBackendDocMDIFrame()
{
	wxASSERT(ms_mainFrame == nullptr);

	if (ms_mainFrame == nullptr) {
		ms_mainFrame = this;
	}
}

ibBackendDocMDIFrame* ibBackendDocMDIFrame::GetDocMDIFrame()
{
	return ms_mainFrame;
}

///////////////////////////////////////////////////////////////////////////

ibBackendDocMDIFrame::~ibBackendDocMDIFrame() {

	wxASSERT(ms_mainFrame != nullptr);

	if (ms_mainFrame != nullptr) {
		ms_mainFrame = nullptr;
	}
}