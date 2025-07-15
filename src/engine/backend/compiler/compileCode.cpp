////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, 2—-team
//	Description : compile module 
////////////////////////////////////////////////////////////////////////////

#include "compileCode.h"
#include "codeDef.h"

#include "system/systemManager.h"

#pragma warning(push)
#pragma warning(disable : 4018)

//////////////////////////////////////////////////////////////////////
//                           Constants
//////////////////////////////////////////////////////////////////////

// array of mathematical operation priorities
static std::array<int, 256> gs_operPriority = {0};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction CCompileCode
//////////////////////////////////////////////////////////////////////

CCompileCode::CCompileCode() :
    CTranslateCode(),
    m_parent(nullptr), m_rootContext(new CCompileContext),
    m_bExpressionOnly(false), m_changedCode(false),
    m_onlyFunction(false)
{
    InitializeCompileModule();

    // we donТt look for local variables in parent contexts!
    m_rootContext->m_numFindLocalInParent = 0;
}

CCompileCode::CCompileCode(const wxString& strModuleName, const wxString& strDocPath, bool onlyFunction) :
    CTranslateCode(strModuleName, strDocPath),
    m_parent(nullptr), m_rootContext(new CCompileContext),
    m_bExpressionOnly(false), m_changedCode(false),
    m_onlyFunction(onlyFunction)
{
    InitializeCompileModule();

    // we donТt look for local variables in parent contexts!
    m_rootContext->m_numFindLocalInParent = 0;
}

CCompileCode::CCompileCode(const wxString& strFileName) :
    CTranslateCode(strFileName),
    m_parent(nullptr), m_rootContext(new CCompileContext),
    m_bExpressionOnly(false), m_changedCode(false),
    m_onlyFunction(false)
{
    InitializeCompileModule();

    // we donТt look for local variables in parent contexts!
    m_rootContext->m_numFindLocalInParent = 0;
}

CCompileCode::~CCompileCode()
{
    Reset();

    m_listExternValue.clear();
    m_listContextValue.clear();

    wxDELETE(m_rootContext);
}

void CCompileCode::InitializeCompileModule()
{
    if (gs_operPriority[gs_operPriority.size() - 1])
        return;

    gs_operPriority['+'] = 10;
    gs_operPriority['-'] = 10;
    gs_operPriority['*'] = 30;
    gs_operPriority['/'] = 30;
    gs_operPriority['%'] = 30;
    gs_operPriority['!'] = 50;

    gs_operPriority[KEY_OR] = 1;
    gs_operPriority[KEY_AND] = 2;

    gs_operPriority['>'] = 3;
    gs_operPriority['<'] = 3;
    gs_operPriority['='] = 3;

    gs_operPriority[gs_operPriority.size() - 1] = true;
}

void CCompileCode::Reset()
{
    m_listHashConst.clear();
    m_cByteCode.Reset();

    m_rootContext->m_numDoNumber = 0;
    m_rootContext->m_numReturn = 0;
    m_rootContext->m_numTempVar = 0;
    m_rootContext->m_numFindLocalInParent = 1;

    m_rootContext->m_listContinue.clear();
    m_rootContext->m_listBreak.clear();

    m_rootContext->m_listLabel.clear();
    m_rootContext->m_listLabelDef.clear();

    m_rootContext->m_strCurFuncName = wxEmptyString;

    // clear functions & variables 
    for (auto& function : m_rootContext->m_listFunction) {
        wxDELETE(function.second);
    }

    m_rootContext->m_listVariable.clear();
    m_rootContext->m_listFunction.clear();

    m_listCallFunc.clear();
}

void CCompileCode::PrepareModuleData()
{
    for (auto& externValue : m_listExternValue) {
        m_rootContext->AddVariable(externValue.first, wxEmptyString, true);
        m_cByteCode.m_listExternValue.push_back(externValue.second);
    }
    
    for (auto& contextValue : m_listContextValue) {
        m_rootContext->AddVariable(contextValue.first, wxEmptyString, true, true);
        m_cByteCode.m_listExternValue.push_back(contextValue.second);
    }

    for (auto& pair : m_listContextValue) {

        CValue* contextValue = pair.second;
        wxASSERT(contextValue);
        contextValue->PrepareNames();

        // adding variables from context
        for (unsigned int i = 0; i < contextValue->GetNProps(); i++) {

            const wxString& strPropName = contextValue->GetPropName(i);

            // determine the number and type of the variable
            CVariable contextVariable(strPropName);
            contextVariable.m_strContextVar = pair.first;
            contextVariable.m_bContext = true;
            contextVariable.m_bExport = true;
            contextVariable.m_numVariable = i;

            GetContext()->m_listVariable.insert_or_assign(
                stringUtils::MakeUpper(strPropName), contextVariable
            );
        }

        CCompileContext* mainContext = GetContext();

        // add methods from context
        for (long i = 0; i < contextValue->GetNMethods(); i++) {

            const wxString& strMethodName = contextValue->GetMethodName(i);

            CCompileContext* compileContext(new CCompileContext(mainContext));
            compileContext->SetModule(this);

            if (contextValue->HasRetVal(i)) compileContext->m_numReturn = RETURN_FUNCTION;
            else compileContext->m_numReturn = RETURN_PROCEDURE;

            // define the number and type of the function
            CFunction* contextFunction = new CFunction(strMethodName, compileContext);
            contextFunction->m_nStart = i;
            contextFunction->m_bContext = true;
            contextFunction->m_bExport = true;
            contextFunction->m_strContextVar = pair.first;

            CParamVariable contextVariable;
            contextVariable.m_valData.m_numArray = DEF_VAR_DEFAULT;
            contextVariable.m_valData.m_numIndex = DEF_VAR_DEFAULT;

            for (long arg = 0; arg < contextValue->GetNParams(i); arg++) {
                contextFunction->m_listParam.push_back(contextVariable);
            }

            contextFunction->m_strRealName = strMethodName;
            contextFunction->m_strShortDescription = contextValue->GetMethodHelper(i);

            // check for typing
            GetContext()->m_listFunction.insert_or_assign(
                stringUtils::MakeUpper(strMethodName), contextFunction
            );
        }
    }
}

/**
 * SetError
 * Purpose:
 * Remember the translation error and throw an exception
 * Return value:
 * The method does not return control!
 */

void CCompileCode::SetError(int codeError, const wxString& errorDesc)
{
    wxString strFileName, strModuleName, strDocPath; int currPos = 0, currLine = 0;

    if (m_numCurrentCompile >= m_listLexem.size()) {
        m_numCurrentCompile = m_listLexem.size() - 1;
    }

    if (m_numCurrentCompile >= 0 && m_numCurrentCompile < m_listLexem.size()) {

        strFileName = m_listLexem[m_numCurrentCompile].m_strFileName;
        strModuleName = m_listLexem[m_numCurrentCompile].m_strModuleName;
        strDocPath = m_listLexem[m_numCurrentCompile].m_strDocPath;

        if (m_listLexem[m_numCurrentCompile].m_lexType != ENDPROGRAM) {
            currLine = m_listLexem[m_numCurrentCompile].GetLine();
            currPos = m_listLexem[m_numCurrentCompile].EndPos();
        }
        else {
            currLine = m_listLexem[m_numCurrentCompile - 1].GetLine();
            currPos = m_listLexem[m_numCurrentCompile - 1].EndPos();
        }
    }

    CTranslateCode::SetError(codeError,
        strFileName, strModuleName, strDocPath,
        currPos, currLine,
        errorDesc);
}

/**
 * another function option
 */

void CCompileCode::SetError(int nErr, const wxUniChar& c)
{
    SetError(nErr, wxString::Format(wxT("%c"), c));
}

/**
 * ProcessError
 * Purpose:
 * Remember the translation error and throw an exception
 * Return value:
 * The method does not return control!
 */

void CCompileCode::ProcessError(const wxString& strFileName,
    const wxString& strModuleName, const wxString& strDocPath,
    unsigned int currPos, unsigned int currLine,
    const wxString& strCodeLineError, int codeError, const wxString& strErrorDesc) const
{
    CBackendException::ProcessError(
        strFileName,
        strModuleName, strDocPath,
        currPos, currLine,
        strCodeLineError, codeError, strErrorDesc
    );
}

//////////////////////////////////////////////////////////////////////
// Compiling
//////////////////////////////////////////////////////////////////////

/**
 *adding information about the current line to the byte code
 */

void CCompileCode::AddLineInfo(CByteUnit& code)
{
    code.m_strModuleName = m_strModuleName;
    code.m_strDocPath = m_strDocPath;
    code.m_strFileName = m_strFileName;

    if (m_numCurrentCompile >= 0 && m_numCurrentCompile < m_listLexem.size()) {
        if (m_listLexem[m_numCurrentCompile].m_lexType != ENDPROGRAM) {
            code.m_strModuleName = m_listLexem[m_numCurrentCompile].m_strModuleName;
            code.m_strDocPath = m_listLexem[m_numCurrentCompile].m_strDocPath;
            code.m_strFileName = m_listLexem[m_numCurrentCompile].m_strFileName;
        }
        code.m_numString = m_listLexem[m_numCurrentCompile].m_numString;
        code.m_numLine = m_listLexem[m_numCurrentCompile].m_numLine;
    }
}

/**
 * GetLexem
 * Purpose:
 * Get the next token from the list of byte codes and increment the current position counter by 1
 * Return value:
 * 0 or pointer to token
 */

CLexem CCompileCode::GetLexem()
{
    CLexem lex;
    if (m_numCurrentCompile + 1 < m_listLexem.size()) {
        lex = m_listLexem[++m_numCurrentCompile];
    }
    return lex;
}

// get the next token from a list of bytecodes without incrementing the current position counter
CLexem CCompileCode::PreviewGetLexem()
{
    CLexem lex;
    while (true) {
        lex = GetLexem();
        if (!(lex.m_lexType == DELIMITER && lex.m_numData == ';')) break;
    }
    m_numCurrentCompile--;
    return lex;
}

/**
 * GETLexem
 * Purpose:
 * Get the next token from the list of byte codes and increment the current position counter by 1
 * Return value:
 * no (if failure occurs, an exception is thrown)
 */

CLexem CCompileCode::GETLexem()
{
    CLexem lex = GetLexem();
    if (lex.m_lexType == ERRORTYPE) {
        m_numCurrentCompile--;
        SetError(ERROR_CODE_DEFINE);
        return CLexem();
    }
    return lex;
}

