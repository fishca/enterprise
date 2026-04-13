#include "compileContext.h"
#include "compileCode.h"

////////////////////////////////////////////////////////////////////////
// CCompileContext CCompileContext CCompileContext CCompileContext//
////////////////////////////////////////////////////////////////////////

/**
* Create a new variable identifier
*/

CParamUnit CCompileContext::CreateVariable(const wxString strPrefix)
{
	const wxString& strTempName = wxString::Format(strPrefix + wxT("%d"), ++m_numTempVar); //@temp_ - to ensure the uniqueness of the name

	CParamUnit variable = GetVariable(strTempName, false, false, false, true); //we look for a temporary variable only in the local context
	variable.m_numArray = DEF_VAR_TEMP; //flag of a temporary local variable
	return variable;
}

/**
* Adds a new variable to the list
* Returns the added variable as a CParamUnit
*/

CParamUnit CCompileContext::AddVariable(const wxString& strVarName,
	const wxString& typeVar, bool exportVar, bool contextVar, bool tempVar)
{
	std::shared_ptr<CVariable> foundedVariable = nullptr;
	if (FindVariable(strVarName, foundedVariable)) { //there was a declaration + repeated declaration = error
		m_compileModule->SetError(ERROR_IDENTIFIER_DUPLICATE, strVarName);
		return CParamUnit();
	}

	unsigned int numVariable = m_listVariable.size();
	PushVariable(strVarName, wxEmptyString, numVariable,
		typeVar, exportVar, contextVar, tempVar);

	CParamUnit variable;

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

CParamUnit CCompileContext::GetVariable(const wxString& strVarName, bool bFindInParent, bool bCheckError, bool contextVar, bool tempVar)
{
	int numCanUseLocalInParent = m_numFindLocalInParent;

	std::shared_ptr <CVariable> currentVariable = nullptr;
	if (!FindVariable(strVarName, currentVariable)) {

		//search in parent contexts (modules)
		if (bFindInParent) {
			int numParent = 0;
			CCompileContext* pCurContext = m_parentContext;
			while (pCurContext) {
				numParent++;
				if (numParent > MAX_OBJECTS_LEVEL) {
					//CSystemFunction::Message(pCurContext->m_compileModule->GetModuleName());
					if (numParent > 2 * MAX_OBJECTS_LEVEL) {
						CBackendCoreException::Error(_("Recursive call of modules!"));
					}
				}
				if (pCurContext->FindVariable(strVarName, currentVariable)) { // found
					//check if this is an export variable or not (if m_numFindLocalInParent=true, then you can take local variables of the parent)
					if (numCanUseLocalInParent > 0 ||
						currentVariable->m_bExport) {

						CParamUnit variable;

						//determine the variable number
						variable.m_numArray = numParent;
						variable.m_numIndex = currentVariable->m_numVariable;
						variable.m_strType = currentVariable->m_strType;

						return variable;
					}
				}
				numCanUseLocalInParent--;
				pCurContext = pCurContext->m_parentContext;
			}
		}

		if (bCheckError) {
			m_compileModule->SetError(ERROR_VAR_NOT_FOUND, strVarName); //display an error message
			return CParamUnit();
		}

		// there was no variable declaration yet - add
		return AddVariable(strVarName, wxEmptyString, contextVar, contextVar, tempVar);
	}

	wxASSERT(currentVariable);

	CParamUnit variable;

	//determine the number and type of the variable
	variable.m_numArray = 0;
	variable.m_numIndex = currentVariable->m_numVariable;
	variable.m_strType = currentVariable->m_strType;

	return variable;
}

void CCompileContext::PushVariable(const wxString& strVarName, const wxString& strContextVar, unsigned int numVariable,
	const wxString& typeVar, bool exportVar, bool contextVar, bool tempVar)
{
	std::shared_ptr<CVariable> currentVariable(
		new CVariable(stringUtils::MakeUpper(strVarName))
	);

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

void CCompileContext::PushFunction(const wxString& strFuncName, const wxString& strContextVar, const wxString& strShortDescription, unsigned int numFunction, bool hasRetVal, int argCount)
{
	std::shared_ptr<CCompileContext::CFunction> contextFunction(
		new CFunction(stringUtils::MakeUpper(strFuncName), CreateContext(hasRetVal ? RETURN_FUNCTION : RETURN_PROCEDURE))
	);

	contextFunction->m_nStart = numFunction;
	contextFunction->m_bContext = true;
	contextFunction->m_bExport = true;

	contextFunction->m_strContext = strContextVar; //variable for which the attribute is called

	if (argCount > 0) contextFunction->m_listParam.reserve(argCount);

	for (long arg = 0; arg < argCount; arg++) {
		CFunction::CParamVariable contextVariable;
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

bool CCompileContext::FindVariable(const wxString& strVarName, std::shared_ptr<CVariable>& foundedVar, bool contextVar)
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

bool CCompileContext::FindFunction(const wxString& strFuncName, std::shared_ptr<CFunction>& foundedFunc, bool contextVar)
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
void CCompileContext::DoLabels()
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