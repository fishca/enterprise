////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : value size 
////////////////////////////////////////////////////////////////////////////

#include "valueSize.h"

//////////////////////////////////////////////////////////////////////
wxIMPLEMENT_DYNAMIC_CLASS(ibValueSize, ibValue);

ibValue::ibValueMethodHelper ibValueSize::m_methodHelper;

ibValueSize::ibValueSize() : ibValue(ibValueTypes::TYPE_VALUE), m_size(wxDefaultSize)
{
}

ibValueSize::ibValueSize(const wxSize& size) : ibValue(ibValueTypes::TYPE_VALUE), m_size(size)
{
}

bool ibValueSize::Init(ibValue** paParams, const long lSizeArray)
{
	if (lSizeArray == 1 && paParams[0]->GetType() == ibValueTypes::TYPE_STRING) {
		m_size = typeConv::StringToSize(paParams[0]->GetString());
		return true;
	}
	else if (lSizeArray > 1) {
		m_size = wxSize(paParams[0]->GetInteger(), paParams[1]->GetInteger());
		return true;
	}
	return false;
}

enum
{
	eX,
	eY
};

void ibValueSize::PrepareNames() const
{
	m_methodHelper.ClearHelper();

	m_methodHelper.AppendProp(wxT("X"));
	m_methodHelper.AppendProp(wxT("Y"));
}

bool ibValueSize::SetPropVal(const long lPropNum, const ibValue& varPropVal)
{
	switch (lPropNum)
	{
	case eX:
		m_size.x = varPropVal.GetInteger();
		return true;
	case eY:
		m_size.y = varPropVal.GetInteger();
		return true;
	}
	return false;
}

bool ibValueSize::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	switch (lPropNum)
	{
	case eX:
		pvarPropVal = m_size.x;
		return true;
	case eY:
		pvarPropVal = m_size.y;
		return true;
	}
	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

VALUE_TYPE_REGISTER(ibValueSize, "Size", string_to_clsid("VL_SIZE"));