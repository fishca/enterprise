#include "workerPoolGUI.h"

#include "backend/session/session.h"

#include <wx/app.h>

#include <stdexcept>

std::future<void> ibWorkerPoolGUI::Submit(ibSession* /*session*/, Task task)
{
	auto promise = std::make_shared<std::promise<void>>();
	auto future  = promise->get_future();

	if (m_stop.load(std::memory_order_acquire)) {
		promise->set_exception(std::make_exception_ptr(
			std::runtime_error("worker pool is stopped")));
		return future;
	}

	// No wx loop yet (host startup, or headless run mode misconfigured) —
	// run the task inline so the future fulfils immediately. Avoids
	// dropping work on the floor in transitional states.
	if (wxTheApp == nullptr) {
		try {
			task();
			promise->set_value();
		}
		catch (...) {
			promise->set_exception(std::current_exception());
		}
		return future;
	}

	// Schedule on the wx main thread. CallAfter posts a wx event that
	// the next idle pump executes — safe to call from any thread.
	wxTheApp->CallAfter([t = std::move(task), p = promise]() mutable {
		try {
			t();
			p->set_value();
		}
		catch (...) {
			p->set_exception(std::current_exception());
		}
	});

	return future;
}

void ibWorkerPoolGUI::DropSession(ibSession* /*session*/)
{
	// No per-session bookkeeping — tasks share the wx event queue,
	// which has no per-session partition. Nothing to drop.
}

void ibWorkerPoolGUI::CancelSession(ibSession* session)
{
	// Same cooperative-cancel contract as the headless pool: set the
	// flag, the interpreter sees it on the next opcode and unwinds.
	if (session != nullptr)
		session->RequestCancel();
}

void ibWorkerPoolGUI::Stop()
{
	// Flag-only stop. wx handles pending CallAfter events through its
	// own shutdown path — no separate join because there are no OS
	// threads owned by this pool.
	m_stop.store(true, std::memory_order_release);
}
