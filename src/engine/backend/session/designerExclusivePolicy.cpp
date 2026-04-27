#include "designerExclusivePolicy.h"
#include "sessionRegistry.h"
#include "sessionSnapshot.h"

#include "backend/appData.h"

ibDesignerExclusivePolicy::ibDesignerExclusivePolicy(ibSessionRegistry* registry)
	: m_registry(registry)
{
}

bool ibDesignerExclusivePolicy::CanAdd(const ibSession& session, wxString& reason)
{
	// Only designer Adds are subject to the exclusion; everyone else
	// passes unconditionally.
	if (session.Identity().m_appMode != eDESIGNER_MODE)
		return true;

	if (m_registry == nullptr)
		return true;   // no registry to probe through — be permissive

	// Scan the cluster snapshot for other designer rows. Each candidate
	// gets one row-lock probe to decide alive vs zombie. Skip our own
	// row (might be in the snapshot if a prior sweep raced the
	// INSERT — harmless).
	const auto snap       = m_registry->GetClusterSnapshot();
	const std::string ownId = session.GetId();
	for (unsigned int i = 0; i < snap.GetSessionCount(); ++i) {
		if (snap.GetSessionApplication(i) != eDESIGNER_MODE)
			continue;

		const wxString otherGuid = snap.GetSession(i);
		const std::string otherId = std::string(otherGuid.ToUTF8().data());
		if (otherId == ownId)
			continue;

		if (m_registry->ProbeSessionRowLock(otherGuid)) {
			// Probe took the lock → no one else holds it → zombie row.
			// Sweep will DELETE it on its next pass. Don't block the new
			// designer.
			continue;
		}

		// Probe failed → owner is alive → veto.
		reason = wxString::Format(
			_("Another designer process is already running:\n%s, %s, %s"),
			snap.GetStartedDate(i),
			snap.GetComputerName(i),
			snap.GetUserName(i));
		return false;
	}

	return true;
}
