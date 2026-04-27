#ifndef _PROCUNIT_H__
#define _PROCUNIT_H__

#include "procContext.h"

struct ibProcUnitState;   // procUnitState.h — forward decl; full type via ibSession::GetPUState()

class BACKEND_API ibProcUnit {
public:

	//Constructors/destructors
	ibProcUnit() : m_numAutoDeleteParent(0),
		m_pByteCode(nullptr),
		m_pppArrayList(nullptr),
		m_ppArrayCode(nullptr) {
	}

	virtual ~ibProcUnit() { Clear(); }

	//Methods
	void Reset();

	void Clear() {
		m_procParent.clear();
		Reset();
	}

	void SetParent(ibProcUnit* procParent) {
		m_procParent.clear();
		if (procParent != nullptr) {
			unsigned int count = procParent->m_procParent.size();
			m_procParent.push_back(procParent);
			for (unsigned int i = 1; i <= count; i++) {
				m_procParent.push_back(procParent->m_procParent[i - 1]);
			}
		}
	}

	ibProcUnit* GetParent(unsigned int iLevel = 0) const {
		if (iLevel >= m_procParent.size()) {
			wxASSERT(iLevel == 0);
			return nullptr;
		}
		else {
			wxASSERT(iLevel == m_procParent.size() - 1 || m_procParent[iLevel]);
			return m_procParent[iLevel];
		}
	}

	unsigned int GetParentCount() const { return m_procParent.size(); }
	ibByteCode* GetByteCode() const { return m_pByteCode; }

	void Execute(ibByteCode& ByteCode) { Execute(ByteCode, nullptr, true); }
	void Execute(ibByteCode& ByteCode, bool bRunModule) { Execute(ByteCode, nullptr, bRunModule); }
	void Execute(ibByteCode& ByteCode, ibValue& pvarRetValue, bool bRunModule = true) { Execute(ByteCode, &pvarRetValue, bRunModule); }

private:
	void Execute(ibByteCode& ByteCode, ibValue* pvarRetValue, bool bRunModule = true);
	void Execute(ibRunContext* pContext, ibValue* pvarRetValue, bool bDelta); // bDelta=true - flag for executing module operators that come at the end of functions and procedures
public:

	static bool Evaluate(const wxString& strExpression, ibRunContext* pRunContext, ibValue& pvarRetValue, bool bCompileBlock);
	bool CompileExpression(ibRunContext* pRunContext, ibValue& pvarRetValue, ibCompileCode& cModule, bool bCompileBlock);

	//call an arbitrary function of the executable module
	long FindExportMethod(const wxString& strMethodName) const { return FindMethod(strMethodName, false, 2); }

	//Search for export functions
	long FindMethod(const wxString& strMethodName, bool bError = false, int bExportOnly = 0) const;

	long FindFunction(const wxString& strMethodName, bool bError = false, int bExportOnly = 0) const;
	long FindProcedure(const wxString& strMethodName, bool bError = false, int bExportOnly = 0) const;

	template <typename ...Types>
	inline void CallAsProc(const wxString& funcName, Types&&... args) {
		ibValue* ppParams[] = { &args..., nullptr };
		CallAsProc(funcName, ppParams, (const long)sizeof ...(args));
	}

	template <typename ...Types>
	inline void CallAsFunc(const wxString& funcName, ibValue& pvarRetValue, Types&&... args) {
		ibValue* ppParams[] = { &args..., nullptr };
		CallAsFunc(funcName, pvarRetValue, ppParams, (const long)sizeof ...(args));
	}

	bool CallAsProc(const wxString& funcName, ibValue** ppParams, const long lSizeArray);
	bool CallAsFunc(const wxString& funcName, ibValue& pvarRetValue, ibValue** ppParams, const long lSizeArray);

	void CallAsProc(const long lCodeLine, ibValue** ppParams, const long lSizeArray);
	void CallAsFunc(const long lCodeLine, ibValue& pvarRetValue, ibValue** ppParams, const long lSizeArray);

	long FindProp(const wxString& strPropName) const;

	bool SetPropVal(const wxString& strPropName, const ibValue& varPropVal);
	bool SetPropVal(const long lPropNum, const ibValue& varPropVal); //setting attribute

	bool GetPropVal(const wxString& strPropName, ibValue& pvarPropVal);
	bool GetPropVal(const long lPropNum, ibValue& pvarPropVal);//attribute value

	// Interpreter state (currentRunModule, runContext stack, errorPlace,
	// recCount) lives on ibProcUnitState — accessed through
	// ibSession::GetPUState()->X. The previous static forwarders on
	// ibProcUnit (GetCurrentRunModule / GetCurrentRunContext / Raise /
	// ...) were removed; callers go through GetPUState() directly,
	// which returns nullptr when no session is bound and lets the
	// caller decide on the null-handling policy explicitly.

protected:

	//attributes:
	int m_numAutoDeleteParent; //flag for deleting the parent module
	ibByteCode* m_pByteCode = nullptr;
	ibValue*** m_pppArrayList = {}; //pointers to arrays of variable pointers (0 - local variables, 1 - variables of the current module, 2 and higher - variables of parent modules)
	ibProcUnit** m_ppArrayCode = {}; //pointers to arrays of executable modules (0 - current module, 1 and higher - parent modules)
	std::vector <ibProcUnit*> m_procParent;

	// Per-thread state (m_currentRunModule, ms_runContext, s_nRecCount,
	// s_errorPlace) lives as thread_local in procUnit.cpp. The storage
	// cannot be declared `static thread_local` on an exported (BACKEND_API)
	// class — MSVC C2492 forbids the combination. Access goes through the
	// static inline/out-of-line methods below, each of which forwards to
	// the file-scope thread_local in procUnit.cpp.

	ibRunContext m_cCurContext;
};

class BACKEND_API ibProcUnitEvaluate : public ibProcUnit {
public:

	//Constructors/destructors
	virtual ~ibProcUnitEvaluate();
};

#endif