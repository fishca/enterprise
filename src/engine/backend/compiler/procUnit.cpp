////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, 2Ñ-team
//	Description : Processor unit 
////////////////////////////////////////////////////////////////////////////

#include "compileCode.h"
#include "procUnit.h"

#include "debugger/debugServer.h"
#include "system/systemManager.h"

#define curCode	m_pByteCode->m_listCode[lCodeLine]

#define index1	curCode.m_param1.m_numIndex
#define index2	curCode.m_param2.m_numIndex
#define index3	curCode.m_param3.m_numIndex
#define index4	curCode.m_param4.m_numIndex

#define array1	curCode.m_param1.m_numArray
#define array2	curCode.m_param2.m_numArray
#define array3	curCode.m_param3.m_numArray
#define array4	curCode.m_param4.m_numArray

#define locVariable1 *m_pRefLocVars[index1]
#define locVariable2 *m_pRefLocVars[index2]
#define locVariable3 *m_pRefLocVars[index3]
#define locVariable4 *m_pRefLocVars[index4]

#define variable(x) (array##x<=0?*pRefLocVars[index##x]:(array##x==DEF_VAR_CONST?m_pByteCode->m_listConst[index##x]:*m_pppArrayList[array##x+(bDelta?1:0)][index##x]))

#define variable1 variable(1)
#define variable2 variable(2)
#define variable3 variable(3)
#define variable4 variable(4)

//**************************************************************************************************************
//*                                              support error place                                           *
//**************************************************************************************************************

CProcUnit* CProcUnit::m_currentRunModule = nullptr;

struct CErrorPlace {

	bool IsEmpty() const { return m_errorLine == wxNOT_FOUND; }
	void Reset() { m_byteCode = m_skipByteCode = nullptr; m_errorLine = wxNOT_FOUND; }

	long m_errorLine = wxNOT_FOUND;
	CByteCode* m_byteCode = nullptr;
	CByteCode* m_skipByteCode = nullptr;
};

static CErrorPlace s_errorPlace;

void CProcUnit::Raise() {
	s_errorPlace.Reset(); //initialize the error place
	s_errorPlace.m_skipByteCode = CProcUnit::GetCurrentByteCode(); //return back to the called module (if any)
}

std::vector <CRunContext*> CProcUnit::ms_runContext;

//**************************************************************************************************************
//*                                              stack support                                                 *
//**************************************************************************************************************

static short s_nRecCount = 0; //control looping

inline void BeginByteCode(CRunContext* pCode) { CProcUnit::AddRunContext(pCode); }
inline bool EndByteCode()
{
	unsigned int n = CProcUnit::GetCountRunContext();
	if (n > 0)
		CProcUnit::BackRunContext();
	n--;
	if (n <= 0)
		return false;
	return true;
}

//Stack reset
inline void ResetByteCode() { while (EndByteCode()); }

struct CProcStackGuard {

	CProcStackGuard(CRunContext* runContext) {
		if (s_nRecCount > MAX_REC_COUNT) { //critical error
			wxString strError;
			for (unsigned int i = 0; i < CProcUnit::GetCountRunContext(); i++) {
				CRunContext* pLastContext = CProcUnit::GetRunContext(i);
				wxASSERT(pLastContext);
				CByteCode* m_pByteCode = pLastContext->GetByteCode();
				wxASSERT(m_pByteCode);
				strError += wxString::Format("\n%s (#line %d)",
					m_pByteCode->m_strModuleName,
					m_pByteCode->m_listCode[pLastContext->m_lCurLine].m_numLine + 1
				);
			}
			CBackendException::Error(_("Number of recursive calls exceeded the maximum allowed value!\nCall stack :") + strError);
		}
		s_nRecCount++;
		m_currentContext = runContext;
		BeginByteCode(runContext);
	}

	~CProcStackGuard() { s_nRecCount--; EndByteCode(); }

private:
	CRunContext* m_currentContext;
};

//**************************************************************************************************************
//*                                              inline functions                                              *
//**************************************************************************************************************

//checking variable availability
#define CHECK_READONLY(Operation)\
if(cValue1.m_bReadOnly)\
{\
 CValue cVal;\
 Operation(cVal,cValue2,cValue3);\
 cValue1.SetValue(cVal);\
 return;\
}\
if(cValue1.m_typeClass==eValueTypes::TYPE_REFFER)\
 cValue1.m_pRef->DecrRef();\

//Functions for quickly working with the CValue type
inline void AddValue(CValue& cValue1, const CValue& cValue2, const CValue& cValue3)
{
	CHECK_READONLY(AddValue);
	cValue1.m_typeClass = cValue2.GetType();
	if (cValue1.m_typeClass == eValueTypes::TYPE_NUMBER) {
		cValue1.m_fData = cValue2.GetNumber() + cValue3.GetNumber();
	}
	else if (cValue1.m_typeClass == eValueTypes::TYPE_DATE) {
		if (cValue3.m_typeClass == eValueTypes::TYPE_DATE) { //date + date -> number
			cValue1.m_typeClass = eValueTypes::TYPE_NUMBER;
			cValue1.m_fData = cValue2.GetDate() + cValue3.GetDate();
		}
		else {
			cValue1.m_dData = cValue2.m_dData + cValue3.GetDate();
		}
	}
	else {
		cValue1.m_typeClass = eValueTypes::TYPE_STRING;
		cValue1.m_sData = cValue2.GetString() + cValue3.GetString();
	}
}

inline void SubValue(CValue& cValue1, const CValue& cValue2, const CValue& cValue3)
{
	CHECK_READONLY(SubValue);
	cValue1.m_typeClass = cValue2.GetType();
	if (cValue1.m_typeClass == eValueTypes::TYPE_NUMBER) {
		cValue1.m_fData = cValue2.GetNumber() - cValue3.GetNumber();
	}
	else if (cValue1.m_typeClass == eValueTypes::TYPE_DATE) {
		if (cValue3.m_typeClass == eValueTypes::TYPE_DATE) { //date - date -> number
			cValue1.m_typeClass = eValueTypes::TYPE_NUMBER;
			cValue1.m_fData = cValue2.GetDate() - cValue3.GetDate();
		}
		else {
			cValue1.m_dData = cValue2.m_dData - cValue3.GetDate();
		}
	}
	else {
		CBackendException::Error("Subtraction operation cannot be applied for this type (%s)", cValue2.GetClassName());
	}
}

inline void MultValue(CValue& cValue1, const CValue& cValue2, const CValue& cValue3)
{
	CHECK_READONLY(MultValue);
	cValue1.m_typeClass = cValue2.GetType();
	if (cValue1.m_typeClass == eValueTypes::TYPE_NUMBER) {
		cValue1.m_fData = cValue2.GetNumber() * cValue3.GetNumber();
	}
	else if (cValue1.m_typeClass == eValueTypes::TYPE_DATE) {
		if (cValue3.m_typeClass == eValueTypes::TYPE_DATE) { //date * date -> number
			cValue1.m_typeClass = eValueTypes::TYPE_NUMBER;
			cValue1.m_fData = cValue2.GetDate() * cValue3.GetDate();
		}
		else {
			cValue1.m_dData = cValue2.m_dData * cValue3.GetDate();
		}
	}
	else {
		CBackendException::Error("Multiplication operation cannot be applied for this type (%s)", cValue2.GetClassName());
	}
}

