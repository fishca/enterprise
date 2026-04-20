////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : main events
////////////////////////////////////////////////////////////////////////////

#include "moduleManager.h"
#include "backend/appData.h"

//*********************************************************************************************************
//*                                   Events "moduleManager"                                              *
//*********************************************************************************************************

// Events fire on the current session's ProcUnit via GetProcUnit()
// delegate — handles both the desktop legacy path (same ProcUnit
// always) and the per-session path (InitRuntimeForSession populates
// session's m_procUnitMap).
bool ibValueModuleManagerConfiguration::BeforeStart()
{
	if (appData->DesignerMode())
		return true;

	try {
		ibValue bCancel = false;
		if (ibProcUnit* pu = GetProcUnit()) {
			pu->CallAsProc(wxT("beforeStart"), bCancel);
		}
		return !bCancel.GetBoolean();
	}
	catch (...) {
		return false;
	}
}

void ibValueModuleManagerConfiguration::OnStart()
{
	if (appData->DesignerMode())
		return;

	try {
		if (ibProcUnit* pu = GetProcUnit()) {
			pu->CallAsProc(wxT("onStart"));
		}
	}
	catch (...) {
	}
}

bool ibValueModuleManagerConfiguration::BeforeExit()
{
	if (appData->DesignerMode())
		return true;

	try {
		ibValue bCancel = false;
		if (ibProcUnit* pu = GetProcUnit()) {
			pu->CallAsProc(wxT("beforeExit"), bCancel);
		}
		return !bCancel.GetBoolean();
	}
	catch (...) {
		return false;
	}
}

void ibValueModuleManagerConfiguration::OnExit()
{
	if (appData->DesignerMode())
		return;

	try {
		if (ibProcUnit* pu = GetProcUnit()) {
			pu->CallAsProc(wxT("onExit"));
		}
	}
	catch (...) {
	}
}
