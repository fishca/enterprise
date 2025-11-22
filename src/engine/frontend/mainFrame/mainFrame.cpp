////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxwidgets community
//	Description : main frame window
////////////////////////////////////////////////////////////////////////////

#include "mainFrame.h"
#include "mainFrameChild.h"

//common 
#include "frontend/docView/docManager.h"
#include "frontend/mainFrame/objinspect/objinspect.h"

#include "frontend/win/theme/luna_tabart.h"
#include "frontend/win/theme/luna_dockart.h"

//***********************************************************************************
//*                                 mainFrame                                       *
//***********************************************************************************

CDocMDIFrame* CDocMDIFrame::s_instance = nullptr;

void CDocMDIFrame::InitFrame(CDocMDIFrame* frame)
{
	if (s_instance == nullptr && frame != nullptr) {
		s_instance = frame;
		wxTheApp->SetTopWindow(s_instance);
		s_instance->CreateGUI();
	}
}

#include <wx/display.h>

bool CDocMDIFrame::ShowFrame()
{
	if (s_instance != nullptr && !s_instance->IsShown()) {

		// Get the primary display
		wxDisplay display;

		// Get the PPI (pixels per inch)
		const wxRect clientArea = display.GetClientArea();

		s_instance->SetSize(
			clientArea.GetWidth() - 100,
			clientArea.GetHeight() - 100
		);

		s_instance->SetFocus();
		s_instance->Center();

		if (s_instance->Show()) {
			s_instance->Raise();
			return true;
		}

		return false;
	}

	return false;
}

void CDocMDIFrame::DestroyFrame()
{
	if (s_instance != nullptr) {
		s_instance->Destroy();
	}
}

//***********************************************************************************
//*                                 Constructor                                     *
//***********************************************************************************

CDocMDIFrame::CDocMDIFrame(const wxString& title,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxString& strName)
	: wxDocParentFrameAnyBase(this), m_objectInspector(nullptr), m_callRaiseFrame(false)
{
	Create(title, pos, size, style | wxNO_FULL_REPAINT_ON_RESIZE);
}

#include "backend/compiler/value.h"

bool CDocMDIFrame::Create(const wxString& title,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxString& strName)
{
	if (!wxAuiMDIParentFrame::Create(nullptr, wxID_ANY, title, pos, size, style, strName))
		return false;

	m_docManager = nullptr;

	this->Bind(wxEVT_MENU, &CDocMDIFrame::OnExit, this, wxID_EXIT);
	this->Bind(wxEVT_CLOSE_WINDOW, &CDocMDIFrame::OnCloseWindow, this);

	this->SetArtProvider(new wxAuiLunaTabArt());

	// notify wxAUI which valueForm to use
	m_mgr.SetManagedWindow(this);
	m_mgr.SetArtProvider(new wxAuiLunaDockArt());

	SetIcon(wxICON(oes));
	return true;
}

wxWindow* CDocMDIFrame::CreateChildFrame(CMetaView* view, const wxPoint& pos, const wxSize& size, long style)
{
	// create a child valueForm of appropriate class for the current mode
	CMetaDocument* document = view->GetDocument();

	if ((style & wxCREATE_SDI_FRAME) != 0) {

		wxWindow* parent = wxTheApp->GetTopWindow();

		for (wxWindow* window : wxTopLevelWindows) {
			if (window->IsKindOf(CLASSINFO(wxDialog))) {
				if (((wxDialog*)window)->IsModal()) {
					parent = window; break;
				}
			}
		}

		CDialogDocChildFrame* subframe = new CDialogDocChildFrame(document, view, parent, wxID_ANY, document->GetTitle(), pos, size, style & ~wxCREATE_SDI_FRAME);
		subframe->SetIcon(document->GetIcon());
		subframe->SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
		subframe->Center();
		return subframe;
	}

	CAuiDocChildFrame* subframe = new CAuiDocChildFrame(document, view, s_instance, wxID_ANY, document->GetTitle(), pos, size, style);
	subframe->SetIcon(document->GetIcon());
	subframe->SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
	return subframe;
}

void CDocMDIFrame::RefreshFrame()
{
	if (m_docManager != nullptr) {
		for (auto& doc : m_docManager->GetDocumentsVector())
			doc->UpdateAllViews();
	}

	Refresh();
}

void CDocMDIFrame::RaiseFrame()
{
	if (!m_callRaiseFrame && CDocMDIFrame::IsFocusable()) {
		CallAfter([&]() {
			CDocMDIFrame::Raise();
			m_callRaiseFrame = false;
			}
		);
		m_callRaiseFrame = true;
	}
}

#if wxUSE_MENUS
void CDocMDIFrame::SetMenuBar(wxMenuBar* pMenuBar)
{
	if (m_pMyMenuBar == nullptr) {

		//Remove the Window menu from the old menu bar
		RemoveWindowMenu(GetMenuBar());

		//Add the Window menu to the new menu bar.
		AddWindowMenu(pMenuBar);
	}

	wxFrame::SetMenuBar(pMenuBar);
}
#endif // wxUSE_MENUS

wxAuiMDIClientWindow* CDocMDIFrame::OnCreateClient()
{
	class wxAuiMDIClientWindowImpl : public wxAuiMDIClientWindow {
	public:
		wxAuiMDIClientWindowImpl() : wxAuiMDIClientWindow() {}
		wxAuiMDIClientWindowImpl(wxAuiMDIParentFrame* parent, long style = 0) : wxAuiMDIClientWindow(parent, style) {}

	protected:

		//A general selection function
		virtual int DoModifySelection(size_t n, bool events) {
			wxAuiNotebook::Freeze();
			const int selection = wxAuiNotebook::DoModifySelection(n, events);
			wxAuiNotebook::Thaw();
			return selection;
		}
	};

	return new wxAuiMDIClientWindowImpl(this);
}

// bring window to front
void CDocMDIFrame::Raise()
{
#if __WXMSW__
	// Simulate a key press
	::keybd_event((BYTE)0, 0, 0 /* key press */, 0);
	::keybd_event((BYTE)0, 0, KEYEVENTF_KEYUP, 0);
#endif

	wxAuiMDIParentFrame::Raise();
}

bool CDocMDIFrame::Destroy()
{
	wxAuiMDIClientWindow* client_window = GetClientWindow();
	wxCHECK_MSG(client_window, false, wxS("Missing MDI Client Window"));

	client_window->Freeze();

	bool success = m_docManager != nullptr ?
		m_docManager->CloseDocuments() : true;

	if (success && m_docManager != nullptr) {
#if wxUSE_CONFIG
		m_docManager->FileHistorySave(*wxConfig::Get());
#endif // wxUSE_CONFIG
		wxDELETE(m_docManager);
	}

	client_window->Thaw();

	return success &&
		wxAuiMDIParentFrame::Destroy();
}

CDocMDIFrame::~CDocMDIFrame()
{
	if (s_instance == this) s_instance = nullptr;
	// deinitialize the valueForm manager
	m_mgr.UnInit();
}

//********************************************************************************
//*                                    System                                    *
//********************************************************************************

void CDocMDIFrame::OnExit(wxCommandEvent& WXUNUSED(event))
{
	this->Close();
}

void CDocMDIFrame::OnCloseWindow(wxCloseEvent& event)
{
	bool allowClose = event.CanVeto() ? AllowClose() : true;
	// The user decided not to close finally, abort.
	if (allowClose) event.Skip();
	// Just skip the event, base class handler will destroy the window.
	else event.Veto();
}