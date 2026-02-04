////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : main events
////////////////////////////////////////////////////////////////////////////

#include "moduleManager.h"
#include "backend/appData.h"

//*********************************************************************************************************
//*                                   Events "moduleManager"                                              *
//*********************************************************************************************************

bool CValueModuleManagerConfiguration::BeforeStart()
{
	if (!appData->DesignerMode()) {
		try {
			CValue bCancel = false;
			if (m_procUnit != nullptr) {
				m_procUnit->CallAsProc(wxT("beforeStart"), bCancel);
			}
			return !bCancel.GetBoolean();
		}
		catch (...)
		{
			return false;
		};
	};

	return true;
}

void CValueModuleManagerConfiguration::OnStart()
{
	if (!appData->DesignerMode()) {
		try {
			if (m_procUnit != nullptr) {
				m_procUnit->CallAsProc(wxT("onStart"));
			}
		}
		catch (...) {
		};
	};
}

bool CValueModuleManagerConfiguration::BeforeExit()
{
	if (!appData->DesignerMode()) {
		try {
			CValue bCancel = false;
			if (m_procUnit != nullptr) {
				m_procUnit->CallAsProc(wxT("beforeExit"), bCancel);
			}
			return !bCancel.GetBoolean();
		}
		catch (...) {
			return false;
		};
	};

	return true;
}

void CValueModuleManagerConfiguration::OnExit()
{
	if (!appData->DesignerMode()) {
		try {
			if (m_procUnit != nullptr) {
				m_procUnit->CallAsProc(wxT("onExit"));
			}
		}
		catch (...) {
		};
	};
}