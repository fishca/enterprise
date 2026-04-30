#include "firebirdDatabaseLayer.h"

#include "backend/databaseLayer/databaseErrorCodes.h"
#include "backend/databaseLayer/databaseLayerException.h"

#include "engine/ibase.h"

#if _USE_DYNAMIC_DATABASE_LAYER_LINKING == 1
#include "firebirdInterface.h"
#endif

#include "firebirdPreparedStatement.h"
#include "firebirdResultSet.h"

#include <wx/file.h>
#include <wx/stdpaths.h>
#include <wx/tokenzr.h>
#include <wx/regex.h>

// ctor()
ibDatabaseLayerFirebird::ibDatabaseLayerFirebird()
	: ibDatabaseLayer()
{
	m_pDatabase = 0;
	m_pTransaction = 0;

	m_pStatus = new ISC_STATUS_ARRAY();
#if _USE_DYNAMIC_DATABASE_LAYER_LINKING == 1
	m_pInterface = new ibInterfaceFirebird();
	
	if (!m_pInterface->Init())
	{
		SetErrorCode(DATABASE_LAYER_ERROR_LOADING_LIBRARY);
		SetErrorMessage(wxT("Error loading Firebird library"));
		ThrowDatabaseException();
		return;
	}

#endif

	m_strServer = wxT("");  // assume embedded database in this case
	m_strUser = wxT("sysdba");
	m_strPassword = wxT("masterkey");
	m_strDatabase = wxT("");
	m_strRole = wxEmptyString;
}

ibDatabaseLayerFirebird::ibDatabaseLayerFirebird(const wxString& strDatabase)
	: ibDatabaseLayer()
{
	m_pDatabase = 0;
	m_pTransaction = 0;

	m_pStatus = new ISC_STATUS_ARRAY();
#if _USE_DYNAMIC_DATABASE_LAYER_LINKING == 1
	m_pInterface = new ibInterfaceFirebird();
	
	if (!m_pInterface->Init())
	{
		SetErrorCode(DATABASE_LAYER_ERROR_LOADING_LIBRARY);
		SetErrorMessage(wxT("Error loading Firebird library"));
		ThrowDatabaseException();
		return;
	}

#endif

	m_strServer = wxT("");  // assume embedded database in this case
	m_strUser = wxT("sysdba");
	m_strPassword = wxT("masterkey");
	m_strRole = wxEmptyString;

	Open(strDatabase);
}

ibDatabaseLayerFirebird::ibDatabaseLayerFirebird(const wxString& strDatabase, const wxString& strUser, const wxString& strPassword)
	: ibDatabaseLayer()
{
	m_pDatabase = 0;
	m_pTransaction = 0;

	m_pStatus = new ISC_STATUS_ARRAY();
#if _USE_DYNAMIC_DATABASE_LAYER_LINKING == 1
	m_pInterface = new ibInterfaceFirebird();
	if (!m_pInterface->Init())
	{
		SetErrorCode(DATABASE_LAYER_ERROR_LOADING_LIBRARY);
		SetErrorMessage(wxT("Error loading Firebird library"));
		ThrowDatabaseException();
		return;
	}
#endif

	m_strServer = wxT("");  // assume embedded database in this case
	m_strUser = strUser;
	m_strPassword = strPassword;
	m_strRole = wxEmptyString;

	Open(strDatabase);
}

ibDatabaseLayerFirebird::ibDatabaseLayerFirebird(const wxString& strServer, const wxString& strDatabase, const wxString& strUser, const wxString& strPassword)
	: ibDatabaseLayer()
{
	m_pDatabase = 0;
	m_pTransaction = 0;

	m_pStatus = new ISC_STATUS_ARRAY();
#if _USE_DYNAMIC_DATABASE_LAYER_LINKING == 1
	m_pInterface = new ibInterfaceFirebird();
	if (!m_pInterface->Init())
	{
		SetErrorCode(DATABASE_LAYER_ERROR_LOADING_LIBRARY);
		SetErrorMessage(wxT("Error loading Firebird library"));
		ThrowDatabaseException();
		return;
	}
#endif

	m_strServer = strServer;
	m_strUser = strUser;
	m_strPassword = strPassword;
	m_strRole = wxEmptyString;

	Open(strDatabase);
}

ibDatabaseLayerFirebird::ibDatabaseLayerFirebird(const wxString& strServer, const wxString& strDatabase, const wxString& strUser, const wxString& strPassword, const wxString& strRole)
	: ibDatabaseLayer()
{
	m_pDatabase = 0;
	m_pTransaction = 0;

	m_pStatus = new ISC_STATUS_ARRAY();
#if _USE_DYNAMIC_DATABASE_LAYER_LINKING == 1
	m_pInterface = new ibInterfaceFirebird();
	if (!m_pInterface->Init())
	{
		SetErrorCode(DATABASE_LAYER_ERROR_LOADING_LIBRARY);
		SetErrorMessage(wxT("Error loading Firebird library"));
		ThrowDatabaseException();
		return;
	}
#endif

	m_strServer = strServer;
	m_strUser = strUser;
	m_strPassword = strPassword;
	m_strRole = strRole;

	Open(strDatabase);
}

ibDatabaseLayerFirebird::ibDatabaseLayerFirebird(const ibDatabaseLayerFirebird& src) 
{
	m_pDatabase = 0;
	m_pTransaction = 0;

	m_pStatus = new ISC_STATUS_ARRAY();
#if _USE_DYNAMIC_DATABASE_LAYER_LINKING == 1
	m_pInterface = new ibInterfaceFirebird();
	if (!m_pInterface->Init())
	{
		SetErrorCode(DATABASE_LAYER_ERROR_LOADING_LIBRARY);
		SetErrorMessage(wxT("Error loading Firebird library"));
		ThrowDatabaseException();
		return;
	}
#endif

	m_strServer = src.m_strServer;
	m_strUser = src.m_strUser;
	m_strPassword = src.m_strPassword;
	m_strRole = src.m_strRole;

	Open(src.m_strDatabase);
}

