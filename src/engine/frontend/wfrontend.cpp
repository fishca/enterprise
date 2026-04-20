#include "wfrontend.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/imagpng.h>
#include <wx/mstream.h>

#include "backend/appData.h"
#include "backend/guid.h"
#include "backend/session/session.h"
#include "backend/session/sessionRegistry.h"
#include "backend/backend_exception.h"
#include "backend/databaseLayer/connectionPool.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/databaseLayer/databaseResultSet.h"
#include "backend/metadataConfiguration.h"
#include "backend/metaCollection/metaObject.h"
#include "backend/metaCollection/metaObjectMetadata.h"
#include "backend/metaCollection/metaFormObject.h"

#include <iostream>

#include "../../3rdparty/nlohmann/json.hpp"

#include "web/webSession.h"
#include "web/webApplication.h"
#include "web/webFrame.h"
#include "web/webChildFrame.h"
#include "web/webWindow.h"
#include "visualView/visualHostClient.h"
#include "visualView/ctrl/sizer.h"
#include "visualView/ctrl/widgets.h"

namespace {

// Metadata-generation counter. Starts at 1; incremented by the sweep
// watcher every time sys_config.file_guid diverges from the cached
// value (designer deployed a new config). Surfaced in every /session
// response so clients can compare against their last-seen gen and
// pop a "configuration changed" banner when the server got bumped —
// even after the session itself was evicted (in which case /session
// returns the unauthenticated stub but still carries the current gen).
std::atomic<std::uint64_t> g_metaGeneration{ 1 };

class SessionManager {
public:
	SessionManager()
	{
		// Background idle-session reaper. Nobody calls /logout on tab-
		// close (the beforeunload beacon raced with GET / and was
		// removed), so sessions orphan as soon as the browser goes away.
		// A daemon sweep evicts anything untouched for longer than the
		// idle threshold — otherwise every abandoned tab keeps its
		// runtime + forms + ibValues alive until process exit.
		m_sweepThread = std::thread([this]() { SweepLoop(); });
	}

	~SessionManager()
	{
		// Signal stop + wake the sweep so program shutdown doesn't
		// block on the full interval. Clear() below runs OnExit on
		// every surviving session.
		{
			std::lock_guard<std::mutex> lk(m_sweepMtx);
			m_sweepStop = true;
		}
		m_sweepCv.notify_all();
		if (m_sweepThread.joinable())
			m_sweepThread.join();
	}

	// Bump LastActiveMs on the session. Called by every HTTP handler
	// at lookup time so the idle-sweep knows the client is still alive.
	// No-op for unknown ids.
	void Touch(const std::string& id)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_sessions.find(id);
		if (it != m_sessions.end() && it->second)
			it->second->Touch();
	}

	// Allocate a fresh unauthenticated session. The returned id is
	// handed back as a cookie so subsequent requests (in particular
	// POST /login) can identify the client.
	std::string Create()
	{
		return CreateWith(MakeRandomId());
	}

	// Same, but the caller supplies the id — used when the JS client
	// minted a per-tab id in sessionStorage (so every browser tab
	// gets its own ibWebSession instead of sharing one via cookie).
	// If the id already exists, reuses; otherwise creates.
	std::string CreateWith(const std::string& id)
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_sessions.find(id) != m_sessions.end())
				return id;
		}
		auto session = std::make_shared<ibWebSession>(id, std::string());
		if (!session->OnInit())
			return std::string();

		std::lock_guard<std::mutex> lock(m_mutex);
		m_sessions.emplace(id, std::move(session));
		return id;
	}

	bool Login(const std::string& id, const std::string& user, const std::string& password)
	{
		// shared_ptr copy under lock keeps the session alive for the
		// whole Login — a racing Destroy (e.g. pagehide beacon on
		// reload) erases from the map but our copy keeps ibWebSession
		// allocated until we return.
		std::shared_ptr<ibWebSession> s;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			auto it = m_sessions.find(id);
			if (it == m_sessions.end()) return false;
			s = it->second;
		}
		return s->Login(user, password);
	}

	bool Exists(const std::string& id) const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_sessions.find(id) != m_sessions.end();
	}

	void Destroy(const std::string& id)
	{
		std::shared_ptr<ibWebSession> dying;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			auto it = m_sessions.find(id);
			if (it == m_sessions.end())
				return;
			dying = std::move(it->second);
			m_sessions.erase(it);
		}
		// Run OnExit outside the lock so per-session teardown (which may
		// touch backend state) does not serialise every other request.
		// Any concurrent handler thread that looked up this session
		// before the erase still holds its own shared_ptr; `dying` here
		// may be NOT the last reference — the real destruction happens
		// once every borrower drops. That's safe because OnExit is
		// idempotent-on-already-exited and the dtor handles the rest.
		if (dying) dying->OnExit();
	}

	std::size_t Count() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_sessions.size();
	}

	// Tab count for one session — lets GET / decide whether F5 should
	// swap an empty session for a fresh OnStart. Returns 0 on unknown
	// or unauthenticated sessions (they have no frame yet).
	std::size_t TabCount(const std::string& id) const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_sessions.find(id);
		if (it == m_sessions.end()) return 0;
		ibWebApplication* app = it->second->App();
		if (app == nullptr) return 0;
		ibWebFrame* frame = app->GetFrame();
		return frame != nullptr ? frame->TabCount() : 0;
	}

	// PNG-encoded bytes of the tab's icon. Empty on missing session,
	// missing tab or empty icon. Goes through wxImage → memory-stream
	// so we don't have to touch the filesystem.
	std::string TabIconPNG(const std::string& id, int tabIndex) const
	{
		// shared_ptr keeper — see SessionManager::Login comment for why
		// raw pointer outside the lock is unsafe.
		std::shared_ptr<ibWebSession> keeper;
		ibWebDocChildFrame* tab = nullptr;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			auto it = m_sessions.find(id);
			if (it == m_sessions.end()) return std::string();
			keeper = it->second;
			ibWebApplication* app = keeper->App();
			if (app == nullptr) return std::string();
			ibWebFrame* frame = app->GetFrame();
			if (frame == nullptr) return std::string();
			if (tabIndex < 0 ||
				static_cast<std::size_t>(tabIndex) >= frame->TabCount())
				return std::string();
			tab = frame->Tab(static_cast<std::size_t>(tabIndex));
		}
		if (tab == nullptr) return std::string();
		const wxIcon& icon = tab->GetIcon();
		if (!icon.IsOk()) return std::string();

		// wxBitmap(wxIcon) is a cheap convert; wxImage is what knows
		// PNG. wxMemoryOutputStream + wxImage::SaveFile(... PNG) is the
		// standard way to get raw bytes without wxFile.
		wxBitmap bmp(icon);
		wxImage  img = bmp.ConvertToImage();
		if (!img.IsOk()) return std::string();
		wxMemoryOutputStream mem;
		if (!img.SaveFile(mem, wxBITMAP_TYPE_PNG)) return std::string();
		const std::size_t n = mem.GetSize();
		std::string out(n, '\0');
		mem.CopyTo(out.data(), n);
		return out;
	}

	// Defined after OpenFormInSession below — needs the helper in scope.
	std::string OpenForm(const std::string& id, int metaID);
	std::string FireAction(const std::string& id, int controlID);
	std::string FireKind(const std::string& id, int controlID, const std::string& kind);
	std::string FireTextChange(const std::string& id, int controlID, const std::string& newValue);
	std::string FireToggle(const std::string& id, int controlID, bool checked);
	std::string ActiveHostJSON(const std::string& id);

	// Borrow the session's ibWebApplication for the duration of a
	// blocking operation (SSE wait). Returns null if id is unknown or
	// the session isn't authenticated. The pointer stays valid only
	// while the session is in the map — idle-sweep may erase in the
	// meantime; the caller's `wait_for` timeout is the safety net.
	ibWebApplication* FindApp(const std::string& id);

	std::string SessionInfo(const std::string& id);
	std::string ActivateTab(const std::string& id, int tabIndex);
	std::string CloseTab(const std::string& id, int tabIndex);

	void Clear()
	{
		std::unordered_map<std::string, std::shared_ptr<ibWebSession>> dying;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			dying.swap(m_sessions);
		}
		for (auto& kv : dying) {
			if (kv.second) kv.second->OnExit();
		}
	}

