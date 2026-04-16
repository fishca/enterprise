#include "appData.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/databaseLayer/databaseErrorCodes.h"

#include "backend/backend_mainFrame.h"
#include "backend/backend_exception.h"

///////////////////////////////////////////////////////////////////////////////
#define timeInterval 5
///////////////////////////////////////////////////////////////////////////////

wxCriticalSection ibApplicationData::ibApplicationDataSessionUpdater::ms_sessionLocker;

///////////////////////////////////////////////////////////////////////////////
//								ibApplicationDataSessionUpdater
///////////////////////////////////////////////////////////////////////////////

void ibApplicationData::ibApplicationDataSessionUpdater::Job_ClearLostSession()
{
	m_session_db->BeginTransaction();

	wxDateTime prevDateTime = m_currentDateTime.GetValue();
	(void)prevDateTime.Subtract(wxTimeSpan(0, 0, timeInterval));

	// Prepared statement avoids string formatting of the date and the SQL-injection surface.
	// The previous code also did a dead SELECT before the DELETE (result never read) — dropped.
	ibStatementGuard stmt(m_session_db,
		m_session_db->PrepareStatement(
			wxT("DELETE FROM %s WHERE lastActive < ?;"),
			session_table)
	);

	if (stmt) {
		stmt->SetParamDate(1, prevDateTime);
		stmt->RunQuery();
	}

	m_session_db->Commit();
}

void ibApplicationData::ibApplicationDataSessionUpdater::Job_CalcActiveSession()
{
	m_session_db->BeginTransaction();

	// Use a prepared statement for the GUID lookup — avoids SQL injection via session GUID
	// and lets the driver cache the plan across every 3-second tick.
	ibStatementGuard selectStmt(m_session_db,
		m_session_db->PrepareStatement(
			wxT("SELECT session, userName, lastActive FROM %s WHERE session = ?;"),
			session_table)
	);
	if (selectStmt) {
		selectStmt->SetParamString(1, m_session.str());
	}
	ibResultSetGuard selectResult(m_session_db,
		selectStmt ? selectStmt->RunQueryWithResults() : nullptr);

	const wxDateTime& currentTime = wxDateTime::Now();

	if (selectResult && selectResult->Next()) {

		// Prepared UPDATE: plugs user-name SQL-injection (if name contains an apostrophe).
		ibStatementGuard updateStmt(m_session_db,
			m_session_db->PrepareStatement(
				wxT("UPDATE %s SET lastActive = ?, userName = ? WHERE session = ?;"),
				session_table)
		);
		if (updateStmt) {
			updateStmt->SetParamDate(1, currentTime);
			updateStmt->SetParamString(2, appData->GetUserName());
			updateStmt->SetParamString(3, m_session.str());
			updateStmt->RunQuery();
		}
	}
	else {

		//start lost session
		ibStatementGuard insertStmt(m_session_db,
			m_session_db->PrepareStatement(
				wxT("INSERT INTO %s (session, userName, application, started, lastActive, computer) VALUES (?,?,?,?,?,?);"),
				session_table)
		);

		if (insertStmt) {
			insertStmt->SetParamString(1, m_session.str());
			insertStmt->SetParamString(2, appData->GetUserName()); //empty user!
			insertStmt->SetParamInt(3, appData->GetAppMode());
			insertStmt->SetParamDate(4, appData->GetStartedDate());
			insertStmt->SetParamDate(5, currentTime);
			insertStmt->SetParamString(6, appData->GetComputerName());

			insertStmt->RunQuery();
		}
	}

	m_session_db->Commit();
}

void ibApplicationData::ibApplicationDataSessionUpdater::Job_UpdateActiveSession()
{
	// Build the snapshot OUTSIDE the critical section — the old code held ms_sessionLocker
	// for the entire duration of the DB read, blocking UI threads that call GetSessionArray()
	// for the full query latency. Now we only hold it for the swap.
	ibApplicationDataSessionArray snapshot;

	ibResultSetGuard result(m_session_db,
		m_session_db->RunQueryWithResults(
			wxT("SELECT userName, application, started, computer, session FROM %s ORDER BY started, session;"),
			session_table)
	);

	if (result) {
		while (result->Next()) {
			snapshot.AppendSession(
				static_cast<ibRunMode>(result->GetResultInt("application")),
				result->GetResultDate("started"),
				result->GetResultString("userName"),
				result->GetResultString("computer"),
				result->GetResultString("session")
			);
		}
	}

	{
		wxCriticalSectionLocker enter(ms_sessionLocker);
		m_sessionArray = std::move(snapshot);
	}
}