// dtor()
ibDatabaseLayerFirebird::~ibDatabaseLayerFirebird()
{
	Close();
	ISC_STATUS_ARRAY* pStatus = (ISC_STATUS_ARRAY*)m_pStatus;
	wxDELETEA(pStatus);
	m_pStatus = NULL;
	wxDELETE(m_pInterface);
	m_pInterface = NULL;
}

// open database
bool ibDatabaseLayerFirebird::Open(const wxString& strDatabase)
{
	m_strDatabase = strDatabase;
	return Open();
}

bool ibDatabaseLayerFirebird::Open(const wxString& strDatabase, const wxString& strUser, const wxString& strPassword)
{
	m_strUser = strUser;
	m_strPassword = strPassword;

	return Open(strDatabase);
}

bool ibDatabaseLayerFirebird::Open(const wxString& strServer, const wxString& strDatabase, const wxString& strUser, const wxString& strPassword)
{
	m_strServer = strServer;
	m_strUser = strUser;
	m_strPassword = strPassword;

	return Open(strDatabase);
}

bool ibDatabaseLayerFirebird::Open()
{
	ResetErrorCodes();

	if (m_pInterface == NULL)
		return false;

	//wxCSConv conv(wxT("UTF-8"));
	//SetEncoding(&conv);

	// Combine the server and databsae path strings to pass into the isc_attach_databse function
	wxString strDatabaseUrl;

	if (m_strServer.IsEmpty()) {
		strDatabaseUrl = m_strDatabase; // Embedded database, just supply the file name
	}
	else {
		strDatabaseUrl = m_strServer + wxT(":") + m_strDatabase;
	}

	wxCharBuffer urlBuffer = ConvertToUnicodeStream(strDatabaseUrl);
	unsigned int urlLength = GetEncodedStreamLength(strDatabaseUrl);

	std::string dpbBuffer;
	{
		dpbBuffer.push_back(isc_dpb_version1);
		dpbBuffer.push_back(isc_dpb_sql_dialect);
		dpbBuffer.push_back(1); // 1 byte long
		dpbBuffer.push_back(SQL_DIALECT_CURRENT);

		// page_size DPB: only honoured by isc_create_database (ignored on
		// attach). Encoded as big-endian 2 bytes per legacy DPB. FB 5
		// supports up to 32768; widen via uint32 before shifting so
		// 32768 doesn't sign-overflow.
		const uint32_t pageSize = (uint32_t)m_pageSize;
		dpbBuffer.push_back(isc_dpb_page_size);
		dpbBuffer.push_back(2);
		dpbBuffer.push_back((char)((pageSize >> 8) & 0xFF));
		dpbBuffer.push_back((char)(pageSize & 0xFF));

		// UTF8 character set:
		//   isc_dpb_set_db_charset — only honoured on CREATE DATABASE; sets
		//     the database default charset stored in the header page.
		//   isc_dpb_lc_ctype       — sets the *connection* charset; honoured
		//     on every attach. Without it FB falls back to NONE and returns
		//     raw bytes, which corrupts non-ASCII (Cyrillic, etc.) on read.
		// Both are pushed so a freshly-created DB has UTF8 as default and
		// every attach (create or existing) talks UTF8 to the engine.
		const char sCharset[] = "UTF8";
		dpbBuffer.push_back(isc_dpb_set_db_charset);
		dpbBuffer.push_back(sizeof(sCharset) - 1);
		dpbBuffer.append(sCharset);

		dpbBuffer.push_back(isc_dpb_lc_ctype);
		dpbBuffer.push_back(sizeof(sCharset) - 1);
		dpbBuffer.append(sCharset);

		// force_write = 0 — async writes. FB defaults a freshly CREATE'd
		// embedded DB to forced/synchronous writes (FlushFileBuffers per
		// dirty page); on Windows NTFS that's ~5–10× slower than async.
		// Async still uses careful-writes ordering, so a process crash
		// loses at most ~1s of pending writes — acceptable for OES
		// embedded dev/test. Existing DBs ignore this DPB on attach
		// (FB stores the flag in the header); flip with `gfix -w async`
		// or `ALTER DATABASE SET WRITE = ASYNC` when you want it on
		// pre-existing files.
		dpbBuffer.push_back(isc_dpb_force_write);
		dpbBuffer.push_back(1);
		dpbBuffer.push_back(0);

		// Session time zone = UTC. FB 4+ supports TIMESTAMP WITH TIME ZONE;
		// pinning the session to UTC means timestamps stored/returned over
		// this connection are always normalized to UTC regardless of the
		// client OS time zone. Plain TIMESTAMP columns are unaffected.
		// Avoids "the same row reads different times on different
		// machines" surprises.
		const char sTimeZone[] = "UTC";
		dpbBuffer.push_back((char)isc_dpb_session_time_zone);
		dpbBuffer.push_back(sizeof(sTimeZone) - 1);
		dpbBuffer.append(sTimeZone);

		dpbBuffer.push_back(isc_dpb_utf8_filename);
		dpbBuffer.push_back(urlLength);
		dpbBuffer.append(urlBuffer);

		if (m_strUser.length() > 0)
		{
			int nUsernameLength = m_strUser.length();
			dpbBuffer.push_back(isc_dpb_user_name);
			dpbBuffer.push_back(nUsernameLength);
			dpbBuffer.append(m_strUser);
		}

		if (m_strPassword.length() > 0)
		{
			int nPasswordLength = m_strPassword.length();
			dpbBuffer.push_back(isc_dpb_password);
			dpbBuffer.push_back(nPasswordLength);
			dpbBuffer.append(m_strPassword);
		}

		if (m_strRole.length() > 0)
		{
			int nRoleLength = m_strRole.length();
			dpbBuffer.push_back(isc_dpb_sql_role_name);
			dpbBuffer.push_back(nRoleLength);
			dpbBuffer.append(m_strRole);
		}
	}

	// If the layer was previously opened, detach the old handle before
	// creating a new one — otherwise the existing isc_db_handle leaks
	// and the second isc_attach_database silently allocates a fresh
	// handle that callers won't see.
	if (m_pDatabase) {
		try { Close(); } catch (...) { /* best-effort — fall through */ }
	}
	m_pDatabase = 0;

	isc_db_handle pDatabase = m_pDatabase;

	int nReturn = 0;

	if (m_strServer.IsEmpty())
	{
		if (!wxFile::Exists(strDatabaseUrl)){

			wxFileName fileDatabase(m_strDatabase);
			// Mkdir(..., wxPATH_MKDIR_FULL) is the recursive variant —
			// wxMkDir on a multi-level path silently fails, leaving FB
			// to error out with "I/O error during open" when the
			// containing directory doesn't exist yet.
			wxFileName::Mkdir(fileDatabase.GetPath(), 0777, wxPATH_MKDIR_FULL);

			nReturn = m_pInterface->GetIscCreateDatabase()(*(ISC_STATUS_ARRAY*)m_pStatus,
				(unsigned short)urlLength, (const char*)urlBuffer,
				&pDatabase,
				(unsigned short)dpbBuffer.length(), dpbBuffer.c_str(),
				(unsigned short)0);
		}
		else
		{
			nReturn = m_pInterface->GetIscAttachDatabase()(*(ISC_STATUS_ARRAY*)m_pStatus,
				(unsigned short)urlLength, (const char*)urlBuffer,
				&pDatabase,
				(unsigned short)dpbBuffer.length(), dpbBuffer.c_str());
		}
	}
	else
	{
		nReturn = m_pInterface->GetIscAttachDatabase()(*(ISC_STATUS_ARRAY*)m_pStatus, urlLength, (char*)(const char*)urlBuffer,
			&pDatabase,
			dpbBuffer.length(), dpbBuffer.c_str());
	}

	m_pDatabase = pDatabase;

	if (nReturn != 0)
	{
		InterpretErrorCodes();
		ThrowDatabaseException();

		return false;
	}

	return true;
}

