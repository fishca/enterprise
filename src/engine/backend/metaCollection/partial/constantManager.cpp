////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : constants manager
////////////////////////////////////////////////////////////////////////////

#include "constantManager.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueManagerDataObjectConstant, CValue);

#include "backend/metaData.h"
#include "backend/objCtor.h"

class_identifier_t CValueManagerDataObjectConstant::GetClassType() const
{
	const IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(m_metaObject, eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CValueManagerDataObjectConstant::GetClassName() const
{
	const IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(m_metaObject, eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CValueManagerDataObjectConstant::GetString() const
{
	const IMetaData* metaData = m_metaObject->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(m_metaObject, eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

CValue::CMethodHelper CValueManagerDataObjectConstant::m_methodHelper;

enum Func {
	enSet = 0,
	enGet
};

void CValueManagerDataObjectConstant::PrepareNames() const
{
	m_methodHelper.ClearHelper();
	m_methodHelper.AppendFunc(wxT("Set"), 1, wxT("Set(value : any)"));
	m_methodHelper.AppendFunc(wxT("Get"), wxT("Get()"));
}

bool CValueManagerDataObjectConstant::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	CValuePtr<CValueRecordDataObjectConstant> recordDataObjectValue = m_metaObject->CreateRecordDataObjectValue();

	switch (lMethodNum)
	{
	case enSet:
		recordDataObjectValue->SetConstValue(*paParams[0]);
		return true;
	case enGet:
		pvarRetValue = recordDataObjectValue->GetConstValue();
		return true;
	}

	return false;
}