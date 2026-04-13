////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : main events
////////////////////////////////////////////////////////////////////////////

#include "moduleManager.h"
#include "backend/appData.h"

//*********************************************************************************************************
//*                                   Events "moduleManager"                                              *
//*********************************************************************************************************

bool ibValueModuleManagerConfiguration::BeforeStart()
{
	if (!appData->DesignerMode()) {
		try {
			ibValue bCancel = false;
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

void ibValueModuleManagerConfiguration::OnStart()
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

bool ibValueModuleManagerConfiguration::BeforeExit()
{
	if (!appData->DesignerMode()) {
		try {
			ibValue bCancel = false;
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

void ibValueModuleManagerConfiguration::OnExit()
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