// close database  
bool ibDatabaseLayerFirebird::Close()
{
	CloseResultSets();
	CloseStatements();

	if (m_pDatabase)
	{
		// Roll back any TX still open on this handle so isc_detach_database
		// doesn't reject the disconnect with "active transactions". Errors
		// are intentionally ignored here — we're tearing down the
		// connection and the rollback is best-effort.
		if (m_pTransaction)
		{
			isc_tr_handle pTransaction = m_pTransaction;
			m_pInterface->GetIscRollbackTransaction()(*(ISC_STATUS_ARRAY*)m_pStatus, &pTransaction);
			m_pTransaction = 0;
		}

		isc_db_handle pDatabase = m_pDatabase;
		int nReturn = m_pInterface->GetIscDetachDatabase()(*(ISC_STATUS_ARRAY*)m_pStatus, &pDatabase);
		m_pDatabase = 0;
		if (nReturn != 0)
		{
			InterpretErrorCodes();
			ThrowDatabaseException();
			return false;
		}
	}

	return true;
}

bool ibDatabaseLayerFirebird::IsOpen()
{
	return (m_pDatabase != 0);
}

// transaction support
void ibDatabaseLayerFirebird::DoBeginTransaction(const ibTxOptions& opts)
{
	ResetErrorCodes();

	if (!m_pDatabase)
		return;

	// TPB layouts (read-committed / no-rec-version, the OES default).
	// Wait vs nowait drives whether SELECT ... WITH LOCK contention
	// blocks or surfaces immediately as a lock-conflict exception —
	// TryProbeRowLock uses the latter.
	//
	//   ISOLATION_READ_UNCOMMITTED         = version3, write, wait,   read_committed, rec_version
	//   ISOLATION_READ_COMMITTED           = version3, write, wait,   read_committed, no_rec_version
	//   ISOLATION_REPEATABLE_READ          = version3, write, wait,   concurrency
	//   ISOLATION_SERIALIZABLE             = version3, write, wait,   consistency
	//   ISOLATION_READ_COMMITTED_READ_ONLY = version3, read,  wait,   read_committed, no_rec_version
	//
	// wait-mode TPB also pins a 30-second lock timeout: without it the
	// caller blocks indefinitely on contention (UI hangs, daemons stall).
	// 30 s is long enough that legitimate brief contention resolves and
	// short enough that a stuck peer surfaces as an exception. nowait
	// mode skips the timeout knob — the engine fails immediately anyway.
	// Encoding per FB legacy TPB: tag, length=4, little-endian int32 secs.
	static const std::string isc_tpb_waitMode = {
		isc_tpb_version3, isc_tpb_write, isc_tpb_wait, isc_tpb_read_committed, isc_tpb_no_rec_version,
		(char)isc_tpb_lock_timeout, 4, 30, 0, 0, 0
	};
	static const std::string isc_tpb_nowaitMode = {
		isc_tpb_version3, isc_tpb_write, isc_tpb_nowait, isc_tpb_read_committed, isc_tpb_no_rec_version
	};
	// Read-only mode: read + read_committed + read_consistency (FB 4+).
	// read_consistency makes every statement in this TX see a stable
	// snapshot of committed data without holding the snapshot for the
	// entire TX (vs isc_tpb_concurrency which does). No write-intent
	// locks are acquired, so SELECT-heavy paths don't conflict with
	// concurrent writers. Lock timeout is irrelevant for a read-only TX
	// but kept for symmetry — engine ignores it when no locks taken.
	static const std::string isc_tpb_readOnlyMode = {
		isc_tpb_version3, isc_tpb_read, isc_tpb_wait, isc_tpb_read_committed, (char)isc_tpb_read_consistency,
		(char)isc_tpb_lock_timeout, 4, 30, 0, 0, 0
	};
	const std::string& isc_tpb =
		opts.noWait   ? isc_tpb_nowaitMode :
		opts.readOnly ? isc_tpb_readOnlyMode :
		                isc_tpb_waitMode;

	isc_db_handle pDatabase = m_pDatabase;
	isc_tr_handle pTransaction = 0;

	int nReturn = m_pInterface->GetIscStartTransaction()(
		*(ISC_STATUS_ARRAY*)m_pStatus, &pTransaction, 1, &pDatabase,
		isc_tpb.size(), isc_tpb.c_str());

	m_pDatabase = pDatabase;

	if (nReturn != 0)
	{
		InterpretErrorCodes();
		ThrowDatabaseException();
		return;
	}

	m_pTransaction = pTransaction;
}