inline void DivValue(CValue& cValue1, const CValue& cValue2, const CValue& cValue3)
{
	CHECK_READONLY(DivValue);
	cValue1.m_typeClass = cValue2.GetType();
	if (cValue1.m_typeClass == eValueTypes::TYPE_NUMBER) {
		const number_t& flNumber3 = cValue3.GetNumber();
		if (flNumber3.IsZero())
			CBackendException::Error("Divide by zero");
		cValue1.m_fData = cValue2.GetNumber() / flNumber3;
	}
	else {
		CBackendException::Error("Division operation cannot be applied for this type (%s)", cValue2.GetClassName());
	};
}

inline void ModValue(CValue& cValue1, const CValue& cValue2, const CValue& cValue3)
{
	CHECK_READONLY(ModValue);
	cValue1.m_typeClass = cValue2.GetType();
	if (cValue1.m_typeClass == eValueTypes::TYPE_NUMBER) {
		ttmath::Int<TTMATH_BITS(128)> val128_2, val128_3;
		const number_t& flNumber3 = cValue3.GetNumber(); flNumber3.ToInt(val128_3);
		if (val128_3.IsZero())
			CBackendException::Error("Divide by zero");
		const number_t& flNumber2 = cValue2.GetNumber(); flNumber2.ToInt(val128_2);
		cValue1.m_fData = val128_2 % val128_3;
	}
	else {
		CBackendException::Error("Modulo operation cannot be applied for this type (%s)", cValue2.GetClassName());
	}
}

//Implementation of comparison operators
inline void CompareValueGT(CValue& cValue1, const CValue& cValue2, const CValue& cValue3)
{
	CHECK_READONLY(CompareValueGT);
	cValue1.m_typeClass = eValueTypes::TYPE_BOOLEAN;
	cValue1.m_bData = cValue2.CompareValueGT(cValue3);
}

inline void CompareValueGE(CValue& cValue1, const CValue& cValue2, const CValue& cValue3)
{
	CHECK_READONLY(CompareValueGT);
	cValue1.m_typeClass = eValueTypes::TYPE_BOOLEAN;
	cValue1.m_bData = cValue2.CompareValueGT(cValue3);
}

inline void CompareValueLS(CValue& cValue1, const CValue& cValue2, const CValue& cValue3)
{
	CHECK_READONLY(CompareValueLS);
	cValue1.m_typeClass = eValueTypes::TYPE_BOOLEAN;
	cValue1.m_bData = cValue2.CompareValueLS(cValue3);
}

inline void CompareValueLE(CValue& cValue1, const CValue& cValue2, const CValue& cValue3)
{
	CHECK_READONLY(CompareValueLE);
	cValue1.m_typeClass = eValueTypes::TYPE_BOOLEAN;
	cValue1.m_bData = cValue2.CompareValueLE(cValue3);
}

inline void CompareValueEQ(CValue& cValue1, const CValue& cValue2, const CValue& cValue3)
{
	CHECK_READONLY(CompareValueEQ);
	cValue1.m_typeClass = eValueTypes::TYPE_BOOLEAN;
	cValue1.m_bData = cValue2.CompareValueEQ(cValue3);
}

inline void CompareValueNE(CValue& cValue1, const CValue& cValue2, const CValue& cValue3)
{
	CHECK_READONLY(CompareValueNE);
	cValue1.m_typeClass = eValueTypes::TYPE_BOOLEAN;
	cValue1.m_bData = cValue2.CompareValueNE(cValue3);
}

inline void CopyValue(CValue& cValue1, CValue& cValue2)
{
	if (&cValue1 == &cValue2)
		return;

	//checking variable availability and checking references
	if (cValue1.m_bReadOnly) {
		cValue1.SetValue(cValue2);
		return;
	}
	else {//Reset
		if (cValue1.m_pRef && cValue1.m_typeClass == eValueTypes::TYPE_REFFER)
			cValue1.m_pRef->DecrRef();

		cValue1.m_typeClass = eValueTypes::TYPE_EMPTY;
		cValue1.m_sData = wxEmptyString;

		cValue1.m_pRef = nullptr;
	}

	if (cValue2.m_typeClass == eValueTypes::TYPE_REFFER) {
		cValue1 = cValue2.GetValue();
		return;
	}

	cValue1.m_typeClass = cValue2.m_typeClass;

	switch (cValue2.m_typeClass)
	{
	case eValueTypes::TYPE_NULL:
		break;
	case eValueTypes::TYPE_BOOLEAN:
		cValue1.m_bData = cValue2.m_bData;
		break;
	case eValueTypes::TYPE_NUMBER:
		cValue1.m_fData = cValue2.m_fData;
		break;
	case eValueTypes::TYPE_STRING:
		cValue1.m_sData = cValue2.m_sData;
		break;
	case eValueTypes::TYPE_DATE:
		cValue1.m_dData = cValue2.m_dData;
		break;
	case eValueTypes::TYPE_REFFER:
		cValue1.m_pRef = cValue2.m_pRef; cValue1.m_pRef->IncrRef();
		break;
	case eValueTypes::TYPE_OLE:
	case eValueTypes::TYPE_ENUM:
	case eValueTypes::TYPE_VALUE:
		cValue1.m_typeClass = eValueTypes::TYPE_REFFER;
		cValue1.m_pRef = &cValue2; cValue1.m_pRef->IncrRef();
		break;
	default: cValue1.m_typeClass = eValueTypes::TYPE_EMPTY;
	}
}

inline void MoveValue(CValue&& cValue1, CValue&& cValue2)
{
	if (&cValue1 == &cValue2)
		return;

	cValue1.m_typeClass = cValue2.m_typeClass;

	switch (cValue2.m_typeClass)
	{
	case eValueTypes::TYPE_NULL:
		break;
	case eValueTypes::TYPE_BOOLEAN:
		cValue1.m_bData = std::move(cValue2.m_bData);
		break;
	case eValueTypes::TYPE_NUMBER:
		cValue1.m_fData = std::move(cValue2.m_fData);
		break;
	case eValueTypes::TYPE_STRING:
		cValue1.m_sData = std::move(cValue2.m_sData);
		break;
	case eValueTypes::TYPE_DATE:
		cValue1.m_dData = std::move(cValue2.m_dData);
		break;
	case eValueTypes::TYPE_REFFER:
		cValue1.m_pRef = cValue2.m_pRef;
		cValue1.m_pRef->IncrRef();
		break;
	case eValueTypes::TYPE_OLE:
	case eValueTypes::TYPE_ENUM:
	case eValueTypes::TYPE_VALUE:
		cValue1.m_typeClass = eValueTypes::TYPE_REFFER;
		cValue1.m_pRef = &cValue2;
		cValue1.m_pRef->IncrRef();
		break;
	default: cValue1.m_typeClass = eValueTypes::TYPE_EMPTY;
	}

	cValue2.Reset();
}

