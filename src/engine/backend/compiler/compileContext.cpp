#include "compileContext.h"
#include "compileCode.h"

////////////////////////////////////////////////////////////////////////
// ibCompileContext ibCompileContext ibCompileContext ibCompileContext//
////////////////////////////////////////////////////////////////////////

/**
* Create a new variable identifier
*/

ibParamUnit ibCompileContext::CreateVariable(const wxString& strPrefix)
{
	if (m_numReturn == RETURN_BLOCK)
		return m_parentContext->CreateVariable(strPrefix);

	const wxString& strTempName =
		wxString::Format(strPrefix + wxT("%d"), ++m_numTempVar); //@temp_ - to ensure the uniqueness of the name

	ibParamUnit variable =
		GetVariable(strTempName, false, false, false, true); //we look for a temporary variable only in the local context
	variable.m_numArray = DEF_VAR_TEMP; //flag of a temporary local variable
	return variable;
}

/**
* Adds a new variable to the list
* Returns the added variable as a ibParamUnit
*/

ibParamUnit ibCompileContext::AddVariable(const wxString& strVarName,
	const wxString& typeVar, bool exportVar, bool contextVar, bool tempVar)
{
	// Dup-check is OWN-scope only — direct find_if on m_listVariable.
	// Going through FindVariable would also hit bytecode-fallback
	// (parent module entries), which is wrong: declaring `Var X` here
	// is allowed even if a parent module exports an X.
	auto existing = std::find_if(m_listVariable.begin(), m_listVariable.end(),
		[&strVarName](const auto& pair) { return stringUtils::CompareString(strVarName, pair.first); });
	if (existing != m_listVariable.end()) {
		m_compileModule->SetError(ERROR_IDENTIFIER_DUPLICATE, strVarName);
		return ibParamUnit();
	}

	if (m_numReturn == RETURN_BLOCK) {

		ibParamUnit variable = CreateVariable(wxT("@context"));

		PushVariable(strVarName, wxT(""), variable.m_numIndex,
			typeVar, exportVar, true, tempVar);

		return variable;
	}

	unsigned int numVariable = m_listVariable.size();
	PushVariable(strVarName, wxT(""), numVariable,
		typeVar, exportVar, contextVar, tempVar);

	ibParamUnit variable;

	//determine the number and type of the variable
	variable.m_numArray = 0;
	variable.m_strType = typeVar;
	variable.m_numIndex = numVariable;

	return variable;
}

/**
* The function returns the variable number by the string name
* Search for the variable definition, starting from the current context to all parent
* If the required variable does not exist, then a new variable definition is created
*/

