////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : main app
////////////////////////////////////////////////////////////////////////////

#include "mainApp.h"
#include "backend/appData.h"

#include <wx/clipbrd.h>
#include <wx/debugrpt.h>
#include <wx/fs_arc.h>
#include <wx/fs_filter.h>
#include <wx/fs_mem.h>
#include <wx/stdpaths.h>

#include "resources/splashLogo.xpm"

#if wxVERSION_NUMBER >= 2905 && wxVERSION_NUMBER <= 3100
#include <wx/xrc/xh_auinotbk.h>
#elif wxVERSION_NUMBER > 3100
#include <wx/xrc/xh_aui.h>
#endif

wxIMPLEMENT_APP(CEnterpriseApp);

//////////////////////////////////////////////////////////////////////////////////

//mainFrame
#include "mainFrame/mainFrameEnterprise.h"

//////////////////////////////////////////////////////////////////////////////////
#if wxUSE_CMDLINE_PARSER
#include <wx/cmdline.h>

void CEnterpriseApp::OnInitCmdLine(wxCmdLineParser& parser)
{
	// FILE ENTRY 
	parser.AddOption(wxT("file"), wxT("pwd"), "Start from current dir", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);

	// SERVER ENTRY 
	parser.AddOption(wxT("srv"), wxT("srv"), "Start using server address", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption(wxT("p"), wxT("p"), "Start using port", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption(wxT("db"), wxT("db"), "Start from current database", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption(wxT("usr"), wxT("usr"), "Start from current user", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption(wxT("pwd"), wxT("pwd"), "Start from current password", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);

	//user db 
	parser.AddOption(wxT("ib_usr"), wxT("ib_usr"), "Start from current user", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption(wxT("ib_pwd"), wxT("ib_pwd"), "Start from current password", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);

	//debug   
	parser.AddSwitch(wxT("debug"), wxT("debug"), "Enable debugging for application.", wxCMD_LINE_VAL_NONE);

	return wxApp::OnInitCmdLine(parser);
}

bool CEnterpriseApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
	// FILE ENTRY 
	parser.Found(wxT("file"), &m_strFile);

	// SERVER ENTRY
	parser.Found(wxT("srv"), &m_strServer);
	parser.Found(wxT("p"), &m_strPort);
	parser.Found(wxT("db"), &m_strDatabase);
	parser.Found(wxT("usr"), &m_strUser);
	parser.Found(wxT("pwd"), &m_strPassword);

	//user db 
	parser.Found(wxT("ib_usr"), &m_strIBUser);
	parser.Found(wxT("ib_pwd"), &m_strIBPassword);

	//debug 
	m_debugEnable = parser.FoundSwitch(wxT("debug")) == wxCMD_SWITCH_ON;

	return wxApp::OnCmdLineParsed(parser);
}
#endif

//////////////////////////////////////////////////////////////////////////////////

#include "backend/backend_exception.h"

bool CEnterpriseApp::OnInit()
{
	wxSocketBase::Initialize();
	return wxApp::OnInit();
}

int CEnterpriseApp::OnRun()
{
	// Abnormal Termination Handling
#if wxUSE_ON_FATAL_EXCEPTION && wxUSE_STACKWALKER
	::wxHandleFatalExceptions(true);
#endif

	// Get the data directory
	bool ret = false;

	if (m_strFile.IsEmpty()) {
		ret = appDataCreateServer(eRunMode::eENTERPRISE_MODE,
			m_strServer, m_strPort, m_strUser, m_strPassword, m_strDatabase
		);
	}
	else {
		ret = appDataCreateFile(eRunMode::eENTERPRISE_MODE,
			m_strFile
		);
	}

	if (!ret) {
		const wxString &strLastError = CBackendException::GetLastError(); 
		if (!strLastError.IsEmpty()) wxMessageBox(strLastError);
		return 1;
	}

	CProcessSplashScreen* splashScreenLoader =
		new CProcessSplashScreen(wxBitmap(splashLogo_xpm),
			wxSPLASH_CENTRE_ON_SCREEN,
			-1, nullptr, -1, wxDefaultPosition, wxDefaultSize,
			wxBORDER_SIMPLE
		);

	// Init handlers
	wxInitAllImageHandlers();

	wxXmlResource::Get()->InitAllHandlers();
#if wxVERSION_NUMBER >= 2905 && wxVERSION_NUMBER <= 3100
	wxXmlResource::Get()->AddHandler(new wxAuiNotebookXmlHandler);
#elif wxVERSION_NUMBER > 3100
	wxXmlResource::Get()->AddHandler(new wxAuiXmlHandler);
#endif

#ifdef __WXMSW__
	::DisableProcessWindowsGhosting();
#endif

#if DEBUG 
	wxLog::AddTraceMask(wxTRACE_MemAlloc);
	wxLog::AddTraceMask(wxTRACE_ResAlloc);
#if wxUSE_LOG
#if defined(__WXGTK__)
	wxLog::AddTraceMask("clipboard");
#elif defined(__WXMSW__)
	wxLog::AddTraceMask(wxTRACE_OleCalls);
#endif
#endif // wxUSE_LOG
#endif

	// Log to stderr while working on the command line
	delete wxLog::SetActiveTarget(new wxLogStderr);

	// Message output to the same as the log target
	delete wxMessageOutput::Set(new wxMessageOutputLog);

#if wxUSE_LIBPNG
	wxImage::AddHandler(new wxPNGHandler);
#endif

	// Support loading files from memory
	// Used to load the XRC preview, but could be useful elsewhere
	wxFileSystem::AddHandler(new wxMemoryFSHandler);

	// Support for loading files from archives
	wxFileSystem::AddHandler(new wxArchiveFSHandler);
	wxFileSystem::AddHandler(new wxFilterFSHandler);

#if wxUSE_LIBPNG
	wxImage::AddHandler(new wxPNGHandler);
#endif
	mainFrameCreate(CDocEnterpriseMDIFrame);
	if (!appData->Connect(m_strIBUser, m_strIBPassword, m_debugEnable ? _app_start_create_debug_server_flag : _app_start_default_flag)) {
		const wxString& strLastError = CBackendException::GetLastError();
		if (!strLastError.IsEmpty()) wxMessageBox(strLastError);
		mainFrameDestroy();
		if (splashScreenLoader != nullptr) splashScreenLoader->Destroy();
		return 1;
	}
	if (splashScreenLoader != nullptr) splashScreenLoader->Destroy();
	bool allow = mainFrameShow();
	if (!allow) return 1;
	return wxApp::OnRun();
}

void CEnterpriseApp::OnUnhandledException()
{
}

#include "backend/system/value/valueOLE.h"

void CEnterpriseApp::OnFatalException()
{
	//generate dump
	wxDebugReport report;

	report.AddCurrentDump();
	report.AddExceptionDump();

	//release all created com-objects
	CValueOLE::ReleaseComObjects();

	if (wxSocketBase::IsInitialized())
		wxSocketBase::Shutdown();

	appDataDestroy();

	//show error
	wxDebugReportPreviewStd preview;
	if (preview.Show(report)) report.Process();
}

int CEnterpriseApp::OnExit()
{
	//release all created com-objects
	CValueOLE::ReleaseComObjects();

	if (wxSocketBase::IsInitialized())
		wxSocketBase::Shutdown();

	mainFrameDestroy();

	bool suñcess_exit = wxApp::OnExit();

	appDataDestroy();

	// Allow clipboard data to persist after close
	wxTheClipboard->Flush();
	wxTheClipboard->Close();

	return suñcess_exit;
}