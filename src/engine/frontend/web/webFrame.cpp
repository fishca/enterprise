#include "webFrame.h"

#include <iostream>

#include "backend/backend_form.h"
#include "backend/session/session.h"
#include "backend/compiler/value.h"
#include "backend/metaCollection/metaFormObject.h"   // ibValueMetaObjectFormBase::CreateAndBuildForm

#include "frontend/docView/docView.h"   // ibMetaView
#include "visualView/ctrl/form.h"
#include "visualView/ctrl/frame.h"
#include "visualView/visualHostClient.h"

#include "webChildFrame.h"
#include "webApplication.h"   // for GetSessionContext on GetSession()

ibWebFrame::ibWebFrame(ibWebApplication* app) : m_app(app) {}

ibSession* ibWebFrame::GetSession() const
{
	// Per-cookie ibWebApplication carries the ticket's session (bound
	// at Login time via ibWebSession::Login → SetSessionContext). No
	// app / no session set — return nullptr; callers guard.
	return m_app != nullptr ? m_app->GetSessionContext() : nullptr;
}

ibWebFrame::~ibWebFrame()
{
	// The OnExit path is responsible for running DeleteAllViews on every
	// tab's doc BEFORE deleting the frame — that must happen on the
	// session worker thread so procUnit's per-thread state is valid.
	// By the time we reach this dtor (on the HTTP thread during session
	// destroy), docs/views/hosts are already gone; only the tab shells
	// remain. Dereferencing tab->GetDocument() here would dynamic_cast
	// a dangling pointer and wedge the server.
	//
	// Just clear the tab vector — ~ibWebDocChildFrame only nulls its
	// (already-dangling) m_view/m_doc, no UB.
	m_pendingCloses.clear();
	m_activeForm = nullptr;
	m_tabs.clear();
}

void ibWebFrame::SetStatusText(const wxString& strStatus, int /*number*/)
{
	m_status = strStatus;
}

ibBackendValueForm* ibWebFrame::CreateNewForm(
	const ibValueMetaObjectFormBase* creator,
	ibBackendControlFrame* backendControl,
	ibSourceDataObject*    srcObject,
	const ibUniqueKey&     formGuid)
{
	std::cerr << "[tabs] CreateNewForm creator=" << (void*)creator
		<< " tabs_before=" << m_tabs.size() << std::endl;
	// Low-level factory, mirror of desktop ibFrontendDocMDIFrame::
	// CreateNewForm: just allocates the ibValueForm ref, no LoadFormData
	// / BuildForm. ibValueMetaObjectFormBase::CreateAndBuildForm calls
	// ibBackendValueForm::CreateNewForm which lands here — if we
	// re-dispatched through CreateAndBuildForm from this method we'd
	// recurse infinitely (OnStart script tab-spawn, 2026-04-19). Callers
	// that need BuildForm fallback (sidebar click via OpenFormInSession)
	// invoke CreateAndBuildForm themselves above this call.
	//
	// Parent descriptor wiring happens inside ibValueForm — it has
	// ownerControl + access to backend_mainFrame for the UI fallback.
	ibControlFrame* ownerControl = dynamic_cast<ibControlFrame*>(backendControl);
	return ibValue::CreateAndPrepareValueRef<ibValueForm>(
		creator, ownerControl, srcObject, formGuid);
}

ibUniqueKey ibWebFrame::CreateFormUniqueKey(
	const ibBackendControlFrame* ownerControl,
	const ibSourceDataObject* sourceObject,
	const ibUniqueKey& formGuid)
{
	// Same fallback chain desktop uses (formGuid → ownerControl
	// guid → sourceObject guid → fresh random guid). Without this,
	// ibBackendDocFrame's default returns wxNullUniqueKey for
	// every form, breaking FindDocByUniqueKey.
	return ibFormVisualDocument::CreateFormUniqueKey(ownerControl, sourceObject, formGuid);
}

void ibWebFrame::AdoptTab(std::unique_ptr<ibWebDocChildFrame> tab, ibValueForm* form)
{
	if (tab == nullptr) return;
	m_activeTab  = m_tabs.size();
	m_tabs.push_back(std::move(tab));
	if (form != nullptr) m_activeForm = form;
	std::cerr << "[tabs] AdoptTab form=" << (void*)form
		<< " now " << m_tabs.size() << " tab(s)" << std::endl;
}

