#ifndef __MODULE_INFO_H__
#define __MODULE_INFO_H__

#include "backend/compiler/compileModule.h"
#include "backend/compiler/procUnit.h"

#include <memory>

class ibSession;

// Pure interface implemented by any descriptor that terminates a
// runtime tree. Lets nested descriptors reach "whoever owns this
// runtime" via GetRoot() walk without hard-coding the concrete class.
// Today: ibValueModuleManagerConfiguration. Tomorrow: standalone
// external DP-as-root for mini-sessions, Designer-style compile-only
// roots — each implements GetSession() on its own terms.
class BACKEND_API ibRuntimeRoot {
public:
	virtual ~ibRuntimeRoot() = default;
	virtual ibSession* GetSession() const = 0;
};

class BACKEND_API ibRuntimeModuleDataObject {
public:

	// Method call. Resolve the runtime through GetProcUnit() (session-
	// aware delegate) rather than the raw m_procUnit field — main-module
	// ProcUnits now live only in ibSession::m_procUnitMap (populated
	// by InitRuntimeForSession), so going straight to the field would miss
	// them and skip the call silently.
	//
	// The local shared_ptr keeps the ProcUnit alive for the whole call —
	// without it, a concurrent session teardown would drop the map
	// entry and leave pu dangling mid-Execute (fast-F5 UAF crash,
	// project_refresh_execute_crash.md).
	bool ExecuteProc(const wxString& strMethodName,
		ibValue** paParams, const long lSizeArray)
	{
		if (auto pu = GetProcUnit())
			return pu->CallAsProc(strMethodName, paParams, lSizeArray);
		return false;
	}

	bool ExecuteFunc(const wxString& strMethodName,
		ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray)
	{
		if (auto pu = GetProcUnit())
			return pu->CallAsFunc(strMethodName, pvarRetValue, paParams, lSizeArray);
		return false;
	}

	// Allocate the runtime slot (ProcUnit) for this descriptor on
	// demand. No-op in Designer mode (no script execution ever) and
	// when the slot is already populated. Callers that previously
	// inlined the make_shared + DesignerMode check use this single
	// entry instead.
	void InitializeRuntime();

	// Drop the runtime slot — symmetric teardown for InitializeRuntime.
	// Used by ExitRuntimeForSession to release this descriptor's
	// ProcUnit at session end. No-op if already empty.
	void ResetRuntime() { m_procUnit.reset(); }

	// Bind (or rebind) a named context variable on this descriptor's
	// compile module. The value is an ibValue that exposes its method
	// table + attributes at compile time — both the "full object"
	// case (ThisObject — record with data + methods, thisForm —
	// controls + events) and the "methods-only" case (Manager — root
	// or common-module singleton providing shared helpers). Lazy-
	// creates m_compileModule from the descriptor's meta-object when
	// it isn't wired yet — subclass code reads like a recipe:
	// BindContextVariable → Compile → Run.
	void BindContextVariable(const wxString& name, class ibValue* value);

	// Compile this descriptor's module. Skips in Designer mode (the
	// designer only wants AST-level compile state for intellisense and
	// never emits runtime-reachable bytecode). Returns false when no
	// compile module is wired. Backend compile errors propagate as
	// ibBackendException — callers with swallow semantics wrap in
	// their own try/catch.
	bool Compile();

	// Run the compiled top-level of this descriptor on its ProcUnit —
	// initialization entry run at object/form creation, fires handlers
	// registered at module scope. Skips in Designer mode (no execution
	// ever) and when either side of the pair is absent. delta matches
	// ibProcUnit::Execute's semantics — forms / constants pass true
	// (preserve caller's stack state); record objects default to false.
	void Run(bool delta = false);

	// Low-level execute — unconditional (no Designer guard). Prefer
	// Run() in subclass code; Execute() remains for compatibility and
	// for paths that re-Execute on already-compiled state.
	void Execute(bool delta = false);

