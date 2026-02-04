////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : catalog manager
////////////////////////////////////////////////////////////////////////////

#include "catalogManager.h"
#include "backend/metaData.h"

#include "commonObject.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueManagerDataObjectCatalog, CValue);

CValueManagerDataObjectCatalog::CValueManagerDataObjectCatalog(CValueMetaObjectCatalog* metaObject) : 
	m_methodHelper(new CMethodHelper()), m_metaObject(metaObject)
{
}

CValueManagerDataObjectCatalog::~CValueManagerDataObjectCatalog()
{
	wxDELETE(m_methodHelper);
}

CValueMetaObjectCommonModule* CValueManagerDataObjectCatalog::GetModuleManager() const { return m_metaObject->GetModuleManager(); }

#include "reference/reference.h"

CValueReferenceDataObject* CValueManagerDataObjectCatalog::EmptyRef() const 
{
	return CValueReferenceDataObject::Create(m_metaObject);
}

#include "backend/objCtor.h"

class_identifier_t CValueManagerDataObjectCatalog::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CValueManagerDataObjectCatalog::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CValueManagerDataObjectCatalog::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
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
	eEmptyRef
};

void CValueManagerDataObjectCatalog::PrepareNames() const
{
	IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);
	IValueModuleManager* moduleManager = metaData->GetModuleManager();
	wxASSERT(moduleManager);

	m_methodHelper->ClearHelper();
	m_methodHelper->AppendFunc("createElement", "createElement()");
	m_methodHelper->AppendFunc("createGroup", "createGroup()");
	m_methodHelper->AppendFunc("select", "select()");
	m_methodHelper->AppendFunc("findByCode", 1, "findByCode(string)");
	m_methodHelper->AppendFunc("findByDescription", 1, "findByDescription(string)");
	m_methodHelper->AppendFunc("getForm", 3, "getForm(string, owner, guid)");
	m_methodHelper->AppendFunc("getListForm", 3, "getListForm(string, owner, guid)");
	m_methodHelper->AppendFunc("getSelectForm", 3, "getSelectForm(string, owner, guid)");
	m_methodHelper->AppendFunc("emptyRef", "emptyRef()");

	CValue* pRefData = moduleManager->FindCommonModule(m_metaObject->GetModuleManager());
	if (pRefData != nullptr) {
		// add methods from context
		for (long idx = 0; idx < pRefData->GetNMethods(); idx++) {
			m_methodHelper->CopyMethod(pRefData->GetPMethods(), idx);
		}
	}
}

#include "selector/objectSelector.h"

bool CValueManagerDataObjectCatalog::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);

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
	case eEmptyRef:
		pvarRetValue = EmptyRef();
		return true; 
	}

	IValueModuleManager* moduleManager = metaData->GetModuleManager();
	wxASSERT(moduleManager);

	CValue* pRefData =
		moduleManager->FindCommonModule(m_metaObject->GetModuleManager());
	if (pRefData != nullptr)
		return pRefData->CallAsFunc(lMethodNum, pvarRetValue, paParams, lSizeArray);
	return false;
}