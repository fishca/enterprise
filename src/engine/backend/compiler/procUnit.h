#ifndef _PROCUNIT_H__
#define _PROCUNIT_H__

#include "procContext.h"

class BACKEND_API CProcUnit {
public:

	//Constructors/destructors
	CProcUnit() : m_pppArrayList(nullptr),
		m_ppArrayCode(nullptr),
		m_pByteCode(nullptr),
		m_numAutoDeleteParent(0) {
	}

	virtual ~CProcUnit() { Clear(); }

	//Methods
	void Reset() {

		if (m_pppArrayList != nullptr) {
			wxDELETEA(m_pppArrayList);
		}

		if (m_ppArrayCode != nullptr) {
			wxDELETE(m_ppArrayCode);
		}

		if (m_currentRunModule == this) {
			m_currentRunModule = nullptr;
		}

		m_numAutoDeleteParent = 0;

		m_pppArrayList = nullptr;
		m_ppArrayCode = nullptr;
		m_pByteCode = nullptr;
	}

	void Clear() {
		m_procParent.clear();
		Reset();
	}

	void SetParent(CProcUnit* procParent) {
		m_procParent.clear();
		if (procParent != nullptr) {
			unsigned int count = procParent->m_procParent.size();
			m_procParent.push_back(procParent);
			for (unsigned int i = 1; i <= count; i++) {
				m_procParent.push_back(procParent->m_procParent[i - 1]);
			}
		}
	}

	CProcUnit* GetParent(unsigned int iLevel = 0) const {
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
	CByteCode* GetByteCode() const { return m_pByteCode; }

	void Execute(CByteCode& ByteCode) { Execute(ByteCode, nullptr, true); }
	void Execute(CByteCode& ByteCode, bool bRunModule) { Execute(ByteCode, nullptr, bRunModule); }
	void Execute(CByteCode& ByteCode, CValue& pvarRetValue, bool bRunModule = true) { Execute(ByteCode, &pvarRetValue, bRunModule); }

private:
	void Execute(CByteCode& ByteCode, CValue* pvarRetValue, bool bRunModule = true);
	void Execute(CRunContext* pContext, CValue* pvarRetValue, bool bDelta); // bDelta=true - flag for executing module operators that come at the end of functions and procedures
public:

	static bool Evaluate(const wxString& strExpression, CRunContext* pRunContext, CValue& pvarRetValue, bool bCompileBlock);
	bool CompileExpression(CRunContext* pRunContext, CValue& pvarRetValue, CCompileCode& cModule, bool bCompileBlock);

	//call an arbitrary function of the executable module
	long FindExportMethod(const wxString& strMethodName) const { return FindMethod(strMethodName, false, 2); }

	//Search for export functions
	long FindMethod(const wxString& strMethodName, bool bError = false, int bExportOnly = 0) const;

	long FindFunction(const wxString& strMethodName, bool bError = false, int bExportOnly = 0) const;
	long FindProcedure(const wxString& strMethodName, bool bError = false, int bExportOnly = 0) const;

	template <typename ...Types>
	inline void CallAsProc(const wxString& funcName, Types&&... args) {
		CValue* ppParams[] = { &args..., nullptr };
		CallAsProc(funcName, ppParams, (const long)sizeof ...(args));
	}

	template <typename ...Types>
	inline void CallAsFunc(const wxString& funcName, CValue& pvarRetValue, Types&&... args) {
		CValue* ppParams[] = { &args..., nullptr };
		CallAsFunc(funcName, pvarRetValue, ppParams, (const long)sizeof ...(args));
	}

	bool CallAsProc(const wxString& funcName, CValue** ppParams, const long lSizeArray);
	bool CallAsFunc(const wxString& funcName, CValue& pvarRetValue, CValue** ppParams, const long lSizeArray);

	void CallAsProc(const long lCodeLine, CValue** ppParams, const long lSizeArray);
	void CallAsFunc(const long lCodeLine, CValue& pvarRetValue, CValue** ppParams, const long lSizeArray);

	long FindProp(const wxString& strPropName) const;

	bool SetPropVal(const wxString& strPropName, const CValue& varPropVal);
	bool SetPropVal(const long lPropNum, const CValue& varPropVal); //setting attribute

	bool GetPropVal(const wxString& strPropName, CValue& pvarPropVal);
	bool GetPropVal(const long lPropNum, CValue& pvarPropVal);//attribute value

	//run module 
	static CProcUnit* GetCurrentRunModule() { return m_currentRunModule; }
	static void ClearCurrentRunModule() { m_currentRunModule = nullptr; }

	//run context
	static void CProcUnit::AddRunContext(CRunContext* runContext) { ms_runContext.push_back(runContext); }
	static unsigned int CProcUnit::GetCountRunContext() { return ms_runContext.size(); }

	static CRunContext* CProcUnit::GetPrevRunContext() {
		if (ms_runContext.size() < 2)
			return nullptr;
		return ms_runContext[ms_runContext.size() - 2];
	}

	static CRunContext* CProcUnit::GetCurrentRunContext() {
		if (!ms_runContext.size())
			return nullptr;
		return ms_runContext.back();
	}

	static CRunContext* CProcUnit::GetRunContext(unsigned int idx) {
		if (ms_runContext.size() < idx)
			return nullptr;
		return ms_runContext[idx];
	}

	static void CProcUnit::BackRunContext() { ms_runContext.pop_back(); }

	static CByteCode* GetCurrentByteCode() {
		const CRunContext* runContext = GetCurrentRunContext();
		if (runContext != nullptr)
			return runContext->GetByteCode();
		return nullptr;
	}

	static void Raise();

protected:

	//attributes:
	int m_numAutoDeleteParent; //flag for deleting the parent module
	CByteCode* m_pByteCode = nullptr;
	CValue*** m_pppArrayList = {}; //pointers to arrays of variable pointers (0 - local variables, 1 - variables of the current module, 2 and higher - variables of parent modules)
	CProcUnit** m_ppArrayCode = {}; //pointers to arrays of executable modules (0 - current module, 1 and higher - parent modules)
	std::vector <CProcUnit*> m_procParent;

	//static attributes
	static CProcUnit* m_currentRunModule;

	CRunContext m_cCurContext;

	//static attributes
	static std::vector <CRunContext*> ms_runContext; //list of executable module codes
};

class BACKEND_API CProcUnitEvaluate : public CProcUnit {
public:

	//Constructors/destructors
	virtual ~CProcUnitEvaluate();
};

#endif 