	ibRuntimeModuleDataObject();
	ibRuntimeModuleDataObject(ibCompileModule* compileCode);
	virtual ~ibRuntimeModuleDataObject();

	// Module-object currently wired in m_compileModule. Thin post-
	// compile accessor — only meaningful AFTER compile is created.
	const ibValueMetaObjectModuleBase* GetMetaObject() const {
		return GetCompileModule() ? GetCompileModule()->GetModuleObject() : nullptr;
	}

	// Compile-target — the single module this descriptor compiles
	// scripts from. Used by BindContextVariable for lazy
	// ibCompileModule creation BEFORE compile is wired.
	//
	// Different concept from the record-data hierarchy's GetMetaObject()
	// which returns a CONTAINER (ibValueMetaObjectGenericData) that
	// holds manager module + object module + potential further modules.
	// Split responsibility:
	//   - GetMetaObject() on record-data    → the container.
	//   - GetMetaForCompile()               → the single module inside
	//     the container that drives THIS descriptor's bytecode
	//     (usually container->GetModuleObject() for catalog/document
	//     objects; form / common module are modules themselves and
	//     return the metadata directly).
	// Default reads through m_compileModule — works for eager-compile
	// subclasses (Unit, Configuration, External DP) whose compile is
	// wired in their ctor; lazy-compile subclasses override.
	virtual const ibValueMetaObjectModuleBase* GetMetaForCompile() const {
		return m_compileModule ? m_compileModule->GetModuleObject() : nullptr;
	}

	virtual ibCompileModule* GetCompileModule() const { return m_compileModule; }
	// Session-aware: when a SessionScope is active, first checks the
	// session's ProcUnit map. Falls back to the descriptor-owned
	// m_procUnit (legacy path, still in use on desktop until sessions
	// fully own runtimes). See project_unified_session_architecture.md.
	//
	// Returns shared_ptr — callers MUST keep it alive across any call
	// on the ProcUnit. A raw pointer cached across CallAsProc is a UAF
	// if the session is concurrently removed (fast-F5 crash).
	virtual std::shared_ptr<ibProcUnit> GetProcUnit() const;

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

	// Parent descriptor in the runtime tree — scope chain + session
	// lookup. Raw pointer because parent-outlives-child is an ownership
	// invariant (session owns root through ibValuePtr, root owns nested
	// through m_listCommonModuleManager / form tabs / script locals).
	// Root descriptor (ibValueModuleManagerConfiguration) leaves this
	// nullptr and overrides GetSession() to return its m_session field.
	// Eval is the only case where parent is a live runtime-executing
	// node picked up via Current() at compile time.
	//
	// Side-effect: cascades to the descriptor's compile+runtime layer.
	// If m_compileModule and parent->GetCompileModule() both exist, the
	// compile module's parent is updated (scope chain for symbol
	// resolution). Same for m_procUnit (call stack frame chain). This
	// lets call sites set parent once at the descriptor level instead
	// of separately wiring both compile and runtime parents.
	void SetParent(ibRuntimeModuleDataObject* parent);
	ibRuntimeModuleDataObject* GetParent() const { return m_parent; }

	// Walk the parent chain until a node returns its session. Root
	// descriptor overrides this with its m_session. Default: recurse.
	// Returns nullptr for detached descriptors (legacy singleton
	// mm, descriptors created outside any session).
	virtual ibSession* GetSession() const;

	// Walk the parent chain until we hit a node that acts as runtime
	// root (implements ibRuntimeRoot). Configuration is today's sole
	// root; external DPs / reports parent into it. Returns nullptr
	// for orphan descriptors — no root reachable upward.
	virtual ibRuntimeRoot* GetRoot() const;

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

	// Parent descriptor — set by creation paths (AddCommonModule,
	// CreateNewForm, AddObject) so GetSession() can walk up. Raw ptr;
	// safe as long as parent outlives child (enforced by owning
	// containers).
	ibRuntimeModuleDataObject* m_parent = nullptr;
};

#endif
