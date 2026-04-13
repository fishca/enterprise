#ifndef _MODULE_MANAGER_EXT_H__
#define _MODULE_MANAGER_EXT_H__

#include "backend/moduleManager/moduleManager.h"
#include "backend/metaCollection/partial/dataProcessor.h"
#include "backend/metaCollection/partial/dataReport.h"

class BACKEND_API CValueModuleManagerExternalDataProcessor : public IValueModuleManager {
public:

	virtual CCompileModule* GetCompileModule() const;
	virtual CProcUnit* GetProcUnit() const;

	virtual std::map<wxString, CValue*>& GetContextVariables();

	//metaData and external variant
	CValueModuleManagerExternalDataProcessor(IMetaData* metaData = nullptr, CValueMetaObjectDataProcessor* metaObject = nullptr);
	virtual ~CValueModuleManagerExternalDataProcessor();

	//return external module
	virtual IValueRecordDataObjectExt* GetObjectValue() const { return m_objectValue; }

	//Create common module
	virtual bool CreateMainModule();

	//destroy common module
	virtual bool DestroyMainModule();

	//start common module
	virtual bool StartMainModule(bool force = false);

	//exit common module
	virtual bool ExitMainModule(bool force = false);

	// this method is automatically called to initialize attribute and method names.
	virtual void PrepareNames() const;
	
	//method call
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

	virtual long FindProp(const wxString& strName) const;

private:
	CValueRecordDataObjectDataProcessor* m_objectValue;
};

class BACKEND_API CValueModuleManagerExternalReport : public IValueModuleManager {
public:

	virtual CCompileModule* GetCompileModule() const;
	virtual CProcUnit* GetProcUnit() const;

	virtual std::map<wxString, CValue*>& GetContextVariables();

	//metaData and external variant
	CValueModuleManagerExternalReport(IMetaData* metaData = nullptr, CValueMetaObjectReport* metaObject = nullptr);
	virtual ~CValueModuleManagerExternalReport();

	//return external module
	virtual IValueRecordDataObjectExt* GetObjectValue() const { return m_objectValue; }

	//Create common module
	virtual bool CreateMainModule();

	//destroy common module
	virtual bool DestroyMainModule();

	//start common module
	virtual bool StartMainModule(bool force = false);

	//exit common module
	virtual bool ExitMainModule(bool force = false);

	// this method is automatically called to initialize attribute and method names.
	virtual void PrepareNames() const;
	
	//method call
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value
	virtual long FindProp(const wxString& strName) const;

private:
	CValueRecordDataObjectReport* m_objectValue;
};

#endif