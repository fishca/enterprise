#ifndef _COMPILE_CONTEXT__H_
#define _COMPILE_CONTEXT__H_

#include "backend/compiler/byteCode.h"

struct CCompileContext;

struct CVariable
{
	CVariable() : m_bExport(false), m_bContext(false), m_bTempVar(false), m_numVariable(0) {}
	CVariable(const wxString& strVariableName) : m_strName(strVariableName), m_bExport(false), m_bContext(false), m_bTempVar(false), m_numVariable(0) {}

	bool m_bExport;
	bool m_bContext;
	bool m_bTempVar;
	unsigned int m_numVariable;
	wxString m_strName; // Variable name
	wxString m_strType; // Value type
	wxString m_strContextVar; // Context variable name
	wxString m_strRealName; // Real variable name
};

struct CParamVariable
{
	CParamVariable() : m_bByRef(false) {
		m_valData.m_numArray = -1;
		m_valData.m_numIndex = -1;
	}

	bool m_bByRef;
	wxString m_strName; // Variable name
	wxString m_strType; // Value type
	CParamUnit m_valData; // Default value
};

//function definition
struct CFunction
{
	CFunction(const wxString& funcName, CCompileContext* compileContext = nullptr) :
		m_strName(funcName),
		m_compileContext(compileContext),
		m_bExport(false),
		m_bContext(false),
		m_lVarCount(0), m_nStart(0), m_nFinish(0), m_numLine(0)
	{
		m_realRetValue.m_numArray = 0;
		m_realRetValue.m_numIndex = 0;
	}

	bool m_bExport, m_bContext;

	wxString m_strRealName; //Function name
	wxString m_strName; //Function name in uppercase
	wxString m_strType; //type (in English notation), if it is a typed function

	CCompileContext* m_compileContext;//compilation context

	unsigned int m_lVarCount;// number of local variables
	unsigned int m_nStart;// starting position in bytecode array
	unsigned int m_nFinish;//final position in bytecode array

	CParamUnit m_realRetValue;//for storing variable during real call

	//for IntelliSense
	unsigned int m_numLine; //source line number (for breakpoints)

	wxString m_strShortDescription;//includes the entire line after the Function (Procedure) keyword
	wxString m_strLongDescription;//includes the entire merged (i.e. without empty lines) comment block before the function (procedure) definition
	wxString m_strContextVar; //name of the context variable

	std::vector<CParamVariable> m_listParam;
};

struct CLabel
{
	int		 m_numLine = 0;
	int		 m_numError = 0;
	wxString m_strName;
};

struct CCompileContext {

	//Setting jump addresses for Continue and Break commands
	void StartDoList() {

		//create lists for Continue and Break commands (they will store the addresses of byte codes where the corresponding commands were encountered)
		m_numDoNumber++;
		m_listContinue[m_numDoNumber] = new std::vector<int>();
		m_listBreak[m_numDoNumber] = new std::vector<int>();
	}

	//Setting jump addresses for Continue and Break commands
	void FinishDoList(CByteCode& cByteCode, int gotoContinue, int gotoBreak) {
		std::vector<int>* pListC = m_listContinue[m_numDoNumber];
		std::vector<int>* pListB = m_listBreak[m_numDoNumber];
		if (pListC == 0 || pListB == 0) {
#ifdef DEBUG 
			wxLogDebug("Error (FinishDoList) gotoContinue=%d, gotoBreak=%d\n", gotoContinue, gotoBreak);
			wxLogDebug("m_numDoNumber=%d\n", m_numDoNumber);
#endif 
			m_numDoNumber--;
			return;
		}
		for (unsigned int i = 0; i < pListC->size(); i++) {
			cByteCode.m_listCode[*pListC[i].data()].m_param1.m_numIndex = gotoContinue;
		}
		for (unsigned int i = 0; i < pListB->size(); i++) {
			cByteCode.m_listCode[*pListB[i].data()].m_param1.m_numIndex = gotoBreak;
		}
		m_listContinue.erase(m_numDoNumber);
		m_listContinue.erase(m_numDoNumber);
		delete pListC;
		delete pListB;
		m_numDoNumber--;
	}

	void DoLabels();

	void SetModule(CCompileCode* module) { m_compileModule = module; }

	CParamUnit CreateVariable(const wxString strPrefix = wxT("@temp_"));
	CParamUnit AddVariable(const wxString& strVarName, const wxString& strType = wxEmptyString, bool bExport = false, bool bContext = false, bool bTempVar = false);
	CParamUnit GetVariable(const wxString& strVarName, bool bFindInParent = true, bool bCheckError = false, bool bContext = false, bool bTempVar = false);

	bool FindVariable(const wxString& strVarName, std::shared_ptr<CVariable>& foundedVar, bool context = false);
	bool FindFunction(const wxString& funcName, std::shared_ptr<CFunction>& foundedFunc, bool context = false);

	CCompileContext(CCompileContext* parentContext = nullptr) :
		m_parentContext(parentContext), m_functionContext(nullptr), m_compileModule(nullptr),
		m_numDoNumber(0), m_numReturn(0), m_numTempVar(0), m_numFindLocalInParent(1) {
	}

	CCompileCode* m_compileModule;
	CCompileContext* m_parentContext; //parent context

	//current context 
	std::shared_ptr<CFunction> m_functionContext;

	//VARIABLES
	std::map<wxString, std::shared_ptr<CVariable>> m_listVariable;

	int m_numTempVar;//current temporary variable number
	int m_numFindLocalInParent;//flag for searching variables in the parent (one level up), in other cases only export variables are searched in parents)

	//FUNCTIONS AND PROCEDURES
	std::map<wxString, std::shared_ptr<CFunction>> m_listFunction; //list of encountered function definitions

	short m_numReturn;//RETURN operator processing mode: RETURN_NONE,RETURN_PROCEDURE,RETURN_FUNCTION
	wxString m_strCurFuncName;//name of the current compiled function (for processing the recursive function call option)

	//LOOPS
	//Service attributes
	unsigned short m_numDoNumber;//nested loop number

	std::map<unsigned short, std::vector<int>*> m_listContinue;//addresses of Continue operators
	std::map<unsigned short, std::vector<int>*> m_listBreak;//addresses of Break operators

	//LABELS
	std::map<wxString, unsigned int> m_listLabelDef; //declarations
	std::vector<std::shared_ptr<CLabel>> m_listLabel; //list of encountered transitions to labels
};

#endif