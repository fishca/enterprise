#ifndef __ENUM_FACTORY_H__
#define __ENUM_FACTORY_H__

#include "value.h"

//realization factory pattern 
class CEnumFactory : public CValue {
public:

	CEnumFactory();
	virtual ~CEnumFactory();

	// these methods need to be overridden in your aggregate objects:
	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names	
		//PrepareNames(); 
		return m_methodHelper; 
	}

	virtual void PrepareNames() const; // this method is automatically called to initialize attribute and method names.

	// this method is automatically called to initialize attribute and method names.
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

protected:
	CMethodHelper *m_methodHelper;
};

#endif 