void ibDatabaseLayerFirebird::DoCommit()
{
	ResetErrorCodes();

	if (!m_pDatabase || !m_pTransaction)
		return;

	isc_tr_handle pTransaction = m_pTransaction;
	int nReturn = m_pInterface->GetIscCommitTransaction()(*(ISC_STATUS_ARRAY*)m_pStatus, &pTransaction);
	// Whether the commit succeeded or not, FB invalidates the handle —
	// drop our copy so a stray DoRollBack / DoCommit can't double-free.
	m_pTransaction = 0;
	if (nReturn != 0)
	{
		InterpretErrorCodes();
		ThrowDatabaseException();
	}
}

void ibDatabaseLayerFirebird::DoRollBack()
{
	ResetErrorCodes();

	if (!m_pDatabase || !m_pTransaction)
		return;

	isc_tr_handle pTransaction = m_pTransaction;
	int nReturn = m_pInterface->GetIscRollbackTransaction()(*(ISC_STATUS_ARRAY*)m_pStatus, &pTransaction);
	m_pTransaction = 0;
	if (nReturn != 0)
	{
		InterpretErrorCodes();
		ThrowDatabaseException();
	}
}

// IsActiveTransaction inherits the base-class default (m_txDepth > 0) —
// the counter on the base is the source of truth and matches the
// drivers that don't have a native handle to probe.

// --- Row-level pessimistic locks -----------------------------------------

bool ibDatabaseLayerFirebird::HoldRowLocks(const wxString& tableName,
                                          const wxString& pkColumn,
                                          const std::vector<wxString>& pkValues)
{
	// Outer TX would hijack the row-lock semantics: BeginTransaction
	// below only bumps the depth counter, the SELECT WITH LOCK then
	// runs inside the caller's TX and the locks live until *that* TX
	// commits — not until ReleaseRowLocks(). Refuse rather than silently
	// produce wrong cluster-coordination behaviour.
	if (IsActiveTransaction() && !m_rowLocksHeld)
		return false;

	// A previous hold must be committed before we stack a fresh one —
	// otherwise two TXs would both target sys_session and their commits
	// would interleave in unhelpful ways.
	if (m_rowLocksHeld) {
		try { Commit(); } catch (...) {
			try { RollBack(); } catch (...) {}
		}
		m_rowLocksHeld = false;
	}

	if (pkValues.empty()) return true;

	try {
		BeginTransaction();
	}
	catch (...) {
		return false;
	}

	// Build the IN-list with placeholders so driver escapes values (guids
	// are trusted, but use the same code path as the rest of sys_session).
	wxString sql = wxT("SELECT ") + pkColumn + wxT(" FROM ") + tableName + wxT(" WHERE ") + pkColumn + wxT(" IN (");
	for (std::size_t i = 0; i < pkValues.size(); ++i) {
		if (i) sql += wxT(",");
		sql += wxT("?");
	}
	sql += wxT(") WITH LOCK");

	ibPreparedStatement* stmt = DoPrepareStatement(sql);
	if (!stmt) {
		try { RollBack(); } catch (...) {}
		return false;
	}

	for (std::size_t i = 0; i < pkValues.size(); ++i)
		stmt->SetParamString(int(i + 1), pkValues[i]);

	int lockedCount = 0;
	bool sqlOk = false;
	try {
		ibDatabaseResultSet* rs = stmt->RunQueryWithResults();
		if (rs) {
			while (rs->Next()) ++lockedCount;
			rs->Close();
			CloseResultSet(rs);
		}
		sqlOk = true;
	}
	catch (...) {
		sqlOk = false;
	}
	CloseStatement(stmt);

	if (!sqlOk || lockedCount != int(pkValues.size())) {
		try { RollBack(); } catch (...) {}
		return false;
	}

	m_rowLocksHeld = true;
	return true;
}

void ibDatabaseLayerFirebird::ReleaseRowLocks()
{
	if (!m_rowLocksHeld) return;
	try {
		Commit();
	}
	catch (...) {
		// Commit failed — TX may still be open and holding locks.
		// Try a rollback so the cluster coordination row isn't left
		// pinned by a dead-but-uncommitted TX of ours.
		try { RollBack(); } catch (...) {}
	}
	m_rowLocksHeld = false;
}