inline bool IsEmptyValue(const CValue& cValue1)
{
	return cValue1.IsEmpty();
}

#define IsHasValue(cValue1) (!IsEmptyValue(cValue1))

inline void SetTypeBoolean(CValue& cValue1, bool bValue)
{
	//check variable availability and reference check
	if (cValue1.m_bReadOnly) {
		cValue1.SetValue(bValue);
		return;
	}
	cValue1.Reset();
	cValue1.m_typeClass = eValueTypes::TYPE_BOOLEAN;
	cValue1.m_bData = bValue;
}

inline void SetTypeNumber(CValue& cValue1, const number_t& fValue)
{
	//check variable availability and reference check
	if (cValue1.m_bReadOnly) {
		cValue1.SetValue(fValue);
		return;
	}
	cValue1.Reset();
	cValue1.m_typeClass = eValueTypes::TYPE_NUMBER;
	cValue1.m_fData = fValue;
}

#define CheckAndError(variable, name)\
{\
 if(variable.m_typeClass!=eValueTypes::TYPE_REFFER)\
 CBackendException::Error("No attribute or method found '%s' - a variable is not an aggregate object", name);\
 else\
 CBackendException::Error("Aggregate object field not found '%s'", name);\
}

//Index arrays
inline bool SetArrayValue(CValue& cValue1, const CValue& cValue2, CValue& cValue3)
{
	return cValue1.SetAt(cValue2, cValue3);
}

inline bool GetArrayValue(CValue& cValue1, CValue& cValue2, const CValue& cValue3)
{
	CValue retValue;
	if (cValue2.GetAt(cValue3, retValue)) {
		CopyValue(cValue1, retValue);
		return true;
	}
	return false;
}

inline CValue GetValue(CValue& cValue1)
{
	if (cValue1.m_bReadOnly
		&& cValue1.m_typeClass != eValueTypes::TYPE_REFFER) {
		CValue cVal;
		CopyValue(cVal, cValue1);
		return cVal;
	}
	return cValue1;
}

#pragma region iterator_support
class CValueIterator : public CValue {
	wxDECLARE_DYNAMIC_CLASS(CValueIterator);
public:
	CValueIterator(CValue& ownerValue = CValue()) : CValue(eValueTypes::TYPE_VALUE),
		m_ownerValue(ownerValue), m_currentPos(0) {
		ResetIterator();
	}
	virtual ~CValueIterator() { Reset(); }
	bool GetCurrentValue(CValue& pvarParamValue) const {
		if (m_currentPos >= m_ownerValue.GetIteratorCount())
			return false;
		CopyValue(pvarParamValue, m_ownerValue.GetIteratorAt(m_currentPos));
		return true;
	}
	bool NextIterator() {
		if (m_currentPos >= m_ownerValue.GetIteratorCount())
			return false;
		m_currentPos++;
		return true;
	};
	void ResetIterator() { m_currentPos = 0; };
protected:
	unsigned long m_currentPos = 0;
private:
	CValue& m_ownerValue;
};

//**************************************************************************************************************
wxIMPLEMENT_DYNAMIC_CLASS(CValueIterator, CValue);
//**************************************************************************************************************

const class_identifier_t g_valueIterator = string_to_clsid("SO_ITER");
#pragma endregion

//////////////////////////////////////////////////////////////////////
//						Construction/Destruction                    //
//////////////////////////////////////////////////////////////////////

