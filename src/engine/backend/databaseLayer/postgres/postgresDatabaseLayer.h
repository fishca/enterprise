#ifndef __POSTGRESQL_DATABASE_LAYER_H__
#define __POSTGRESQL_DATABASE_LAYER_H__

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "backend/databaseLayer/databaseLayerDef.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/databaseLayer/preparedStatement.h"

#if _USE_DYNAMIC_DATABASE_LAYER_LINKING == 1
class ibInterfacePostgres;
#endif

class BACKEND_API ibDatabaseLayerPostgres : public ibDatabaseLayer
{
public:
	// Information that can be specified for a PostgreSQL database
	//  host or hostaddr
	//  port
	//  dbname
	//  user
	//  password
	// ctor
	ibDatabaseLayerPostgres();
	ibDatabaseLayerPostgres(const wxString& strDatabase);
	ibDatabaseLayerPostgres(const wxString& strServer, const wxString& strDatabase);
	ibDatabaseLayerPostgres(const wxString& strDatabase, const wxString& strUser, const wxString& strPassword);
	ibDatabaseLayerPostgres(const wxString& strServer, const wxString& strDatabase, const wxString& strUser, const wxString& strPassword);
	ibDatabaseLayerPostgres(const wxString& strServer, const wxString& strPort, const wxString& strDatabase, const wxString& strUser, const wxString& strPassword);
	ibDatabaseLayerPostgres(void* pDatabase) { m_pDatabase = pDatabase; }
	ibDatabaseLayerPostgres(const ibDatabaseLayerPostgres& src);

	// dtor
	virtual ~ibDatabaseLayerPostgres();

	// open database
	virtual bool Open();
	virtual bool Open(const wxString& strDatabase);
	virtual bool Open(const wxString& strServer, const wxString& strDatabase);
	virtual bool Open(const wxString& strDatabase, const wxString& strUser, const wxString& strPassword);
	virtual bool Open(const wxString& strServer, const wxString& strDatabase, const wxString& strUser, const wxString& strPassword);
	virtual bool Open(const wxString& strServer, const wxString& strPort, const wxString& strDatabase, const wxString& strUser, const wxString& strPassword);

	// close database
	virtual bool Close();

	// Is the connection to the database open?
	virtual bool IsOpen();

	/// clone database  
	virtual ibDatabaseLayer* Clone() { return new ibDatabaseLayerPostgres(*this); }

	// transaction support
	virtual void BeginTransaction(const ibTxOptions& opts = {});
	virtual void Commit();
	virtual void RollBack();

	// IsActiveTransaction uses the base-class default which reads the
	// shared `m_transaction_is_active` flag.

	// Pessimistic row-lock probe for ibSessionRegistry's designer-
	// exclusive policy. PG implementation: BEGIN → SELECT ... FOR
	// UPDATE NOWAIT → ROLLBACK. NOWAIT makes the probe fail-fast when
	// another connection holds the row, so a live owner surfaces as
	// a caught exception instead of a blocked probe thread.
	virtual bool TryProbeRowLock(const wxString& tableName,
		const wxString& pkColumn, const wxString& pkValue) override;

	// Database schema API contributed by M. Szeftel (author of wxActiveRecordGenerator)
	virtual bool DatabaseExists(const wxString& table);
	virtual bool TableExists(const wxString& table);
	virtual bool ViewExists(const wxString& view);

	virtual wxArrayString GetTables();
	virtual wxArrayString GetViews();
	virtual wxArrayString GetColumns(const wxString& table);

	virtual int GetDatabaseLayerType() const {
		return DATABASELAYER_POSTGRESQL;
	}

	static int TranslateErrorCode(int nCode);
	static bool IsAvailable();

protected:

	// query database
	virtual int DoRunQuery(const wxString& strQuery, bool bParseQuery);
	virtual ibDatabaseResultSet* DoRunQueryWithResults(const wxString& strQuery);

	// ibPreparedStatement support
	virtual ibPreparedStatement* DoPrepareStatement(const wxString& strQuery);

private:

	// m_transaction_is_active now lives on the base class — shared flag
	// read by ibDatabaseLayer::IsActiveTransaction.

#if _USE_DYNAMIC_DATABASE_LAYER_LINKING == 1
	ibInterfacePostgres* m_pInterface;
#endif
	wxString m_strServer;
	wxString m_strDatabase;
	wxString m_strUser;
	wxString m_strPassword;
	wxString m_strPort;

	void* m_pDatabase;
};

#endif // __POSTGRESQL_DATABASE_LAYER_H__

