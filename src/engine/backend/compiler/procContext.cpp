#include "procContext.h"
#include "procUnit.h"

//*************************************************************************************************
//*                                        RunContextSmall                                        *
//*************************************************************************************************

ibRunContextSmall::~ibRunContextSmall()
{
	if (m_lVarCount > MAX_STATIC_VAR) {
		wxDELETEA(m_pLocVars);
		wxDELETEA(m_pRefLocVars);
	}
}

//*************************************************************************************************
//*                                        RunContext                                             *
//*************************************************************************************************

ibRunContext::~ibRunContext()
{
	//erase loc vars 
	if (m_lVarCount > MAX_STATIC_VAR) {
		wxDELETEA(m_pLocVars);
		wxDELETEA(m_pRefLocVars);
	}
}

const ibByteCode* ibRunContext::GetByteCode() const
{
	return m_procUnit != nullptr ?
		m_procUnit->GetByteCode() : nullptr;
}