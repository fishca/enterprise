#ifndef _MODULE_MANAGER_EXT_H__
#define _MODULE_MANAGER_EXT_H__

#include "backend/moduleManager/moduleManager.h"
#include "backend/metaCollection/partial/dataProcessor.h"
#include "backend/metaCollection/partial/dataReport.h"

class BACKEND_API CModuleManagerExternalDataProcessor : public IModuleManager {
public:

	virtual CCompileModule* GetCompileModule() const;
	virtual CProcUnit* GetProcUnit() const;

	virtual std::map<wxString, CValue*>& GetContextVariables();

	//metaData and external variant
	CModuleManagerExternalDataProcessor(IMetaData* metaData = nullptr, CMetaObjectDataProcessor* metaObject = nullptr);
	virtual ~CModuleManagerExternalDataProcessor();

	//return external module
	virtual IRecordDataObjectExt* GetObjectValue() const { return m_objectValue; }

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
	CRecordDataObjectDataProcessor* m_objectValue;
};

class BACKEND_API CModuleManagerExternalReport : public IModuleManager {
public:

	virtual CCompileModule* GetCompileModule() const;
	virtual CProcUnit* GetProcUnit() const;

	virtual std::map<wxString, CValue*>& GetContextVariables();

	//metaData and external variant
	CModuleManagerExternalReport(IMetaData* metaData = nullptr, CMetaObjectReport* metaObject = nullptr);
	virtual ~CModuleManagerExternalReport();

	//return external module
	virtual IRecordDataObjectExt* GetObjectValue() const { return m_objectValue; }

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
	CRecordDataObjectReport* m_objectValue;
};

#endif