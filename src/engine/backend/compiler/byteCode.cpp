#include "byteCode.h"

////////////////////////////////////////////////////////////////////////
// ibByteBinder — per-execution binding session                       //
////////////////////////////////////////////////////////////////////////

// m_slots is sized to (max m_slotIndex among External/Context entries) + 1
// so SetVar / pre-flight indexing by slot is in-range. Local / ContextProp
// entries are skipped — they're not "must-bind" and don't consume a slot
// in the binder (Locals come from frame init, ContextProps go through
// OPER_GET_A on parent).
static size_t computeBinderSlotCount(const std::vector<ibByteCode::ibByteCodeVarInfo>& vars)
{
	long maxSlot = -1;
	for (const auto& v : vars) {
		if (!v.IsBindRequired()) continue;
		if (v.m_slotIndex > maxSlot) maxSlot = v.m_slotIndex;
	}
	return static_cast<size_t>(maxSlot + 1);
}

ibByteBinder::ibByteBinder(const std::vector<ibByteCode::ibByteCodeVarInfo>& vars, bool delta /*= true*/)
	: m_vars(vars),
	  m_delta(delta),
	  m_slots(computeBinderSlotCount(vars), nullptr)
{
}

void ibByteBinder::SetVar(const wxString& name, ibValue* value)
{
	for (const auto& v : m_vars) {
		if (!v.IsBindRequired()) continue;
		if (!stringUtils::CompareString(v.m_strRealName, name)) continue;
		m_slots[v.m_slotIndex] = value;
		return;
	}
	// Name not declared as External/Context — silently ignored. Caller
	// may pass extras; the runtime only reads what bytecode declared.
}

////////////////////////////////////////////////////////////////////////
// ibByteCode ibByteCode ibByteCode ibByteCode						      //
////////////////////////////////////////////////////////////////////////

// All Find* functions read the unified m_listFunc. Visibility filter
// per kind:
//   - "Export*" variants: skip kind=Local (privates not visible).
//   - Plain (non-Export) variants: see all kinds.
// Function vs procedure split: m_bCodeRet (true = function with return,
// false = procedure).

long ibByteCode::FindMethod(const wxString& strMethodName) const
{
	auto iterator = std::find_if(m_listFunc.begin(), m_listFunc.end(),
		[&](const auto& fn) { return stringUtils::CompareString(strMethodName, fn.m_strRealName); });
	if (iterator != m_listFunc.end())
		return (long)*iterator;
	return wxNOT_FOUND;
}

long ibByteCode::FindExportMethod(const wxString& strMethodName) const
{
	auto iterator = std::find_if(m_listFunc.begin(), m_listFunc.end(),
		[&](const auto& fn) {
			if (fn.IsLocal()) return false;
			return stringUtils::CompareString(strMethodName, fn.m_strRealName);
		});
	if (iterator != m_listFunc.end())
		return (long)*iterator;
	return wxNOT_FOUND;
}

long ibByteCode::FindFunction(const wxString& funcName) const
{
	auto iterator = std::find_if(m_listFunc.begin(), m_listFunc.end(),
		[&](const auto& fn) { return fn.m_bCodeRet && stringUtils::CompareString(funcName, fn.m_strRealName); });
	if (iterator != m_listFunc.end())
		return (long)*iterator;
	return wxNOT_FOUND;
}

long ibByteCode::FindExportFunction(const wxString& funcName) const
{
	auto iterator = std::find_if(m_listFunc.begin(), m_listFunc.end(),
		[&](const auto& fn) {
			if (fn.IsLocal()) return false;
			return fn.m_bCodeRet && stringUtils::CompareString(funcName, fn.m_strRealName);
		});
	if (iterator != m_listFunc.end())
		return (long)*iterator;
	return wxNOT_FOUND;
}

long ibByteCode::FindProcedure(const wxString& procName) const
{
	auto iterator = std::find_if(m_listFunc.begin(), m_listFunc.end(),
		[&](const auto& fn) { return !fn.m_bCodeRet && stringUtils::CompareString(procName, fn.m_strRealName); });
	if (iterator != m_listFunc.end())
		return (long)*iterator;
	return wxNOT_FOUND;
}

long ibByteCode::FindExportProcedure(const wxString& procName) const
{
	auto iterator = std::find_if(m_listFunc.begin(), m_listFunc.end(),
		[&](const auto& fn) {
			if (fn.IsLocal()) return false;
			return !fn.m_bCodeRet && stringUtils::CompareString(procName, fn.m_strRealName);
		});
	if (iterator != m_listFunc.end())
		return (long)*iterator;
	return wxNOT_FOUND;
}

// Bytecode-driven cross-module symbol resolution. Recursive walk:
// self (depth 0) → m_dependencies (depth+1) → m_parent chain
// (depth+1 per step). First match wins. depth = "frames up to root
// descriptor at runtime" — feeds m_numArray on the caller's
// emitted opcode.
//
// Visibility budget — fullVisDepth: at depth ≤ budget, all entries
// visible (Local + Export + ContextMethod). At depth > budget, only
// non-Local (Export + ContextMethod). Default budget 0 = only own
// bytecode sees privates.

namespace {

ibByteCode::ResolvedFunc ResolveFunctionAt(const ibByteCode* bc, const wxString& name, int depth, int fullVisDepth)
{
	if (bc == nullptr) return {};
	auto it = std::find_if(bc->m_listFunc.begin(), bc->m_listFunc.end(),
		[&](const auto& fn) {
			if (depth > fullVisDepth && fn.IsLocal()) return false;
			return stringUtils::CompareString(name, fn.m_strRealName);
		});
	if (it != bc->m_listFunc.end())
		return { &(*it), depth };
	for (const ibByteCode* dep : bc->m_dependencies) {
		if (auto r = ResolveFunctionAt(dep, name, depth + 1, fullVisDepth)) return r;
	}
	if (bc->m_parent != nullptr)
		return ResolveFunctionAt(bc->m_parent, name, depth + 1, fullVisDepth);
	return {};
}

} // anonymous

ibByteCode::ResolvedFunc ibByteCode::ResolveFunction(const wxString& funcName, int fullVisDepth) const
{
	return ResolveFunctionAt(this, funcName, 0, fullVisDepth);
}