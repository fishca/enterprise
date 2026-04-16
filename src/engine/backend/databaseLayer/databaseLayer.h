#ifndef __DATABASE_LAYER_H__
#define __DATABASE_LAYER_H__

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/hashset.h>
#include <wx/arrstr.h>
#include <wx/variant.h>

#include "databaseLayerDef.h"
#include "databaseErrorReporter.h"
#include "databaseStringConverter.h"
#include "databaseResultSet.h"
#include "preparedStatement.h"

WX_DECLARE_HASH_SET(ibDatabaseResultSet*, wxPointerHash, wxPointerEqual, DatabaseResultSetHashSet);
WX_DECLARE_HASH_SET(ibPreparedStatement*, wxPointerHash, wxPointerEqual, DatabaseStatementHashSet);

class BACKEND_API ibDatabaseLayer : public ibDatabaseErrorReporter, public ibDatabaseStringConverter
{
public:
	/// Constructor
	ibDatabaseLayer();

	/// Destructor
	virtual ~ibDatabaseLayer();

	// Open database
	virtual bool Open(const wxString& strDatabase) = 0;

	/// close database  
	virtual bool Close() = 0;

	/// Is the connection to the database open?
	virtual bool IsOpen() = 0;

	/// clone database  
	virtual ibDatabaseLayer *Clone() = 0;

	// transaction support
	
	/// Begin a transaction
	virtual void BeginTransaction() = 0;
	
	/// Commit the current transaction
	virtual void Commit() = 0;
	
	/// Rollback the current transaction
	virtual void RollBack() = 0;

	/// Is the transaction to the database active?
	virtual bool IsActiveTransaction() = 0;

	// Define formatted run query 

	/// Run an insert, update, or delete query on the database
	WX_DEFINE_VARARG_FUNC(int, RunQuery, 1, (const wxFormatString&),
		DoRunQueryWchar, DoRunQueryUtf8);

	///  Run a select query on the database
	WX_DEFINE_VARARG_FUNC(ibDatabaseResultSet*, RunQueryWithResults, 1, (const wxFormatString&),
		DoRunQueryWithResultsWchar, DoRunQueryWithResultsUtf8);

	/// ibPreparedStatement support
	WX_DEFINE_VARARG_FUNC(ibPreparedStatement*, PrepareStatement, 1, (const wxFormatString&),
		DoPrepareStatementWchar, DoPrepareStatementUtf8);

	// function names more consistent with JDBC and wxSQLite3
	// these just provide wrappers for existing functions

	/// See RunQuery
	WX_DEFINE_VARARG_FUNC(int, ExecuteUpdate, 1, (const wxFormatString&),
		DoRunQueryWchar, DoRunQueryUtf8);

	/// See RunQueryWithResults
	WX_DEFINE_VARARG_FUNC(ibDatabaseResultSet*, ExecuteQuery, 1, (const wxFormatString&),
		DoRunQueryWithResultsWchar, DoRunQueryWithResultsUtf8);

	/// Close a result set returned by the database or a prepared statement previously
	virtual bool CloseResultSet(ibDatabaseResultSet*& pResultSet);

	// ibPreparedStatement support

	/// Close a prepared statement previously prepared by the database
	virtual bool CloseStatement(ibPreparedStatement*& pStatement);

	// Database schema API contributed by M. Szeftel (author of wxActiveRecordGenerator)
	/// Check for the existence of a table by name
	virtual bool TableExists(const wxString& table) = 0;
	
	/// Check for the existence of a view by name
	virtual bool ViewExists(const wxString& view) = 0;
	
	/// Retrieve all table names
	virtual wxArrayString GetTables() = 0;
	
	/// Retrieve all view names
	virtual wxArrayString GetViews() = 0;

	/// Retrieve all column names for a table
	virtual wxArrayString GetColumns(const wxString& table) = 0;

	// Database single result retrieval API contributed by Guru Kathiresan
	/// With the GetSingleResultX API, two additional exception types are thrown:
	///   DATABASE_LAYER_NO_ROWS_FOUND - No database rows were returned
	///   DATABASE_LAYER_NON_UNIQUE_RESULTSET - More than one database row was returned

	/// Retrieve a single integer value from a query
	/// If multiple records are returned from the query, a DATABASE_LAYER_NON_UNIQUE_RESULTSET exception
	///  is thrown unless bRequireUniqueResult is false
	virtual int GetSingleResultInt(const wxString& strSQL, int nField, bool bRequireUniqueResult = true);
	virtual int GetSingleResultInt(const wxString& strSQL, const wxString& strField, bool bRequireUniqueResult = true);

	/// Retrieve a single string value from a query
	/// If multiple records are returned from the query, a DATABASE_LAYER_NON_UNIQUE_RESULTSET exception
	///  is thrown unless bRequireUniqueResult is false
	virtual wxString GetSingleResultString(const wxString& strSQL, int nField, bool bRequireUniqueResult = true);
	virtual wxString GetSingleResultString(const wxString& strSQL, const wxString& strField, bool bRequireUniqueResult = true);

