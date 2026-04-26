#include "session.h"
#include "sessionRegistry.h"

#include "backend/moduleManager/moduleManager.h"
#include "backend/metadataConfiguration.h"

#include <utility>
#include <chrono>
#include <shared_mutex>
#include <thread>
#include <unordered_map>

namespace {
// Per-thread current-session map. Lookup semantics depend on the
// access mode owned by ibSessionRegistry — see ibSession::Current.
std::shared_mutex s_currentMutex;
std::unordered_map<std::thread::id, ibSession*> s_currentByThread;
} // namespace

ibSession::ibSession(std::string id, ibSessionKind kind)
	: m_id(std::move(id))
	, m_kind(kind)
	, m_workDate(wxDateTime::Now())
{
}

ibSession::~ibSession()
{
	// Ensure the destruction chain for objects created by this session
	// runs even when the session falls off without an explicit ClearRoot
	// (registry-driven Remove, abnormal teardown). Idempotent — ClearRoot
	// is a no-op when m_root is already null.
	ClearRoot();
}

ibValueModuleManagerConfiguration* ibSession::GetModuleManager() const
{
	return m_root;   // ibValuePtr's implicit operator T*()
}

void ibSession::Close(bool force)
{
	// Main-thread teardown hook — wx frame destruction must run on the
	// thread that created the frame. Derived ibGUISession overrides
	// OnDestroySession to schedule frame->Destroy() through wx's event
	// loop. force=false lets the override veto (AllowClose script,
	// unsaved-data prompt); force=true skips the check and tears down.
	if (!OnDestroySession(force) && !force)
		return;

	// Submit Remove@Urgent — registry-thread ProcessRemove erases m_own,
	// which drops the last shared_ptr and destroys this session.
	auto& reg = ibSessionRegistry::Instance();
	if (reg.IsFatal())
		return;
	ibRegistryRequest req;
	req.kind    = ibRegistryRequestKind::Remove;
	req.session = shared_from_this();
	reg.Submit(std::move(req), ibPriority::Urgent);
}

void ibSession::Detach(std::chrono::milliseconds timeout)
{
	auto& reg = ibSessionRegistry::Instance();
	if (reg.IsFatal()) return;
	if (State() != ibSessionState::Added) return;

	// Prime the axis so WaitForAuth below sees the actual handler result.
	if (Auth() == ibAuthState::Authenticated)
		TransitionAuth(ibAuthState::Authenticated);

	ibRegistryRequest req;
	req.kind    = ibRegistryRequestKind::Detach;
	req.session = shared_from_this();
	reg.Submit(std::move(req), ibPriority::Normal);

	WaitForAuth(Auth(), timeout);
}

void ibSession::SetActivity(const wxString& activity)
{
	auto& reg = ibSessionRegistry::Instance();
	if (reg.IsFatal()) return;

	ibRegistryRequest req;
	req.kind     = ibRegistryRequestKind::SetActivity;
	req.session  = shared_from_this();
	req.activity = activity;
	reg.Submit(std::move(req), ibPriority::Low);
}

ibValueModuleManagerConfiguration* ibSession::CreateRoot(ibMetaDataConfigurationBase* metaData)
{
	// Per-session root mm — replaces the legacy ibMetaDataConfigurationFile
	// process-singleton. Each session owns its own copy of the metadata
	// tree's runtime state. CreateRoot only allocates; CreateMainModule
	// (compile) runs from RunDatabase after common-module descriptors are
	// registered via OnBeforeRunMetaObject. Idempotent — second call with
	// the same metadata returns the existing root unchanged.
	if (metaData == nullptr)
		return nullptr;
	if (m_root)
		return m_root;
	auto* commonMeta = metaData->GetCommonMetaObject();
	if (commonMeta == nullptr)
		return nullptr;

	m_root = ibValuePtr<ibValueModuleManagerConfiguration>(
		new ibValueModuleManagerConfiguration(metaData, commonMeta));
	return m_root;
}

bool ibSession::CompileRoot()
{
	return m_root && m_root->CreateMainModule();
}

bool ibSession::DestroyRoot()
{
	return m_root && m_root->DestroyMainModule();
}

void ibSession::ClearRoot()
{
	if (m_root) {
		m_root->DestroyMainModule();
		m_root = nullptr;
	}
}

