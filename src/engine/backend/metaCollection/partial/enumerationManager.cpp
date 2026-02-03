////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : enumeration manager
////////////////////////////////////////////////////////////////////////////

#include "enumerationManager.h"
#include "backend/metaData.h"

#include "commonObject.h"
#include "reference/reference.h"

wxIMPLEMENT_DYNAMIC_CLASS(CManagerDataObjectEnumeration, CValue);

CManagerDataObjectEnumeration::CManagerDataObjectEnumeration(CMetaObjectEnumeration* metaObject) :
	m_methodHelper(new CMethodHelper()), m_metaObject(metaObject)
{
}

CManagerDataObjectEnumeration::~CManagerDataObjectEnumeration()
{
	wxDELETE(m_methodHelper);
}

CMetaObjectCommonModule* CManagerDataObjectEnumeration::GetModuleManager() const { return m_metaObject->GetModuleManager(); }

#include "backend/objCtor.h"

class_identifier_t CManagerDataObjectEnumeration::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CManagerDataObjectEnumeration::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CManagerDataObjectEnumeration::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

enum Func {
	eGetForm,
	eGetListForm,
	eGetSelectForm,
};

void CManagerDataObjectEnumeration::PrepareNames() const
{
	IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);
	IModuleManager* moduleManager = metaData->GetModuleManager();
	wxASSERT(moduleManager);

	m_methodHelper->ClearHelper();
	m_methodHelper->AppendFunc("getForm", 3, "getForm(string, owner, guid)");
	m_methodHelper->AppendFunc("getListForm", 3, "getListForm(string, owner, guid)");
	m_methodHelper->AppendFunc("getSelectForm", 3, "getSelectForm(string, owner, guid)");

	//fill custom attributes 
	for (unsigned int idx = 0; idx < m_metaObject->GetChildCount(); idx++) {
		auto child = m_metaObject->GetChild(idx);
		if (g_metaEnumCLSID != child->GetClassType())
			continue;
		if (child->IsDeleted())
			continue;
		m_methodHelper->AppendProp(
			child->GetName(),
			true, false,
			child->GetMetaID(),
			wxNOT_FOUND
		);
	}

	CValue* pRefData = moduleManager->FindCommonModule(m_metaObject->GetModuleManager());
	if (pRefData != nullptr) {
		// add methods from context
		for (long idx = 0; idx < pRefData->GetNMethods(); idx++) {
			m_methodHelper->CopyMethod(pRefData->GetPMethods(), idx);
		}
	}
}

//****************************************************************************
//*                              Override attribute                          *
//****************************************************************************
bool CManagerDataObjectEnumeration::SetPropVal(const long lPropNum, CValue& cValue) {
	return false;
}

bool CManagerDataObjectEnumeration::GetPropVal(const long lPropNum, CValue& pvarPropVal) {
	pvarPropVal = CReferenceDataObject::Create(m_metaObject,
		m_metaObject->GetEnumObjectArray()[lPropNum]->GetGuid()
	);
	return true;
}

bool CManagerDataObjectEnumeration::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);

	switch (lMethodNum)
	{
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
	case eGetSelectForm:
	{
		CValueGuid* guidVal = lSizeArray > 2 ? paParams[2]->ConvertToType<CValueGuid>() : nullptr;
		pvarRetValue = m_metaObject->GetSelectForm(paParams[0]->GetString(),
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