void CProcUnit::Execute(CRunContext* pContext, CValue* pvarRetValue, bool bDelta)
{
	struct CTryLabel {

		CTryLabel() :m_lStartLine(0), m_lEndLine(0) {}
		CTryLabel(const long& lStartLine, const long& lEndLine) :m_lStartLine(lStartLine), m_lEndLine(lEndLine) {}

		long m_lStartLine, m_lEndLine;
	};

#ifdef DEBUG
	if (pContext == nullptr) {
		CBackendException::Error("No execution context defined!");
		if (m_pByteCode == nullptr)
			CBackendException::Error("No execution code set!");
	}
#endif

	pContext->SetProcUnit(this);

	CProcStackGuard stackGuard(pContext);

	CValue* pLocVars = pContext->m_pLocVars;
	CValue** pRefLocVars = pContext->m_pRefLocVars;

	CByteUnit* pCodeList = m_pByteCode->m_listCode.data();

	long lCodeLine = pContext->m_lStart;
	long lFinish = m_pByteCode->m_listCode.size();
	long lPrevLine = wxNOT_FOUND;

	std::vector<CTryLabel> tryList;

start_label:

	try { //slower by 2-3% for each nested module
		while (lCodeLine < lFinish) {

			if (!CBackendException::IsEvalMode()) {
				pContext->m_lCurLine = lCodeLine;
				m_currentRunModule = this;
			}

			//enter in debugger
			if (debugServer != nullptr && debugServer->IsDestroySignal())
				break;
			else if (debugServer != nullptr && !CBackendException::IsEvalMode())
				debugServer->EnterDebugger(pContext, curCode, lPrevLine);

			switch (curCode.m_numOper)
			{
			case OPER_CONST: CopyValue(variable1, m_pByteCode->m_listConst[index2]); break;
			case OPER_CONSTN: SetTypeNumber(variable1, index2); break;
			case OPER_ADD: AddValue(variable1, variable2, variable3); break;
			case OPER_SUB: SubValue(variable1, variable2, variable3); break;
			case OPER_DIV: DivValue(variable1, variable2, variable3); break;
			case OPER_MOD: ModValue(variable1, variable2, variable3); break;
			case OPER_MULT: MultValue(variable1, variable2, variable3); break;
			case OPER_LET: CopyValue(variable1, variable2); break;
			case OPER_INVERT: SetTypeNumber(variable1, -variable2.GetNumber()); break;
			case OPER_NOT: SetTypeBoolean(variable1, IsEmptyValue(variable2)); break;
			case OPER_AND: if (IsHasValue(variable2) && IsHasValue(variable3))
				SetTypeBoolean(variable1, true); else SetTypeBoolean(variable1, false);
				break;
			case OPER_OR:
				if (IsHasValue(variable2) || IsHasValue(variable3))
					SetTypeBoolean(variable1, true);
				else SetTypeBoolean(variable1, false); break;
			case OPER_EQ: CompareValueEQ(variable1, variable2, variable3); break;
			case OPER_NE: CompareValueNE(variable1, variable2, variable3); break;
			case OPER_GT: CompareValueGT(variable1, variable2, variable3); break;
			case OPER_LS: CompareValueLS(variable1, variable2, variable3); break;
			case OPER_GE: CompareValueGE(variable1, variable2, variable3); break;
			case OPER_LE: CompareValueLE(variable1, variable2, variable3); break;
			case OPER_IF:
				if (IsEmptyValue(variable1))
					lCodeLine = index2 - 1;
				break;
			case OPER_FOR:
				if (variable1.m_typeClass != eValueTypes::TYPE_NUMBER)
					CBackendException::Error("Only variables with type can be used to organize the loop \"number\"");
				if (variable1.m_fData == variable2.m_fData)
					lCodeLine = index3 - 1;
				break;
			case OPER_FOREACH:
			{
				if (!variable2.HasIterator())
					CBackendException::Error("Undefined value iterator");
				if (g_valueIterator != variable3.GetClassType())
					CopyValue(variable3, CValue(new CValueIterator(variable2)));
				CValueIterator* iterator = variable3.ConvertToType<CValueIterator>();
				if (!iterator->GetCurrentValue(variable1)) {
					variable3.Reset(); lCodeLine = index4 - 1;
				}
			} break;
			case OPER_NEXT:
			{
				if (variable1.m_typeClass == eValueTypes::TYPE_NUMBER)
					variable1.m_fData++;
				lCodeLine = index2 - 1;
			} break;
			case OPER_NEXT_ITER:
			{
				CValueIterator* value_iterator =
					variable1.ConvertToType<CValueIterator>();
				value_iterator->NextIterator();
				lCodeLine = index2 - 1;
			} break;
			case OPER_ITER:
			{
				if (IsHasValue(variable2))
					CopyValue(variable1, variable3);
				else
					CopyValue(variable1, variable4);
			}  break;
			case OPER_NEW:
			{
				CValue* pRetValue = &variable1;
				CRunContextSmall cRunContext(array2);
				cRunContext.m_lParamCount = array2;
				const wxString className = m_pByteCode->m_listConst[index2].m_sData;
				//load parameters
				for (long i = 0; i < cRunContext.m_lParamCount; i++) {
					lCodeLine++;
					if (index1 >= 0) {
						if (variable1.m_bReadOnly && variable1.m_typeClass != eValueTypes::TYPE_REFFER) {
							CopyValue(cRunContext.m_pLocVars[i], variable1);
						}
						else {
							cRunContext.m_pRefLocVars[i] = &variable1;
						}
					}
				}
				CopyValue(*pRetValue, CValue::CreateObject(className, cRunContext.m_lParamCount > 0 ? cRunContext.m_pRefLocVars : nullptr, cRunContext.m_lParamCount));
			} break;
			case OPER_SET_A:
			{//setting attribute
				const wxString& strPropName = m_pByteCode->m_listConst[index2].m_sData;
				const long lPropNum = variable1.FindProp(strPropName);
				if (lPropNum < 0) CheckAndError(variable1, strPropName);
				if (!variable1.IsPropWritable(lPropNum)) CBackendException::Error("Object field not writable (%s)", strPropName);
				variable1.SetPropVal(lPropNum, GetValue(variable3));
			} break;
			case OPER_GET_A://get attribute
			{
				CValue* pRetValue = &variable1;
				CValue* pVariable2 = &variable2;
				const wxString& strPropName = m_pByteCode->m_listConst[index3].m_sData;
				const long lPropNum = variable2.FindProp(strPropName);
				if (lPropNum < 0) CheckAndError(variable2, strPropName);
				if (!variable2.IsPropReadable(lPropNum)) CBackendException::Error("Object field not readable (%s)", strPropName);
				CValue vRet; bool result = variable2.GetPropVal(lPropNum, vRet);
				if (result && vRet.m_typeClass == eValueTypes::TYPE_REFFER)
					*pRetValue = vRet;
				else if (result)
					CopyValue(*pRetValue, vRet);
				break;
			}
			case OPER_CALL_M://method call
			{
				CValue* pRetValue = &variable1;
				CValue* pVariable2 = &variable2;

				const wxString& funcName = m_pByteCode->m_listConst[index3].m_sData;
				long lMethodNum = wxNOT_FOUND;
				//call optimization
				CValue* storageValue = reinterpret_cast<CValue*>(array4);
				if (storageValue && storageValue == pVariable2->GetRef()) { //previously there were calls
					lMethodNum = index4;
#ifdef DEBUG
					lMethodNum = pVariable2->FindMethod(funcName);
					if (lMethodNum != index4) CBackendException::Error("Error value %d must %d (It is recommended to turn off method optimization)", index4, lMethodNum);
#endif
				}
				else {//there were no calls
					lMethodNum = pVariable2->FindMethod(funcName);
					index4 = lMethodNum;
					array4 = reinterpret_cast<wxLongLong_t>(pVariable2->GetRef());
				}

				if (lMethodNum < 0)
					CheckAndError(variable2, funcName);

				CRunContextSmall cRunContext(std::max(array3, MAX_STATIC_VAR));
				cRunContext.m_lParamCount = array3;

				// too many parameters
				const long paramCount = pVariable2->GetNParams(lMethodNum);

				if (paramCount < cRunContext.m_lParamCount)
					CBackendException::Error(ERROR_MANY_PARAMS, funcName, funcName);
				else if (paramCount == wxNOT_FOUND && cRunContext.m_lParamCount == 0)
					CBackendException::Error(ERROR_MANY_PARAMS, funcName, funcName);

				//load parameters
				for (long i = 0; i < cRunContext.m_lParamCount; i++) {
					lCodeLine++;
					if (index1 >= 0 && !pVariable2->GetParamDefValue(lMethodNum, i, *cRunContext.m_pRefLocVars[i])) {
						if (variable1.m_bReadOnly && variable1.m_typeClass != eValueTypes::TYPE_REFFER) {
							CopyValue(cRunContext.m_pLocVars[i], variable1);
						}
						else {
							cRunContext.m_pRefLocVars[i] = &variable1;
						}
					}
				}

				if (pVariable2->HasRetVal(lMethodNum)) {
					pVariable2->CallAsFunc(lMethodNum, *pRetValue, cRunContext.m_pRefLocVars, cRunContext.m_lParamCount);
				}
				else {
					// operator =
					if (m_pByteCode->m_listCode[lCodeLine + 1].m_numOper == OPER_LET)
						CBackendException::Error(ERROR_USE_PROCEDURE_AS_FUNCTION, funcName, funcName);
					pVariable2->CallAsProc(lMethodNum, cRunContext.m_pRefLocVars, cRunContext.m_lParamCount);
				} break;
			}
			case OPER_CALL:
			{ //call a regular function
				const long lModuleNumber = array2;
				CRunContext cRunContext(index3);
				cRunContext.m_lStart = index2;
				cRunContext.m_lParamCount = array3;
				CByteCode* pLocalByteCode = m_ppArrayCode[lModuleNumber]->m_pByteCode;
				cRunContext.m_compileContext = reinterpret_cast<CCompileContext*>(pLocalByteCode->m_listCode[cRunContext.m_lStart].m_param1.m_numArray);
				CValue* pRetValue = &variable1;
				//load parameters
				for (long i = 0; i < cRunContext.m_lParamCount; i++) {
					lCodeLine++;
					if (curCode.m_numOper == OPER_SETCONST) {
						CopyValue(cRunContext.m_pLocVars[i], pLocalByteCode->m_listConst[index1]);
					}
					else {
						if (variable1.m_bReadOnly || index2 == 1) {//pass parameter by value
							CopyValue(cRunContext.m_pLocVars[i], variable1);
						}
						else {
							cRunContext.m_pRefLocVars[i] = &variable1;
						}
					}
				}
				m_ppArrayCode[lModuleNumber]->Execute(&cRunContext, pRetValue, false);
				break;
			}
			case OPER_SET_ARRAY:
				if (!SetArrayValue(variable1, variable2, GetValue(variable3)))
					CBackendException::Error(_("Cannot set array value '%s'"), variable3.GetString());
				break; //setting the array value
			case OPER_GET_ARRAY:
				if (!GetArrayValue(variable1, variable2, variable3))
					CBackendException::Error(_("Cannot get array value '%s'"), variable3.GetString());
				break; //getting the array value
			case OPER_GOTO: case OPER_ENDTRY:
			{
				const long tryCodeLine = index1;
				const long trySize = tryList.size() - 1;
				if (trySize >= 0) {
					if (tryCodeLine >= tryList[trySize].m_lEndLine ||
						tryCodeLine <= tryList[trySize].m_lStartLine) {
						tryList.resize(trySize);//exit from try..catch scope
					}
				}
				lCodeLine = tryCodeLine - 1;//since we'll add 1 later
			} break;
			case OPER_TRY:
				tryList.emplace_back(lCodeLine, index1);
				break; //transition on error
			case OPER_RAISE: CBackendException::Error(CBackendException::GetLastError()); break;
			case OPER_RAISE_T: CBackendException::Error(m_pByteCode->m_listConst[index1].GetString()); break;
			case OPER_RET:
				if (index1 != DEF_VAR_NORET) {
					if (pvarRetValue == nullptr)
						CBackendException::Error(_("Cannot set return value in procedure!"));
					CopyValue(*pvarRetValue, variable1);
				}
			case OPER_ENDFUNC:
			case OPER_END:
				lCodeLine = lFinish;
				break; //exit
			case OPER_FUNC: if (bDelta) {
				while (lCodeLine < lFinish) {
					if (curCode.m_numOper != OPER_ENDFUNC) {
						lCodeLine++;
					}
					else break;
				}
			} break; //this is the initial run - skip the bodies of procedures and functions
			case OPER_SET_TYPE:
				variable1.SetType(CValue::GetVTByID(array2));
				break;
				//Operators for working with typed data
				//NUMBER
			case OPER_ADD + TYPE_DELTA1: variable1.m_fData = variable2.m_fData + variable3.m_fData; break;
			case OPER_SUB + TYPE_DELTA1: variable1.m_fData = variable2.m_fData - variable3.m_fData; break;
			case OPER_DIV + TYPE_DELTA1: if (variable3.m_fData.IsZero()) { CBackendException::Error("Divide by zero"); } variable1.m_fData = variable2.m_fData / variable3.m_fData; break;
			case OPER_MOD + TYPE_DELTA1: if (variable3.m_fData.IsZero()) { CBackendException::Error("Divide by zero"); } variable1.m_fData = variable2.m_fData.Round() % variable3.m_fData.Round(); break;
			case OPER_MULT + TYPE_DELTA1: variable1.m_fData = variable2.m_fData * variable3.m_fData; break;
			case OPER_LET + TYPE_DELTA1: variable1.m_fData = variable2.m_fData; break;
			case OPER_NOT + TYPE_DELTA1: variable1.m_fData = variable2.m_fData.IsZero(); break;
			case OPER_INVERT + TYPE_DELTA1: variable1.m_fData = -variable2.m_fData; break;
			case OPER_EQ + TYPE_DELTA1: variable1.m_fData = (variable2.m_fData == variable3.m_fData); break;
			case OPER_NE + TYPE_DELTA1: variable1.m_fData = (variable2.m_fData != variable3.m_fData); break;
			case OPER_GT + TYPE_DELTA1: variable1.m_fData = (variable2.m_fData > variable3.m_fData); break;
			case OPER_LS + TYPE_DELTA1: variable1.m_fData = (variable2.m_fData < variable3.m_fData); break;
			case OPER_GE + TYPE_DELTA1: variable1.m_fData = (variable2.m_fData >= variable3.m_fData); break;
			case OPER_LE + TYPE_DELTA1: variable1.m_fData = (variable2.m_fData <= variable3.m_fData); break;
			case OPER_SET_ARRAY + TYPE_DELTA1:
				if (!SetArrayValue(variable1, variable2, GetValue(variable3)))
					CBackendException::Error(_("Cannot set array value '%s'"), variable3.GetString());
				break;//set array value
			case OPER_GET_ARRAY + TYPE_DELTA1:
				if (!GetArrayValue(variable1, variable2, variable3))
					CBackendException::Error(_("Cannot get array value '%s'"), variable3.GetString());
				break; //getting the array value
			case OPER_IF + TYPE_DELTA1: if (variable1.m_fData.IsZero()) lCodeLine = index2 - 1; break;
				//STRING
			case OPER_ADD + TYPE_DELTA2: variable1.m_sData = variable2.m_sData + variable3.m_sData; break;
			case OPER_LET + TYPE_DELTA2: variable1.m_sData = variable2.m_sData; break;
			case OPER_SET_ARRAY + TYPE_DELTA2:
				if (!SetArrayValue(variable1, variable2, GetValue(variable3)))
					CBackendException::Error(_("Cannot set array value '%s'"), variable3.GetString());
				break; //set array value
			case OPER_GET_ARRAY + TYPE_DELTA2:
				if (!GetArrayValue(variable1, variable2, variable3))
					CBackendException::Error(_("Cannot get array value '%s'"), variable3.GetString());
				break; //getting the array value
			case OPER_IF + TYPE_DELTA2: if (variable1.m_sData.IsEmpty()) lCodeLine = index2 - 1; break;
				//DATE
			case OPER_ADD + TYPE_DELTA3: variable1.m_dData = variable2.m_dData + variable3.m_dData; break;
			case OPER_SUB + TYPE_DELTA3: variable1.m_dData = variable2.m_dData - variable3.m_dData; break;
			case OPER_DIV + TYPE_DELTA3: if (variable3.m_dData == 0) { CBackendException::Error("Divide by zero"); } variable1.m_dData = variable2.m_dData / variable3.GetInteger(); break;
			case OPER_MOD + TYPE_DELTA3: if (variable3.m_dData == 0) { CBackendException::Error("Divide by zero"); } variable1.m_dData = (int)variable2.m_dData % variable3.GetInteger(); break;
			case OPER_MULT + TYPE_DELTA3: variable1.m_dData = variable2.m_dData * variable3.m_dData; break;
			case OPER_LET + TYPE_DELTA3: variable1.m_dData = variable2.m_dData; break;
			case OPER_NOT + TYPE_DELTA3: variable1.m_dData = ~variable2.m_dData; break;
			case OPER_INVERT + TYPE_DELTA3: variable1.m_dData = -variable2.m_dData; break;
			case OPER_EQ + TYPE_DELTA3: variable1.m_dData = (variable2.m_dData == variable3.m_dData); break;
			case OPER_NE + TYPE_DELTA3: variable1.m_dData = (variable2.m_dData != variable3.m_dData); break;
			case OPER_GT + TYPE_DELTA3: variable1.m_dData = (variable2.m_dData > variable3.m_dData); break;
			case OPER_LS + TYPE_DELTA3: variable1.m_dData = (variable2.m_dData < variable3.m_dData); break;
			case OPER_GE + TYPE_DELTA3: variable1.m_dData = (variable2.m_dData >= variable3.m_dData); break;
			case OPER_LE + TYPE_DELTA3: variable1.m_dData = (variable2.m_dData <= variable3.m_dData); break;
			case OPER_SET_ARRAY + TYPE_DELTA3:
				if (!SetArrayValue(variable1, variable2, GetValue(variable3)))
					CBackendException::Error(_("Cannot set array value '%s'"), variable3.GetString());
				break; //setting the array value
			case OPER_GET_ARRAY + TYPE_DELTA3:
				if (!GetArrayValue(variable1, variable2, variable3))
					CBackendException::Error(_("Cannot get array value '%s'"), variable3.GetString());
				break; //getting the array value
			case OPER_IF + TYPE_DELTA3: if (!variable1.m_dData) lCodeLine = index2 - 1; break;
				//BOOLEAN
			case OPER_ADD + TYPE_DELTA4: variable1.m_bData = variable2.m_bData + variable3.m_bData; break;
			case OPER_LET + TYPE_DELTA4: variable1.m_bData = variable2.m_bData; break;
			case OPER_NOT + TYPE_DELTA4: variable1.m_bData = !variable2.m_bData; break;
			case OPER_INVERT + TYPE_DELTA4: variable1.m_bData = !variable2.m_bData; break;
			case OPER_EQ + TYPE_DELTA4: variable1.m_bData = (variable2.m_bData == variable3.m_bData); break;
			case OPER_NE + TYPE_DELTA4: variable1.m_bData = (variable2.m_bData != variable3.m_bData); break;
			case OPER_GT + TYPE_DELTA4: variable1.m_bData = (variable2.m_bData > variable3.m_bData); break;
			case OPER_LS + TYPE_DELTA4: variable1.m_bData = (variable2.m_bData < variable3.m_bData); break;
			case OPER_GE + TYPE_DELTA4: variable1.m_bData = (variable2.m_bData >= variable3.m_bData); break;
			case OPER_LE + TYPE_DELTA4: variable1.m_bData = (variable2.m_bData <= variable3.m_bData); break;
			case OPER_IF + TYPE_DELTA4: if (!variable1.m_bData) lCodeLine = index2 - 1; break;
			}
			lCodeLine++;
		}
	}
	catch (const CBackendInterrupt* err) {
		CSystemFunction::Message(err->what(),
			eStatusMessage::eStatusMessage_Error
		);
		while (lCodeLine < lFinish) {
			if (curCode.m_numOper != OPER_GOTO
				&& curCode.m_numOper != OPER_NEXT
				&& curCode.m_numOper != OPER_NEXT_ITER) {
				lCodeLine++;
			}
			else {
				lCodeLine++;
				goto start_label;
				break;
			}
		}
	}
	catch (const CBackendException* err) {
		const long trySize = tryList.size() - 1;
		if (trySize >= 0) {
			s_errorPlace.Reset(); //Error is handled in this module - erase the error location
			const long tryCodeLine = tryList[trySize].m_lEndLine;
			tryList.resize(trySize);
			lCodeLine = tryCodeLine;
			goto start_label;
		}
		//there is no handler in this module - save the error location for the following modules
		//But we don't throw an error right away, because we don't know if there are any handlers further
		if (!s_errorPlace.m_byteCode) {
			if (m_pByteCode != s_errorPlace.m_skipByteCode) { //the Error system function throws an exception only for child modules
				//previously saved the original error location (i.e. the error didn't occur in this module)
				s_errorPlace.m_byteCode = m_pByteCode;
				s_errorPlace.m_errorLine = lCodeLine;
			}
		}
		CBackendException::ProcessError(m_pByteCode->m_listCode[lCodeLine], err->what());
	}
}

