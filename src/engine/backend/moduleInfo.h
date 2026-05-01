#ifndef __MODULE_INFO_H__
#define __MODULE_INFO_H__

#include "backend/compiler/compileModule.h"
#include "backend/compiler/procUnit.h"

#include <memory>

// Marker interface for descriptors that terminate a runtime tree.
// Today: ibValueModuleManagerConfiguration. Used by ibRuntimeModuleDataObject::GetRoot
// parent walk to find "the root above this nested descriptor".
class BACKEND_API ibRuntimeRoot {
public:
	virtual ~ibRuntimeRoot() = default;
};

class BACKEND_API ibRuntimeModuleDataObject {
public:

	// Method call. Resolves the runtime through GetProcUnit(); local
	// shared_ptr keeps the ProcUnit alive for the whole call — without
	// it, a concurrent session teardown would drop the last ref and
	// leave pu dangling mid-Execute (fast-F5 UAF crash,
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
	//
	// Two forms:
	//   - Execute(binder): caller supplies a fully-populated ibByteBinder
	//     (descriptor's runtime values for ThisObject / Reference / etc.
	//     filled via SetVar). Preferred — descriptors fill bindings at
	//     execution time, not at compile-time staging.
	//   - Execute(delta): legacy fallback — builds binder from compile-
	//     side maps (m_listExternValue / m_listContextValue) populated
	//     by AddContextVariable at module init. Kept for paths that
	//     haven't migrated to per-execute binding yet.
	void Execute(ibByteBinder& br);
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
	// Session-aware: when a ibSessionScope is active, first checks the
	// session's ProcUnit map. Falls back to the descriptor-owned
	// m_procUnit (legacy path, still in use on desktop until sessions
	// fully own runtimes). See project_unified_session_architecture.md.
	//
	// Returns shared_ptr — callers MUST keep it alive across any call
	// on the ProcUnit. A raw pointer cached across CallAsProc is a UAF
	// if the session is concurrently removed (fast-F5 crash).
	virtual std::shared_ptr<ibProcUnit> GetProcUnit() const;

	// Parent descriptor in the runtime tree — scope chain for symbol
	// resolution and call-stack frame chain. Raw pointer because
	// parent-outlives-child is an ownership invariant (session owns
	// root through ibValuePtr, root owns nested through
	// m_listCommonModuleManager / form tabs / script locals). Eval is
	// the only case where parent is a live runtime-executing node
	// picked up via Current() at compile time.
	//
	// Side-effect: cascades to the descriptor's compile+runtime layer.
	// If m_compileModule and parent->GetCompileModule() both exist, the
	// compile module's parent is updated (scope chain for symbol
	// resolution). Same for m_procUnit (call stack frame chain). This
	// lets call sites set parent once at the descriptor level instead
	// of separately wiring both compile and runtime parents.
	void SetParent(ibRuntimeModuleDataObject* parent);
	ibRuntimeModuleDataObject* GetParent() const { return m_parent; }

	// Walk the parent chain until we hit a node that acts as runtime
	// root (implements ibRuntimeRoot). Configuration is today's sole
	// root; external DPs / reports parent into it. Returns nullptr
	// for orphan descriptors — no root reachable upward.
	virtual ibRuntimeRoot* GetRoot() const;

protected:
	// Populate a value's method-helper from this descriptor's bytecode —
	// every kind=Export entry in m_listFunc / m_listVar is appended as
	// a method / prop alias eProcUnit (the canonical alias all
	// descriptor subclasses use for their script-side surface). Replaces
	// the duplicated pattern that lived in 10 PrepareNames callsites
	// (catalog/document/constant/.../form/moduleManager*).
	//
	// No-op when there's no live ProcUnit or no bytecode (Designer mode,
	// not-yet-compiled descriptor). Helper is read through GetProcUnit()
	// to pin the shared_ptr alive for the duration — same pattern as
	// ExecuteProc / ExecuteFunc above.
	void ExportNamesToHelper(ibValue::ibValueMethodHelper* helper, long alias) const {
		if (helper == nullptr) return;
		const auto pu = GetProcUnit();
		if (!pu) return;
		const ibByteCode* bc = pu->GetByteCode();
		if (bc == nullptr) return;
		for (const auto& fn : bc->m_listFunc) {
			if (!fn.IsExport()) continue;
			helper->AppendMethod(fn.m_strRealName,
				bc->GetNParams(fn),
				bc->HasRetVal(fn),
				(long)fn,
				alias);
		}
		for (const auto& v : bc->m_listVar) {
			if (!v.IsExport()) continue;
			helper->AppendProp(v.m_strRealName, v, alias);
		}
	}

	ibCompileModule* m_compileModule;
	// Descriptor owns its runtime slot. shared_ptr so a long-running
	// CallAsProc can pin the ProcUnit alive even if concurrent teardown
	// drops the descriptor (project_refresh_execute_crash.md).
	std::shared_ptr<ibProcUnit> m_procUnit;

	// Per-descriptor binding session — wraps bytecode's manifest +
	// slot table. Created lazily after compile (manifest is finalized
	// then). Subclasses populate via SetVar() at any point before
	// Execute — typical pattern: BindContextVariable() forwards to
	// m_binder->SetVar() so descriptor-instance values (ThisObject,
	// Reference) are wired without compile-time staging.
	std::unique_ptr<ibByteBinder> m_binder;

	// Parent descriptor — set by creation paths (AddCommonModule,
	// CreateNewForm, AddObject) so scope chain walks up correctly.
	// Raw ptr; safe as long as parent outlives child (enforced by
	// owning containers).
	ibRuntimeModuleDataObject* m_parent = nullptr;
};

#endif
