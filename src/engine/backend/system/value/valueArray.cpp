////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : array value   
////////////////////////////////////////////////////////////////////////////

#include "valueArray.h"
#include "backend/backend_exception.h"


wxIMPLEMENT_DYNAMIC_CLASS(CValueArray, CValue);

//////////////////////////////////////////////////////////////////////

CValue::CMethodHelper CValueArray::m_methodHelper;

bool CValueArray::Init()
{
	return true;
}

bool CValueArray::Init(CValue** paParams, const long lSizeArray)
{
	if (lSizeArray < 1)
		return false;

	if (paParams[0]->GetType() == eValueTypes::TYPE_NUMBER) {
		const number_t& number = paParams[0]->GetNumber();
		if (number > 0) {
			m_listValue.resize(number.ToUInt());
			return true;
		}
	}
	return false;
}

#include "appdata.h"

void CValueArray::CheckIndex(unsigned int index) const //array index must start from 1
{
	if ((index < 0 || index >= m_listValue.size() && !appData->DesignerMode()))
		CBackendCoreException::Error(_("Index goes beyond array"));
}

//working with an array as an aggregate object
//listing string keys
void CValueArray::PrepareNames() const
{
	m_methodHelper.ClearHelper();
	m_methodHelper.AppendConstructor(1, wxT("Array(num : number)"));

	m_methodHelper.AppendFunc(wxT("Add"), 1, wxT("Add(value : any)"));
	m_methodHelper.AppendFunc(wxT("Insert"), 2, wxT("Insert(index, value : any)"));

	m_methodHelper.AppendFunc(wxT("Count"), wxT("Count()"));
	m_methodHelper.AppendFunc(wxT("Find"), 1, wxT("Find(value : any)"));
	m_methodHelper.AppendFunc(wxT("Clear"), wxT("Clear()"));
	m_methodHelper.AppendFunc(wxT("Get"), 1, wxT("Get(index)"));
	m_methodHelper.AppendFunc(wxT("Set"), 2, wxT("Set(index, value : any)"));
	m_methodHelper.AppendFunc(wxT("Remove"), 1, wxT("Remove(index : number)"));
}

bool CValueArray::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enAdd: 
		Add(*paParams[0]);
		return true;
	case enInsert: 
		Insert(paParams[0]->GetUInteger(), *paParams[1]); 
		return true;
	case enCount:
		pvarRetValue = Count();
		return true;
	case enFind: 
		pvarRetValue = Find(*paParams[0]);
		return true;
	case enClear: 
		Clear(); 
		return true;
	case enGet:
		return GetAt(*paParams[0], pvarRetValue);
	case enSet:  
		return SetAt(*paParams[0], *paParams[1]);
	case enRemove: 
		Remove(paParams[0]->GetUInteger());
		return true;
	}

	return false;
}

bool CValueArray::GetAt(const CValue& varKeyValue, CValue& pvarValue) //array index must start from 0
{
	CheckIndex(varKeyValue.GetUInteger());
	pvarValue = m_listValue[varKeyValue.GetUInteger()];
	return true;
}

bool CValueArray::SetAt(const CValue& varKeyValue, const CValue& varValue)//array index must start from 0
{
	CheckIndex(varKeyValue.GetUInteger());
	m_listValue[varKeyValue.GetUInteger()] = varValue;
	return true;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

VALUE_TYPE_REGISTER(CValueArray, "Array", string_to_clsid("VL_ARR"));