void ibApplicationData::ibApplicationDataSessionUpdater::ClearLostSessionUpdater()
{
	// DELETE stale rows first (prepared statement, no lock held — pure DB I/O).
	m_session_db->BeginTransaction();

	wxDateTime cutoff = wxDateTime::Now();
	(void)cutoff.Subtract(wxTimeSpan(0, 0, timeInterval));

	ibStatementGuard stmt(m_session_db,
		m_session_db->PrepareStatement(
			wxT("DELETE FROM %s WHERE lastActive < ?;"),
			session_table)
	);
	if (stmt) {
		stmt->SetParamDate(1, cutoff);
		stmt->RunQuery();
	}

	m_session_db->Commit();

	// Rebuild the snapshot and swap it in — same pattern as Job_UpdateActiveSession,
	// so UI only blocks for the pointer swap, not for the DB read.
	Job_UpdateActiveSession();
}

///////////////////////////////////////////////////////////////////////////////

ibApplicationData::ibApplicationDataSessionUpdater::ibApplicationDataSessionUpdater(ibApplicationData* application, const ibGuid& session) :
	wxThread(wxTHREAD_JOINABLE),
	m_sessionCreated(false), m_sessionStarted(false),
	m_sessionUpdaterLoop(false),
	m_session_db(application->m_db != nullptr ? application->m_db->Clone() : nullptr),
	m_session(session)
{
	wxThread::SetPriority(wxPRIORITY_MIN);
}

bool ibApplicationData::ibApplicationDataSessionUpdater::InitSessionUpdater()
{
	if (m_session_db == nullptr)
		return false;

	if (wxThread::Run() != wxThreadError::wxTHREAD_NO_ERROR)
		return false;

	m_sessionUpdaterLoop = true;

	// 20-second bound — must comfortably cover Entry()'s designer grace window
	// (timeInterval + 2s) plus DB work. Previously this was an unbounded wait
	// that would deadlock the main thread if Entry() died before signalling.
	const int kMaxSpins = 400;   // 400 * 50ms = 20s
	for (int i = 0; i < kMaxSpins && m_sessionUpdaterLoop.load(std::memory_order_acquire); ++i) {
		if (m_sessionCreated.load(std::memory_order_acquire))
			break;
		wxMilliSleep(50);
	}

	m_sessionUpdaterLoop = false;
	return m_sessionCreated.load(std::memory_order_acquire);
}

void ibApplicationData::ibApplicationDataSessionUpdater::StartSessionUpdater()
{
	m_sessionStarted = true;
}

const ibApplicationDataSessionArray ibApplicationData::ibApplicationDataSessionUpdater::GetSessionArray() const
{
	wxCriticalSectionLocker enter(ms_sessionLocker);
	return m_sessionArray;
}

bool ibApplicationData::ibApplicationDataSessionUpdater::VerifySessionUpdater() const
{
	if (appData->GetAppMode() == eDESIGNER_MODE) {
		for (unsigned int idx = 0; idx < m_sessionArray.GetSessionCount(); idx++) {
			if (eDESIGNER_MODE == m_sessionArray.GetSessionApplication(idx)) {
				try {
					ibBackendCoreException::Error(
						_("Another designer process is already running:\n%s, %s, %s"),
						m_sessionArray.GetStartedDate(idx),
						m_sessionArray.GetComputerName(idx),
						m_sessionArray.GetUserName(idx)
					);
				}
				catch (const ibBackendException* err) {
					ibBackendExceptionPtr guard(err);
				}
				return false;
			}
		}
	}

	return true;
}

ibApplicationData::ibApplicationDataSessionUpdater::~ibApplicationDataSessionUpdater()
{
	if (m_session_db != nullptr) m_session_db->Close();
}

///////////////////////////////////////////////////////////////////////////////

