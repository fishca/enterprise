#ifndef _CONSTANTS_MANAGER_H__
#define _CONSTANTS_MANAGER_H__

#include "constant.h"

class CValueManagerDataObjectConstant :
	public IValueManagerObject {
public:

	CValueManagerDataObjectConstant(CValueMetaObjectConstant* metaConst = nullptr) : m_metaObject(metaConst) {}
	virtual ~CValueManagerDataObjectConstant() {}

	virtual CValueMetaObjectConstant* GetMetaObject() const { return m_metaObject; }

	virtual CMethodHelper* GetPMethods() const {
		//PrepareNames();  
		return &m_methodHelper;
	}

	virtual void PrepareNames() const;
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);

	//Get ref class 
	virtual class_identifier_t GetClassType() const;

	//types 
	virtual wxString GetClassName() const;
	virtual wxString GetString() const;

protected:
	CValueMetaObjectConstant* m_metaObject;
	static CMethodHelper m_methodHelper;
private:
	wxDECLARE_DYNAMIC_CLASS(CValueManagerDataObjectConstant);
};


#endif // !_CONSTANTS_MANAGER_H__
