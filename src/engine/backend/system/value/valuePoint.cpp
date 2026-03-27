////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : value point 
////////////////////////////////////////////////////////////////////////////

#include "valuePoint.h"

//////////////////////////////////////////////////////////////////////
wxIMPLEMENT_DYNAMIC_CLASS(ibValuePoint, ibValue);

ibValue::ibValueMethodHelper ibValuePoint::m_methodHelper;

ibValuePoint::ibValuePoint() : ibValue(ibValueTypes::TYPE_VALUE), m_point(wxDefaultPosition)
{
}

ibValuePoint::ibValuePoint(const wxPoint& point) : ibValue(ibValueTypes::TYPE_VALUE), m_point(point)
{
}

bool ibValuePoint::Init(ibValue** paParams, const long lSizeArray)
{
	if (lSizeArray == 1 && paParams[0]->GetType() == ibValueTypes::TYPE_STRING) {
		m_point = typeConv::StringToPoint(paParams[0]->GetString());
		return true;
	}
	else if (lSizeArray > 1) {
		m_point = wxPoint(paParams[0]->GetInteger(), paParams[1]->GetInteger());
		return true;
	}
	return false;
}

enum
{
	eX,
	eY
};

void ibValuePoint::PrepareNames() const
{
	m_methodHelper.ClearHelper();

	m_methodHelper.AppendProp(wxT("X"));
	m_methodHelper.AppendProp(wxT("Y"));
}

bool ibValuePoint::SetPropVal(const long lPropNum, const ibValue& varPropVal)
{
	switch (lPropNum)
	{
	case eX:
		m_point.x = varPropVal.GetInteger();
		return true;
	case eY:
		m_point.y = varPropVal.GetInteger();
		return true;
	}

	return false;
}

bool ibValuePoint::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	switch (lPropNum)
	{
	case eX:
		pvarPropVal = m_point.x;
		return true;
	case eY:
		pvarPropVal = m_point.y;
		return true;
	}

	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

VALUE_TYPE_REGISTER(ibValuePoint, "Point", string_to_clsid("VL_PONT"));