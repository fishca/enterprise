////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : constants manager
////////////////////////////////////////////////////////////////////////////

#include "constantManager.h"

wxIMPLEMENT_DYNAMIC_CLASS(CManagerDataObjectConstant, CValue);

CManagerDataObjectConstant::CManagerDataObjectConstant(CMetaObjectConstant* metaConst) :
	m_metaConst(metaConst)
{
}

CManagerDataObjectConstant::~CManagerDataObjectConstant()
{
}

#include "backend/metaData.h"
#include "backend/objCtor.h"

class_identifier_t CManagerDataObjectConstant::GetClassType() const
{
	IMetaData* metaData = m_metaConst->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(m_metaConst, eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CManagerDataObjectConstant::GetClassName() const
{
	IMetaData* metaData = m_metaConst->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(m_metaConst, eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CManagerDataObjectConstant::GetString() const
{
	IMetaData* metaData = m_metaConst->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(m_metaConst, eCtorMetaType::eCtorMetaType_Manager);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

CValue::CMethodHelper CManagerDataObjectConstant::m_methodHelper;

enum Func {
	enSet = 0,
	enGet
};

void CManagerDataObjectConstant::PrepareNames() const
{
	m_methodHelper.ClearHelper();
	m_methodHelper.AppendFunc("set", 1, "set(value)");
	m_methodHelper.AppendFunc("get", "get()");
}

bool CManagerDataObjectConstant::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	CValuePtr<CRecordDataObjectConstant> recordDataObjectValue = m_metaConst->CreateRecordDataObjectValue();

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