wxThread::ExitCode ibApplicationData::ibApplicationDataSessionUpdater::Entry()
{
	// clear lost sessions
	ClearLostSessionUpdater();

	Job_UpdateActiveSession();

	// Grace window for Designer: if a previous designer was force-killed via Task Manager
	// less than `timeInterval` seconds ago, its row is still "fresh" (lastActive was
	// updated within the cutoff) so ClearLostSessionUpdater skipped it, and we'd falsely
	// report "another designer is already running". Wait a little longer than timeInterval
	// so the zombie row ages past the cutoff, then retry the cleanup once.
	if (appData->GetAppMode() == eDESIGNER_MODE) {
		bool designerConflict = false;
		{
			wxCriticalSectionLocker enter(ms_sessionLocker);
			for (unsigned int idx = 0; idx < m_sessionArray.GetSessionCount(); ++idx) {
				if (eDESIGNER_MODE == m_sessionArray.GetSessionApplication(idx)) {
					designerConflict = true;
					break;
				}
			}
		}
		if (designerConflict) {
			// Sleep in small slices so shutdown requests are honored promptly.
			const int graceMs = (timeInterval + 2) * 1000;
			for (int slept = 0; slept < graceMs && !TestDestroy(); slept += 250)
				wxMilliSleep(250);
			ClearLostSessionUpdater();
			Job_UpdateActiveSession();
		}
	}

	//verify session updater
	if (!VerifySessionUpdater())
		return (wxThread::ExitCode)1;

	//start empty session
	{
		ibStatementGuard insertStmt(m_session_db,
			m_session_db->PrepareStatement(
				wxT("INSERT INTO %s (session, userName, application, started, lastActive, computer) VALUES (?,?,?,?,?,?);"),
				session_table)
		);

		if (!insertStmt)
			return (wxThread::ExitCode)1;

		insertStmt->SetParamString(1, m_session.str());
		insertStmt->SetParamString(2, wxEmptyString); //empty user!
		insertStmt->SetParamInt(3, appData->GetAppMode());
		insertStmt->SetParamDate(4, appData->GetStartedDate());
		insertStmt->SetParamDate(5, wxDateTime::Now());
		insertStmt->SetParamString(6, appData->GetComputerName());

		insertStmt->RunQuery();
	}

	m_sessionArray.AppendSession(
		appData->GetAppMode(), appData->GetStartedDate(), appData->GetUserName(), appData->GetComputerName(), m_session.str()
	);

	m_sessionCreated = true;

	// Tick cadence controls how fast this instance sees other clients joining/leaving.
	// 1000 ms gives near-real-time visibility while keeping DB load modest:
	// each tick = own UPDATE + full SELECT, so 10 clients @ 1Hz ≈ 10 UPDATE + 10 SELECT /sec.
	// Cutoff for lost-session cleanup (timeInterval=5s) still leaves 4s safety margin.
	// Job_ClearLostSession is expensive for every instance to run — stagger it to every
	// third tick (~3 s) so we keep that heartbeat behaviour.
	const unsigned int kTickMs = 1000;
	const unsigned int kClearEveryNthTick = 3;
	unsigned int tickCounter = 0;

	bool lastSessionStarted = m_sessionStarted;
	while (!TestDestroy()) {

		m_currentDateTime = wxDateTime::UNow();

		if ((tickCounter % kClearEveryNthTick) == 0)
			Job_ClearLostSession();
		Job_CalcActiveSession();
		Job_UpdateActiveSession();
		++tickCounter;

		const wxTimeSpan& job_allTotal_done = wxDateTime::UNow() - m_currentDateTime;

		for (unsigned int milliseconds =
			(unsigned int)job_allTotal_done.GetMilliseconds().GetValue();
			milliseconds < kTickMs;
			milliseconds += 250)
		{
			if (m_sessionStarted != lastSessionStarted)
				break;

			if (TestDestroy())
				break;

			wxMilliSleep(250);
		}

		lastSessionStarted = m_sessionStarted;
	};

	// Final DELETE with a prepared statement — last injection surface in this thread.
	int result = DATABASE_LAYER_QUERY_RESULT_ERROR;
	ibStatementGuard finalStmt(m_session_db,
		m_session_db->PrepareStatement(
			wxT("DELETE FROM %s WHERE session = ?;"),
			session_table)
	);
	if (finalStmt) {
		finalStmt->SetParamString(1, m_session.str());
		result = finalStmt->RunQuery();
	}

	m_sessionArray.ClearSession();
	// Order matters: drop "started" first so observers see "no longer running" before
	// "never created", never the impossible (started && !created) combo.
	m_sessionStarted = false;
	m_sessionCreated = false;

	if (result != DATABASE_LAYER_QUERY_RESULT_ERROR)
		return (wxThread::ExitCode)1;

	return (wxThread::ExitCode)0;
}

