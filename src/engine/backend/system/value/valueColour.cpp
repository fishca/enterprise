////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : value colour  
////////////////////////////////////////////////////////////////////////////

#include "valueColour.h"



//////////////////////////////////////////////////////////////////////
wxIMPLEMENT_DYNAMIC_CLASS(ibValueColour, ibValue);

ibValue::ibValueMethodHelper ibValueColour::m_methodHelper;

ibValueColour::ibValueColour() :
	ibValue(ibValueTypes::TYPE_VALUE), m_colour()
{
}

ibValueColour::ibValueColour(const wxColour& colour) :
	ibValue(ibValueTypes::TYPE_VALUE), m_colour(colour)
{
}

bool ibValueColour::Init(ibValue** paParams, const long lSizeArray)
{
	if (lSizeArray > 0 && paParams[0]->GetType() == ibValueTypes::TYPE_STRING) {
		m_colour = typeConv::StringToColour(paParams[0]->GetString());
		return true;
	}
	else if (lSizeArray > 2) {
		m_colour = wxColour(
			paParams[0]->GetInteger(),
			paParams[1]->GetInteger(),
			paParams[2]->GetInteger()
		);
		return true;
	}
	return false;
}

enum
{
	enColorR,
	enColorG,
	enColorB
};

void ibValueColour::PrepareNames() const
{
	m_methodHelper.ClearHelper();

	m_methodHelper.AppendProp(wxT("R"));
	m_methodHelper.AppendProp(wxT("G"));
	m_methodHelper.AppendProp(wxT("B"));
}

bool ibValueColour::SetPropVal(const long lPropNum, const ibValue& varPropVal)
{
	switch (lPropNum)
	{
	case enColorR:
		m_colour.Set((unsigned char)varPropVal.GetUInteger(), m_colour.Green(), m_colour.Blue());
		return true;
	case enColorG:
		m_colour.Set(m_colour.Red(), (unsigned char)varPropVal.GetUInteger(), m_colour.Blue());
		return true;
	case enColorB:
		m_colour.Set(m_colour.Red(), m_colour.Green(), (unsigned char)varPropVal.GetUInteger());
		return true;
	}
	return false;
}

bool ibValueColour::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	switch (lPropNum)
	{
	case enColorR:
		pvarPropVal = m_colour.Red();
		return true;
	case enColorG:
		pvarPropVal = m_colour.Green();
		return true;
	case enColorB:
		pvarPropVal = m_colour.Blue();
		return true;
	}
	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

VALUE_TYPE_REGISTER(ibValueColour, "Colour", string_to_clsid("VL_COLOR"));