bool ibDatabaseLayerFirebird::TryProbeRowLock(const wxString& tableName,
                                              const wxString& pkColumn,
                                              const wxString& pkValue)
{
	// Probing inside an outer TX is meaningless: the inner Begin only
	// bumps the depth counter, the SELECT WITH LOCK then runs in the
	// caller's wait-mode TX and would block instead of failing fast.
	// Caller would also poison its own TX on conflict. Refuse.
	if (IsActiveTransaction())
		return false;

	// NOWAIT transaction so contention surfaces as an exception on
	// RunQueryWithResults — no sit-and-wait.
	try {
		BeginTransaction({ /*.noWait=*/true });
	}
	catch (...) {
		return false;
	}

	wxString sql = wxT("SELECT ") + pkColumn + wxT(" FROM ") + tableName + wxT(" WHERE ") + pkColumn + wxT(" = ? WITH LOCK");
	ibPreparedStatement* stmt = DoPrepareStatement(sql);
	bool gotLock = false;
	if (stmt) {
		stmt->SetParamString(1, pkValue);
		try {
			ibDatabaseResultSet* rs = stmt->RunQueryWithResults();
			if (rs) {
				// Row exists AND we successfully took its lock → no other
				// connection holds it (NOWAIT would have thrown otherwise).
				if (rs->Next()) gotLock = true;
				rs->Close();
				CloseResultSet(rs);
			}
		}
		catch (...) {
			// Lock conflict (owner still alive on another connection) or
			// transient DB error — either way, don't touch the row.
			gotLock = false;
		}
		CloseStatement(stmt);
	}

	// Always release — probe must never keep a lock outliving the call.
	try { RollBack(); } catch (...) {}
	return gotLock;
}

// query database
int ibDatabaseLayerFirebird::DoRunQuery(const wxString& strQuery, bool bParseQuery)
{
	ResetErrorCodes();
	if (m_pDatabase != 0)
	{
		wxCharBuffer sqlDebugBuffer = ConvertToUnicodeStream(strQuery);
#ifdef DEBUG
		wxLogDebug(wxT("Running query: \"%s\"\n"), (const char*)sqlDebugBuffer);
#endif // !DEBUG
		wxArrayString QueryArray;
		if (bParseQuery)
			QueryArray = ParseQueries(strQuery);
		else
			QueryArray.push_back(strQuery);

		wxArrayString::iterator start = QueryArray.begin();
		wxArrayString::iterator stop = QueryArray.end();

		long rows = 1;
		if (QueryArray.size() > 0)
		{
			bool bQuickieTransaction = false;

			if (m_pTransaction == 0)
			{
				// If there's no transaction is progress, run this as a quick one-timer transaction
				bQuickieTransaction = true;
			}

			if (bQuickieTransaction)
			{
				BeginTransaction();
				if (GetErrorCode() != DATABASE_LAYER_OK)
				{
					wxLogError(wxT("Unable to start transaction"));
					ThrowDatabaseException();
					return DATABASE_LAYER_QUERY_RESULT_ERROR;
				}
			}

			while (start != stop)
			{
				wxCharBuffer sqlBuffer = ConvertToUnicodeStream(*start);
				isc_db_handle pDatabase = m_pDatabase;
				isc_tr_handle pTransaction = m_pTransaction;
				int nReturn = m_pInterface->GetIscDsqlExecuteImmediate()(*(ISC_STATUS_ARRAY*)m_pStatus, &pDatabase, &pTransaction, GetEncodedStreamLength(*start), (char*)(const char*)sqlBuffer, SQL_DIALECT_CURRENT, NULL);
				m_pDatabase = pDatabase;
				m_pTransaction = pTransaction;
				if (nReturn != 0)
				{
					InterpretErrorCodes();
					// Manually rollback so any error messages from the
					// rollback don't clobber the original failure.
					isc_tr_handle pTr = m_pTransaction;
					m_pInterface->GetIscRollbackTransaction()(*(ISC_STATUS_ARRAY*)m_pStatus, &pTr);
					m_pTransaction = 0;

					ThrowDatabaseException();
					return DATABASE_LAYER_QUERY_RESULT_ERROR;
				}
				start++;
			}

			if (bQuickieTransaction)
			{
				Commit();
				if (GetErrorCode() != DATABASE_LAYER_OK)
				{
					ThrowDatabaseException();
					return DATABASE_LAYER_QUERY_RESULT_ERROR;
				}
			}
		}

		return rows;
	}
	else
	{
		wxLogError(wxT("Database handle is NULL"));
		return DATABASE_LAYER_QUERY_RESULT_ERROR;
	}
}

