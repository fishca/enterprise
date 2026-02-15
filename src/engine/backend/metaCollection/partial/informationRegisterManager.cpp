////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : information register manager
////////////////////////////////////////////////////////////////////////////

#include "informationRegisterManager.h"
#include "backend/metaData.h"

#include "commonObject.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueManagerDataObjectInformationRegister, CValue);

CValueManagerDataObjectInformationRegister::CValueManagerDataObjectInformationRegister(CValueMetaObjectInformationRegister* metaObject) :
	m_methodHelper(new CMethodHelper()), m_metaObject(metaObject)
{
}

CValueManagerDataObjectInformationRegister::~CValueManagerDataObjectInformationRegister()
{
	wxDELETE(m_methodHelper);
}

CValueMetaObjectCommonModule* CValueManagerDataObjectInformationRegister::GetModuleManager() const {
	return m_metaObject->GetModuleManager();
}

#include "backend/objCtor.h"

class_identifier_t CValueManagerDataObjectInformationRegister::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CValueManagerDataObjectInformationRegister::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CValueManagerDataObjectInformationRegister::GetString() const
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
	eGetTemplate,
};

void CValueManagerDataObjectInformationRegister::PrepareNames() const
{
	IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);
	IValueModuleManager* moduleManager = metaData->GetModuleManager();
	wxASSERT(moduleManager);

	m_methodHelper->ClearHelper();
	m_methodHelper->AppendFunc(wxT("CreateRecordSet"), wxT("CreateRecordSet()"));
	m_methodHelper->AppendFunc(wxT("CreateRecordManager"), wxT("CreateRecordManager()"));
	m_methodHelper->AppendFunc(wxT("CreateRecordKey"), wxT("CreateRecordKey()"));
	m_methodHelper->AppendFunc(wxT("Get"), 1, wxT("Get(Filter...)"));
	m_methodHelper->AppendFunc(wxT("GetFirst"), 3, wxT("GetFirst(beginOfPeriod, filter...)"));
	m_methodHelper->AppendFunc(wxT("GetLast"), 2, wxT("GetLast(endOfPeriod, filter...)"));
	m_methodHelper->AppendFunc(wxT("SliceFirst"), 2, wxT("SliceFirst(beginOfPeriod, filter...)"));
	m_methodHelper->AppendFunc(wxT("SliceLast"), 2, wxT("SliceLast(endOfPeriod, filter...)"));
	m_methodHelper->AppendFunc(wxT("Select"), wxT("Select()"));
	m_methodHelper->AppendFunc(wxT("GetForm"), 3, wxT("GetForm(string, owner, guid)"));
	m_methodHelper->AppendFunc(wxT("GetRecordForm"), 3, wxT("GetRecordForm(string, owner, guid)"));
	m_methodHelper->AppendFunc(wxT("GetListForm"), 3, wxT("GetListForm(string, owner, guid)"));
	m_methodHelper->AppendFunc(wxT("GetTemplate"), 1, wxT("GetTemplate(string)"));

	CValue * pRefData = moduleManager->FindCommonModule(m_metaObject->GetModuleManager());
	if (pRefData != nullptr) {
		// add methods from context
		for (long idx = 0; idx < pRefData->GetNMethods(); idx++) {
			m_methodHelper->CopyMethod(pRefData->GetPMethods(), idx);
		}
	}
}

#include "selector/objectSelector.h"

bool CValueManagerDataObjectInformationRegister::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
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
		pvarRetValue = CValue::CreateAndPrepareValueRef<CValueRecordKeyObject>(m_metaObject);
		return true;
	case eGet:
		pvarRetValue = lSizeArray > 1 ?
			CValueManagerDataObjectInformationRegister::Get(*paParams[0], *paParams[1])
			: lSizeArray > 0 ?
			CValueManagerDataObjectInformationRegister::Get(*paParams[0]) :
			CValueManagerDataObjectInformationRegister::Get();
		return true;
	case eGetFirst:
		pvarRetValue = lSizeArray > 1 ?
			CValueManagerDataObjectInformationRegister::GetFirst(*paParams[0], *paParams[1])
			: CValueManagerDataObjectInformationRegister::GetFirst(*paParams[0]);
		return true;
	case eGetLast:
		pvarRetValue = lSizeArray > 1 ?
			CValueManagerDataObjectInformationRegister::GetLast(*paParams[0], *paParams[1])
			: CValueManagerDataObjectInformationRegister::GetLast(*paParams[0]);
		return true;
	case eSliceFirst:
		pvarRetValue = lSizeArray > 1 ?
			CValueManagerDataObjectInformationRegister::SliceFirst(*paParams[0], *paParams[1])
			: CValueManagerDataObjectInformationRegister::SliceFirst(*paParams[0]);
	case eSliceLast:
		pvarRetValue = lSizeArray > 1 ?
			CValueManagerDataObjectInformationRegister::SliceLast(*paParams[0], *paParams[1])
			: CValueManagerDataObjectInformationRegister::SliceLast(*paParams[0]);
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
	case eGetRecordForm:
	{
		CValueRecordKeyObject* keyVal = lSizeArray > 2 ? paParams[2]->ConvertToType<CValueRecordKeyObject>() : nullptr;
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