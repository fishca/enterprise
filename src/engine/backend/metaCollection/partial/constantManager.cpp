////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : constants manager
////////////////////////////////////////////////////////////////////////////

#include "constantManager.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueManagerDataObjectConstant, CValue);

CValueManagerDataObjectConstant::CValueManagerDataObjectConstant(CValueMetaObjectConstant* metaConst) :
	m_metaConst(metaConst)
{
}

CValueManagerDataObjectConstant::~CValueManagerDataObjectConstant()
{
}

#include "backend/metaData.h"
#include "backend/objCtor.h"

class_identifier_t CValueManagerDataObjectConstant::GetClassType() const
{
	IMetaData* metaData = m_metaConst->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(m_metaConst, eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CValueManagerDataObjectConstant::GetClassName() const
{
	IMetaData* metaData = m_metaConst->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(m_metaConst, eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CValueManagerDataObjectConstant::GetString() const
{
	IMetaData* metaData = m_metaConst->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(m_metaConst, eCtorMetaType::eCtorMetaType_Manager);
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
	CValuePtr<CValueRecordDataObjectConstant> recordDataObjectValue = m_metaConst->CreateRecordDataObjectValue();

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