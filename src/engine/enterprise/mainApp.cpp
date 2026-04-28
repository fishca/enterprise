////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : main app
////////////////////////////////////////////////////////////////////////////

#include "mainApp.h"
#include "backend/appData.h"
#include "backend/backend_mainFrame.h"
#include "frontend/session/guiSession.h"   // transitively pulls backend/session/session.h
#include "backend/session/sessionRegistry.h"

#include <wx/clipbrd.h>
#include <wx/debugrpt.h>
#include <wx/filename.h>
#include <wx/fs_arc.h>
#include <wx/fs_filter.h>
#include <wx/fs_mem.h>
#include <wx/stdpaths.h>

#ifdef __WXMSW__
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

namespace {
// Persistent minidump writer — fires as a top-level SEH filter BEFORE
// wxHandleFatalExceptions shows its (ephemeral) debug-report dialog.
// wxDebugReport wipes its temp directory when the preview is closed;
// the dumps we write here live in bin/.../crashdumps/ and survive.
static LPTOP_LEVEL_EXCEPTION_FILTER s_prevFilter = nullptr;

static LONG WINAPI PersistentCrashDumpFilter(EXCEPTION_POINTERS* ep)
{
	wxString exePath = wxStandardPaths::Get().GetExecutablePath();
	wxString crashDir = wxFileName(exePath).GetPath() + wxFILE_SEP_PATH + wxT("crashdumps");
	wxFileName::Mkdir(crashDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	const wxString stamp = wxDateTime::Now().Format(wxT("%Y%m%dT%H%M%S"));
	const wxString dumpPath = wxString::Format(wxT("%s%centerprise_%u_%s.dmp"),
		crashDir, wxFILE_SEP_PATH,
		static_cast<unsigned>(::GetCurrentProcessId()),
		stamp);

	HANDLE hFile = ::CreateFileW(dumpPath.wc_str(), GENERIC_WRITE, 0, nullptr,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile != INVALID_HANDLE_VALUE) {
		MINIDUMP_EXCEPTION_INFORMATION mei = {};
		mei.ThreadId = ::GetCurrentThreadId();
		mei.ExceptionPointers = ep;
		mei.ClientPointers = FALSE;

		const MINIDUMP_TYPE type = static_cast<MINIDUMP_TYPE>(
			MiniDumpWithDataSegs |
			MiniDumpWithHandleData |
			MiniDumpWithUnloadedModules |
			MiniDumpWithThreadInfo |
			MiniDumpWithFullMemory);

		::MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(),
			hFile, type, ep ? &mei : nullptr, nullptr, nullptr);
		::CloseHandle(hFile);
	}

	// Chain to wx's handler (which invokes OnFatalException → report dialog).
	return s_prevFilter ? s_prevFilter(ep) : EXCEPTION_CONTINUE_SEARCH;
}

static void InstallPersistentCrashDump()
{
	if (s_prevFilter == nullptr)
		s_prevFilter = ::SetUnhandledExceptionFilter(PersistentCrashDumpFilter);
}
} // namespace
#endif // __WXMSW__

#include "resources/splashLogo.xpm"

#if wxVERSION_NUMBER >= 2905 && wxVERSION_NUMBER <= 3100
#include <wx/xrc/xh_auinotbk.h>
#elif wxVERSION_NUMBER > 3100
#include <wx/xrc/xh_aui.h>
#endif

wxIMPLEMENT_APP(ibAppEnterprise);

//////////////////////////////////////////////////////////////////////////////////

//mainFrame
#include "mainFrame/mainFrameEnterprise.h"

//////////////////////////////////////////////////////////////////////////////////
#if wxUSE_CMDLINE_PARSER
#include <wx/cmdline.h>

