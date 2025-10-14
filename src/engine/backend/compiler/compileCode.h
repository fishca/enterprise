#ifndef _COMPILE_CODE_H__
#define _COMPILE_CODE_H__

#include "translateCode.h"
#include "compileContext.h"

//*******************************************************************
//*							  Class: compiler                       *
//*******************************************************************

class BACKEND_API CCompileCode : public CTranslateCode {

	struct CCallFunction {

		wxString m_strName;			// name of called function
		wxString m_strRealName;		// name of called function

		CParamUnit m_puRetValue;	// variable where the result of the function execution should be returned
		CParamUnit m_puContextVal;  // pointer to the Context variable

		unsigned int m_numAddLine = 0;	// position in the bytecode array where the call was encountered (for the case where there is a call, but the function has not yet been declared)
		unsigned int m_numError = 0;	// to display error messages

		unsigned int m_numString = 0;	// source text number (for error output)
		unsigned int m_numLine = 0;		// source line number (for breakpoints)

		int m_numIsSet = 0;			// a sign that there is no assignment

		wxString m_strModuleName;	// module name (since it is possible to include connections from different modules)

		std::vector<CParamUnit> m_listParam; // list of passed parameters (list of variables, if the value is not specified, then d.b. (-1, -1))
	};

	friend class CProcUnit;
	friend struct CCompileContext;

public:

	CCompileCode();
	CCompileCode(const wxString& strModuleName, const wxString& strDocPath, bool onlyFunction = false);
	CCompileCode(const wxString& strFileName);

	virtual ~CCompileCode();

	// basic methods:
	void Reset(); // resetting data to reuse an object	
	void ResetAndFree() { Reset(); ClearLexem(); } // resetting and free data to reuse an object

	void PrepareModuleData();

	void AddVariable(const wxString& strName, const CValue& value);	// support for external variables
	void AddVariable(const wxString& strName, CValue* pValue);		// support for external variables

	void AddContextVariable(const wxString& strName, const CValue& value);
	void AddContextVariable(const wxString& strName, CValue* pValue);

	void RemoveVariable(const wxString& strName);

	void SetParent(CCompileCode* parent); // setting the parent module and prohibited max. ancestor
	virtual CCompileCode* GetParent() const { return m_parent; }

	virtual bool Recompile(); // recompiling a module from a meta object

	virtual bool Compile(); // compiling a module from a meta object
	virtual bool Compile(const wxString& strCode); // Compiling module

public:

	static void InitializeCompileModule();

	CCompileContext* GetContext() {
		m_rootContext->SetModule(this);
		return m_rootContext;
	};

private:

	// methods for displaying errors during compilation::
	void SetError(int nErr, const wxString& strError = wxEmptyString);
	void SetError(int nErr, const wxUniChar& c);

public:

	CParamUnit GetExpression(CCompileContext* context, int priority = 0);

	//attributes:
	bool m_onlyFunction; // true - only functions and export functions 

	CCompileCode* m_parent; // parent module (i.e., in relation to the current one, it acts as a global module)

	// current context of variables, functions and labels
	CByteCode		m_cByteCode;        // output array of bytecodes for execution by the virtual machine

	CCompileContext* m_rootContext; // root context 

	int				m_numCurrentCompile;		// current position in the token array
	bool			m_bExpressionOnly;		// expression evaluation only(no new function assignments)
	bool			m_changedCode;

	// matching external variables
	std::map<wxString, CValue*> m_listExternValue;

	// matching context variables
	std::map<wxString, CValue*> m_listContextValue;

protected:

	virtual void DoSetError(int codeError,
		const wxString& strFileName, const wxString& strModuleName, const wxString& strDocPath,
		unsigned int currPos, unsigned int currLine,
		const wxString& strErrorDesc) const;

	const CLexem& PreviewGetLexem();
	const CLexem& GetLexem();
	const CLexem& GETLexem();

	void GETDelimeter(const wxUniChar& c);

	bool IsDelimeter(const wxUniChar& c);
	bool IsKeyWord(int nKey);

	bool IsNextDelimeter(const wxUniChar& c);
	bool IsNextKeyWord(int nKey);

	void GETKeyWord(int nKey);

	wxString GETIdentifier(bool strRealName = false);
	CValue GETConstant();

	void AddLineInfo(CByteUnit& code);

	bool CompileModule();
	bool CompileFunction(CCompileContext* context);
	bool CompileDeclaration(CCompileContext* context);
	bool CompileBlock(CCompileContext* context);
	bool CompileNewObject(CCompileContext* context);
	bool CompileGoto(CCompileContext* context);
	bool CompileIf(CCompileContext* context);
	bool CompileWhile(CCompileContext* context);
	bool CompileFor(CCompileContext* context);
	bool CompileForeach(CCompileContext* context);

	CParamUnit GetCallFunction(CCompileContext* context, const wxString& strName, const int& nIsSet);
	CParamUnit GetCurrentIdentifier(CCompileContext* context, int& nIsSet);

	CParamUnit FindConst(const CValue& constData);

	bool PushCallFunction(const std::shared_ptr<CCallFunction>& function);
	bool GetFunction(const wxString& strName, std::shared_ptr<CFunction>& function, int* pNumFunction = nullptr);

	bool IsTypeVar(const wxString& sVariable = wxEmptyString);
	wxString GetTypeVar(const wxString& sVariable = wxEmptyString);
	void AddTypeSet(const CParamUnit& sVariable);

	const int GetConstString(const wxString& strConstName);

	std::map<wxString, unsigned int> m_listHashConst;
	std::vector<std::shared_ptr<CCallFunction>> m_listCallFunc;	// list of encountered procedure and function calls
};

#endif 