/**
 * GETDelimeter
 * Purpose:
 * Get the next token as the given delimiter
 * Return value:
 * no (if failure occurs, an exception is thrown)
 */

void CCompileCode::GETDelimeter(const wxUniChar& c)
{
    CLexem lex = GETLexem();
    if (!(lex.m_lexType == DELIMITER && c == lex.m_numData)) {
        m_numCurrentCompile--;
        SetError(ERROR_DELIMETER, c);
    }
}

/**
 * IsKeyWord
 * Purpose:
 * Check if the current bytecode token is a given keyword
 * Return value:
 * true, false
 */

bool CCompileCode::IsKeyWord(int k)
{
    if (m_numCurrentCompile + 1 < m_listLexem.size()) {
        const CLexem& lex = m_listLexem[m_numCurrentCompile];
        if (lex.m_lexType == KEYWORD && lex.m_numData == k)
            return true;
    }
    return false;
}

/**
 * IsNextKeyWord
 * Purpose:
 * Check if the next bytecode token is a given keyword
 * Return value:
 * true,false
 */

bool CCompileCode::IsNextKeyWord(int k)
{
    if (m_numCurrentCompile + 1 < m_listLexem.size()) {
        const CLexem& lex = m_listLexem[m_numCurrentCompile + 1];
        if (lex.m_lexType == KEYWORD && k == lex.m_numData)
            return true;
    }
    return false;
}

/**
 * IsDelimeter
 * Purpose:
 * Check if the current bytecode token is a given delimiter
 * Return value:
 * true,false
 */

bool CCompileCode::IsDelimeter(const wxUniChar& c)
{
    if (m_numCurrentCompile + 1 < m_listLexem.size()) {
        const CLexem& lex = m_listLexem[m_numCurrentCompile];
        if (lex.m_lexType == DELIMITER && c == lex.m_numData)
            return true;
    }
    return false;
}

/**
 * IsNextDelimeter
 * Purpose:
 * Check if the next bytecode token is a given delimiter
 * Return value:
 * true,false
 */

bool CCompileCode::IsNextDelimeter(const wxUniChar& c)
{
    if (m_numCurrentCompile + 1 < m_listLexem.size()) {
        const CLexem& lex = m_listLexem[m_numCurrentCompile + 1];
        if (lex.m_lexType == DELIMITER && c == lex.m_numData)
            return true;
    }
    return false;
}

/**
 * GETKeyWord
 * Get the next token as the given keyword
 * Return value:
 * no (if failure occurs, an exception is thrown)
 */

void CCompileCode::GETKeyWord(int nKey)
{
    const CLexem& lex = GETLexem();
    if (!(lex.m_lexType == KEYWORD && lex.m_numData == nKey)) {
        m_numCurrentCompile--;
        SetError(ERROR_KEYWORD,
            wxString::Format(wxT("%s"), s_listKeyWord[nKey].m_strKeyWord)
        );
    }
}

/**
 * GETIdentifier
 * Get the next token as the given keyword
 * Return value:
 * identifier string
 */

wxString CCompileCode::GETIdentifier(bool strRealName)
{
    const CLexem& lex = GETLexem();
    if (lex.m_lexType != IDENTIFIER) {
        m_numCurrentCompile--;
        SetError(ERROR_IDENTIFIER_DEFINE);
        return wxEmptyString;
    }

    if (strRealName) {
        return lex.m_valData.m_sData;
    }

    return lex.m_strData;
}

/**
 * GETConstant
 * Get the next token as a constant
 * Return value:
 * constant
 */

CValue CCompileCode::GETConstant()
{
    CLexem lex;
    int iNumRequire = 0;
    if (IsNextDelimeter('-') || IsNextDelimeter('+')) {
        iNumRequire = 1;
        if (IsNextDelimeter('-'))
            iNumRequire = -1;
        lex = GETLexem();
    }

    lex = GETLexem();

    if (lex.m_lexType != CONSTANT) {
        SetError(ERROR_CONST_DEFINE);
        return CValue();
    }

    if (iNumRequire) {
        // check that the constant is of numeric type
        if (lex.m_valData.GetType() != eValueTypes::TYPE_NUMBER) {
            SetError(ERROR_CONST_DEFINE);
            return CValue();
        }
        // change sign for minus
        if (iNumRequire == -1) {
            lex.m_valData.m_fData = -lex.m_valData.m_fData;
        }
    }
    return lex.m_valData;
}

// getting the number with a string constant (to determine the method number)
const int CCompileCode::GetConstString(const wxString& strConstName)
{
    auto& it = std::find_if(m_listHashConst.begin(), m_listHashConst.end(),
        [strConstName](std::pair <const wxString, unsigned int>& pair)
    {
        return stringUtils::CompareString(strConstName, pair.first);
    }
    );

    if (it != m_listHashConst.end())
        return it->second - 1;

    m_cByteCode.m_listConst.push_back(strConstName);
    m_listHashConst.insert_or_assign(strConstName, m_cByteCode.m_listConst.size());

    return m_cByteCode.m_listConst.size() - 1;
}

/**
 * AddVariable
 * Purpose:
 * Add the name and address of an external variable to a special array for later use
 */

void CCompileCode::AddVariable(const wxString& strVarName, const CValue& vObject)
{
    if (strVarName.IsEmpty())
        return;

    // take into account external variables during compilation
    m_listExternValue[strVarName.Upper()] = vObject.m_typeClass == eValueTypes::TYPE_REFFER
        ? vObject.GetRef() : const_cast<CValue*>(&vObject);

    //set the flag for recompilation
    m_changedCode = true;
}

/**
 * AddVariable
 * Purpose:
 * Add the name and address of an external variable to a special array for later use
 */

void CCompileCode::AddVariable(const wxString& strVarName, CValue* pValue)
{
    if (strVarName.IsEmpty())
        return;

    // take into account external variables during compilation
    m_listExternValue[strVarName.Upper()] = pValue;

    //set the flag for recompilation
    m_changedCode = true;
}

/**
 * AddContextVariable
 * Purpose:
 * Add the name and address of an external variable to a special array for later use
 */

void CCompileCode::AddContextVariable(const wxString& strVarName, const CValue& vObject)
{
    if (strVarName.IsEmpty())
        return;

    //adding variables from context
    m_listContextValue[strVarName.Upper()] = vObject.m_typeClass == eValueTypes::TYPE_REFFER ? vObject.GetRef() : const_cast<CValue*>(&vObject);

    //set the flag for recompilation
    m_changedCode = true;
}

/**
 * AddContextVariable
 * Purpose:
 * Add the name and address of an external variable to a special array for later use
 */

void CCompileCode::AddContextVariable(const wxString& strVarName, CValue* pValue)
{
    if (strVarName.IsEmpty())
        return;

    //adding variables from context
    m_listContextValue[strVarName.Upper()] = pValue;

    //set the flag for recompilation
    m_changedCode = true;
}

/**
 * RemoveVariable
 * Purpose:
 * Remove the name and address of an external variable
 */

void CCompileCode::RemoveVariable(const wxString& strVarName)
{
    if (strVarName.IsEmpty())
        return;

    m_listExternValue.erase(strVarName.Upper());
    m_listContextValue.erase(strVarName.Upper());

    //set the flag for recompilation
    m_changedCode = true;
}

/**
 * Recompile
 * Purpose:
 * Translation and compilation of source code into bytecode (object code)
 * Return value:
 * true,false
 */

bool CCompileCode::Recompile()
{
    //clear functions & variables
    Reset();

    //prepare lexem 
    if (!PrepareLexem()) {
        return false;
    }

    // prepare context variables
    PrepareModuleData();

    // compilation 
    if (CompileModule()) {
        m_changedCode = false;
        return true;
    }

    return false;
}

/**
 * Compile
 * Purpose:
 * Translation and compilation of source code into bytecode (object code)
 * Return value:
 * true,false
 */

bool CCompileCode::Compile()
{
    //clear functions & variables
    Reset();

    //prepare lexem 
    if (!PrepareLexem()) {
        return false;
    }

    // prepare context variables
    PrepareModuleData();

    // compilation 
    if (CompileModule()) {
        m_changedCode = false;
        return true;
    }

    return false;
}

/**
 * Compile
 * Purpose:
 * Translation and compilation of source code into bytecode (object code)
 * Return value:
 * true,false
 */

bool CCompileCode::Compile(const wxString& strCode)
{
    //clear functions & variables
    Reset();

    Load(strCode);

    //prepare lexem
    if (!PrepareLexem()) {
        return false;
    }

    // prepare context variables
    PrepareModuleData();

    // compilation 
    if (CompileModule()) {
        m_changedCode = false;
        return true;
    }

    return false;
}

bool CCompileCode::IsTypeVar(const wxString& strType)
{
    if (!strType.IsEmpty()) {
        if (CValue::IsRegisterCtor(strType, eCtorObjectType::eCtorObjectType_object_primitive))
            return true;
    }
    const CLexem& lex = PreviewGetLexem();
    if (CValue::IsRegisterCtor(lex.m_strData, eCtorObjectType::eCtorObjectType_object_primitive))
        return true;
    return false;
}

wxString CCompileCode::GetTypeVar(const wxString& strType)
{
    if (!strType.IsEmpty()) {
        if (!CValue::IsRegisterCtor(strType, eCtorObjectType::eCtorObjectType_object_primitive)) {
            SetError(ERROR_TYPE_DEF);
            return wxEmptyString;
        }
        return strType.Upper();
    }
    const CLexem& lex = GETLexem();
    if (!CValue::IsRegisterCtor(lex.m_strData, eCtorObjectType::eCtorObjectType_object_primitive)) {
        SetError(ERROR_TYPE_DEF);
        return wxEmptyString;
    }
    return lex.m_strData.Upper();
}

/**
 * CompileDeclaration
 * Purpose:
 * Compiling explicit variable declarations
 * Return value:
 * true,false
 */