///////////////////////////////////////////////////////////////////////////////
//								ibApplicationData
///////////////////////////////////////////////////////////////////////////////

bool ibApplicationData::TableAlreadyCreated()
{
	return db_query->TableExists(user_table) &&
		db_query->TableExists(session_table);
}

///////////////////////////////////////////////////////////////////////////////

void ibApplicationData::CreateTableUser()
{
	if (!db_query->TableExists(user_table)) {
		if (db_query->GetDatabaseLayerType() == DATABASELAYER_POSTGRESQL) {
			db_query->RunQuery("create table %s ("
				"guid              VARCHAR(36)   NOT NULL PRIMARY KEY,"
				"name              VARCHAR(64)  NOT NULL,"
				"fullName          VARCHAR(128)  NOT NULL,"
				"changed		   TIMESTAMP  NOT NULL,"
				"dataSize          INTEGER       NOT NULL,"
				"binaryData        BYTEA      NOT NULL);", user_table);
		}
		else {
			db_query->RunQuery("create table %s ("
				"guid              VARCHAR(36)   NOT NULL PRIMARY KEY,"
				"name              VARCHAR(64)  NOT NULL,"
				"fullName          VARCHAR(128)  NOT NULL,"
				"changed		   TIMESTAMP  NOT NULL,"
				"dataSize          INTEGER       NOT NULL,"
				"binaryData        BLOB      NOT NULL);", user_table);
		}
		db_query->RunQuery("create index if not exists user_index on %s (guid, name);", user_table);
	}
}

void ibApplicationData::CreateTableSession()
{
	if (!db_query->TableExists(session_table)) {

		db_query->RunQuery(wxT("create table %s ("
			"session              VARCHAR(36) NOT NULL PRIMARY KEY,"
			"userName             VARCHAR(64) NOT NULL,"
			"application	   INTEGER  NOT NULL,"
			"started		   TIMESTAMP  NOT NULL,"
			"lastActive		   TIMESTAMP  NOT NULL,"
			"computer          VARCHAR(128) NOT NULL);"),
			session_table
		);

		if (db_query->GetDatabaseLayerType() != DATABASELAYER_FIREBIRD) {

			db_query->RunQuery(
				wxT("create index if not exists session_index_1 on %s (session, userName);"),
				session_table
			);

			db_query->RunQuery(
				wxT("create index if not exists session_index_2 on %s (session);"),
				session_table
			);

			db_query->RunQuery(
				wxT("create index if not exists session_index_3 on %s (lastActive);"),
				session_table
			);
		}
		else
		{
			db_query->RunQuery(
				wxT("create index session_index_1 on %s (session, userName);"),
				session_table
			);

			db_query->RunQuery(
				wxT("create index session_index_2 on %s (session);"),
				session_table
			);

			db_query->RunQuery(
				wxT("create index session_index_3 on %s (lastActive);"),
				session_table
			);
		}
	}
}

void ibApplicationData::CreateTableEvent()
{
	if (!db_query->TableExists(event_table)) {
	}
}

bool ibApplicationData::ClearTableUser()
{
	if (!db_query->TableExists(user_table))
		return false;

	db_query->RunQuery(wxT("DELETE FROM %s;"), user_table);
	return true;
}

///////////////////////////////////////////////////////////////////////////////

enum
{
	eBlockPswd = 0x0234530,
	eBlockRole = 0x0234540,
	eBlockLang = 0x0234550,
};

#include "fileSystem/fs.h"

ibApplicationDataUserInfo ibApplicationData::ReadUserData(const ibGuid& userGuid) const
{
	ibApplicationDataUserInfo userInfo;
	if (!userGuid.isValid())
		return userInfo;

	ibResultSetGuard result(db_query,
		db_query->RunQueryWithResults(wxT("SELECT * FROM %s WHERE guid = '%s';"), user_table, userGuid.str()));

	if (result && result->Next()) {

		userInfo.m_strUserGuid = result->GetResultString(wxT("guid"));
		userInfo.m_strUserName = result->GetResultString(wxT("name"));
		userInfo.m_strUserFullName = result->GetResultString(wxT("fullName"));

		wxMemoryBuffer buffer;
		result->GetResultBlob(wxT("binaryData"), buffer);
		ibReaderMemory userReader(buffer);

		wxMemoryBuffer bufferPassword;
		if (userReader.r_chunk(eBlockPswd, bufferPassword)) {
			ReadUserData_Password(bufferPassword, userInfo);
		}

		wxMemoryBuffer bufferRole;
		if (userReader.r_chunk(eBlockRole, bufferRole)) {
			ReadUserData_Role(bufferRole, userInfo);
		}

		wxMemoryBuffer bufferLang;
		if (userReader.r_chunk(eBlockLang, bufferLang)) {
			ReadUserData_Language(bufferLang, userInfo);
		}
	}

	return userInfo;
}

