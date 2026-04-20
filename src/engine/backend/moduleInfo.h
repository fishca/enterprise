#ifndef __MODULE_INFO_H__
#define __MODULE_INFO_H__

#include "backend/compiler/compileModule.h"
#include "backend/compiler/procUnit.h"

#include <memory>

class BACKEND_API ibModuleDataObject {
public:

	// Method call. Resolve the runtime through GetProcUnit() (session-
	// aware delegate) rather than the raw m_procUnit field — main-module
	// ProcUnits now live only in ibSession::m_procUnitMap (populated
	// by InitRuntimeForSession), so going straight to the field would miss
	// them and skip the call silently.
	bool ExecuteProc(const wxString& strMethodName,
		ibValue** paParams, const long lSizeArray)
	{
		if (ibProcUnit* pu = GetProcUnit())
			return pu->CallAsProc(strMethodName, paParams, lSizeArray);
		return false;
	}

	bool ExecuteFunc(const wxString& strMethodName,
		ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray)
	{
		if (ibProcUnit* pu = GetProcUnit())
			return pu->CallAsFunc(strMethodName, pvarRetValue, paParams, lSizeArray);
		return false;
	}

	ibModuleDataObject();
	ibModuleDataObject(ibCompileModule* compileCode);
	virtual ~ibModuleDataObject();

	const ibValueMetaObjectModuleBase* GetMetaObject() const {
		return GetCompileModule() ? GetCompileModule()->GetModuleObject() : nullptr;
	}

	virtual ibCompileModule* GetCompileModule() const { return m_compileModule; }
	// Session-aware: when a SessionScope is active, first checks the
	// session's ProcUnit map. Falls back to the descriptor-owned
	// m_procUnit (legacy path, still in use on desktop until sessions
	// fully own runtimes). See project_unified_session_architecture.md.
	virtual ibProcUnit* GetProcUnit() const;

	// After m_procUnit is set (by whatever module manager created it),
	// register the shared_ptr under the current session so that
	// subsequent GetProcUnit() calls resolve via the session instead of
	// the legacy descriptor field. No-op when no SessionScope is active
	// or m_procUnit is null.
	void AttachToCurrentSession() const;

	// Inverse of AttachToCurrentSession — remove this descriptor's
	// entry from the current session's ProcUnit map. Called from the
	// dtor so the session's map doesn't accumulate dangling descriptor
	// keys when short-lived instance-level runtimes (Catalog object,
	// Document object, form, constant) tear down. No-op when no
	// SessionScope is active.
	void DetachFromCurrentSession() const;

protected:
	ibCompileModule* m_compileModule;
	// shared_ptr lets sessions co-own the ProcUnit via AttachProcUnit —
	// during the migration both the descriptor and each user session's
	// map hold the same ref; descriptor-only ownership was the legacy
	// state. Typical lifetime is process-scope on desktop / shared
	// across sessions until we fully move runtime to session-scope.
	std::shared_ptr<ibProcUnit> m_procUnit;
	// Session id this descriptor attached its ProcUnit to (via
	// AttachToCurrentSession). Empty when never attached or after
	// DetachFromCurrentSession. Stored as id (not raw ctx*) because
	// the session can be Destroy'd before the descriptor — a raw
	// pointer would UAF in the dtor. At detach time we do
	// ibSessionRegistry::Find(m_attachedSessionId): if null, the session
	// already went away and the map entry is gone too.
	mutable std::string m_attachedSessionId;
};

#endif
