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

ibSession::~ibSession()
{
	// No lock in the dtor — at refcount-0 nobody else holds a pointer to
	// this session, so the map is already single-owner. Locking here
	// risks a deadlock when destruction races with a worker thread that
	// is tearing down its own state through the same ibSession (the
	// worker's pending GetProcUnitFor could be waiting on the same
	// mutex when the dtor fires from the releasing thread).
	m_procUnitMap.clear();
	// m_root drops via member destruction after this body returns —
	// nested common-module / object descriptors held by the root fall
	// with it (shared_ptr cascade).
}

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

// m_procUnitMap is touched from multiple threads: HTTP login path
// (InitRuntimeForSession Attach), HTTP logout path (ExitRuntimeForSession
// Detach) and the session's own worker thread (GetProcUnit → Get on
// every script dispatch). unordered_map is not thread-safe, so every
// accessor locks m_procUnitMtx. Returning shared_ptr — caller needs to
// outlive the CallAsProc even if the session is concurrently removed
// from the registry (rapid F5 crash, 2026-04-21).
std::shared_ptr<ibProcUnit> ibSession::GetProcUnitFor(const ibRuntimeModuleDataObject* descriptor) const
{
	if (descriptor == nullptr) return nullptr;
	std::lock_guard<std::mutex> lk(m_procUnitMtx);
	auto it = m_procUnitMap.find(descriptor);
	return it != m_procUnitMap.end() ? it->second : nullptr;
}

void ibSession::AttachProcUnit(const ibRuntimeModuleDataObject* descriptor, std::shared_ptr<ibProcUnit> pu)
{
	if (descriptor == nullptr) return;
	std::lock_guard<std::mutex> lk(m_procUnitMtx);
	m_procUnitMap[descriptor] = std::move(pu);
}

void ibSession::DetachProcUnit(const ibRuntimeModuleDataObject* descriptor)
{
	if (descriptor == nullptr) return;
	std::lock_guard<std::mutex> lk(m_procUnitMtx);
	m_procUnitMap.erase(descriptor);
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