private:
	static std::string MakeRandomId()
	{
		// Shared format with ibGuid / sys_session.session — dashed UUID
		// so browser cookie, X-OES-Session header, SessionManager map
		// key, and the registry's session guid all read as the same
		// string (no extra parse step when one crosses layers).
		return ibGuid::newGuid().str();
	}

	// Idle-session sweep + metadata-change watch. Wakes every sweep
	// interval, collects session ids whose LastActiveMs is older than
	// the idle cutoff, destroys them outside the map-lock (OnExit runs
	// scripts + teardown that mustn't serialise every other request).
	// Also polls sys_config.file_guid — on change (designer deployed a
	// new configuration to the live DB), evicts every user session so
	// each client lands on a fresh login attempt. Stops on m_sweepStop.
	void SweepLoop()
	{
		using namespace std::chrono;
		// Tuning: 30-minute idle cutoff matches a reasonable "user
		// stepped away" window. Sweep every 15s — coarse enough that
		// the reaper + metadata watch stay cheap, fine enough that
		// deployed-config changes propagate within a Designer→try-it
		// roundtrip.
		// 2-minute idle cutoff — faster reclamation of cookie sessions
		// after browser tab close. The JS client fires a beforeunload
		// beacon to /logout (best-effort; some browsers drop it); this
		// cutoff is the guaranteed upper bound if the beacon misses.
		constexpr std::int64_t kIdleMs   = 2 * 60 * 1000;
		constexpr auto         kInterval = seconds(15);

		// Capture the metadata guid observed at server start. Compared
		// to the live sys_config.file_guid on every tick — any diff
		// means Designer pushed a new config; evict all user sessions
		// so their stale compiled state can't leak into responses.
		wxString lastObservedMetaGuid;
		if (activeMetaData != nullptr)
			lastObservedMetaGuid = activeMetaData->GetConfigGuid().str();

		for (;;) {
			{
				std::unique_lock<std::mutex> lk(m_sweepMtx);
				if (m_sweepCv.wait_for(lk, kInterval,
						[this]{ return m_sweepStop.load(); }))
					return;  // stopped
			}

			// 1) Idle eviction.
			const std::int64_t nowMs = duration_cast<milliseconds>(
				system_clock::now().time_since_epoch()).count();
			std::vector<std::string> victims;
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				for (auto& kv : m_sessions) {
					if (kv.second &&
						nowMs - kv.second->LastActiveMs() > kIdleMs) {
						victims.push_back(kv.first);
					}
				}
			}
			for (const std::string& id : victims) {
				std::cerr << "[sweep] evicting idle session " << id
					<< std::endl;
				Destroy(id);
			}

			// 2) Metadata-change watch. Polling fallback for the FB
			//    event-based notifier (isc_que_events) that will ship
			//    later. Runs against a borrowed pool connection so the
			//    main thread's DB ops aren't blocked. On shutdown the
			//    pool hands back nullptr — we simply skip the tick.
			CheckMetadataAndEvict(lastObservedMetaGuid);

			// 3) Admin-driven "reload" signal. `JobCheckSignal` on the
			//    registry thread latches a flag when an admin writes
			//    signal='reload' to any own row; we consume it here
			//    (one-shot) and evict every user session just like the
			//    metadata-change path. Kept separate from the metadata
			//    watch so an operator can force a re-login cycle even
			//    if the file_guid hasn't changed (e.g. ad-hoc kick-all
			//    from an admin console).
			if (ibSessionRegistry::Instance().ConsumeReloadRequest()) {
				std::cerr << "[signal] reload directive — evicting "
				          << "all user sessions" << std::endl;
				std::vector<std::string> ids;
				{
					std::lock_guard<std::mutex> lock(m_mutex);
					ids.reserve(m_sessions.size());
					for (auto& kv : m_sessions) ids.push_back(kv.first);
				}
				for (const std::string& id : ids) Destroy(id);
			}
		}
	}

	// Poll sys_config.file_guid against the observed guid. If changed,
	// update the cached guid, log the transition, and evict every
	// session — each client will get a 401 on its next request and
	// re-login, landing on the (soon-to-be-reloaded) fresh metadata.
	// Metadata reload itself is a follow-up — this first cut just
	// drops stale sessions and logs that the server binary needs to
	// catch up.
	void CheckMetadataAndEvict(wxString& lastObservedMetaGuid)
	{
		if (activeMetaData == nullptr) return;

		ibConnectionPool* pool = ibApplicationData::GetConnectionPool();
		if (pool == nullptr) return;

		std::shared_ptr<ibDatabaseLayer> conn = pool->Checkout();
		if (conn == nullptr) return;

		wxString currentGuid;
		try {
			// The deployed config lives in sys_config (ibConfigType_Load);
			// sys_config_save is the designer's draft. We only react to
			// the deployed guid so draft edits don't evict live users.
			ibResultSetGuard rs(conn,
				conn->RunQueryWithResults(wxT("SELECT file_guid FROM sys_config; ")));
			if (rs && rs->Next())
				currentGuid = rs->GetResultString(wxT("file_guid"));
		} catch (...) {
			// Swallow — transient DB errors shouldn't kill the sweep.
			// ibDatabaseLayerException is conditionally compiled so we
			// stay generic here.
			return;
		}

		if (currentGuid.IsEmpty() || currentGuid == lastObservedMetaGuid)
			return;

		std::cerr << "[metadata] file_guid changed: "
		          << lastObservedMetaGuid.ToUTF8().data() << " -> "
		          << currentGuid.ToUTF8().data()
		          << " — evicting all user sessions + reloading compile" << std::endl;
		lastObservedMetaGuid = currentGuid;
		// Bump the generation so clients polling /session can detect
		// that their compile context is stale even though the 401 that
		// follows doesn't itself carry a distinct error code.
		g_metaGeneration.fetch_add(1, std::memory_order_relaxed);

		// Evict every user session. Each HTTP request made afterwards
		// hits "unknown cookie → 401" and the client UI prompts for
		// re-login. Destroy runs OnExit → StopWorker (joins the
		// session's worker thread) so by the time the loop returns,
		// no session-scope runtime is live — safe to swap compile.
		std::vector<std::string> ids;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			ids.reserve(m_sessions.size());
			for (auto& kv : m_sessions) ids.push_back(kv.first);
		}
		for (const std::string& id : ids) Destroy(id);

		// Server-side recompile against the freshly-deployed config.
		//   CloseDatabase(forceCloseFlag) — runs OnBefore/After-CloseMetaObject
		//                                   + DestroyMainModule (tears down
		//                                   the module manager). The
		//                                   in-memory m_commonObject tree
		//                                   itself survives, so...
		//   LoadDatabase()                — re-reads the serialised
		//                                   binary from sys_config, clears
		//                                   the tree and rebuilds it from
		//                                   the NEW blob that designer
		//                                   just deployed. Without this
		//                                   step Run would happily re-run
		//                                   the old tree; no visible
		//                                   config change for clients.
		//   RunDatabase()                 — CreateMainModule, new
		//                                   compile. StartMainModule
		//                                   (the old tail of RunDatabase)
		//                                   moved to per-session
		//                                   InitRuntimeForSession, so
		//                                   this finishes with a fresh
		//                                   compile and the next login
		//                                   spins runtime up on it.
		if (activeMetaData != nullptr) {
			try {
				if (!activeMetaData->CloseDatabase(forceCloseFlag)) {
					std::cerr << "[metadata] CloseDatabase FAILED — server restart required" << std::endl;
				}
				else if (!activeMetaData->LoadDatabase()) {
					std::cerr << "[metadata] LoadDatabase FAILED — server restart required" << std::endl;
				}
				else if (!activeMetaData->RunDatabase()) {
					std::cerr << "[metadata] RunDatabase FAILED — server restart required" << std::endl;
				}
				else {
					std::cerr << "[metadata] recompile OK; next login picks up new config" << std::endl;
				}
			}
			catch (...) {
				std::cerr << "[metadata] reload threw — server restart required" << std::endl;
			}
		}
	}

	mutable std::mutex m_mutex;
	// shared_ptr (not unique_ptr) so handler methods can grab a copy
	// under m_mutex and keep the session alive while they work outside
	// the lock. Destroy() removing the map entry no longer races with
	// in-flight Login / FireAction / OpenForm — the session survives
	// until every borrower drops its copy.
	std::unordered_map<std::string, std::shared_ptr<ibWebSession>> m_sessions;

	// Sweep-thread plumbing. m_sweepCv lets the dtor unblock the
	// in-flight `wait_for` instead of waiting the full interval.
	std::thread              m_sweepThread;
	std::mutex               m_sweepMtx;
	std::condition_variable  m_sweepCv;
	std::atomic<bool>        m_sweepStop { false };
};

