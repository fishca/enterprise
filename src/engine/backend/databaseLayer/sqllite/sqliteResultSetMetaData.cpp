#include "sqliteResultSetMetaData.h"

/*
   SPECIAL NOTE:
	The sqlite3_column_* functions are 0-based, but the JDBC IResultSetMetaData
	functions are 1-based.  To be consistent with the 1-based JDBC (and also
	IPreparedStatement paramters, we are using a 1-based system here.)
 */

 // ctor
CSqliteResultSetMetaData::CSqliteResultSetMetaData(sqlite3_stmt* pStmt)
{
	m_pSqliteStatement = pStmt;
}

int CSqliteResultSetMetaData::GetColumnType(int i)
{
	int returnType = COLUMN_UNKNOWN;
	wxString columnTypeString = ConvertFromUnicodeStream(sqlite3_column_decltype(m_pSqliteStatement, i - 1));
	columnTypeString.MakeUpper();
	if (columnTypeString.IsSameAs(wxT("INTEGER")) || columnTypeString.IsSameAs(wxT("INT")) || columnTypeString.IsSameAs(wxT("LONG")))
		returnType = COLUMN_INTEGER;
	else if (columnTypeString.IsSameAs(wxT("STRING")) || columnTypeString.StartsWith(wxT("VARCHAR")))
		returnType = COLUMN_STRING;
	else if (columnTypeString.IsSameAs(wxT("DOUBLE")) || columnTypeString.IsSameAs(wxT("FLOAT")))
		returnType = COLUMN_DOUBLE;
	else if (columnTypeString.IsSameAs(wxT("BOOL")))
		returnType = COLUMN_BOOL;
	else if (columnTypeString.IsSameAs(wxT("BLOB")))
		returnType = COLUMN_BLOB;
	else if (columnTypeString.IsSameAs(wxT("DATE")) || columnTypeString.IsSameAs(wxT("DATETIME")) || columnTypeString.IsSameAs(wxT("TIMESTAMP")))
		returnType = COLUMN_DATE;
	else
		returnType = COLUMN_UNKNOWN;

	if (returnType == COLUMN_UNKNOWN)
	{
		int columnType = sqlite3_column_type(m_pSqliteStatement, i - 1);
		switch (columnType)
		{
		case SQLITE_INTEGER:
			puts("sqlite3_column_type returned SQLITE_INTEGER");
			returnType = COLUMN_INTEGER;
			break;
		case SQLITE_FLOAT:
			returnType = COLUMN_DOUBLE;
			break;
		case SQLITE_TEXT:
			returnType = COLUMN_STRING;
			break;
		case SQLITE_BLOB:
			returnType = COLUMN_BLOB;
			break;
		case SQLITE_NULL:
			returnType = COLUMN_NULL;
			break;
		default:
			returnType = COLUMN_UNKNOWN;
			break;
		};
	}

	return returnType;
}

int CSqliteResultSetMetaData::GetColumnSize(int i)
{
	return -1;
}

wxString CSqliteResultSetMetaData::GetColumnName(int i)
{
	wxString columnName = ConvertFromUnicodeStream(sqlite3_column_name(m_pSqliteStatement, i - 1));
	return columnName;
}

int CSqliteResultSetMetaData::GetColumnCount()
{
	return sqlite3_column_count(m_pSqliteStatement);
}


