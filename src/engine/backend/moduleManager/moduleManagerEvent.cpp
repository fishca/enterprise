////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : main events
////////////////////////////////////////////////////////////////////////////

#include "moduleManager.h"
#include "backend/appData.h"

//*********************************************************************************************************
//*                                   Events "moduleManager"                                              *
//*********************************************************************************************************

// Events fire on the root's own ProcUnit (m_procUnit — populated by
// InitRuntimeForSession). Each handler holds a local shared_ptr for
// the entire CallAsProc — without the hold, a concurrent teardown
// (Remove@Urgent from fast-F5 /logout) would drop the last ref and
// leave the pointer dangling mid-Execute. See
// project_refresh_execute_crash.md.
//
// Handlers take m_runtimeMutex — same lock as Init/ExitRuntimeForSession
// — because CallAsProc enters ibProcUnit::Execute which mutates shared
// compileModule state (procUnit.cpp:1025 writes m_listConst[i].m_bReadOnly;
// :1029 reads m_compileModule->m_rootContext). Two concurrent logins
// under rapid F5 had one session's Execute hit operator[] on m_listCode
// while another session's Execute was in the same function. Serializing
// startup is cheap (login is rare) and eliminates the race class without
// the full bytecode↔compileModule decoupling
// (project_bytecode_compile_decoupling.md).
bool ibValueModuleManagerConfiguration::BeforeStart()
{
	if (appData->DesignerMode())
		return true;

	try {
		ibValue bCancel = false;
		std::lock_guard<std::mutex> lock(m_runtimeMutex);
		if (auto pu = GetProcUnit())
			pu->CallAsProc(wxT("beforeStart"), bCancel);
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
		std::lock_guard<std::mutex> lock(m_runtimeMutex);
		if (auto pu = GetProcUnit()) {
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
		std::lock_guard<std::mutex> lock(m_runtimeMutex);
		if (auto pu = GetProcUnit()) {
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
		std::lock_guard<std::mutex> lock(m_runtimeMutex);
		if (auto pu = GetProcUnit()) {
			pu->CallAsProc(wxT("onExit"));
		}
	}
	catch (...) {
	}
}
