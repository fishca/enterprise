#ifndef __BYTE_CODE_H__
#define __BYTE_CODE_H__

#include "backend/compiler/value.h"

//*******************************************************************************

class BACKEND_API CCompileCode;

//*******************************************************************************

struct CParamRunUnit {
	wxLongLong_t m_numArray = 0;
	wxLongLong_t m_numIndex = 0;
};

struct CParamUnit : CParamRunUnit {
	wxString	 m_strType;			//variable type in English notation (in case of explicit typing)
};

//storing one program step
struct CByteUnit {

	short		 m_numOper = 0;			//instruction code
	unsigned int m_numString = 0;		//source text number (for error output)
	unsigned int m_numLine = 0;			//source line number (for breakpoints)

	//parameters for instructions:
	CParamRunUnit	 m_param1;
	CParamRunUnit	 m_param2;
	CParamRunUnit	 m_param3;
	CParamRunUnit	 m_param4;			 // - used for optimization

	wxString	 m_strModuleName;	 // module name (since it is possible to include connections from different modules)
	wxString	 m_strDocPath; 	 	 // unique path to the document
	wxString	 m_strFileName; 	 // path to the file (if external processing)
};

struct CByteCode {

	struct CByteFunction {

		operator long() const { return m_lCodeLine; }

		long m_lCodeParamCount = 0;
		long m_lCodeLine = -1;
		bool m_bCodeRet = false;
	};

public:

	void SetModule(CCompileCode* compileCode) { m_compileModule = compileCode; }

	long FindMethod(const wxString& strMethodName) const;
	long FindExportMethod(const wxString& strMethodName) const;

	long FindFunction(const wxString& funcName) const;
	long FindExportFunction(const wxString& funcName) const;

	long FindProcedure(const wxString& procName) const;
	long FindExportProcedure(const wxString& procName) const;

	long FindVariable(const wxString& strVarName) const;
	long FindExportVariable(const wxString& strVarName) const;

	long GetNParams(const long lCodeLine) const {
		// if is not initializer then set no return value
		auto iterator = std::find_if(m_listFunc.begin(), m_listFunc.end(),
			[lCodeLine](const auto pair) { return lCodeLine == ((long)pair.second); });
		if (iterator != m_listFunc.end())
			return iterator->second.m_lCodeParamCount;
		return 0;
	}

	bool HasRetVal(const long lCodeLine) const {
		// if is not initializer then set no return value
		auto iterator = std::find_if(m_listFunc.begin(), m_listFunc.end(),
			[lCodeLine](const auto pair) { return pair.second.m_bCodeRet && lCodeLine == ((long)pair.second); });
		return iterator != m_listFunc.end();
	}

	void Reset() {
		m_lStartModule = 0;
		m_lVarCount = 0;
		m_strModuleName = wxEmptyString;
		m_bCompile = false;
		m_parent = nullptr;
		m_compileModule = nullptr;
		m_listCode.clear();
		m_listConst.clear();
		m_listVar.clear();
		m_listFunc.clear();
		m_listExportVar.clear();
		m_listExportFunc.clear();
		m_listExternValue.clear();
	}

	//Attributes:
	CByteCode* m_parent = nullptr; //Parent bytecode (for checking)
	bool m_bCompile = false; //indication of successful compilation
	CCompileCode* m_compileModule = nullptr;
	long m_lVarCount = 0; // number of local variables in the module
	long m_lStartModule = 0; // beginning of the module start position
	wxString m_strModuleName;//name of the executable module to which the bytecode belongs

	//list of external and context variables
	std::vector <CValue*> m_listExternValue;
	std::vector <CByteUnit> m_listCode;//executable code of the module
	std::vector <CValue> m_listConst;//list of module constants

	std::map<wxString, long> m_listVar; //list of module variables
	std::map<wxString, CByteFunction> m_listFunc; //list of module functions and procedures
	std::map<wxString, long> m_listExportVar; //list of module export variables
	std::map<wxString, CByteFunction> m_listExportFunc; //list of module export functions and procedures
};

#endif // !_BYTE_CODE_H__
