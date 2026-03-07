////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : catalog manager
////////////////////////////////////////////////////////////////////////////

#include "catalogManager.h"
#include "backend/metaData.h"

#include "commonObject.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueManagerDataObjectCatalog, CValue);

CValueMetaObjectCommonModule* CValueManagerDataObjectCatalog::GetModuleManager() const { return m_metaObject->GetModuleManager(); }

#include "reference/reference.h"

CValueReferenceDataObject* CValueManagerDataObjectCatalog::EmptyRef() const
{
	return CValueReferenceDataObject::Create(m_metaObject);
}

enum Func {
	eCreateElement = 0,
	eCreateGroup,
	eSelect,
	eFindByCode,
	eFindByDescription,
	eGetForm,
	eGetListForm,
	eGetSelectForm,
	eGetTemplate,
	eEmptyRef
};

void CValueManagerDataObjectCatalog::PrepareNames() const
{
	IValueManagerDataObject::PrepareNames();

	m_methodHelper->AppendFunc(wxT("CreateElement"), wxT("CreateElement()"));
	m_methodHelper->AppendFunc(wxT("CreateGroup"), wxT("CreateGroup()"));
	m_methodHelper->AppendFunc(wxT("Select"), wxT("Select()"));
	m_methodHelper->AppendFunc(wxT("FindByCode"), 1, wxT("FindByCode(code : string)"));
	m_methodHelper->AppendFunc(wxT("FindByDescription"), 1, wxT("FindByDescription(descr : string)"));
	m_methodHelper->AppendFunc(wxT("GetForm"), 3, wxT("GetForm(name : string, owner : any, id : guid)"));
	m_methodHelper->AppendFunc(wxT("GetListForm"), 3, wxT("GetListForm(name : string, owner : any, id : guid)"));
	m_methodHelper->AppendFunc(wxT("GetSelectForm"), 3, wxT("GetSelectForm(name : string, owner : any, id : guid)"));
	m_methodHelper->AppendFunc(wxT("GetTemplate"), 1, wxT("GetTemplate(name : string)"));
	m_methodHelper->AppendFunc(wxT("EmptyRef"), wxT("EmptyRef()"));
}

#include "selector/objectSelector.h"

bool CValueManagerDataObjectCatalog::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case eCreateElement:
		pvarRetValue = m_metaObject->CreateObjectValue(eObjectMode::OBJECT_ITEM);
		return true;
	case eCreateGroup:
		pvarRetValue = m_metaObject->CreateObjectValue(eObjectMode::OBJECT_FOLDER);
		return true;
	case eSelect:
		pvarRetValue = CValue::CreateAndPrepareValueRef<CValueSelectorRecordDataObject>(m_metaObject);
		return true;
	case eFindByCode:
		pvarRetValue = FindByCode(*paParams[0]);
		return true;
	case eFindByDescription:
		pvarRetValue = FindByDescription(*paParams[0]);
		return true;
	case eGetForm: {
		CValueGuid* guidVal = lSizeArray > 2 ? paParams[2]->ConvertToType<CValueGuid>() : nullptr;
		pvarRetValue = m_metaObject->GetGenericForm(lSizeArray > 0 ? paParams[0]->GetString() : wxEmptyString,
			lSizeArray > 1 ? paParams[1]->ConvertToType<IBackendControlFrame>() : nullptr,
			guidVal ? ((CGuid)*guidVal) : CGuid());
		return true;
	}
	case eGetListForm: {
		CValueGuid* guidVal = lSizeArray > 2 ? paParams[2]->ConvertToType<CValueGuid>() : nullptr;
		pvarRetValue = m_metaObject->GetListForm(lSizeArray > 0 ? paParams[0]->GetString() : wxEmptyString,
			lSizeArray > 1 ? paParams[1]->ConvertToType<IBackendControlFrame>() : nullptr,
			guidVal ? ((CGuid)*guidVal) : CGuid());
		return true;
	}
	case eGetSelectForm: {
		CValueGuid* guidVal = lSizeArray > 2 ? paParams[2]->ConvertToType<CValueGuid>() : nullptr;
		pvarRetValue = m_metaObject->GetSelectForm(lSizeArray > 0 ? paParams[0]->GetString() : wxEmptyString,
			lSizeArray > 1 ? paParams[1]->ConvertToType<IBackendControlFrame>() : nullptr,
			guidVal ? ((CGuid)*guidVal) : CGuid());
		return true;
	}
	case eGetTemplate:
		pvarRetValue = m_metaObject->GetTemplate(paParams[0]->GetString());
		return true;
	case eEmptyRef:
		pvarRetValue = EmptyRef();
		return true;
	}

	return IValueManagerDataObject::CallAsFunc(lMethodNum, pvarRetValue, paParams, lSizeArray);
}