ibParamUnit ibCompileContext::GetVariable(const wxString& strVarName, bool bFindInParent, bool bCheckError, bool contextVar, bool tempVar)
{
	int numCanUseLocalInParent = m_numFindLocalInParent;

	std::shared_ptr <ibVariable> currentVariable = nullptr;
	if (!FindVariable(strVarName, currentVariable)) {

		//search in parent contexts (modules)
		if (bFindInParent) {

			int numParent = 0, numContext = 0;
			if (m_numReturn == RETURN_BLOCK)
				numContext++;

			// Bail out on pathological depth — same threshold for every
			// hop (compile-context walk and bc walk).
			auto guardRecursion = [&]() {
				if (numParent > 2 * MAX_OBJECTS_LEVEL)
					ibBackendCoreException::Error(_("Recursive call of modules!"));
			};

			// Visibility check + emit. Depth is the runtime frame index
			// (m_pppArrayList layer); blockReturn switches it to
			// DEF_VAR_TEMP for through-block reach. Returns true (with
			// `out` filled) only if visibility budget allows.
			auto tryEmit = [&](const std::shared_ptr<ibVariable>& cur, int depth, bool blockReturn, ibParamUnit& out) {
				if (!(m_numReturn == RETURN_BLOCK || numCanUseLocalInParent > 0 || cur->m_bExport))
					return false;
				out.m_numArray = blockReturn ? (long long)DEF_VAR_TEMP : (long long)depth;
				out.m_numIndex = cur->m_numVariable;
				out.m_strType = cur->m_strType;
				return true;
			};

			// 1. Compile-context chain (block → function → root of own
			//    ibCompileCode, then up through parent compile-contexts
			//    where they are still live).
			for (ibCompileContext* pCurContext = m_parentContext; pCurContext != nullptr; pCurContext = pCurContext->m_parentContext) {
				numParent++;
				if (pCurContext->m_numReturn == RETURN_BLOCK)
					numContext++;
				guardRecursion();

				if (pCurContext->FindVariable(strVarName, currentVariable)) {
					ibParamUnit variable;
					if (tryEmit(currentVariable, numParent - numContext,
					            pCurContext->m_numReturn == RETURN_BLOCK, variable))
						return variable;
				}
				numCanUseLocalInParent--;
			}

			// 2. Eval host function frame (only when this compile is an
			//    eval expression opened inside a function). Function
			//    frame is its own runtime layer between eval and host's
			//    module body; account for it in numParent so the bc walk
			//    below emits depths shifted by +1.
			//    Source: ibCompileCode::GetEvalHostFunction() — virtual,
			//    returns nullptr for non-eval modules; ibCompileEval
			//    overrides with the host fn captured at ctor time.
			if (auto* hostFn = m_compileModule->GetEvalHostFunction()) {
				const auto& fn = *hostFn;
				auto fIt = std::find_if(fn.m_listLocals.begin(), fn.m_listLocals.end(),
					[&strVarName](const auto& v) { return stringUtils::CompareString(strVarName, v.m_strRealName); });
				if (fIt != fn.m_listLocals.end()) {
					ibParamUnit variable;
					variable.m_numArray = (numParent - numContext) + 1;
					variable.m_numIndex = fIt->m_slotIndex;
					return variable;
				}
				numParent++;  // function frame layer occupied even on miss
			}

			// 3. Bytecode parent chain (cross-bc — AOT-loaded ancestors
			//    with no live compile-context, e.g. host module body
			//    while compiling an eval expression). Emitter decides
			//    direct OPER_GET vs OPER_GET_A by foundedVar->m_strContext.
			//    bc.m_parent is constructed genealogically (form →
			//    object → manager → root for catalogs etc.; eval
			//    inherits its host's chain) — foreign bcs never appear,
			//    so scoped bindings (ThisForm / ThisObject) are
			//    reachable iff they live on a real ancestor.
			for (const ibByteCode* pCurByteCode = m_compileModule->m_cByteCode.m_parent;
			     pCurByteCode != nullptr; pCurByteCode = pCurByteCode->m_parent) {
				numParent++;
				guardRecursion();

				if (pCurByteCode->FindVariable(strVarName, currentVariable)) {
					ibParamUnit variable;
					if (tryEmit(currentVariable, numParent - numContext, /*blockReturn=*/false, variable))
						return variable;
				}
				numCanUseLocalInParent--;
			}
		}

		if (bCheckError) {
			m_compileModule->SetError(ERROR_VAR_NOT_FOUND, strVarName); //display an error message
			return ibParamUnit();
		}

		// there was no variable declaration yet - add
		return AddVariable(strVarName, wxEmptyString, contextVar, contextVar, tempVar);
	}

	wxASSERT(currentVariable);

	ibParamUnit variable;

	// FindVariable here is own-scope only (no bytecode fallback) —
	// reaching this branch means the variable lives in this very
	// compile-context's m_listVariable, so m_numArray=0 is correct.
	// Cross-frame resolution (host function param / parent module
	// binding) goes through GetVariable's bytecode fallback below
	// where found.depth is preserved on ibParamUnit::m_numArray.
	if (m_numReturn == RETURN_BLOCK)
		variable.m_numArray = DEF_VAR_TEMP;
	else
		variable.m_numArray = 0;

	variable.m_numIndex = currentVariable->m_numVariable;
	variable.m_strType = currentVariable->m_strType;

	return variable;
}

void ibCompileContext::PushVariable(const wxString& strVarName, const wxString& strContextVar, unsigned int numVariable,
	const wxString& typeVar, bool exportVar, bool contextVar, bool tempVar)
{
	// Map key is the original-cased name — lookups go through
	// std::find_if + stringUtils::CompareString (case-insensitive),
	// so there's no need to upper-normalize storage. Renderers
	// (debugger / catalog object PrepareNames) read the key directly
	// and now show the user's source casing instead of UPPERCASE.
	std::shared_ptr<ibVariable> currentVariable(new ibVariable(strVarName));

	currentVariable->m_strRealName = strVarName;
	currentVariable->m_bExport = exportVar;
	currentVariable->m_bContext = contextVar;

	currentVariable->m_strContext = strContextVar;  //variable for which the attribute is called

	currentVariable->m_bTempVar = tempVar;
	currentVariable->m_strType = typeVar;
	currentVariable->m_numVariable = numVariable;

	m_listVariable.insert_or_assign(
		currentVariable->m_strName, std::move(currentVariable)
	);
}