//nRunModule parameters:
//false-do not run
//true-run
void CProcUnit::Execute(CByteCode& cByteCode, CValue* pvarRetValue, bool bRunModule)
{
	Reset();

	if (!cByteCode.m_bCompile)
		CBackendException::Error("Module: %s not compiled!", cByteCode.m_strModuleName);

	s_nRecCount = 0;
	m_pByteCode = &cByteCode;

	//check the conformity of modules (compiled and running)
	if (GetParent() && GetParent()->m_pByteCode != m_pByteCode->m_parent) {
		m_pByteCode = nullptr;
		CBackendException::Error("System error - compilation failed (#1)\nModule:%s\nParent1:%s\nParent2:%s",
			cByteCode.m_strModuleName,
			cByteCode.m_parent->m_strModuleName,
			GetParent()->m_pByteCode->m_strModuleName
		);
	}
	else if (!GetParent() && m_pByteCode->m_parent) {
		m_pByteCode = nullptr;
		CBackendException::Error("System error - compilation failed (#2)\nModule1:%s\nParent1:%s",
			cByteCode.m_strModuleName,
			cByteCode.m_parent->m_strModuleName
		);
	}

	m_cCurContext.SetLocalCount(cByteCode.m_lVarCount);
	m_cCurContext.m_lStart = cByteCode.m_lStartModule;

	unsigned int nParentCount = GetParentCount();

	m_ppArrayCode = new CProcUnit * [nParentCount + 1];
	m_ppArrayCode[0] = this;

	m_pppArrayList = new CValue * *[nParentCount + 2];
	m_pppArrayList[0] = m_cCurContext.m_pRefLocVars;
	m_pppArrayList[1] = m_cCurContext.m_pRefLocVars;//start with 1, because 0 means local context

	for (unsigned int i = 0; i < nParentCount; i++) {
		CProcUnit* pCurUnit = GetParent(i);
		m_ppArrayCode[i + 1] = pCurUnit;
		m_pppArrayList[i + 2] = pCurUnit->m_cCurContext.m_pRefLocVars;
	}

	//support for external variables
	for (unsigned int i = 0; i < cByteCode.m_listExternValue.size(); i++) {
		if (cByteCode.m_listExternValue[i]) {
			m_cCurContext.m_pRefLocVars[i] = cByteCode.m_listExternValue[i];
		}
	}

	bool bDelta = true;

	//Initial initialization of module variables
	unsigned int lFinish = m_pByteCode->m_listCode.size();
	CValue** pRefLocVars = m_cCurContext.m_pRefLocVars;

	for (unsigned int lCodeLine = 0; lCodeLine < lFinish; lCodeLine++) {
		CByteUnit& byte = m_pByteCode->m_listCode[lCodeLine];
		if (byte.m_numOper == OPER_SET_TYPE) {
			variable1.SetType(CValue::GetVTByID(array2));
		}
	}

	//Disable writing constants
	for (unsigned int i = 0; i < m_pByteCode->m_listConst.size(); i++) {
		m_pByteCode->m_listConst[i].m_bReadOnly = true;
	}

	if (bRunModule) {
		m_cCurContext.m_compileContext = cByteCode.m_compileModule->m_rootContext;
		Execute(&m_cCurContext, pvarRetValue, bDelta);
	}
}