void ibSession::EnsureRoot()
{
	// Wired by ibSessionRegistry::NotifyAuthenticated to land between
	// OnFirstConnect (metadataCreate) and OnAuthenticated (RunDatabase /
	// CompileRoot). CreateRoot itself is idempotent; this wrapper just
	// guards on activeMetaData so headless sessions (Launcher, technical)
	// without metadata don't fault.
	if (m_root) return;
	if (activeMetaData == nullptr) return;
	CreateRoot(activeMetaData);
}

ibSession* ibSession::Current()
{
	const auto mode = ibSessionRegistry::Instance().GetAccessMode();
	std::shared_lock<std::shared_mutex> lk(s_currentMutex);
	switch (mode) {
	case AccessMode::Single:
		// One session per process. Map holds at most one entry; return
		// the lone value regardless of calling thread. Empty map
		// (pre-bind / post-clear) → nullptr.
		return s_currentByThread.empty()
			? nullptr
			: s_currentByThread.begin()->second;
	case AccessMode::Shared:
		// Per-thread lookup with fallback to the registry's system session.
		if (auto it = s_currentByThread.find(std::this_thread::get_id()); it != s_currentByThread.end())
			return it->second;
		return ibSessionRegistry::Instance().GetFallback();
	}
	return nullptr;
}

void ibSession::SetAccessMode(AccessMode mode)
{
	ibSessionRegistry::Instance().SetAccessMode(mode);
}

ibSession::AccessMode ibSession::GetAccessMode()
{
	return ibSessionRegistry::Instance().GetAccessMode();
}

void ibSession::SetFallback(ibSession* s)
{
	ibSessionRegistry::Instance().SetFallback(s);
}

void ibSession::ClearFallback()
{
	ibSessionRegistry::Instance().ClearFallback();
}

ibSession* ibSession::GetByThread(std::thread::id tid)
{
	const auto mode = ibSessionRegistry::Instance().GetAccessMode();
	std::shared_lock<std::shared_mutex> lk(s_currentMutex);
	switch (mode) {
	case AccessMode::Single:
		return s_currentByThread.empty()
			? nullptr
			: s_currentByThread.begin()->second;
	case AccessMode::Shared:
		if (auto it = s_currentByThread.find(tid); it != s_currentByThread.end())
			return it->second;
		return ibSessionRegistry::Instance().GetFallback();
	}
	return nullptr;
}

std::vector<std::pair<std::thread::id, ibSession*>> ibSession::SnapshotByThread()
{
	std::shared_lock<std::shared_mutex> lk(s_currentMutex);
	std::vector<std::pair<std::thread::id, ibSession*>> out;
	out.reserve(s_currentByThread.size());
	for (const auto& kv : s_currentByThread)
		out.emplace_back(kv.first, kv.second);
	return out;
}

void ibSession::BindSessionToThread(ibSession* s, std::thread::id tid)
{
	std::unique_lock<std::shared_mutex> lk(s_currentMutex);
	if (s != nullptr)
		s_currentByThread[tid] = s;
	else
		s_currentByThread.erase(tid);
}

void ibSession::UnbindThread(std::thread::id tid)
{
	std::unique_lock<std::shared_mutex> lk(s_currentMutex);
	s_currentByThread.erase(tid);
}

void ibSession::UnbindSession(ibSession* s)
{
	if (s == nullptr) return;
	std::unique_lock<std::shared_mutex> lk(s_currentMutex);
	for (auto it = s_currentByThread.begin(); it != s_currentByThread.end();) {
		if (it->second == s)
			it = s_currentByThread.erase(it);
		else
			++it;
	}
}

ibBackendDocFrame* ibSession::CurrentFrame()
{
	ibSession* s = Current();
	return s != nullptr ? s->GetFrame() : nullptr;
}

wxString ibSession::Reason() const
{
	std::lock_guard<std::mutex> lk(m_mtx);
	return m_reason;
}

void ibSession::Transition(ibSessionState next, const wxString& reason)
{
	{
		std::lock_guard<std::mutex> lk(m_mtx);
		if (!reason.IsEmpty()) m_reason = reason;
		m_state.store(next, std::memory_order_release);
	}
	// notify_all — multiple producers may wait on different predicates
	// (WaitForState vs WaitForAuth). Spurious wakes short-circuit via the
	// predicate lambda.
	m_cv.notify_all();
}

void ibSession::TransitionAuth(ibAuthState next, const wxString& reason)
{
	{
		std::lock_guard<std::mutex> lk(m_mtx);
		if (!reason.IsEmpty()) m_reason = reason;
		m_auth.store(next, std::memory_order_release);
	}
	m_cv.notify_all();
}

