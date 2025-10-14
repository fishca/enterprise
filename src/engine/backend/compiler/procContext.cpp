#include "procContext.h"
#include "procUnit.h"

//*************************************************************************************************
//*                                        RunContextSmall                                        *
//*************************************************************************************************

CRunContextSmall::~CRunContextSmall()
{
	if (m_lVarCount > MAX_STATIC_VAR) {
		wxDELETEA(m_pLocVars);
		wxDELETEA(m_pRefLocVars);
	}
}

//*************************************************************************************************
//*                                        RunContext                                             *
//*************************************************************************************************

CRunContext::~CRunContext()
{
	//erase loc vars 
	if (m_lVarCount > MAX_STATIC_VAR) {
		wxDELETEA(m_pLocVars);
		wxDELETEA(m_pRefLocVars);
	}
}

CByteCode* CRunContext::GetByteCode() const
{
	return m_procUnit != nullptr ?
		m_procUnit->GetByteCode() : nullptr;
}