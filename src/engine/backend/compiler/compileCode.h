#ifndef _COMPILE_CODE_H__
#define _COMPILE_CODE_H__

#include "translateCode.h"
#include "compileContext.h"

//*******************************************************************
//*							  Class: compiler                       *
//*******************************************************************

class BACKEND_API ibCompileCode : public ibTranslateCode {

	struct ibCallFunction {

		wxString m_strName;			// name of called function
		wxString m_strRealName;		// name of called function

		ibParamUnit m_puRetValue;	// variable where the result of the function execution should be returned
		ibParamUnit m_puContextVal;  // pointer to the Context variable

		unsigned int m_numAddLine = 0;	// position in the bytecode array where the call was encountered (for the case where there is a call, but the function has not yet been declared)
		unsigned int m_numError = 0;	// to display error messages

		unsigned int m_numString = 0;	// source text number (for error output)
		unsigned int m_numLine = 0;		// source line number (for breakpoints)

		int m_numIsSet = 0;			// a sign that there is no assignment

		wxString m_strModuleName;	// module name (since it is possible to include connections from different modules)

		std::vector<ibParamUnit> m_listParam; // list of passed parameters (list of variables, if the value is not specified, then d.b. (-1, -1))
	};

	friend class ibProcUnit;

public:

	ibCompileCode();
	ibCompileCode(const wxString& strModuleName, const wxString& strDocPath, bool onlyFunction = false);
	ibCompileCode(const wxString& strFileName);

	virtual ~ibCompileCode();

	// basic methods:
	void Reset(); // resetting data to reuse an object	
	void ResetAndFree() { Reset(); ClearLexem(); } // resetting and free data to reuse an object

	void PrepareModuleData();

	void AddVariable(const wxString& strName, const ibValue& value);	// support for external variables
	void AddVariable(const wxString& strName, ibValue* pValue);		// support for external variables

	void AddContextVariable(const wxString& strName, const ibValue& value);
	void AddContextVariable(const wxString& strName, ibValue* pValue);

	void RemoveVariable(const wxString& strName);

	void SetParent(ibCompileCode* parent); // setting the parent module and prohibited max. ancestor

	// Base ibCompileCode has no compile-side parent — only the
	// ibCompileModule subclass holds a typed `m_parentModule` for
	// compile-orchestration cascade (parent-recompile in Designer
	// mode, parent bytecode wire-up on Reset). Subclass call sites
	// reach `m_parentModule` directly; no virtual on base needed.
	//
	// This shape replaces the bytecode → ibCompileCode back-pointer
	// (`m_compileModule`): parent chain lives entirely on the
	// ibCompileModule side, bytecode is self-contained — foundation
	// for AOT cache + production-without-source.

	virtual bool Recompile(); // recompiling a module from a meta object

	virtual bool Compile(); // compiling a module from a meta object
	virtual bool Compile(const wxString& strCode); // Compiling module

	// Eval host function (the function frame eval expression is opening
	// into). Non-eval compile modules return nullptr; ibCompileEval
	// (defined in procUnit.cpp) overrides to return the host fn pointer
	// captured at ctor time. compileContext.cpp's bc walk reads this to
	// expose host's params + locals to eval expressions (e.g. watch on
	// a function parameter `x`).
	virtual const ibByteCode::ibByteFunction* GetEvalHostFunction() const { return nullptr; }

public:

	static void InitializeCompileModule();
	static void SetCodeStyle(short codeStyle);

	ibCompileContext* GetContext() const { return m_rootContext; }

	// Wire one bytecode dependency on this module's bytecode. Forward
	// to ibByteCode::AddDependency, which atomically pushes pointer
	// + id + version snapshot. Preferred entry point at compile-time
	// call sites (CompileExpression eval setup, future common-module
	// import wiring) — keeps callers from reaching into m_cByteCode
	// directly.
	void AddDependency(ibByteCode* dep) {
		m_cByteCode.AddDependency(dep);
	}

