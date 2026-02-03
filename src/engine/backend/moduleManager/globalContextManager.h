#ifndef __MANAGER_BASE_H__
#define __MANAGER_BASE_H__

#include "backend/metaData.h"

class CValueGlobalContextManager : public CValue {
	wxDECLARE_DYNAMIC_CLASS(CValueGlobalContextManager);
public:

	CValueGlobalContextManager(IMetaData* metaData = nullptr) : CValue(eValueTypes::TYPE_VALUE, true),
		m_methodHelper(new CMethodHelper()), m_metaData(metaData)
	{
	}

	virtual ~CValueGlobalContextManager() { wxDELETE(m_methodHelper); }

	virtual CMethodHelper* GetPMethods() const {
		//PrepareNames();  
		return m_methodHelper;
	}

	virtual void PrepareNames() const;

	//attributes
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);

protected:

	//metaData 
	IMetaData* m_metaData;

	//methods 
	CMethodHelper* m_methodHelper;
};

#endif 