ibSessionState ibSession::WaitForState(ibSessionState from, std::chrono::milliseconds timeout)
{
	std::unique_lock<std::mutex> lk(m_mtx);
	m_cv.wait_for(lk, timeout, [this, from]{
		return m_state.load(std::memory_order_acquire) != from;
	});
	return m_state.load(std::memory_order_acquire);
}

ibAuthState ibSession::WaitForAuth(ibAuthState from, std::chrono::milliseconds timeout)
{
	std::unique_lock<std::mutex> lk(m_mtx);
	m_cv.wait_for(lk, timeout, [this, from]{
		return m_auth.load(std::memory_order_acquire) != from;
	});
	return m_auth.load(std::memory_order_acquire);
}

bool ibSession::Open(const wxString& user, const wxString& password)
{
	// Must be Added (registry accepted the session); Anonymous or AuthFailed
	// on the auth axis — either is a valid retry point.
	if (State() != ibSessionState::Added) return false;

	auto& reg = ibSessionRegistry::Instance();
	if (reg.IsFatal()) return false;

	constexpr auto timeout = std::chrono::seconds(20);

	auto submitAttach = [&](const wxString& u, const wxString& p) {
		// Reset auth axis so WaitForAuth below has a known "from" value —
		// otherwise a prior AuthFailed state would trigger the wait
		// immediately without the new Attach having been processed.
		TransitionAuth(ibAuthState::Anonymous);

		ibRegistryRequest req;
		req.kind     = ibRegistryRequestKind::Attach;
		req.session  = shared_from_this();
		req.user     = u;
		req.password = p;
		reg.Submit(std::move(req), ibPriority::Normal);

		return WaitForAuth(ibAuthState::Anonymous, timeout);
	};

	ibAuthState res = submitAttach(user, password);
	if (res == ibAuthState::Authenticated) {
		// NotifyAuthenticated fires three phases in order:
		//   1. OnFirstConnect listeners — process-level metadata bootstrap
		//      (metadataCreate, populates activeMetaData) on the first auth.
		//   2. session->EnsureRoot — per-session root mm allocated NOW so
		//      step 3's listeners can rely on GetModuleManager() != null.
		//   3. OnAuthenticated listeners — per-session bring-up
		//      (RunDatabase fires OnBefore/AfterRunMetaObject which read
		//      session->mm; CompileRoot; InitRuntimeForSession).
		reg.NotifyAuthenticated(this);
		return true;
	}

	// Interactive fallback — GUI override shows login dialog (shared for
	// designer + enterprise via ibGUISession::OnShowAuthenticate). The
	// dialog's OK handler populates singleton m_userInfo / m_sessionRawPassword
	// via AuthenticationAndSetUser, which mirrors into this session too
	// (main thread has ibSessionScope bound to us). On false return auth
	// fails and the caller reports the original error.
	if (!OnShowAuthenticate(user, password)) return false;

	res = submitAttach(m_userInfo.m_strUserName, m_sessionRawPassword);
	if (res == ibAuthState::Authenticated)
		reg.NotifyAuthenticated(this);
	return res == ibAuthState::Authenticated;
}

// --- ibSessionThreadBinding --------------------------------------------

ibSessionThreadBinding::ibSessionThreadBinding(ibSession* s) noexcept
	: m_tid(std::this_thread::get_id())
{
	ibSession::BindSessionToThread(s, m_tid);
}

ibSessionThreadBinding::~ibSessionThreadBinding()
{
	ibSession::UnbindThread(m_tid);
}

// --- ibSessionScope -----------------------------------------------------

ibSessionScope::ibSessionScope(ibSession* s)
{
	const std::thread::id tid = std::this_thread::get_id();
	std::unique_lock<std::shared_mutex> lk(s_currentMutex);
	auto it = s_currentByThread.find(tid);
	m_prev = it != s_currentByThread.end() ? it->second : nullptr;
	if (s != nullptr)
		s_currentByThread[tid] = s;
	else
		s_currentByThread.erase(tid);
}

ibSessionScope::~ibSessionScope()
{
	const std::thread::id tid = std::this_thread::get_id();
	std::unique_lock<std::shared_mutex> lk(s_currentMutex);
	if (m_prev != nullptr)
		s_currentByThread[tid] = m_prev;
	else
		s_currentByThread.erase(tid);
}