bool CCompileCode::CompileDeclaration(CCompileContext* context)
{
    const CLexem& lex = PreviewGetLexem(); wxString strType;
    if (lex.m_lexType == IDENTIFIER) {
        strType = GetTypeVar(); // typed setting of variables
    }
    else {
        GETKeyWord(KEY_VAR);
    }

    while (true) {
        wxString strRealName = GETIdentifier(true);
        wxString strName = stringUtils::MakeUpper(strRealName);
        int numParent = 0;
        CCompileContext* pCurContext = context;
        while (pCurContext) {
            numParent++;
            if (numParent > MAX_OBJECTS_LEVEL) {
                CSystemFunction::Message(pCurContext->m_compileModule->GetModuleName());
                if (numParent > 2 * MAX_OBJECTS_LEVEL) {
                    CBackendException::Error("Recursive call of modules!");
                }
            }
            CVariable* currentVariable = nullptr;
            if (pCurContext->FindVariable(strName, currentVariable)) { // found
                if (currentVariable->m_bExport ||
                    pCurContext->m_compileModule == this) {
                    SetError(ERROR_DEF_VARIABLE, strRealName);
                    return false;
                }
            }
            pCurContext = pCurContext->m_parentContext;
        }

        int nArrayCount = -1;
        if (IsNextDelimeter('[')) { // this is an array declaration
            nArrayCount = 0;
            GETDelimeter('[');
            if (!IsNextDelimeter(']')) {
                CValue vConst = GETConstant();
                if (vConst.GetType() != eValueTypes::TYPE_NUMBER ||
                    vConst.GetNumber() < 0) {
                    SetError(ERROR_ARRAY_SIZE_CONST);
                    return false;
                }
                nArrayCount = vConst.GetInteger();
            }
            GETDelimeter(']');
        }

        bool bExport = false;

        if (IsNextKeyWord(KEY_EXPORT)) {
            if (bExport) // there was an Export announcement
                break;
            GETKeyWord(KEY_EXPORT);
            bExport = true;
        }

        // there was no variable declaration yet - add
        CParamUnit variable =
            context->AddVariable(strRealName, strType, bExport);

        if (nArrayCount >= 0) { // write information about the arrays
            CByteUnit code;
            AddLineInfo(code);
            code.m_numOper = OPER_SET_ARRAY_SIZE;
            code.m_param1 = variable;
            code.m_param2.m_numArray = nArrayCount;//число элементов в массиве
            m_cByteCode.m_listCode.push_back(code);
        }

        AddTypeSet(variable);

        if (IsNextDelimeter('=')) { // initial initialization - works only inside the text of modules (but not re-declaring procedures and functions)

            if (nArrayCount >= 0)  GETDelimeter(','); // error!

            GETDelimeter('=');

            CByteUnit code;
            AddLineInfo(code);
            code.m_numOper = OPER_LET;
            code.m_param1 = variable;
            code.m_param2 = GetExpression(context);
            m_cByteCode.m_listCode.push_back(code);
        }

        if (!IsNextDelimeter(','))
            break;

        GETDelimeter(',');
    }

    return true;
}

/**
 *CompileModule
 * Purpose:
 * Compiling all bytecode (creating object code from a set of tokens)
 * Return value:
 * true,false
*/

bool CCompileCode::CompileModule()
{
    CLexem lex;

    // set the cursor to the beginning of the token array
    m_numCurrentCompile = -1;
    CCompileContext* mainContext = GetContext(); // context of the module itself

    while ((lex = PreviewGetLexem()).m_lexType != ERRORTYPE) {
        if ((KEYWORD == lex.m_lexType && lex.m_numData == KEY_VAR) || (IDENTIFIER == lex.m_lexType && IsTypeVar(lex.m_strData))) {
            if (!m_onlyFunction) {
                CompileDeclaration(mainContext); // load variable declaration
            }
            else {
                SetError(ERROR_ONLY_FUNCTION);
                return false;
            }
        }
        else if (KEYWORD == lex.m_lexType && (KEY_PROCEDURE == lex.m_numData || KEY_FUNCTION == lex.m_numData)) {
            CompileFunction(mainContext); // load function declaration
            // don't forget to restore the current module context (if necessary)...
        }
        else break;
    }

    // load the executable body of the module
    m_cByteCode.m_lStartModule = 0;
    CompileBlock(mainContext);

    mainContext->DoLabels();

    // set the end of the program
    CByteUnit code;
    AddLineInfo(code);
    code.m_numOper = OPER_END;

    m_cByteCode.m_listCode.push_back(code);
    m_cByteCode.m_lVarCount = mainContext->m_listVariable.size();

    // we finish processing procedures and functions that were called before they were declared
    // for this, at the end of the bytecode array, add new code to call such functions,
    // and for correct operation we insert GOTO statements into places of early calls
    for (auto& callFunc : m_listCallFunc) {
        m_cByteCode.m_listCode[callFunc->m_numAddLine].m_param1.m_numIndex =
            m_cByteCode.m_listCode.size(); // go to function call
        if (PushCallFunction(callFunc)) {
            // correcting labels
            CByteUnit code;
            AddLineInfo(code);
            code.m_numOper = OPER_GOTO;
            code.m_numLine = callFunc->m_numLine;
            code.m_numString = callFunc->m_numString;
            code.m_param1.m_numIndex = callFunc->m_numAddLine + 1; // after calling the function we go back
            m_cByteCode.m_listCode.push_back(code);
        }
    }

    // get a list of variables
    for (auto it : mainContext->m_listVariable) {
        if (it.second.m_bTempVar || it.second.m_bContext)
            continue;
        m_cByteCode.m_listVar[it.first] = it.second.m_numVariable;
        if (it.second.m_bExport) {
            m_cByteCode.m_listExportVar[it.first] = it.second.m_numVariable;
        }
    }

    if (m_numCurrentCompile + 1 < m_listLexem.size() - 1) {
        SetError(ERROR_END_PROGRAM);
        return false;
    }

    m_cByteCode.SetModule(this);

    // compilation completed successfully
    m_cByteCode.m_bCompile = true;

    return true;
}

// search for function definition in the current module and all parent ones
CFunction* CCompileCode::GetFunction(const wxString& strName, int* pNumFunction)
{
    int numCanUseLocalInParent = m_rootContext->m_numFindLocalInParent - 1;
    int numFunction = 0;
    // search in the current module
    CFunction* foundedFunc = nullptr;
    if (!GetContext()->FindFunction(strName, foundedFunc)) {
        CCompileCode* pCurModule = m_parent;
        while (pCurModule != nullptr) {
            numFunction++;
            if (pCurModule->GetContext()->FindFunction(strName, foundedFunc)) { // found
                // see if this is an export function or not
                if (numCanUseLocalInParent > 0 || foundedFunc->m_bExport)
                    break;//ок
                foundedFunc = nullptr;
            }
            numCanUseLocalInParent--;
            pCurModule = pCurModule->m_parent;
        }
    }
    if (pNumFunction)
        *pNumFunction = numFunction;
    return foundedFunc;
}

// adding the bytecode of the function call to the array
bool CCompileCode::PushCallFunction(const std::shared_ptr<CCallFunction>& callFunction)
{
    int numModule = 0;

    // find the definition of the function
    CFunction* foundedFunc = GetFunction(callFunction->m_strName, &numModule);
    if (foundedFunc == nullptr) {
        m_numCurrentCompile = callFunction->m_numError;
        SetError(ERROR_CALL_FUNCTION, callFunction->m_strRealName);// there is no such function in the module
        return false;
    }

    if (!callFunction->m_numIsSet && foundedFunc->m_compileContext && foundedFunc->m_compileContext->m_numReturn != RETURN_FUNCTION) {
        m_numCurrentCompile = callFunction->m_numError;
        SetError(ERROR_USE_PROCEDURE_AS_FUNCTION, foundedFunc->m_strRealName);
        return false;
    }

    // check the match between the number of passed and declared parameters
    unsigned int numRealCount = callFunction->m_listParam.size();
    unsigned int numDefCount = foundedFunc->m_listParam.size();

    if (numRealCount > numDefCount) {
        m_numCurrentCompile = callFunction->m_numError;
        SetError(ERROR_MANY_PARAMS);// too many parameters
        return false;
    }

    CByteUnit code;
    AddLineInfo(code);

    code.m_numString = callFunction->m_numString;
    code.m_numLine = callFunction->m_numLine;
    code.m_strModuleName = callFunction->m_strModuleName;

    if (foundedFunc->m_bContext) { // virtual function - calling replacements with the construct Context.FunctionName(...)
        code.m_numOper = OPER_CALL_M;
        code.m_param1 = callFunction->m_puRetValue;		// variable into which the value is returned
        code.m_param2 = callFunction->m_puContextVal;	// variable on which the method is called
        code.m_param3.m_numIndex = GetConstString(callFunction->m_strName);	// number of the called method from the list of encountered methods
        code.m_param3.m_numArray = numDefCount;	// number of parameters
    }
    else {
        code.m_numOper = OPER_CALL;
        code.m_param1 = callFunction->m_puRetValue;	// variable into which the value is returned
        code.m_param2.m_numArray = numModule;		// module number
        code.m_param2.m_numIndex = foundedFunc->m_nStart;	// starting position
        code.m_param3.m_numArray = numDefCount;			// number of parameters
        code.m_param3.m_numIndex = foundedFunc->m_lVarCount;	// number of local variables
        code.m_param4 = callFunction->m_puContextVal;	// context variable
    }

    m_cByteCode.m_listCode.push_back(code);

    for (unsigned int i = 0; i < numDefCount; i++) {
        CByteUnit code;
        AddLineInfo(code);
        code.m_numOper = OPER_SET; // parameters are being passed
        bool defaultValue = false;
        if (i < numRealCount) {
            code.m_param1 = callFunction->m_listParam[i];
            if (code.m_param1.m_numArray == DEF_VAR_SKIP) { // need to substitute the default value
                defaultValue = true;
            }
            else {  //тип передачи значений
                code.m_param2.m_numIndex = foundedFunc->m_listParam[i].m_bByRef;
            }
        }
        else {
            defaultValue = true;
        }
        if (defaultValue) {
            if (foundedFunc->m_listParam[i].m_valData.m_numArray == DEF_VAR_SKIP) {
                m_numCurrentCompile = callFunction->m_numError;
                SetError(ERROR_FEW_PARAMS);	// too few parameters
                return false;
            }
            code.m_numOper = OPER_SETCONST;	// default values
            code.m_param1 = foundedFunc->m_listParam[i].m_valData;
        }
        m_cByteCode.m_listCode.push_back(code);
    }

    return true;
}

/**
 * CompileFunction
 * Purpose:
 * Creating object code for one function (procedure)
 * Algorithm:
 * - Determine the number of formal parameters
 * - Define ways to call formal parameters (by reference or by value)
 * - Define default values
 * - Determine the number of local variables
 * - Determine whether the function returns a value
 *
 * Return value:
 * true,false
 */

