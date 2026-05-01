////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko 
//	Description : module information for ibValue 
////////////////////////////////////////////////////////////////////////////

#include "moduleInfo.h"

#include "appData.h"                 // DesignerMode() guard in Compile()
#include "backend/metaCollection/metaModuleObject.h"  // ibValueMetaObjectModuleBase full type for GetGuid/GetClassType

ibRuntimeModuleDataObject::ibRuntimeModuleDataObject() :
	m_compileModule(nullptr)
{
}

ibRuntimeModuleDataObject::ibRuntimeModuleDataObject(ibCompileModule* compileCode) :
	m_compileModule(compileCode)
{
}

ibRuntimeModuleDataObject::~ibRuntimeModuleDataObject()
{
	wxDELETE(m_compileModule);
	// m_procUnit auto-destructs via shared_ptr when the last reference
	// drops.
}

std::shared_ptr<ibProcUnit> ibRuntimeModuleDataObject::GetProcUnit() const
{
	return m_procUnit;
}

ibRuntimeRoot* ibRuntimeModuleDataObject::GetRoot() const
{
	// Default: walk up. Root descriptor overrides and returns `this`
	// cast to the interface.
	return m_parent ? m_parent->GetRoot() : nullptr;
}

void ibRuntimeModuleDataObject::InitializeRuntime()
{
	// ProcUnit is the runtime slot — allocated lazily per descriptor.
	// Designer never executes so no slot is needed; duplicate calls
	// (already-set slot) short-circuit to keep the same instance.
	if (m_procUnit != nullptr || appData->DesignerMode())
		return;
	m_procUnit = std::make_shared<ibProcUnit>();
	// Propagate parent's ProcUnit as scope chain if parent is already
	// wired — allows SetParent to be called BEFORE this, subsequent
	// InitializeRuntime picks up the parent automatically.
	if (m_parent != nullptr) {
		if (auto parentPu = m_parent->GetProcUnit())
			m_procUnit->SetParent(parentPu.get());
	}
}

void ibRuntimeModuleDataObject::BindContextVariable(const wxString& name, ibValue* value)
{
	// Lazy-create m_compileModule on first BindContextVariable —
	// subclass provides its meta-object via GetMetaObject() override.
	// ibCompileModule ctor takes non-const meta; const_cast is safe —
	// compile only reads the meta tree, never mutates it.
	if (m_compileModule == nullptr) {
		if (const ibValueMetaObjectModuleBase* meta = GetMetaForCompile()) {
			m_compileModule = new ibCompileModule(
				const_cast<ibValueMetaObjectModuleBase*>(meta));
			// Propagate parent's compile scope chain — SetParent can
			// be called before BindContextVariable; we pick up the
			// parent compile on creation.
			if (m_parent != nullptr) {
				if (ibCompileModule* parentCompile = m_parent->GetCompileModule())
					m_compileModule->SetParent(parentCompile);
			}
		}
	}
	if (m_compileModule != nullptr)
		m_compileModule->AddContextVariable(name, value);
	// If the runtime binder is already built (post-Compile), forward
	// the value into its slot table too — keeps compile-time staging
	// and runtime binder in sync without subclass plumbing.
	if (m_binder != nullptr)
		m_binder->SetVar(name, value);
}

void ibRuntimeModuleDataObject::Run(bool delta)
{
	// Designer never executes script — the editor only cares about
	// AST / symbol table. Runtime / codeRunner / daemon all go through
	// here.
	if (appData->DesignerMode())
		return;
	Execute(delta);
}

bool ibRuntimeModuleDataObject::Compile()
{
	if (m_compileModule == nullptr)
		return false;
	// Designer never runs bytecode — keep compile state untouched so
	// the intellisense walker sees a consistent AST without emit
	// side-effects that only make sense at runtime.
	if (appData->DesignerMode())
		return true;
	if (m_compileModule->Compile()) {
		// Stamp the freshly-compiled bytecode with the descriptor
		// identity. m_id is the descriptor's GUID — stable across
		// renames in Designer; cache rows in sys_bytecode_cache key
		// off it. m_descriptorClsid records the kind (CommonModule /
		// FormModule / etc.) for sanity checks at AOT-load time and
		// for diag tooling. Both are best-effort here: if
		// GetMetaForCompile is null the bytecode keeps zeroed
		// identity (treated as "any" by validators). See
		// runtime-facade.md and bytecode-deployment plan.
		if (const ibValueMetaObjectModuleBase* meta = GetMetaForCompile()) {
			m_compileModule->m_cByteCode.m_id = meta->GetGuid();
			m_compileModule->m_cByteCode.m_descriptorClsid = meta->GetClassType();
		}
		// Per-compile version fingerprint. Random GUID for now —
		// a future iteration will derive this deterministically
		// from source_hash + metadata_version + compiler_version
		// + kind_bindings_hash so cache hits are recognisable.
		// Random is enough for the current goal: any successful
		// recompile bumps m_version, so dependents that snapshotted
		// the previous value detect drift on next load.
		m_compileModule->m_cByteCode.m_version = ibGuid::newGuid();

		// m_listVar is final now — instantiate the descriptor's
		// binding session against it (filtered to External/Context
		// entries inside the binder) and seed from the compile-side
		// staging (AddContextVariable / AddVariable values added
		// before Compile). After this point, BindContextVariable also
		// forwards to m_binder so any further wiring stays in sync.
		m_binder = std::make_unique<ibByteBinder>(
			m_compileModule->m_cByteCode.m_listVar);
		for (auto& kv : m_compileModule->m_listExternValue)  m_binder->SetVar(kv.first, kv.second);
		for (auto& kv : m_compileModule->m_listContextValue) m_binder->SetVar(kv.first, kv.second);
	}
	return true;
}

void ibRuntimeModuleDataObject::Execute(ibByteBinder& br)
{
	if (m_procUnit != nullptr && m_compileModule != nullptr)
		m_procUnit->Execute(m_compileModule->m_cByteCode, br);
}

void ibRuntimeModuleDataObject::Execute(bool delta)
{
	// Use the descriptor's own binder — populated post-Compile from
	// compile-side staging and kept in sync by BindContextVariable.
	// Subclasses can also reach m_binder->SetVar() directly for
	// per-execute live values.
	if (m_procUnit != nullptr && m_compileModule != nullptr && m_binder != nullptr) {
		m_binder->SetDelta(delta);
		Execute(*m_binder);
	}
}

void ibRuntimeModuleDataObject::SetParent(ibRuntimeModuleDataObject* parent)
{
	m_parent = parent;

	// Cascade to compile+runtime layers so callers don't have to wire
	// them separately. Propagate only when both sides have the
	// corresponding object — parent may be a bare descriptor without a
	// compiled module (e.g. root in Designer mode) or without a
	// ProcUnit (system session that never runs scripts).
	if (m_compileModule != nullptr && parent != nullptr) {
		if (ibCompileModule* parentCompile = parent->GetCompileModule())
			m_compileModule->SetParent(parentCompile);
	}
	if (m_procUnit != nullptr && parent != nullptr) {
		if (auto parentPu = parent->GetProcUnit())
			m_procUnit->SetParent(parentPu.get());
	}
}
