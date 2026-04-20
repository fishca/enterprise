////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko 
//	Description : module information for ibValue 
////////////////////////////////////////////////////////////////////////////

#include "moduleInfo.h"

#include "session/session.h"
#include "session/sessionRegistry.h"

ibModuleDataObject::ibModuleDataObject() :
	m_compileModule(nullptr)
{
}

ibModuleDataObject::ibModuleDataObject(ibCompileModule* compileCode) :
	m_compileModule(compileCode)
{
}

ibModuleDataObject::~ibModuleDataObject()
{
	// Best-effort: drop our entry from the current session's ProcUnit
	// map so per-instance descriptors (Catalog objects, Document
	// objects, forms, constants) don't leave dangling keys behind.
	// No-op when no SessionScope is active on this thread — not every
	// descriptor's dtor runs under a session.
	DetachFromCurrentSession();
	wxDELETE(m_compileModule);
	// m_procUnit auto-destructs via shared_ptr when the last reference
	// drops (may be co-owned by sessions via AttachProcUnit).
}

ibProcUnit* ibModuleDataObject::GetProcUnit() const
{
	// Prefer the current session's ProcUnit when one is scoped onto
	// this thread. During the migration the descriptor-owned m_procUnit
	// stays populated by the module manager (desktop path), so we
	// gracefully fall back when the session hasn't attached its own
	// yet — ensures zero behaviour change while the switch-over lands
	// incrementally.
	if (auto* ctx = ibSession::Current()) {
		if (auto* pu = ctx->GetProcUnitFor(this))
			return pu;
	}
	return m_procUnit.get();
}

void ibModuleDataObject::AttachToCurrentSession() const
{
	if (!m_procUnit)
		return;
	if (auto* ctx = ibSession::Current()) {
		ctx->AttachProcUnit(this, m_procUnit);
		// Remember which session we attached to so the dtor can detach
		// even if it runs outside SessionScope (e.g. on a cleanup thread
		// or after the caller dropped its SessionScope). Lookup by id
		// stays safe when the session is already gone.
		m_attachedSessionId = ctx->GetId();
	}
}

void ibModuleDataObject::DetachFromCurrentSession() const
{
	// Prefer the session we actually attached to — handles the "dtor
	// runs on a thread without SessionScope" case.
	if (!m_attachedSessionId.empty()) {
		if (auto* ctx = ibSessionRegistry::Instance().Find(m_attachedSessionId))
			ctx->DetachProcUnit(this);
		// If Find returned null the session was already Destroy'd;
		// its m_procUnitMap went with it, so there's nothing to detach.
		m_attachedSessionId.clear();
		return;
	}
	// Fallback — descriptor never recorded an attach id (e.g. someone
	// attached via raw AttachProcUnit path without going through
	// AttachToCurrentSession). Best-effort detach from Current().
	if (auto* ctx = ibSession::Current())
		ctx->DetachProcUnit(this);
}