ibFrontendWindow* ibWebFrame::CreateChildFrame(
	ibMetaView* view,
	const wxPoint& /*pos*/,
	const wxSize&  /*size*/,
	long           /*style*/)
{
	// Web-side static factory — mirror of the desktop
	// ibFrontendDocMDIFrame::CreateChildFrame. Works with any
	// ibMetaDocument subclass (form, tabular, text, report …),
	// not just ibFormVisualDocument. Title comes from the doc's
	// GetTitle(); any per-type-specific wiring (e.g. ibValueForm
	// tracking for ActiveWindow() on form tabs) happens in a
	// narrow downcast below, leaving non-form docs untouched.
	if (view == nullptr) return nullptr;

	// Per-tab session is pinned by the worker loop's ibSessionScope; its
	// GetFrame() is our ibWebFrame. No helper needed — direct path.
	ibSession* session = ibSession::Current();
	auto* webFrame = session != nullptr
		? dynamic_cast<ibWebFrame*>(session->GetFrame()) : nullptr;
	if (webFrame == nullptr) return nullptr;

	ibMetaDocument* doc = view->GetDocument();
	// doc->GetTitle() is still empty at this stage — the doc was just
	// constructed and its title is set by ibFormVisualDocument::OnCreate,
	// which the caller invokes AFTER CreateChildFrame. Pull the title
	// from the ibValueForm directly when the doc is a form doc, so the
	// tab strip doesn't flash "#0/#1/..." (its fallback when both
	// t.title and t.formName are empty) during the first serialize.
	wxString title = doc != nullptr ? doc->GetTitle() : wxString();
	if (title.IsEmpty()) {
		if (auto* formDoc = dynamic_cast<ibFormVisualDocument*>(doc)) {
			if (ibValueForm* valueForm = formDoc->GetValueForm())
				title = valueForm->GetControlTitle();
		}
	}

	auto childFrame = std::make_unique<ibWebDocChildFrame>(
		doc, view, /*parent*/ webFrame, title);
	ibWebDocChildFrame* raw = childFrame.get();

	// Wire the view's web-frame back-pointer so ibMetaView::ShowFrame
	// can reach this tab and flip its shown/active state. Desktop gets
	// the same effect via wxView::SetFrame (done by wxDocChildFrame
	// ctor); web plumbs it explicitly here.
	view->SetWebFrame(raw);

	// Only form-docs feed the frame's ActiveWindow()-tracking slot;
	// other doc types (future tabular / text / report views) still
	// get a tab but don't pollute m_activeForm.
	ibValueForm* form = nullptr;
	if (auto* visualDoc = dynamic_cast<ibFormVisualDocument*>(doc))
		form = visualDoc->GetValueForm();

	// Pull the tab icon from the form the same way wxAuiMDIChildFrame
	// would on desktop. ibValueForm::GetIcon falls back to a built-in
	// XPM when the metaForm has no custom image, so the tab always has
	// something to show if the client supports icons.
	if (form != nullptr) {
		const wxIcon icon = form->GetIcon();
		if (icon.IsOk())
			raw->SetIcon(icon);
	}

	webFrame->AdoptTab(std::move(childFrame), form);
	return raw;
}

void ibWebFrame::SetActiveTab(std::size_t i)
{
	if (i >= m_tabs.size())
		return;
	m_activeTab = i;
	ibVisualHostClient* host = m_tabs[i]->GetHost();
	m_activeForm = host != nullptr ? host->GetValueForm() : nullptr;
}

void ibWebFrame::MarkTabForCloseByForm(const ibValueForm* form)
{
	if (form == nullptr) return;
	// Dedup — the same form may be marked twice (e.g. beforeClose
	// script calls CloseForm again).
	for (const ibValueForm* q : m_pendingCloses)
		if (q == form) return;
	m_pendingCloses.push_back(form);
}

void ibWebFrame::DrainPendingCloses()
{
	if (m_pendingCloses.empty()) return;
	std::vector<const ibValueForm*> pending;
	pending.swap(m_pendingCloses);
	std::cerr << "[life] DrainPendingCloses count=" << pending.size() << std::endl;
	for (const ibValueForm* form : pending) {
		for (std::size_t i = 0; i < m_tabs.size(); ++i) {
			auto* visualDoc = dynamic_cast<ibFormVisualDocument*>(
				m_tabs[i]->GetDocument());
			if (visualDoc == nullptr || visualDoc->GetValueForm() != form)
				continue;
			std::cerr << "[life] DeleteAllViews form=" << (void*)form
				<< " doc=" << (void*)visualDoc << std::endl;
			// Now — outside any control's event handler — we can safely
			// destroy the view, which cascades into host and all its
			// child controls (toolbar, tools, textctrl etc.). wxDocument's
			// DeleteAllViews contract also deletes the doc itself when
			// the last view goes, so m_tabs[i]->m_doc becomes dangling
			// (tab dtor nulls it, doesn't delete).
			visualDoc->DeleteAllViews();
			std::cerr << "[life] after DeleteAllViews" << std::endl;

			if (i == m_activeTab)
				m_activeForm = nullptr;
			std::unique_ptr<ibWebDocChildFrame> dying =
				std::move(*(m_tabs.begin() + i));
			m_tabs.erase(m_tabs.begin() + i);
			if (m_tabs.empty()) {
				m_activeTab  = 0;
				m_activeForm = nullptr;
			} else {
				if (m_activeTab >= m_tabs.size())
					m_activeTab = m_tabs.size() - 1;
				auto* newDoc = dynamic_cast<ibFormVisualDocument*>(
					m_tabs[m_activeTab]->GetDocument());
				m_activeForm = newDoc != nullptr ? newDoc->GetValueForm() : nullptr;
			}
			break;
		}
	}
}

bool ibWebFrame::CloseTab(std::size_t i)
{
	if (i >= m_tabs.size())
		return false;

	// Single path for BOTH the toolbar ActionClose and the browser's tab-X:
	// call ibValueForm::CloseForm (which fires beforeClose with veto,
	// then onClose) and mark-for-deferred-close via MarkTabForCloseByForm.
	// Do NOT erase from m_tabs here — if we did, DrainPendingCloses' tab
	// lookup would fail (tab is already gone) and nobody would call
	// DeleteAllViews, so the view / host / ibFormVisualDocument /
	// ibValueForm would all leak. DrainPendingCloses runs at the end of
	// Dispatch and does the real teardown (DeleteAllViews cascade) plus
	// the erase + active-tab fixup.
	ibVisualHostClient* dyingHost = m_tabs[i]->GetHost();
	ibValueForm* dyingForm = dyingHost != nullptr ? dyingHost->GetValueForm() : nullptr;
	if (dyingForm != nullptr && !dyingForm->CloseForm())
		return false;   // script vetoed — tab stays
	return true;
}