SessionManager& Sessions()
{
	static SessionManager s_instance;
	return s_instance;
}

std::atomic<bool> g_initialized{ false };
std::mutex        g_initMutex;
std::string       g_lastError;

// HTTP host:port the container is bound on. Set by
// wfrontendSetServerAddress from the host (wenterprise-server main);
// read by ibWebSession::Login to stamp `sys_session.address` when a
// cookie session is registered.
std::mutex        g_addrMutex;
std::string       g_serverAddress;

void RememberError()
{
	const wxString& msg = ibBackendException::GetLastError();
	g_lastError = msg.IsEmpty() ? "unknown backend error" : std::string(msg.mb_str(wxConvUTF8));
}

} // namespace

// ---------------------------------------------------------------------------
// Exports
// ---------------------------------------------------------------------------

extern "C" WFRONTEND_API const char* wfrontendVersion()
{
	return "wfrontend 0.1 (web)";
}

WFRONTEND_API void wfrontendSetServerAddress(const std::string& host, int port)
{
	std::lock_guard<std::mutex> lk(g_addrMutex);
	g_serverAddress = host + ":" + std::to_string(port);
}

WFRONTEND_API std::string wfrontendServerAddress()
{
	std::lock_guard<std::mutex> lk(g_addrMutex);
	return g_serverAddress;
}

namespace {

bool FinishConnect(const std::string& ibUser, const std::string& ibPassword)
{
	if (!appData->Connect(
		wxString::FromUTF8(ibUser.c_str()),
		wxString::FromUTF8(ibPassword.c_str())))
	{
		RememberError();
		appDataDestroy();
		return false;
	}
	return true;
}

} // namespace