ibApplicationDataUserInfo ibApplicationData::ReadUserData(const wxString& strUserName) const
{
	ibApplicationDataUserInfo userInfo;
	if (strUserName.IsEmpty())
		return userInfo;

	ibResultSetGuard result(db_query,
		db_query->RunQueryWithResults(wxT("SELECT * FROM %s WHERE name = '%s';"), user_table, strUserName));

	if (result && result->Next()) {

		userInfo.m_strUserGuid = result->GetResultString(wxT("guid"));
		userInfo.m_strUserName = result->GetResultString(wxT("name"));
		userInfo.m_strUserFullName = result->GetResultString(wxT("fullName"));

		wxMemoryBuffer buffer;
		result->GetResultBlob(wxT("binaryData"), buffer);
		ibReaderMemory userReader(buffer);

		wxMemoryBuffer bufferPassword;
		if (userReader.r_chunk(eBlockPswd, bufferPassword)) {
			ReadUserData_Password(bufferPassword, userInfo);
		}

		wxMemoryBuffer bufferRole;
		if (userReader.r_chunk(eBlockRole, bufferRole)) {
			ReadUserData_Role(bufferRole, userInfo);
		}

		wxMemoryBuffer bufferLang;
		if (userReader.r_chunk(eBlockLang, bufferLang)) {
			ReadUserData_Language(bufferLang, userInfo);
		}
	}

	return userInfo;
}

bool ibApplicationData::SaveUserData(const ibApplicationDataUserInfo& userInfo) const
{
	ibStatementGuard stmt(db_query,
		db_query->GetDatabaseLayerType() != DATABASELAYER_FIREBIRD
			? db_query->PrepareStatement(
				wxT("INSERT INTO %s (guid, name, fullName, changed, dataSize, binaryData) VALUES(?, ?, ?, ?, ?, ?) ON CONFLICT (guid) DO UPDATE SET guid = excluded.guid, name = excluded.name, fullName = excluded.fullName, changed = excluded.changed, dataSize = excluded.dataSize, binaryData = excluded.binaryData; "), user_table)
			: db_query->PrepareStatement(
				wxT("UPDATE OR INSERT INTO %s (guid, name, fullName, changed, dataSize, binaryData) VALUES(?, ?, ?, ?, ?, ?) MATCHING (guid);"), user_table)
	);

	if (!stmt)
		return false;

	stmt->SetParamString(1, userInfo.m_strUserGuid);
	stmt->SetParamString(2, userInfo.m_strUserName);
	stmt->SetParamString(3, userInfo.m_strUserFullName);
	stmt->SetParamDate(4, wxDateTime::Now());

	ibWriterMemory writer;

	writer.w_chunk(eBlockPswd, SaveUserData_Password(userInfo));
	writer.w_chunk(eBlockRole, SaveUserData_Role(userInfo));
	writer.w_chunk(eBlockLang, SaveUserData_Language(userInfo));

	stmt->SetParamNumber(5, writer.size());
	stmt->SetParamBlob(6, writer.pointer(), writer.size());

	const int result = stmt->RunQuery();
	return result != DATABASE_LAYER_QUERY_RESULT_ERROR;
}

///////////////////////////////////////////////////////////////////////////////