//Search for a function in a module by name
//bExportOnly=0-search for any functions in the current module + exported ones in parent modules
//bExportOnly=1-search for exported functions in the current and parent modules
//bExportOnly=2-search for exported functions in the current module only
long CProcUnit::FindMethod(const wxString& strMethodName, bool bError, int bExportOnly) const
{
	if (m_pByteCode == nullptr ||
		!m_pByteCode->m_bCompile) {
		CBackendException::Error(_("Module not compiled!"));
	}

	long lCodeLine = bExportOnly ?
		m_pByteCode->FindExportMethod(strMethodName) :
		m_pByteCode->FindMethod(strMethodName);

	if (bError && lCodeLine < 0)
		CBackendException::Error("Procedure or function \"%s\" not found!", strMethodName);

	if (lCodeLine >= 0) {
		return lCodeLine;
	}
	if (GetParent() &&
		bExportOnly <= 1) {
		unsigned int nCodeSize = m_pByteCode->m_listCode.size();
		lCodeLine = GetParent()->FindMethod(strMethodName, false, 1);
		if (lCodeLine >= 0) {
			return nCodeSize + lCodeLine;
		}
	}
	return wxNOT_FOUND;
}

long CProcUnit::FindFunction(const wxString& strMethodName, bool bError, int bExportOnly) const
{
	if (m_pByteCode == nullptr ||
		!m_pByteCode->m_bCompile) {
		CBackendException::Error(_("Module not compiled!"));
	}

	long lCodeLine = bExportOnly ?
		m_pByteCode->FindExportFunction(strMethodName) :
		m_pByteCode->FindFunction(strMethodName);

	if (bError && lCodeLine < 0)
		CBackendException::Error("Function \"%s\" not found!", strMethodName);

	if (lCodeLine >= 0)
		return lCodeLine;

	if (GetParent() &&
		bExportOnly <= 1) {
		unsigned int nCodeSize = m_pByteCode->m_listCode.size();
		lCodeLine = GetParent()->FindFunction(strMethodName, false, 1);
		if (lCodeLine >= 0) {
			return nCodeSize + lCodeLine;
		}
	}
	return wxNOT_FOUND;
}

