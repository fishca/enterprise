#ifndef _MAIN_FRAME_H__
#define _MAIN_FRAME_H__

#include <wx/wx.h>
#include <wx/aui/aui.h>
#include <wx/docview.h>
#include <wx/splash.h>
#include <wx/stc/stc.h>

#include "backend/backend_mainFrame.h"
#include "frontend/frontend.h"

#if defined(backend_mainFrame)
#undef backend_mainFrame
#endif

class CMetaView;

#define mainFrame            		 (CDocMDIFrame::GetFrame())
#define mainFrameCreate(frame)       (CDocMDIFrame::InitFrame(new frame))
#define mainFrameShow()				 (CDocMDIFrame::ShowFrame())
#define mainFrameDestroy()  		 (CDocMDIFrame::DestroyFrame())

#include "objinspect/objinspect.h"

#include "settings/keybinder.h"
#include "settings/fontcolorsettings.h"
#include "settings/editorsettings.h"

//********************************************************************************
//*                                 ID's                                         *
//********************************************************************************

#define wxCREATE_SDI_FRAME 0x16000

#define wxAUI_DEFAULT_COLOUR wxColour(41, 57, 85) 
#define wxAUI_WHITE_COLOUR wxColour(255, 255, 255) 

class FRONTEND_API CDocMDIFrame :
	public IBackendDocMDIFrame, public wxAuiMDIParentFrame,
	public wxDocParentFrameAnyBase {
public:

	virtual wxMenu* GetDefaultMenu(int idMenu) const { return nullptr; }

	virtual void CreateGUI() = 0;

	virtual void Modify(bool modify) {}
	virtual bool IsModified() const { return false; }

	virtual bool AuthenticationUser(const wxString& userName, const wxString& userPassword) const;

	virtual IMetaData* FindMetadataByPath(const wxString& strFileName) const;

#pragma region _frontend_call_h__

	// Form support
	virtual IBackendValueForm* ActiveWindow() const override;
	virtual IBackendValueForm* CreateNewForm(const IValueMetaObjectForm* creator, class IBackendControlFrame* ownerControl = nullptr,
		class ISourceDataObject* srcObject = nullptr, const CUniqueKey& formGuid = wxNullUniqueKey) override;

	virtual CUniqueKey CreateFormUniqueKey(const IBackendControlFrame* ownerControl,
		const ISourceDataObject* sourceObject, const CUniqueKey& formGuid);

	virtual class IBackendValueForm* FindFormByUniqueKey(const IBackendControlFrame* ownerControl,
		const ISourceDataObject* sourceObject, const CUniqueKey& formGuid);

	virtual class IBackendValueForm* FindFormByUniqueKey(const CUniqueKey& guid) override;
	virtual class IBackendValueForm* FindFormByControlUniqueKey(const CUniqueKey& guid) override;
	virtual class IBackendValueForm* FindFormBySourceUniqueKey(const CUniqueKey& guid) override;

	virtual bool UpdateFormUniqueKey(const CUniquePairKey& guid) override;

	// Grid support
	virtual bool ShowSpreadSheetDocument(const wxString& strTitle, wxObjectDataPtr<CBackendSpreadsheetObject>& spreadSheetDocument) override;
	virtual bool PrintSpreadSheetDocument(const wxObjectDataPtr<CBackendSpreadsheetObject>& doc, bool showPrintDlg = true) override;

#pragma endregion 

	virtual void RefreshFrame() override;
	virtual void RaiseFrame() override;

	virtual wxAuiToolBar* GetMainFrameToolbar() const { return m_mainFrameToolbar; }
	virtual wxAuiToolBar* GetDocToolbar() const { return m_docToolbar; }

	virtual wxFrame* GetFrameHandler() const { return s_instance; }

	virtual IPropertyObject* GetProperty() const;
	virtual bool SetProperty(IPropertyObject* prop);

	virtual void SetTitle(const wxString& title) override { wxAuiMDIParentFrame::SetTitle(title); }
	virtual void SetStatusText(const wxString& text, int number = 0) override { wxAuiMDIParentFrame::SetStatusText(text, number); }
	virtual bool Show(bool show = true) override { return (show && AllowRun() || !show && AllowClose()) && wxAuiMDIParentFrame::Show(show); }

#if wxUSE_MENUS
	virtual void SetMenuBar(wxMenuBar* pMenuBar) override;
#endif // wxUSE_MENUS

	virtual wxAuiMDIClientWindow* OnCreateClient() override;

	// bring window to front
	virtual void Raise() override;

	//destroy window 
	virtual bool Destroy() override;

protected:

	// hook the document manager into event handling chain here
	virtual bool TryBefore(wxEvent& event) override {
		// It is important to send the event to the base class first as
		// wxMDIParentFrame overrides its TryBefore() to send the menu events
		// to the currently active child valueForm and the child must get them
		// before our own TryProcessEvent() is executed, not afterwards.
		return wxAuiMDIParentFrame::TryBefore(event) || TryProcessEvent(event);
	}

	virtual bool AllowRun() const { return true; }
	virtual bool AllowClose() const { return true; }

	CDocMDIFrame(const wxString& title,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_FRAME_STYLE,
		const wxString& name = wxASCII_STR(wxFrameNameStr));

	bool Create(const wxString& title,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxDEFAULT_FRAME_STYLE,
		const wxString& name = wxASCII_STR(wxFrameNameStr));

public:

	virtual ~CDocMDIFrame();

	static wxWindow* CreateChildFrame(CMetaView* view,
		const wxPoint& pos, const wxSize& size, long style = wxDEFAULT_FRAME_STYLE);

	static CObjectInspector* GetObjectInspector() {
		if (s_instance != nullptr)
			return s_instance->m_objectInspector;
		return nullptr;
	}

	static CDocMDIFrame* GetFrame() { return s_instance; }

	// Force the static appData instance to Init()
	static void InitFrame(CDocMDIFrame* mf);
	static bool ShowFrame();

	static void DestroyFrame();

	KeyBinder             GetKeyBinder() const { return m_keyBinder; }
	FontColorSettings     GetFontColorSettings() const { return m_fontColorSettings; }
	EditorSettings        GetEditorSettings() const { return m_editorSettings; }

	/**
	* Show property in mainFrame
	*/
	bool IsShownInspector();
	void ShowInspector();

	// Activate view 
	void ActivateView(CMetaView* view, bool activate = true);

protected:

	// Events 
	void OnCloseWindow(wxCloseEvent& event);
	void OnExit(wxCommandEvent& WXUNUSED(event));

	virtual void CreatePropertyPane();

	class CFrameManager : public wxAuiManager {
	public:
		CFrameManager(wxWindow* managedWnd = nullptr,
			unsigned int flags = wxAUI_MGR_DEFAULT) :
			wxAuiManager(managedWnd, flags) {
		}

		void Refresh() { Repaint(); }
	};

	static CDocMDIFrame* s_instance;

	CObjectInspector* m_objectInspector;

	KeyBinder             m_keyBinder;
	FontColorSettings     m_fontColorSettings;
	EditorSettings        m_editorSettings;

	bool m_callRaiseFrame;

	wxAuiToolBar* m_mainFrameToolbar;
	wxAuiToolBar* m_docToolbar;

	// Create frame manager 
	CFrameManager m_mgr;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////

class CDocBottomStatusBar : public wxStatusBar {
public:

	CDocBottomStatusBar() : wxStatusBar() {};
	CDocBottomStatusBar(wxWindow* parent,
		wxWindowID id = wxID_ANY,
		long style = wxSTB_DEFAULT_STYLE,
		const wxString& name = wxStatusBarNameStr)
		: wxStatusBar(parent, id, style, name)
	{
		wxStatusBar::SetBackgroundColour(wxAUI_DEFAULT_COLOUR);
		wxStatusBar::SetForegroundColour(wxAUI_WHITE_COLOUR);

		m_statusBarText = new wxStaticText(this, wxID_ANY, wxEmptyString, wxPoint(5, 5), wxDefaultSize, 0);
		m_statusBarText->Show();
	}

	virtual void DoUpdateStatusText(int field) override {
		m_statusBarText->SetLabelText(
			GetStatusText(field)
		);
	}

private:
	wxStaticText* m_statusBarText;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////

class CProcessSplashScreen : public wxSplashScreen {
public:
	CProcessSplashScreen(const wxBitmap& bitmap, long splashStyle = wxSPLASH_CENTRE_ON_SCREEN, int milliseconds = -1,
		wxWindow* parent = nullptr, wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxSIMPLE_BORDER | wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP) :
		wxSplashScreen(bitmap, splashStyle, milliseconds,
			parent, id,
			pos, size, style
		)
	{
		wxTheApp->SetTopWindow(this);

		//Needed to get the splashscreen to paint
		wxSplashScreen::Update();
	}

	virtual int FilterEvent(wxEvent& event) wxOVERRIDE { return Event_Skip; }
};

//pane 
#define wxAUI_PANE_METADATA wxT("metadataWindow")
#define wxAUI_PANE_PROPERTY wxT("propertyWindow")
#define wxAUI_PANE_BOTTOM	wxT("bottomWindow"	)

#endif 