ibDatabaseResultSet* ibDatabaseLayerFirebird::DoRunQueryWithResults(const wxString& strQuery)
{
	ResetErrorCodes();
	if (m_pDatabase != 0)
	{
		wxCharBuffer sqlDebugBuffer = ConvertToUnicodeStream(strQuery);
#if DEBUG 
		wxLogDebug(wxT("Running query: \"%s\""), (const char*)sqlDebugBuffer);
#endif
		wxArrayString QueryArray = ParseQueries(strQuery);

		if (QueryArray.size() > 0)
		{
			bool bQuickieTransaction = false;

			if (m_pTransaction == 0)
			{
				// If there's no transaction is progress, run this as a quick one-timer transaction
				bQuickieTransaction = true;
			}

			if (QueryArray.size() > 1)
			{
				if (bQuickieTransaction)
				{
					BeginTransaction();
					if (GetErrorCode() != DATABASE_LAYER_OK)
					{
						wxLogError(wxT("Unable to start transaction"));
						ThrowDatabaseException();
						return NULL;
					}
				}

				// Assume that only the last statement in the array returns the result set
				for (unsigned int i = 0; i < QueryArray.size() - 1; i++)
				{
					DoRunQuery(QueryArray[i], false);
					if (GetErrorCode() != DATABASE_LAYER_OK)
					{
						ThrowDatabaseException();
						return NULL;
					}
				}

				// Now commit all the previous queries before calling the query that returns a result set
				if (bQuickieTransaction)
				{
					Commit();
					if (GetErrorCode() != DATABASE_LAYER_OK)
					{
						ThrowDatabaseException();
						return NULL;
					}
				}
			} // End check if there are more than one query in the array

			isc_tr_handle pQueryTransaction = NULL;
			bool bManageTransaction = false;
			if (bQuickieTransaction)
			{
				bManageTransaction = true;

				std::string tpbBuffer;
				{
					//tpbBuffer.push_back(isc_tpb_lock_timeout);
					//tpbBuffer.push_back(60);
				}

				isc_db_handle pDatabase = m_pDatabase;
				int nReturn = m_pInterface->GetIscStartTransaction()(*(ISC_STATUS_ARRAY*)m_pStatus, &pQueryTransaction, 1, &pDatabase, tpbBuffer.size(), tpbBuffer.data());
				m_pDatabase = pDatabase;
				if (nReturn != 0)
				{
					InterpretErrorCodes();
					ThrowDatabaseException();
				}
			}
			else
			{
				pQueryTransaction = m_pTransaction;
			}

			isc_stmt_handle pStatement = NULL;
			isc_db_handle pDatabase = m_pDatabase;
			int nReturn = m_pInterface->GetIscDsqlAllocateStatement()(*(ISC_STATUS_ARRAY*)m_pStatus, &pDatabase, &pStatement);
			m_pDatabase = pDatabase;
			if (nReturn != 0)
			{
				InterpretErrorCodes();

				// Manually try to rollback the transaction rather than calling the member RollBack function
				//  so that we can ignore the error messages
				m_pInterface->GetIscRollbackTransaction()(*(ISC_STATUS_ARRAY*)m_pStatus, &pQueryTransaction);

				ThrowDatabaseException();
				return NULL;
			}

			wxCharBuffer sqlBuffer = ConvertToUnicodeStream(QueryArray[QueryArray.size() - 1]);
			nReturn = m_pInterface->GetIscDsqlPrepare()(*(ISC_STATUS_ARRAY*)m_pStatus, &pQueryTransaction, &pStatement, 0, (char*)(const char*)sqlBuffer, SQL_DIALECT_CURRENT, NULL);
			if (nReturn != 0)
			{
				InterpretErrorCodes();

				// Manually try to rollback the transaction rather than calling the member RollBack function
				//  so that we can ignore the error messages
				m_pInterface->GetIscRollbackTransaction()(*(ISC_STATUS_ARRAY*)m_pStatus, &pQueryTransaction);

				ThrowDatabaseException();
				return NULL;
			}

			//--------------------------------------------------------------

			XSQLDA* pOutputSqlda = (XSQLDA*)malloc(XSQLDA_LENGTH(1));
			pOutputSqlda->sqln = 1;
			pOutputSqlda->version = SQLDA_VERSION1;

			// Make sure that we have enough space allocated for the result set
			nReturn = m_pInterface->GetIscDsqlDescribe()(*(ISC_STATUS_ARRAY*)m_pStatus, &pStatement, SQL_DIALECT_CURRENT, pOutputSqlda);
			if (nReturn != 0)
			{
				free(pOutputSqlda);
				InterpretErrorCodes();

				// Manually try to rollback the transaction rather than calling the member RollBack function
				//  so that we can ignore the error messages
				m_pInterface->GetIscRollbackTransaction()(*(ISC_STATUS_ARRAY*)m_pStatus, &pQueryTransaction);

				ThrowDatabaseException();
				return NULL;
			}

			if (pOutputSqlda->sqld > pOutputSqlda->sqln)
			{
				int nColumns = pOutputSqlda->sqld;
				free(pOutputSqlda);
				pOutputSqlda = (XSQLDA*)malloc(XSQLDA_LENGTH(nColumns));
				pOutputSqlda->sqln = nColumns;
				pOutputSqlda->version = SQLDA_VERSION1;
				nReturn = m_pInterface->GetIscDsqlDescribe()(*(ISC_STATUS_ARRAY*)m_pStatus, &pStatement, SQL_DIALECT_CURRENT, pOutputSqlda);
				if (nReturn != 0)
				{
					free(pOutputSqlda);
					InterpretErrorCodes();

					// Manually try to rollback the transaction rather than calling the member RollBack function
					//  so that we can ignore the error messages
					m_pInterface->GetIscRollbackTransaction()(*(ISC_STATUS_ARRAY*)m_pStatus, &pQueryTransaction);

					ThrowDatabaseException();
					return NULL;
				}
			}

			// Create the result set object
			ibDatabaseResultSetFirebird* pResultSet = new ibDatabaseResultSetFirebird(m_pInterface, m_pDatabase, pQueryTransaction, pStatement, pOutputSqlda, true, bManageTransaction);
			pResultSet->SetEncoding(GetEncoding());
			if (pResultSet->GetErrorCode() != DATABASE_LAYER_OK)
			{
				SetErrorCode(pResultSet->GetErrorCode());
				SetErrorMessage(pResultSet->GetErrorMessage());

				// Manually try to rollback the transaction rather than calling the member RollBack function
				//  so that we can ignore the error messages
				m_pInterface->GetIscRollbackTransaction()(*(ISC_STATUS_ARRAY*)m_pStatus, &pQueryTransaction);

				// Wrap the result set deletion in try/catch block if using exceptions.
				//We want to make sure the original error gets to the user
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
				try
				{
#endif
					delete pResultSet;
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
				}
				catch (ibDatabaseLayerException& e)
				{
				}
#endif

				ThrowDatabaseException();
			}

			// Now execute the SQL
			nReturn = m_pInterface->GetIscDsqlExecute()(*(ISC_STATUS_ARRAY*)m_pStatus, &pQueryTransaction, &pStatement, SQL_DIALECT_CURRENT, NULL);
			if (nReturn != 0)
			{
				InterpretErrorCodes();

				// Manually try to rollback the transaction rather than calling the member RollBack function
				//  so that we can ignore the error messages
				m_pInterface->GetIscRollbackTransaction()(*(ISC_STATUS_ARRAY*)m_pStatus, &pQueryTransaction);

				// Wrap the result set deletion in try/catch block if using exceptions.
				//  We want to make sure the isc_dsql_execute error gets to the user
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
				try
				{
#endif
					delete pResultSet;
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
				}
				catch (ibDatabaseLayerException& e)
				{
				}
#endif

				ThrowDatabaseException();
				return NULL;
			}

			//--------------------------------------------------------------

			LogResultSetForCleanup(pResultSet);
			return pResultSet;
		}
		else
			return NULL;
	}
	else
	{
		wxLogError(wxT("Database handle is NULL"));
		return NULL;
	}
}

