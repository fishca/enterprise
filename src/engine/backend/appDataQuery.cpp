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

	ibStatementGuard stmt(db_query,
		db_query->PrepareStatement(wxT("SELECT * FROM %s WHERE guid = ?;"), user_table));
	if (stmt)
		stmt->SetParamString(1, userGuid.str());

	ibResultSetGuard result(db_query,
		stmt ? stmt->RunQueryWithResults() : nullptr);

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

	ibStatementGuard stmt(db_query,
		db_query->PrepareStatement(wxT("SELECT * FROM %s WHERE name = ?;"), user_table));
	if (stmt)
		stmt->SetParamString(1, strUserName);

	ibResultSetGuard result(db_query,
		stmt ? stmt->RunQueryWithResults() : nullptr);

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