// Register the PNG handler once per process so wxImage::SaveFile(...,
// wxBITMAP_TYPE_PNG) works for tab-icon encoding (GET /tab/<i>/icon).
// wfrontend.dll links against wxCore's image support; wes.exe itself
// doesn't, hence registration has to happen here rather than in main.
static void EnsurePngHandler()
{
	if (!wxImage::FindHandler(wxBITMAP_TYPE_PNG))
		wxImage::AddHandler(new wxPNGHandler);
}

WFRONTEND_API bool wfrontendInitFile(
	const std::string& filePath,
	const std::string& ibUser,
	const std::string& ibPassword,
	const std::string& locale)
{
	std::lock_guard<std::mutex> lock(g_initMutex);
	if (g_initialized.load())
		return true;  // idempotent

	EnsurePngHandler();

	if (!appDataCreateFile(ibRunMode::eWEB_ENTERPRISE_MODE,
		wxString::FromUTF8(filePath.c_str()),
		wxString::FromUTF8(locale.c_str())))
	{
		RememberError();
		return false;
	}

	if (!FinishConnect(ibUser, ibPassword))
		return false;

	g_initialized.store(true);
	return true;
}

WFRONTEND_API bool wfrontendInitServer(
	const std::string& server,
	const std::string& port,
	const std::string& user,
	const std::string& password,
	const std::string& database,
	const std::string& ibUser,
	const std::string& ibPassword,
	const std::string& locale)
{
	std::lock_guard<std::mutex> lock(g_initMutex);
	if (g_initialized.load())
		return true;

	EnsurePngHandler();

	if (!appDataCreateServer(ibRunMode::eWEB_ENTERPRISE_MODE,
		wxString::FromUTF8(server.c_str()),
		wxString::FromUTF8(port.c_str()),
		wxString::FromUTF8(user.c_str()),
		wxString::FromUTF8(password.c_str()),
		wxString::FromUTF8(database.c_str()),
		wxString::FromUTF8(locale.c_str())))
	{
		RememberError();
		return false;
	}

	if (!FinishConnect(ibUser, ibPassword))
		return false;

	g_initialized.store(true);
	return true;
}

WFRONTEND_API bool wfrontendKickSessionByGuid(const std::string& sessionGuid)
{
	// Delegates to the registry's admin helper — same UPDATE path the
	// designer dialog uses, so kick-via-HTTP and kick-via-designer share
	// exactly one implementation.
	return ibSessionRegistry::Instance().Kick(
		wxString::FromUTF8(sessionGuid.c_str()));
}

WFRONTEND_API bool wfrontendReloadSessionByGuid(const std::string& sessionGuid)
{
	return ibSessionRegistry::Instance().Reload(
		wxString::FromUTF8(sessionGuid.c_str()));
}

WFRONTEND_API void wfrontendShutdown()
{
	std::lock_guard<std::mutex> lock(g_initMutex);
	if (!g_initialized.exchange(false))
		return;
	Sessions().Clear();
	appDataDestroy();
	g_lastError.clear();
}

WFRONTEND_API std::string wfrontendLastError()
{
	std::lock_guard<std::mutex> lock(g_initMutex);
	return g_lastError;
}

WFRONTEND_API std::string wfrontendConfigName()
{
	if (!g_initialized.load() || activeMetaData == nullptr)
		return {};
	const wxString& n = activeMetaData->GetConfigName();
	return std::string(n.mb_str(wxConvUTF8));
}

WFRONTEND_API bool wfrontendHasUsers()
{
	if (!g_initialized.load() || appData == nullptr)
		return false;
	// HasAllowedUser() is private — use the public user list getter
	// instead and treat any entry as "auth required".
	return !appData->GetAllowedUser().empty();
}

namespace {

// Helper: scan common-object children of a given CLSID into a JSON
// array of {id, name, synonym} entries for the navigation panel.
nlohmann::json CollectSection(const ibValueMetaObject* common, const ibClassID& clsid)
{
	nlohmann::json items = nlohmann::json::array();
	if (common == nullptr)
		return items;

	for (unsigned int i = 0; i < common->GetChildCount(); ++i) {
		ibValueMetaObject* child = dynamic_cast<ibValueMetaObject*>(common->GetChild(i));
		if (child == nullptr || child->GetClassType() != clsid || child->IsDeleted())
			continue;
		items.push_back({
			{ "id",      child->GetMetaID() },
			{ "name",    std::string(child->GetName().mb_str(wxConvUTF8)) },
			{ "synonym", std::string(child->GetSynonym().mb_str(wxConvUTF8)) },
		});
	}
	return items;
}

} // namespace

WFRONTEND_API std::string wfrontendMenuJSON()
{
	if (!g_initialized.load() || activeMetaData == nullptr)
		return "{}";

	ibValueMetaObjectConfiguration* common = activeMetaData->GetCommonMetaObject();
	if (common == nullptr)
		return "{}";

	// Same set of "main business object" kinds the desktop shows in
	// the object tree — catalogs, documents, reports and so on.
	nlohmann::json root = {
		{ "configuration", std::string(activeMetaData->GetConfigName().mb_str(wxConvUTF8)) },
		{ "sections", {
			{ { "type", "catalogs"  }, { "caption", "Catalogs"  }, { "items", CollectSection(common, g_metaCatalogCLSID)              } },
			{ { "type", "documents" }, { "caption", "Documents" }, { "items", CollectSection(common, g_metaDocumentCLSID)             } },
			{ { "type", "enums"     }, { "caption", "Enums"     }, { "items", CollectSection(common, g_metaEnumerationCLSID)          } },
			{ { "type", "constants" }, { "caption", "Constants" }, { "items", CollectSection(common, g_metaConstantCLSID)             } },
			{ { "type", "reports"   }, { "caption", "Reports"   }, { "items", CollectSection(common, g_metaReportCLSID)               } },
			{ { "type", "dataproc"  }, { "caption", "Data processors" }, { "items", CollectSection(common, g_metaDataProcessorCLSID)  } },
			{ { "type", "iregs"     }, { "caption", "Information registers"  }, { "items", CollectSection(common, g_metaInformationRegisterCLSID)  } },
			{ { "type", "aregs"     }, { "caption", "Accumulation registers" }, { "items", CollectSection(common, g_metaAccumulationRegisterCLSID) } },
		}},
	};
	return root.dump(2);
}