ibPreparedStatement* ibDatabaseLayerFirebird::DoPrepareStatement(const wxString& strQuery)
{
	ResetErrorCodes();

	ibPreparedStatementFirebird* pStatement = ibPreparedStatementFirebird::CreateStatement(m_pInterface, m_pDatabase, m_pTransaction, strQuery, GetEncoding());
	if (pStatement && (pStatement->GetErrorCode() != DATABASE_LAYER_OK))
	{
		SetErrorCode(pStatement->GetErrorCode());
		SetErrorMessage(pStatement->GetErrorMessage());
		wxDELETE(pStatement); // This sets the pointer to NULL after deleting it

		ThrowDatabaseException();
		return NULL;
	}

	LogStatementForCleanup(pStatement);
	return pStatement;
}

bool ibDatabaseLayerFirebird::TableExists(const wxString& table)
{
	// Initialize variables
	bool bReturn = false;
	// Keep these variables outside of scope so that we can clean them up
	//  in case of an error
	ibPreparedStatement* pStatement = NULL;
	ibDatabaseResultSet* pResult = NULL;

#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	try
	{
#endif
		wxString tableUpperCase = table.Upper();
		wxString query = wxT("SELECT COUNT(*) FROM RDB$RELATIONS WHERE RDB$SYSTEM_FLAG=0 AND RDB$VIEW_BLR IS NULL AND RDB$RELATION_NAME=?;");
		pStatement = DoPrepareStatement(query);
		if (pStatement)
		{
			pStatement->SetParamString(1, tableUpperCase);
			pResult = pStatement->ExecuteQuery();
			if (pResult)
			{
				if (pResult->Next())
				{
					if (pResult->GetResultInt(1) != 0)
					{
						bReturn = true;
					}
				}
			}
		}
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	}
	catch (ibDatabaseLayerException& e)
	{
		if (pResult != NULL)
		{
			CloseResultSet(pResult);
			pResult = NULL;
		}

		if (pStatement != NULL)
		{
			CloseStatement(pStatement);
			pStatement = NULL;
		}

		throw e;
		}
#endif

	if (pResult != NULL)
	{
		CloseResultSet(pResult);
		pResult = NULL;
	}

	if (pStatement != NULL)
	{
		CloseStatement(pStatement);
		pStatement = NULL;
	}

	return bReturn;
	}

bool ibDatabaseLayerFirebird::ViewExists(const wxString& view)
{
	// Initialize variables
	bool bReturn = false;
	// Keep these variables outside of scope so that we can clean them up
	//  in case of an error
	ibPreparedStatement* pStatement = NULL;
	ibDatabaseResultSet* pResult = NULL;

#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	try
	{
#endif
		wxString viewUpperCase = view.Upper();
		wxString query = wxT("SELECT COUNT(*) FROM RDB$RELATIONS WHERE RDB$SYSTEM_FLAG=0 AND RDB$VIEW_BLR IS NOT NULL AND RDB$RELATION_NAME=?;");
		pStatement = DoPrepareStatement(query);
		if (pStatement)
		{
			pStatement->SetParamString(1, viewUpperCase);
			pResult = pStatement->ExecuteQuery();
			if (pResult)
			{
				if (pResult->Next())
				{
					if (pResult->GetResultInt(1) != 0)
					{
						bReturn = true;
					}
				}
			}
		}
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	}
	catch (ibDatabaseLayerException& e)
	{
		if (pResult != NULL)
		{
			CloseResultSet(pResult);
			pResult = NULL;
		}

		if (pStatement != NULL)
		{
			CloseStatement(pStatement);
			pStatement = NULL;
		}

		throw e;
		}
#endif

	if (pResult != NULL)
	{
		CloseResultSet(pResult);
		pResult = NULL;
	}

	if (pStatement != NULL)
	{
		CloseStatement(pStatement);
		pStatement = NULL;
	}

	return bReturn;
	}

wxArrayString ibDatabaseLayerFirebird::GetTables()
{
	wxArrayString returnArray;

	ibDatabaseResultSet* pResult = NULL;
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	try
	{
#endif
		wxString query = wxT("SELECT RDB$RELATION_NAME FROM RDB$RELATIONS WHERE RDB$SYSTEM_FLAG=0 AND RDB$VIEW_BLR IS NULL");
		pResult = ExecuteQuery(query);

		while (pResult->Next())
		{
			returnArray.Add(pResult->GetResultString(1).Trim());
		}
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	}
	catch (ibDatabaseLayerException& e)
	{
		if (pResult != NULL)
		{
			CloseResultSet(pResult);
			pResult = NULL;
		}

		throw e;
		}
#endif

	if (pResult != NULL)
	{
		CloseResultSet(pResult);
		pResult = NULL;
	}

	return returnArray;
	}

