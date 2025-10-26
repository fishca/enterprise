#include "backend_mainFrame.h"

IBackendDocMDIFrame* IBackendDocMDIFrame::ms_mainFrame = nullptr;

///////////////////////////////////////////////////////////////////////////

IBackendDocMDIFrame::IBackendDocMDIFrame()
{
	wxASSERT(ms_mainFrame == nullptr);

	if (ms_mainFrame == nullptr) {
		ms_mainFrame = this;
	}
}

IBackendDocMDIFrame* IBackendDocMDIFrame::GetDocMDIFrame()
{
	return ms_mainFrame;
}

///////////////////////////////////////////////////////////////////////////

IBackendDocMDIFrame::~IBackendDocMDIFrame() {

	wxASSERT(ms_mainFrame != nullptr);

	if (ms_mainFrame != nullptr) {
		ms_mainFrame = nullptr;
	}
}