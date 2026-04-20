#include "sessionTicket.h"
#include "sessionRegistry.h"

ibSessionTicket::ibSessionTicket(std::shared_ptr<ibSession> session, ibSessionRegistry* registry)
	: m_session(std::move(session))
	, m_registry(registry)
{
}

ibSessionTicket::~ibSessionTicket()
{
	DoRelease();
}

ibSessionTicket::ibSessionTicket(ibSessionTicket&& other) noexcept
	: m_session(std::move(other.m_session))
	, m_registry(other.m_registry)
{
	other.m_registry = nullptr;
}

ibSessionTicket& ibSessionTicket::operator=(ibSessionTicket&& other) noexcept
{
	if (this != &other) {
		DoRelease();
		m_session        = std::move(other.m_session);
		m_registry       = other.m_registry;
		other.m_registry = nullptr;
	}
	return *this;
}

ibAttachResult ibSessionTicket::Attach(const wxString& user, const wxString& password,
                                       std::chrono::milliseconds timeout)
{
	if (!IsValid())                          return ibAttachResult::RegistryDown;
	if (m_registry->IsFatal())               return ibAttachResult::RegistryDown;
	if (State() != ibSessionState::Added)    return ibAttachResult::RegistryDown;

	// Reset the auth axis to Anonymous so WaitForAuth below has a
	// known starting point — otherwise a prior AuthFailed would trigger
	// the wait immediately without the new Attach having run.
	m_session->TransitionAuth(ibAuthState::Anonymous);

	ibRegistryRequest req;
	req.kind     = ibRegistryRequestKind::Attach;
	req.session  = m_session;
	req.user     = user;
	req.password = password;
	m_registry->Submit(std::move(req), ibPriority::Normal);

	ibAuthState result = m_session->WaitForAuth(ibAuthState::Anonymous, timeout);
	switch (result) {
		case ibAuthState::Authenticated: return ibAttachResult::Ok;
		case ibAuthState::AuthFailed:    return ibAttachResult::WrongPassword;
		case ibAuthState::Anonymous:     return ibAttachResult::Timeout;
	}
	return ibAttachResult::Timeout;
}

void ibSessionTicket::Detach(std::chrono::milliseconds timeout)
{
	if (!IsValid() || m_registry->IsFatal()) return;
	if (State() != ibSessionState::Added)    return;

	// Prime the axis so WaitForAuth below sees the actual handler result.
	if (Auth() == ibAuthState::Authenticated)
		m_session->TransitionAuth(ibAuthState::Authenticated);

	ibRegistryRequest req;
	req.kind    = ibRegistryRequestKind::Detach;
	req.session = m_session;
	m_registry->Submit(std::move(req), ibPriority::Normal);

	// Wait for Auth to move out of whatever it was — handler ends at
	// Anonymous, so any observed change means we're done. Timeout here is
	// a soft best-effort; if registry is swamped the ticket stays usable.
	m_session->WaitForAuth(Auth(), timeout);
}

void ibSessionTicket::SetActivity(const wxString& activity)
{
	if (!IsValid() || m_registry->IsFatal()) return;

	ibRegistryRequest req;
	req.kind     = ibRegistryRequestKind::SetActivity;
	req.session  = m_session;
	req.activity = activity;
	m_registry->Submit(std::move(req), ibPriority::Low);
}

void ibSessionTicket::Release()
{
	DoRelease();
}

void ibSessionTicket::DoRelease()
{
	if (!IsValid()) return;
	// Urgent so the DELETE of our row overtakes pending Normal-priority
	// Adds from other producers — frees lock-pool slots faster and keeps
	// sys_session tidy.
	ibRegistryRequest rm;
	rm.kind    = ibRegistryRequestKind::Remove;
	rm.session = m_session;
	if (!m_registry->IsFatal())
		m_registry->Submit(std::move(rm), ibPriority::Urgent);
	m_session.reset();
	m_registry = nullptr;
}