long CProcUnit::FindProcedure(const wxString& strMethodName, bool bError, int bExportOnly) const
{
	if (m_pByteCode == nullptr ||
		!m_pByteCode->m_bCompile) {
		CBackendException::Error(_("Module not compiled!"));
	}

	long lCodeLine = bExportOnly ?
		m_pByteCode->FindExportProcedure(strMethodName) :
		m_pByteCode->FindProcedure(strMethodName);

	if (bError && lCodeLine < 0)
		CBackendException::Error(_("Procedure \"%s\" not found!"), strMethodName);

	if (lCodeLine >= 0) {
		return lCodeLine;
	}
	if (GetParent() &&
		bExportOnly <= 1) {
		unsigned int nCodeSize = m_pByteCode->m_listCode.size();
		lCodeLine = GetParent()->FindProcedure(strMethodName, false, 1);
		if (lCodeLine >= 0) {
			return nCodeSize + lCodeLine;
		}
	}
	return wxNOT_FOUND;
}

//Calling a procedure by name
//The call is made only in the current module
bool CProcUnit::CallAsProc(const wxString& funcName, CValue** ppParams, const long lSizeArray)
{
	if (m_pByteCode != nullptr) {
		const long lCodeLine = m_pByteCode->FindMethod(funcName);
		if (lCodeLine != wxNOT_FOUND) {
			CallAsProc(lCodeLine, ppParams, lSizeArray);
			return true;
		}
	}
	return false;
}

//Calling a function by name
//The call is made only in the current module
bool CProcUnit::CallAsFunc(const wxString& funcName, CValue& pvarRetValue, CValue** ppParams, const long lSizeArray)
{
	if (m_pByteCode != nullptr) {
		const long lCodeLine = m_pByteCode->FindMethod(funcName);
		if (lCodeLine != wxNOT_FOUND) {
			CallAsFunc(lCodeLine, pvarRetValue, ppParams, lSizeArray);
			return true;
		}
	}
	return false;
}

//Calling a procedure by its address in the byte code array
//The call is made incl. and in the parent module
void CProcUnit::CallAsProc(const long lCodeLine, CValue** ppParams, const long lSizeArray)
{
	if (m_pByteCode == nullptr || !m_pByteCode->m_bCompile)
		CBackendException::Error(_("Module not compiled!"));

	const long lCodeSize = m_pByteCode->m_listCode.size();
	if (lCodeLine >= lCodeSize) {
		if (!GetParent())
			CBackendException::Error(_("Error calling module procedure!"));
		GetParent()->CallAsProc(lCodeLine - lCodeSize, ppParams, lSizeArray);
	}

	CRunContext cRunContext(index3);// number of local variables

	cRunContext.m_lParamCount = array3;//number of formal parameters
	cRunContext.m_lStart = lCodeLine;
	cRunContext.m_compileContext = reinterpret_cast<CCompileContext*>(m_pByteCode->m_listCode[cRunContext.m_lStart].m_param1.m_numArray);

	//load parameters
	memcpy(&cRunContext.m_pRefLocVars[0], &ppParams[0], std::min(lSizeArray, cRunContext.m_lParamCount) * sizeof(CValue*));

	//execute arbitrary code
	Execute(&cRunContext, nullptr, false);
}

//Calling a function by its address in the byte code array
//The call is made incl. and in the parent module
void CProcUnit::CallAsFunc(const long lCodeLine, CValue& pvarRetValue, CValue** ppParams, const long lSizeArray)
{
	if (m_pByteCode == nullptr || !m_pByteCode->m_bCompile)
		CBackendException::Error(_("Module not compiled!"));

	const long lCodeSize = m_pByteCode->m_listCode.size();
	if (lCodeLine >= lCodeSize) {
		if (!GetParent())
			CBackendException::Error(_("Error calling module function!"));
		GetParent()->CallAsFunc(lCodeLine - lCodeSize, pvarRetValue, ppParams, lSizeArray);
	}

	CRunContext cRunContext(index3);// number of local variables

	cRunContext.m_lParamCount = array3;//number of formal parameters
	cRunContext.m_lStart = lCodeLine;
	cRunContext.m_compileContext = reinterpret_cast<CCompileContext*>(m_pByteCode->m_listCode[cRunContext.m_lStart].m_param1.m_numArray);

	//load parameters
	memcpy(&cRunContext.m_pRefLocVars[0], &ppParams[0], std::min(lSizeArray, cRunContext.m_lParamCount) * sizeof(CValue*));

	//execute arbitrary code
	Execute(&cRunContext, &pvarRetValue, false);
}

long CProcUnit::FindProp(const wxString& strPropName) const
{
	auto iterator = std::find_if(m_pByteCode->m_listExportVar.begin(), m_pByteCode->m_listExportVar.end(),
		[strPropName](const auto pair) {return stringUtils::CompareString(strPropName, pair.first); });
	if (iterator != m_pByteCode->m_listExportVar.end())
		return (long)iterator->second;
	return wxNOT_FOUND;
}

