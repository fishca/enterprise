////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : enum factory 
////////////////////////////////////////////////////////////////////////////

#include "enumFactory.h"
#include "backend/propertyManager/propertyManager.h"

//*********************************************************************************************************
//*                                   Singleton class "enumFactory"                                       *
//*********************************************************************************************************

ibValueEnumFactory::ibValueEnumFactory() :
	ibValue(ibValueTypes::TYPE_VALUE, true), m_methodHelper(new ibValueMethodHelper())
{
	for (auto &ctor : ibValue::GetListCtorsByType(ibCtorObjectType_object_enum)) {
		m_methodHelper->AppendProp(ctor->GetClassName());
	}
}

ibValueEnumFactory::~ibValueEnumFactory() {
	wxDELETE(m_methodHelper);
}

void ibValueEnumFactory::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	for (auto& ctor : ibValue::GetListCtorsByType(ibCtorObjectType_object_enum)) {
		m_methodHelper->AppendProp(ctor->GetClassName());
	}
}

bool ibValueEnumFactory::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	const wxString &strEnumeration = GetPropName(lPropNum);
	if (!ibValue::IsRegisterCtor(strEnumeration))
		return false;
	pvarPropVal = ibValue::CreateObject(strEnumeration);
	return true;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

CONTEXT_TYPE_REGISTER(ibValueEnumFactory, "EnumManager", string_to_clsid("CO_ENMR"));
