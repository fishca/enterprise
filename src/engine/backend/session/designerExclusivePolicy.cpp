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
		return true;   // no registry to scan through — be permissive

	// Scan the cluster snapshot for other designer rows. Liveness model
	// is heartbeat-based now (sys_session.lastActive updated each refresh
	// tick by the owner; sweep deletes rows whose lastActive is older
	// than kStaleCutoffSec). The historical TryProbeRowLock probe was
	// dropped from the registry's own bookkeeping and produces false
	// positives here — no row lock is ever held, so the probe always
	// succeeds and policy treats every live designer as a zombie.
	//
	// New rule: any other designer row visible in the snapshot vetoes
	// the new designer. If the owner is dead, sweep DELETEs its stale
	// row within ~kStaleCutoffSec (≈30s); user can retry after that.
	const auto snap       = m_registry->GetClusterSnapshot();
	const wxString& ownId = session.GetId();
	for (unsigned int i = 0; i < snap.GetSessionCount(); ++i) {
		if (snap.GetSessionApplication(i) != eDESIGNER_MODE)
			continue;

		const wxString otherGuid = snap.GetSession(i);
		if (otherGuid == ownId)
			continue;

		// Another designer row visible — veto.
		reason = wxString::Format(
			_("Another designer process is already running:\n%s, %s, %s"),
			snap.GetStartedDate(i),
			snap.GetComputerName(i),
			snap.GetUserName(i));
		return false;
	}

	return true;
}
