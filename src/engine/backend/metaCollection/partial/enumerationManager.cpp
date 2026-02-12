////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : enumeration manager
////////////////////////////////////////////////////////////////////////////

#include "enumerationManager.h"
#include "backend/metaData.h"

#include "commonObject.h"
#include "reference/reference.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueManagerDataObjectEnumeration, CValue);

CValueManagerDataObjectEnumeration::CValueManagerDataObjectEnumeration(CValueMetaObjectEnumeration* metaObject) :
	m_methodHelper(new CMethodHelper()), m_metaObject(metaObject)
{
}

CValueManagerDataObjectEnumeration::~CValueManagerDataObjectEnumeration()
{
	wxDELETE(m_methodHelper);
}

CValueMetaObjectCommonModule* CValueManagerDataObjectEnumeration::GetModuleManager() const { return m_metaObject->GetModuleManager(); }

#include "backend/objCtor.h"

class_identifier_t CValueManagerDataObjectEnumeration::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CValueManagerDataObjectEnumeration::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CValueManagerDataObjectEnumeration::GetString() const
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
	eGetTemplate,
};

void CValueManagerDataObjectEnumeration::PrepareNames() const
{
	IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);
	IValueModuleManager* moduleManager = metaData->GetModuleManager();
	wxASSERT(moduleManager);

	m_methodHelper->ClearHelper();
	m_methodHelper->AppendFunc("getForm", 3, "getForm(string, owner, guid)");
	m_methodHelper->AppendFunc("getListForm", 3, "getListForm(string, owner, guid)");
	m_methodHelper->AppendFunc("getSelectForm", 3, "getSelectForm(string, owner, guid)");
	m_methodHelper->AppendFunc("getTemplate", 1, "getTemplate(string)");

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
bool CValueManagerDataObjectEnumeration::SetPropVal(const long lPropNum, CValue& cValue) {
	return false;
}

bool CValueManagerDataObjectEnumeration::GetPropVal(const long lPropNum, CValue& pvarPropVal) {
	pvarPropVal = CValueReferenceDataObject::Create(m_metaObject,
		m_metaObject->GetEnumObjectArray()[lPropNum]->GetGuid()
	);
	return true;
}

bool CValueManagerDataObjectEnumeration::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
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