void ibCompileContext::PushFunction(const wxString& strFuncName, const wxString& strContextVar, const wxString& strShortDescription, unsigned int numFunction, bool hasRetVal, int argCount)
{
	std::shared_ptr<ibCompileContext::ibFunction> contextFunction(
		new ibFunction(strFuncName, CreateContext(hasRetVal ? RETURN_FUNCTION : RETURN_PROCEDURE))
	);

	contextFunction->m_nStart = numFunction;
	contextFunction->m_bContext = true;
	contextFunction->m_bExport = true;

	contextFunction->m_strContext = strContextVar; //variable for which the attribute is called

	if (argCount > 0) contextFunction->m_listParam.reserve(argCount);

	for (long arg = 0; arg < argCount; arg++) {
		ibFunction::ibParamVariable contextVariable;
		contextVariable.m_puValue.m_numArray = DEF_VAR_DEFAULT;
		contextVariable.m_puValue.m_numIndex = DEF_VAR_DEFAULT;
		contextFunction->m_listParam.emplace_back(std::move(contextVariable));
	}

	contextFunction->m_strRealName = strFuncName;
	contextFunction->m_strShortDescription = strShortDescription;

	m_listFunction.insert_or_assign(
		contextFunction->m_strName, std::move(contextFunction)
	);
}

/**
 * Search for a variable in a hash array
 * Returns true - if the variable is found
 */

bool ibCompileContext::FindVariable(const wxString& strVarName, std::shared_ptr<ibVariable>& foundedVar, bool contextVar)
{
	auto it = std::find_if(m_listVariable.begin(), m_listVariable.end(),
		[strVarName](const auto pair) {return stringUtils::CompareString(strVarName, pair.first); });

	if (contextVar) {

		if (it != m_listVariable.end()) {
			foundedVar = it->second;
			return it->second->m_bContext;
		}

		if (m_parentContext && m_parentContext->FindVariable(strVarName, foundedVar, contextVar))
			return true;

		// Reached the top compile-context. Drop down to the parent
		// BYTECODE chain and walk it looking for a context-related
		// entry. Single-hop wouldn't be enough — there may be an
		// intermediate bc (form bc, object bc) between the eval and
		// the root common module that holds the bindings. First match
		// in any ancestor wins; emitter (compileCode.cpp) checks
		// foundedVar->m_strContext to decide direct vs OPER_GET_A.
		if (m_parentContext == nullptr) {
			for (const ibByteCode* bc = m_compileModule->m_cByteCode.m_parent; bc != nullptr; bc = bc->m_parent) {
				if (bc->FindVariable(strVarName, foundedVar)) {
					return foundedVar->IsContextRelated();
				}
			}
		}

		foundedVar = nullptr;
		return false;
	}
	else if (it != m_listVariable.end()) {
		foundedVar = it->second;
		return true;
	}

	foundedVar = nullptr;
	return false;
}

/**
 * Search for a variable in a hash array
 * Returns true - if the variable is found
 */

bool ibCompileContext::FindFunction(const wxString& strFuncName, std::shared_ptr<ibFunction>& foundedFunc, bool contextVar)
{
	auto it = std::find_if(m_listFunction.begin(), m_listFunction.end(),
		[strFuncName](const auto pair) { return stringUtils::CompareString(strFuncName, pair.first); });

	if (contextVar) {
		if (it != m_listFunction.end() && it->second) {
			foundedFunc = it->second;
			return it->second->m_bContext;
		}

		if (m_parentContext && m_parentContext->FindFunction(strFuncName, foundedFunc, contextVar))
			return true;

		// Reached the top compile-context. Drop down to the parent
		// BYTECODE chain — symmetric with FindVariable's fallback.
		// Emitter (compileCode.cpp) decides direct OPER_CALL vs
		// OPER_CALL_M based on foundedFunc->m_strContext.
		if (m_parentContext == nullptr) {
			for (const ibByteCode* bc = m_compileModule->m_cByteCode.m_parent; bc != nullptr; bc = bc->m_parent) {
				if (bc->FindFunction(strFuncName, foundedFunc)) {
					return foundedFunc->IsContextRelated();
				}
			}
		}

		foundedFunc = nullptr;
		return false;
	}
	else if (it != m_listFunction.end() && it->second) {
		foundedFunc = it->second;
		return true;
	}

	foundedFunc = nullptr;
	return false;
}

/**
 * Linking GOTO statements to labels
 */
void ibCompileContext::CreateLabels()
{
	wxASSERT(m_compileModule != nullptr);
	for (unsigned int i = 0; i < m_listLabel.size(); i++) {

		const wxString& strName = m_listLabel[i]->m_strName;
		const int oldLine = m_listLabel[i]->m_numLine;

		//look for such a label in the list of declared labels
		unsigned int currLine = m_listLabelDef[strName];
		if (!currLine) {
			m_compileModule->m_numCurrentCompile = m_listLabel[i]->m_numError;
			m_compileModule->SetError(ERROR_LABEL_DEFINE, strName); // duplicate label definitions occurred
		}

		// write the address of the label:
		m_compileModule->m_cByteCode.m_listCode[oldLine].m_param1.m_numIndex = currLine + 1;
	}
};