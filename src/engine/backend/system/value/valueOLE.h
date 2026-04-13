#ifndef __VALUE_OLE_H__
#define __VALUE_OLE_H__

#include "backend/compiler/value.h"

class BACKEND_API CValueOLE :
	public CValue {
	wxString m_objectName;
#ifdef __WXMSW__
	IDispatch* m_dispatch = nullptr;
	IStream* m_streamDispatch = nullptr;
	IDispatch* m_currentDispatch = nullptr;
#endif 
	CMethodHelper* m_methodHelper;
#ifdef __WXMSW__
	CLSID m_clsId;
#endif
private:
#ifdef __WXMSW__
	IDispatch* DoCreateInstance();
#endif
#ifdef __WXMSW__
	void AddFromArray(CValue& pvarRetValue, long* aPos, SAFEARRAY* psa, SAFEARRAYBOUND* aDims, int nLastDim) const;
	bool FromVariant(const VARIANT& oleVariant, CValue& pvarRetValue) const;
	CValue FromVariantArray(SAFEARRAY* psa) const;
	VARIANT FromValue(const CValue& varRetValue) const;
	void FreeValue(VARIANT& oleVariant);
#endif
	friend class CDebuggerServer;
public:

	//STA
	static void CreateStreamForDispatch();
	static void ReleaseStreamForDispatch();

	static void ReleaseComObjects();

	//MTA
	static void GetInterfaceAndReleaseStream();

public:

#ifdef __WXMSW__
	IDispatch* GetDispatch() const {
		return m_dispatch;
	}
#endif

	virtual CMethodHelper* GetPMethods() const {
		return m_methodHelper;
	}

	virtual void PrepareNames() const;

	virtual long FindMethod(const wxString& strMethodName) const;
	virtual long FindProp(const wxString& strPropName) const;

	virtual bool IsPropReadable(const long lPropNum) const {
		return true;
	}
	virtual bool IsPropWritable(const long lPropNum) const {
		return true;
	}

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);//setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);//attribute value

	//WORK AS AN AGGREGATE OBJECT
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);

#ifdef __WXMSW__
	CValueOLE();
private:
	CValueOLE(const CLSID& clsId, IDispatch* dispatch, const wxString& objectName);
public:
#else
	CValueOLE();
#endif

	virtual ~CValueOLE();

	bool Create(const wxString& strOleName);

	virtual bool Init() {
		return false;
	}

	virtual bool Init(CValue** paParams, const long lSizeArray);

	virtual wxString GetString() const {
		return m_objectName;
	}

	//operator '=='
	virtual bool CompareValueEQ(const CValue& cParam) const {
		CValueOLE* m_pValueOLE = dynamic_cast<CValueOLE*>(cParam.GetRef());
		if (m_pValueOLE) return m_dispatch == m_pValueOLE->m_dispatch;
		else return false;
	}

	//operator '!='
	virtual bool CompareValueNE(const CValue& cParam) const {
		CValueOLE* m_pValueOLE = dynamic_cast<CValueOLE*>(cParam.GetRef());
		if (m_pValueOLE) return m_dispatch != m_pValueOLE->m_dispatch;
		else return false;
	}

	//check is empty
	virtual bool IsEmpty() const {
		return false;
	}

protected:
	wxDECLARE_DYNAMIC_CLASS_NO_COPY(CValueOLE);
};

#endif 