WFRONTEND_API std::string wfrontendCreateSession()
{
	return Sessions().Create();
}

WFRONTEND_API std::string wfrontendCreateSessionWithId(const std::string& id)
{
	return Sessions().CreateWith(id);
}

WFRONTEND_API bool wfrontendLogin(const std::string& sessionId,
	const std::string& user, const std::string& password)
{
	return Sessions().Login(sessionId, user, password);
}

WFRONTEND_API bool wfrontendSessionExists(const std::string& sessionId)
{
	return Sessions().Exists(sessionId);
}

WFRONTEND_API void wfrontendDestroySession(const std::string& sessionId)
{
	Sessions().Destroy(sessionId);
}

WFRONTEND_API std::size_t wfrontendSessionCount()
{
	return Sessions().Count();
}

WFRONTEND_API std::size_t wfrontendSessionTabCount(const std::string& sessionId)
{
	Sessions().Touch(sessionId);
	return Sessions().TabCount(sessionId);
}

WFRONTEND_API std::string wfrontendTabIconPNG(const std::string& sessionId, int tabIndex)
{
	Sessions().Touch(sessionId);
	return Sessions().TabIconPNG(sessionId, tabIndex);
}

namespace {

// Open-form implementation, reached via the session manager's slot.
std::string OpenFormInSession(ibWebSession* session, int metaID)
{
	if (session == nullptr || !session->IsAuthenticated() || activeMetaData == nullptr)
		return "{}";

	// Pin the tab's ibSession as Current() on this HTTP worker thread so
	// moduleManager->GetProcUnit() resolves via session->GetProcUnitFor
	// (the main / common module ProcUnits InitRuntimeForSession attached
	// at Login). Without the scope, Current() is null on worker threads,
	// the form's parent-ProcUnit comes up nullptr, and Execute throws
	// "compilation failed (#2)" at form-open.
	SessionScope scope(session->Session());

	// Forms live one or two levels deep under the configuration (common
	// forms directly, object-owned forms under their catalog/document).
	// Filtering by both CLSIDs (common-form + regular form) makes the
	// id lookup unambiguous — FindAnyObjectByFilter without a CLSID
	// can pick up a same-id non-form sibling.
	// Walk the tree ourselves to avoid the templated FindAnyObjectByFilter
	// overload-resolution dance — we only need one specific hit.
	ibValueMetaObjectFormBase* metaForm = nullptr;
	{
		std::function<void(const ibValueMetaObject*)> walk;
		walk = [&](const ibValueMetaObject* node) {
			if (metaForm != nullptr || node == nullptr) return;
			for (unsigned int i = 0; i < node->GetChildCount() && metaForm == nullptr; ++i) {
				auto* child = dynamic_cast<ibValueMetaObject*>(node->GetChild(i));
				if (child == nullptr || child->IsDeleted()) continue;
				if (auto* form = dynamic_cast<ibValueMetaObjectFormBase*>(child)) {
					if (form->GetMetaID() == metaID) { metaForm = form; return; }
				}
				walk(child);
			}
		};
		walk(activeMetaData->GetCommonMetaObject());
	}
	if (metaForm == nullptr)
		return "{}";

	ibWebApplication* app = session->App();
	if (app == nullptr) return "{}";

	ibWebFrame* frame = app->GetFrame();
	if (frame == nullptr) return "{}";

	// CreateAndBuildForm runs the LoadFormData / BuildForm fallback for
	// forms without a designer-drawn layout (empty tabs otherwise). It
	// lands back in ibWebFrame::CreateNewForm via ibBackendValueForm::
	// CreateNewForm, which is now a pure factory — no re-entry.
	//
	// Note: we deliberately do NOT dispatch via
	// ibBackendCommandItem::ShowFormByCommandType here. That path was
	// designed for the "interface subsystem" buttons in enterprise.exe
	// sidebar (see mainFrameEnterpriseInterface.cpp:703) where each
	// subsection maps to a command type (FormList / FormSelect / etc.).
	// The web sidebar opens a specific form by metaID — closer to
	// designer's metadata-tree "open form" click.
	ibBackendValueForm* form = metaForm->CreateAndBuildForm(
		metaForm, nullptr, nullptr, wxNullUniqueKey);
	if (form == nullptr)
		return "{}";
	form->ShowForm();

	ibWebDocChildFrame* tab = frame->Tab(frame->ActiveTab());
	if (tab == nullptr) return "{}";

	ibVisualHostClient* host = tab->GetHost();
	if (host == nullptr) return "{}";

	// No CreateAndUpdateVisualHost here: CreateNewForm → ShowForm →
	// ibFormVisualEditView::OnCreate already did the initial build.
	// Doing it again would be a redundant Clear+Create+Update. The
	// host's tree is fresh — just serialise.
	app->MarkDirty();

	return host->ToJSON().dump(2);
}

} // namespace

std::string SessionManager::OpenForm(const std::string& id, int metaID)
{
	std::shared_ptr<ibWebSession> keeper;
	ibWebSession* s = nullptr;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_sessions.find(id);
		if (it == m_sessions.end()) return "{}";
		keeper = it->second;
		s = keeper.get();
	}
	// Out of the lock — form creation touches appData singletons and
	// may take non-trivial time; other sessions shouldn't serialise
	// behind it.
	return OpenFormInSession(s, metaID);
}

WFRONTEND_API std::string wfrontendOpenForm(const std::string& sessionId, int metaID)
{
	Sessions().Touch(sessionId);
	return Sessions().OpenForm(sessionId, metaID);
}