wxArrayString ibDatabaseLayerFirebird::GetViews()
{
	wxArrayString returnArray;

	ibDatabaseResultSet* pResult = NULL;
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	try
	{
#endif
		wxString query = wxT("SELECT RDB$RELATION_NAME FROM RDB$RELATIONS WHERE RDB$SYSTEM_FLAG=0 AND RDB$VIEW_BLR IS NOT NULL");
		pResult = ExecuteQuery(query);

		while (pResult->Next())
		{
			returnArray.Add(pResult->GetResultString(1).Trim());
		}
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	}
	catch (ibDatabaseLayerException& e)
	{
		if (pResult != NULL)
		{
			CloseResultSet(pResult);
			pResult = NULL;
		}

		throw e;
		}
#endif

	if (pResult != NULL)
	{
		CloseResultSet(pResult);
		pResult = NULL;
	}

	return returnArray;
	}

wxArrayString ibDatabaseLayerFirebird::GetColumns(const wxString& table)
{
	// Initialize variables
	wxArrayString returnArray;
	// Keep these variables outside of scope so that we can clean them up
	//  in case of an error
	ibPreparedStatement* pStatement = NULL;
	ibDatabaseResultSet* pResult = NULL;

#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	try
	{
#endif
		wxString tableUpperCase = table.Upper();
		wxString query = wxT("SELECT RDB$FIELD_NAME FROM RDB$RELATION_FIELDS WHERE RDB$RELATION_NAME=?;");
		pStatement = DoPrepareStatement(query);
		if (pStatement)
		{
			pStatement->SetParamString(1, tableUpperCase);
			pResult = pStatement->ExecuteQuery();
			if (pResult)
			{
				while (pResult->Next())
				{
					returnArray.Add(pResult->GetResultString(1).Trim());
				}
			}
		}
#if _USE_DATABASE_LAYER_EXCEPTIONS == 1
	}
	catch (ibDatabaseLayerException& e)
	{
		if (pResult != NULL)
		{
			CloseResultSet(pResult);
			pResult = NULL;
		}

		if (pStatement != NULL)
		{
			CloseStatement(pStatement);
			pStatement = NULL;
		}

		throw e;
		}
#endif

	if (pResult != NULL)
	{
		CloseResultSet(pResult);
		pResult = NULL;
	}

	if (pStatement != NULL)
	{
		CloseStatement(pStatement);
		pStatement = NULL;
	}

	return returnArray;
	}

int ibDatabaseLayerFirebird::TranslateErrorCode(int nCode)
{
	// Ultimately, this will probably be a map of Firebird database error code values to ibDatabaseLayer values
	// For now though, we'll just return the original error code
	return nCode;
}

//wxString ibDatabaseLayerFirebird::TranslateErrorCodeToString(ibInterfaceFirebird* pInterface, int nCode, ISC_STATUS_ARRAY status)
wxString ibDatabaseLayerFirebird::TranslateErrorCodeToString(ibInterfaceFirebird* pInterface, int nCode, void* status)
{
	char szError[512];
	wxString strReturn;

	if (nCode > -901) // Error codes less than -900 indicate that it wasn't a SQL error but an ibase system error
	{
		long* pVector = (long*)status;
		pInterface->GetFbInterpret()(szError, 512, (const ISC_STATUS**)&pVector);

		strReturn = wxString::Format(wxT("%s\n"), szError);
		while (pInterface->GetFbInterpret()(szError, 512, (const ISC_STATUS**)&pVector))
		{
			strReturn += wxString::Format(wxT("%s\n"), szError);
		}

		pInterface->GetIscSqlInterprete()(nCode, szError, sizeof(szError));
		strReturn += wxString::Format(wxT("%s\n"), szError);
	}
	else
	{
		pInterface->GetIscSqlInterprete()(nCode, szError, sizeof(szError));
		wxCharBuffer systemEncoding = wxLocale::GetSystemEncodingName().mb_str();
		strReturn = ibDatabaseStringConverter::ConvertFromUnicodeStream(szError, (const char*)systemEncoding);
	}

	return strReturn;
}

void ibDatabaseLayerFirebird::InterpretErrorCodes()
{
	//wxLogDebug(wxT("ibDatabaseLayerFirebird::InterpretErrorCodes()"));

	long nSqlCode = m_pInterface->GetIscSqlcode()(*(ISC_STATUS_ARRAY*)m_pStatus);
	SetErrorMessage(ibDatabaseLayerFirebird::TranslateErrorCodeToString(m_pInterface, nSqlCode, *(ISC_STATUS_ARRAY*)m_pStatus));
	if (nSqlCode < -900)  // Error codes less than -900 indicate that it wasn't a SQL error but an ibase system error
	{
		SetErrorCode(ibDatabaseLayerFirebird::TranslateErrorCode(*((ISC_STATUS_ARRAY*)m_pStatus)[1]));
	}
	else
	{
		SetErrorCode(ibDatabaseLayerFirebird::TranslateErrorCode(nSqlCode));
	}
}

bool ibDatabaseLayerFirebird::IsAvailable()
{
	bool bAvailable = false;
	ibInterfaceFirebird* pInterface = new ibInterfaceFirebird();
	bAvailable = pInterface && pInterface->Init();
	wxDELETE(pInterface);
	return bAvailable;
}

