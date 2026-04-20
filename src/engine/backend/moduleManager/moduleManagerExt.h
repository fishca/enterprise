#ifndef _MODULE_MANAGER_EXT_H__
#define _MODULE_MANAGER_EXT_H__

#include "backend/moduleManager/moduleManager.h"
#include "backend/metaCollection/partial/dataProcessor.h"
#include "backend/metaCollection/partial/dataReport.h"

class BACKEND_API ibValueModuleManagerExternalDataProcessor : public ibValueModuleManager {
public:

	virtual ibCompileModule* GetCompileModule() const;
	virtual ibProcUnit* GetProcUnit() const;

	virtual std::map<wxString, ibValue*>& GetContextVariables();

	//metaData and external variant
	ibValueModuleManagerExternalDataProcessor(ibMetaData* metaData = nullptr, ibValueMetaObjectDataProcessor* metaObject = nullptr);
	virtual ~ibValueModuleManagerExternalDataProcessor();

	//return external module
	virtual ibValueRecordDataObjectExt* GetObjectValue() const { return m_objectValue; }

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
	virtual bool CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray);

	virtual bool SetPropVal(const long lPropNum, const ibValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, ibValue& pvarPropVal);                   //attribute value

	virtual long FindProp(const wxString& strName) const;

private:
	ibValueRecordDataObjectDataProcessor* m_objectValue;
};

class BACKEND_API ibValueModuleManagerExternalReport : public ibValueModuleManager {
public:

	virtual ibCompileModule* GetCompileModule() const;
	virtual ibProcUnit* GetProcUnit() const;

	virtual std::map<wxString, ibValue*>& GetContextVariables();

	//metaData and external variant
	ibValueModuleManagerExternalReport(ibMetaData* metaData = nullptr, ibValueMetaObjectReport* metaObject = nullptr);
	virtual ~ibValueModuleManagerExternalReport();

	//return external module
	virtual ibValueRecordDataObjectExt* GetObjectValue() const { return m_objectValue; }

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
	virtual bool CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray);

	virtual bool SetPropVal(const long lPropNum, const ibValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, ibValue& pvarPropVal);                   //attribute value
	virtual long FindProp(const wxString& strName) const;

private:
	ibValueRecordDataObjectReport* m_objectValue;
};

#endif