namespace {

// Recursively visit every metadata node and collect form descendants.
void CollectForms(const ibValueMetaObject* node, nlohmann::json& out)
{
	if (node == nullptr)
		return;
	for (unsigned int i = 0; i < node->GetChildCount(); ++i) {
		const ibValueMetaObject* child = dynamic_cast<ibValueMetaObject*>(node->GetChild(i));
		if (child == nullptr || child->IsDeleted())
			continue;
		if (const auto* form = dynamic_cast<const ibValueMetaObjectFormBase*>(child)) {
			const ibValueMetaObject* owner = dynamic_cast<const ibValueMetaObject*>(form->GetParent());
			out.push_back({
				{ "id",         form->GetMetaID() },
				{ "name",       std::string(form->GetName().mb_str(wxConvUTF8)) },
				{ "synonym",    std::string(form->GetSynonym().mb_str(wxConvUTF8)) },
				{ "owner_id",   owner ? (int)owner->GetMetaID() : 0 },
				{ "owner_name", owner ? std::string(owner->GetName().mb_str(wxConvUTF8)) : std::string() },
			});
		}
		CollectForms(child, out);
	}
}

} // namespace

namespace {

std::string FireActionInSession(ibWebSession* session, int controlID)
{
	// HTTP-layer shim: validate session auth, submit the dispatch +
	// JSON-build to the session worker thread, wait for the result.
	// The worker is the sole script-runner — HTTP threads never touch
	// ibProcUnit directly, so concurrent requests on the same session
	// serialise naturally through the worker queue.
	if (session == nullptr || !session->IsAuthenticated()) return "{}";
	ibWebApplication* app = session->App();
	if (app == nullptr) return "{}";

	return app->RunOnWorker([app, controlID]() -> std::string {
		try {
			if (!app->DispatchControlAction(controlID))
				return "{}";
		}
		catch (const ibBackendException&) {
			return R"({"error":"script exception"})";
		}
		catch (...) {
			return R"({"error":"unknown exception"})";
		}
		ibVisualHostClient* host = app->GetActiveHost();
		return host != nullptr ? host->ToJSON().dump(2) : std::string("{}");
	}).get();
}

} // namespace

std::string SessionManager::FireAction(const std::string& id, int controlID)
{
	std::shared_ptr<ibWebSession> keeper;
	ibWebSession* s = nullptr;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_sessions.find(id);
		if (it == m_sessions.end()) return "{}";
		keeper = it->second;
		s = keeper.get();
	}
	return FireActionInSession(s, controlID);
}

WFRONTEND_API std::string wfrontendFireAction(const std::string& sessionId, int controlID)
{
	Sessions().Touch(sessionId);
	return Sessions().FireAction(sessionId, controlID);
}

namespace {
std::string FireKindInSession(ibWebSession* session, int controlID,
	const std::string& kind)
{
	if (session == nullptr || !session->IsAuthenticated()) return "{}";
	ibWebApplication* app = session->App();
	if (app == nullptr) return "{}";

	return app->RunOnWorker([app, controlID, kind]() -> std::string {
		try {
			const wxString wkind(kind.c_str(), wxConvUTF8);
			if (!app->Dispatch(controlID, wkind, wxString()))
				return "{}";
		}
		catch (const ibBackendException&) {
			return R"({"error":"script exception"})";
		}
		catch (...) {
			return R"({"error":"unknown exception"})";
		}
		ibVisualHostClient* host = app->GetActiveHost();
		return host != nullptr ? host->ToJSON().dump(2) : std::string("{}");
	}).get();
}
} // namespace

std::string SessionManager::FireKind(const std::string& id, int controlID,
	const std::string& kind)
{
	std::shared_ptr<ibWebSession> keeper;
	ibWebSession* s = nullptr;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_sessions.find(id);
		if (it == m_sessions.end()) return "{}";
		keeper = it->second;
		s = keeper.get();
	}
	return FireKindInSession(s, controlID, kind);
}

WFRONTEND_API std::string wfrontendFireKind(const std::string& sessionId,
	int controlID, const std::string& kind)
{
	Sessions().Touch(sessionId);
	return Sessions().FireKind(sessionId, controlID, kind);
}

namespace {
std::string FireTextChangeInSession(ibWebSession* session, int controlID,
	const std::string& newValue)
{
	if (session == nullptr || !session->IsAuthenticated()) return "{}";
	ibWebApplication* app = session->App();
	if (app == nullptr) return "{}";

	return app->RunOnWorker([app, controlID, newValue]() -> std::string {
		try {
			const wxString w(newValue.c_str(), wxConvUTF8);
			if (!app->DispatchTextChange(controlID, w))
				return "{}";
		}
		catch (const ibBackendException&) {
			return R"({"error":"script exception"})";
		}
		catch (...) {
			return R"({"error":"unknown exception"})";
		}
		ibVisualHostClient* host = app->GetActiveHost();
		return host != nullptr ? host->ToJSON().dump(2) : std::string("{}");
	}).get();
}
} // namespace

std::string SessionManager::FireTextChange(const std::string& id, int controlID,
	const std::string& newValue)
{
	std::shared_ptr<ibWebSession> keeper;
	ibWebSession* s = nullptr;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_sessions.find(id);
		if (it == m_sessions.end()) return "{}";
		keeper = it->second;
		s = keeper.get();
	}
	return FireTextChangeInSession(s, controlID, newValue);
}

WFRONTEND_API std::string wfrontendFireTextChange(const std::string& sessionId,
	int controlID, const std::string& newValue)
{
	Sessions().Touch(sessionId);
	return Sessions().FireTextChange(sessionId, controlID, newValue);
}

namespace {
std::string FireToggleInSession(ibWebSession* session, int controlID, bool checked)
{
	if (session == nullptr || !session->IsAuthenticated()) return "{}";
	ibWebApplication* app = session->App();
	if (app == nullptr) return "{}";

	return app->RunOnWorker([app, controlID, checked]() -> std::string {
		try {
			if (!app->DispatchToggle(controlID, checked))
				return "{}";
		}
		catch (const ibBackendException&) {
			return R"({"error":"script exception"})";
		}
		catch (...) {
			return R"({"error":"unknown exception"})";
		}
		ibVisualHostClient* host = app->GetActiveHost();
		return host != nullptr ? host->ToJSON().dump(2) : std::string("{}");
	}).get();
}
} // namespace

std::string SessionManager::FireToggle(const std::string& id, int controlID, bool checked)
{
	std::shared_ptr<ibWebSession> keeper;
	ibWebSession* s = nullptr;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_sessions.find(id);
		if (it == m_sessions.end()) return "{}";
		keeper = it->second;
		s = keeper.get();
	}
	return FireToggleInSession(s, controlID, checked);
}