bool ibApplicationData::LoadUserInfoFromBuffer(wxMemoryBuffer& buffer)
{
	ibReaderMemory reader = buffer;

	ibReaderMemory* prevReaderMemory = nullptr; u64 meta_id = 0;

	while (!reader.eof()) {

		ibReaderMemory* readerMemory = reader.open_chunk_iterator(meta_id, &*prevReaderMemory);
		if (!readerMemory)
			break;

		ibApplicationDataUserInfo userInfo;

		userInfo.m_strUserGuid = readerMemory->r_stringZ();
		userInfo.m_strUserName = readerMemory->r_stringZ();
		userInfo.m_strUserFullName = readerMemory->r_stringZ();

		wxMemoryBuffer bufferPassword;
		if (readerMemory->r_chunk(eBlockPswd, bufferPassword)) {
			ReadUserData_Password(bufferPassword, userInfo);
		}

		wxMemoryBuffer bufferRole;
		if (readerMemory->r_chunk(eBlockRole, bufferRole)) {
			ReadUserData_Role(bufferRole, userInfo);
		}

		wxMemoryBuffer bufferLang;
		if (readerMemory->r_chunk(eBlockLang, bufferLang)) {
			ReadUserData_Language(bufferLang, userInfo);
		}

		SaveUserData(userInfo);

		prevReaderMemory = readerMemory;
	};

	return true;
}

bool ibApplicationData::SaveUserInfoToBuffer(wxMemoryBuffer& buffer) const
{
	ibResultSetGuard result(db_query,
		db_query->RunQueryWithResults(wxT("SELECT guid FROM %s;"), user_table));

	if (!result)
		return false;

	ibWriterMemory writer; unsigned int idx = 0;

	while (result->Next()) {

		const ibApplicationDataUserInfo& userInfo = ReadUserData(ibGuid(result->GetResultString(wxT("guid"))));

		ibWriterMemory userWriter;

		userWriter.w_stringZ(userInfo.m_strUserGuid);
		userWriter.w_stringZ(userInfo.m_strUserName);
		userWriter.w_stringZ(userInfo.m_strUserFullName);

		userWriter.w_chunk(eBlockPswd, SaveUserData_Password(userInfo));
		userWriter.w_chunk(eBlockRole, SaveUserData_Role(userInfo));
		userWriter.w_chunk(eBlockLang, SaveUserData_Language(userInfo));

		writer.w_chunk(idx++, userWriter.buffer());
	}

	buffer = writer.buffer();
	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool ibApplicationData::HasAllowedUser() const
{
	ibResultSetGuard result(db_query,
		db_query->RunQueryWithResults(wxT("SELECT name FROM %s;"), user_table));

	if (!result) return false;
	return result->Next();
}

std::vector<ibApplicationDataShortUserInfo> ibApplicationData::GetAllowedUser() const
{
	ibResultSetGuard result(db_query,
		db_query->RunQueryWithResults(wxT("SELECT guid, name, fullName FROM %s;"), user_table));

	std::vector<ibApplicationDataShortUserInfo> userInfo;

	if (!result)
		return userInfo;

	while (result->Next()) {

		ibApplicationDataShortUserInfo entry;
		entry.m_strUserGuid = result->GetResultString(wxT("guid"));
		entry.m_strUserName = result->GetResultString(wxT("name"));
		entry.m_strUserFullName = result->GetResultString(wxT("fullName"));
		userInfo.push_back(entry);
	}

	return userInfo;
}

bool ibApplicationData::StartSession(const wxString& strUserName, const wxString& strUserPassword)
{
	if (!CloseSession())
		return false;

	m_sessionUpdater = std::shared_ptr<ibApplicationDataSessionUpdater>(
		new ibApplicationDataSessionUpdater(this, m_sessionGuid)
	);

	bool succes = true;

	if (!m_sessionUpdater->InitSessionUpdater())
		return false;

	if (!AuthenticationAndSetUser(strUserName, strUserPassword)) {

		succes = false;

		if (backend_mainFrame != nullptr &&
			backend_mainFrame->AuthenticationUser(strUserName, strUserPassword)) {
			succes = true;
		}
	}

	if (succes)
		m_sessionUpdater->StartSessionUpdater();
	else if (!succes && !CloseSession())
		return false;

	m_connected_to_db = succes;
	return succes;
}

bool ibApplicationData::CloseSession()
{
	if (m_sessionUpdater != nullptr) {

		if (m_sessionUpdater->IsRunning() &&
			m_sessionUpdater->Delete() != wxTHREAD_NO_ERROR)
			return false;

		m_sessionUpdater = nullptr;
	}

	// Wipe the in-memory raw password as soon as the session ends.
	m_sessionRawPassword.clear();

	m_connected_to_db = false;
	return true;
}