bool CCompileCode::CompileFunction(CCompileContext* context)
{
    CCompileContext* functionContext = nullptr;

    // we are now at the token level, where the FUNCTION or PROCEDURE keyword is specified
    if (IsNextKeyWord(KEY_FUNCTION)) {
        GETKeyWord(KEY_FUNCTION);
        functionContext = new CCompileContext(context);	// create a new context in which we will compile the function body
        functionContext->SetModule(this);
        functionContext->m_numReturn = RETURN_FUNCTION;
    }
    else if (IsNextKeyWord(KEY_PROCEDURE)) {
        GETKeyWord(KEY_PROCEDURE);
        functionContext = new CCompileContext(context);	// create a new context in which we will compile the body of the procedure
        functionContext->SetModule(this);
        functionContext->m_numReturn = RETURN_PROCEDURE;
    }
    else {
        SetError(ERROR_FUNC_DEFINE);
        return false;
    }

    // pull out the text of the function declaration
    CLexem lex = PreviewGetLexem();

    wxString strShortDescription;
    const int numLine = lex.m_numLine;
    int numRes = m_strBuffer.find('\n', lex.m_numLine);
    if (numRes >= 0) {
        strShortDescription = m_strBuffer.Mid(lex.m_numLine, numRes - lex.m_numLine - 1);
        numRes = strShortDescription.find_first_of('/');
        if (numRes > 0) {
            if (strShortDescription[numRes - 1] == '/') { // so this is a comment
                strShortDescription = strShortDescription.Mid(numRes + 1);
            }
        }
        else {
            numRes = strShortDescription.find_first_of(')');
            strShortDescription = strShortDescription.Left(numRes + 1);
        }
    }

    // get the function name
    const wxString& strFuncRealName = GETIdentifier(true);
    const wxString& strFuncName = stringUtils::MakeUpper(strFuncRealName);

    const int errorPlace = m_numCurrentCompile;

    CFunction* createdFunction = new CFunction(strFuncName, functionContext);

    createdFunction->m_strRealName = strFuncRealName;
    createdFunction->m_strShortDescription = strShortDescription;
    createdFunction->m_numLine = numLine;

    // compile the list of formal parameters + register them as local
    GETDelimeter('(');

    while (!IsNextDelimeter(')')) {

        // check for typing
        const wxString typeVar = IsTypeVar() ?
            GetTypeVar() : wxEmptyString;

        CParamVariable cVariable;
        if (IsNextKeyWord(KEY_VAL)) {
            GETKeyWord(KEY_VAL);
            cVariable.m_bByRef = true;
        }

        const wxString& strRealName = GETIdentifier(true);

        cVariable.m_strName = strRealName;
        cVariable.m_strType = typeVar;

        CVariable* foundedVar = nullptr;

        // register this variable as local
        if (functionContext->FindVariable(strRealName, foundedVar)) { // there was an announcement + repeated announcement = error
            SetError(ERROR_IDENTIFIER_DUPLICATE, strRealName);
            return false;
        }

        if (IsNextDelimeter('[')) { // this is an array
            GETDelimeter('[');
            GETDelimeter(']');
        }
        else if (IsNextDelimeter('=')) {
            GETDelimeter('=');
            cVariable.m_valData = FindConst(GETConstant());
        }

        functionContext->AddVariable(strRealName, typeVar);
        createdFunction->m_listParam.push_back(cVariable);
        if (!IsNextDelimeter(',') || IsNextDelimeter(')'))
            break;
        GETDelimeter(',');
    }
    GETDelimeter(')');
    if (IsNextKeyWord(KEY_EXPORT)) {
        GETKeyWord(KEY_EXPORT);
        createdFunction->m_bExport = true;
    }

    int numParent = 0;
    CCompileContext* pCurContext = context;
    while (pCurContext != nullptr) {
        numParent++;
        if (numParent > MAX_OBJECTS_LEVEL) {
            CSystemFunction::Message(pCurContext->m_compileModule->GetModuleName());
            if (numParent > 2 * MAX_OBJECTS_LEVEL) {
                CBackendException::Error("Recursive call of modules!");
            }
        }

        CFunction* foundedFunc = nullptr;
        if (pCurContext->FindFunction(strFuncName, foundedFunc)) { // found
            if (foundedFunc != createdFunction && foundedFunc->m_bExport) {
                m_numCurrentCompile = errorPlace;
                SetError(ERROR_DEF_FUNCTION, strFuncRealName);
                return false;
            }
        }

        pCurContext = pCurContext->m_parentContext;
    }

    context->m_listFunction[strFuncName] = createdFunction;

    // insert information about the function into the bytecode array:
    CByteUnit code0;
    AddLineInfo(code0);
    code0.m_numOper = OPER_FUNC;
#if defined(_LP64) || defined(__LP64__) || defined(__arch64__) || defined(_WIN64)
    code0.m_param1.m_numArray = reinterpret_cast<wxLongLong_t>(functionContext);
#else
    code0.m_param1.m_numArray = reinterpret_cast<int>(functionContext);
#endif
    m_cByteCode.m_listCode.push_back(code0);

    const long lAddres = createdFunction->m_nStart = m_cByteCode.m_listCode.size() - 1;

    m_cByteCode.m_listFunc[strFuncName] = {
        (long)createdFunction->m_listParam.size(),
        lAddres,
        functionContext->m_numReturn == RETURN_FUNCTION
    };

    if (createdFunction->m_bExport) {
        m_cByteCode.m_listExportFunc[strFuncName] = {
            (long)createdFunction->m_listParam.size(),
            lAddres,
            functionContext->m_numReturn == RETURN_FUNCTION
        };
    }

    for (unsigned int i = 0; i < createdFunction->m_listParam.size(); i++) {
        //add set oper
        CByteUnit code;
        AddLineInfo(code);
        if (createdFunction->m_listParam[i].m_valData.m_numArray == DEF_VAR_CONST) {
            code.m_numOper = OPER_SETCONST;// parameters are being passed
        }
        else {
            code.m_numOper = OPER_SET;// parameters are being passed
        }
        code.m_param1 = createdFunction->m_listParam[i].m_valData;
        code.m_param2.m_numIndex = createdFunction->m_listParam[i].m_bByRef;
        m_cByteCode.m_listCode.push_back(code);

        //Set type variable
        CParamUnit variable;
        variable.m_strType = createdFunction->m_listParam[i].m_strType;
        variable.m_numArray = 0;
        variable.m_numIndex = i;// index matches the number
        AddTypeSet(variable);
    }

    context->m_strCurFuncName = strFuncName;
    CompileBlock(functionContext);
    functionContext->DoLabels();
    context->m_strCurFuncName = wxEmptyString;

    if (functionContext->m_numReturn == RETURN_FUNCTION) {
        GETKeyWord(KEY_ENDFUNCTION);
    }
    else {
        GETKeyWord(KEY_ENDPROCEDURE);
    }

    CByteUnit code;
    AddLineInfo(code);
    code.m_numOper = OPER_ENDFUNC;
    m_cByteCode.m_listCode.push_back(code);

    createdFunction->m_nFinish = m_cByteCode.m_listCode.size() - 1;
    createdFunction->m_lVarCount = functionContext->m_listVariable.size();

    m_cByteCode.m_listCode[lAddres].m_param3.m_numIndex = createdFunction->m_lVarCount;// number of local variables
    m_cByteCode.m_listCode[lAddres].m_param3.m_numArray = createdFunction->m_listParam.size();//number of formal parameters

    functionContext->SetFunction(createdFunction);
    return true;
}

/**
 * record information about the type of variable
 */

void CCompileCode::AddTypeSet(const CParamUnit& variable)
{
    if (!variable.m_strType.IsEmpty()) {
        CByteUnit code;
        AddLineInfo(code);
        code.m_numOper = OPER_SET_TYPE;
        code.m_param1 = variable;
        code.m_param2.m_numArray = CValue::GetIDObjectFromString(variable.m_strType);
        m_cByteCode.m_listCode.push_back(code);
    }
}

// macro checking variable Var for type Str
#define CheckTypeDef(var,type) if(wxStrlen(type) > 0)\
	{\
		if(!stringUtils::CompareString(var.m_strType, type)){\
			if (CValue::CompareObjectName(type, eValueTypes::TYPE_BOOLEAN)) SetError(ERROR_BAD_TYPE_EXPRESSION_B);\
			else if (CValue::CompareObjectName(type, eValueTypes::TYPE_NUMBER)) SetError(ERROR_BAD_TYPE_EXPRESSION_N);\
			else if (CValue::CompareObjectName(type, eValueTypes::TYPE_STRING)) SetError(ERROR_BAD_TYPE_EXPRESSION_S);\
			else if (CValue::CompareObjectName(type, eValueTypes::TYPE_DATE)) SetError(ERROR_BAD_TYPE_EXPRESSION_D);\
			else SetError(ERROR_BAD_TYPE_EXPRESSION);\
		}\
		if (CValue::CompareObjectName(type, eValueTypes::TYPE_NUMBER)) code.m_numOper+=TYPE_DELTA1;\
		else if (CValue::CompareObjectName(type, eValueTypes::TYPE_STRING)) code.m_numOper+=TYPE_DELTA2;\
		else if (CValue::CompareObjectName(type, eValueTypes::TYPE_DATE)) code.m_numOper+=TYPE_DELTA3;\
        else if (CValue::CompareObjectName(type, eValueTypes::TYPE_BOOLEAN)) code.m_numOper+=TYPE_DELTA4;\
	}

// macro for adjusting the operation by variable type
// if it is typed, then the typed operation will be performed
#define CorrectTypeDef(sKey)\
if(!sKey.m_strType.IsEmpty())\
{\
	if (CValue::CompareObjectName(sKey.m_strType, eValueTypes::TYPE_NUMBER)) code.m_numOper+=TYPE_DELTA1;\
	else if (CValue::CompareObjectName(sKey.m_strType, eValueTypes::TYPE_STRING)) code.m_numOper+=TYPE_DELTA2;\
	else if (CValue::CompareObjectName(sKey.m_strType, eValueTypes::TYPE_DATE)) code.m_numOper+=TYPE_DELTA3;\
    else if (CValue::CompareObjectName(sKey.m_strType, eValueTypes::TYPE_BOOLEAN))  code.m_numOper+=TYPE_DELTA4;\
	else SetError(ERROR_BAD_TYPE_EXPRESSION);\
}