WFRONTEND_API std::string wfrontendFireToggle(const std::string& sessionId,
	int controlID, bool checked)
{
	Sessions().Touch(sessionId);
	return Sessions().FireToggle(sessionId, controlID, checked);
}

ibWebApplication* SessionManager::FindApp(const std::string& id)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_sessions.find(id);
	if (it == m_sessions.end() || it->second == nullptr) return nullptr;
	if (!it->second->IsAuthenticated()) return nullptr;
	return it->second->App();
}

std::string SessionManager::ActiveHostJSON(const std::string& id)
{
	// Route through the session worker so ToJSON serialises with any
	// in-flight mutation (SetValue from /change, m_selValue rewrites
	// from Clear, etc). Previously this ran directly on the HTTP
	// thread and could read a half-mutated wxString mid-walk — user
	// saw heap corruption in ConvertedBuffer::Extend when typing +
	// the 2s poll fired concurrently (stack trace 2026-04-19).
	std::shared_ptr<ibWebSession> keeper;
	ibWebSession* s = nullptr;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_sessions.find(id);
		if (it == m_sessions.end()) return "{}";
		keeper = it->second;
		s = keeper.get();
	}
	if (s == nullptr || !s->IsAuthenticated()) return "{}";
	ibWebApplication* app = s->App();
	if (app == nullptr) return "{}";
	return app->RunOnWorker([app]() -> std::string {
		ibVisualHostClient* host = app->GetActiveHost();
		try {
			return host != nullptr ? host->ToJSON().dump(2) : std::string("{}");
		}
		catch (...) {
			return std::string("{}");
		}
	}).get();
}

WFRONTEND_API std::string wfrontendActiveHostJSON(const std::string& sessionId)
{
	Sessions().Touch(sessionId);
	return Sessions().ActiveHostJSON(sessionId);
}

WFRONTEND_API WfrontendLiveUpdate wfrontendLiveWait(
	const std::string& sessionId, std::uint64_t lastSeen, int timeoutMs)
{
	// Pin the session in scope so it can't be swept out from under the
	// waiter. `Sessions().m_mutex` only guards the map — the session
	// object itself is owned by the unique_ptr in the map, which the
	// idle-sweep may erase. Take the mutex briefly, grab the raw ptr,
	// drop the mutex, wait on the app's CV. If the session dies mid-wait
	// the CV won't fire from our side — rely on the timeout to release.
	// Future hardening: shared_ptr<ibWebSession> so the waiter pins
	// lifetime. For today's synchronous flow the timeout is acceptable.
	Sessions().Touch(sessionId);

	ibWebApplication* app = Sessions().FindApp(sessionId);
	if (app == nullptr) {
		WfrontendLiveUpdate r;
		r.seq  = lastSeen;
		r.json = "{}";
		return r;
	}

	const std::uint64_t newSeq = app->WaitForChange(lastSeen, timeoutMs);

	WfrontendLiveUpdate r;
	r.seq  = newSeq;
	r.json = Sessions().ActiveHostJSON(sessionId);
	return r;
}

namespace {

std::string SessionInfoFromSession(ibWebSession* s)
{
	nlohmann::json j;
	j["authenticated"] = s != nullptr && s->IsAuthenticated();
	j["tabs"] = nlohmann::json::array();
	j["metaGeneration"] = g_metaGeneration.load(std::memory_order_relaxed);
	if (s == nullptr || !s->IsAuthenticated()) return j.dump(2);

	ibWebApplication* app = s->App();
	if (app == nullptr) return j.dump(2);
	ibWebFrame* frame = app->GetFrame();
	if (frame == nullptr) return j.dump(2);

	j["tabCount"]  = (std::size_t)frame->TabCount();
	j["activeTab"] = (std::size_t)frame->ActiveTab();
	for (std::size_t i = 0; i < frame->TabCount(); ++i) {
		ibWebDocChildFrame* tab = frame->Tab(i);
		nlohmann::json t;
		t["index"] = i;
		t["modified"] = false;
		t["hasIcon"]  = false;
		if (tab != nullptr) {
			t["title"] = std::string(tab->GetTitle().mb_str(wxConvUTF8));
			t["hasIcon"] = tab->GetIcon().IsOk();
			if (auto* host = tab->GetHost()) {
				if (auto* form = host->GetValueForm()) {
					wxString name = form->GetControlTitle();
					if (name.IsEmpty()) {
						// Fallbacks when the form's Title property and
						// metaObject/metaForm synonyms are all empty: prefer
						// the metaForm's internal name over the client's
						// "#N" default. Matches how the forms list in the
						// sidebar resolves to .synonym || .name.
						if (const auto* metaForm = form->GetFormMetaObject())
							name = metaForm->GetName();
					}
					t["formName"] = std::string(name.mb_str(wxConvUTF8));
					// Modified state: desktop renders a '*' prefix in the
					// tab label when wxDocument::IsModified() is true; the
					// form's m_formModified mirrors that.
					t["modified"] = form->IsModified();
				}
			}
		}
		j["tabs"].push_back(std::move(t));
	}
	return j.dump(2);
}

} // namespace

std::string SessionManager::SessionInfo(const std::string& id)
{
	ibWebSession* s = nullptr;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_sessions.find(id);
		if (it == m_sessions.end()) return ([]{
			nlohmann::json j;
			j["authenticated"] = false;
			j["tabs"] = nlohmann::json::array();
			j["metaGeneration"] = g_metaGeneration.load(std::memory_order_relaxed);
			return j.dump();
		}());
		s = it->second.get();
	}
	// Route through the session worker: SessionInfoFromSession iterates
	// tabs and calls ibValueForm::GetControlTitle → ibPropertyTString
	// ::GetValueAsTranslateString → ibBackendLocalization::
	// CreateLocalizationArray, which touches shared non-thread-safe state
	// (locale cache, wxString internals). Worker may be mutating the same
	// state concurrently — user saw a crash in CreateLocalizationArray
	// from the /session HTTP thread (dump 2026-04-19 17:12). Same pattern
	// as ActiveHostJSON.
	ibWebApplication* app = s != nullptr ? s->App() : nullptr;
	if (app == nullptr) return SessionInfoFromSession(s);
	return app->RunOnWorker([s]() -> std::string {
		try { return SessionInfoFromSession(s); }
		catch (...) { return std::string(([]{
			nlohmann::json j;
			j["authenticated"] = false;
			j["tabs"] = nlohmann::json::array();
			j["metaGeneration"] = g_metaGeneration.load(std::memory_order_relaxed);
			return j.dump();
		}())); }
	}).get();
}

