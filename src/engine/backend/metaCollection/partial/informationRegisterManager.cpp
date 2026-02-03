////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : information register manager
////////////////////////////////////////////////////////////////////////////

#include "informationRegisterManager.h"
#include "backend/metaData.h"

#include "commonObject.h"

wxIMPLEMENT_DYNAMIC_CLASS(CManagerDataObjectInformationRegister, CValue);

CManagerDataObjectInformationRegister::CManagerDataObjectInformationRegister(CMetaObjectInformationRegister* metaObject) : 
	m_methodHelper(new CMethodHelper()), m_metaObject(metaObject)
{
}

CManagerDataObjectInformationRegister::~CManagerDataObjectInformationRegister()
{
	wxDELETE(m_methodHelper);
}

CMetaObjectCommonModule* CManagerDataObjectInformationRegister::GetModuleManager() const {
	return m_metaObject->GetModuleManager();
}

#include "backend/objCtor.h"

class_identifier_t CManagerDataObjectInformationRegister::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CManagerDataObjectInformationRegister::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CManagerDataObjectInformationRegister::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

enum {
	eCreateRecordSet,
	eCreateRecordManager,
	eCreateRecordKey,
	eGet,
	eGetFirst,
	eGetLast,
	eSliceFirst,
	eSliceLast,
	eSelect,
	eGetForm,
	eGetRecordForm,
	eGetListForm,
};

void CManagerDataObjectInformationRegister::PrepareNames() const
{
	IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);
	IModuleManager* moduleManager = metaData->GetModuleManager();
	wxASSERT(moduleManager);

	m_methodHelper->ClearHelper();
	m_methodHelper->AppendFunc("createRecordSet", "createRecordSet()");
	m_methodHelper->AppendFunc("createRecordManager", "createRecordManager()");
	m_methodHelper->AppendFunc("createRecordKey", "createRecordKey()");
	m_methodHelper->AppendFunc("get", 2, "get(Filter...)");
	m_methodHelper->AppendFunc("getFirst", 3, "sliceFirst(beginOfPeriod, filter...)");
	m_methodHelper->AppendFunc("getLast", 2, "sliceLast(endOfPeriod, filter...)");
	m_methodHelper->AppendFunc("sliceFirst", 2, "sliceFirst(beginOfPeriod, filter...)");
	m_methodHelper->AppendFunc("sliceLast", 2, "sliceLast(endOfPeriod, filter...)");
	m_methodHelper->AppendFunc("select", "select()");
	m_methodHelper->AppendFunc("getForm", 3, "getForm(string, owner, guid)");
	m_methodHelper->AppendFunc("getRecordForm", 3, "getRecordForm(string, owner, guid)");
	m_methodHelper->AppendFunc("getListForm", 3, "getListForm(string, owner, guid)");

	CValue* pRefData = moduleManager->FindCommonModule(m_metaObject->GetModuleManager());
	if (pRefData != nullptr) {
		// add methods from context
		for (long idx = 0; idx < pRefData->GetNMethods(); idx++) {
			m_methodHelper->CopyMethod(pRefData->GetPMethods(), idx);
		}
	}
}

#include "selector/objectSelector.h"

bool CManagerDataObjectInformationRegister::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);

	switch (lMethodNum)
	{
	case eCreateRecordSet:
		pvarRetValue = m_metaObject->CreateRecordSetObjectValue();
		return true;
	case eCreateRecordManager:
		pvarRetValue = m_metaObject->CreateRecordManagerObjectValue();
		return true;
	case eCreateRecordKey:
		pvarRetValue = CValue::CreateAndPrepareValueRef<CRecordKeyObject>(m_metaObject);
		return true;
	case eGet:
		pvarRetValue = lSizeArray > 1 ?
			CManagerDataObjectInformationRegister::Get(*paParams[0], *paParams[1])
			: lSizeArray > 0 ?
			CManagerDataObjectInformationRegister::Get(*paParams[0]) :
			CManagerDataObjectInformationRegister::Get();
		return true;
	case eGetFirst:
		pvarRetValue = lSizeArray > 1 ?
			CManagerDataObjectInformationRegister::GetFirst(*paParams[0], *paParams[1])
			: CManagerDataObjectInformationRegister::GetFirst(*paParams[0]);
		return true;
	case eGetLast:
		pvarRetValue = lSizeArray > 1 ?
			CManagerDataObjectInformationRegister::GetLast(*paParams[0], *paParams[1])
			: CManagerDataObjectInformationRegister::GetLast(*paParams[0]);
		return true;
	case eSliceFirst:
		pvarRetValue = lSizeArray > 1 ?
			CManagerDataObjectInformationRegister::SliceFirst(*paParams[0], *paParams[1])
			: CManagerDataObjectInformationRegister::SliceFirst(*paParams[0]);
	case eSliceLast:
		pvarRetValue = lSizeArray > 1 ?
			CManagerDataObjectInformationRegister::SliceLast(*paParams[0], *paParams[1])
			: CManagerDataObjectInformationRegister::SliceLast(*paParams[0]);
		return true;
	case eSelect:
		pvarRetValue = CValue::CreateAndPrepareValueRef<CSelectorRegisterObject>(m_metaObject);
		return true;
	case eGetForm:
	{
		CValueGuid* guidVal = lSizeArray > 2 ? paParams[2]->ConvertToType<CValueGuid>() : nullptr;
		pvarRetValue = m_metaObject->GetGenericForm(paParams[0]->GetString(),
			lSizeArray > 1 ? paParams[1]->ConvertToType<IBackendControlFrame>() : nullptr,
			guidVal ? ((CGuid)*guidVal) : CGuid());
		return true;
	}
	case eGetRecordForm:
	{
		CRecordKeyObject* keyVal = lSizeArray > 2 ? paParams[2]->ConvertToType<CRecordKeyObject>() : nullptr;
		pvarRetValue = m_metaObject->GetRecordForm(paParams[0]->GetString(),
			lSizeArray > 1 ? paParams[1]->ConvertToType<IBackendControlFrame>() : nullptr,
			keyVal ? (keyVal->GetUniqueKey()) : nullptr);
		return true;
	}
	case eGetListForm:
	{
		CValueGuid* guidVal = lSizeArray > 2 ? paParams[2]->ConvertToType<CValueGuid>() : nullptr;
		pvarRetValue = m_metaObject->GetListForm(paParams[0]->GetString(),
			lSizeArray > 1 ? paParams[1]->ConvertToType<IBackendControlFrame>() : nullptr,
			guidVal ? ((CGuid)*guidVal) : CGuid());
		return true;
	}
	}

	IModuleManager* moduleManager = metaData->GetModuleManager();
	wxASSERT(moduleManager);

	CValue* pRefData = moduleManager->FindCommonModule(m_metaObject->GetModuleManager());
	if (pRefData != nullptr)
		return pRefData->CallAsFunc(lMethodNum, pvarRetValue, paParams, lSizeArray);
	return false;
}