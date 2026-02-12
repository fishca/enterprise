////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : acc register manager
////////////////////////////////////////////////////////////////////////////

#include "accumulationRegisterManager.h"
#include "backend/metaData.h"

#include "commonObject.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueManagerDataObjectAccumulationRegister, CValue);

CValueManagerDataObjectAccumulationRegister::CValueManagerDataObjectAccumulationRegister(CValueMetaObjectAccumulationRegister* metaObject) :
	m_methodHelper(new CMethodHelper()), m_metaObject(metaObject)
{
}

CValueManagerDataObjectAccumulationRegister::~CValueManagerDataObjectAccumulationRegister()
{
	wxDELETE(m_methodHelper);
}

CValueMetaObjectCommonModule* CValueManagerDataObjectAccumulationRegister::GetModuleManager() const {
	return m_metaObject->GetModuleManager();
}

#include "backend/objCtor.h"

class_identifier_t CValueManagerDataObjectAccumulationRegister::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CValueManagerDataObjectAccumulationRegister::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CValueManagerDataObjectAccumulationRegister::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

enum Func {
	eCreateRecordSet,
	eCreateRecordKey,
	eBalance,
	eTurnover,
	eSelect,
	eGetForm,
	eGetListForm,
	eGetTemplate,
};

void CValueManagerDataObjectAccumulationRegister::PrepareNames() const
{
	IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);
	IValueModuleManager* moduleManager = metaData->GetModuleManager();
	wxASSERT(moduleManager);

	m_methodHelper->ClearHelper();
	m_methodHelper->AppendFunc("createRecordSet", "createRecordSet()");
	m_methodHelper->AppendFunc("createRecordKey", "createRecordKey()");
	m_methodHelper->AppendFunc("balance", 2, "balance(period, filter...)");
	m_methodHelper->AppendFunc("turnovers", 4, "turnovers(beginOfPeriod, endOfPeriod, filter...)");
	m_methodHelper->AppendFunc("select", "select()");
	m_methodHelper->AppendFunc("getForm", 3, "getForm(string, owner, guid)");
	m_methodHelper->AppendFunc("getRecordForm", 3, "getRecordForm(string, owner, guid)");
	m_methodHelper->AppendFunc("getListForm", 3, "getListForm(string, owner, guid)");
	m_methodHelper->AppendFunc("getTemplate", 1, "getTemplate(string)");

	CValue* pRefData = moduleManager->FindCommonModule(m_metaObject->GetModuleManager());
	if (pRefData != nullptr) {
		// add methods from context
		for (long idx = 0; idx < pRefData->GetNMethods(); idx++) {
			m_methodHelper->CopyMethod(pRefData->GetPMethods(), idx);
		}
	}
}

#include "selector/objectSelector.h"

bool CValueManagerDataObjectAccumulationRegister::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);

	switch (lMethodNum)
	{
	case eCreateRecordSet:
		pvarRetValue = m_metaObject->CreateRecordSetObjectValue();
		return true;
	case eCreateRecordKey:
		pvarRetValue = CValue::CreateAndPrepareValueRef<CValueRecordKeyObject>(m_metaObject);
		return true;
	case eBalance: pvarRetValue = lSizeArray > 1 ?
		CValueManagerDataObjectAccumulationRegister::Balance(*paParams[0], *paParams[1]) : CValueManagerDataObjectAccumulationRegister::Balance(*paParams[0]);
		return true;
	case eTurnover: pvarRetValue = lSizeArray > 2 ?
		CValueManagerDataObjectAccumulationRegister::Turnovers(*paParams[0], paParams[1], paParams[2]) : CValueManagerDataObjectAccumulationRegister::Turnovers(*paParams[0], *paParams[1]);
		return true;
	case eSelect:
		pvarRetValue = CValue::CreateAndPrepareValueRef<CValueSelectorRegisterDataObject>(m_metaObject);
		return true;
	case eGetForm:
	{
		CValueGuid* guidVal = lSizeArray > 2 ? paParams[2]->ConvertToType<CValueGuid>() : nullptr;
		pvarRetValue = m_metaObject->GetGenericForm(paParams[0]->GetString(),
			lSizeArray > 1 ? paParams[1]->ConvertToType<IBackendControlFrame>() : nullptr,
			guidVal ? ((CGuid)*guidVal) : CGuid());
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
	case eGetTemplate:
		pvarRetValue = m_metaObject->GetTemplate(paParams[0]->GetString());
		return true;
	}

	IValueModuleManager* moduleManager = metaData->GetModuleManager();
	wxASSERT(moduleManager);

	CValue* pRefData = moduleManager->FindCommonModule(m_metaObject->GetModuleManager());
	if (pRefData != nullptr)
		return pRefData->CallAsFunc(lMethodNum, pvarRetValue, paParams, lSizeArray);
	return false;
}