WFRONTEND_API std::string wfrontendSessionInfoJSON(const std::string& sessionId)
{
	Sessions().Touch(sessionId);
	return Sessions().SessionInfo(sessionId);
}

namespace {

// Shared helper: switch the frame's active tab index and re-walk the new
// active host so the browser gets the switched-to form's JSON back as
// the response body. "{}" on any precondition miss.
std::string ActivateTabInSession(ibWebSession* session, int tabIndex)
{
	if (session == nullptr || !session->IsAuthenticated())
		return "{}";
	ibWebApplication* app = session->App();
	if (app == nullptr) return "{}";

	// Route through the session worker — tab switching rebuilds the
	// control tree, same shared state the worker's script runs touch.
	// Doing it on the HTTP thread lets timer ticks / pending dispatches
	// race with the rebuild and crash on half-destroyed nodes.
	return app->RunOnWorker([app, tabIndex]() -> std::string {
		ibWebFrame* frame = app->GetFrame();
		if (frame == nullptr) return "{}";
		if (tabIndex < 0 || static_cast<std::size_t>(tabIndex) >= frame->TabCount())
			return "{}";

		frame->SetActiveTab(static_cast<std::size_t>(tabIndex));

		ibWebDocChildFrame* tab = frame->Tab(frame->ActiveTab());
		if (tab == nullptr) return "{}";
		ibVisualHostClient* host = tab->GetHost();
		if (host == nullptr) return "{}";
		// No rebuild: target tab's ibWebWindow tree was built once at
		// OnCreate and has stayed in sync through control setters.
		// Switching active tab is metadata-only (frame->SetActiveTab) —
		// the destination host's JSON is already current.
		app->MarkDirty();
		return host->ToJSON().dump(2);
	}).get();
}

std::string CloseTabInSession(ibWebSession* session, int tabIndex)
{
	if (session == nullptr || !session->IsAuthenticated())
		return "{}";
	ibWebApplication* app = session->App();
	if (app == nullptr) return "{}";

	// Worker-serialised: tab destruction tears down the control tree
	// and releases the form's ibValuePtr refs. Any pending worker task
	// (script, timer, dispatch) holding a pointer into that tree
	// finishes first because the worker queue is single-threaded.
	return app->RunOnWorker([app, tabIndex]() -> std::string {
		ibWebFrame* frame = app->GetFrame();
		if (frame == nullptr) return "{}";
		if (tabIndex < 0 || static_cast<std::size_t>(tabIndex) >= frame->TabCount())
			return "{}";

		// CloseTab fires beforeClose/onClose on the dying form and
		// returns false if beforeClose vetoed. On veto the tab stays
		// open and we return the current active tree so the client
		// re-renders (browser had optimistically removed the tab DOM).
		if (!frame->CloseTab(static_cast<std::size_t>(tabIndex))) {
			ibWebDocChildFrame* tab = frame->Tab(frame->ActiveTab());
			if (tab == nullptr) return "{}";
			ibVisualHostClient* host = tab->GetHost();
			return host != nullptr ? host->ToJSON().dump(2) : std::string("{}");
		}

		// CloseTab now only marks the form for close. Drain here so the
		// tab is actually erased and DeleteAllViews runs — without this
		// the doc / host / form ibValuePtr refs stay live and leak.
		frame->DrainPendingCloses();

		if (frame->TabCount() == 0) { app->MarkDirty(); return "{}"; }
		ibWebDocChildFrame* tab = frame->Tab(frame->ActiveTab());
		if (tab == nullptr) return "{}";
		ibVisualHostClient* host = tab->GetHost();
		if (host == nullptr) return "{}";
		// No rebuild: the now-active tab's tree has been kept in sync
		// with its form's state through control setters; just serialise.
		app->MarkDirty();
		return host->ToJSON().dump(2);
	}).get();
}

} // namespace

std::string SessionManager::ActivateTab(const std::string& id, int tabIndex)
{
	std::shared_ptr<ibWebSession> keeper;
	ibWebSession* s = nullptr;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_sessions.find(id);
		if (it == m_sessions.end()) return "{}";
		keeper = it->second;
		s = keeper.get();
	}
	return ActivateTabInSession(s, tabIndex);
}

std::string SessionManager::CloseTab(const std::string& id, int tabIndex)
{
	std::shared_ptr<ibWebSession> keeper;
	ibWebSession* s = nullptr;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_sessions.find(id);
		if (it == m_sessions.end()) return "{}";
		keeper = it->second;
		s = keeper.get();
	}
	return CloseTabInSession(s, tabIndex);
}

WFRONTEND_API std::string wfrontendActivateTab(const std::string& sessionId, int tabIndex)
{
	Sessions().Touch(sessionId);
	return Sessions().ActivateTab(sessionId, tabIndex);
}

WFRONTEND_API std::string wfrontendCloseTab(const std::string& sessionId, int tabIndex)
{
	Sessions().Touch(sessionId);
	return Sessions().CloseTab(sessionId, tabIndex);
}

WFRONTEND_API std::string wfrontendCtorsJSON()
{
	nlohmann::json arr = nlohmann::json::array();
	// Enumerate across all object-type buckets so we see controls,
	// system types, and plain values alike.
	for (int t = 0; t <= (int)ibCtorObjectType::ibCtorObjectType_object_control; ++t) {
		auto list = ibValue::GetListCtorsByType(static_cast<ibCtorObjectType>(t));
		for (auto* ctor : list) {
			if (ctor == nullptr) continue;
			arr.push_back({
				{ "className", std::string(ctor->GetClassName().mb_str(wxConvUTF8)) },
				{ "objectType", t },
			});
		}
	}
	return arr.dump(2);
}

WFRONTEND_API std::string wfrontendFormsJSON()
{
	nlohmann::json arr = nlohmann::json::array();
	if (g_initialized.load() && activeMetaData != nullptr)
		CollectForms(activeMetaData->GetCommonMetaObject(), arr);
	return arr.dump(2);
}