bool CProcUnit::SetPropVal(const long lPropNum, const CValue& varPropVal)//setting attribute
{
	*m_cCurContext.m_pRefLocVars[lPropNum] = varPropVal;
	return true;
}

bool CProcUnit::SetPropVal(const wxString& strPropName, const CValue& varPropVal)//setting attribute
{
	long lPropNum = FindProp(strPropName);
	if (lPropNum != wxNOT_FOUND) {
		*m_cCurContext.m_pRefLocVars[lPropNum] = varPropVal;
	}
	else {
		const long lPropPos = m_cCurContext.GetLocalCount();
		m_cCurContext.SetLocalCount(lPropPos + 1);
		m_cCurContext.m_cLocVars[lPropPos] = CValue(strPropName);
		*m_cCurContext.m_pRefLocVars[lPropPos] = varPropVal;
	}
	return true;
}

bool CProcUnit::GetPropVal(const long lPropNum, CValue& pvarPropVal) //attribute value
{
	pvarPropVal = m_cCurContext.m_pRefLocVars[lPropNum];
	return true;
}

bool CProcUnit::GetPropVal(const wxString& strPropName, CValue& pvarPropVal) //setting attribute
{
	const long lPropNum = FindProp(strPropName);
	if (lPropNum != wxNOT_FOUND) {
		pvarPropVal = m_cCurContext.m_pRefLocVars[lPropNum];
		return true;
	}
	return false;
}

bool CProcUnit::Evaluate(const wxString& strExpression, CRunContext* pRunContext, CValue& pvarRetValue, bool ñompileBlock)
{
	if (pRunContext == nullptr)
		pRunContext = CProcUnit::GetCurrentRunContext();

	if (strExpression.IsEmpty() || pRunContext == nullptr)
		return false;

	bool isEvalMode = CBackendException::IsEvalMode();
	if (!isEvalMode) CBackendException::SetEvalMode(true);

	auto iterator = std::find_if(pRunContext->m_listEval.begin(), pRunContext->m_listEval.end(),
		[strExpression](const auto pair) {return stringUtils::CompareString(strExpression, pair.first); });

	std::shared_ptr<CProcUnitEvaluate> runEvaluate = nullptr;
	if (iterator == pRunContext->m_listEval.end()) { //this text has not yet been compiled

		CCompileCode* compileExpression = new CCompileCode;
		compileExpression->Load(strExpression);

		CProcUnitEvaluate* evalUnit = new CProcUnitEvaluate;
		if (!evalUnit->CompileExpression(pRunContext, pvarRetValue, *compileExpression, ñompileBlock)) {
			//delete from memory
			wxDELETE(evalUnit);
			wxDELETE(compileExpression);
			if (!isEvalMode) CBackendException::SetEvalMode(false);
			return false;
		}

		runEvaluate = std::shared_ptr<CProcUnitEvaluate>(evalUnit);

		//everything is OK
		pRunContext->m_listEval.insert_or_assign(stringUtils::MakeUpper(strExpression), runEvaluate);
	}
	else {
		runEvaluate = iterator->second;
	}

	//Launch
	bool bDelta = false;

	CCompileContext* compileContext = pRunContext->m_compileContext;
	wxASSERT(compileContext);
	CCompileCode* compileCode = compileContext->m_compileModule;
	wxASSERT(compileCode);
	if (compileCode->m_bExpressionOnly) {
		CCompileContext* curContext = compileContext;
		CCompileCode* curModule = compileCode;
		while (curContext != nullptr) {
			if (!curModule->m_bExpressionOnly)
				break;
			curContext = curContext->m_parentContext;
			curModule = compileCode->GetParent();
		}
		if (curContext && curContext->m_numReturn == RETURN_NONE) {
			bDelta = true;
		}
	}
	else if (compileContext->m_numReturn == RETURN_NONE) {
		bDelta = true;
	}

	try {
		runEvaluate->Execute(&runEvaluate->m_cCurContext, &pvarRetValue, bDelta);
	}
	catch (const CBackendException*) {
		if (!isEvalMode) CBackendException::SetEvalMode(false);
		return false;
	}

	if (!isEvalMode) CBackendException::SetEvalMode(false);
	return true;
}

bool CProcUnit::CompileExpression(CRunContext* pRunContext, CValue& pvarRetValue, CCompileCode& cModule, bool bCompileBlock)
{
	CByteCode* const byteCode = pRunContext->GetByteCode();

	//set the expression calling context as parents
	if (byteCode != nullptr) {
		cModule.m_cByteCode.m_parent = byteCode;
		cModule.m_parent = byteCode->m_compileModule;
		cModule.m_rootContext->m_parentContext = pRunContext->m_compileContext;
	}

	cModule.m_bExpressionOnly = true;
	cModule.m_rootContext->m_numFindLocalInParent = 2;
	cModule.m_numCurrentCompile = wxNOT_FOUND;

	try {
		if (!cModule.PrepareLexem()) {
			return false;
		}
	}
	catch (...) {
		return false;
	}

	cModule.m_cByteCode.m_compileModule = &cModule;

	//process the bytecode array to return the expression result
	CByteUnit code;
	code.m_numOper = OPER_RET;

	try {
		if (bCompileBlock)
			cModule.CompileBlock(cModule.GetContext());
		else
			code.m_param1 = cModule.GetExpression(cModule.GetContext());
	}
	catch (...)
	{
		return false;
	}

	if (!bCompileBlock) {
		cModule.m_cByteCode.m_listCode.push_back(code);
	}

	CByteUnit code2;
	code2.m_numOper = OPER_END;

	cModule.m_cByteCode.m_listCode.push_back(code2);
	cModule.m_cByteCode.m_lVarCount = cModule.m_rootContext->m_listVariable.size();

	//flag of compilation completion
	cModule.m_cByteCode.m_bCompile = true;

	//Project in memory
	SetParent(pRunContext->m_procUnit);

	try {
		CCompileContext* compileContext = pRunContext->m_compileContext;
		wxASSERT(compileContext);
		CCompileCode* compleModule = compileContext->m_compileModule;
		wxASSERT(compleModule);
		Execute(cModule.m_cByteCode, pvarRetValue, false);
		if (compleModule->m_bExpressionOnly) {
			int nParentNumber = 1;
			CCompileContext* curContext = compileContext;
			CCompileCode* curModule = compleModule;
			while (curContext != nullptr) {
				if (curModule->m_bExpressionOnly)
					nParentNumber++;
				curContext = curContext->m_parentContext; curModule = compleModule->GetParent();
			}
			m_pppArrayList[nParentNumber] = pRunContext->m_procUnit->m_pppArrayList[nParentNumber - 1];
		}
		else {
			m_pppArrayList[1] = pRunContext->m_pRefLocVars;
		}
		m_cCurContext.m_compileContext = cModule.m_rootContext;
	}
	catch (...) {
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
//						Construction/Destruction                    //
//////////////////////////////////////////////////////////////////////

CProcUnitEvaluate::~CProcUnitEvaluate()
{
	if (m_pByteCode != nullptr) {
		delete m_pByteCode->m_compileModule;
	}
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

SYSTEM_TYPE_REGISTER(CValueIterator, "iterator", g_valueIterator);