void ibAppEnterprise::OnInitCmdLine(wxCmdLineParser& parser)
{
	// Short names are legacy (matched what the /flag-style spawner used);
	// long names match wenterprise-server so one builder emits flags that
	// parse identically across enterprise/designer/daemon/wes.
	parser.AddOption(wxT("file"),   wxT("file"),     "Database file path",      wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption(wxT("srv"),    wxT("server"),   "Database server address", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption(wxT("p"),      wxT("dbport"),   "Database server port",    wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption(wxT("db"),     wxT("db"),       "Database name",           wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption(wxT("usr"),    wxT("user"),     "Database user",           wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption(wxT("pwd"),    wxT("password"), "Database password",       wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption(wxT("ib_usr"), wxT("ibuser"),   "IB user",                 wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption(wxT("ib_pwd"), wxT("ibpwd"),    "IB password",             wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption(wxT("lc"),     wxT("locale"),   "UI locale",               wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddSwitch(wxT("debug"),  wxT("debug"),    "Enable debug attach.",    wxCMD_LINE_VAL_NONE);

	return wxApp::OnInitCmdLine(parser);
}

bool ibAppEnterprise::OnCmdLineParsed(wxCmdLineParser& parser)
{
	// FILE ENTRY 
	parser.Found(wxT("file"), &m_strFile);

	// SERVER ENTRY
	parser.Found(wxT("srv"), &m_strServer);
	parser.Found(wxT("p"), &m_strPort);
	parser.Found(wxT("db"), &m_strDatabase);
	parser.Found(wxT("usr"), &m_strUser);
	parser.Found(wxT("pwd"), &m_strPassword);

	// USER 
	parser.Found(wxT("ib_usr"), &m_strIBUser);
	parser.Found(wxT("ib_pwd"), &m_strIBPassword);

	// LOCALE
	parser.Found(wxT("lc"), &m_strLocale);

	// DEBUG 
	m_debugEnable = parser.FoundSwitch(wxT("debug")) == wxCMD_SWITCH_ON;

	return wxApp::OnCmdLineParsed(parser);
}
#endif

//////////////////////////////////////////////////////////////////////////////////

#include "backend/backend_exception.h"

// ibEnterpriseSession — concrete GUI session for enterprise.exe. Lives
// here (not in a dedicated header) because only this exe ever needs it.
// OnCreateSession runs on the main thread after the registry has Added
// the session; it instantiates the exe-specific frame class — the only
// place where `ibFrontendDocMDIFrameEnterprise` is visible, since the
// concrete frame type is not exported to frontend.dll.
class ibEnterpriseSession : public ibGUISession {
public:
	using ibGUISession::ibGUISession;

	bool OnCreateSession() override {
		AttachFrame(new ibFrontendDocMDIFrameEnterprise);
		return m_frame != nullptr;
	}
};

bool ibAppEnterprise::OnInit()
{
	wxSocketBase::Initialize();
	return wxApp::OnInit();
}

int ibAppEnterprise::OnRun()
{
	// Abnormal Termination Handling
#if wxUSE_ON_FATAL_EXCEPTION && wxUSE_STACKWALKER
	::wxHandleFatalExceptions(true);
#endif
#ifdef __WXMSW__
	// Install our top-level SEH filter AFTER wx's so ours runs first
	// (SetUnhandledExceptionFilter replaces and returns the previous).
	// Writes a persistent minidump to <exe>/crashdumps/ on every fatal SEH.
	InstallPersistentCrashDump();
#endif

	// Decide whether this is a file-based launch (Firebird embedded /
	// SQLite — `--file=…`) or a server launch (`--server=… --db=…`).
	// Reject the no-arg case explicitly: the previous behaviour fell
	// through to appDataCreateServer with empty server/port/db, the
	// PostgreSQL driver opened a connection with all-empty credentials
	// and the resulting ThrowDatabaseException re-entered the
	// half-initialised session registry. Surfacing the missing-arg case
	// here is far easier to diagnose than the assertion behind it.
	bool ret = false;

	if (!m_strFile.IsEmpty()) {
		ret = appDataCreateFile(ibRunMode::eENTERPRISE_MODE,
			m_strFile, m_strLocale
		);
	}
	else if (!m_strServer.IsEmpty() && !m_strDatabase.IsEmpty()) {
		ret = appDataCreateServer(ibRunMode::eENTERPRISE_MODE,
			m_strServer, m_strPort, m_strUser, m_strPassword, m_strDatabase, m_strLocale
		);
	}
	else {
		wxMessageBox(
			_("Cannot start enterprise.exe — no infobase specified.\n\n"
			  "Provide one of:\n"
			  "  --file=<path>          (Firebird embedded / SQLite file)\n"
			  "  --server=<host> --db=<name> [--dbport=…] [--user=…] [--password=…]\n\n"
			  "Or launch through launcher.exe to pick a saved infobase."),
			_("OES Enterprise"),
			wxOK | wxICON_ERROR
		);
		return 1;
	}

	if (!ret) {
		const wxString &strLastError = ibBackendException::GetLastError();
		if (!strLastError.IsEmpty()) wxMessageBox(strLastError);
		return 1;
	}

	ibProcessSplashScreen* splashScreenLoader =
		new ibProcessSplashScreen(wxBitmap(splashLogo_xpm),
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
	// Flow (enterprise thick client):
	//   1. CreateSession<ibEnterpriseSession> — session is registered
	//      in the registry; its OnCreateSession hook instantiates the
	//      frame + wires the back-link on the main thread.
	//   2. Authenticate — attaches user creds to the ticket; interactive
	//      dialog fallback shows through session->GetFrame().
	//   3. LoadMetadata — compile descriptors.
	//   4. mainFrameShow — EnsureRuntime lazily creates root + ProcUnits,
	//      AllowRun fires BeforeStart veto.
	// Stash flags so OnFirstConnect listener picks them up when
	// LoadMetadata fires from the registry event chain.
	appData->m_loadMetadataFlags = m_debugEnable
		? _app_start_create_debug_server_flag
		: _app_start_default_flag;

	// AccessMode was set by appData's ctor based on runMode. Registry
	// listeners (wired in appData ctor) handle BindSessionToThread,
	// LoadMetadata, CreateRoot + CompileRoot + InitRuntimeForSession
	// through OnFirstConnect / OnAuthenticated.
	ibSession* session = appData->CreateSession<ibEnterpriseSession>();
	if (session == nullptr || !session->Open(m_strIBUser, m_strIBPassword)) {
		if (session != nullptr) session->Close();
		if (splashScreenLoader != nullptr) splashScreenLoader->Destroy();
		wxMessageBox(_("Authentication failed"));
		return 1;
	}

	if (splashScreenLoader != nullptr) splashScreenLoader->Destroy();
	if (!session->ShowFrame()) return 1;
	return wxApp::OnRun();
}

void ibAppEnterprise::OnUnhandledException()
{
	// Reached when a C++ exception escapes to wxApp's event loop (not a
	// structured exception — that goes through OnFatalException). Same
	// treatment: capture context + minidump, offer the report.
	wxDebugReportCompress report;
	report.AddAll(wxDebugReport::Context_Current);

	wxDebugReportPreviewStd preview;
	if (preview.Show(report)) report.Process();
}

#ifdef __WXMSW__
#include "backend/system/value/valueOLE.h"
#endif

void ibAppEnterprise::OnFatalException()
{
	// Collect everything at the exception context: XML report + minidump.
	// AddAll(Context_Exception) captures state at the fault point (register
	// values, stack, loaded modules), which is what a debugger needs to
	// resolve the real crash site — AddCurrentContext alone only records
	// where OnFatalException itself is running.
	wxDebugReportCompress report;
	report.AddAll(wxDebugReport::Context_Exception);

	//release all created com-objects
#ifdef __WXMSW__
	ibValueOLE::ReleaseComObjects();
#endif

	if (wxSocketBase::IsInitialized())
		wxSocketBase::Shutdown();

	appDataDestroy();

	//show error
	wxDebugReportPreviewStd preview;
	if (preview.Show(report)) report.Process();
}

int ibAppEnterprise::OnExit()
{
	//release all created com-objects
#ifdef __WXMSW__
	ibValueOLE::ReleaseComObjects();
#endif

	if (wxSocketBase::IsInitialized())
		wxSocketBase::Shutdown();

	// Tear every session down through the session manager BEFORE
	// wxApp::OnExit. registry->Stop() submits Remove@Urgent for each
	// session in m_own and drains the queue — OnDisconnect listeners
	// fire while the wx event loop is still alive, so any frame-Destroy
	// scheduled from there gets dispatched. Without this the event
	// loop dies first and the Destroy events stay queued. Idempotent —
	// ~ibApplicationData calls Stop again best-effort.
	if (appData != nullptr) {
		auto* registry = appData->GetSessionRegistry();
		if (registry != nullptr) registry->Stop();
	}

	bool suсcess_exit = wxApp::OnExit();

	appDataDestroy();

	// Allow clipboard data to persist after close
	if (wxTheClipboard->Open()) {
		wxTheClipboard->Flush();
		wxTheClipboard->Close();
	}

	return suсcess_exit;
}