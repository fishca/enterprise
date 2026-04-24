////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko 
//	Description : module information for ibValue 
////////////////////////////////////////////////////////////////////////////

#include "moduleInfo.h"

#include "appData.h"                 // DesignerMode() guard in Compile()
#include "session/session.h"
#include "session/sessionRegistry.h"

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

std::shared_ptr<ibProcUnit> ibRuntimeModuleDataObject::GetProcUnit() const
{
	// ProcUnit lives directly on the descriptor — each descriptor owns
	// its runtime slot via InitializeRuntime(). Session / thread_local
	// lookup is no longer needed: per-instance descriptors (form, record
	// object, constant) have their own m_procUnit; common modules and
	// root are wired at InitRuntimeForSession into the same field.
	return m_procUnit;
}

void ibRuntimeModuleDataObject::AttachToCurrentSession() const
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

void ibRuntimeModuleDataObject::DetachFromCurrentSession() const
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

ibSession* ibRuntimeModuleDataObject::GetSession() const
{
	// Default: walk up the parent chain. Root descriptors
	// (ibValueModuleManagerConfiguration) override and return their
	// m_session directly. Orphan descriptors (legacy singleton mm, no
	// parent wired) return nullptr.
	return m_parent ? m_parent->GetSession() : nullptr;
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
	m_compileModule->Compile();
	return true;
}

void ibRuntimeModuleDataObject::Execute(bool delta)
{
	if (m_procUnit != nullptr && m_compileModule != nullptr)
		m_procUnit->Execute(m_compileModule->m_cByteCode, delta);
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
