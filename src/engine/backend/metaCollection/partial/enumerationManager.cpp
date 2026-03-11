////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : enumeration manager
////////////////////////////////////////////////////////////////////////////

#include "enumerationManager.h"
#include "backend/metaData.h"

#include "commonObject.h"
#include "reference/reference.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueManagerDataObjectEnumeration, CValue);

CValueMetaObjectCommonModule* CValueManagerDataObjectEnumeration::GetModuleManager() const { return m_metaObject->GetModuleManager(); }

enum Func {
	eGetForm,
	eGetListForm,
	eGetSelectForm,
	eGetTemplate,
};

void CValueManagerDataObjectEnumeration::PrepareNames() const
{
	IValueManagerDataObject::PrepareNames();

	m_methodHelper->AppendFunc(wxT("GetForm"), 3, wxT("GetForm(name : string, owner : any, id : guid)"));
	m_methodHelper->AppendFunc(wxT("GetListForm"), 3, wxT("GetListForm(name : string, owner : any, id : guid)"));
	m_methodHelper->AppendFunc(wxT("GetSelectForm"), 3, wxT("GetSelectForm(name : string, owner : any, id : guid)"));
	m_methodHelper->AppendFunc(wxT("GetTemplate"), 1, wxT("GetTemplate(name : string)"));

	//fill custom attributes 
	for (auto object : m_metaObject->GetEnumObjectArray()) {

		m_methodHelper->AppendProp(
			object->GetName(),
			true, false,
			object->GetMetaID(),
			wxNOT_FOUND
		);
	}
}

//****************************************************************************
//*                              Override attribute                          *
//****************************************************************************
bool CValueManagerDataObjectEnumeration::SetPropVal(const long lPropNum, CValue& cValue)
{
	return false;
}

bool CValueManagerDataObjectEnumeration::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const IValueMetaObject* valueObject =
		m_metaObject->FindEnumObjectByFilter<meta_identifier_t>(m_methodHelper->GetPropData(lPropNum));

	if (valueObject == nullptr)
		return false;

	pvarPropVal = CValueReferenceDataObject::Create(m_metaObject, valueObject->GetGuid());
	return true;
}

bool CValueManagerDataObjectEnumeration::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
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

	return IValueManagerDataObject::CallAsFunc(lMethodNum, pvarRetValue, paParams, lSizeArray);
}