#include "session.h"

#include "backend/moduleManager/moduleManager.h"

#include <utility>
#include <chrono>

namespace {
thread_local ibSession* tl_currentSession = nullptr;
} // namespace

ibSession::ibSession(std::string id, ibSessionKind kind)
	: m_id(std::move(id))
	, m_kind(kind)
{
}

ibSession::~ibSession() = default;

ibValueModuleManagerConfiguration* ibSession::GetModuleManager() const
{
	return m_root;   // ibValuePtr's implicit operator T*()
}

ibValueModuleManagerConfiguration& ibSession::CreateRoot(
	ibMetaData* metaData,
	ibValueMetaObjectConfiguration* commonMeta)
{
	m_root = ibValuePtr<ibValueModuleManagerConfiguration>(
		new ibValueModuleManagerConfiguration(this, metaData, commonMeta));
	return *m_root;
}

void ibSession::ClearRoot()
{
	m_root = nullptr;
}

ibSession* ibSession::Current()
{
	return tl_currentSession;
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

// --- SessionScope -----------------------------------------------------

SessionScope::SessionScope(ibSession* s)
	: m_prev(tl_currentSession)
{
	tl_currentSession = s;
}

SessionScope::~SessionScope()
{
	tl_currentSession = m_prev;
}