	/// Retrieve a single long value from a query
	/// If multiple records are returned from the query, a DATABASE_LAYER_NON_UNIQUE_RESULTSET exception
	///  is thrown unless bRequireUniqueResult is false
	virtual long GetSingleResultLong(const wxString& strSQL, int nField, bool bRequireUniqueResult = true);
	virtual long GetSingleResultLong(const wxString& strSQL, const wxString& strField, bool bRequireUniqueResult = true);

	/// Retrieve a single bool value from a query
	/// If multiple records are returned from the query, a DATABASE_LAYER_NON_UNIQUE_RESULTSET exception
	///  is thrown unless bRequireUniqueResult is false
	virtual bool GetSingleResultBool(const wxString& strSQL, int nField, bool bRequireUniqueResult = true);
	virtual bool GetSingleResultBool(const wxString& strSQL, const wxString& strField, bool bRequireUniqueResult = true);

	/// Retrieve a single date/time value from a query
	/// If multiple records are returned from the query, a DATABASE_LAYER_NON_UNIQUE_RESULTSET exception
	///  is thrown unless bRequireUniqueResult is false
	virtual wxDateTime GetSingleResultDate(const wxString& strSQL, int nField, bool bRequireUniqueResult = true);
	virtual wxDateTime GetSingleResultDate(const wxString& strSQL, const wxString& strField, bool bRequireUniqueResult = true);

	/// Retrieve a single Blob value from a query
	/// If multiple records are returned from the query, a DATABASE_LAYER_NON_UNIQUE_RESULTSET exception
	///  is thrown unless bRequireUniqueResult is false
	virtual void* GetSingleResultBlob(const wxString& strSQL, int nField, wxMemoryBuffer& buffer, bool bRequireUniqueResult = true);
	virtual void* GetSingleResultBlob(const wxString& strSQL, const wxString& strField, wxMemoryBuffer& buffer, bool bRequireUniqueResult = true);

	/// Retrieve a single double value from a query
	/// If multiple records are returned from the query, a DATABASE_LAYER_NON_UNIQUE_RESULTSET exception
	///  is thrown unless bRequireUniqueResult is false
	virtual double GetSingleResultDouble(const wxString& strSQL, int nField, bool bRequireUniqueResult = true);
	virtual double GetSingleResultDouble(const wxString& strSQL, const wxString& strField, bool bRequireUniqueResult = true);

	/// Retrieve a single number value from a query
	/// If multiple records are returned from the query, a DATABASE_LAYER_NON_UNIQUE_RESULTSET exception
	///  is thrown unless bRequireUniqueResult is false
	virtual ibNumber GetSingleResultNumber(const wxString& strSQL, int nField, bool bRequireUniqueResult = true);
	virtual ibNumber GetSingleResultNumber(const wxString& strSQL, const wxString& strField, bool bRequireUniqueResult = true);

	/// Retrieve all the values of one field in a result set
	virtual wxArrayInt GetResultsArrayInt(const wxString& strSQL, int nField);
	virtual wxArrayInt GetResultsArrayInt(const wxString& strSQL, const wxString& Field);

	virtual wxArrayString GetResultsArrayString(const wxString& strSQL, int nField);
	virtual wxArrayString GetResultsArrayString(const wxString& strSQL, const wxString& Field);

	virtual wxArrayLong GetResultsArrayLong(const wxString& strSQL, int nField);
	virtual wxArrayLong GetResultsArrayLong(const wxString& strSQL, const wxString& Field);

#if wxCHECK_VERSION(2, 7, 0)
	virtual wxArrayDouble GetResultsArrayDouble(const wxString& strSQL, int nField);
	virtual wxArrayDouble GetResultsArrayDouble(const wxString& strSQL, const wxString& Field);
#endif

	virtual int GetDatabaseLayerType() const = 0;

	/// Close all result set objects that have been generated but not yet closed
	void CloseResultSets();

	/// Close all prepared statement objects that have been generated but not yet closed
	void CloseStatements();

protected:

	// query database

	/// Run an insert, update, or delete query on the database
	virtual int DoRunQuery(const wxString& strQuery, bool bParseQueries) = 0;

	/// Run a select query on the database
	virtual ibDatabaseResultSet* DoRunQueryWithResults(const wxString& strQuery) = 0;

	// prepared statement support

	/// Prepare a SQL statement which can be reused with different parameters
	virtual ibPreparedStatement* DoPrepareStatement(const wxString& strQuery) = 0;
	
protected:

	/// Add result set object pointer to the list for "garbage collection"
	void LogResultSetForCleanup(ibDatabaseResultSet* pResultSet) { m_ResultSets.insert(pResultSet); }
	/// Add prepared statement object pointer to the list for "garbage collection"
	void LogStatementForCleanup(ibPreparedStatement* pStatement) { m_Statements.insert(pStatement); }

private:

	int GetSingleResultInt(const wxString& strSQL, const wxVariant* field, bool bRequireUniqueResult = true);
	wxString GetSingleResultString(const wxString& strSQL, const wxVariant* field, bool bRequireUniqueResult = true);
	long GetSingleResultLong(const wxString& strSQL, const wxVariant* field, bool bRequireUniqueResult = true);
	bool GetSingleResultBool(const wxString& strSQL, const wxVariant* field, bool bRequireUniqueResult = true);
	wxDateTime GetSingleResultDate(const wxString& strSQL, const wxVariant* field, bool bRequireUniqueResult = true);
	void* GetSingleResultBlob(const wxString& strSQL, const wxVariant* field, wxMemoryBuffer& buffer, bool bRequireUniqueResult = true);
	double GetSingleResultDouble(const wxString& strSQL, const wxVariant* field, bool bRequireUniqueResult = true);
	ibNumber GetSingleResultNumber(const wxString& strSQL, const wxVariant* field, bool bRequireUniqueResult = true);
	wxArrayInt GetResultsArrayInt(const wxString& strSQL, const wxVariant* field);
	wxArrayString GetResultsArrayString(const wxString& strSQL, const wxVariant* field);
	wxArrayLong GetResultsArrayLong(const wxString& strSQL, const wxVariant* field);

#if wxCHECK_VERSION(2, 7, 0)
	wxArrayDouble GetResultsArrayDouble(const wxString& strSQL, const wxVariant* field);
#endif

	DatabaseResultSetHashSet m_ResultSets;
	DatabaseStatementHashSet m_Statements;

#if !wxUSE_UTF8_LOCALE_ONLY
	int DoRunQueryWchar(const wxChar* format, ...);
	ibDatabaseResultSet* DoRunQueryWithResultsWchar(const wxChar* format, ...);
	ibPreparedStatement* DoPrepareStatementWchar(const wxChar* format, ...);
#endif
#if wxUSE_UNICODE_UTF8
	int DoRunQueryUtf8(const wxChar* format, ...);
	ibDatabaseResultSet* DoRunQueryWithResultsUtf8(const wxChar* format, ...);
	ibPreparedStatement* DoPrepareStatementUtf8(const wxChar* format, ...);
#endif
};

#include <memory>

// RAII guard for an ibDatabaseResultSet* obtained from ibDatabaseLayer::RunQueryWithResults
// or ibPreparedStatement::RunQueryWithResults. Calls rs->Close() and db->CloseResultSet(rs)
// in the destructor so early returns and exceptions don't leak. The db parameter accepts
// either a raw ibDatabaseLayer* or a std::shared_ptr<ibDatabaseLayer> (the common form
// across OES — e.g. the db_query macro returns shared_ptr by value).
class BACKEND_API ibResultSetGuard {
public:
	ibResultSetGuard(ibDatabaseLayer* db, ibDatabaseResultSet* rs) : m_db(db), m_rs(rs) {}
	ibResultSetGuard(const std::shared_ptr<ibDatabaseLayer>& db, ibDatabaseResultSet* rs) : m_db(db.get()), m_rs(rs) {}
	~ibResultSetGuard() { reset(nullptr); }
	ibResultSetGuard(const ibResultSetGuard&) = delete;
	ibResultSetGuard& operator=(const ibResultSetGuard&) = delete;

	ibDatabaseResultSet* get() const { return m_rs; }
	ibDatabaseResultSet* operator->() const { return m_rs; }
	explicit operator bool() const { return m_rs != nullptr; }

	void reset(ibDatabaseResultSet* rs) {
		if (m_rs != nullptr && m_db != nullptr) {
			m_rs->Close();
			m_db->CloseResultSet(m_rs);
		}
		m_rs = rs;
	}

private:
	ibDatabaseLayer* m_db;
	ibDatabaseResultSet* m_rs;
};

// RAII guard for an ibPreparedStatement* obtained from ibDatabaseLayer::PrepareStatement.
// Calls db->CloseStatement(stmt) in the destructor. Same shared_ptr accommodation as above.
class BACKEND_API ibStatementGuard {
public:
	ibStatementGuard(ibDatabaseLayer* db, ibPreparedStatement* stmt) : m_db(db), m_stmt(stmt) {}
	ibStatementGuard(const std::shared_ptr<ibDatabaseLayer>& db, ibPreparedStatement* stmt) : m_db(db.get()), m_stmt(stmt) {}
	~ibStatementGuard() { reset(nullptr); }
	ibStatementGuard(const ibStatementGuard&) = delete;
	ibStatementGuard& operator=(const ibStatementGuard&) = delete;

	ibPreparedStatement* get() const { return m_stmt; }
	ibPreparedStatement* operator->() const { return m_stmt; }
	explicit operator bool() const { return m_stmt != nullptr; }

	void reset(ibPreparedStatement* stmt) {
		if (m_stmt != nullptr && m_db != nullptr) {
			m_db->CloseStatement(m_stmt);
		}
		m_stmt = stmt;
	}

private:
	ibDatabaseLayer* m_db;
	ibPreparedStatement* m_stmt;
};

#endif // __DATABASE_LAYER_H__

