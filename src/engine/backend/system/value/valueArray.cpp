////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : array value   
////////////////////////////////////////////////////////////////////////////

#include "valueArray.h"
#include "backend/backend_exception.h"


wxIMPLEMENT_DYNAMIC_CLASS(ibValueArray, ibValue);

//////////////////////////////////////////////////////////////////////

ibValue::ibValueMethodHelper ibValueArray::m_methodHelper;

bool ibValueArray::Init()
{
	return true;
}

bool ibValueArray::Init(ibValue** paParams, const long lSizeArray)
{
	if (lSizeArray < 1)
		return false;

	if (paParams[0]->GetType() == ibValueTypes::TYPE_NUMBER) {
		const ibNumber& number = paParams[0]->GetNumber();
		if (number > 0) {
			m_listValue.resize(number.ToUInt());
			return true;
		}
	}
	return false;
}

#include "appdata.h"

void ibValueArray::CheckIndex(unsigned int index) const //array index must start from 1
{
	if ((index < 0 || index >= m_listValue.size() && !appData->DesignerMode()))
		ibBackendCoreException::Error(_("Index goes beyond array"));
}

//working with an array as an aggregate object
//listing string keys
void ibValueArray::PrepareNames() const
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

bool ibValueArray::CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray)
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

bool ibValueArray::GetAt(const ibValue& varKeyValue, ibValue& pvarValue) //array index must start from 0
{
	CheckIndex(varKeyValue.GetUInteger());
	pvarValue = m_listValue[varKeyValue.GetUInteger()];
	return true;
}

bool ibValueArray::SetAt(const ibValue& varKeyValue, const ibValue& varValue)//array index must start from 0
{
	CheckIndex(varKeyValue.GetUInteger());
	m_listValue[varKeyValue.GetUInteger()] = varValue;
	return true;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

VALUE_TYPE_REGISTER(ibValueArray, "Array", string_to_clsid("VL_ARR"));