/**
 * CompileBlock
 * Purpose:
 * Creating object code for one block (a piece of code between any
 * operator brackets like LOOP...ENDDO, IF...ENDIF, etc.
 * nIterNumber - nested block number
 * Return value:
 * true,false
 */

bool CCompileCode::CompileBlock(CCompileContext* context)
{
    CLexem lex;
    while ((lex = PreviewGetLexem()).m_lexType != ERRORTYPE)
    {
        /*if (IDENTIFIER == lex.m_lexType && IsTypeVar(lex.m_strData))
        {
            CompileDeclaration();
        }
        else*/
        if (KEYWORD == lex.m_lexType)
        {
            switch (lex.m_numData)
            {
                case KEY_VAR: // setting variables and arrays
                    CompileDeclaration(context);
                    break;
                case KEY_NEW:
                    CompileNewObject(context);
                    break;
                case KEY_IF:
                    CompileIf(context);
                    break;
                case KEY_WHILE:
                    CompileWhile(context);
                    break;
                case KEY_FOREACH:
                    CompileForeach(context);
                    break;
                case KEY_FOR:
                    CompileFor(context);
                    break;
                case KEY_GOTO:
                    CompileGoto(context);
                    break;
                case KEY_RETURN:
                {
                    GETKeyWord(KEY_RETURN);

                    if (context->m_numReturn == RETURN_NONE) {
                        SetError(ERROR_USE_RETURN); // return operator cannot be used outside a procedure or function
                        return false;
                    }

                    CByteUnit code;
                    AddLineInfo(code);
                    code.m_numOper = OPER_RET;

                    if (context->m_numReturn == RETURN_FUNCTION) { // some value is returned

                        if (IsNextDelimeter(';')) {
                            SetError(ERROR_EXPRESSION_REQUIRE);
                            return false;
                        }

                        code.m_param1 = GetExpression(context);
                    }
                    else {
                        code.m_param1.m_numArray = DEF_VAR_NORET;
                        code.m_param1.m_numIndex = DEF_VAR_NORET;
                    }

                    m_cByteCode.m_listCode.push_back(code);
                    break;
                }
                case KEY_TRY:
                {
                    GETKeyWord(KEY_TRY);
                    CByteUnit code;
                    AddLineInfo(code);
                    code.m_numOper = OPER_TRY;
                    m_cByteCode.m_listCode.push_back(code);

                    const int lineTry = m_cByteCode.m_listCode.size() - 1;

                    CompileBlock(context);
                    code.m_numOper = OPER_ENDTRY;
                    m_cByteCode.m_listCode.push_back(code);

                    const int addrLine = m_cByteCode.m_listCode.size() - 1;

                    m_cByteCode.m_listCode[lineTry].m_param1.m_numIndex = m_cByteCode.m_listCode.size();

                    GETKeyWord(KEY_EXCEPT);
                    CompileBlock(context);
                    GETKeyWord(KEY_ENDTRY);

                    m_cByteCode.m_listCode[addrLine].m_param1.m_numIndex = m_cByteCode.m_listCode.size();
                    break;
                }
                case KEY_RAISE:
                {
                    GETKeyWord(KEY_RAISE);
                    CByteUnit code;
                    AddLineInfo(code);
                    if (IsNextDelimeter('(')) {
                        code.m_numOper = OPER_RAISE_T;
                        GETDelimeter('(');
                        code.m_param1 = GetExpression(context);
                        GETDelimeter(')');
                    }
                    else {
                        code.m_numOper = OPER_RAISE;
                    }
                    m_cByteCode.m_listCode.push_back(code);
                    break;
                }
                case KEY_CONTINUE:
                {
                    GETKeyWord(KEY_CONTINUE);
                    if (context->m_listContinue[context->m_numDoNumber]) {
                        CByteUnit code;
                        AddLineInfo(code);
                        code.m_numOper = OPER_GOTO;
                        m_cByteCode.m_listCode.push_back(code);
                        const int addrLine = m_cByteCode.m_listCode.size() - 1;
                        std::vector<int>* pList = context->m_listContinue[context->m_numDoNumber];
                        pList->push_back(addrLine);
                    }
                    else {
                        SetError(ERROR_USE_CONTINUE); // continue statement can only be used inside a loop
                        return false;
                    }
                    break;
                }
                case KEY_BREAK:
                {
                    GETKeyWord(KEY_BREAK);
                    if (context->m_listBreak[context->m_numDoNumber] != nullptr) {
                        CByteUnit code;
                        AddLineInfo(code);
                        code.m_numOper = OPER_GOTO;
                        m_cByteCode.m_listCode.push_back(code);
                        const int addrLine = m_cByteCode.m_listCode.size() - 1;
                        std::vector<int>* pList = context->m_listBreak[context->m_numDoNumber];
                        pList->push_back(addrLine);
                    }
                    else {
                        SetError(ERROR_USE_BREAK); // break operator can only be used inside a loop
                        return false;
                    }
                    break;
                }
                case KEY_FUNCTION:
                case KEY_PROCEDURE:
                {
                    GetLexem();
                    SetError(ERROR_USE_BLOCK);
                    return false;
                    break;
                }
                default:
                    return true;	// means the operator bracket ending this block has been encountered (for example, ENDIF, ENDDO, ENDFUNCTION, etc.)
            }
        }
        else
        {
            lex = GetLexem();

            if (IDENTIFIER == lex.m_lexType) {
                context->m_numTempVar = 0;
                if (IsNextDelimeter(':')) {// this is a label task encountered
                    unsigned int pLabel = context->m_listLabelDef[lex.m_strData];
                    if (pLabel > 0) {
                        SetError(ERROR_IDENTIFIER_DUPLICATE, lex.m_strData);// duplicate label definitions occurred
                        return false;
                    }
                    // write the address of the label:
                    context->m_listLabelDef[lex.m_strData] = m_cByteCode.m_listCode.size() - 1;
                    GETDelimeter(':');
                }
                else { //function and method calls, expression assignments are processed here

                    m_numCurrentCompile--;// step back

                    int numSet = 1;
                    if (m_onlyFunction && context == GetContext()) {
                        SetError(ERROR_ONLY_FUNCTION);
                        return false;
                    }

                    CParamUnit variable = GetCurrentIdentifier(context, numSet);//get the left side of the expression (before the '=' sign)
                    if (numSet) { //if there is a right side, i.e. the '=' sign
                        GETDelimeter('=');//this is an assignment of some expression to a variable
                        CParamUnit expression = GetExpression(context);
                        CByteUnit code;
                        code.m_numOper = OPER_LET;
                        AddLineInfo(code);

                        CheckTypeDef(expression, variable.m_strType);
                        variable.m_strType = expression.m_strType;

                        bool bShortLet = false; int n = 0;

                        if (DEF_VAR_TEMP == expression.m_numArray) { //reduce only temporary variables
                            n = m_cByteCode.m_listCode.size() - 1;
                            if (n >= 0) {
                                int nOperation = m_cByteCode.m_listCode[n].m_numOper % TYPE_DELTA1;
                                nOperation = nOperation % TYPE_DELTA1;
                                if (OPER_MULT == nOperation ||
                                    OPER_DIV == nOperation ||
                                    OPER_ADD == nOperation ||
                                    OPER_SUB == nOperation ||
                                    OPER_MOD == nOperation ||
                                    OPER_GT == nOperation ||
                                    OPER_GE == nOperation ||
                                    OPER_LS == nOperation ||
                                    OPER_LE == nOperation ||
                                    OPER_NE == nOperation ||
                                    OPER_EQ == nOperation
                                    )
                                {
                                    bShortLet = true;//shorten one assignment
                                }
                            }
                        }

                        if (bShortLet) {
                            m_cByteCode.m_listCode[n].m_param1 = variable;
                        }
                        else {
                            code.m_param1 = variable;
                            code.m_param2 = expression;
                            m_cByteCode.m_listCode.push_back(code);
                        }
                    }
                }
            }
            else {
                if (DELIMITER == lex.m_lexType
                    && ';' == lex.m_numData)
                {
                }
                else if (ENDPROGRAM == lex.m_lexType) {
                    break;
                }
                else {
                    SetError(ERROR_CODE);
                    return false;
                }
            }
        }
    }//while
    return true;
}//CompileBlock

bool CCompileCode::CompileNewObject(CCompileContext* context)
{
    GETKeyWord(KEY_NEW);

    wxString strClassName = GETIdentifier(true);
    const int numConst = GetConstString(strClassName);

    std::vector <CParamUnit> listParam;

    if (IsNextDelimeter('(')) { // this is a method call
        GETDelimeter('(');
        while (!IsNextDelimeter(')')) {
            if (IsNextDelimeter(',')) {
                CParamUnit data;
                data.m_numArray = DEF_VAR_SKIP;// missing parameter
                data.m_numIndex = DEF_VAR_SKIP;
                listParam.push_back(data);
            }
            else {
                listParam.emplace_back(GetExpression(context));
                if (!IsNextDelimeter(',') || IsNextDelimeter(')'))
                    break;
            }
            GETDelimeter(',');
        }
        GETDelimeter(')');
    }

    if (!CValue::IsRegisterCtor(strClassName, eCtorObjectType::eCtorObjectType_object_value)) {
        SetError(ERROR_CALL_CONSTRUCTOR, strClassName);
        return false;
    }

    CByteUnit code;
    AddLineInfo(code);

    code.m_numOper = OPER_NEW;
    code.m_param2.m_numIndex = numConst;//number of the called method from the list of encountered methods
    code.m_param2.m_numArray = listParam.size();// number of parameters

    CParamUnit variable = context->CreateVariable();
    code.m_param1 = variable;// variable into which the value is returned
    m_cByteCode.m_listCode.push_back(code);

    for (unsigned int arg = 0; arg < listParam.size(); arg++) {

        CByteUnit code;
        AddLineInfo(code);
        code.m_numOper = OPER_SET;
        code.m_param1 = listParam[arg];

        m_cByteCode.m_listCode.push_back(code);
    }

    return true;
}

/**
 * CompileGoto
 * Purpose:
 * Compiling the GOTO statement (determining the location of the jump label
 * for subsequent replacement with address and type = LABEL)
 * Return value:
 * true,false
 */

