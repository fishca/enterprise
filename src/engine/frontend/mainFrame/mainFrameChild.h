#ifndef _MAINFRAMECHILD_H__
#define _MAINFRAMECHILD_H__

#include <wx/aui/aui.h>
#include <wx/docview.h>
#include <wx/cmdproc.h>

#include "frontend/frontend.h"

class FRONTEND_API CAuiMDIChildFrame :
	public wxAuiMDIChildFrame {
public:

	CAuiMDIChildFrame() : wxAuiMDIChildFrame() {}
	CAuiMDIChildFrame(wxAuiMDIParentFrame* parent,
		wxWindowID winid,
		const wxString& title,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_FRAME_STYLE,
		const wxString& name = wxASCII_STR(wxFrameNameStr))
		:
		wxAuiMDIChildFrame(parent, winid, title, pos, size, style, name) {
	}

	virtual ~CAuiMDIChildFrame() {}

	bool Create(wxAuiMDIParentFrame* parent,
		wxWindowID id,
		const wxString& title,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_FRAME_STYLE,
		const wxString& name = wxASCII_STR(wxFrameNameStr)) {

		wxAuiMDIClientWindow* pClientWindow = parent->GetClientWindow();
		wxASSERT_MSG((pClientWindow != NULL), wxT("Missing MDI client window."));

		// see comment in constructor
		if (style & wxMINIMIZE)
			m_activateOnCreate = false;

		// create the window hidden to prevent flicker
		wxWindow::Show(false);
		wxWindow::Create(pClientWindow,
			id,
			wxDefaultPosition,
			size,
			wxNO_BORDER, name);

		SetMDIParentFrame(parent);

		m_title = title;

		return true;
	}

	virtual bool Show(bool show = true) override {

		wxAuiMDIClientWindow* pClientWindow = m_pMDIParentFrame->GetClientWindow();
		wxASSERT_MSG((pClientWindow != NULL), wxT("Missing MDI client window."));

		pClientWindow->Freeze();

		if (wxAuiMDIChildFrame::Show(show)) {

			const size_t idx = pClientWindow->FindPage(this);

			if (show && idx == wxNOT_FOUND) {

				const wxSize sizeIcon(wxSystemSettings::GetMetric(wxSYS_SMALLICON_X, this),
					wxSystemSettings::GetMetric(wxSYS_SMALLICON_Y, this));

				wxBitmap mdiChildIcon;
				mdiChildIcon.CopyFromIcon(m_icons.GetIcon(sizeIcon));
				pClientWindow->AddPage(this, m_title, m_activateOnCreate, mdiChildIcon);

			}
			else if (!show && idx != wxNOT_FOUND) {
				pClientWindow->RemovePage(idx);
			}

			// Check that the parent notion of the active child coincides with our one.
			// This is less obvious that it seems because we must honour
			// m_activateOnCreate flag but only if it's not the first child because
			// this one becomes active unconditionally.
			wxASSERT_MSG
			(
				(m_activateOnCreate || pClientWindow->GetPageCount() == 1)
				== (m_pMDIParentFrame->GetActiveChild() == this),
				wxS("Logic error: child [not] activated when it should [not] have been.")
			);
		}

		pClientWindow->Thaw();
		pClientWindow->Update();
		return true;
	}

	virtual bool Destroy() override {

		wxAuiMDIParentFrame* pParentFrame = GetMDIParentFrame();
		wxASSERT_MSG(pParentFrame, wxT("Missing MDI Parent Frame"));

		wxAuiMDIClientWindow* pClientWindow = pParentFrame->GetClientWindow();
		wxASSERT_MSG(pClientWindow, wxT("Missing MDI Client Window"));

		pClientWindow->Freeze();

		if (pParentFrame->GetActiveChild() == this) {
			// deactivate ourself
			wxActivateEvent event(wxEVT_ACTIVATE, false, GetId());
			event.SetEventObject(this);
			GetEventHandler()->ProcessEvent(event);

			pParentFrame->SetChildMenuBar(nullptr);
		}

		wxAuiTabCtrl* tabCtrl = nullptr;

		bool success = false; int page_tab_idx = 0;
		if (pClientWindow->FindTab(this, &tabCtrl, &page_tab_idx)) {

			// state the window hidden to prevent flicker
			if (pClientWindow->GetPageCount() == 1)
				tabCtrl->Show(false);

			const int page_idx = pClientWindow->GetPageIndex(this);
			success = page_idx != wxNOT_FOUND ? 
				pClientWindow->DeletePage(page_idx) : false;
		}

		pClientWindow->Thaw();
		pClientWindow->Update();
		return success;
	}

	virtual void SetIcons(const wxIconBundle& icons) override {

		m_icons = icons;
		wxAuiMDIChildFrame::SetIcons(icons);
	}
};

class FRONTEND_API CAuiDocChildFrame :
	public wxDocChildFrameAny<CAuiMDIChildFrame, wxAuiMDIParentFrame> {
public:

	// default ctor, use Create after it
	CAuiDocChildFrame() {
	}

	// ctor for a valueForm showing the given view of the specified document
	CAuiDocChildFrame(wxDocument* doc,
		wxView* view,
		wxAuiMDIParentFrame* parent,
		wxWindowID id,
		const wxString& title,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_FRAME_STYLE,
		const wxString& name = wxASCII_STR(wxFrameNameStr))
		:
		wxDocChildFrameAny(doc, view,
			parent, id, title, pos, size, style, name)
	{
	}

	virtual ~CAuiDocChildFrame();

	bool Create(wxDocument* doc,
		wxView* view,
		wxAuiMDIParentFrame* parent,
		wxWindowID id,
		const wxString& title,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_FRAME_STYLE,
		const wxString& name = wxASCII_STR(wxFrameNameStr))
	{
		return wxDocChildFrameAny::Create
		(
			doc, view,
			parent, id, title, pos, size, style, name
		);
	}

#if wxUSE_MENUS
	virtual void SetMenuBar(wxMenuBar* menuBar) override;
	virtual wxMenuBar* GetMenuBar() const override;
#endif // wxUSE_MENUS

	virtual void SetLabel(const wxString& label) override;

protected:

#if __WXMSW__
	// override base class version to add menu bar accel processing
	virtual bool MSWTranslateMessage(WXMSG* msg) override { return false; }
#endif

private:

	wxDECLARE_CLASS(CAuiDocChildFrame);
	wxDECLARE_NO_COPY_CLASS(CAuiDocChildFrame);
};

class FRONTEND_API CDialogDocChildFrame :
	public wxDocChildFrameAny<wxDialog, wxWindow> {
public:

	CDialogDocChildFrame()
	{
	}

	CDialogDocChildFrame(wxDocument* doc,
		wxView* view,
		wxWindow* parent,
		wxWindowID id,
		const wxString& title,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_DIALOG_STYLE,
		const wxString& name = wxASCII_STR(wxDialogNameStr))
		: wxDocChildFrameAny(doc, view,
			parent, id, title, pos, size, style, name)
	{
	}

	bool Create(wxDocument* doc,
		wxView* view,
		wxWindow* parent,
		wxWindowID id,
		const wxString& title,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_DIALOG_STYLE,
		const wxString& name = wxASCII_STR(wxDialogNameStr))
	{
		return wxDocChildFrameAny::Create
		(
			doc, view,
			parent, id, title, pos, size, style, name
		);
	}

private:

	wxDECLARE_CLASS(CDialogDocChildFrame);
	wxDECLARE_NO_COPY_CLASS(CDialogDocChildFrame);
};
#endif 