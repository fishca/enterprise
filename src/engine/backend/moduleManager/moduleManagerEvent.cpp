////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : main events
////////////////////////////////////////////////////////////////////////////

#include "moduleManager.h"
#include "backend/appData.h"
#include "backend/session/session.h"
#include <iostream>

//*********************************************************************************************************
//*                                   Events "moduleManager"                                              *
//*********************************************************************************************************

// Events fire on the current session's ProcUnit via GetProcUnit()
// delegate — handles both the desktop legacy path (same ProcUnit
// always) and the per-session path (InitRuntimeForSession populates
// session's m_procUnitMap).
// Each handler holds a local shared_ptr for the entire CallAsProc —
// without the hold, a concurrent session teardown (Remove@Urgent from
// fast-F5 /logout) would drop the last ref via DetachProcUnit and
// leave the pointer dangling mid-Execute. See
// project_refresh_execute_crash.md.
//
// The handlers also take m_runtimeMutex — same lock that
// Init/ExitRuntimeForSession uses — because CallAsProc enters
// ibProcUnit::Execute which MUTATES shared compileModule state
// (procUnit.cpp:1025 writes m_listConst[i].m_bReadOnly; :1029 reads
// m_compileModule->m_rootContext). Two concurrent logins under rapid
// F5 had one session's Execute hit operator[] on m_listCode while
// another session's Execute was in the same function, producing an
// SEH 0xc0000005 inside vector<ibByteUnit>::operator[]. Serializing
// startup across sessions is cheap (login is rare) and eliminates the
// class of races without the full bytecode↔compileModule decoupling
// described in project_bytecode_compile_decoupling.md.
bool ibValueModuleManagerConfiguration::BeforeStart()
{
	if (appData->DesignerMode())
		return true;

	try {
		ibValue bCancel = false;
		std::lock_guard<std::mutex> lock(m_runtimeMutex);
		auto pu = GetProcUnit();
		std::cerr << "[BeforeStart] mgr=" << this
		          << " current=" << ibSession::Current()
		          << " pu=" << pu.get()
		          << " pu.byteCode=" << (pu ? pu->GetByteCode() : nullptr)
		          << std::endl;
		if (pu) {
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