bool CCompileCode::CompileGoto(CCompileContext* context)
{
    GETKeyWord(KEY_GOTO);

    CLabel data;
    data.m_strName = GETIdentifier();
    data.m_numLine = m_cByteCode.m_listCode.size();//запоминаем те переходы, которые потом надо будет обработать
    data.m_numError = m_numCurrentCompile;

    context->m_listLabel.push_back(data);

    CByteUnit code;
    AddLineInfo(code);
    code.m_numOper = OPER_GOTO;
    m_cByteCode.m_listCode.push_back(code);

    return true;
}

/*
 * GetCurrentIdentifier
 * Purpose:
 * Compiling an identifier (defining its type as a variable, attribute or function, method)
 * numIsSet - at the input: 1 - a sign that an expression assignment may be expected (if the '=' sign is encountered)
 * Return value:
 * numIsSet - at the output: 1 - a sign that the assignment of the expression is exactly expected (i.e. the '=' sign must be encountered)
 * index number of the variable where the identifier value lies
*/

CParamUnit CCompileCode::GetCurrentIdentifier(CCompileContext* context, int& numIsSet)
{
    CParamUnit variable; int numPrevSet = numIsSet;

    const wxString& strRealName = GETIdentifier(true);
    const wxString& strName = stringUtils::MakeUpper(strRealName);

    if (IsNextDelimeter('(')) { // this is a function call
        CFunction* foundedFunc = nullptr;
        if (m_rootContext->FindFunction(strName, foundedFunc, true)) {
            const int numConst = GetConstString(strRealName);
            std::vector <CParamUnit> listParam;
            GETDelimeter('(');
            while (!IsNextDelimeter(')')) {
                if (IsNextDelimeter(',')) {
                    CParamUnit data;
                    data.m_numArray = DEF_VAR_SKIP; // missing parameter
                    data.m_numIndex = DEF_VAR_SKIP;
                    listParam.push_back(data);
                }
                else {
                    listParam.emplace_back(GetExpression(context));
                    if (!IsNextDelimeter(',') || IsNextDelimeter(')'))
                        break;
                }
                GETDelimeter(',');
            }
            GETDelimeter(')');
            if (!numIsSet && foundedFunc != nullptr &&
                foundedFunc->m_compileContext && foundedFunc->m_compileContext->m_numReturn != RETURN_FUNCTION) {
                SetError(ERROR_USE_PROCEDURE_AS_FUNCTION, foundedFunc->m_strRealName);
                return CParamUnit();
            }
            if (listParam.size() > foundedFunc->m_listParam.size()) {
                SetError(ERROR_MANY_PARAMS); // too many parameters
                return CParamUnit();
            }
            CByteUnit code;
            AddLineInfo(code);
            code.m_numOper = OPER_CALL_M;

            // variable on which the method is called
            code.m_param2 = context->GetVariable(foundedFunc->m_strContextVar, true, false, true);
            code.m_param3.m_numIndex = numConst;//number of the called method from the list of encountered methods
            code.m_param3.m_numArray = listParam.size();// number of parameters
            variable = context->CreateVariable();
            code.m_param1 = variable;// variable into which the value is returned
            m_cByteCode.m_listCode.push_back(code);

            for (unsigned int i = 0; i < listParam.size(); i++) {
                CByteUnit code;
                AddLineInfo(code);
                code.m_numOper = OPER_SET;
                code.m_param1 = listParam[i];
                m_cByteCode.m_listCode.push_back(code);
            }
        }
        else {
            variable = GetCallFunction(context, strRealName, numIsSet);
        }
        if (IsTypeVar(strRealName)) {
            variable.m_strType = GetTypeVar(strRealName);	// this is a type cast
        }
        numIsSet = 0;
    }
    else { //this is a variable call
        CVariable* foundedVar = nullptr; numIsSet = 1;
        if (m_rootContext->FindVariable(strRealName, foundedVar, true)) {
            CByteUnit code;
            AddLineInfo(code);
            const int numConst = GetConstString(strRealName);
            if (IsNextDelimeter('=') && numPrevSet == 1) {
                GETDelimeter('='); numIsSet = 0;
                code.m_numOper = OPER_SET_A;
                code.m_param1 = context->GetVariable(foundedVar->m_strContextVar, true, false, true);//variable for which the attribute is called
                code.m_param2.m_numIndex = numConst;//number of the called method from the list of encountered attributes and methods
                code.m_param3 = GetExpression(context);
                m_cByteCode.m_listCode.push_back(code);
                return variable;
            }
            else {
                code.m_numOper = OPER_GET_A;
                code.m_param2 = context->GetVariable(foundedVar->m_strContextVar, true, false, true);//variable for which the attribute is called
                code.m_param3.m_numIndex = numConst;//number of the attribute to be called from the list of attributes and methods encountered
                variable = context->CreateVariable();
                code.m_param1 = variable;// variable into which the value is returned
                m_cByteCode.m_listCode.push_back(code);
            }
        }
        else {
            bool bCheckError = !numPrevSet;
            if (IsNextDelimeter('.') || IsNextDelimeter('[')) //this variable contains a method call or array
                bCheckError = true;
            variable = context->GetVariable(strRealName, true, bCheckError);
        }
    }

loopLabel:

    if (IsNextDelimeter('[')) { // this is an array
        GETDelimeter('[');
        CParamUnit variableKey = GetExpression(context);
        GETDelimeter(']');
        //determine the call type (i.e. is it setting or getting an array value)
        //Example:
        //Arr[10]=12; - Set
        //ј=Arr[10]; - Get
        //Arr[10][2]=12; - Get,Set
        numIsSet = 0;
        if (IsNextDelimeter('[')) { //check the array variable type (multidimensional array support)
            CByteUnit code;
            AddLineInfo(code);
            code.m_numOper = OPER_CHECK_ARRAY;
            code.m_param1 = variable;//variable is an array
            code.m_param2 = variableKey;//array index
            m_cByteCode.m_listCode.push_back(code);
        }
        if (IsNextDelimeter('=') && numPrevSet == 1) {
            GETDelimeter('=');
            CByteUnit code;
            AddLineInfo(code);
            code.m_numOper = OPER_SET_ARRAY;
            code.m_param1 = variable;//variable is an array
            code.m_param2 = variableKey;//array index (more precisely, key since an associative array is used)
            code.m_param3 = GetExpression(context);

            CorrectTypeDef(variableKey);// check value type of index variable

            m_cByteCode.m_listCode.push_back(code);
            return variable;
        }
        else {
            CByteUnit code;
            AddLineInfo(code);
            code.m_numOper = OPER_GET_ARRAY;
            code.m_param2 = variable;//variable - array
            code.m_param3 = variableKey;//array index (more precisely key since associative array is used)
            variable = context->CreateVariable();
            code.m_param1 = variable;// variable into which the value is returned
            CorrectTypeDef(variableKey);// check value type индексной переменной
            m_cByteCode.m_listCode.push_back(code);
        }
        goto loopLabel;
    }

    if (IsNextDelimeter('.')) { // this is a method call или атрибута агрегатного объекта
        GETDelimeter('.');
        wxString strIdentifier = GETIdentifier(true);
        const int numConst = GetConstString(strIdentifier);
        if (IsNextDelimeter('(')) { // this is a method call
            std::vector <CParamUnit> listParam;
            GETDelimeter('(');
            while (!IsNextDelimeter(')')) {
                if (IsNextDelimeter(',')) {
                    CParamUnit data;
                    data.m_numArray = DEF_VAR_SKIP;// missing parameter
                    data.m_numIndex = DEF_VAR_SKIP;
                    listParam.push_back(data);
                }
                else {
                    listParam.emplace_back(GetExpression(context));
                    if (!IsNextDelimeter(',') || IsNextDelimeter(')'))
                        break;
                }
                GETDelimeter(',');
            }
            GETDelimeter(')');

            CByteUnit code;
            AddLineInfo(code);
            code.m_numOper = OPER_CALL_M;
            code.m_param2 = variable; // variable on which the method is called
            code.m_param3.m_numIndex = numConst;//number of the called method from the list of encountered methods
            code.m_param3.m_numArray = listParam.size();// number of parameters
            variable = context->CreateVariable();
            code.m_param1 = variable;// variable into which the value is returned
            m_cByteCode.m_listCode.push_back(code);
            for (unsigned int i = 0; i < listParam.size(); i++) {
                CByteUnit code;
                AddLineInfo(code);
                code.m_numOper = OPER_SET;
                code.m_param1 = listParam[i];
                m_cByteCode.m_listCode.push_back(code);
            }

            numIsSet = 0;
        }
        else { //otherwise - attribute call
            //define the call type (i.e. is it attribute setting or getting)
            //Example:
            //A=Cat.Product; - Get
            //Cat.Product=0; - Set
            //Cat.Product.Code=0; - Get,Set
            CByteUnit code;
            AddLineInfo(code);
            if (IsNextDelimeter('=') && numPrevSet == 1) {
                GETDelimeter('='); 	numIsSet = 0;
                code.m_numOper = OPER_SET_A;
                code.m_param1 = variable;//variable for which the attribute is called
                code.m_param2.m_numIndex = numConst;//number of the called method from the list of encountered attributes and methods
                code.m_param3 = GetExpression(context);
                m_cByteCode.m_listCode.push_back(code);
                return variable;
            }
            else {
                code.m_numOper = OPER_GET_A;
                code.m_param2 = variable;//variable for which the attribute is called
                code.m_param3.m_numIndex = numConst;//number of the called attribute from the list of encountered attributes and methods
                variable = context->CreateVariable();
                code.m_param1 = variable;// variable into which the value is returned
                m_cByteCode.m_listCode.push_back(code);
            }
        }
        goto loopLabel;
    }

    return variable;
}

