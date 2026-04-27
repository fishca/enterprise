#ifndef __WEB_CLIENT_SESSION_H__
#define __WEB_CLIENT_SESSION_H__

// ibWebClientSession — concrete per-tab session class for the wes web
// frontend. Owns its main frame (ibWebFrame) typed; backend code reaches
// it through GetFrame() (the abstract ibBackendDocFrame*).
//
// Instantiated only by ibWebSession::Login through
// appData->CreateSession<ibWebClientSession>(presetGuid, address).

#include "backend/session/session.h"

class ibWebFrame;

class ibWebClientSession : public ibSession {
public:
	using ibSession::ibSession;   // (std::string, ibSessionKind) ctor

	// Backend-facing accessor. Out-of-line because the implicit
	// ibWebFrame* → ibBackendDocFrame* upcast requires the complete
	// ibWebFrame type (forward decl above is enough for storage and
	// pointer assignment, not for the upcast).
	ibBackendDocFrame* GetFrame() const override;

	// Typed setter used by ibWebApplication after ibWebFrame is built.
	// Out-of-line for the same reason — the override implies an upcast.
	void SetFrame(ibWebFrame* f);

	// Typed accessor for web-internal callers (no cast).
	ibWebFrame* Frame() const { return m_frame; }

private:
	ibWebFrame* m_frame = nullptr;
};

#endif
