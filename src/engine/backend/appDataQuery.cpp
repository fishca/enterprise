#include "appData.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/databaseLayer/databaseErrorCodes.h"
#include "backend/databaseLayer/connectionPool.h"

#include "backend/backend_exception.h"

#include "backend/session/sessionRegistry.h"


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
			"computer          VARCHAR(128) NOT NULL,"
			// --- session-registry extensions (2026-04-20) ---
			// pid             = owner process id (for admin / kick / debugger attach)
			// address         = "host:port" for web processes; "" for desktop
			// currentActivity = last scripted/engine label ("idle", "running:OnStart", "reload", ...)
			// exclusive       = 1 when this session holds process-wide monopoly mode; 0 otherwise.
			//                   Cluster-aware exclusive gate reads this column from peer rows
			//                   to block new Connects when another process is exclusive.
			"pid               INTEGER,"
			"address           VARCHAR(256),"
			"currentActivity   VARCHAR(128),"
			"exclusive         INTEGER);"),
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

// Additive column migration for sys_session. Existing databases created
// before the session-registry refactor (pid / address / currentActivity
// added 2026-04-20) are transparently upgraded on startup — registry
// writes / reads assume these columns exist, so an old schema would
// trip INSERT and snapshot SELECT otherwise. Columns are nullable, so
// legacy rows stay valid until the next heartbeat rewrite.
void ibApplicationData::MigrateTableSession()
{
	if (!db_query->TableExists(session_table))
		return;

	wxArrayString cols = db_query->GetColumns(session_table);
	auto has = [&](const wxString& col) {
		for (std::size_t i = 0; i < cols.GetCount(); ++i)
			if (cols[i].CmpNoCase(col) == 0) return true;
		return false;
	};

	if (!has(wxT("pid"))) {
		try { db_query->RunQuery(wxT("ALTER TABLE %s ADD pid INTEGER"), session_table); }
		catch (...) { /* best-effort — driver may not support this DDL */ }
	}
	if (!has(wxT("address"))) {
		try { db_query->RunQuery(wxT("ALTER TABLE %s ADD address VARCHAR(256)"), session_table); }
		catch (...) {}
	}
	if (!has(wxT("currentActivity"))) {
		try { db_query->RunQuery(wxT("ALTER TABLE %s ADD currentActivity VARCHAR(128)"), session_table); }
		catch (...) {}
	}
	if (!has(wxT("kind"))) {
		// ibSessionKind — session-level role (WebServer=5, WebClient=100,
		// desktop kinds share numeric values with ibRunMode). Distinct
		// from `application` which stores process-level ibRunMode.
		try { db_query->RunQuery(wxT("ALTER TABLE %s ADD kind INTEGER"), session_table); }
		catch (...) {}
	}
	if (!has(wxT("signal"))) {
		// Admin → registry control channel. A non-empty value is picked
		// up by the session's owning process on its next JobCheckSignal
		// tick; the handler acts and clears the signal. "kick" is the
		// first supported value — more (reload, refresh) may follow.
		try { db_query->RunQuery(wxT("ALTER TABLE %s ADD signal VARCHAR(32)"), session_table); }
		catch (...) {}
	}
	if (!has(wxT("exclusive"))) {
		// Process-wide monopoly mode marker. 1 = this session holds
		// exclusive; cluster-aware gate in ProcessAdd / ProcessSetExclusive
		// reads peer rows to detect another process holding it.
		try { db_query->RunQuery(wxT("ALTER TABLE %s ADD exclusive INTEGER"), session_table); }
		catch (...) {}
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

#include "fileSystem/fs.h"

// User-record DB I/O moved onto ibUserInfo as static factories; see
// backend/userInfo.{h,cpp}. ibApplicationData no longer mediates the
// sys_user round-trip — call sites use ibUserInfo::Read / Save / Serialize
// / Deserialize directly.

///////////////////////////////////////////////////////////////////////////////

bool ibApplicationData::LoadUserInfoFromBuffer(wxMemoryBuffer& buffer)
{
	ibReaderMemory reader = buffer;

	ibReaderMemory* prevReaderMemory = nullptr; u64 meta_id = 0;

	while (!reader.eof()) {

		ibReaderMemory* readerMemory = reader.open_chunk_iterator(meta_id, &*prevReaderMemory);
		if (!readerMemory)
			break;

		ibUserInfo::Save(ibUserInfo::Deserialize(*readerMemory));

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
		ibWriterMemory userWriter;
		ibUserInfo::Read(ibGuid(result->GetResultString(wxT("guid"))))
			.Serialize(userWriter);
		writer.w_chunk(idx++, userWriter.buffer());
	}

	buffer = writer.buffer();
	return true;
}

///////////////////////////////////////////////////////////////////////////////

// HasAllowedUser / GetAllowedUser moved onto ibUserInfo as
// ibUserInfo::HasAny / ibUserInfo::ListAll — sys_user table-wide
// queries belong with the type, not on the singleton.

// -----------------------------------------------------------------------
// Phased session lifecycle — apps compose CreateSession (or the typed
// CreateSession<T>) with session->Open() so the registry ticket
// stays visible during login-retry loops. There is no one-shot
// "Connect/StartSession" anymore; failed Open keeps the anonymous row
// in sys_session until the caller drops the ticket explicitly.
// -----------------------------------------------------------------------

ibSession* ibApplicationData::CreateSession()
{
	// Default-factory passthrough — registry builds a plain ibSession.
	// Used by codeRunner / daemon / headless callers and by the wes
	// process's own system session bring-up. GUI apps go through the
	// typed CreateSession<T>() template overload (defined in
	// sessionRegistry.h after the registry class).
	return m_sessionRegistry->CreateSessionWithFactory(m_runMode, m_strComputer, {});
}