bool CCompileCode::CompileIf(CCompileContext* context)
{
    std::vector <int> listAddrLine;

    GETKeyWord(KEY_IF);

    CByteUnit code;
    AddLineInfo(code);
    code.m_numOper = OPER_IF;

    CParamUnit variable = GetExpression(context);
    code.m_param1 = variable;
    CorrectTypeDef(variable);// check value type

    m_cByteCode.m_listCode.push_back(code);

    int nLastIFLine = m_cByteCode.m_listCode.size() - 1;

    GETKeyWord(KEY_THEN);
    CompileBlock(context);

    while (IsNextKeyWord(KEY_ELSEIF)) {

        // write the output from all checks for the previous block
        code.m_numOper = OPER_GOTO;
        m_cByteCode.m_listCode.push_back(code);
        listAddrLine.push_back(m_cByteCode.m_listCode.size() - 1);//the parameter for the GOTO operator will be known later

        //for the previous condition, set the jump address if the condition does not match
        m_cByteCode.m_listCode[nLastIFLine].m_param2.m_numIndex = m_cByteCode.m_listCode.size();

        GETKeyWord(KEY_ELSEIF);
        AddLineInfo(code);
        code.m_numOper = OPER_IF;

        variable = GetExpression(context);
        code.m_param1 = variable;
        CorrectTypeDef(variable);// check value type

        m_cByteCode.m_listCode.push_back(code);
        nLastIFLine = m_cByteCode.m_listCode.size() - 1;

        GETKeyWord(KEY_THEN);
        CompileBlock(context);
    }

    if (IsNextKeyWord(KEY_ELSE)) {
        // write the output from all checks for the previous block
        AddLineInfo(code);
        code.m_numOper = OPER_GOTO;
        m_cByteCode.m_listCode.push_back(code);
        listAddrLine.push_back(m_cByteCode.m_listCode.size() - 1);//the parameter for the GOTO operator will be known later

        //for the previous condition, set the jump address if the condition does not match
        m_cByteCode.m_listCode[nLastIFLine].m_param2.m_numIndex = m_cByteCode.m_listCode.size();
        nLastIFLine = 0;

        GETKeyWord(KEY_ELSE);
        CompileBlock(context);
    }

    GETKeyWord(KEY_ENDIF);

    const int numCurCompile = m_cByteCode.m_listCode.size();

    //for the last condition, set the jump address if the condition does not match
    m_cByteCode.m_listCode[nLastIFLine].m_param2.m_numIndex = numCurCompile;

    //Set the parameter for the GOTO operator - exit from all local conditions
    for (unsigned int i = 0; i < listAddrLine.size(); i++) {
        m_cByteCode.m_listCode[listAddrLine[i]].m_param1.m_numIndex = numCurCompile;
    }

    return true;
}

bool CCompileCode::CompileWhile(CCompileContext* context)
{
    context->StartDoList();

    const int nStartWhile = m_cByteCode.m_listCode.size();

    GETKeyWord(KEY_WHILE);
    CByteUnit code;
    AddLineInfo(code);
    code.m_numOper = OPER_IF;

    CParamUnit variable = GetExpression(context);
    code.m_param1 = variable;
    CorrectTypeDef(variable);// check value type

    const int numEndWhile = m_cByteCode.m_listCode.size();

    m_cByteCode.m_listCode.push_back(code);

    GETKeyWord(KEY_DO);
    CompileBlock(context);
    GETKeyWord(KEY_ENDDO);

    CByteUnit code2;
    AddLineInfo(code2);
    code2.m_numOper = OPER_GOTO;
    code2.m_param1.m_numIndex = nStartWhile;
    m_cByteCode.m_listCode.push_back(code2);

    m_cByteCode.m_listCode[numEndWhile].m_param2.m_numIndex = m_cByteCode.m_listCode.size();

    // remember the transition addresses for the Continue and Break commands
    context->FinishDoList(m_cByteCode, m_cByteCode.m_listCode.size() - 1, m_cByteCode.m_listCode.size());

    return true;
}

bool CCompileCode::CompileFor(CCompileContext* context)
{
    context->StartDoList();

    GETKeyWord(KEY_FOR);

    const wxString& strRealName = GETIdentifier(true);
    const wxString& strName = stringUtils::MakeUpper(strRealName);

    CParamUnit variable = context->GetVariable(strRealName);

    // check variable type
    if (!variable.m_strType.IsEmpty()) {
        if (!CValue::CompareObjectName(variable.m_strType, eValueTypes::TYPE_NUMBER)) {
            SetError(ERROR_NUMBER_TYPE);
            return false;
        }
    }

    GETDelimeter('=');
    CParamUnit variable2 = GetExpression(context);

    CByteUnit code0;
    AddLineInfo(code0);
    code0.m_numOper = OPER_LET;
    code0.m_param1 = variable;
    code0.m_param2 = variable2;
    m_cByteCode.m_listCode.push_back(code0);

    // check value type
    if (!variable.m_strType.IsEmpty()) {
        if (!CValue::CompareObjectName(variable2.m_strType, eValueTypes::TYPE_NUMBER)) {
            SetError(ERROR_BAD_TYPE_EXPRESSION);
            return false;
        }
    }

    GETKeyWord(KEY_TO);

    CParamUnit variableTo =
        context->GetVariable(strName + wxT("@to"), true, false, false, true); //loop variable

    CByteUnit code1;
    AddLineInfo(code1);
    code1.m_numOper = OPER_LET;
    code1.m_param1 = variableTo;
    code1.m_param2 = GetExpression(context);
    m_cByteCode.m_listCode.push_back(code1);

    CByteUnit code;
    AddLineInfo(code);
    code.m_numOper = OPER_FOR;
    code.m_param1 = variable;
    code.m_param2 = variableTo;
    m_cByteCode.m_listCode.push_back(code);

    const int nStartFOR = m_cByteCode.m_listCode.size() - 1;

    GETKeyWord(KEY_DO);
    CompileBlock(context);
    GETKeyWord(KEY_ENDDO);

    CByteUnit code2;
    AddLineInfo(code2);
    code2.m_numOper = OPER_NEXT;
    code2.m_param1 = variable;
    code2.m_param2.m_numIndex = nStartFOR;
    m_cByteCode.m_listCode.push_back(code2);

    m_cByteCode.m_listCode[nStartFOR].m_param3.m_numIndex = m_cByteCode.m_listCode.size();

    // remember the transition addresses for the Continue and Break commands
    context->FinishDoList(m_cByteCode, m_cByteCode.m_listCode.size() - 1, m_cByteCode.m_listCode.size());

    return true;
}

bool CCompileCode::CompileForeach(CCompileContext* context)
{
    context->StartDoList();

    GETKeyWord(KEY_FOREACH);

    wxString strRealName = GETIdentifier(true);
    wxString strName = stringUtils::MakeUpper(strRealName);

    CParamUnit variable = context->GetVariable(strRealName);

    GETKeyWord(KEY_IN);

    CParamUnit variableIn =
        context->GetVariable(strName + wxT("@in_"), true, false, false, true); //loop variable

    CByteUnit code1;
    AddLineInfo(code1);
    code1.m_numOper = OPER_LET;
    code1.m_param1 = variableIn;
    code1.m_param2 = GetExpression(context);
    m_cByteCode.m_listCode.push_back(code1);

    CParamUnit variableIt =
        context->GetVariable(strName + wxT("@it_"), true, false, false, true);  //storage iterpos;

    CByteUnit code;
    AddLineInfo(code);
    code.m_numOper = OPER_FOREACH;
    code.m_param1 = variable;
    code.m_param2 = variableIn;
    code.m_param3 = variableIt; // for storage iterpos;

    m_cByteCode.m_listCode.push_back(code);

    const int numStartFOREACH = m_cByteCode.m_listCode.size() - 1;

    GETKeyWord(KEY_DO);
    CompileBlock(context);
    GETKeyWord(KEY_ENDDO);

    CByteUnit code2;
    AddLineInfo(code2);
    code2.m_numOper = OPER_NEXT_ITER;
    code2.m_param1 = variableIt; // for storage iterpos;
    code2.m_param2.m_numIndex = numStartFOREACH;
    m_cByteCode.m_listCode.push_back(code2);

    m_cByteCode.m_listCode[numStartFOREACH].m_param4.m_numIndex = m_cByteCode.m_listCode.size();

    // remember the transition addresses for the Continue and Break commands
    context->FinishDoList(m_cByteCode, m_cByteCode.m_listCode.size() - 1, m_cByteCode.m_listCode.size());
    return true;
}

/**
 * processing a function or procedure call
 */

CParamUnit CCompileCode::GetCallFunction(CCompileContext* context, const wxString& strRealName, const int& numIsSet)
{
    std::shared_ptr<CCallFunction> callFunc(new CCallFunction);

    callFunc->m_strName = stringUtils::MakeUpper(strRealName);
    callFunc->m_strRealName = strRealName;
    callFunc->m_numError = m_numCurrentCompile;// to display messages when errors occur

    GETDelimeter('(');

    while (!IsNextDelimeter(')')) {
        if (IsNextDelimeter(',')) {
            CParamUnit data;
            data.m_numArray = DEF_VAR_SKIP;// missing parameter
            data.m_numIndex = DEF_VAR_SKIP;
            callFunc->m_listParam.push_back(data);
        }
        else {
            callFunc->m_listParam.emplace_back(GetExpression(context));
            if (!IsNextDelimeter(',') || IsNextDelimeter(')'))
                break;
        }
        GETDelimeter(',');
    }

    GETDelimeter(')');

    CByteUnit code;
    AddLineInfo(code);

    callFunc->m_numString = code.m_numString;
    callFunc->m_numLine = code.m_numLine;
    callFunc->m_strModuleName = code.m_strModuleName;
    callFunc->m_puRetValue = context->CreateVariable();

    callFunc->m_numIsSet = numIsSet;

    CFunction* foundedFunc = nullptr;

    if (m_bExpressionOnly) foundedFunc = GetFunction(callFunc->m_strName);
    else context->FindFunction(callFunc->m_strName, foundedFunc);

    if (foundedFunc != nullptr && context->m_strCurFuncName != callFunc->m_strName) {
        if (!PushCallFunction(callFunc))
            return CParamUnit();
    }
    else {
        if (m_bExpressionOnly) {
            SetError(ERROR_CALL_FUNCTION, strRealName);
            return CParamUnit();
        }
        code.m_numOper = OPER_GOTO;// jump to the end of the bytecode where the expanded call will be made
        m_cByteCode.m_listCode.push_back(code);
        callFunc->m_numAddLine = m_cByteCode.m_listCode.size() - 1;
        m_listCallFunc.push_back(callFunc);
    }

    return callFunc->m_puRetValue;
}

/**
 * gets the constant number from a unique list of values
 * (if such a value is not in the list, it is created)
 */