	// Build a runtime binding session from this compile-code's
	// extern / context value maps. Forwards to bytecode's CreateBinder
	// for the empty session, then fills via SetVar() — name → live
	// ibValue* — so the result is ready for procUnit->Execute().
	// AOT path bypasses this entirely (manager fills binder directly
	// from its own registry, no compile-context staging).
	ibByteBinder CreateBinder(bool delta = true) {
		ibByteBinder br = m_cByteCode.CreateBinder(delta);
		for (auto& kv : m_listExternValue)  br.SetVar(kv.first, kv.second);
		for (auto& kv : m_listContextValue) br.SetVar(kv.first, kv.second);
		return br;
	}


public:

	ibParamUnit GetExpression(ibCompileContext* context, int priority = 0);

	//attributes:
	bool m_onlyFunction; // true - only functions and export functions

	// current context of variables, functions and labels
	ibByteCode		m_cByteCode;        // output array of bytecodes for execution by the virtual machine

	ibCompileContext* m_rootContext; // root context 

	int				m_numCurrentCompile;		// current position in the token array
	bool			m_changedCode;

	// Compile-mode predicate. Default false (regular module compile).
	// ibCompileEval (procUnit.cpp) overrides to return true so the
	// "no new function declarations / no GOTO" parser gates fire for
	// eval / watch expressions. Replaces the legacy m_bExpressionOnly
	// field — kept off the base class since it's true for exactly one
	// derived class.
	virtual bool IsExpressionOnly() const { return false; }

	// matching external variables
	std::map<wxString, ibValue*> m_listExternValue;

	// matching context variables
	std::map<wxString, ibValue*> m_listContextValue;

protected:

	virtual void DoSetError(int codeError,
		const wxString& strFileName, const wxString& strModuleName, const wxString& strDocPath,
		unsigned int currPos, unsigned int currLine,
		const wxString& strErrorDesc) const;

	const ibLexem& PreviewGetLexem();
	const ibLexem& GetLexem();
	const ibLexem& GETLexem();

	void GETDelimeter(const wxUniChar& c);

	bool IsDelimeter(const wxUniChar& c);
	bool IsKeyWord(int nKey);

	bool IsNextDelimeter(const wxUniChar& c);
	bool IsNextKeyWord(int nKey);

	void GETKeyWord(int nKey);

	wxString GETIdentifier(bool strRealName = false);
	ibValue GETConstant();

	void AddLineInfo(ibByteUnit& code);

	bool CompileModule();
	bool CompileFunction(ibCompileContext* context);
	bool CompileDeclaration(ibCompileContext* context);
	bool CompileBlock(ibCompileContext* context);
	bool CompileNewObject(ibCompileContext* context);
	bool CompileGoto(ibCompileContext* context);
	bool CompileIf(ibCompileContext* context);
	bool CompileWhile(ibCompileContext* context);
	bool CompileFor(ibCompileContext* context);
	bool CompileForeach(ibCompileContext* context);
	bool CompileException(ibCompileContext* context);

	ibParamUnit GetCallFunction(ibCompileContext* context, const wxString& strName, const int& nIsSet);
	ibParamUnit GetCurrentIdentifier(ibCompileContext* context, int& nIsSet);

	ibParamUnit FindConst(const ibValue& constData);

	bool PushCallFunction(const std::shared_ptr<ibCallFunction>& function);
	bool GetFunction(const wxString& strName, std::shared_ptr<ibCompileContext::ibFunction>& function, int* pNumFunction = nullptr);

	bool IsTypeVar(const wxString& sVariable = wxEmptyString);
	wxString GetTypeVar(const wxString& sVariable = wxEmptyString);
	void AddTypeSet(const ibParamUnit& sVariable);

	const int GetConstString(const wxString& strConstName);

	std::map<wxString, unsigned int> m_listHashConst;
	std::vector<std::shared_ptr<ibCallFunction>> m_listCallFunc;	// list of encountered procedure and function calls

private:

	// methods for displaying errors during compilation::
	void SetError(int nErr, const wxString& strError = wxEmptyString);
	void SetError(int nErr, const wxUniChar& c);

	wxString m_strCurFuncName;//name of the current compiled function (for processing the recursive function call option)

	friend struct ibCompileContext;
};

#endif 