CParamUnit CCompileCode::FindConst(const CValue& constData)
{
    const wxString& strConstant =
        wxString::Format(wxT("%d:%s"), constData.GetType(), constData.GetString());

    CParamUnit variable;
    variable.m_numArray = DEF_VAR_CONST;

    if (m_listHashConst.find(strConstant) != m_listHashConst.end()) {
        variable.m_numIndex = m_listHashConst.at(strConstant) - 1;
    }
    else {
        variable.m_numIndex = m_cByteCode.m_listConst.size();
        m_cByteCode.m_listConst.push_back(constData);
        m_listHashConst.insert_or_assign(strConstant, variable.m_numIndex + 1);
    }
    variable.m_strType = GetTypeVar(constData.GetClassName());
    return variable;
}

#define SetOper(x)	code.m_numOper=x;

/**
 * compiling an arbitrary expression (service calls from the function itself)
 */

CParamUnit CCompileCode::GetExpression(CCompileContext* context, int nPriority)
{
    CParamUnit variable;
    CLexem lex = GETLexem();

    // first we process Left operators
    if ((lex.m_lexType == KEYWORD && lex.m_numData == KEY_NOT) || (lex.m_lexType == DELIMITER && lex.m_numData == '!')) {

        variable = context->CreateVariable();
        variable.m_strType = CValue::GetNameObjectFromVT(eValueTypes::TYPE_BOOLEAN, true);

        AddTypeSet(variable);

        CParamUnit variable2 = GetExpression(context);// , gs_operPriority['!']);

        CByteUnit code;
        code.m_numOper = OPER_NOT;
        AddLineInfo(code);

        if (!variable2.m_strType.IsEmpty()) { 
            CheckTypeDef(variable2, CValue::GetNameObjectFromVT(eValueTypes::TYPE_BOOLEAN)); 
        }
        
        code.m_param1 = variable;
        code.m_param2 = variable2;

        m_cByteCode.m_listCode.push_back(code);
    }
    else if ((lex.m_lexType == KEYWORD && lex.m_numData == KEY_NEW))
    {
        const wxString strObjectName = GETIdentifier(true);
        const int numConst = GetConstString(strObjectName);

        std::vector <CParamUnit> listParam;

        if (IsNextDelimeter('(')) { // this is a method call
            GETDelimeter('(');
            while (!IsNextDelimeter(')')) {
                if (IsNextDelimeter(','))
                {
                    CParamUnit data;
                    data.m_numArray = DEF_VAR_SKIP;// missing parameter
                    data.m_numIndex = DEF_VAR_SKIP;
                    listParam.push_back(data);
                }
                else
                {
                    listParam.emplace_back(GetExpression(context));
                    if (!IsNextDelimeter(',') || IsNextDelimeter(')'))
                        break;
                }
                GETDelimeter(',');
            }
            GETDelimeter(')');
        }

        if (lex.m_numData == KEY_NEW && !CValue::IsRegisterCtor(strObjectName, eCtorObjectType::eCtorObjectType_object_value)) {
            SetError(ERROR_CALL_CONSTRUCTOR, strObjectName);
            return CParamUnit();
        }

        CByteUnit code;
        AddLineInfo(code);
        code.m_numOper = OPER_NEW;

        code.m_param2.m_numIndex = numConst;//number of the called method from the list of encountered methods
        code.m_param2.m_numArray = listParam.size();// number of parameters

        variable = context->CreateVariable();
        code.m_param1 = variable;// variable into which the value is returned
        m_cByteCode.m_listCode.push_back(code);

        for (unsigned int arg = 0; arg < listParam.size(); arg++) {
            CByteUnit code;
            AddLineInfo(code);
            code.m_numOper = OPER_SET;
            code.m_param1 = listParam[arg];
            m_cByteCode.m_listCode.push_back(code);
        }
    }
    else if (lex.m_lexType == DELIMITER && lex.m_numData == '(')
    {
        variable = GetExpression(context);
        GETDelimeter(')');
    }
    else if (lex.m_lexType == DELIMITER && lex.m_numData == '?')
    {
        variable = context->CreateVariable();
        CByteUnit code;
        AddLineInfo(code);
        code.m_numOper = OPER_ITER;
        code.m_param1 = variable;
        GETDelimeter('(');
        code.m_param2 = GetExpression(context);
        GETDelimeter(',');
        code.m_param3 = GetExpression(context);
        GETDelimeter(',');
        code.m_param4 = GetExpression(context);
        GETDelimeter(')');
        m_cByteCode.m_listCode.push_back(code);
    }
    else if (lex.m_lexType == IDENTIFIER)
    {
        m_numCurrentCompile--;// step back
        int numSet = 0;
        variable = GetCurrentIdentifier(context, numSet);
    }
    else if (lex.m_lexType == CONSTANT)
    {
        variable = FindConst(lex.m_valData);
    }
    else if ((lex.m_lexType == DELIMITER && lex.m_numData == '+') || (lex.m_lexType == DELIMITER && lex.m_numData == '-'))
    {
        // check the admissibility of such assignment
        const int numCurPriority = gs_operPriority[lex.m_numData];

        if (nPriority >= numCurPriority) {
            SetError(ERROR_EXPRESSION);// сompare the priorities of the left (previous operation) and the currently running operation
            return CParamUnit();
        }

        // this is a user-defined expression sign
        if (lex.m_numData == '+')// do nothing (ignore)
        {
            CByteUnit code;
            variable = GetExpression(context, nPriority);
            if (!variable.m_strType.IsEmpty()) {
                CheckTypeDef(variable, CValue::GetNameObjectFromVT(eValueTypes::TYPE_NUMBER));
            }
            variable.m_strType = CValue::GetNameObjectFromVT(eValueTypes::TYPE_NUMBER, true);
            return variable;
        }
        else
        {
            variable = GetExpression(context, 100);//super high priority!
            CByteUnit code;
            AddLineInfo(code);
            code.m_numOper = OPER_INVERT;

            if (!variable.m_strType.IsEmpty()) {
                CheckTypeDef(variable, CValue::GetNameObjectFromVT(eValueTypes::TYPE_NUMBER));
            }

            code.m_param2 = variable;
            variable = context->CreateVariable();
            variable.m_strType = CValue::GetNameObjectFromVT(eValueTypes::TYPE_NUMBER, true);
            code.m_param1 = variable;
            m_cByteCode.m_listCode.push_back(code);
        }
    }
    else
    {
        m_numCurrentCompile--;
        SetError(ERROR_EXPRESSION);
        return CParamUnit();
    }

    // now we process Right Operators
    // so in variable we have the first index of the expression variable

delimOperation:

    lex = PreviewGetLexem();

    if (lex.m_lexType == DELIMITER && lex.m_numData == ')')
        return variable;

    // we look to see if there are any further operators for performing actions on this variable
    if ((lex.m_lexType == DELIMITER && lex.m_numData != ';') || (lex.m_lexType == KEYWORD && lex.m_numData == KEY_AND) || (lex.m_lexType == KEYWORD && lex.m_numData == KEY_OR))
    {
        if (lex.m_numData >= 0 && lex.m_numData <= 255)
        {
            const int numCurPriority = gs_operPriority[lex.m_numData];
            if (nPriority < numCurPriority) { // сompare the priorities of the left (previous operation) and the currently running operation
                CByteUnit code;
                AddLineInfo(code);
                lex = GetLexem();
                if (lex.m_numData == '*') {
                    SetOper(OPER_MULT);
                }
                else if (lex.m_numData == '/') {
                    SetOper(OPER_DIV);
                }
                else if (lex.m_numData == '+') {
                    SetOper(OPER_ADD);
                }
                else if (lex.m_numData == '-') {
                    SetOper(OPER_SUB);
                }
                else if (lex.m_numData == '%') {
                    SetOper(OPER_MOD);
                }
                else if (lex.m_numData == KEY_AND) {
                    SetOper(OPER_AND);
                }
                else if (lex.m_numData == KEY_OR) {
                    SetOper(OPER_OR);
                }
                else if (lex.m_numData == '>') {
                    SetOper(OPER_GT);
                    if (IsNextDelimeter('=')) {
                        GETDelimeter('=');
                        SetOper(OPER_GE);
                    }
                }
                else if (lex.m_numData == '<') {
                    SetOper(OPER_LS);
                    if (IsNextDelimeter('=')) {
                        GETDelimeter('=');
                        SetOper(OPER_LE);
                    }
                    else if (IsNextDelimeter('>')) {
                        GETDelimeter('>');
                        SetOper(OPER_NE);
                    }
                }
                else if (lex.m_numData == '=') {
                    SetOper(OPER_EQ);
                }
                else {
                    SetError(ERROR_EXPRESSION);
                    return CParamUnit();
                }

                CParamUnit puVariable1 = context->CreateVariable();
                CParamUnit puVariable2 = variable;
                CParamUnit puVariable3 = GetExpression(context, numCurPriority);

                if (puVariable3.m_numArray != DEF_VAR_TEMP && puVariable3.m_numArray != DEF_VAR_CONST) { // extra. checking for prohibited operations
                    if (CValue::CompareObjectName(puVariable2.m_strType, eValueTypes::TYPE_STRING)) {
                        if (OPER_DIV == code.m_numOper
                            || OPER_MOD == code.m_numOper
                            || OPER_MULT == code.m_numOper
                            || OPER_AND == code.m_numOper
                            || OPER_OR == code.m_numOper) {
                            SetError(ERROR_TYPE_OPERATION);
                            return CParamUnit();
                        }
                    }
                }

                if (puVariable2.m_numArray != DEF_VAR_CONST && puVariable2.m_numArray != DEF_VAR_TEMP) { // constants are not checked - because they are typified by default
                    CheckTypeDef(puVariable3, puVariable2.m_strType);
                }

                puVariable1.m_strType = puVariable2.m_strType;

                if (code.m_numOper >= OPER_GT && code.m_numOper <= OPER_NE) {
                    puVariable1.m_strType = CValue::GetNameObjectFromVT(eValueTypes::TYPE_BOOLEAN, true);
                }

                code.m_param1 = puVariable1;
                code.m_param2 = puVariable2;
                code.m_param3 = puVariable3;

                m_cByteCode.m_listCode.push_back(code);

                variable = puVariable1;
                goto delimOperation;
            }
        }
    }

    return variable;
}

void CCompileCode::SetParent(CCompileCode* setParent)
{
    m_cByteCode.m_parent = nullptr;

    m_parent = setParent;
    m_rootContext->m_parentContext = nullptr;

    if (m_parent != nullptr) {
        m_cByteCode.m_parent = &m_parent->m_cByteCode;
        m_rootContext->m_parentContext = m_parent->m_rootContext;
    }

    OnSetParent(setParent);
}

#pragma warning(pop)